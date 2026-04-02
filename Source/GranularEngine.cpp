#include "GranularEngine.h"

float Grain::getEnvelopeValue(float position) const
{
    // Simple Hann window envelope
    if (position <= 0.0f || position >= 1.0f)
        return 0.0f;
    
    return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * position));
}

GranularEngine::GranularEngine()
    : ProcessingModule("Granular", Type::Granular),
      random(std::random_device{}()),
      randomDist(0.0f, 1.0f)
{
    grains.resize(maxGrains);
}

GranularEngine::~GranularEngine() = default;

void GranularEngine::prepare(double sr, int blockSize, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // Max 60 second buffer at 48kHz
    int maxBufferSamples = (int)(60.0 * sampleRate);
    circularBuffer.prepare(numChannels, maxBufferSamples);
    circularBuffer.setLoopLength((int)(loopLengthMs * 0.001 * sampleRate));
    
    inputBuffer.setSize(numChannels, blockSize);
    
    setGrainSize(grainSizeMs);
    setDensity(density);
}

void GranularEngine::releaseResources()
{
    circularBuffer.clear();
}

void GranularEngine::setGrainSize(float ms)
{
    grainSizeMs = ms;
    grainSizeSamples = (int)(ms * 0.001 * sampleRate);
}

void GranularEngine::setDensity(int grainsPerBlock)
{
    density = juce::jlimit(1, maxGrains, grainsPerBlock);
    // Calculate interval between grains based on density
    if (density > 0)
        grainInterval = grainSizeSamples / (float)density;
}

void GranularEngine::setScatter(float scatterAmount)
{
    scatter = juce::jlimit(0.0f, 1.0f, scatterAmount);
}

void GranularEngine::setPitchShift(float ratio)
{
    pitchRatio = ratio;
}

void GranularEngine::setLoopLength(float ms)
{
    loopLengthMs = ms;
    int samples = (int)(ms * 0.001 * sampleRate);
    circularBuffer.setLoopLength(samples);
}

void GranularEngine::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    if (!enabled)
        circularBuffer.clear();
}

void GranularEngine::setOverdub(bool enabled)
{
    overdub = enabled;
}

void GranularEngine::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 1.0f, fb);
}

void GranularEngine::clearLoop()
{
    circularBuffer.clear();
}

void GranularEngine::triggerGrain()
{
    // Find inactive grain
    for (auto& grain : grains)
    {
        if (!grain.active)
        {
            grain.active = true;
            grain.length = grainSizeSamples;
            grain.pitchRatio = pitchRatio;
            grain.currentPosition = 0.0f;
            grain.amplitude = 0.8f;
            
            // Scatter determines randomness of start position
            int loopSamples = circularBuffer.getLoopLength();
            if (loopSamples > 0)
            {
                float scatterOffset = scatter * randomDist(random) * loopSamples;
                int writePos = circularBuffer.getWritePosition();
                grain.startPosition = (float)(writePos - grain.length - (int)scatterOffset);
                while (grain.startPosition < 0)
                    grain.startPosition += loopSamples;
            }
            else
            {
                grain.startPosition = 0;
            }
            
            // Stereo panning (slight variation)
            grain.pan = 0.3f + randomDist(random) * 0.4f;  // Slightly centered
            
            break;
        }
    }
}

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
        
    int numSamples = buffer.getNumSamples();
    
    // Store input for feedback/overdub mixing
    for (int ch = 0; ch < numChannels; ++ch)
        inputBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    
    // Clear output buffer (we'll accumulate grains)
    juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
    outputBuffer.clear();
    
    // Process sample by sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write to circular buffer with feedback if looping enabled
        if (loopEnabled)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float inputSample = inputBuffer.getSample(ch, sample);
                
                // If overdubbing, mix with existing loop
                // For simplicity, we just write the input directly
                // In a full implementation, we'd read-modify-write with feedback
                buffer.setSample(ch, sample, inputSample);
            }
            circularBuffer.write(buffer, sample, 1);
        }
        
        // Trigger new grains based on density
        samplesUntilNextGrain -= 1.0f;
        if (samplesUntilNextGrain <= 0.0f)
        {
            triggerGrain();
            samplesUntilNextGrain = grainInterval * (0.8f + randomDist(random) * 0.4f);
        }
        
        // Process active grains
        float leftSample = 0.0f;
        float rightSample = 0.0f;
        int activeCount = 0;
        
        for (auto& grain : grains)
        {
            if (!grain.active)
                continue;
            
            // Calculate position within grain (0-1)
            float grainProgress = grain.currentPosition / grain.length;
            
            if (grainProgress >= 1.0f)
            {
                grain.active = false;
                continue;
            }
            
            // Get envelope value
            float envelope = grain.getEnvelopeValue(grainProgress);
            
            // Read from circular buffer
            float readPos = grain.startPosition + grain.currentPosition * grain.pitchRatio;
            
            float grainLeft = circularBuffer.read(0, readPos) * envelope * grain.amplitude;
            float grainRight = (numChannels > 1) 
                ? circularBuffer.read(1, readPos) * envelope * grain.amplitude 
                : grainLeft;
            
            // Apply panning
            float leftGain = std::cos(grain.pan * juce::MathConstants<float>::halfPi);
            float rightGain = std::sin(grain.pan * juce::MathConstants<float>::halfPi);
            
            leftSample += grainLeft * leftGain;
            rightSample += grainRight * rightGain;
            
            // Advance grain
            grain.currentPosition += 1.0f;
            activeCount++;
        }
        
        // Soft limiter (simple tanh)
        leftSample = std::tanh(leftSample * 0.5f);
        rightSample = std::tanh(rightSample * 0.5f);
        
        outputBuffer.setSample(0, sample, leftSample);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, rightSample);
    }
    
    // Copy output back to main buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        buffer.copyFrom(ch, 0, outputBuffer, ch, 0, numSamples);
    }
}

void GranularEngine::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("grainSize", "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("density", "Density",
        juce::NormalisableRange<float>(1.0f, 50.0f, 1.0f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("scatter", "Scatter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 25.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("pitch", "Pitch",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("loopLength", "Loop Length",
        juce::NormalisableRange<float>(100.0f, 60000.0f, 100.0f), 4000.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("loopEnabled", "Loop", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("overdub", "Overdub", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>("feedback", "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 80.0f));
}

void GranularEngine::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "grainSize") setGrainSize(newValue);
    else if (parameterID == "density") setDensity((int)newValue);
    else if (parameterID == "scatter") setScatter(newValue / 100.0f);
    else if (parameterID == "pitch") setPitchShift(std::pow(2.0f, newValue / 12.0f));
    else if (parameterID == "loopLength") setLoopLength(newValue);
    else if (parameterID == "loopEnabled") setLoopEnabled(newValue > 0.5f);
    else if (parameterID == "overdub") setOverdub(newValue > 0.5f);
    else if (parameterID == "feedback") setFeedback(newValue / 100.0f);
}
