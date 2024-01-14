#include "FluidSynthComponent.h"
// #include "juce_gui_basics/widgets/juce_Slider.h"
#include "JuceHeader.h"
#include "FluidSynthSynth.h"
#include "../../MidiConstants.h"


FluidSynthComponent::FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
//      midiVolumeSlider(vts, "fs.midiVolume"),
//      attackSlider(vts, "fs.attackRate"),
//      decaySlider(vts, "fs.decayRate"),
//      sustainSlider(vts, "fs.sustainLevel"),
//      releaseSlider(vts, "fs.releaseRate"),

//      midiVolumeSlider(vts, FluidSynthParam::MIDI_VOLUME),
      attackSlider(vts, FluidSynthParam::ATTACK_RATE),
      holdSlider(vts, FluidSynthParam::HOLD),
      decaySlider(vts, FluidSynthParam::DECAY_RATE),
      sustainSlider(vts, FluidSynthParam::SUSTAIN_LEVEL),
      releaseSlider(vts, FluidSynthParam::RELEASE_RATE),

//      fileChooser("File", File(), true, false, false, "*.sf2;*.sf3", String(),
//       "Choose a Soundfont file to load into the synthesizer") {
      fileChooser(valueTreeState) {
//  midiVolumeSlider.component.setTextValueSuffix(" Volume");

  addAndMakeVisible(fileChooser);

  for (auto& s : {&attackSlider, &holdSlider, &decaySlider, &sustainSlider, &releaseSlider}) {
    s->component.setRange(-12000, 8000, 1);
    addAndMakeVisible(s);
  }
}

FluidSynthComponent::~FluidSynthComponent() {

}

void FluidSynthComponent::updateControls() {
}

void FluidSynthComponent::resized() {
  printf("FluidSynthComponent::resized\n");

  fileChooser.setBounds(30, 30, getWidth() - 60, 40);

  //  midiVolume.setBounds(/* appropriate bounds */);
  auto sliders = {&attackSlider, &holdSlider, &decaySlider, &sustainSlider, &releaseSlider};
  int i = 0;
  for (auto slider : sliders) {
    slider->setBounds(40 + (i*30), 70, 20, getHeight() - 100);
    ++i;
  }
}

//void FluidSynthComponent::sliderValueChanged(juce::Slider* component) {
//  printf("component value changed");
//  midiVolumeSlider.getValue();
//  auto* param = vts.getParameter("fs.midiVolume");
//  if (param != nullptr) {
//    // Get the current value of the parameter
//    printf(" value is: %f\n", param->getValue());
//  } else {
//    printf(" null val");
//  }
//
//  //  audioProcessor.noteOnVel = midiVolume.getValue();
//}

void FluidSynthComponent::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::purple);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}