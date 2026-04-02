#include "ProcessingModule.h"

ProcessingChain::ProcessingChain()
{
}

ProcessingChain::~ProcessingChain()
{
}

void ProcessingChain::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    for (auto& module : modules)
    {
        module->prepare(sampleRate, samplesPerBlock, numChannels);
    }
}

void ProcessingChain::releaseResources()
{
    for (auto& module : modules)
    {
        module->releaseResources();
    }
}

void ProcessingChain::processBlock(juce::AudioBuffer<float>& buffer)
{
    for (auto& module : modules)
    {
        if (!module->isBypassed())
        {
            module->processBlock(buffer);
        }
    }
}

void ProcessingChain::addModule(std::unique_ptr<ProcessingModule> module)
{
    modules.push_back(std::move(module));
}

void ProcessingChain::removeModule(size_t index)
{
    if (index < modules.size())
    {
        modules.erase(modules.begin() + index);
    }
}

void ProcessingChain::moveModule(size_t fromIndex, size_t toIndex)
{
    if (fromIndex < modules.size() && toIndex < modules.size())
    {
        std::swap(modules[fromIndex], modules[toIndex]);
    }
}

void ProcessingChain::clear()
{
    modules.clear();
}

void ProcessingChain::setModuleBypassed(size_t index, bool bypassed)
{
    if (index < modules.size())
    {
        modules[index]->setBypassed(bypassed);
    }
}
