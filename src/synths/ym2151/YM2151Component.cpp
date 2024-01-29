#include "YM2151Component.h"

YM2151Component::YM2151Component(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
      //      synth(s),
      fileChooser(valueTreeState),
      presetButton("")
{
  addAndMakeVisible(fileChooser);
  addAndMakeVisible(presetButton);
  presetButton.onClick = [&]
  {
//    PopupMenu menu;
//    populatePresetMenu(menu);
//    menu.showMenuAsync (PopupMenu::Options{}.withTargetComponent (presetButton));
  };
}

YM2151Component::~YM2151Component() {
}

void YM2151Component::updateControls() {
}

void YM2151Component::resized() {
  printf("YM2151Component::resized\n");

  fileChooser.setBounds(30, 10, (getWidth() - 60) / 2, 30);
  presetButton.setBounds(30, 40, (getWidth() - 60) / 2, 30);
}

void YM2151Component::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::purple);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

  //  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void YM2151Component::parameterChanged(const String& parameterID, float newValue) {
}

void YM2151Component::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                                   const Identifier& property) {

}