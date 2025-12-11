#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct OPMOpParams {
  uint8_t AR, D1R, D2R, RR, D1L, TL, KS, MUL, DT1, DT2, AMS_EN;
};

struct OPMChannelParams {
  uint8_t PAN, FL, CON, AMS, PMS, SLOT_MASK, NE;
};

struct OPMLFOParams {
  uint8_t LFRQ, AMD, PMD, WF, NFRQ;
};

struct OPMDriverData {
  std::string name;
  std::vector<uint8_t> dataBytes;
};

struct OPMPatch {
  std::string name;
  uint16_t number;
  OPMLFOParams lfoParams;
  OPMChannelParams channelParams;
  OPMOpParams opParams[4];
  OPMDriverData driver;
};
