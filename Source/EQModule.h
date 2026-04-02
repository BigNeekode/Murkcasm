#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "ProcessingModule.h"

// ============================================
// 4-BAND PARAMETRIC EQ
// ============================================
class ParametricEQModule : public ProcessingModule
{
public:
    struct Band
    {
        enum class Type { LowShelf, Peak, HighShelf };
        
        Type type;
        float frequency;
        float gainDb;
        float Q;
        
        juce::dsp::IIR::Filter<float> filter;
    };

    ParametricEQModule();
    ~ParametricEQModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Band setters
    void setLowFreq(float freq);
    void setLowGain(float gainDb);
    void setLowMidFreq(float freq);
    void setLowMidGain(float gainDb);
    void setLowMidQ(float Q);
    void setHighMidFreq(float freq);
    void setHighMidGain(float gainDb);
    void setHighMidQ(float Q);
    void setHighFreq(float freq);
    void setHighGain(float gainDb);
    
    // Enable/disable bands
    void setBandEnabled(int bandIndex, bool enabled);

private:
    void updateBandCoefficients(int bandIndex);
    void updateAllCoefficients();

    std::array<Band, 4> bands;
    std::array<bool, 4> bandEnabled = { true, true, true, true };
    
    double sampleRate = 44100.0;
    int numChannels = 2;
    
    // Working buffer for cascading filters
    juce::AudioBuffer<float> workBuffer;
};
