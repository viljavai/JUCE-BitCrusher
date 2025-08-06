/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class RibCrusherAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RibCrusherAudioProcessorEditor (RibCrusherAudioProcessor&);
    ~RibCrusherAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    juce::Slider bitDepthSlider;
    juce::Slider sampleRateSlider;
    juce::Slider bitShiftSlider;
    //juce::Slider mixSlider;

    juce::ToggleButton ditherButton;

    juce::TextEditor exprEditor;

    //Slider object ^ should be declared before this
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ditherAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitShiftAttachment;
    //std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::Listener> editorAttachment;

    juce::Label bitDepthLabel;
    juce::Label sampleRateLabel;
    juce::Label bitShiftLabel;
    //juce::Label mixLabel;

    juce::Label ditherLabel;

    juce::Label errorLabel;

    RibCrusherAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RibCrusherAudioProcessorEditor)
};
