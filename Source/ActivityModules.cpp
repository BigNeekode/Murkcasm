#include "ActivityModules.h"
#include <juce_dsp/juce_dsp.h>

// ============================================
// CHOP MODULE IMPLEMENTATION
// ============================================
ChopModule::ChopModule()
    : ProcessingModule("Chop", Type::Activity),
      random(std::random_device{}())
{
}

ChopModule::~ChopModule() = default;

void ChopModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 5 second max buffer
    buffer.prepare(numChannels, (int)(5.0 * sampleRate));
    
    setSliceLength(sliceLengthMs);
    setGapLength(gapLengthMs);
}

void ChopModule::releaseResources()
{
    buffer.clear();
}

void ChopModule::setSliceLength(float ms)
{
    sliceLengthMs = ms;
    sliceSamples = (int)(ms * 0.001 * sampleRate);
}

void ChopModule::setGapLength(float ms)
{
    gapLengthMs = ms;
    gapSamples = (int)(ms * 0.001 * sampleRate);
}

void ChopModule::setStutter(int repeats)
{
    stutterRepeats = juce::jlimit(1, 8, repeats);
}

void ChopModule::processBlock(juce::AudioBuffer<float>& outputBuffer)
{
    if (bypassed)
    {
        buffer.write(outputBuffer, 0, outputBuffer.getNumSamples());
        return;
    }

    int numSamples = outputBuffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write input to buffer
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = outputBuffer.getSample(ch, sample);
            buffer.write(outputBuffer, sample, 1);
        }
        
        // State machine
        float outLeft = 0.0f, outRight = 0.0f;
        
        if (currentState == State::Playing)
        {
            // Read from buffer
            if (reverseSlices)
                readPosition = sliceSamples - (samplesInCurrentState % sliceSamples);
            else
                readPosition = samplesInCurrentState % sliceSamples;
            
            int readPos = (buffer.getWritePosition() - sliceSamples + (int)readPosition);
            while (readPos < 0) readPos += buffer.getBufferLength();
            
            outLeft = buffer.read(0, (float)readPos);
            outRight = (numChannels > 1) ? buffer.read(1, (float)readPos) : outLeft;
            
            samplesInCurrentState++;
            
            // Check if slice is done
            if (samplesInCurrentState >= sliceSamples)
            {
                currentStutter++;
                if (currentStutter >= stutterRepeats)
                {
                    currentStutter = 0;
                    if (gapSamples > 0)
                    {
                        currentState = State::Silent;
                        samplesInCurrentState = 0;
                    }
                    else
                    {
                        samplesInCurrentState = 0;
                    }
                }
                else
                {
                    samplesInCurrentState = 0;
                }
            }
        }
        else // Silent
        {
            outLeft = outRight = 0.0f;
            samplesInCurrentState++;
            
            if (samplesInCurrentState >= gapSamples)
            {
                currentState = State::Playing;
                samplesInCurrentState = 0;
            }
        }
        
        outputBuffer.setSample(0, sample, outLeft);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, outRight);
    }
}

void ChopModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("chopSlice", "Chop Slice Length",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chopGap", "Chop Gap",
        juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterInt>("chopStutter", "Chop Stutter", 1, 8, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>("chopReverse", "Chop Reverse", false));
}

void ChopModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "chopSlice") setSliceLength(newValue);
    else if (parameterID == "chopGap") setGapLength(newValue);
    else if (parameterID == "chopStutter") setStutter((int)newValue);
    else if (parameterID == "chopReverse") reverseSlices = newValue > 0.5f;
}

// ============================================
// STRETCH MODULE IMPLEMENTATION
// ============================================
StretchModule::StretchModule()
    : ProcessingModule("Stretch", Type::Activity)
{
}

StretchModule::~StretchModule() = default;

void StretchModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 10 second max input buffer
    inputBuffer.prepare(numChannels, (int)(10.0 * sampleRate));
    
    // Create grains for overlap-add
    const int numGrains = 8;
    grains.resize(numGrains);
    for (auto& grain : grains)
    {
        grain.samples[0].resize(windowSize);
        grain.samples[1].resize(windowSize);
    }
    
    // Create Hann window
    window.resize(windowSize);
    for (int i = 0; i < windowSize; ++i)
    {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (windowSize - 1)));
    }
    
    setStretchFactor(stretchFactor);
    setWindowSize(windowSize * 1000.0f / sampleRate);
}

