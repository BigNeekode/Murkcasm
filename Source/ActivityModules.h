#pragma once

#include "ProcessingModule.h"
#include "CircularBuffer.h"
#include <random>

// ============================================
// ACTIVITY MODE: CHOP - Tempo-synced slicing
// ============================================
class ChopModule : public ProcessingModule
{
public:
    ChopModule();
    ~ChopModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Chop-specific
    void setSliceLength(float ms);  // Length of each slice
    void setGapLength(float ms);    // Silence between slices
    void setStutter(int repeats);   // Repeat each slice N times

private:
    CircularBuffer buffer;
    
    // Chop state
    enum class State { Playing, Silent };
    State currentState = State::Playing;
    int samplesInCurrentState = 0;
    int sliceSamples = 4410;      // 100ms default
    int gapSamples = 0;
    int stutterCount = 1;
    int currentStutter = 0;
    float readPosition = 0.0f;
    
    // Parameters
    float sliceLengthMs = 100.0f;
    float gapLengthMs = 0.0f;
    int stutterRepeats = 1;
    bool reverseSlices = false;
    
    std::mt19937 random;
};

// ============================================
// ACTIVITY MODE: STRETCH - Time stretch without pitch change
// ============================================
class StretchModule : public ProcessingModule
{
public:
    StretchModule();
    ~StretchModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setStretchFactor(float factor);  // 0.5 = half speed, 2.0 = double
    void setWindowSize(float ms);         // Analysis window

private:
    // Overlap-add time stretching
    struct Grain
    {
        std::vector<float> samples[2];  // Stereo grain storage
        int writePos = 0;
        int readPos = 0;
        bool active = false;
    };
    
    std::vector<Grain> grains;
    CircularBuffer inputBuffer;
    
    float stretchFactor = 1.0f;
    int windowSize = 2048;
    int hopSizeAnalysis = 512;
    int hopSizeSynthesis = 512;
    int grainIndex = 0;
    
    std::vector<float> window;  // Hann window
};

// ============================================
// ACTIVITY MODE: REVERSE - Backwards playback
// ============================================
class ReverseModule : public ProcessingModule
{
public:
    ReverseModule();
    ~ReverseModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setReverseInterval(float ms);  // How often to reverse
    void setFadeLength(float ms);       // Crossfade length

private:
    CircularBuffer buffer;
    
    enum class Direction { Forward, Reverse };
    Direction currentDir = Direction::Forward;
    
    int intervalSamples = 44100;  // 1 second default
    int samplesInCurrentDir = 0;
    int fadeSamples = 256;        // Crossfade samples
    int fadeCounter = 0;
    bool isFading = false;
    
    float readPosition = 0.0f;
};

// ============================================
// ACTIVITY MODE: SCATTER - Random jump cuts
// ============================================
class ScatterModule : public ProcessingModule
{
public:
    ScatterModule();
    ~ScatterModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setScatterAmount(float amount);   // 0-1 randomness
    void setJumpRange(float ms);           // Max jump distance
    void setDensity(float d);              // Jump frequency

private:
    CircularBuffer buffer;
    std::mt19937 random;
    std::uniform_real_distribution<float> dist;
    
    float scatterAmount = 0.5f;
    float jumpRangeMs = 1000.0f;
    int jumpRangeSamples = 44100;
    float density = 0.5f;
    
    int samplesUntilJump = 0;
    float readPosition = 0.0f;
    float targetPosition = 0.0f;
    float currentJumpSpeed = 1.0f;
};

// ============================================
// ACTIVITY MODE: GLITCH - Stutter/repeat effects
// ============================================
class GlitchModule : public ProcessingModule
{
public:
    GlitchModule();
    ~GlitchModule() override;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void setGlitchProbability(float prob);  // Chance of glitch
    void setRepeatLength(float ms);         // Length of glitch repeat
    void setRepeatCount(int count);         // How many repeats

private:
    CircularBuffer buffer;
    std::mt19937 random;
    
    enum class State { Normal, Repeating };
    State currentState = State::Normal;
    
    float glitchProbability = 0.1f;
    int repeatLengthSamples = 11025;  // 250ms
    int repeatCount = 4;
    int currentRepeat = 0;
    int samplesInState = 0;
    
    float glitchStartPosition = 0.0f;
    float readPosition = 0.0f;
    int normalDuration = 44100;  // 1 second between potential glitches
};
