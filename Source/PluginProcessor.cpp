/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ExprParser.h"

using namespace std;
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
                       ), 
                    apvts(*this, nullptr, "Parameters", createParameterLayout())
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

    juce::dsp::ProcessSpec spec = { sampleRate, static_cast<juce::uint32> (samplesPerBlock), static_cast<juce::uint32> (getMainBusNumOutputChannels())  };
    dryWetMixer.prepare(spec);
    
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
    int bitShiftVal = apvts.getRawParameterValue("BITSHIFT")->load();
    bool ditherEnabled = apvts.getRawParameterValue("DITHER")->load();

    int N = int(hostSamplerate/samplerateVal);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i=totalNumInputChannels; i<totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    //======================================================//

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    dryWetMixer.pushDrySamples(audioBlock);

    auto& tokens = parsedExpr;
    std::vector<uint32_t> stack;

    for (int channel=0; channel<totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample=0; sample<buffer.getNumSamples(); ++sample)
        {   
            // 1) expression parsing
            int inputInt = int(channelData[sample] * 127.5f + 128); // Map to 8-bit encoding [0,255]
            int bytebeatValue = evaluateExpr(parsedExpr, tCount++, inputInt);
            float sampleValue = (bytebeatValue & 0xFF) / 127.5f - 1.0f; // Map back to range [-1,1]

            // 2) downsampling, sample and hold
            // https://forum.juce.com/t/seeking-help-with-free-ratio-downsampler-plugin-dsp/18344/3
            // we don't have a sample held, capture current sample
            if (sampleCount[channel] == 0) {
                currentSamples[channel] = sampleValue;
            }
            // repeat heldSample for N samples
            channelData[sample] = currentSamples[channel];
            if (++sampleCount[channel] >= N) {
                // release
                sampleCount[channel] = 0;
            }

            float ditherVal = 0.0f;
            if (ditherEnabled) {
                // 3) TPDF dithering
                // https://robin-prillwitz.de/misc/tpdf/tpdf.html
                // scale ditherVal by one quantization step
                float scalingFactor = 1.0f / (1 << bitDepthVal);
                ditherVal = (random.nextFloat() - random.nextFloat()) * scalingFactor;
            }
            float ditheredSample = channelData[sample] + ditherVal;

            // 4) Change bit depth (with dither)
            // We want int values (signed n-bit int) between 2^(bitDepthVal-1)-1 and -(2^(bitDepthVal-1))
            // ex. bitDepthVal=8 -> int values between 127 and -128 (-127)
            int maxVal = (1 << (bitDepthVal - 1)) - 1;
            // quantization:
            // round incoming floats to the nearest int value in range [-maxVal, maxVal]
            int intSample = int(juce::roundToInt(ditheredSample * maxVal));
            // output buffer eats floats, so static cast
            float quantSample = static_cast<float>(intSample) / maxVal;

            // substractive dither done here
            float processedSample;
            if (ditherEnabled) {
                processedSample = quantSample - ditherVal;
            }
            else {
                processedSample = quantSample;
            }
            
            // 5) bit shift
            int shiftedInt = (intSample << bitShiftVal);
            shiftedInt = juce::jlimit(-maxVal, maxVal, shiftedInt);
            // normalize
            processedSample = float(shiftedInt) / float(maxVal);

            channelData[sample] = processedSample;
        }
    }
    float mixVal = apvts.getRawParameterValue("MIX")->load();
    dryWetMixer.setWetMixProportion(mixVal);
    dryWetMixer.mixWetSamples(audioBlock);
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

    juce::MemoryOutputStream stream(destData, false);
    apvts.state.writeToStream(stream);
}

void RibCrusherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    if (tree.isValid())
        apvts.replaceState(tree);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RibCrusherAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout RibCrusherAudioProcessor::createParameterLayout()
    {
    vector<unique_ptr<juce::RangedAudioParameter>> params;
    // AudioParameterInt is a type of RangedAudioParameter
    params.push_back(make_unique<juce::AudioParameterInt>("BITDEPTH", "Bitdepth", 2, 16, 16));
    params.push_back(make_unique<juce::AudioParameterBool>("DITHER", "Dither", true));
    params.push_back(make_unique<juce::AudioParameterFloat>("SAMPLERATE", "Sample rate", 110.0f, 44100.0f, 44100.0f));
    params.push_back(make_unique<juce::AudioParameterInt>("BITSHIFT", "Bitshift", 0, 64, 0));
    params.push_back(make_unique<juce::AudioParameterFloat>("MIX", "Mix", 0.0f, 1.0f, 0.2f));
    return { params.begin(), params.end() };
    }