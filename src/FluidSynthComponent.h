#pragma once

#include "SynthComponent.h"

class FluidSynthComponent : public SynthComponent,
                            private juce::Slider::Listener {
public:
  explicit FluidSynthComponent(juce::AudioProcessorValueTreeState& vts);

  void updateControls() override;
  void resized() override;

private:
  void sliderValueChanged (juce::Slider* slider) override;

  void paint (juce::Graphics& g);

  juce::AudioProcessorValueTreeState& valueTreeState;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
  juce::Slider midiVolumeSlider;
};
