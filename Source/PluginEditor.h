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
class FODEQAudioProcessorEditor  : public juce::AudioProcessorEditor, juce::AudioProcessorParameter::Listener, juce::Timer
{
public:
    FODEQAudioProcessorEditor (FODEQAudioProcessor&);
    ~FODEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // juce::AudioProcessorParameter::Listener interface
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

    // juce::Timer interface
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FODEQAudioProcessor& audioProcessor;

    // GUI Sliders
    CustomRotarySlider PeakFreqSlider, PeakGainSlider, PeakQualitySlider, LowCutFreqSlider, HighCutFreqSlider, LowCutSlopeSlider, HighCutSlopeSlider;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    SliderAttachment PeakFreqSliderAttachment, PeakGainSliderAttachment, PeakQualitySliderAttachment, LowCutFreqSliderAttachment, HighCutFreqSliderAttachment, LowCutSlopeSliderAttachment, HighCutSlopeSliderAttachment;

    std::vector<juce::Component*> GetSliderComponents();

    // Chain instance for visualizer
    MonoChain monoChain;

    juce::Atomic<bool> ParametersChanged = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessorEditor)
};
