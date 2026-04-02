#include "EffectModules.h"

// ============================================
// REVERB MODULE IMPLEMENTATION
// ============================================
ReverbModule::ReverbModule()
    : ProcessingModule("Reverb", Type::Effect)
{
}

ReverbModule::~ReverbModule() = default;

void ReverbModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)numChannels;
    
    reverb.prepare(spec);
    
    reverbParams.roomSize = roomSize;
    reverbParams.damping = damping;
    reverbParams.width = width;
    reverbParams.wetLevel = wetLevel;
    reverbParams.dryLevel = dryLevel;
    
    reverb.setParameters(reverbParams);
}

void ReverbModule::releaseResources()
{
}

void ReverbModule::setRoomSize(float size)
{
    roomSize = juce::jlimit(0.0f, 1.0f, size);
    reverbParams.roomSize = roomSize;
    reverb.setParameters(reverbParams);
}

void ReverbModule::setDamping(float d)
{
    damping = juce::jlimit(0.0f, 1.0f, d);
    reverbParams.damping = damping;
    reverb.setParameters(reverbParams);
}

void ReverbModule::setWidth(float w)
{
    width = juce::jlimit(0.0f, 1.0f, w);
    reverbParams.width = width;
    reverb.setParameters(reverbParams);
}

void ReverbModule::setWetLevel(float wet)
{
    wetLevel = juce::jlimit(0.0f, 1.0f, wet);
    dryLevel = 1.0f - wetLevel;
    reverbParams.wetLevel = wetLevel;
    reverbParams.dryLevel = dryLevel;
    reverb.setParameters(reverbParams);
}

void ReverbModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}

void ReverbModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 30.0f));
}

void ReverbModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "reverbSize") setRoomSize(newValue / 100.0f);
    else if (parameterID == "reverbDamping") setDamping(newValue / 100.0f);
    else if (parameterID == "reverbWidth") setWidth(newValue / 100.0f);
    else if (parameterID == "reverbMix") setWetLevel(newValue / 100.0f);
}

// ============================================
// DELAY MODULE IMPLEMENTATION
// ============================================
DelayModule::DelayModule()
    : ProcessingModule("Delay", Type::Effect)
{
}

DelayModule::~DelayModule() = default;

void DelayModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // 5 second max delay
    bufferSize = (int)(5.0 * sampleRate);
    delayBuffer.setSize(numChannels, bufferSize);
    delayBuffer.clear();
    
    writePosition = 0;
    
    setDelayTimeLeft(500.0f);
    setDelayTimeRight(750.0f);
}

void DelayModule::releaseResources()
{
    delayBuffer.clear();
}

void DelayModule::setDelayTimeLeft(float ms)
{
    delaySamplesLeft = (int)(ms * 0.001 * sampleRate);
    delaySamplesLeft = juce::jlimit(1, bufferSize - 1, delaySamplesLeft);
}

void DelayModule::setDelayTimeRight(float ms)
{
    delaySamplesRight = (int)(ms * 0.001 * sampleRate);
    delaySamplesRight = juce::jlimit(1, bufferSize - 1, delaySamplesRight);
}

void DelayModule::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 0.99f, fb);
}

void DelayModule::setPingPong(bool pingPong)
{
    pingPongMode = pingPong;
}

void DelayModule::setWetLevel(float wet)
{
    wetLevel = juce::jlimit(0.0f, 1.0f, wet);
}

void DelayModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
    
    int numSamples = buffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float inputLeft = buffer.getSample(0, sample);
        float inputRight = numChannels > 1 ? buffer.getSample(1, sample) : inputLeft;
        
        // Calculate read positions
        int readPosLeft = writePosition - delaySamplesLeft;
        if (readPosLeft < 0) readPosLeft += bufferSize;
        
        int readPosRight = writePosition - delaySamplesRight;
        if (readPosRight < 0) readPosRight += bufferSize;
        
        // Read delayed samples
        float delayedLeft = delayBuffer.getSample(0, readPosLeft);
        float delayedRight = delayBuffer.getSample(1, readPosRight);
        
        // Ping-pong: cross feedback
        float writeLeft, writeRight;
        if (pingPongMode)
        {
            writeLeft = inputLeft + delayedRight * feedback;
            writeRight = inputRight + delayedLeft * feedback;
        }
        else
        {
            writeLeft = inputLeft + delayedLeft * feedback;
            writeRight = inputRight + delayedRight * feedback;
        }
        
        // Write to delay buffer
        delayBuffer.setSample(0, writePosition, writeLeft);
        delayBuffer.setSample(1, writePosition, writeRight);
        
        // Output: mix dry and wet
        buffer.setSample(0, sample, inputLeft * (1.0f - wetLevel) + delayedLeft * wetLevel);
        if (numChannels > 1)
            buffer.setSample(1, sample, inputRight * (1.0f - wetLevel) + delayedRight * wetLevel);
        
        writePosition++;
        if (writePosition >= bufferSize)
            writePosition = 0;
    }
}

void DelayModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("delayLeft", "Delay Left",
        juce::NormalisableRange<float>(1.0f, 5000.0f, 1.0f), 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("delayRight", "Delay Right",
        juce::NormalisableRange<float>(1.0f, 5000.0f, 1.0f), 750.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 99.0f, 1.0f), 30.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("delayPingPong", "Delay PingPong", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>("delayMix", "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
}

void DelayModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "delayLeft") setDelayTimeLeft(newValue);
    else if (parameterID == "delayRight") setDelayTimeRight(newValue);
    else if (parameterID == "delayFeedback") setFeedback(newValue / 100.0f);
    else if (parameterID == "delayPingPong") setPingPong(newValue > 0.5f);
    else if (parameterID == "delayMix") setWetLevel(newValue / 100.0f);
}

// ============================================
// FILTER MODULE IMPLEMENTATION
// ============================================
FilterModule::FilterModule()
    : ProcessingModule("Filter", Type::Effect)
{
}

FilterModule::~FilterModule() = default;

void FilterModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    filters.resize(numChannels);
    for (auto& filter : filters)
    {
        filter.reset();
    }
    
    updateFilter();
}

void FilterModule::releaseResources()
{
}

void FilterModule::setMode(FilterMode mode)
{
    currentMode = mode;
    updateFilter();
}

void FilterModule::setCutoff(float freq)
{
    cutoffFreq = juce::jlimit(20.0f, 20000.0f, freq);
    updateFilter();
}

void FilterModule::setResonance(float q)
{
    resonance = juce::jlimit(0.1f, 10.0f, q);
    updateFilter();
}

void FilterModule::setGain(float gain)
{
    gainDb = juce::jlimit(-18.0f, 18.0f, gain);
    updateFilter();
}

void FilterModule::updateFilter()
{
    for (auto& filter : filters)
    {
        switch (currentMode)
        {
            case FilterMode::LowPass:
                filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
            case FilterMode::HighPass:
                filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                break;
            case FilterMode::BandPass:
                filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                break;
        }
        
        filter.setCutoffFrequency(cutoffFreq);
        filter.setResonance(resonance);
    }
}

void FilterModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
    
    for (int ch = 0; ch < numChannels && ch < (int)filters.size(); ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = filters[ch].processSample(channelData[sample]);
        }
    }
}

void FilterModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterChoice>("filterMode", "Filter Mode",
        juce::StringArray{"Low Pass", "High Pass", "Band Pass"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>("filterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 2000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("filterResonance", "Filter Resonance",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 0.707f));
}

void FilterModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "filterMode")
    {
        int mode = (int)newValue;
        if (mode == 0) setMode(FilterMode::LowPass);
        else if (mode == 1) setMode(FilterMode::HighPass);
        else if (mode == 2) setMode(FilterMode::BandPass);
    }
    else if (parameterID == "filterCutoff") setCutoff(newValue);
    else if (parameterID == "filterResonance") setResonance(newValue);
}

// ============================================
// CHORUS MODULE IMPLEMENTATION
// ============================================
ChorusModule::ChorusModule()
    : ProcessingModule("Chorus", Type::Effect)
{
}

