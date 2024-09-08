#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(FODEQAudioProcessor& p) : 
    audioProcessor(p)
{
	// Listen for when parameters change
	const auto& AudioProcessorParameters = audioProcessor.getParameters();
	for (auto Parameter : AudioProcessorParameters)
	{
		Parameter->addListener(this);
	}

	startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
	const auto& AudioProcessorParameters = audioProcessor.getParameters();
	for (auto Parameter : AudioProcessorParameters)
	{
		Parameter->removeListener(this);
	}
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	// Set our atomic flag to true
	ParametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
	// Query the atomic flag to decide if the chain needs updating and our component needs to be repainted
	constexpr bool NewValue = false;
	constexpr bool ValueToCompare = true;
	if (ParametersChanged.compareAndSetBool(NewValue, ValueToCompare))
	{
		// Update the mono chain 
		auto SampleRate = audioProcessor.getSampleRate();
		auto ChainSettings = GetChainSettings(audioProcessor.ValueTreeState);

		auto PeakCoefficients = MakePeakFilter(ChainSettings, SampleRate);
		SetCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, PeakCoefficients);

		auto LowCutCoefficients = MakeLowCutFilter(ChainSettings, SampleRate);
		auto HighCutCoefficients = MakeHighCutFilter(ChainSettings, SampleRate);

		UpdateCutFilter(monoChain.get<ChainPositions::LowCut>(), LowCutCoefficients, ChainSettings.LowCutSlope);
		UpdateCutFilter(monoChain.get<ChainPositions::HighCut>(), HighCutCoefficients, ChainSettings.HighCutSlope);

		// Signal a repaint so a new response curve gets drawn
		repaint();
	}
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
	using namespace juce;
	g.fillAll(Colours::black);

	// Draw visualizer
	// Get plugin area bounds
	auto FreqResponseArea = getLocalBounds();
	auto Width = FreqResponseArea.getWidth();

	// Get our filter chain elements
	auto& LowCut = monoChain.get<ChainPositions::LowCut>();
	auto& Peak = monoChain.get<ChainPositions::Peak>();
	auto& HighCut = monoChain.get<ChainPositions::HighCut>();
	auto SampleRate = audioProcessor.getSampleRate();

	// Iterate through each pixel and compute the magnitude at that frequency. Magnitude's
	// expressed as gain units which are multiplicative (not additive like with decibels).
	std::vector<double> Magnitudes;
	Magnitudes.resize(Width); // One magnitude per pixel
	for (int i = 0; i < Width; ++i)
	{
		// Map from pixel space to frequency space (mapping the normalized pixel number to its frequency within the human hearing range)
		double Magnitude = 1.f; // Starting gain
		auto MinRange = 20.0;
		auto MaxRange = 20000.0;
		auto Frequency = mapToLog10(double(i) / double(Width), MinRange, MaxRange);

		// Check if the peak band is bypassed
		if (!monoChain.isBypassed<ChainPositions::Peak>())
			Magnitude *= Peak.coefficients->getMagnitudeForFrequency(Frequency, SampleRate);

		if (!LowCut.isBypassed<0>())
			Magnitude *= LowCut.get<0>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!LowCut.isBypassed<1>())
			Magnitude *= LowCut.get<1>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!LowCut.isBypassed<2>())
			Magnitude *= LowCut.get<2>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!LowCut.isBypassed<3>())
			Magnitude *= LowCut.get<3>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);

		if (!HighCut.isBypassed<0>())
			Magnitude *= HighCut.get<0>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!HighCut.isBypassed<1>())
			Magnitude *= HighCut.get<1>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!HighCut.isBypassed<2>())
			Magnitude *= HighCut.get<2>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);
		if (!HighCut.isBypassed<3>())
			Magnitude *= HighCut.get<3>().coefficients->getMagnitudeForFrequency(Frequency, SampleRate);

		// Convert the magnitude to decibels and store it
		Magnitudes[i] = Decibels::gainToDecibels(Magnitude);
	}

	// Convert vector to path which we can draw
	Path ResponseCurve;

	// Define max and min positions in the window
	const double OutputMin = FreqResponseArea.getBottom();
	const double OutputMax = FreqResponseArea.getY();

	// Peak control can go from +24 to -24, so we want our response curve window to have this range
	const double TargetRangeMin = -24.0;
	const double TargetRangeMax = 24.0;

	auto Map = [&](double Input)
		{
			return jmap(Input, TargetRangeMin, TargetRangeMax, OutputMin, OutputMax);
		};

	ResponseCurve.startNewSubPath(FreqResponseArea.getX(), Map(Magnitudes.front()));
	for (size_t i = 1; i < Magnitudes.size(); ++i)
	{
		ResponseCurve.lineTo(FreqResponseArea.getX() + i, Map(Magnitudes[i]));
	}

	// Draw background border
	const float CornerSize = 4.f;
	const float LineThickness = 1.f;
	g.setColour(Colours::orange);
	g.drawRoundedRectangle(FreqResponseArea.toFloat(), CornerSize, LineThickness);

	// Draw path
	float StrokeThickness = 2.f;
	PathStrokeType StrokeType(StrokeThickness);
	g.setColour(Colours::white);
	g.strokePath(ResponseCurve, StrokeType);
}

