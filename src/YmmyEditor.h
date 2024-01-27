#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class YmmyProcessor;
class SynthComponent;

enum SynthType { FluidSynth, YM2151 };

class YmmyEditor :
    public AudioProcessorEditor,
    public ValueTree::Listener,
    private Slider::Listener,
    private Button::Listener {
public:
  // Constants
  static constexpr int headerHeight = 50;
  static constexpr int keyboardHeight = 60;

  YmmyEditor(YmmyProcessor&, juce::AudioProcessorValueTreeState&);
  ~YmmyEditor();

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged,
                                        const Identifier& property) override;
  void setCurrentSynth(SynthType synthType);
  void sliderValueChanged(juce::Slider* slider) override;
  void buttonClicked(Button* button) override;

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  YmmyProcessor& audioProcessor;
  AudioProcessorValueTreeState& valueTreeState;

  std::unique_ptr<SynthComponent> currentSynth;

  LookAndFeel* laf;

  Slider midiVolume;  // [1]
  MidiKeyboardComponent midiKeyboard;

  Label groupLabel;
  Label channelLabel;
  Slider groupSlider    { Slider::IncDecButtons, Slider::TextBoxAbove };
  Slider channelSlider    { Slider::IncDecButtons, Slider::TextBoxAbove };
//  Label channelLabel;
//  TextButton incChannelButton;
//  TextButton decChannelButton;


  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YmmyEditor)
};
