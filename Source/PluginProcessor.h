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
    static void SetCoefficients(Coefficients& Old, const Coefficients& Replacements);
    template<int Index, typename ChainType, typename CoefficientType>
    void UpdateCoefficient(ChainType& Chain, const CoefficientType& Coefficients)
    {
        SetCoefficients(Chain.get<Index>().coefficients, Coefficients[Index]);
        Chain.setBypassed<Index>(false);
    }

    template<typename ChainType, typename CoefficientType>
    void UpdateCutFilter(ChainType& Chain, const CoefficientType& Coefficients, const Slope& Slope)
    {
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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessor)
};