void StretchModule::releaseResources()
{
    inputBuffer.clear();
}

void StretchModule::setStretchFactor(float factor)
{
    stretchFactor = juce::jlimit(0.25f, 4.0f, factor);
    hopSizeSynthesis = (int)(hopSizeAnalysis * stretchFactor);
}

void StretchModule::setWindowSize(float ms)
{
    windowSize = (int)(ms * 0.001 * sampleRate);
    windowSize = juce::jlimit(256, 8192, windowSize);
    
    hopSizeAnalysis = windowSize / 4;
    setStretchFactor(stretchFactor);
    
    // Resize window
    window.resize(windowSize);
    for (int i = 0; i < windowSize; ++i)
    {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (windowSize - 1)));
    }
}

void StretchModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed || stretchFactor == 1.0f)
    {
        inputBuffer.write(buffer, 0, buffer.getNumSamples());
        return;
    }

    int numSamples = buffer.getNumSamples();
    juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
    outputBuffer.clear();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write to input buffer
        inputBuffer.write(buffer, sample, 1);
        
        // Output accumulated grains
        float outLeft = 0.0f, outRight = 0.0f;
        
        for (auto& grain : grains)
        {
            if (!grain.active) continue;
            
            if (grain.readPos < windowSize)
            {
                float windowVal = window[grain.readPos];
                outLeft += grain.samples[0][grain.readPos] * windowVal;
                outRight += grain.samples[1][grain.readPos] * windowVal;
                grain.readPos++;
            }
            else
            {
                grain.active = false;
            }
        }
        
        outputBuffer.setSample(0, sample, outLeft);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, outRight);
        
        // Trigger new grain?
        grainIndex++;
        if (grainIndex >= hopSizeSynthesis)
        {
            grainIndex = 0;
            
            // Find inactive grain
            for (auto& grain : grains)
            {
                if (!grain.active)
                {
                    // Capture from input buffer
                    int readPos = inputBuffer.getWritePosition() - windowSize;
                    while (readPos < 0) readPos += inputBuffer.getBufferLength();
                    
                    for (int i = 0; i < windowSize; ++i)
                    {
                        int pos = (readPos + i) % inputBuffer.getBufferLength();
                        grain.samples[0][i] = inputBuffer.read(0, (float)pos);
                        grain.samples[1][i] = inputBuffer.read(1, (float)pos);
                    }
                    
                    grain.writePos = 0;
                    grain.readPos = 0;
                    grain.active = true;
                    break;
                }
            }
        }
    }
    
    // Copy output
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.copyFrom(ch, 0, outputBuffer, ch, 0, numSamples);
}

void StretchModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("stretchFactor", "Stretch Factor",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("stretchWindow", "Stretch Window",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 50.0f));
}

void StretchModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "stretchFactor") setStretchFactor(newValue);
    else if (parameterID == "stretchWindow") setWindowSize(newValue);
}

// ============================================
// REVERSE MODULE IMPLEMENTATION
// ============================================
ReverseModule::ReverseModule()
    : ProcessingModule("Reverse", Type::Activity)
{
}

ReverseModule::~ReverseModule() = default;

void ReverseModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 5 second buffer
    buffer.prepare(numChannels, (int)(5.0 * sampleRate));
    
    setReverseInterval(1000.0f);
    setFadeLength(10.0f);
}

void ReverseModule::releaseResources()
{
    buffer.clear();
}

void ReverseModule::setReverseInterval(float ms)
{
    intervalSamples = (int)(ms * 0.001 * sampleRate);
}

void ReverseModule::setFadeLength(float ms)
{
    fadeSamples = (int)(ms * 0.001 * sampleRate);
}

