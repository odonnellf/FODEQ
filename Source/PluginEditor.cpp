/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FODEQAudioProcessorEditor::FODEQAudioProcessorEditor (FODEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* SliderComponent : GetSliderComponents())
    {
        addAndMakeVisible(SliderComponent);
    }

    setSize (600, 400);
}

FODEQAudioProcessorEditor::~FODEQAudioProcessorEditor()
{
}

//==============================================================================
void FODEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void FODEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto BoundingBox = getLocalBounds();

	// Reserve the top third of the display for showing the frequency response of the filter chain
    // Reserve the bottom two thirds of the display for the placement of all of our sliders
    auto FreqResponseArea = BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.33);
    auto LowCutArea = BoundingBox.removeFromLeft(BoundingBox.getWidth() * 0.33);
    auto HighCutArea = BoundingBox.removeFromRight(BoundingBox.getWidth() * 0.5);

    LowCutFreqSlider.setBounds(LowCutArea.removeFromTop(LowCutArea.getHeight() * 0.5));
    LowCutSlopeSlider.setBounds(LowCutArea);

    HighCutFreqSlider.setBounds(HighCutArea.removeFromTop(LowCutArea.getHeight() * 0.5));
    HighCutSlopeSlider.setBounds(HighCutArea);

    PeakFreqSlider.setBounds(BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.33));
    PeakGainSlider.setBounds(BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.5));
    PeakQualitySlider.setBounds(BoundingBox);
}

std::vector<juce::Component*> FODEQAudioProcessorEditor::GetSliderComponents()
{
    return { &PeakFreqSlider, &PeakGainSlider, &PeakQualitySlider, &LowCutFreqSlider, &HighCutFreqSlider, &LowCutSlopeSlider, &HighCutSlopeSlider };
}
