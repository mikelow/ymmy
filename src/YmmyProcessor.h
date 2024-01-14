#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class Synth;

class YmmyProcessor : public AudioProcessor {
public:
  static constexpr int maxChannels = 48;

  YmmyProcessor();
  ~YmmyProcessor();

  void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override;

public:
  float noteOnVel;
  //==============================================================================
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const juce::String getProgramName (int index) override;
  void changeProgramName (int index, const juce::String& newName) override;

  //==============================================================================
  void getStateInformation (juce::MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;

  void addSynth(std::unique_ptr<Synth> synth);
  void removeSynth(Synth* synth);

  MidiKeyboardState keyboardState;

private:
  int currentChannel;
  std::vector<std::unique_ptr<Synth>> synths;
  std::unordered_map<int, Synth*> channelToSynthMap;
//    std::unique_ptr<juce::AudioProcessorValueTreeState> state;
  AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

//  Synthesiser synth;
  AudioProcessorValueTreeState vts;
  int channelGroup;

    //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YmmyProcessor)
};

