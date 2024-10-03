#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

struct IntParameter {
  juce::String id;
  juce::String name;
  int min;
  int max;
  int defaultVal;
};

enum SynthType { FluidSynth, YM2151 };

static std::unordered_map<std::string, SynthType> stringToSynthType = {
  {"soundfont", FluidSynth},
  {"ym2151", YM2151}
};

static std::unordered_map<SynthType, std::string> synthTypeToString = {
  {FluidSynth, "soundfont"},
  {YM2151, "ym2151"}
};

class Synth {
public:
  Synth(AudioProcessorValueTreeState& valueTreeState);
  virtual ~Synth() = default;

  virtual SynthType getSynthType() = 0;
//  virtual std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup() = 0;
  virtual void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;
  virtual void prepareToPlay (double sampleRate, int samplesPerBlock) = 0;
  virtual int getNumPrograms() = 0;
  virtual int getCurrentProgram() = 0;
  virtual void setCurrentProgram (int index) = 0;
  virtual const juce::String getProgramName (int index) = 0;

protected:
  static int CheckForChannelGroupSysExEvent(MidiMessage& message);

  AudioProcessorValueTreeState& vts;
};
