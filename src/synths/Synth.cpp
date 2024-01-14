#include "Synth.h"

Synth::Synth(AudioProcessorValueTreeState& valueTreeState)
    : vts(valueTreeState) {

}

int Synth::CheckForChannelGroupSysExEvent(MidiMessage& message) {
  if (message.isSysEx()) {
    auto sysexData = message.getSysExData();
    auto sysexDataSize = message.getSysExDataSize();
    if (sysexData[0] == 0x7D && sysexData[1] < 0x7E) {
      return static_cast<int>(sysexData[1]);
      //          auto wrappedMsg = juce::MidiMessage(sysexData + 2, m.getSysExDataSize() - 2, 0);
      //          handleMidiMessage(wrappedMsg);
    }
  }
  return -1;
}