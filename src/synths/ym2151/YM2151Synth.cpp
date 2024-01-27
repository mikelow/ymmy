#include "YM2151Synth.h"

YM2151Synth::YM2151Synth(AudioProcessorValueTreeState& valueTreeState)
    : Synth(valueTreeState),
      channelGroup(0) {
  initialize();

//  vts.state.addListener(this);

//  vts.addParameterListener("selectedGroup", this);
//  vts.addParameterListener("selectedChannel", this);
//  vts.addParameterListener("fs.bank", this);
//  vts.addParameterListener("fs.preset", this);
//  for (const auto &[param, genId]: paramToGenerator) {
//    vts.addParameterListener(param, this);
//  }
}