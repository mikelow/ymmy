#pragma once

#include "../SynthComponent.h"
#include "../../components/FilePicker.h"
#include "../../components/YmmySlider.h"

class FluidSynthComponent : public SynthComponent {
//                            private juce::Slider::Listener {
public:
  typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

  explicit FluidSynthComponent(juce::AudioProcessorValueTreeState& valueTreeState);
  ~FluidSynthComponent();

  void updateControls() override;
  void resized() override;

private:
//  void sliderValueChanged (juce::Slider* component) override;

  void paint (juce::Graphics& g) override;

//  YmmySlider midiVolumeSlider;
  FilePicker fileChooser;

  YmmySlider attackSlider;
  YmmySlider holdSlider;
  YmmySlider decaySlider;
  YmmySlider sustainSlider;
  YmmySlider releaseSlider;
//  juce::Slider midiVolumeSlider;

//  std::unique_ptr<SliderAttachment> sliderAttachment;
};