//==============================================================================
FODEQAudioProcessorEditor::FODEQAudioProcessorEditor (FODEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
	responseCurveComponent(audioProcessor),
    PeakFreqSliderAttachment(audioProcessor.ValueTreeState, "Peak Freq", PeakFreqSlider),
    PeakGainSliderAttachment(audioProcessor.ValueTreeState, "Peak Gain", PeakGainSlider),
    PeakQualitySliderAttachment(audioProcessor.ValueTreeState, "Peak Quality", PeakQualitySlider),
    LowCutFreqSliderAttachment(audioProcessor.ValueTreeState, "LowCut Freq", LowCutFreqSlider),
    HighCutFreqSliderAttachment(audioProcessor.ValueTreeState, "HighCut Freq", HighCutFreqSlider),
    LowCutSlopeSliderAttachment(audioProcessor.ValueTreeState, "LowCut Slope", LowCutSlopeSlider),
    HighCutSlopeSliderAttachment(audioProcessor.ValueTreeState, "HighCut Slope", HighCutSlopeSlider)
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
    using namespace juce;
    g.fillAll (Colours::black);
}

void FODEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto BoundingBox = getLocalBounds();

	// Reserve the top third of the display for showing the frequency response of the filter chain
    // Reserve the bottom two thirds of the display for the placement of all of our sliders
    auto FreqResponseArea = BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.33);

	responseCurveComponent.setBounds(FreqResponseArea);

    auto LowCutArea = BoundingBox.removeFromLeft(BoundingBox.getWidth() * 0.33);
    auto HighCutArea = BoundingBox.removeFromRight(BoundingBox.getWidth() * 0.5);

    LowCutFreqSlider.setBounds(LowCutArea.removeFromTop(LowCutArea.getHeight() * 0.5));
    LowCutSlopeSlider.setBounds(LowCutArea);

    HighCutFreqSlider.setBounds(HighCutArea.removeFromTop(HighCutArea.getHeight() * 0.5));
    HighCutSlopeSlider.setBounds(HighCutArea);

    PeakFreqSlider.setBounds(BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.33));
    PeakGainSlider.setBounds(BoundingBox.removeFromTop(BoundingBox.getHeight() * 0.5));
    PeakQualitySlider.setBounds(BoundingBox);
}

std::vector<juce::Component*> FODEQAudioProcessorEditor::GetSliderComponents()
{
    return { &PeakFreqSlider, &PeakGainSlider, &PeakQualitySlider, &LowCutFreqSlider, &HighCutFreqSlider, &LowCutSlopeSlider, &HighCutSlopeSlider, &responseCurveComponent };
}
