#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};  

// A basic example audio EQ plugin
class FODEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FODEQAudioProcessorEditor (FODEQAudioProcessor&);
    ~FODEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FODEQAudioProcessor& audioProcessor;

    // GUI Sliders
    CustomRotarySlider PeakFreqSlider, PeakGainSlider, PeakQualitySlider, LowCutFreqSlider, HighCutFreqSlider, LowCutSlopeSlider, HighCutSlopeSlider;

    std::vector<juce::Component*> GetSliderComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessorEditor)
};
