#pragma once

#include "YM2151Synth.h"
#include "../SynthComponent.h"
#include "../../components/FilePicker.h"
#include "../../components/YmmySlider.h"

class YM2151Component : public SynthComponent,
                        public AudioProcessorValueTreeState::Listener {
public:
  typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

  YM2151Component(juce::AudioProcessorValueTreeState& valueTreeState);
  ~YM2151Component();

  void updateControls() override;
  void resized() override;
  void populatePresetMenu(PopupMenu& menu);

  void parameterChanged(const String& parameterID, float newValue);
  void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property);

private:
  void paint (juce::Graphics& g) override;

  FilePicker fileChooser;
  TextButton presetButton;
};
