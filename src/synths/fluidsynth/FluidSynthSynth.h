#pragma once

#include "../Synth.h"
#include "fluidsynth.h"

namespace FluidSynthParam {
  extern const juce::String FILTER_RESONANCE;
  extern const juce::String FILTER_CUT_OFF;
  extern const juce::String ATTACK_RATE;
  extern const juce::String HOLD;
  extern const juce::String DECAY_RATE;
  extern const juce::String SUSTAIN_LEVEL;
  extern const juce::String RELEASE_RATE;

  extern IntParameter parameters[];
  extern std::map<juce::String, IntParameter> paramIdToParam;
}

class FluidSynthSynth: public Synth,
                       public ValueTree::Listener,
                       public AudioProcessorValueTreeState::Listener {
public:
  FluidSynthSynth(AudioProcessorValueTreeState& vts);
  ~FluidSynthSynth();
  void initialize();
  void reset();

//  std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup() override;
  static std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup();
//  static void updateValueTreeState(AudioProcessorValueTreeState& vts);
  static const ValueTree getInitialChildValueTree();

  SynthType getSynthType() override { return FluidSynth; }
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  int getNumPrograms() override;
  int getCurrentProgram() override;
  const juce::String getProgramName (int index) override;
  void setCurrentProgram (int index) override;
  void setControllerValue(int controller, int value);

  void updateParamsFromSynth();

  //==============================================================================
  virtual void parameterChanged (const String& parameterID, float newValue) override;

  virtual void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged,
                                        const Identifier& property) override;
  inline virtual void valueTreeChildAdded (ValueTree& parentTree,
                                          ValueTree& childWhichHasBeenAdded) override {};
  inline virtual void valueTreeChildRemoved (ValueTree& parentTree,
                                            ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved) override {};
  inline virtual void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved,
                                                 int oldIndex, int newIndex) override {};
  inline virtual void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) override {};
  virtual void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) override;

  void refreshBanks();
  void unloadAndLoadFont(const String &absPath);
  void unloadAndLoadFontFromMemory(void *sf, size_t fileSize);
  void loadFont(const String &absPath);
  void loadFontFromMemory(void *sf, fluid_long_long_t fileSize);

private:
  uint32_t read6BitVariableLengthQuantity(const uint8_t* buffer, int maxLength, int& bytesRead);
  void read7BitChunk(const uint8_t* encodedData, uint8_t* decodedData);
  void handleSysex(MidiMessage& message);
  void processMidiMessage(MidiMessage& m);

private:
  static const StringArray programChangeParams;
  int32_t incomingSF2Size;
  juce::MemoryBlock incomingSF2File;

  int sfont_id;
  int selectedGroup;
  int selectedChannel;
  int channelGroup;
  double currentSampleRate;
  std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
  std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth;
};
