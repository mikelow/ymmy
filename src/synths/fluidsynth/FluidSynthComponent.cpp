#include "FluidSynthComponent.h"
#include "JuceHeader.h"
#include "FluidSynthSynth.h"
#include "fluidsynth.h"

FluidSynthComponent::FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
//      synth(s),
      fileChooser(valueTreeState),
      attackSlider(vts, FluidSynthParam::ATTACK_RATE),
      holdSlider(vts, FluidSynthParam::HOLD),
      decaySlider(vts, FluidSynthParam::DECAY_RATE),
      sustainSlider(vts, FluidSynthParam::SUSTAIN_LEVEL),
      releaseSlider(vts, FluidSynthParam::RELEASE_RATE),
      sliders{&attackSlider, &holdSlider, &decaySlider, &sustainSlider, &releaseSlider},
      sliderParams{
          FluidSynthParam::parameters[0],
          FluidSynthParam::parameters[1],
          FluidSynthParam::parameters[2],
          FluidSynthParam::parameters[3],
          FluidSynthParam::parameters[4]
      },
      paramToComponent{
          {FluidSynthParam::ATTACK_RATE, &attackSlider},
          {FluidSynthParam::HOLD, &holdSlider},
          {FluidSynthParam::DECAY_RATE, &decaySlider},
          {FluidSynthParam::SUSTAIN_LEVEL, &sustainSlider},
          {FluidSynthParam::RELEASE_RATE, &releaseSlider},
      }
{
//      fileChooser("File", File(), true, false, false, "*.sf2;*.sf3", String(),
//       "Choose a Soundfont file to load into the synthesizer") {
//  midiVolumeSlider.component.setTextValueSuffix(" Volume");


  addAndMakeVisible(fileChooser);

  // Set the slider range values and add them to the UI
  for (auto slider : sliders) {
    auto& param = FluidSynthParam::paramIdToParam[slider->paramId];
    slider->component.setRange(param.min, param.max, 1);
    addAndMakeVisible(slider);
  }

  for (const auto& param: sliderParams) {
    vts.addParameterListener(param.id, this);
  }

//  std::map<juce::String, Component&> test = {
//      {FluidSynthParam::ATTACK_RATE, attackSlider},
//      {FluidSynthParam::HOLD, holdSlider},
//      {FluidSynthParam::DECAY_RATE, decaySlider},
//      {FluidSynthParam::SUSTAIN_LEVEL, sustainSlider},
//      {FluidSynthParam::RELEASE_RATE, releaseSlider},
//  };

//  vts.state.addListener(this);
}

FluidSynthComponent::~FluidSynthComponent() {
//  vts.state.removeListener(this);
}

void FluidSynthComponent::updateControls() {
}

void FluidSynthComponent::resized() {
  printf("FluidSynthComponent::resized\n");

  fileChooser.setBounds(30, 30, getWidth() - 60, 40);

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

void FluidSynthComponent::parameterChanged(const String& parameterID, float newValue) {

  if (paramToComponent.find(parameterID) == paramToComponent.end()) {
    return;
  }
  auto component = paramToComponent[parameterID];
  const auto slider = dynamic_cast<YmmySlider*>(component);
  if (slider) {
    slider->component.setValue(newValue);
  }
}

void FluidSynthComponent::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                               const Identifier& property) {
//  if (treeWhosePropertyHasChanged.getType() == StringRef("settings")) {
//    int selectedChannel = treeWhosePropertyHasChanged.getProperty("selectedChannel", 1);
//    if (property == StringRef("selectedChannel")) {
//      for (std::size_t i = 0; i < std::size(sliders); ++i) {
//        auto param = sliderParams[i];
//        auto test = fluid_synth_get_gen(synth->get(), selectedChannel, param.);
//      }
//
////      for (const auto &[param, genId]: paramToGenerator) {
////        //        vts.addParameterListener(param, this);
////        auto test = fluid_synth_get_gen(synth.get(), selectedChannel, genId);
////        printf("genID: %d: %f\n", genId, test);
////      }
//    }
//  }
}