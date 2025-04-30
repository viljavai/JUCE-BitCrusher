/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/**
*/


class RibCrusherAudioProcessor  : public juce::AudioProcessor
{


public:
    //==============================================================================
    RibCrusherAudioProcessor();
    ~RibCrusherAudioProcessor() override;

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

    // https://www.youtube.com/watch?v=nkQPsYOdIrk&ab_channel=TheAudioProgrammer
    juce::AudioProcessorValueTreeState apvts;
    juce::Random random;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    // this functions as a counter for tracking repeating sample in downsampling
    // vector because number of channels is dynamic
    std::vector<int> sampleCount;
    // this stores the repeating sample in downsampling
    // vector because number of channels is dynamic
    // NOTE! this is a float [-1,1]
    std::vector<float> currentSamples;
    double hostSamplerate = 0.0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RibCrusherAudioProcessor)
};
