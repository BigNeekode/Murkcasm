#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ProcessingModule.h"
#include "GranularEngine.h"
#include "ActivityModules.h"
#include "EffectModules.h"
#include "ModulationMatrix.h"

class MicrocosmAudioProcessor : public juce::AudioProcessor
{
public:
    MicrocosmAudioProcessor();
    ~MicrocosmAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Processing chain access
    ProcessingChain& getProcessingChain() { return processingChain; }
    
    // Add/Remove modules
    void addModule(std::unique_ptr<ProcessingModule> module);
    void removeModule(size_t index);
    void moveModule(size_t fromIndex, size_t toIndex);
    void toggleModuleBypass(size_t index);
    
    // Reset chain to default configuration
    void resetToDefaultChain();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Modulation matrix access
    ModulationMatrix& getModulationMatrix() { return modulationMatrix; }

private:
    juce::AudioProcessorValueTreeState apvts;
    
    // Modular processing chain (Helix-style)
    ProcessingChain processingChain;
    
    // Modulation system
    ModulationMatrix modulationMatrix;
    
    // Dry/wet mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;
    double currentSampleRate = 44100.0;
    
    // Main mix parameter
    float mainMix = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicrocosmAudioProcessor)
};
