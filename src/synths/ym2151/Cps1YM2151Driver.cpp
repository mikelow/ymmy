#include "Cps1YM2151Driver.h"
#include "YM2151Synth.h"

#include <algorithm>
#include <array>

namespace {
constexpr size_t kNumOperators = 4;
constexpr size_t kCps1PayloadSize = 2 + (kNumOperators * 2);

uint8_t ascendingKeyScaleTable[] = {
  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x5,
  0xA,  0xF, 0x14, 0x19,
  0x1E, 0x22, 0x27, 0x2C,
  0x31, 0x36, 0x3B, 0x40,
  0x45, 0x4A, 0x4F, 0x54,
  0x59, 0x5E, 0x62, 0x67,
  0x6C, 0x71, 0x76, 0x7B,
  0x80, 0x85, 0x8A, 0x8F,
  0x94, 0x99, 0x9E, 0xA2,
  0xA7, 0xAC, 0xB1, 0xB6,
  0xBB, 0xC0, 0xC5, 0xCA,
  0xCF, 0xD4, 0xD9, 0xDE,
  0xE2, 0xE7, 0xEC, 0xF1,
  0xF6, 0xFB, 0xFE, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t descendingKeyScaleTable[] = {
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFE, 0xFD, 0xFC, 0xFA,
  0xF8, 0xF6, 0xF3, 0xF0,
  0xEC, 0xE8, 0xE4, 0xDF,
  0xDA, 0xD4, 0xCF, 0xC8,
  0xC2, 0xBB, 0xB3, 0xAB,
  0xA3, 0x9B, 0x92, 0x89,
  0x7F, 0x75, 0x6C, 0x63,
  0x5B, 0x53, 0x4B, 0x43,
  0x3C, 0x36, 0x2F, 0x2A,
  0x24, 0x1F, 0x1A, 0x16,
  0x12,  0xE,  0xB,  0x8,
  0x6,  0x4,  0x2,  0x1,
  0x1,  0x1,  0x1,  0x0,
  0x0,  0x0,  0x0,  0x0
};

uint8_t calculateKeyScaleAttenuation(char key_scale_sensitivity, uint8_t note) {
  uint8_t index = (note >> 1) & 0x3F;
  uint8_t key_scale_factor = (key_scale_sensitivity >= 0) ?
                                ascendingKeyScaleTable[index] :
                                descendingKeyScaleTable[index];
  uint8_t ksa = key_scale_sensitivity & 0xF; // Use lower 4 bits (magnitude)
  uint16_t product = key_scale_factor * ksa;
  return (uint8_t)(product >> 7); // Equivalent to dividing by 128
}
}

bool Cps1YM2151Driver::parseCpsParams(const std::vector<uint8_t>& data, OPMCPSParams& outParams) {
  if (data.size() < kCps1PayloadSize) {
    return false;
  }

  outParams.enable_lfo = data[0];
  outParams.reset_lfo = data[1];

  for (size_t i = 0; i < kNumOperators; ++i) {
    auto baseIndex = 2 + (i * 2);
    outParams.vol_data[i].key_scale_sensitivity = data[baseIndex];
    outParams.vol_data[i].extra_atten = data[baseIndex + 1];
  }
  return true;
}

void Cps1YM2151Driver::assignPatchToChannel(const OPMPatch& patch,
                                            int channel,
                                            YM2151DriverHost& host,
                                            ChannelState& channelState) {
  std::optional<OPMCPSParams> params;
  if (!patch.driver.dataBytes.empty()) {
    OPMCPSParams parsedParams{};
    if (parseCpsParams(patch.driver.dataBytes, parsedParams)) {
      params = parsedParams;
    }
  }
  channelParams[channel] = params;
}

bool Cps1YM2151Driver::updateChannelTL(int channel,
                                       YM2151DriverHost& host,
                                       ChannelState& chState) {
  auto& params = channelParams[channel];
  if (!params.has_value() || !chState.midi.isNoteActive) {
    return false;
  }

  uint8_t note = chState.midi.note;
  uint8_t velocity = chState.midi.velocity;
  uint8_t channelVolume = chState.midi.volume;
  uint8_t channelVolumeAtten = 0x7F - channelVolume;
  channelVolumeAtten += 0x7F - velocity;

  uint8_t attenByte;
  uint8_t volKeyScaleAtten;
  uint8_t CON_limits[4] = { 7, 5, 4, 0 };
  std::array<uint8_t, 4> opAtten{};
  for (int i = 0; i < 4; i++) {
    uint8_t keyScale = params->vol_data[i].key_scale_sensitivity;
    volKeyScaleAtten = calculateKeyScaleAttenuation(keyScale, note);
    auto conLimit = CON_limits[i];
    uint32_t finalAttenuation = volKeyScaleAtten;
    if (chState.ym.CON < conLimit) {
      finalAttenuation += chState.ym.TL[i];
    } else {
      finalAttenuation += chState.ym.TL[i] + channelVolumeAtten;
    }
    attenByte = static_cast<uint8_t>(std::min(finalAttenuation, 0x7FU));
    opAtten[i] = attenByte;
  }
  host.setChannelTL(channel, opAtten);
  return true;
}

bool Cps1YM2151Driver::shouldResetLFOOnNoteOn(int channel) const {
  return channelParams[channel].has_value() && channelParams[channel].value().reset_lfo;
}

bool Cps1YM2151Driver::enableLFO(int channel) const {
  return channelParams[channel].has_value() && channelParams[channel].value().enable_lfo;
}
