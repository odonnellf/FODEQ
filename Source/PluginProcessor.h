#pragma once

#include <JuceHeader.h>

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
    juce::AudioProcessorValueTreeState AudioProcessorValueTreeState {*this, nullptr, "Parameters", CreateParameterLayout()};

private:
    // Type aliases (since the DSP namespace uses a lot of nested namespaces)
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    MonoChain LeftChannelChain;
    MonoChain RightChannelChain;

    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    void UpdatePeakFilter(const ChainSettings& ChainSettings);

    using Coefficients = Filter::CoefficientsPtr;
    static void UpdateCoefficients(Coefficients& Old, const Coefficients& Replacements);

    template<typename ChainType, typename CoefficientType>
    void UpdateCutFilter(ChainType& LowCut, const CoefficientType& CutCoefficients, const Slope& CutSlope)
    {
        LowCut.setBypassed<0>(true);
        LowCut.setBypassed<1>(true);
        LowCut.setBypassed<2>(true);
        LowCut.setBypassed<3>(true);

		switch (CutSlope)
		{
		case Slope_12:
		{
			// If our order is 2 (meaning a 12db/oct slope), CutCoeffecients has 1 value.
			// We will assign our coefficients to the first link in the cut filter chain and also stop bypassing it
			*LowCut.get<0>().coefficients = *CutCoefficients[0];
            LowCut.setBypassed<0>(false);
			break;
		}
		case Slope_24:
		{
			// If our order is 4 (meaning a 12db/oct slope), CutCoeffecients has 2 values.
			// We will assign our coefficients to the first two links in the cut filter chain and also stop bypassing them
			*LowCut.get<0>().coefficients = *CutCoefficients[0];
            LowCut.setBypassed<0>(false);
			*LowCut.get<1>().coefficients = *CutCoefficients[1];
            LowCut.setBypassed<1>(false);
			break;
		}
		case Slope_36:
		{
			*LowCut.get<0>().coefficients = *CutCoefficients[0];
            LowCut.setBypassed<0>(false);
			*LowCut.get<1>().coefficients = *CutCoefficients[1];
            LowCut.setBypassed<1>(false);
			*LowCut.get<2>().coefficients = *CutCoefficients[2];
            LowCut.setBypassed<2>(false);
			break;
		}
		case Slope_48:
		{
			*LowCut.get<0>().coefficients = *CutCoefficients[0];
            LowCut.setBypassed<0>(false);
			*LowCut.get<1>().coefficients = *CutCoefficients[1];
            LowCut.setBypassed<1>(false);
			*LowCut.get<2>().coefficients = *CutCoefficients[2];
            LowCut.setBypassed<2>(false);
			*LowCut.get<3>().coefficients = *CutCoefficients[3];
            LowCut.setBypassed<3>(false);
			break;
		}
		}
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessor)
};