void ReverseModule::processBlock(juce::AudioBuffer<float>& outputBuffer)
{
    if (bypassed)
    {
        buffer.write(outputBuffer, 0, outputBuffer.getNumSamples());
        return;
    }

    int numSamples = outputBuffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write to buffer
        buffer.write(outputBuffer, sample, 1);
        
        float outLeft = 0.0f, outRight = 0.0f;
        
        // Calculate read position based on direction
        if (currentDir == Direction::Forward)
        {
            readPosition += 1.0f;
        }
        else
        {
            readPosition -= 1.0f;
        }
        
        // Keep read position in valid range
        int loopLen = buffer.getLoopLength() > 0 ? buffer.getLoopLength() : buffer.getBufferLength();
        while (readPosition >= loopLen) readPosition -= loopLen;
        while (readPosition < 0) readPosition += loopLen;
        
        // Read from buffer
        outLeft = buffer.read(0, readPosition);
        outRight = (numChannels > 1) ? buffer.read(1, readPosition) : outLeft;
        
        // Handle crossfade when switching directions
        if (isFading)
        {
            fadeCounter--;
            float fadeIn = 1.0f - (float)fadeCounter / fadeSamples;
            float fadeOut = (float)fadeCounter / fadeSamples;
            
            // This is simplified - real implementation would crossfade between forward/reverse streams
            outLeft *= fadeIn;
            outRight *= fadeIn;
            
            if (fadeCounter <= 0)
                isFading = false;
        }
        
        // Check for direction change
        samplesInCurrentDir++;
        if (samplesInCurrentDir >= intervalSamples)
        {
            samplesInCurrentDir = 0;
            currentDir = (currentDir == Direction::Forward) ? Direction::Reverse : Direction::Forward;
            isFading = true;
            fadeCounter = fadeSamples;
        }
        
        outputBuffer.setSample(0, sample, outLeft);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, outRight);
    }
}

void ReverseModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverseInterval", "Reverse Interval",
        juce::NormalisableRange<float>(100.0f, 5000.0f, 10.0f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverseFade", "Reverse Fade",
        juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 10.0f));
}

void ReverseModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "reverseInterval") setReverseInterval(newValue);
    else if (parameterID == "reverseFade") setFadeLength(newValue);
}

// ============================================
// SCATTER MODULE IMPLEMENTATION
// ============================================
ScatterModule::ScatterModule()
    : ProcessingModule("Scatter", Type::Activity),
      random(std::random_device{}()),
      dist(0.0f, 1.0f)
{
}

ScatterModule::~ScatterModule() = default;

void ScatterModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 5 second buffer
    buffer.prepare(numChannels, (int)(5.0 * sampleRate));
    
    setScatterAmount(0.5f);
    setJumpRange(1000.0f);
    setDensity(0.5f);
}

void ScatterModule::releaseResources()
{
    buffer.clear();
}

void ScatterModule::setScatterAmount(float amount)
{
    scatterAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void ScatterModule::setJumpRange(float ms)
{
    jumpRangeMs = ms;
    jumpRangeSamples = (int)(ms * 0.001 * sampleRate);
}

void ScatterModule::setDensity(float d)
{
    density = juce::jlimit(0.0f, 1.0f, d);
    samplesUntilJump = (int)((1.0f - density) * 22050) + 1000;  // 0.5-23ms roughly
}

void ScatterModule::processBlock(juce::AudioBuffer<float>& outputBuffer)
{
    if (bypassed)
    {
        buffer.write(outputBuffer, 0, outputBuffer.getNumSamples());
        return;
    }

    int numSamples = outputBuffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write to buffer
        buffer.write(outputBuffer, sample, 1);
        
        // Check for jump
        samplesUntilJump--;
        if (samplesUntilJump <= 0)
        {
            // Pick new target position
            float jump = (dist(random) - 0.5f) * 2.0f * jumpRangeSamples * scatterAmount;
            targetPosition = readPosition + jump;
            
            int loopLen = buffer.getLoopLength() > 0 ? buffer.getLoopLength() : buffer.getBufferLength();
            while (targetPosition >= loopLen) targetPosition -= loopLen;
            while (targetPosition < 0) targetPosition += loopLen;
            
            // Calculate jump speed (fast jumps = more glitchy)
            currentJumpSpeed = 1.0f + scatterAmount * 10.0f;
            
            // Reset countdown
            samplesUntilJump = (int)((1.0f - density) * 44100) + 1000;
        }
        
        // Move read position toward target
        if (std::abs(targetPosition - readPosition) > 1.0f)
        {
            float diff = targetPosition - readPosition;
            int loopLen = buffer.getLoopLength() > 0 ? buffer.getLoopLength() : buffer.getBufferLength();
            
            // Handle wrap-around
            if (diff > loopLen / 2) diff -= loopLen;
            if (diff < -loopLen / 2) diff += loopLen;
            
            readPosition += diff * 0.1f * currentJumpSpeed;
        }
        else
        {
            readPosition += 1.0f;  // Normal playback
        }
        
        // Keep in bounds
        int loopLen = buffer.getLoopLength() > 0 ? buffer.getLoopLength() : buffer.getBufferLength();
        while (readPosition >= loopLen) readPosition -= loopLen;
        while (readPosition < 0) readPosition += loopLen;
        
        // Read from buffer
        float outLeft = buffer.read(0, readPosition);
        float outRight = (numChannels > 1) ? buffer.read(1, readPosition) : outLeft;
        
        outputBuffer.setSample(0, sample, outLeft);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, outRight);
    }
}

void ScatterModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("scatterAmount", "Scatter Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("scatterRange", "Scatter Range",
        juce::NormalisableRange<float>(10.0f, 5000.0f, 10.0f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("scatterDensity", "Scatter Density",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
}

void ScatterModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "scatterAmount") setScatterAmount(newValue / 100.0f);
    else if (parameterID == "scatterRange") setJumpRange(newValue);
    else if (parameterID == "scatterDensity") setDensity(newValue / 100.0f);
}

// ============================================
// GLITCH MODULE IMPLEMENTATION
// ============================================
GlitchModule::GlitchModule()
    : ProcessingModule("Glitch", Type::Activity),
      random(std::random_device{}())
{
}

GlitchModule::~GlitchModule() = default;

void GlitchModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 2 second buffer
    buffer.prepare(numChannels, (int)(2.0 * sampleRate));
    
    setGlitchProbability(0.1f);
    setRepeatLength(250.0f);
    setRepeatCount(4);
}

void GlitchModule::releaseResources()
{
    buffer.clear();
}

void GlitchModule::setGlitchProbability(float prob)
{
    glitchProbability = juce::jlimit(0.0f, 1.0f, prob);
}

void GlitchModule::setRepeatLength(float ms)
{
    repeatLengthSamples = (int)(ms * 0.001 * sampleRate);
}

void GlitchModule::setRepeatCount(int count)
{
    repeatCount = juce::jlimit(1, 16, count);
}

void GlitchModule::processBlock(juce::AudioBuffer<float>& outputBuffer)
{
    if (bypassed)
    {
        buffer.write(outputBuffer, 0, outputBuffer.getNumSamples());
        return;
    }

    int numSamples = outputBuffer.getNumSamples();
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Write to buffer
        buffer.write(outputBuffer, sample, 1);
        
        float outLeft = 0.0f, outRight = 0.0f;
        
        if (currentState == State::Normal)
        {
            // Check for glitch trigger
            samplesInState++;
            if (samplesInState >= normalDuration)
            {
                if (probDist(random) < glitchProbability)
                {
                    currentState = State::Repeating;
                    samplesInState = 0;
                    currentRepeat = 0;
                    glitchStartPosition = buffer.getWritePosition() - repeatLengthSamples;
                    while (glitchStartPosition < 0) 
                        glitchStartPosition += buffer.getBufferLength();
                    readPosition = glitchStartPosition;
                }
                else
                {
                    samplesInState = 0;
                }
            }
            
            // Normal output
            readPosition = buffer.getWritePosition();
            outLeft = buffer.read(0, readPosition);
            outRight = (numChannels > 1) ? buffer.read(1, readPosition) : outLeft;
        }
        else // Repeating
        {
            // Read from glitch section
            outLeft = buffer.read(0, readPosition);
            outRight = (numChannels > 1) ? buffer.read(1, readPosition) : outLeft;
            
            readPosition++;
            samplesInState++;
            
            // Check if repeat section is done
            if (samplesInState >= repeatLengthSamples)
            {
                samplesInState = 0;
                currentRepeat++;
                readPosition = glitchStartPosition;
                
                if (currentRepeat >= repeatCount)
                {
                    currentState = State::Normal;
                    samplesInState = 0;
                }
            }
        }
        
        outputBuffer.setSample(0, sample, outLeft);
        if (numChannels > 1)
            outputBuffer.setSample(1, sample, outRight);
    }
}

void GlitchModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("glitchProb", "Glitch Probability",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("glitchLength", "Glitch Length",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 250.0f));
    layout.add(std::make_unique<juce::AudioParameterInt>("glitchRepeats", "Glitch Repeats", 1, 16, 4));
}

void GlitchModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "glitchProb") setGlitchProbability(newValue / 100.0f);
    else if (parameterID == "glitchLength") setRepeatLength(newValue);
    else if (parameterID == "glitchRepeats") setRepeatCount((int)newValue);
}
