/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ExprParser.h"
#include "GuiConst.h"

using namespace std;

//==============================================================================
RibCrusherAudioProcessorEditor::RibCrusherAudioProcessorEditor (RibCrusherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
    logoImg = juce::ImageFileFormat::loadFrom(BinaryData::logo_png, BinaryData::logo_pngSize);

    // Bitdepth slider
    bitDepthSlider.setName("Bit Depth");
    bitDepthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bitDepthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    bitDepthSlider.setTextBoxIsEditable(true);
    bitDepthSlider.setTextValueSuffix("bits");
    bitDepthSlider.setSkewFactorFromMidPoint(7);

    bitDepthSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey);
    bitDepthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    addAndMakeVisible(&bitDepthSlider);

    bitDepthLabel.setText("Bit depth", juce::dontSendNotification);
    bitDepthLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    bitDepthLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(bitDepthLabel);

    // Sample rate slider
    sampleRateSlider.setName("Sample Rate");
    sampleRateSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sampleRateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 70, 20);
    sampleRateSlider.setTextBoxIsEditable(true);
    sampleRateSlider.setTextValueSuffix("Hz");
    sampleRateSlider.setSkewFactorFromMidPoint(2200.0f);

    sampleRateSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey);
    sampleRateSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    addAndMakeVisible(&sampleRateSlider);

    sampleRateLabel.setText("Sample rate", juce::dontSendNotification);
    sampleRateLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    sampleRateLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(&sampleRateLabel);

    // Bit shift slider
    bitShiftSlider.setName("Bit shift");
    bitShiftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bitShiftSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    bitShiftSlider.setTextBoxIsEditable(true);

    bitShiftSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey);
    bitShiftSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    addAndMakeVisible(&bitShiftSlider);

    bitShiftLabel.setText("<<", juce::dontSendNotification);
    bitShiftLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    bitShiftLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(bitShiftLabel);

    // Mix slider
    mixSlider.setName("mix");
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 20);
    mixSlider.setTextBoxIsEditable(true);

    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkgrey);
    mixSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    addAndMakeVisible(&mixSlider);

    mixLabel.setText("Dry/Wet mix", juce::dontSendNotification);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    mixLabel.setJustificationType(juce::Justification::bottom);
    addAndMakeVisible(mixLabel);

    // SliderAttachment (AudioProcessorValueTreeState &stateToUse, const String &parameterID, Slider &slider)
    bitDepthAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BITDEPTH", bitDepthSlider);
    sampleRateAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SAMPLERATE", sampleRateSlider);
    ditherAttachment = make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "DITHER", ditherButton);
    wrapToggleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "BYTEWRAP", wrapToggle);
    bitShiftAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "BITSHIFT", bitShiftSlider);
    mixAttachment = make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);
    
    ditherButton.setName("Dither");
    ditherButton.setToggleState(true, juce::dontSendNotification);
    ditherButton.setButtonText("Dither");
    ditherButton.setClickingTogglesState(true);
    ditherButton.setTooltip("Dither on/off");

    ditherButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
    addAndMakeVisible(&ditherButton);

    wrapToggle.setButtonText("Mask output to 8 bits");
    wrapToggle.setToggleState(true, juce::dontSendNotification);
    wrapToggle.setClickingTogglesState(true);
    addAndMakeVisible(wrapToggle);

    infoButton.setColour(juce::TextButton::textColourOnId, juce::Colours::orange);
    addAndMakeVisible(infoButton);
    infoButton.onClick = [this]()
    {
        juce::AlertWindow::showMessageBoxAsync(
          juce::AlertWindow::InfoIcon,
          "Guide",
          "Example formula: x+t*(42&t>>10)\n"
          "You can include variables x (audio input) and t (bytebeat increasing index),"
          "integers and operators listed below in your formulas.\n\n"
          "Operators available:\n\n"
          "- Functions sin()/cos()\n"
          "- Bitwise negation ~\n"
          "- Multiplication, division, modulus * / %\n"
          "- Addition, subtraction + -\n"
          "- Left/right bit shift << >>\n"
          "- Less/greater than (or equal to) < <= > >=\n"
          "- (not) equal to == !=\n"
          "- Bitwise AND &\n"
          "- Bitwise exclusive OR ^\n"
          "- Bitwise inclusive OR |\n"
          "For help, visit: https://github.com/viljavai/RibCrusher"
      );
    };

    addAndMakeVisible(errorLabel);

    exprEditor.setMultiLine(false);
    exprEditor.setColour(juce::TextEditor::textColourId, juce::Colours::orange);
    exprEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::orange);
    exprEditor.setColour(juce::TextEditor::shadowColourId, juce::Colours::black);

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
        audioProcessor.tCount = 0;
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
    g.setFont (20.0f);

    g.drawImage(logoImg, 350, 18, 193, 40, 0, 0, logoImg.getWidth(), logoImg.getHeight());
  }

void RibCrusherAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    bitDepthSlider.setBounds(SLIDERWIDTH, SLIDERHEIGHT, SLIDERWIDTH, SLIDERHEIGHT);
    sampleRateSlider.setBounds(SLIDERWIDTH*2, SLIDERHEIGHT, SLIDERWIDTH, SLIDERHEIGHT);
    bitShiftSlider.setBounds(SLIDERWIDTH*3, SLIDERHEIGHT, SLIDERWIDTH, SLIDERHEIGHT);
    mixSlider.setBounds(SLIDERWIDTH*4, SLIDERHEIGHT, SLIDERWIDTH, SLIDERHEIGHT);

    bitDepthLabel.setBounds(SLIDERWIDTH+SMALLPADDING*4, SLIDERHEIGHT+SLIDERHEIGHT+SMALLPADDING, SLIDERWIDTH, LABELHEIGHT);
    sampleRateLabel.setBounds(SLIDERWIDTH*2+SMALLPADDING, SLIDERHEIGHT+SLIDERHEIGHT+SMALLPADDING, SLIDERWIDTH, LABELHEIGHT);
    bitShiftLabel.setBounds(SLIDERWIDTH*3+SMALLPADDING*7, SLIDERHEIGHT+SLIDERHEIGHT+SMALLPADDING, SLIDERWIDTH, LABELHEIGHT);
    mixLabel.setBounds(SLIDERWIDTH*4+SMALLPADDING, SLIDERHEIGHT+SLIDERHEIGHT+SMALLPADDING, SLIDERWIDTH, LABELHEIGHT);

    ditherButton.setBounds(SLIDERWIDTH+SMALLPADDING*4, SLIDERHEIGHT+SLIDERHEIGHT+LABELHEIGHT+SMALLPADDING*2, SLIDERWIDTH, LABELHEIGHT);

    exprEditor.setBounds(20, 20, 300, 30);

    errorLabel.setBounds(18, 50, 300, 30); // Position below exprEditor
    errorLabel.setColour(juce::Label::textColourId, juce::Colours::orange);

    infoButton.setBounds(getWidth() - 45, getHeight()-40, 30, 30);
    wrapToggle.setBounds(20, 350, 150, 24);

  }
