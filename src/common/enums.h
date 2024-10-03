#pragma once

#include <unordered_map>
#include <string>

enum SynthFileType {
  SoundFont2 = 0,
  YM2151_OPM = 1
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