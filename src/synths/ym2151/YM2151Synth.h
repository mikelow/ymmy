#pragma once

#include "../Synth.h"
#include "ymfm_opm.h"
#include "YM2151Interface.h"
#include "OPMFileLoader.h"

class YM2151Synth: public Synth,
                    public ValueTree::Listener,
                    public AudioProcessorValueTreeState::Listener,
                    public ymfm::ymfm_interface {
public:
  YM2151Synth(AudioProcessorValueTreeState& vts);
  ~YM2151Synth();
  void initialize();
  void reset();

  static std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup();
  static const ValueTree getInitialChildValueTree();

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

private:
  void processMidiMessage(MidiMessage& m);

private:
  static const StringArray programChangeParams;

  int selectedGroup;
  int selectedChannel;
  int channelGroup;

  OPMFileLoader opmLoader;
  // YMFM
  YM2151Interface interface;

  ymfm::ym2151::output_data opm_out;
};
