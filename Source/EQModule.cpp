#include "EQModule.h"

ParametricEQModule::ParametricEQModule()
    : ProcessingModule("EQ", Type::Effect)
{
    // Initialize band types
    bands[0].type = Band::Type::LowShelf;
    bands[1].type = Band::Type::Peak;
    bands[2].type = Band::Type::Peak;
    bands[3].type = Band::Type::HighShelf;
    
    // Default frequencies
    bands[0].frequency = 100.0f;
    bands[1].frequency = 500.0f;
    bands[2].frequency = 2000.0f;
    bands[3].frequency = 8000.0f;
    
    // Default gains (0dB = flat)
    for (auto& band : bands)
        band.gainDb = 0.0f;
    
    // Default Q values
    bands[0].Q = 0.707f;
    bands[1].Q = 1.0f;
    bands[2].Q = 1.0f;
    bands[3].Q = 0.707f;
}

ParametricEQModule::~ParametricEQModule() = default;

void ParametricEQModule::prepare(double sr, int samplesPerBlock, int channels)
{
    sampleRate = sr;
    numChannels = channels;
    
    // Prepare work buffer
    workBuffer.setSize(numChannels, samplesPerBlock);
    
    // Prepare all filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)numChannels;
    
    for (auto& band : bands)
    {
        band.filter.prepare(spec);
    }
    
    updateAllCoefficients();
}

void ParametricEQModule::releaseResources()
{
    workBuffer.clear();
}

void ParametricEQModule::updateBandCoefficients(int bandIndex)
{
    if (bandIndex < 0 || bandIndex >= 4)
        return;
    
    auto& band = bands[bandIndex];
    
    // Use IIR coefficients
    juce::dsp::IIR::Coefficients<float>::Ptr coeffs;
    
    switch (band.type)
    {
        case Band::Type::LowShelf:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, band.frequency, band.Q, juce::Decibels::decibelsToGain(band.gainDb));
            break;
            
        case Band::Type::Peak:
            coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, band.frequency, band.Q, juce::Decibels::decibelsToGain(band.gainDb));
            break;
            
        case Band::Type::HighShelf:
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, band.frequency, band.Q, juce::Decibels::decibelsToGain(band.gainDb));
            break;
    }
    
    if (coeffs)
        band.filter.coefficients = coeffs;
}

void ParametricEQModule::updateAllCoefficients()
{
    for (int i = 0; i < 4; ++i)
    {
        if (bandEnabled[i])
            updateBandCoefficients(i);
        else
        {
            // Set to flat (pass-through)
            bands[i].filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate);
        }
    }
}

void ParametricEQModule::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;
    
    int numSamples = buffer.getNumSamples();
    
    // Process each band in series (cascading)
    for (int bandIndex = 0; bandIndex < 4; ++bandIndex)
    {
        if (!bandEnabled[bandIndex])
            continue;
            
        auto& band = bands[bandIndex];
        
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                data[sample] = band.filter.processSample((size_t)ch, data[sample]);
            }
        }
    }
}

// Band setters
void ParametricEQModule::setLowFreq(float freq)
{
    bands[0].frequency = juce::jlimit(20.0f, 500.0f, freq);
    updateBandCoefficients(0);
}

void ParametricEQModule::setLowGain(float gainDb)
{
    bands[0].gainDb = juce::jlimit(-18.0f, 18.0f, gainDb);
    updateBandCoefficients(0);
}

void ParametricEQModule::setLowMidFreq(float freq)
{
    bands[1].frequency = juce::jlimit(100.0f, 2000.0f, freq);
    updateBandCoefficients(1);
}

void ParametricEQModule::setLowMidGain(float gainDb)
{
    bands[1].gainDb = juce::jlimit(-18.0f, 18.0f, gainDb);
    updateBandCoefficients(1);
}

void ParametricEQModule::setLowMidQ(float Q)
{
    bands[1].Q = juce::jlimit(0.1f, 10.0f, Q);
    updateBandCoefficients(1);
}

void ParametricEQModule::setHighMidFreq(float freq)
{
    bands[2].frequency = juce::jlimit(500.0f, 8000.0f, freq);
    updateBandCoefficients(2);
}

void ParametricEQModule::setHighMidGain(float gainDb)
{
    bands[2].gainDb = juce::jlimit(-18.0f, 18.0f, gainDb);
    updateBandCoefficients(2);
}

void ParametricEQModule::setHighMidQ(float Q)
{
    bands[2].Q = juce::jlimit(0.1f, 10.0f, Q);
    updateBandCoefficients(2);
}

void ParametricEQModule::setHighFreq(float freq)
{
    bands[3].frequency = juce::jlimit(2000.0f, 20000.0f, freq);
    updateBandCoefficients(3);
}

void ParametricEQModule::setHighGain(float gainDb)
{
    bands[3].gainDb = juce::jlimit(-18.0f, 18.0f, gainDb);
    updateBandCoefficients(3);
}

void ParametricEQModule::setBandEnabled(int bandIndex, bool enabled)
{
    if (bandIndex >= 0 && bandIndex < 4)
    {
        bandEnabled[bandIndex] = enabled;
        updateBandCoefficients(bandIndex);
    }
}

void ParametricEQModule::createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    // Low shelf
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLowFreq", "EQ Low Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLowGain", "EQ Low Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    
    // Low-mid peak
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLowMidFreq", "EQ Low-Mid Freq",
        juce::NormalisableRange<float>(100.0f, 2000.0f, 1.0f, 0.5f), 500.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLowMidGain", "EQ Low-Mid Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLowMidQ", "EQ Low-Mid Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 1.0f));
    
    // High-mid peak
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHighMidFreq", "EQ High-Mid Freq",
        juce::NormalisableRange<float>(500.0f, 8000.0f, 1.0f, 0.5f), 2000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHighMidGain", "EQ High-Mid Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHighMidQ", "EQ High-Mid Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 1.0f));
    
    // High shelf
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHighFreq", "EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.5f), 8000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHighGain", "EQ High Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));
    
    // Enable switches
    layout.add(std::make_unique<juce::AudioParameterBool>("eqLowEnable", "EQ Low On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("eqLowMidEnable", "EQ Low-Mid On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("eqHighMidEnable", "EQ High-Mid On", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("eqHighEnable", "EQ High On", true));
}

void ParametricEQModule::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "eqLowFreq") setLowFreq(newValue);
    else if (parameterID == "eqLowGain") setLowGain(newValue);
    else if (parameterID == "eqLowMidFreq") setLowMidFreq(newValue);
    else if (parameterID == "eqLowMidGain") setLowMidGain(newValue);
    else if (parameterID == "eqLowMidQ") setLowMidQ(newValue);
    else if (parameterID == "eqHighMidFreq") setHighMidFreq(newValue);
    else if (parameterID == "eqHighMidGain") setHighMidGain(newValue);
    else if (parameterID == "eqHighMidQ") setHighMidQ(newValue);
    else if (parameterID == "eqHighFreq") setHighFreq(newValue);
    else if (parameterID == "eqHighGain") setHighGain(newValue);
    else if (parameterID == "eqLowEnable") setBandEnabled(0, newValue > 0.5f);
    else if (parameterID == "eqLowMidEnable") setBandEnabled(1, newValue > 0.5f);
    else if (parameterID == "eqHighMidEnable") setBandEnabled(2, newValue > 0.5f);
    else if (parameterID == "eqHighEnable") setBandEnabled(3, newValue > 0.5f);
}
