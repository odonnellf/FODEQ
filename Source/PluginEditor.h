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

struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
    ResponseCurveComponent(FODEQAudioProcessor&);
    ~ResponseCurveComponent();

	// juce::AudioProcessorParameter::Listener interface
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

	// juce::Timer interface
	void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    FODEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> ParametersChanged = false;
    MonoChain monoChain;
};

// A basic example audio EQ plugin
class FODEQAudioProcessorEditor : public juce::AudioProcessorEditor
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

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    SliderAttachment PeakFreqSliderAttachment, PeakGainSliderAttachment, PeakQualitySliderAttachment, LowCutFreqSliderAttachment, HighCutFreqSliderAttachment, LowCutSlopeSliderAttachment, HighCutSlopeSliderAttachment;

    std::vector<juce::Component*> GetSliderComponents();

    ResponseCurveComponent responseCurveComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessorEditor)
};
