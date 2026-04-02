#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ModulationMatrix.h"

class ModulationMatrixView : public juce::Component
{
public:
    ModulationMatrixView(ModulationMatrix& matrix);
    ~ModulationMatrixView() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void refresh();

private:
    struct SourceRow : public juce::Component
    {
        SourceRow(size_t index, ModulationSource* source, ModulationMatrix& matrix);
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        size_t sourceIndex;
        ModulationSource* source;
        ModulationMatrix& modulationMatrix;
        
        juce::Label nameLabel;
        juce::Slider rateSlider;
        juce::Slider depthSlider;
        juce::TextButton targetButton;
        juce::ToggleButton bipolarButton;
        
        void targetButtonClicked();
    };
    
    struct RoutingRow : public juce::Component
    {
        RoutingRow(int index, ModulationRouting& routing, ModulationMatrix& matrix);
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void updateFromRouting();
        
        int routingIndex;
        ModulationRouting& routing;
        ModulationMatrix& modulationMatrix;
        
        juce::Label infoLabel;
        juce::Slider amountSlider;
        juce::TextButton removeButton;
    };
    
    ModulationMatrix& modulationMatrix;
    
    // Source section
    juce::Label sourcesLabel;
    juce::TextButton addSourceButton;
    juce::ComboBox sourceTypeSelector;
    std::vector<std::unique_ptr<SourceRow>> sourceRows;
    
    // Routing section
    juce::Label routingsLabel;
    std::vector<std::unique_ptr<RoutingRow>> routingRows;
    
    void addSourceClicked();
    void addSourceFromSelector();
    
    juce::Viewport sourcesViewport;
    juce::Viewport routingsViewport;
    juce::Component sourcesContainer;
    juce::Component routingsContainer;
};
