#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <random>
#include "ProcessingModule.h"

// Circular buffer for loop recording
class CircularBuffer
{
public:
    CircularBuffer();
    ~CircularBuffer();
    
    void prepare(int numChannels, int maxLengthSamples);
    void clear();
    
    // Write samples to buffer
    void write(const juce::AudioBuffer<float>& input, int startSample, int numSamples);
    
    // Read sample at position with interpolation
    float read(int channel, float position) const;
    
    // Getters
    int getWritePosition() const { return writePosition; }
    int getBufferLength() const { return bufferLength; }
    void setLoopLength(int length);
    int getLoopLength() const { return loopLength; }
    
private:
    juce::AudioBuffer<float> buffer;
    int writePosition = 0;
    int bufferLength = 0;
    int loopLength = 0;
    int numChannels = 0;
};

// A single grain of audio
struct Grain
{
    float startPosition = 0.0f;    // Position in buffer (samples)
    int length = 0;                // Grain length in samples
    float pan = 0.5f;              // 0 = left, 1 = right
    float pitchRatio = 1.0f;       // Playback speed
    float amplitude = 1.0f;
    bool active = false;
    float currentPosition = 0.0f;  // Current read position within grain
    
    // Envelope
    float getEnvelopeValue(float position) const;  // 0-1 position in grain
};

// Granular synthesis engine - now a ProcessingModule
class GranularEngine : public ProcessingModule
{
public:
    GranularEngine();
    ~GranularEngine() override;
    
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer) override;
    void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    // Parameter setters
    void setGrainSize(float ms);
    void setDensity(int grainsPerBlock);
    void setScatter(float scatterAmount); // 0-1
    void setPitchShift(float ratio);
    void setLoopLength(float ms);
    void setLoopEnabled(bool enabled);
    void setOverdub(bool enabled);
    void setFeedback(float fb);  // Loop feedback
    void clearLoop();
    
private:
    void triggerGrain();
    
    CircularBuffer circularBuffer;
    std::vector<Grain> grains;
    std::mt19937 random;
    std::uniform_real_distribution<float> randomDist;
    
    // Parameters
    float grainSizeMs = 100.0f;
    int grainSizeSamples = 4410;
    int density = 10;
    float scatter = 0.25f;
    float pitchRatio = 1.0f;
    float loopLengthMs = 4000.0f;
    bool loopEnabled = false;
    bool overdub = false;
    float feedback = 0.8f;  // Amount of loop that stays on overdub
    
    // Processing
    static constexpr int maxGrains = 100;
    float samplesUntilNextGrain = 0.0f;
    float grainInterval = 0.0f;
    
    // Mix
    juce::AudioBuffer<float> inputBuffer;
    float wetLevel = 1.0f;
};
