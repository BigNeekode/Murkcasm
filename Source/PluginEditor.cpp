#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "EQModule.h"

// ============================================
// MODULE CHAIN VIEW IMPLEMENTATION
// ============================================
ModuleChainView::ModuleSlot::ModuleSlot(size_t index, MicrocosmAudioProcessor& proc)
    : slotIndex(index), processor(proc)
{
}

void ModuleChainView::ModuleSlot::paint(juce::Graphics& g)
{
    // Background
    auto bgColor = isEmpty ? juce::Colour(0xFF2a2a4e) : juce::Colour(0xFF3a3a6e);
    if (isBypassed && !isEmpty)
        bgColor = bgColor.darker(0.3f);
    
    g.fillAll(bgColor);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
    
    // Text
    g.setColour(isBypassed ? juce::Colours::grey : juce::Colours::white);
    g.setFont(12.0f);
    g.drawText(moduleName, getLocalBounds(), juce::Justification::centred, true);
    
    if (!isEmpty)
    {
        // Bypass indicator
        g.setColour(isBypassed ? juce::Colours::red : juce::Colours::green);
        g.fillEllipse(getWidth() - 12, 4, 8, 8);
    }
}

void ModuleChainView::ModuleSlot::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown() && !isEmpty)
    {
        // Right click to toggle bypass
        processor.toggleModuleBypass(slotIndex);
        isBypassed = !isBypassed;
        repaint();
    }
    else if (e.mods.isLeftButtonDown() && !isEmpty)
    {
        // Left click could open parameter editor
        // For now, toggle bypass
        processor.toggleModuleBypass(slotIndex);
        isBypassed = !isBypassed;
        repaint();
    }
}

ModuleChainView::ModuleChainView(MicrocosmAudioProcessor& processor)
    : audioProcessor(processor)
{
    setSize(600, 150);
    
    // Create slots
    for (int i = 0; i < maxSlots; ++i)
    {
        auto slot = std::make_unique<ModuleSlot>(i, processor);
        addAndMakeVisible(*slot);
        slots.push_back(std::move(slot));
    }
    
    // Add module button
    addModuleButton.setButtonText("+ Add Module");
    addModuleButton.onClick = [this] { addModuleClicked(); };
    addAndMakeVisible(addModuleButton);
    
    // Module selector (hidden initially)
    moduleSelector.addItem("Granular", 1);
    moduleSelector.addItem("Chop", 2);
    moduleSelector.addItem("Stretch", 3);
    moduleSelector.addItem("Reverse", 4);
    moduleSelector.addItem("Scatter", 5);
    moduleSelector.addItem("Glitch", 6);
    moduleSelector.addItem("Reverb", 7);
    moduleSelector.addItem("Delay", 8);
    moduleSelector.addItem("Filter", 9);
    moduleSelector.addItem("Chorus", 10);
    moduleSelector.addItem("Bitcrusher", 11);
    moduleSelector.addItem("EQ", 12);
    moduleSelector.onChange = [this] { addModuleFromSelector(); };
    moduleSelector.setVisible(false);
    addAndMakeVisible(moduleSelector);
    
    refresh();
}

ModuleChainView::~ModuleChainView() = default;

void ModuleChainView::refresh()
{
    auto& chain = audioProcessor.getProcessingChain();
    
    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto* module = chain.getModule(i);
        if (module)
        {
            slots[i]->isEmpty = false;
            slots[i]->moduleName = module->getName();
            slots[i]->isBypassed = module->isBypassed();
        }
        else
        {
            slots[i]->isEmpty = true;
            slots[i]->moduleName = "Empty";
            slots[i]->isBypassed = false;
        }
        slots[i]->repaint();
    }
}

void ModuleChainView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1a1a2e));
    
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("Processing Chain (click to bypass, right-click to remove)", 0, 0, getWidth(), 20, juce::Justification::left);
}

void ModuleChainView::resized()
{
    auto area = getLocalBounds().reduced(5);
    area.removeFromTop(25);  // Title space
    
    // Slot layout - horizontal row
    int slotWidth = 70;
    int slotHeight = 80;
    int spacing = 5;
    
    for (size_t i = 0; i < slots.size(); ++i)
    {
        slots[i]->setBounds(5 + i * (slotWidth + spacing), 30, slotWidth, slotHeight);
    }
    
    // Add button at bottom
    addModuleButton.setBounds(5, getHeight() - 35, 100, 30);
    moduleSelector.setBounds(110, getHeight() - 35, 150, 30);
}

