#include "YM2151Synth.h"
#include "../../MidiConstants.h"

YM2151Synth::YM2151Synth(AudioProcessorValueTreeState& valueTreeState)
    : Synth(valueTreeState),
      channelGroup(0) {
//  initialize();

//  vts.state.addListener(this);

//  vts.addParameterListener("selectedGroup", this);
//  vts.addParameterListener("selectedChannel", this);
//  vts.addParameterListener("fs.bank", this);
//  vts.addParameterListener("fs.preset", this);
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.addParameterListener(param, this);
//  }
}

YM2151Synth::~YM2151Synth() {
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.removeParameterListener(param, this);
//  }
//  vts.removeParameterListener("fs.bank", this);
//  vts.removeParameterListener("fs.preset", this);
//  vts.removeParameterListener("selectedGroup", this);
//  vts.removeParameterListener("selectedChannel", this);
//  vts.state.removeListener(this);
}

std::unique_ptr<juce::AudioProcessorParameterGroup> YM2151Synth::createParameterGroup() {
  auto ym2151Params = std::make_unique<juce::AudioProcessorParameterGroup>("ym2151", "YM2151", "|");

//  for (IntParameter p : FluidSynthParam::parameters) {
//    fluidSynthParams->addChild(std::make_unique<juce::AudioParameterInt>(p.id, p.name, p.min, p.max, p.defaultVal));
//  }
  return ym2151Params;
}

const ValueTree YM2151Synth::getInitialChildValueTree() {
  return
      { "ignoredRoot", {},
       {
       }
      };
}

void YM2151Synth::processMidiMessage(MidiMessage& m) {
  auto rawData = m.getRawData();
  if (rawData[0] >= 0xF0) {
    return;
  }
  const uint8_t eventNibble = rawData[0] & 0xf0;
  int channelGroupOffset = channelGroup * 16;

  switch (eventNibble) {
    case NOTE_OFF:
      break;
    case NOTE_ON:
      interface.write(8, 0x79);
      break;
    case KEY_PRESSURE:
      break;
    case CONTROL_CHANGE:
      break;
    case PROGRAM_CHANGE:
      break;
    case CHANNEL_PRESSURE:
      break;
    case PITCH_BEND:
      break;
    case MIDI_SYSEX:
      break;
  }
}

void YM2151Synth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  MidiBuffer processedMidi;
  int time;
  MidiMessage m;

  for (MidiBuffer::Iterator i{midiMessages}; i.getNextEvent(m, time);) {
    processMidiMessage(m);
  }

  buffer.clear();
  auto bufferWritePointers = buffer.getArrayOfWritePointers();
  if (buffer.getNumChannels() < 2) {
    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,"Error",
                                     "Expected at least 2 write buffers for stereo playback. Is there a problem with the current audio device?.");
    return;
  }
  float* writeBuffers[2] = {bufferWritePointers[0], bufferWritePointers[1] };

  interface.generate(writeBuffers, buffer.getNumSamples());
//  fluid_synth_process(synth.get(), buffer.getNumSamples(), 2, writeBuffers, 2, writeBuffers);
}

void YM2151Synth::prepareToPlay(double sampleRate, int samplesPerBlock) {
}

int YM2151Synth::getNumPrograms() {
  return 128;
}

int YM2151Synth::getCurrentProgram() {
  return 0;
}

void YM2151Synth::setCurrentProgram (int index) {
}

const juce::String YM2151Synth::getProgramName (int index) {
  return "Preset";
}

void YM2151Synth::parameterChanged(const String& parameterID, float newValue) {
  DBG("YM2151Synth::parameterChanged() " + parameterID);
}

void YM2151Synth::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
                                           const Identifier& property) {
}

void YM2151Synth::setControllerValue(int controller, int value) {
}

void YM2151Synth::valueTreeRedirected (ValueTree& treeWhichHasBeenChanged) {

}

void YM2151Synth::reset() {
}