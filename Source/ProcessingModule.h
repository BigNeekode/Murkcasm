#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>
#include <string>

// Base class for all processing modules
class ProcessingModule
{
public:
    enum class Type
    {
        Granular,      // Source generator
        Activity,      // Activity modes (chop, stretch, etc)
        Effect         // FX (reverb, delay, filter)
    };

    ProcessingModule(const juce::String& name, Type type) 
        : moduleName(name), moduleType(type) {}
    virtual ~ProcessingModule() = default;

    // Core interface
    virtual void prepare(double sampleRate, int samplesPerBlock, int numChannels) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer) = 0;
    
    // Parameters
    virtual void createParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout) {}
    virtual void parameterChanged(const juce::String& parameterID, float newValue) {}
    
    // Module info
    const juce::String& getName() const { return moduleName; }
    Type getType() const { return moduleType; }
    bool isBypassed() const { return bypassed; }
    void setBypassed(bool shouldBypass) { bypassed = shouldBypass; }

protected:
    juce::String moduleName;
    Type moduleType;
    bool bypassed = false;
    double sampleRate = 44100.0;
    int numChannels = 2;
};

// Processing chain - manages module order
class ProcessingChain
{
public:
    ProcessingChain();
    ~ProcessingChain();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer);

    // Module management
    void addModule(std::unique_ptr<ProcessingModule> module);
    void removeModule(size_t index);
    void moveModule(size_t fromIndex, size_t toIndex);
    void clear();
    
    // Getters
    size_t getNumModules() const { return modules.size(); }
    ProcessingModule* getModule(size_t index) { return modules[index].get(); }
    const std::vector<std::unique_ptr<ProcessingModule>>& getModules() const { return modules; }

    // Bypass
    void setModuleBypassed(size_t index, bool bypassed);

private:
    std::vector<std::unique_ptr<ProcessingModule>> modules;
};
