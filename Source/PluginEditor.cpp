/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RibCrusherAudioProcessorEditor::RibCrusherAudioProcessorEditor (RibCrusherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    // Bitdepth slider
    bitDepthSlider.setName("Bit Depth");
    bitDepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bitDepthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    bitDepthSlider.setTextValueSuffix("bits");
    bitDepthSlider.setSkewFactorFromMidPoint(7);
    addAndMakeVisible(&bitDepthSlider);

    bitDepthLabel.setText("Bit depth", juce::dontSendNotification);
    bitDepthLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    bitDepthLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(bitDepthLabel);

    // Sample rate slider
    sampleRateSlider.setName("Sample Rate");
    sampleRateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sampleRateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    sampleRateSlider.setTextValueSuffix("Hz");
    sampleRateSlider.setSkewFactorFromMidPoint(2200.0f);
    addAndMakeVisible(&sampleRateSlider);

    sampleRateLabel.setText("Sample rate", juce::dontSendNotification);
    sampleRateLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    sampleRateLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(&sampleRateLabel);

    // SliderAttachment (AudioProcessorValueTreeState &stateToUse, const String &parameterID, Slider &slider)
    bitDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BITDEPTH", bitDepthSlider);
    sampleRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SAMPLERATE", sampleRateSlider);
    ditherAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DITHER", ditherButton);

    ditherButton.setName("Dither");
    ditherButton.setToggleState(true, juce::dontSendNotification);
    ditherButton.setButtonText("Dither");
    ditherButton.setClickingTogglesState(true);
    ditherButton.setTooltip("Dither on/off");
    addAndMakeVisible(&ditherButton);

}

RibCrusherAudioProcessorEditor::~RibCrusherAudioProcessorEditor()
{
}

//==============================================================================
void RibCrusherAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (15.0f));}

void RibCrusherAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    const int sliderWidth = 100;
    const int sliderHeight = 100;
    const int labelHeight = 20;
    const int buttonHeight = 30;
    const int spacing = 20;

    // Calculate positions
    const int totalWidth = (2 * sliderWidth) + spacing;
    const int startX = (getWidth() - totalWidth) / 2;
    const int centerY = getHeight() / 2;

    // Set bounds for sliders
    bitDepthSlider.setBounds(startX, centerY - sliderHeight / 2, sliderWidth, sliderHeight);
    sampleRateSlider.setBounds(startX + sliderWidth + spacing, centerY - sliderHeight / 2, sliderWidth, sliderHeight);

    // Set bounds for labels below sliders
    bitDepthLabel.setBounds(startX, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);
    sampleRateLabel.setBounds(startX + sliderWidth + spacing, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);

    // Set bounds for button
    ditherButton.setBounds((getWidth() - sliderWidth) / 2, centerY + sliderHeight / 2 + spacing, sliderWidth, buttonHeight);
}