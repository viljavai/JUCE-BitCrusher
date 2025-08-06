/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ExprParser.h"

using namespace std;

//==============================================================================
RibCrusherAudioProcessorEditor::RibCrusherAudioProcessorEditor (RibCrusherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);

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

    // Bit shift slider
    bitShiftSlider.setName("Bit shift");
    bitShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bitShiftSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(&bitShiftSlider);

    bitShiftLabel.setText("<<", juce::dontSendNotification);
    bitShiftLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    bitShiftLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(bitShiftLabel);

    // Mix slider
    mixSlider.setName("mix");
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    addAndMakeVisible(&mixSlider);

    mixLabel.setText("Dry/Wet mix", juce::dontSendNotification);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    mixLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(mixLabel);

    // SliderAttachment (AudioProcessorValueTreeState &stateToUse, const String &parameterID, Slider &slider)
    bitDepthAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BITDEPTH", bitDepthSlider);
    sampleRateAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SAMPLERATE", sampleRateSlider);
    ditherAttachment = make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DITHER", ditherButton);
    bitShiftAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BITSHIFT", bitShiftSlider);
    mixAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);
    
    ditherButton.setName("Dither");
    ditherButton.setToggleState(true, juce::dontSendNotification);
    ditherButton.setButtonText("Dither");
    ditherButton.setClickingTogglesState(true);
    ditherButton.setTooltip("Dither on/off");
    addAndMakeVisible(&ditherButton);

    addAndMakeVisible(errorLabel);

    exprEditor.setMultiLine(false);

    // on startup
    auto expr = audioProcessor.apvts.state.getProperty("expression", "x").toString();
    exprEditor.setText(expr, juce::dontSendNotification);
    audioProcessor.latestExpr = expr;
    try {
      audioProcessor.parsedExpr = shuntingYard(expr.toStdString());
    } catch (const std::exception& e) {
        errorLabel.setText(e.what(), juce::dontSendNotification);
    audioProcessor.parsedExpr = shuntingYard("x");
    exprEditor.setText("x", juce::dontSendNotification);
}

    exprEditor.onTextChange = [this]() {
      audioProcessor.latestExpr = exprEditor.getText();
      audioProcessor.apvts.state.setProperty("expression", exprEditor.getText(), nullptr);

      try {
        audioProcessor.parsedExpr = shuntingYard(exprEditor.getText().toStdString());
        errorLabel.setText("", juce::dontSendNotification);
      } catch (const exception& e) {
          errorLabel.setText(e.what(), juce::dontSendNotification);
      }
    };
  
    addAndMakeVisible(exprEditor);
}

RibCrusherAudioProcessorEditor::~RibCrusherAudioProcessorEditor()
{
}

//==============================================================================
void RibCrusherAudioProcessorEditor::paint (juce::Graphics& g)
{
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

    const int totalWidth = (2 * sliderWidth) + spacing;
    const int startX = (getWidth() - totalWidth) / 2;
    const int centerY = getHeight() / 2;

    bitDepthSlider.setBounds(startX, centerY - sliderHeight / 2, sliderWidth, sliderHeight);
    sampleRateSlider.setBounds(startX + sliderWidth + spacing, centerY - sliderHeight / 2, sliderWidth, sliderHeight);

    bitDepthLabel.setBounds(startX, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);
    sampleRateLabel.setBounds(startX + sliderWidth + spacing, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);

    bitShiftSlider.setBounds(startX + sliderWidth + 6*spacing, centerY - sliderHeight / 2, sliderWidth, sliderHeight);
    bitShiftLabel.setBounds(startX + sliderWidth + 6*spacing, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);

    mixSlider.setBounds(startX + sliderWidth + 12*spacing, centerY - sliderHeight / 2, sliderWidth, sliderHeight);
    mixLabel.setBounds(startX + sliderWidth + 12*spacing, centerY - sliderHeight / 2 + sliderHeight, sliderWidth, labelHeight);

    ditherButton.setBounds((getWidth() - sliderWidth) / 2, centerY + sliderHeight, sliderWidth, buttonHeight);

    exprEditor.setBounds(20, 20, 300, 30);

    errorLabel.setBounds(20, 60, 300, 30); // Position below exprEditor
    errorLabel.setColour(juce::Label::textColourId, juce::Colours::red);

  }