ChorusModule::~ChorusModule() = default;

void ChorusModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)numChannels;
    
    chorus.prepare(spec);
    chorus.setRate(rate);
    chorus.setDepth(depth);
    chorus.setCentreDelay(centreDelay);
}

void ChorusModule::releaseResources()
{
}

void ChorusModule::setRate(float rateHz)
{
    rate = juce::jlimit(0.0f, 10.0f, rateHz);
    chorus.setRate(rate);
}

void ChorusModule::setDepth(float d)
{
    depth = juce::jlimit(0.0f, 1.0f, d);
    chorus.setDepth(depth);
}

void ChorusModule::setDelay(float ms)
{
    centreDelay = juce::jlimit(1.0f, 100.0f, ms);
    chorus.setCentreDelay(centreDelay);
}

void ChorusModule::setWetLevel(float wet)
{
    wetLevel = juce::jlimit(0.0f, 1.0f, wet);
    chorus.setMix(wetLevel);
}

void ChorusModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);
}

void ChorusModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("chorusRate", "Chorus Rate",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chorusDepth", "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chorusDelay", "Chorus Delay",
        juce::NormalisableRange<float>(1.0f, 50.0f, 0.5f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("chorusMix", "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f));
}

void ChorusModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "chorusRate") setRate(newValue);
    else if (parameterID == "chorusDepth") setDepth(newValue / 100.0f);
    else if (parameterID == "chorusDelay") setDelay(newValue);
    else if (parameterID == "chorusMix") setWetLevel(newValue / 100.0f);
}

// ============================================
// BITCRUSHER MODULE IMPLEMENTATION
// ============================================
BitcrusherModule::BitcrusherModule()
    : ProcessingModule("Bitcrusher", Type::Effect)
{
}

BitcrusherModule::~BitcrusherModule() = default;

void BitcrusherModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
}

void BitcrusherModule::releaseResources()
{
}

void BitcrusherModule::setBitDepth(float bits)
{
    bitDepth = juce::jlimit(1.0f, 16.0f, bits);
}

void BitcrusherModule::setSampleRateReduction(int factor)
{
    sampleRateDiv = juce::jlimit(1, 32, factor);
}

void BitcrusherModule::setMix(float mix)
{
    mixLevel = juce::jlimit(0.0f, 1.0f, mix);
}

void BitcrusherModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed || (bitDepth >= 16.0f && sampleRateDiv <= 1))
        return;
    
    int numSamples = buffer.getNumSamples();
    float quantStep = 1.0f / std::pow(2.0f, bitDepth - 1);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Sample rate reduction (hold samples)
        if (sampleCounter % sampleRateDiv == 0)
        {
            holdLeft = buffer.getSample(0, sample);
            holdRight = numChannels > 1 ? buffer.getSample(1, sample) : holdLeft;
        }
        
        // Bit depth reduction (quantization)
        float crushedLeft = std::round(holdLeft / quantStep) * quantStep;
        float crushedRight = std::round(holdRight / quantStep) * quantStep;
        
        // Mix with dry
        float dryLeft = buffer.getSample(0, sample);
        float dryRight = numChannels > 1 ? buffer.getSample(1, sample) : dryLeft;
        
        buffer.setSample(0, sample, dryLeft * (1.0f - mixLevel) + crushedLeft * mixLevel);
        if (numChannels > 1)
            buffer.setSample(1, sample, dryRight * (1.0f - mixLevel) + crushedRight * mixLevel);
        
        sampleCounter++;
    }
}

void BitcrusherModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>("crushBits", "Bit Depth",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.1f), 16.0f));
    layout.add(std::make_unique<juce::AudioParameterInt>("crushRate", "Sample Rate Div", 1, 32, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>("crushMix", "Crush Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));
}

void BitcrusherModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "crushBits") setBitDepth(newValue);
    else if (parameterID == "crushRate") setSampleRateReduction((int)newValue);
    else if (parameterID == "crushMix") setMix(newValue / 100.0f);
}
