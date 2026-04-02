#include "CircularBuffer.h"

CircularBuffer::CircularBuffer() = default;
CircularBuffer::~CircularBuffer() = default;

void CircularBuffer::prepare(int channels, int maxLengthSamples)
{
    numChannels = channels;
    bufferLength = maxLengthSamples;
    buffer.setSize(numChannels, bufferLength);
    clear();
}

void CircularBuffer::clear()
{
    buffer.clear();
    writePosition = 0;
}

void CircularBuffer::setLoopLength(int length)
{
    loopLength = juce::jmin(length, bufferLength);
}

void CircularBuffer::write(const juce::AudioBuffer<float>& input, int startSample, int numSamples)
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, writePosition, input.getSample(ch, startSample + sample));
        }
        
        writePosition++;
        if (writePosition >= loopLength && loopLength > 0)
            writePosition = 0;
        else if (writePosition >= bufferLength)
            writePosition = 0;
    }
}

float CircularBuffer::read(int channel, float position) const
{
    if (loopLength <= 0) return 0.0f;
    
    // Wrap position
    while (position >= loopLength)
        position -= loopLength;
    while (position < 0)
        position += loopLength;
    
    // Linear interpolation
    int pos1 = (int)position;
    int pos2 = (pos1 + 1) % loopLength;
    float frac = position - pos1;
    
    float sample1 = buffer.getSample(channel, pos1);
    float sample2 = buffer.getSample(channel, pos2);
    
    return sample1 + frac * (sample2 - sample1);
}
