#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

// ============================================
// MODULATION SOURCE - Base class for all modulators
// ============================================
class ModulationSource
{
public:
    enum class Type
    {
        LFOSine,
        LFOTriangle,
        LFORandom,
        EnvFollower,
        RandomStep,
        PerlinNoise
    };

    ModulationSource(const juce::String& name, Type type);
    virtual ~ModulationSource() = default;

    virtual void prepare(double sampleRate, int samplesPerBlock);
    virtual void releaseResources();
    
    // Get current modulation value (-1 to 1, or 0 to 1 for unipolar)
    virtual float getValue() const = 0;
    
    // Process one sample (advance LFO, update envelope, etc.)
    virtual void processSample(float audioSample = 0.0f);
    
    // Process a block (for envelope followers)
    virtual void processBlock(const juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setRate(float rateHz);
    void setDepth(float depth);  // 0-1
    void setOffset(float offset); // Phase offset (0-1)
    void setBipolar(bool bipolar); // true = -1 to 1, false = 0 to 1
    
    // Getters
    const juce::String& getName() const { return name; }
    Type getType() const { return type; }
    float getRate() const { return rate; }
    float getDepth() const { return depth; }
    bool isBipolar() const { return bipolar; }

protected:
    juce::String name;
    Type type;
    
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    
    float rate = 1.0f;      // Hz
    float depth = 1.0f;     // 0-1
    float offset = 0.0f;    // 0-1 phase offset
    bool bipolar = true;    // Output range
    
    float currentValue = 0.0f;
};

// ============================================
// LFO IMPLEMENTATIONS
// ============================================

class LFOSine : public ModulationSource
{
public:
    LFOSine();
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;
};

class LFOTriangle : public ModulationSource
{
public:
    LFOTriangle();
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;
};

class LFORandom : public ModulationSource
{
public:
    LFORandom();
    void prepare(double sampleRate, int samplesPerBlock) override;
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;

private:
    float targetValue = 0.0f;
    float currentValueSmooth = 0.0f;
    float smoothingCoeff = 0.01f;
    std::mt19937 random;
    std::uniform_real_distribution<float> dist;
};

// ============================================
// ENVELOPE FOLLOWER
// ============================================
class EnvelopeFollower : public ModulationSource
{
public:
    EnvelopeFollower();
    void prepare(double sampleRate, int samplesPerBlock) override;
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;
    void processBlock(const juce::AudioBuffer<float>& buffer) override;
    
    void setAttack(float ms);
    void setRelease(float ms);
    void setSensitivity(float db);

private:
    float attackCoeff = 0.1f;
    float releaseCoeff = 0.01f;
    float envelope = 0.0f;
    float sensitivity = 0.0f;  // dB threshold
};

// ============================================
// RANDOM STEP GENERATOR
// ============================================
class RandomStep : public ModulationSource
{
public:
    RandomStep();
    void prepare(double sampleRate, int samplesPerBlock) override;
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;

private:
    int samplesPerStep = 4410;  // Default: 100ms at 44.1kHz
    int sampleCounter = 0;
    std::mt19937 random;
    std::uniform_real_distribution<float> dist;
};

// ============================================
// PERLIN NOISE
// ============================================
class PerlinNoiseMod : public ModulationSource
{
public:
    PerlinNoiseMod();
    float getValue() const override;
    void processSample(float audioSample = 0.0f) override;

private:
    float noise(float x) const;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x) const;
    
    int permutation[512];
    float position = 0.0f;
};
