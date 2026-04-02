#include "ModulationMatrix.h"

// ============================================
// MODULATABLE PARAMETER
// ============================================
ModulatableParameter::ModulatableParameter(const juce::String& n, float defaultVal, 
                                           float minVal, float maxVal, bool bi)
    : name(n), baseValue(defaultVal), minValue(minVal), maxValue(maxVal), bipolar(bi)
{
}

void ModulatableParameter::setBaseValue(float value)
{
    baseValue = juce::jlimit(minValue, maxValue, value);
}

void ModulatableParameter::addModulation(float amount)
{
    modulationSum += amount;
}

float ModulatableParameter::getModulatedValue() const
{
    // modulationSum is typically -1 to 1
    // Apply it as a percentage of the parameter range
    float range = maxValue - minValue;
    float modulated = baseValue + (modulationSum * range * 0.5f);
    
    // Clamp to valid range
    return juce::jlimit(minValue, maxValue, modulated);
}

void ModulatableParameter::startBlock()
{
    modulationSum = 0.0f;
}

// ============================================
// MODULATION MATRIX
// ============================================
ModulationMatrix::ModulationMatrix()
{
}

ModulationMatrix::~ModulationMatrix() = default;

void ModulationMatrix::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    
    for (auto& source : sources)
    {
        source->prepare(sr, samplesPerBlock);
    }
}

void ModulationMatrix::releaseResources()
{
    for (auto& source : sources)
    {
        source->releaseResources();
    }
}

void ModulationMatrix::addSource(std::unique_ptr<ModulationSource> source)
{
    sources.push_back(std::move(source));
}

void ModulationMatrix::removeSource(size_t index)
{
    if (index < sources.size())
    {
        sources.erase(sources.begin() + index);
        
        // Remove routings that use this source
        routings.erase(
            std::remove_if(routings.begin(), routings.end(),
                [index](const ModulationRouting& r) { return r.sourceIndex == (int)index; }),
            routings.end());
        
        // Update indices in remaining routings
        for (auto& routing : routings)
        {
            if (routing.sourceIndex > (int)index)
                routing.sourceIndex--;
        }
    }
}

void ModulationMatrix::registerParameter(int moduleIndex, const juce::String& paramName,
                                        ModulatableParameter* param)
{
    parameters[moduleIndex][paramName] = param;
}

void ModulationMatrix::unregisterParameter(int moduleIndex, const juce::String& paramName)
{
    auto moduleIt = parameters.find(moduleIndex);
    if (moduleIt != parameters.end())
    {
        moduleIt->second.erase(paramName);
    }
}

int ModulationMatrix::addRouting(const ModulationRouting& routing)
{
    routings.push_back(routing);
    return (int)routings.size() - 1;
}

void ModulationMatrix::removeRouting(int routingIndex)
{
    if (routingIndex >= 0 && routingIndex < (int)routings.size())
    {
        routings.erase(routings.begin() + routingIndex);
    }
}

void ModulationMatrix::setRoutingAmount(int routingIndex, float amount)
{
    if (routingIndex >= 0 && routingIndex < (int)routings.size())
    {
        routings[routingIndex].amount = juce::jlimit(-1.0f, 1.0f, amount);
    }
}

void ModulationMatrix::setRoutingEnabled(int routingIndex, bool enabled)
{
    if (routingIndex >= 0 && routingIndex < (int)routings.size())
    {
        routings[routingIndex].enabled = enabled;
    }
}

void ModulationMatrix::processBlock(const juce::AudioBuffer<float>& audioInput)
{
    // Reset all parameter modulations
    for (auto& moduleParams : parameters)
    {
        for (auto& param : moduleParams.second)
        {
            param.second->startBlock();
        }
    }
    
    // Process envelope followers with audio input
    for (size_t i = 0; i < sources.size(); ++i)
    {
        if (sources[i]->getType() == ModulationSource::Type::EnvFollower)
        {
            sources[i]->processBlock(audioInput);
        }
    }
    
    // Apply all routings
    for (const auto& routing : routings)
    {
        if (!routing.enabled)
            continue;
        
        if (routing.sourceIndex < 0 || routing.sourceIndex >= (int)sources.size())
            continue;
        
        auto* source = sources[routing.sourceIndex].get();
        
        // For non-envelope sources, advance them
        if (source->getType() != ModulationSource::Type::EnvFollower)
        {
            for (int sample = 0; sample < audioInput.getNumSamples(); ++sample)
            {
                source->processSample(audioInput.getSample(0, sample));
            }
        }
        
        // Get modulation value and apply to target
        float modValue = source->getValue() * routing.amount;
        
        auto moduleIt = parameters.find(routing.targetModuleIndex);
        if (moduleIt != parameters.end())
        {
            auto paramIt = moduleIt->second.find(routing.targetParameterName);
            if (paramIt != moduleIt->second.end())
            {
                paramIt->second->addModulation(modValue);
            }
        }
    }
}

float ModulationMatrix::getModulatedValue(int moduleIndex, const juce::String& paramName) const
{
    auto moduleIt = parameters.find(moduleIndex);
    if (moduleIt != parameters.end())
    {
        auto paramIt = moduleIt->second.find(paramName);
        if (paramIt != moduleIt->second.end())
        {
            return paramIt->second->getModulatedValue();
        }
    }
    return 0.0f;
}

void ModulationMatrix::clearRoutings()
{
    routings.clear();
}
