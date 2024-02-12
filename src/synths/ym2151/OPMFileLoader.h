#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
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

struct OPMPatch {
  std::string name;
  uint8_t number;
  OPMLFOParams lfoParams;
  OPMChannelParams channelParams;
  OPMOpParams opParams[4];
};

class OPMFileLoader {
public:


public:
  OPMFileLoader();
  ~OPMFileLoader();

  static std::vector<OPMPatch> parseOpmFile(const std::string& fileName);

private:
  static void parseOperatorParams(const std::string& line, OPMOpParams& op);
  static void parseChannelParams(const std::string& line, OPMChannelParams& ch);
  static void parseLFOParams(const std::string& line, OPMLFOParams& lfo);
};