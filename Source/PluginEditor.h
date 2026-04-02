#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "ModulationUI.h"

class ModuleChainView : public juce::Component
{
public:
    ModuleChainView(MicrocosmAudioProcessor& processor);
    ~ModuleChainView() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void refresh();
    
private:
    struct ModuleSlot : public juce::Component
    {
        ModuleSlot(size_t index, MicrocosmAudioProcessor& proc);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        
        size_t slotIndex;
        MicrocosmAudioProcessor& processor;
        bool isEmpty = true;
        juce::String moduleName = "Empty";
        bool isBypassed = false;
    };
    
    MicrocosmAudioProcessor& audioProcessor;
    std::vector<std::unique_ptr<ModuleSlot>> slots;
    
    juce::TextButton addModuleButton;
    juce::ComboBox moduleSelector;
    
    void addModuleClicked();
    void addModuleFromSelector();
    
    static constexpr int maxSlots = 8;
};

class MicrocosmAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MicrocosmAudioProcessorEditor(MicrocosmAudioProcessor&);
    ~MicrocosmAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MicrocosmAudioProcessor& audioProcessor;

    // Main mix
    juce::Slider mixSlider;
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Label mixLabel;
    juce::Label inputLabel;
    juce::Label outputLabel;
    juce::Label titleLabel;
    
    // Tabbed interface
    juce::TabbedComponent tabs;
    
    // Module chain view
    std::unique_ptr<ModuleChainView> chainView;
    
    // Modulation matrix view
    std::unique_ptr<ModulationMatrixView> modulationView;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MicrocosmAudioProcessorEditor)
};
