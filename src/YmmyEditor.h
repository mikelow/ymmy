#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class YmmyProcessor;
class SynthComponent;

enum SynthType { FluidSynth, YM2151 };

class YmmyEditor : public juce::AudioProcessorEditor, private juce::Slider::Listener {
public:
  // Constants
  static constexpr int headerHeight = 50;

  YmmyEditor(YmmyProcessor&, juce::AudioProcessorValueTreeState&);
  ~YmmyEditor();

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  void setCurrentSynth(SynthType synthType);
  void sliderValueChanged(juce::Slider* slider) override;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  YmmyProcessor& audioProcessor;
  juce::AudioProcessorValueTreeState& valueTreeState;

  std::unique_ptr<SynthComponent> currentSynth;

  juce::Slider midiVolume;  // [1]

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YmmyEditor)
};
