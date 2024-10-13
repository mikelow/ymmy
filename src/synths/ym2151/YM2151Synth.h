#pragma once

#include "../Synth.h"
#include "ymfm_opm.h"
#include "YM2151Interface.h"
#include "OPMFileLoader.h"

class YmmyProcessor;

struct YM2151MidiChannelState {
  uint8_t volume;
  uint8_t CON;
  uint8_t SLOT_MASK;
  uint8_t TL[4];
  int8_t KF;
  OPMCPSParams cpsParams[4];
};

class YM2151Synth: public Synth,
                    public ValueTree::Listener,
                    public AudioProcessorValueTreeState::Listener,
                    public ymfm::ymfm_interface {
public:
  YM2151Synth(YmmyProcessor* processor, AudioProcessorValueTreeState& vts);
  ~YM2151Synth();
  void initialize();
  void reset();

  static std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup();
  static const ValueTree getInitialChildValueTree();

  SynthType getSynthType() override { return YM2151; }
  void receiveFile(juce::MemoryBlock&, SynthFileType fileType) override;
  void changePreset(OPMPatch& patch, int channel);
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  int getNumPrograms() override;
  int getCurrentProgram() override;
  const juce::String getProgramName (int index) override;
  void setCurrentProgram (int index) override;
  void setControllerValue(int controller, int value);

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

  void refreshBanks(std::vector<OPMPatch>& patches);

private:
  void handleSysex(MidiMessage& message);
  void processMidiMessage(MidiMessage& m);
  OPMPatch loadPresetFromVST(int bankNum, int presetNum);

private:
  YmmyProcessor* processor;
  LagrangeInterpolator resamplers[2];
  std::unique_ptr<AudioBuffer<float>> nativeBuffer;

  YM2151MidiChannelState midiChannelState[8] = {};

  static const StringArray programChangeParams;

  int selectedGroup;
  int selectedChannel;
  int channelGroup;

  OPMFileLoader opmLoader;
  // YMFM
  YM2151Interface interface;

  ymfm::ym2151::output_data opm_out;
};
