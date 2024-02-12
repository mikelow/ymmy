#include "YM2151Component.h"

YM2151Component::YM2151Component(juce::AudioProcessorValueTreeState& valueTreeState)
    : SynthComponent(valueTreeState),
      //      synth(s),
      fileChooser(
          valueTreeState,
          { "opmFile", { { "path", "" } }, {} },
          "*.opm",
          "Select an OPM file"
      ),
      presetButton("")
{
  addAndMakeVisible(fileChooser);
  addAndMakeVisible(presetButton);
  presetButton.onClick = [&]
  {
    PopupMenu menu;
    populatePresetMenu(menu);
    menu.showMenuAsync (PopupMenu::Options{}.withTargetComponent (presetButton));
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

void YM2151Component::populatePresetMenu(PopupMenu& menu) {
  auto banks = vts.state.getChildWithName("ym2151.banks");
  int numBanks = banks.getNumChildren();
  auto selectedBankParam = vts.getParameter("ym2151.bank");
  auto selectedPresetParam = vts.getParameter("ym2151.preset");
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