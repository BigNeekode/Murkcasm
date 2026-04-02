#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

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