void ModuleChainView::addModuleClicked()
{
    moduleSelector.setVisible(true);
    moduleSelector.setSelectedId(0);
}

void ModuleChainView::addModuleFromSelector()
{
    int id = moduleSelector.getSelectedId();
    if (id == 0) return;
    
    std::unique_ptr<ProcessingModule> module;
    
    switch (id)
    {
        case 1: module = std::make_unique<GranularEngine>(); break;
        case 2: module = std::make_unique<ChopModule>(); break;
        case 3: module = std::make_unique<StretchModule>(); break;
        case 4: module = std::make_unique<ReverseModule>(); break;
        case 5: module = std::make_unique<ScatterModule>(); break;
        case 6: module = std::make_unique<GlitchModule>(); break;
        case 7: module = std::make_unique<ReverbModule>(); break;
        case 8: module = std::make_unique<DelayModule>(); break;
        case 9: module = std::make_unique<FilterModule>(); break;
        case 10: module = std::make_unique<ChorusModule>(); break;
        case 11: module = std::make_unique<BitcrusherModule>(); break;
        case 12: module = std::make_unique<ParametricEQModule>(); break;
    }
    
    if (module)
    {
        // Prepare the module
        module->prepare(44100.0, 512, 2);
        audioProcessor.addModule(std::move(module));
        refresh();
    }
    
    moduleSelector.setVisible(false);
}

// ============================================
// MAIN EDITOR IMPLEMENTATION
// ============================================
MicrocosmAudioProcessorEditor::MicrocosmAudioProcessorEditor(MicrocosmAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tabs(juce::TabbedButtonBar::TabsAtTop)
{
    // Title
    titleLabel.setText("Microcosm", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // Setup sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
        addAndMakeVisible(slider);
        
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };

    setupSlider(mixSlider, mixLabel, "Mix");
    setupSlider(inputGainSlider, inputLabel, "Input");
    setupSlider(outputGainSlider, outputLabel, "Output");

    // Create chain view
    chainView = std::make_unique<ModuleChainView>(audioProcessor);
    
    // Create modulation view
    modulationView = std::make_unique<ModulationMatrixView>(audioProcessor.getModulationMatrix());
    
    // Set up tabs
    tabs.addTab("Modules", juce::Colours::transparentBlack, chainView.get(), false);
    tabs.addTab("Modulation", juce::Colours::transparentBlack, modulationView.get(), false);
    tabs.setTabBackgroundColour(0, juce::Colour(0xFF2a2a4e));
    tabs.setTabBackgroundColour(1, juce::Colour(0xFF2a2a4e));
    addAndMakeVisible(tabs);

    // Attachments
    auto& apvts = audioProcessor.getAPVTS();
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "mix", mixSlider);
    inputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "inputGain", inputGainSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "outputGain", outputGainSlider);

    setSize(700, 500);
}

MicrocosmAudioProcessorEditor::~MicrocosmAudioProcessorEditor() = default;

void MicrocosmAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Gradient background
    juce::ColourGradient gradient(
        juce::Colour(0xFF1a1a2e), 0.0f, 0.0f,
        juce::Colour(0xFF0f0f1a), 0.0f, (float)getHeight(), false);
    g.fillAll(juce::Colour(0xFF1a1a2e));
    
    // Subtle border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
}

void MicrocosmAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(15);
    
    titleLabel.setBounds(area.removeFromTop(40));
    
    // Tabs take most of the space
    tabs.setBounds(area.removeFromTop(280));
    
    area.removeFromTop(20);  // Spacing
    
    // Controls row at bottom
    auto controlsRow = area.removeFromTop(100);
    
    int knobWidth = 80;
    int startX = (getWidth() - 3 * knobWidth) / 2;
    
    inputGainSlider.setBounds(startX, controlsRow.getY(), knobWidth, knobWidth);
    inputLabel.setBounds(startX, controlsRow.getY() - 20, knobWidth, 20);
    
    mixSlider.setBounds(startX + knobWidth + 10, controlsRow.getY(), knobWidth, knobWidth);
    mixLabel.setBounds(startX + knobWidth + 10, controlsRow.getY() - 20, knobWidth, 20);
    
    outputGainSlider.setBounds(startX + 2 * (knobWidth + 10), controlsRow.getY(), knobWidth, knobWidth);
    outputLabel.setBounds(startX + 2 * (knobWidth + 10), controlsRow.getY() - 20, knobWidth, 20);
}
