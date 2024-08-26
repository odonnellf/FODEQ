#pragma once

#include <JuceHeader.h>

struct ChainSettings
{
    float PeakFreq = 0.f;
    float PeakGainInDecibels = 0.f;
    float PeakQuality = 1.f;
    float LowCutFreq = 0.f;
    float HighCutFreq = 0.f;
    int LowCutSlope = 0;
    int HighCutSlope = 0;
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

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FODEQAudioProcessor)
};
