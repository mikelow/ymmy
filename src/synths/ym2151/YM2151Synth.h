#pragma once

#include <array>

#include "../Synth.h"
#include "ymfm_opm.h"
#include "YM2151Interface.h"
#include "OPMFileLoader.h"
#include "YM2151Driver.h"

class YmmyProcessor;

struct YM2151MidiChannelState {
  uint8_t volume = 0x7F;
  uint8_t pan = 64;
  uint8_t RL = 0xC0;
  uint8_t FL = 0;
  uint8_t CON = 0;
  uint8_t SLOT_MASK = 120;
  uint8_t TL[4];
  int8_t KF;
  uint8_t note = 0;
  uint8_t velocity = 0;
  uint8_t isNoteActive = false;
  std::vector<uint8_t> driverData;

public:
  void reset() {
    note = 0;
    volume = 0x7F;
    velocity = 0x7F;
    pan = 64;
    isNoteActive = false;
    RL = 0xC0;
    FL = 0;
    CON = 0;
    SLOT_MASK = 0x78;
    KF = 0;
    memset(TL, 0, sizeof(TL));
    driverData.clear();
  }
};


enum class RLSetting : uint8_t {
  R = 0x80,
  L = 0x40,
  RL = 0xC0,
};

class YM2151Synth: public Synth,
                    public ValueTree::Listener,
                    public AudioProcessorValueTreeState::Listener,
                    public ymfm::ymfm_interface,
                    public YM2151DriverHost {
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
  void setChannelVolume(int channel, const std::array<uint8_t, 4>& atten);
  void setChannelRL(int channel, RLSetting lr);
  void defaultChannelVolumeUpdate(int channel) override;
  void defaultChannelPanUpdate(int channel);
  // void applyDefaultChannelVolume(int channel) override;
  void applyOperatorAttenuation(int channel, const std::array<uint8_t, 4>& attenuation) override;
  void updateChannelVolume(int channel);
  void ensureCurrentDriver();
  void switchDriverIfNeeded(const std::string& desiredDriverName, int targetChannel);
  std::string normalizeDriverName(const std::string& name) const;
  void resetChannelsExcept(int preservedChannel);


private:
  YmmyProcessor* processor;
  LagrangeInterpolator resamplers[2];
  std::unique_ptr<AudioBuffer<float>> nativeBuffer;
  juce::AudioBuffer<float> tempBuffer;

  YM2151MidiChannelState midiChannelState[8] = {};
  std::unique_ptr<YM2151Driver> currentDriver;
  std::string activeDriverKey;

  static const StringArray programChangeParams;

  int selectedGroup;
  int selectedChannel;
  int channelGroup;

  OPMFileLoader opmLoader;
  // YMFM
  YM2151Interface interface;

  ymfm::ym2151::output_data opm_out;
};
