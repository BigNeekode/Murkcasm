#include "PluginProcessor.h"
#include "PluginEditor.h"

MicrocosmAudioProcessor::MicrocosmAudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Set up default chain: Granular -> Activity -> Effects
    resetToDefaultChain();
}

MicrocosmAudioProcessor::~MicrocosmAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout MicrocosmAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Main mix parameter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f, juce::AudioParameterFloatAttributes().withLabel("%")));
    
    // Global parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "inputGain", "Input Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));

    return { params.begin(), params.end() };
}

void MicrocosmAudioProcessor::resetToDefaultChain()
{
    processingChain.clear();
    
    // Default chain: Granular Engine -> Scatter Activity -> Reverb -> Delay
    auto granular = std::make_unique<GranularEngine>();
    auto scatter = std::make_unique<ScatterModule>();
    auto reverb = std::make_unique<ReverbModule>();
    auto delay = std::make_unique<DelayModule>();
    
    processingChain.addModule(std::move(granular));
    processingChain.addModule(std::move(scatter));
    processingChain.addModule(std::move(reverb));
    processingChain.addModule(std::move(delay));
}

void MicrocosmAudioProcessor::addModule(std::unique_ptr<ProcessingModule> module)
{
    processingChain.addModule(std::move(module));
}

void MicrocosmAudioProcessor::removeModule(size_t index)
{
    processingChain.removeModule(index);
}

void MicrocosmAudioProcessor::moveModule(size_t fromIndex, size_t toIndex)
{
    processingChain.moveModule(fromIndex, toIndex);
}

void MicrocosmAudioProcessor::toggleModuleBypass(size_t index)
{
    auto* module = processingChain.getModule(index);
    if (module)
        processingChain.setModuleBypassed(index, !module->isBypassed());
}

void MicrocosmAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    processingChain.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    modulationMatrix.prepare(sampleRate, samplesPerBlock);
    
    dryWetMixer.prepare({ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumInputChannels() });
    dryWetMixer.setWetLatency(0);
}

void MicrocosmAudioProcessor::releaseResources()
{
    processingChain.releaseResources();
    modulationMatrix.releaseResources();
}

void MicrocosmAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameters
    mainMix = apvts.getRawParameterValue("mix")->load() / 100.0f;
    float inputGain = std::pow(10.0f, apvts.getRawParameterValue("inputGain")->load() / 20.0f);
    float outputGain = std::pow(10.0f, apvts.getRawParameterValue("outputGain")->load() / 20.0f);

    // Apply input gain
    buffer.applyGain(inputGain);
    
    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Process modulation matrix (updates all mod sources)
    modulationMatrix.processBlock(buffer);
    
    // Process through chain
    processingChain.processBlock(buffer);
    
    // Apply output gain
    buffer.applyGain(outputGain);
    
    // Mix dry/wet
    dryWetMixer.setWetMixProportion(mainMix);
    dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(dryBuffer));
    dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
}

juce::AudioProcessorEditor* MicrocosmAudioProcessor::createEditor()
{
    return new MicrocosmAudioProcessorEditor(*this);
}

void MicrocosmAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MicrocosmAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicrocosmAudioProcessor();
}
