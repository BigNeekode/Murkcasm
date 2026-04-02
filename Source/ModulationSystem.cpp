#include "ModulationSystem.h"
#include <juce_dsp/juce_dsp.h>

// ============================================
// BASE MODULATION SOURCE
// ============================================
ModulationSource::ModulationSource(const juce::String& n, Type t)
    : name(n), type(t)
{
}

void ModulationSource::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    updatePhaseIncrement();
}

void ModulationSource::releaseResources()
{
}

void ModulationSource::setRate(float r)
{
    rate = juce::jlimit(0.01f, 20.0f, r);
    updatePhaseIncrement();
}

void ModulationSource::setDepth(float d)
{
    depth = juce::jlimit(0.0f, 1.0f, d);
}

void ModulationSource::setOffset(float o)
{
    offset = juce::jlimit(0.0f, 1.0f, o);
    phase = offset;
}

void ModulationSource::setBipolar(bool b)
{
    bipolar = b;
}

void ModulationSource::updatePhaseIncrement()
{
    phaseIncrement = rate / (float)sampleRate;
}

void ModulationSource::processSample(float audioSample)
{
    // Base implementation just advances phase
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
}

void ModulationSource::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // For envelope followers - process the entire block
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        processSample(buffer.getSample(0, sample));
    }
}

// ============================================
// LFO SINE
// ============================================
LFOSine::LFOSine()
    : ModulationSource("LFO Sine", Type::LFOSine)
{
}

float LFOSine::getValue() const
{
    float val = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
    val *= depth;
    
    if (!bipolar)
        val = (val + 1.0f) * 0.5f;
    
    return val;
}

void LFOSine::processSample(float audioSample)
{
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
}

// ============================================
// LFO TRIANGLE
// ============================================
LFOTriangle::LFOTriangle()
    : ModulationSource("LFO Triangle", Type::LFOTriangle)
{
}

float LFOTriangle::getValue() const
{
    float val;
    if (phase < 0.5f)
        val = phase * 4.0f - 1.0f;  // -1 to 1
    else
        val = 3.0f - phase * 4.0f;  // 1 to -1
    
    val *= depth;
    
    if (!bipolar)
        val = (val + 1.0f) * 0.5f;
    
    return val;
}

void LFOTriangle::processSample(float audioSample)
{
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
}

// ============================================
// LFO RANDOM (SMOOTH)
// ============================================
LFORandom::LFORandom()
    : ModulationSource("LFO Random", Type::LFORandom),
      random(std::random_device{}()),
      dist(-1.0f, 1.0f)
{
}

void LFORandom::prepare(double sr, int samplesPerBlock)
{
    ModulationSource::prepare(sr, samplesPerBlock);
    // Smooth over ~10ms
    smoothingCoeff = 1.0f - std::exp(-1.0f / (0.01f * (float)sr));
}

float LFORandom::getValue() const
{
    float val = currentValueSmooth * depth;
    
    if (!bipolar)
        val = (val + 1.0f) * 0.5f;
    
    return val;
}

void LFORandom::processSample(float audioSample)
{
    phase += phaseIncrement;
    
    if (phase >= 1.0f)
    {
        phase -= 1.0f;
        targetValue = dist(random);
    }
    
    // Smooth transition
    currentValueSmooth += (targetValue - currentValueSmooth) * smoothingCoeff;
}

// ============================================
// ENVELOPE FOLLOWER
// ============================================
EnvelopeFollower::EnvelopeFollower()
    : ModulationSource("Env Follower", Type::EnvFollower)
{
    setBipolar(false);  // Envelope is always unipolar
}

void EnvelopeFollower::prepare(double sr, int samplesPerBlock)
{
    ModulationSource::prepare(sr, samplesPerBlock);
    setAttack(10.0f);
    setRelease(100.0f);
}

float EnvelopeFollower::getValue() const
{
    return envelope * depth;
}

void EnvelopeFollower::processSample(float audioSample)
{
    float input = std::abs(audioSample);
    
    // Convert to dB for sensitivity threshold
    float inputDb = input > 0.0001f ? 20.0f * std::log10(input) : -80.0f;
    
    if (inputDb > sensitivity)
    {
        envelope += (input - envelope) * attackCoeff;
    }
    else
    {
        envelope *= (1.0f - releaseCoeff);
    }
    
    // Clamp
    envelope = juce::jlimit(0.0f, 1.0f, envelope);
}

void EnvelopeFollower::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // Process first channel
    const float* data = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        processSample(data[i]);
    }
}

void EnvelopeFollower::setAttack(float ms)
{
    float samples = ms * 0.001f * (float)sampleRate;
    attackCoeff = 1.0f - std::exp(-1.0f / samples);
}

void EnvelopeFollower::setRelease(float ms)
{
    float samples = ms * 0.001f * (float)sampleRate;
    releaseCoeff = 1.0f - std::exp(-1.0f / samples);
}

void EnvelopeFollower::setSensitivity(float db)
{
    sensitivity = db;
}

// ============================================
// RANDOM STEP
// ============================================
RandomStep::RandomStep()
    : ModulationSource("Random Step", Type::RandomStep),
      random(std::random_device{}()),
      dist(-1.0f, 1.0f)
{
}

void RandomStep::prepare(double sr, int samplesPerBlock)
{
    ModulationSource::prepare(sr, samplesPerBlock);
    // Default 100ms steps
    samplesPerStep = (int)(0.1f * (float)sr);
}

float RandomStep::getValue() const
{
    float val = currentValue * depth;
    
    if (!bipolar)
        val = (val + 1.0f) * 0.5f;
    
    return val;
}

void RandomStep::processSample(float audioSample)
{
    sampleCounter++;
    
    if (sampleCounter >= samplesPerStep)
    {
        sampleCounter = 0;
        currentValue = dist(random);
    }
}

// ============================================
// PERLIN NOISE
// ============================================
PerlinNoiseMod::PerlinNoiseMod()
    : ModulationSource("Perlin Noise", Type::PerlinNoise),
      random(std::random_device{}())
{
    // Initialize permutation table
    std::uniform_int_distribution<int> intDist(0, 255);
    int p[256];
    for (int i = 0; i < 256; ++i)
        p[i] = i;
    
    // Shuffle
    std::shuffle(std::begin(p), std::end(p), random);
    
    // Duplicate for wrapping
    for (int i = 0; i < 512; ++i)
        permutation[i] = p[i % 256];
}

float PerlinNoiseMod::fade(float t) const
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float PerlinNoiseMod::lerp(float a, float b, float t) const
{
    return a + t * (b - a);
}

float PerlinNoiseMod::grad(int hash, float x) const
{
    int h = hash & 15;
    float grad = 1.0f + (h & 7);  // Gradient value
    if ((h & 8) != 0) grad = -grad;  // Random sign
    return grad * x;
}

float PerlinNoiseMod::noise(float x) const
{
    int X = (int)std::floor(x) & 255;
    float xf = x - std::floor(x);
    float u = fade(xf);
    
    return lerp(grad(permutation[X], xf),
                grad(permutation[X + 1], xf - 1.0f),
                u);
}

float PerlinNoiseMod::getValue() const
{
    float val = noise(position) * depth;
    
    if (!bipolar)
        val = (val + 1.0f) * 0.5f;
    
    return val;
}

void PerlinNoiseMod::processSample(float audioSample)
{
    position += phaseIncrement * 10.0f;  // Scale for interesting patterns
}
