#pragma once

#include "ModulationSystem.h"
#include <functional>
#include <memory>
#include <map>
#include <vector>

// ============================================
// MODULATABLE PARAMETER - Wraps a parameter to accept modulation
// ============================================
class ModulatableParameter
{
public:
    using ValueChangedCallback = std::function<void(float)>;

    ModulatableParameter(const juce::String& name, float defaultValue, 
                         float minVal, float maxVal, bool bipolar = false);
    
    // Set the base value (from UI/automation)
    void setBaseValue(float value);
    float getBaseValue() const { return baseValue; }
    
    // Add modulation from a source
    void addModulation(float amount);
    
    // Calculate final value after all modulations applied
    float getModulatedValue() const;
    
    // Call at start of each block to reset modulations
    void startBlock();
    
    // Getters
    const juce::String& getName() const { return name; }
    float getMin() const { return minValue; }
    float getMax() const { return maxValue; }
    bool isBipolar() const { return bipolar; }

private:
    juce::String name;
    float baseValue;
    float minValue, maxValue;
    bool bipolar;
    
    float modulationSum = 0.0f;
};

// ============================================
// MODULATION ROUTING - Connects one source to one target
// ============================================
struct ModulationRouting
{
    int sourceIndex = -1;
    int targetModuleIndex = -1;
    juce::String targetParameterName;
    float amount = 0.0f;  // -1 to 1 (negative = inverted)
    bool enabled = true;
};

// ============================================
// MODULATION MATRIX - Manages all routings
// ============================================
class ModulationMatrix
{
public:
    ModulationMatrix();
    ~ModulationMatrix();
    
    void prepare(double sampleRate, int samplesPerBlock);
    void releaseResources();
    
    // Source management
    void addSource(std::unique_ptr<ModulationSource> source);
    void removeSource(size_t index);
    size_t getNumSources() const { return sources.size(); }
    ModulationSource* getSource(size_t index) { return sources[index].get(); }
    
    // Target registration - modules call this to expose modulatable parameters
    void registerParameter(int moduleIndex, const juce::String& paramName,
                          ModulatableParameter* param);
    void unregisterParameter(int moduleIndex, const juce::String& paramName);
    
    // Routing management
    int addRouting(const ModulationRouting& routing);
    void removeRouting(int routingIndex);
    void setRoutingAmount(int routingIndex, float amount);
    void setRoutingEnabled(int routingIndex, bool enabled);
    std::vector<ModulationRouting>& getRoutings() { return routings; }
    
    // Process - updates all sources and applies modulations
    void processBlock(const juce::AudioBuffer<float>& audioInput);
    
    // Get modulated value for a parameter
    float getModulatedValue(int moduleIndex, const juce::String& paramName) const;
    
    // Clear all routings
    void clearRoutings();

private:
    std::vector<std::unique_ptr<ModulationSource>> sources;
    std::vector<ModulationRouting> routings;
    
    // Map: moduleIndex -> (paramName -> parameter)
    std::map<int, std::map<juce::String, ModulatableParameter*>> parameters;
    
    double sampleRate = 44100.0;
};
