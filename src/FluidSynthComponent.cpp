#include "FluidSynthComponent.h"
// #include "juce_gui_basics/widgets/juce_Slider.h"
#include "../JuceLibraryCode/JuceHeader.h"

FluidSynthComponent::FluidSynthComponent(juce::AudioProcessorValueTreeState& vts)
    : valueTreeState(vts) {
  // these define the parameters of our slider object
  midiVolumeSlider.setSliderStyle(juce::Slider::LinearBarVertical);
  midiVolumeSlider.setRange(0.0, 127.0, 1.0);
  midiVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
  midiVolumeSlider.setPopupDisplayEnabled(true, false, this);
  midiVolumeSlider.setTextValueSuffix(" Volume");
  midiVolumeSlider.setValue(1.0);

  // this function adds the slider to the editor
  addAndMakeVisible(&midiVolumeSlider);

  sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      valueTreeState, "fs.midiVolume", midiVolumeSlider);

  // add the listener to the slider
  //  midiVolumeSlider.addListener(this);
}

void FluidSynthComponent::updateControls() {
}

void FluidSynthComponent::resized() {
  printf("FluidSynthComponent::resized\n");
  //  midiVolume.setBounds(/* appropriate bounds */);
  midiVolumeSlider.setBounds(40, 30, 20, getHeight() - 60);
}

void FluidSynthComponent::sliderValueChanged(juce::Slider* slider) {
  printf("slider value changed");
  midiVolumeSlider.getValue();
  auto* param = valueTreeState.getParameter("fs.midiVolume");
  if (param != nullptr) {
    // Get the current value of the parameter
    printf(" value is: %f\n", param->getValue());
  } else {
    printf(" null val");
  }

  //  audioProcessor.noteOnVel = midiVolume.getValue();
}

void FluidSynthComponent::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::purple);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}