#pragma once

#include <JuceHeader.h>

// Type aliases (since the DSP namespace uses a lot of nested namespaces)
using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions
{
	LowCut,
	Peak,
	HighCut
};

enum Slope
{
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};

struct ChainSettings
{
	float PeakFreq = 0.f;
	float PeakGainInDecibels = 0.f;
	float PeakQuality = 1.f;
	float LowCutFreq = 0.f;
	float HighCutFreq = 0.f;
	Slope LowCutSlope = Slope::Slope_12;
	Slope HighCutSlope = Slope::Slope_12;
};

ChainSettings GetChainSettings(juce::AudioProcessorValueTreeState& ValueTreeState);

using Coefficients = Filter::CoefficientsPtr;
void SetCoefficients(Coefficients& Old, const Coefficients& Replacements);

Coefficients MakePeakFilter(const ChainSettings& ChainSettings, double SampleRate);

template<int Index, typename ChainType, typename CoefficientType>
void UpdateCoefficient(ChainType& Chain, const CoefficientType& Coefficients)
{
	SetCoefficients(Chain.get<Index>().coefficients, Coefficients[Index]);
	Chain.setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void UpdateCutFilter(ChainType& Chain, const CoefficientType& Coefficients, const Slope& Slope)
{
	// Bypass all links in the chain, then assign coefficients to chain links based on the order number
	Chain.setBypassed<0>(true);
	Chain.setBypassed<1>(true);
	Chain.setBypassed<2>(true);
	Chain.setBypassed<3>(true);

	switch (Slope)
	{
		// Use case fallthrough
	case Slope_48:
	{
		UpdateCoefficient<3>(Chain, Coefficients);
	}
	case Slope_36:
	{
		UpdateCoefficient<2>(Chain, Coefficients);
	}
	case Slope_24:
	{
		UpdateCoefficient<1>(Chain, Coefficients);
	}
	case Slope_12:
	{
		UpdateCoefficient<0>(Chain, Coefficients);
	}
	}
}

inline auto MakeLowCutFilter(const ChainSettings& ChainSettings, double SampleRate)
{
	return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(ChainSettings.LowCutFreq, SampleRate, 2 * (ChainSettings.LowCutSlope + 1));
}

inline auto MakeHighCutFilter(const ChainSettings& ChainSettings, double SampleRate)
{
	return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(ChainSettings.HighCutFreq, SampleRate, 2 * (ChainSettings.HighCutSlope + 1));
}
//==============================================================================
/**
* A basic EQ 
*/
class FODEQAudioProcessor  : public juce::AudioProcessor
{
public:
	//==============================================================================
	FODEQAudioProcessor();
	~FODEQAudioProcessor() override;

	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

	void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const juce::String getProgramName (int index) override;
	void changeProgramName (int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation (juce::MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;

	// AudioProcessorValueTreeState setup
	static juce::AudioProcessorValueTreeState::ParameterLayout CreateParameterLayout();
	juce::AudioProcessorValueTreeState ValueTreeState {*this, nullptr, "Parameters", CreateParameterLayout()};

private:
	MonoChain LeftChannelChain;
	MonoChain RightChannelChain;

	void UpdatePeakFilter(const ChainSettings& ChainSettings);
	void UpdateLowCutFilters(const ChainSettings& ChainSettings);
	void UpdateHighCutFilters(const ChainSettings& ChainSettings);

	void UpdateFilters();

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessor)
};
