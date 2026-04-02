#pragma once

#include "ProcessingModule.h"
#include <juce_dsp/juce_dsp.h>

// ============================================
// EFFECT: REVERB - Algorithmic reverb
// ============================================
class ReverbModule : public ProcessingModule
{
public:
    ReverbModule();
    ~ReverbModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setRoomSize(float size);      // 0-1
    void setDamping(float damping);    // 0-1
    void setWidth(float width);        // 0-1 (stereo width)
    void setWetLevel(float wet);       // 0-1

private:
    juce::dsp::Reverb::Parameters reverbParams;
    juce::dsp::Reverb reverb;
    
    float roomSize = 0.5f;
    float damping = 0.5f;
    float width = 0.5f;
    float wetLevel = 0.3f;
    float dryLevel = 0.7f;
};

// ============================================
// EFFECT: DELAY - Stereo delay with feedback
// ============================================
class DelayModule : public ProcessingModule
{
public:
    DelayModule();
    ~DelayModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setDelayTimeLeft(float ms);
    void setDelayTimeRight(float ms);
    void setFeedback(float fb);
    void setPingPong(bool pingPong);
    void setWetLevel(float wet);

private:
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    int bufferSize = 0;
    
    int delaySamplesLeft = 22050;   // 500ms default
    int delaySamplesRight = 33150;  // 750ms default
    float feedback = 0.3f;
    bool pingPongMode = false;
    float wetLevel = 0.5f;
    
    float lastLeftSample = 0.0f;
    float lastRightSample = 0.0f;
};

// ============================================
// EFFECT: FILTER - Multi-mode filter
// ============================================
class FilterModule : public ProcessingModule
{
public:
    enum class FilterMode
    {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peak
    };

    FilterModule();
    ~FilterModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setMode(FilterMode mode);
    void setCutoff(float freq);      // Hz
    void setResonance(float q);      // 0.1-10
    void setGain(float gainDb);      // For peak filter

private:
    void updateFilter();
    
    std::vector<juce::dsp::StateVariableTPTFilter<float>> filters;
    
    FilterMode currentMode = FilterMode::LowPass;
    float cutoffFreq = 2000.0f;
    float resonance = 0.707f;
    float gainDb = 0.0f;
};

// ============================================
// EFFECT: CHORUS - Stereo chorus
// ============================================
class ChorusModule : public ProcessingModule
{
public:
    ChorusModule();
    ~ChorusModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setRate(float rateHz);
    void setDepth(float depth);
    void setDelay(float ms);
    void setWetLevel(float wet);

private:
    juce::dsp::Chorus<float> chorus;
    float rate = 1.0f;
    float depth = 0.5f;
    float centreDelay = 10.0f;
    float wetLevel = 0.5f;
};

// ============================================
// EFFECT: BITCRUSHER - Sample rate / bit depth reduction
// ============================================
class BitcrusherModule : public ProcessingModule
{
public:
    BitcrusherModule();
    ~BitcrusherModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setBitDepth(float bits);        // 1-16
    void setSampleRateReduction(int factor);  // 1-32
    void setMix(float mix);

private:
    float bitDepth = 16.0f;
    int sampleRateDiv = 1;
    float mixLevel = 1.0f;
    
    int sampleCounter = 0;
    float holdLeft = 0.0f;
    float holdRight = 0.0f;
};
