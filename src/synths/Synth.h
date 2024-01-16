#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

struct IntParameter {
  juce::String id;
  juce::String name;
  int min;
  int max;
  int defaultVal;
};

class Synth {
public:
  Synth(AudioProcessorValueTreeState& valueTreeState);

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
