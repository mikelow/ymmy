#include "FluidSynthComponent.h"
#include "JuceHeader.h"
#include "FluidSynthSynth.h"
#include "fluidsynth.h"

FluidSynthComponent::FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
//      synth(s),
      fileChooser(valueTreeState),
      presetButton(""),
      attackSlider(vts, FluidSynthParam::ATTACK_RATE),
      holdSlider(vts, FluidSynthParam::HOLD),
      decaySlider(vts, FluidSynthParam::DECAY_RATE),
      sustainSlider(vts, FluidSynthParam::SUSTAIN_LEVEL),
      releaseSlider(vts, FluidSynthParam::RELEASE_RATE),
      sliders{&attackSlider, &holdSlider, &decaySlider, &sustainSlider, &releaseSlider},
      sliderParams {
          FluidSynthParam::parameters[2],
          FluidSynthParam::parameters[3],
          FluidSynthParam::parameters[4],
          FluidSynthParam::parameters[5],
          FluidSynthParam::parameters[6]
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
  addAndMakeVisible(presetButton);
  presetButton.onClick = [&]
  {
    PopupMenu menu;
    populatePresetMenu(menu);
    menu.showMenuAsync (PopupMenu::Options{}.withTargetComponent (presetButton));
  };

  // Set the slider range values and add them to the UI
  for (auto slider : sliders) {
    auto& param = FluidSynthParam::paramIdToParam[slider->paramId];
    slider->component.setRange(param.min, param.max, 1);
    addAndMakeVisible(slider);
  }

  for (const auto& param: sliderParams) {
    vts.addParameterListener(param.id, this);
  }
}

FluidSynthComponent::~FluidSynthComponent() {
}

void FluidSynthComponent::updateControls() {
}

void FluidSynthComponent::resized() {
  printf("FluidSynthComponent::resized\n");

  fileChooser.setBounds(30, 10, (getWidth() - 60) / 2, 30);
  presetButton.setBounds(30, 40, (getWidth() - 60) / 2, 30);

  int i = 0;
  for (auto slider : sliders) {
    slider->setBounds(40 + (i*30), 70, 20, getHeight() - 100);
    ++i;
  }
}

void FluidSynthComponent::populatePresetMenu(PopupMenu& menu) {;3
  auto banks = vts.state.getChildWithName("banks");
  int numBanks = banks.getNumChildren();
  auto selectedBankParam = vts.getParameter("fs.bank");
  auto selectedPresetParam = vts.getParameter("fs.preset");
  AudioParameterInt* selectedBank = dynamic_cast<AudioParameterInt*>(selectedBankParam);
  AudioParameterInt* selectedPreset = dynamic_cast<AudioParameterInt*>(selectedPresetParam);

  for (int b = 0; b < numBanks; ++b) {
    PopupMenu subMenu;

    ValueTree bank = banks.getChild(b);
    int numBankChildren = bank.getNumChildren();
    for (int c = 0; c < numBankChildren; c++) {
      auto preset = bank.getChild(c);
      int presetNum = preset.getProperty("num");
      String presetName = preset.getProperty("name");

      auto callback = [this, b, c, selectedBank, selectedPreset, presetName] {
        int prevBank = selectedBank->get();
        int prevPreset = selectedPreset->get();

        if (prevBank != b) {
          selectedBank->setValueNotifyingHost(b);
        }
        if (prevPreset != c) {
          auto range = selectedPreset->getNormalisableRange();
          selectedPreset->setValueNotifyingHost(range.convertTo0to1(c));
        }
        this->presetButton.setButtonText(presetName);
      };

      subMenu.addItem(String(presetNum) + " " + presetName, callback);
    }
    menu.addSubMenu("Bank " + String(b), subMenu);
  }
}

void FluidSynthComponent::paint(juce::Graphics& g) {
  // fill the whole window white
  g.fillAll(juce::Colours::purple);

  // set the current drawing colour to black
  g.setColour(juce::Colours::black);

  // set the font size and draw text to the screen
  g.setFont(15.0f);

//  g.drawFittedText("Midi Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void FluidSynthComponent::parameterChanged(const String& parameterID, float newValue) {
  printf("FluidSynthComponent::parameterChanged: %s\n", parameterID.toStdString().c_str());
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