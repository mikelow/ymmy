#pragma once

class AudioProcessorValueTreeState;

class FluidSynthSynth {
public:
  FluidSynthSynth(AudioProcessorValueTreeState& valueTreeState);

private:
  AudioProcessorValueTreeState& valueTreeState;
};
