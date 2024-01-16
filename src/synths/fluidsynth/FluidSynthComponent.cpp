#include "FluidSynthComponent.h"
#include "JuceHeader.h"
#include "FluidSynthSynth.h"


FluidSynthComponent::FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
      fileChooser(valueTreeState),
      attackSlider(vts, FluidSynthParam::ATTACK_RATE),
      holdSlider(vts, FluidSynthParam::HOLD),
      decaySlider(vts, FluidSynthParam::DECAY_RATE),
      sustainSlider(vts, FluidSynthParam::SUSTAIN_LEVEL),
      releaseSlider(vts, FluidSynthParam::RELEASE_RATE)
{
//      fileChooser("File", File(), true, false, false, "*.sf2;*.sf3", String(),
//       "Choose a Soundfont file to load into the synthesizer") {
//  midiVolumeSlider.component.setTextValueSuffix(" Volume");

  addAndMakeVisible(fileChooser);

  // Set the slider range values and add them to the UI
  for (auto& slider : {&attackSlider, &holdSlider, &decaySlider, &sustainSlider, &releaseSlider}) {
    auto& param = FluidSynthParam::paramIdToParam[slider->paramId];
    slider->component.setRange(param.min, param.max, 1);
    addAndMakeVisible(slider);
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

void FluidSynthComponent::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::purple);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}