#include "ModulationUI.h"

// ============================================
// SOURCE ROW
// ============================================
ModulationMatrixView::SourceRow::SourceRow(size_t idx, ModulationSource* src, ModulationMatrix& matrix)
    : sourceIndex(idx), source(src), modulationMatrix(matrix)
{
    nameLabel.setText(source ? source->getName() : "Unknown", juce::dontSendNotification);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);
    
    rateSlider.setRange(0.01, 20.0, 0.01);
    rateSlider.setValue(source ? source->getRate() : 1.0);
    rateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    rateSlider.onValueChange = [this] {
        if (source) source->setRate((float)rateSlider.getValue());
    };
    addAndMakeVisible(rateSlider);
    
    depthSlider.setRange(0.0, 100.0, 1.0);
    depthSlider.setValue(source ? source->getDepth() * 100.0 : 100.0);
    depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    depthSlider.onValueChange = [this] {
        if (source) source->setDepth((float)depthSlider.getValue() / 100.0f);
    };
    addAndMakeVisible(depthSlider);
    
    bipolarButton.setButtonText("±");
    bipolarButton.setToggleState(source ? source->isBipolar() : true, juce::dontSendNotification);
    bipolarButton.onClick = [this] {
        if (source) source->setBipolar(bipolarButton.getToggleState());
    };
    addAndMakeVisible(bipolarButton);
    
    targetButton.setButtonText("Route...");
    targetButton.onClick = [this] { targetButtonClicked(); };
    addAndMakeVisible(targetButton);
}

void ModulationMatrixView::SourceRow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF2a2a4e));
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
}

void ModulationMatrixView::SourceRow::resized()
{
    auto area = getLocalBounds().reduced(5);
    
    nameLabel.setBounds(area.removeFromLeft(100));
    bipolarButton.setBounds(area.removeFromRight(30));
    targetButton.setBounds(area.removeFromRight(70));
    
    auto sliders = area;
    rateSlider.setBounds(sliders.removeFromLeft(sliders.getWidth() / 2));
    depthSlider.setBounds(sliders);
}

void ModulationMatrixView::SourceRow::targetButtonClicked()
{
    // Create a popup menu to select target module and parameter
    juce::PopupMenu menu;
    
    menu.addItem(1, "Granular.Scatter", true, false);
    menu.addItem(2, "Chop.SliceLength", true, false);
    menu.addItem(3, "Filter.Cutoff", true, false);
    menu.addItem(4, "Delay.Time", true, false);
    menu.addItem(5, "EQ.Low-Mid Freq", true, false);
    menu.addItem(6, "EQ.High-Mid Gain", true, false);
    
    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        if (result > 0)
        {
            ModulationRouting routing;
            routing.sourceIndex = (int)sourceIndex;
            routing.targetModuleIndex = 0;  // Granular
            routing.amount = 0.5f;
            routing.enabled = true;
            
            switch (result)
            {
                case 1: routing.targetParameterName = "scatter"; break;
                case 2: routing.targetParameterName = "sliceLength"; break;
                case 3: routing.targetParameterName = "filterCutoff"; break;
                case 4: routing.targetParameterName = "delayLeft"; break;
                case 5: routing.targetParameterName = "eqLowMidFreq"; break;
                case 6: routing.targetParameterName = "eqHighMidGain"; break;
            }
            
            modulationMatrix.addRouting(routing);
        }
    });
}

// ============================================
// ROUTING ROW
// ============================================
ModulationMatrixView::RoutingRow::RoutingRow(int idx, ModulationRouting& rout, ModulationMatrix& matrix)
    : routingIndex(idx), routing(rout), modulationMatrix(matrix)
{
    updateFromRouting();
    
    amountSlider.setRange(-100.0, 100.0, 1.0);
    amountSlider.setValue(routing.amount * 100.0);
    amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    amountSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    amountSlider.onValueChange = [this] {
        modulationMatrix.setRoutingAmount(routingIndex, (float)amountSlider.getValue() / 100.0f);
    };
    addAndMakeVisible(amountSlider);
    
    removeButton.setButtonText("X");
    removeButton.onClick = [this] {
        modulationMatrix.removeRouting(routingIndex);
    };
    addAndMakeVisible(removeButton);
}

void ModulationMatrixView::RoutingRow::updateFromRouting()
{
    auto* source = modulationMatrix.getSource(routing.sourceIndex);
    juce::String sourceName = source ? source->getName() : "Unknown";
    
    infoLabel.setText(sourceName + " → " + routing.targetParameterName, juce::dontSendNotification);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
}

void ModulationMatrixView::RoutingRow::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF252545));
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds(), 1);
}

void ModulationMatrixView::RoutingRow::resized()
{
    auto area = getLocalBounds().reduced(5);
    
    removeButton.setBounds(area.removeFromRight(30));
    infoLabel.setBounds(area.removeFromLeft(200));
    amountSlider.setBounds(area);
}

