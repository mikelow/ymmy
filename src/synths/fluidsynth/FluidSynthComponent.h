#pragma once

#include "FluidSynthSynth.h"
#include "../SynthComponent.h"
#include "../../components/FilePicker.h"
#include "../../components/YmmySlider.h"

class FluidSynthComponent : public SynthComponent,
                            public AudioProcessorValueTreeState::Listener {
//                            public ValueTree::Listener,
//                            private juce::Slider::Listener {
public:
  typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

  FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState);
  ~FluidSynthComponent();

  void updateControls() override;
  void resized() override;

  void parameterChanged(const String& parameterID, float newValue);
  void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property);

private:
//  void sliderValueChanged (juce::Slider* component) override;

  void paint (juce::Graphics& g) override;

//  YmmySlider midiVolumeSlider;
//  fluid_synth_t* synth;

  FilePicker fileChooser;

  YmmySlider attackSlider;
  YmmySlider holdSlider;
  YmmySlider decaySlider;
  YmmySlider sustainSlider;
  YmmySlider releaseSlider;

  YmmySlider* sliders[5];
  IntParameter sliderParams[5];
  std::map<juce::String, Component*> paramToComponent;
//  juce::Slider midiVolumeSlider;

//  std::unique_ptr<SliderAttachment> sliderAttachment;
};
