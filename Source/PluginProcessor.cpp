/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

static const juce::String LowCutParameterId = "LowCut Freq";
static const juce::String LowCutParameterName = "LowCut Freq";
static const juce::String HighCutParameterId = "HighCut Freq";
static const juce::String HighCutParameterName = "HighCut Freq";
static const juce::String PeakFreqParameterId = "Peak Freq";
static const juce::String PeakFreqParameterName = "Peak Freq";
static const juce::String PeakGainParameterId = "Peak Gain";
static const juce::String PeakGainParameterName = "Peak Gain";
static const juce::String PeakQualityParameterId = "Peak Quality";
static const juce::String PeakQualityParameterName = "Peak Quality";
static const juce::String LowCutSlopeParameterId = "LowCut Slope";
static const juce::String LowCutSlopeParameterName = "LowCut Slope";
static const juce::String HighCutSlopeParameterId = "HighCut Slope";
static const juce::String HighCutSlopeParameterName = "HighCut Slope";

//==============================================================================
FODEQAudioProcessor::FODEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FODEQAudioProcessor::~FODEQAudioProcessor()
{
}

//==============================================================================
const juce::String FODEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FODEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FODEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FODEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FODEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FODEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FODEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FODEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FODEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void FODEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FODEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // We need to prepare the filters before we use them. We do this via a ProcessSpec object
    // which gets passed to each channel chain (and subsequently to each link in each chain).
    juce::dsp::ProcessSpec ProcessSpec;
    ProcessSpec.maximumBlockSize = samplesPerBlock; // Max num of samples it will process at one time
    ProcessSpec.numChannels = 1; // Mono chains so one channel;
    ProcessSpec.sampleRate = sampleRate;

    // Pass the spec to each chain to prepare for processing
    LeftChannelChain.prepare(ProcessSpec);
    RightChannelChain.prepare(ProcessSpec);

    UpdateFilters();
}

void FODEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FODEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FODEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Always update parameters *before* we process audio through them
    UpdateFilters();

    // Processor chain requires a processing context to get passed to it in order to run audio through the
    // links in the chain. To create a processing context we must supply it with an AudioBlock instance.
    juce::dsp::AudioBlock<float> AudioBlock(buffer);
    
    // Extract the left and right channel from the buffer (channels 0 and 1)
    auto LeftBlock = AudioBlock.getSingleChannelBlock(0);
    auto RightBlock = AudioBlock.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> LeftContext(LeftBlock);
    juce::dsp::ProcessContextReplacing<float> RightContext(RightBlock);

    LeftChannelChain.process(LeftContext);
    RightChannelChain.process(RightContext);
}

//==============================================================================
bool FODEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FODEQAudioProcessor::createEditor()
{
    return new FODEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FODEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    // Write the plugin state to the memory block
    constexpr bool bAppendToExistingBlockContent = true;
    juce::MemoryOutputStream MemOutputStream(destData, bAppendToExistingBlockContent);
    ValueTreeState.state.writeToStream(MemOutputStream);
}

void FODEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    // Restore the plugin state from memory
    auto ValueTree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (ValueTree.isValid())
    {
        // Replace plugin state and update filters with saved parameter values
        ValueTreeState.replaceState(ValueTree);
        UpdateFilters();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout FODEQAudioProcessor::CreateParameterLayout()
{
    // Three equalizer bands:
    // 1. Low-cut: controllable frequency cut-off and slope of the cut-off
    // 2. High-cut: controllable frequency cut-off and slope of the cut-off
    // 3. Parametric/Peak: controllable frequency, gain and quality (narrow or wide)

    juce::AudioProcessorValueTreeState::ParameterLayout Layout;

	const float RangeStart = 20.f; // In Hz
	const float RangeEnd = 20000.f; // In Hz
	const float IntervalValue = 1.f; // Slider will change parameter value in steps of 1
	const float SkewFactor = 0.25f; // Alters the way values are distributes across the range (e.g if we wanted the first half of the slider to cover more/less than just the first half of the range)
    auto NormalRange = juce::NormalisableRange<float>(RangeStart, RangeEnd, IntervalValue, SkewFactor);

    // 1. Low-cut
	const float LowCutDefaultValue = RangeStart;
    Layout.add(std::make_unique<juce::AudioParameterFloat>(LowCutParameterId, LowCutParameterName, NormalRange, LowCutDefaultValue));

    // 2. High-cut
    const float HighCutDefaultValue = RangeEnd;
    Layout.add(std::make_unique<juce::AudioParameterFloat>(HighCutParameterId, HighCutParameterName, NormalRange, HighCutDefaultValue));

    // 3. Peak
    // a) Peak Freq
	const float PeakFreqDefaultValue = 750.f;
    Layout.add(std::make_unique<juce::AudioParameterFloat>(PeakFreqParameterId, PeakFreqParameterName, NormalRange, PeakFreqDefaultValue));

    // b). Peak Gain
	const float PeakGainDefaultValue = 0.f;
    auto PeakGainNormalRange = juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f);
    Layout.add(std::make_unique<juce::AudioParameterFloat>(PeakGainParameterId, PeakGainParameterName, PeakGainNormalRange, PeakGainDefaultValue));

    // c). Peak Quality (Q) -> 0.1 - 10.0
    const float PeakQualityDefaultValue = 1.f;
    auto PeakQualityNormalRange = juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f);
    Layout.add(std::make_unique<juce::AudioParameterFloat>(PeakQualityParameterId, PeakQualityParameterName, PeakQualityNormalRange, PeakQualityDefaultValue));

    // Options for Low-cut and High-cut: changing steepness of filter cut
    juce::StringArray OptionsArray;
    const int NumOptions = 4;
    for (int i = 0; i < NumOptions; ++i)
    {
        juce::String Option;
        Option << (12 + i * 12);
        Option << " db/Oct";
        OptionsArray.add(Option);
    }

    const float CutSlopeDefaultValue = 0.f;
    Layout.add(std::make_unique<juce::AudioParameterChoice>(LowCutSlopeParameterId, LowCutSlopeParameterName, OptionsArray, CutSlopeDefaultValue));
    Layout.add(std::make_unique<juce::AudioParameterChoice>(HighCutSlopeParameterId, HighCutSlopeParameterName, OptionsArray, CutSlopeDefaultValue));

    return Layout;
}