// ============================================
// MAIN MATRIX VIEW
// ============================================
ModulationMatrixView::ModulationMatrixView(ModulationMatrix& matrix)
    : modulationMatrix(matrix)
{
    setSize(600, 400);
    
    sourcesLabel.setText("Modulation Sources", juce::dontSendNotification);
    sourcesLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    sourcesLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(sourcesLabel);
    
    addSourceButton.setButtonText("+ Add Source");
    addSourceButton.onClick = [this] { addSourceClicked(); };
    addAndMakeVisible(addSourceButton);
    
    // Source type selector
    sourceTypeSelector.addItem("LFO Sine", 1);
    sourceTypeSelector.addItem("LFO Triangle", 2);
    sourceTypeSelector.addItem("LFO Random", 3);
    sourceTypeSelector.addItem("Envelope Follower", 4);
    sourceTypeSelector.addItem("Random Step", 5);
    sourceTypeSelector.addItem("Perlin Noise", 6);
    sourceTypeSelector.onChange = [this] { addSourceFromSelector(); };
    sourceTypeSelector.setVisible(false);
    addAndMakeVisible(sourceTypeSelector);
    
    routingsLabel.setText("Active Routings", juce::dontSendNotification);
    routingsLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    routingsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(routingsLabel);
    
    // Set up viewports
    sourcesViewport.setViewedComponent(&sourcesContainer, false);
    sourcesViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(sourcesViewport);
    
    routingsViewport.setViewedComponent(&routingsContainer, false);
    routingsViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(routingsViewport);
    
    refresh();
}

ModulationMatrixView::~ModulationMatrixView() = default;

void ModulationMatrixView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1a1a2e));
}

void ModulationMatrixView::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Sources section
    sourcesLabel.setBounds(area.removeFromTop(25));
    addSourceButton.setBounds(area.removeFromTop(30));
    sourceTypeSelector.setBounds(addSourceButton.getBounds());
    
    int sourcesHeight = 150;
    sourcesViewport.setBounds(area.removeFromTop(sourcesHeight));
    sourcesContainer.setSize(sourcesViewport.getWidth() - 20, 
                            juce::jmax(sourcesHeight, (int)sourceRows.size() * 60));
    
    int y = 0;
    for (auto& row : sourceRows)
    {
        row->setBounds(0, y, sourcesContainer.getWidth(), 55);
        y += 60;
    }
    
    area.removeFromTop(20);  // Spacing
    
    // Routings section
    routingsLabel.setBounds(area.removeFromTop(25));
    routingsViewport.setBounds(area);
    routingsContainer.setSize(routingsViewport.getWidth() - 20,
                             juce::jmax(area.getHeight(), (int)routingRows.size() * 45));
    
    y = 0;
    for (auto& row : routingRows)
    {
        row->setBounds(0, y, routingsContainer.getWidth(), 40);
        y += 45;
    }
}

void ModulationMatrixView::refresh()
{
    sourceRows.clear();
    routingRows.clear();
    
    // Recreate source rows
    for (size_t i = 0; i < modulationMatrix.getNumSources(); ++i)
    {
        auto* source = modulationMatrix.getSource(i);
        auto row = std::make_unique<SourceRow>(i, source, modulationMatrix);
        sourcesContainer.addAndMakeVisible(*row);
        sourceRows.push_back(std::move(row));
    }
    
    // Recreate routing rows
    int idx = 0;
    for (auto& routing : modulationMatrix.getRoutings())
    {
        auto row = std::make_unique<RoutingRow>(idx, routing, modulationMatrix);
        routingsContainer.addAndMakeVisible(*row);
        routingRows.push_back(std::move(row));
        idx++;
    }
    
    resized();
    repaint();
}

void ModulationMatrixView::addSourceClicked()
{
    sourceTypeSelector.setVisible(true);
    sourceTypeSelector.setSelectedId(0);
}

void ModulationMatrixView::addSourceFromSelector()
{
    int id = sourceTypeSelector.getSelectedId();
    if (id == 0) return;
    
    std::unique_ptr<ModulationSource> source;
    
    switch (id)
    {
        case 1: source = std::make_unique<LFOSine>(); break;
        case 2: source = std::make_unique<LFOTriangle>(); break;
        case 3: source = std::make_unique<LFORandom>(); break;
        case 4: source = std::make_unique<EnvelopeFollower>(); break;
        case 5: source = std::make_unique<RandomStep>(); break;
        case 6: source = std::make_unique<PerlinNoiseMod>(); break;
    }
    
    if (source)
    {
        source->prepare(44100.0, 512);
        modulationMatrix.addSource(std::move(source));
        refresh();
    }
    
    sourceTypeSelector.setVisible(false);
}
