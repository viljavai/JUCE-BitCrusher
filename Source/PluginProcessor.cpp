/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RibCrusherAudioProcessor::RibCrusherAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                     // initialization list for the AudioProcessorValueTreeState
                     // AudioProcessorValueTreeState (AudioProcessor &processorToConnectTo, UndoManager *undoManagerToUse, 
                     // const Identifier &valueTreeType, ParameterLayout parameterLayout)
                     //================================================================
 	                 // Creates a state object for a given processor, and sets up all the parameters that will control that processor.
                       ), apvts(*this, nullptr, "Parameters", createParameterLayout())
                    // we have no undo manager, so we pass nullptr
#endif
{
}

RibCrusherAudioProcessor::~RibCrusherAudioProcessor()
{
}

//==============================================================================
const juce::String RibCrusherAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RibCrusherAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RibCrusherAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RibCrusherAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RibCrusherAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RibCrusherAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RibCrusherAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RibCrusherAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RibCrusherAudioProcessor::getProgramName (int index)
{
    return {};
}

void RibCrusherAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RibCrusherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    hostSamplerate = sampleRate;
    auto channelsNum = getTotalNumInputChannels();
    sampleCount.assign(channelsNum,0.0f);
    currentSamples.assign(channelsNum,0.0f);
    
}

void RibCrusherAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RibCrusherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void RibCrusherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    int bitDepthVal = apvts.getRawParameterValue("BITDEPTH")->load();
    int samplerateVal = apvts.getRawParameterValue("SAMPLERATE")->load();
    bool ditherEnabled = apvts.getRawParameterValue("DITHER")->load();

    //std::cout << hostSamplerate << "\n";

    int N = int(hostSamplerate/samplerateVal);
    //std::cout << N << "\n";

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i=totalNumInputChannels; i<totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    //======================================================//

    for (int channel=0; channel<totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample=0; sample<buffer.getNumSamples(); ++sample)
        {   
            // downsampling, sample and hold
            // return sample every N samples i.e for chunk of N samples hold the first value of this chunk for duration of the chunk
            // https://forum.juce.com/t/seeking-help-with-free-ratio-downsampler-plugin-dsp/18344/3
            float heldSample = channelData[sample];

            // we don't have a sample held, capture current sample
            if (sampleCount[channel] == 0) {
                currentSamples[channel] = heldSample;
            }
            // repeat heldSample for N samples
            channelData[sample] = currentSamples[channel];

            if (++sampleCount[channel] >= N) {
                // release
                sampleCount[channel] = 0;
            }

            float ditherVal = 0.0f;
            if (ditherEnabled) {
                // TPDF dithering
                // https://robin-prillwitz.de/misc/tpdf/tpdf.html
                // scale ditherVal by one quantization step
                float scalingFactor = 1.0f / (1 << bitDepthVal);
                ditherVal = (random.nextFloat() - random.nextFloat()) * scalingFactor;
            }
            float ditheredSample = channelData[sample] + ditherVal;

            // Change bit depth (with dither)
            // We want int values (signed n-bit int) between 2^(bitDepthVal-1)-1 and -(2^(bitDepthVal-1))
            // ex. bitDepthVal=8 -> int values between 127 and -128 (-127)
            int maxVal = (1 << (bitDepthVal - 1)) - 1;
            // quantization:
            // round incoming floats to the nearest int value in range [-maxVal, maxVal]
            int intSample = int(juce::roundToInt(ditheredSample * maxVal));
            // output buffer eats floats, so static cast
            float quantSample = static_cast<float>(intSample) / maxVal;

            // substractive dither done here
            if (ditherEnabled) {
                channelData[sample] = quantSample - ditherVal;
            }
            else {
                channelData[sample] = quantSample;
            }
        }
    }
}

//==============================================================================
bool RibCrusherAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RibCrusherAudioProcessor::createEditor()
{
    return new RibCrusherAudioProcessorEditor (*this);
}

//==============================================================================
void RibCrusherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RibCrusherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RibCrusherAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout RibCrusherAudioProcessor::createParameterLayout()
    {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    // AudioParameterInt is a type of RangedAudioParameter
    params.push_back(std::make_unique<juce::AudioParameterInt>("BITDEPTH", "Bitdepth", 2, 16, 16));
    params.push_back(std::make_unique<juce::AudioParameterBool>("DITHER", "Dither", true));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAMPLERATE", "Sample rate", 110.0f, 44100.0f, 44100.0f));
    return { params.begin(), params.end() };
    }