void FODEQAudioProcessor::UpdatePeakFilter(const ChainSettings& ChainSettings)
{
	auto PeakGain = juce::Decibels::decibelsToGain(ChainSettings.PeakGainInDecibels);
	auto PeakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), ChainSettings.PeakFreq, ChainSettings.PeakQuality, PeakGain);

    SetCoefficients(LeftChannelChain.get<ChainPositions::Peak>().coefficients, PeakCoefficients);
    SetCoefficients(RightChannelChain.get<ChainPositions::Peak>().coefficients, PeakCoefficients);
}

void FODEQAudioProcessor::SetCoefficients(Coefficients& Old, const Coefficients& Replacements)
{
    *Old = *Replacements;
}

void FODEQAudioProcessor::UpdateLowCutFilters(const ChainSettings& ChainSettings)
{
	auto LowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(ChainSettings.LowCutFreq, getSampleRate(), 2 * (ChainSettings.LowCutSlope + 1));
	auto& LeftLowCut = LeftChannelChain.get<ChainPositions::LowCut>();
	auto& RightLowCut = RightChannelChain.get<ChainPositions::LowCut>();
	UpdateCutFilter(LeftLowCut, LowCutCoefficients, ChainSettings.LowCutSlope);
	UpdateCutFilter(RightLowCut, LowCutCoefficients, ChainSettings.LowCutSlope);
}

void FODEQAudioProcessor::UpdateHighCutFilters(const ChainSettings& ChainSettings)
{
	auto& HighCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(ChainSettings.HighCutFreq, getSampleRate(), 2 * (ChainSettings.HighCutSlope + 1));
	auto& LeftHighCut = LeftChannelChain.get<ChainPositions::HighCut>();
	auto& RightHighCut = RightChannelChain.get<ChainPositions::HighCut>();
	UpdateCutFilter(LeftHighCut, HighCutCoefficients, ChainSettings.HighCutSlope);
	UpdateCutFilter(RightHighCut, HighCutCoefficients, ChainSettings.HighCutSlope);
}

void FODEQAudioProcessor::UpdateFilters()
{
    auto ChainSettings = GetChainSettings(ValueTreeState);
    UpdateLowCutFilters(ChainSettings);
    UpdatePeakFilter(ChainSettings);
    UpdateHighCutFilters(ChainSettings);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FODEQAudioProcessor();
}

ChainSettings GetChainSettings(juce::AudioProcessorValueTreeState& ValueTreeState)
{
    ChainSettings Settings;

    Settings.LowCutFreq = ValueTreeState.getRawParameterValue(LowCutParameterName)->load();
    Settings.HighCutFreq = ValueTreeState.getRawParameterValue(HighCutParameterName)->load();
    Settings.PeakFreq = ValueTreeState.getRawParameterValue(PeakFreqParameterName)->load();
    Settings.PeakGainInDecibels = ValueTreeState.getRawParameterValue(PeakGainParameterName)->load();
    Settings.PeakQuality = ValueTreeState.getRawParameterValue(PeakQualityParameterName)->load();
    Settings.LowCutSlope = static_cast<Slope>(ValueTreeState.getRawParameterValue(LowCutSlopeParameterName)->load());
    Settings.HighCutSlope = static_cast<Slope>(ValueTreeState.getRawParameterValue(HighCutSlopeParameterName)->load());

    return Settings;
}
