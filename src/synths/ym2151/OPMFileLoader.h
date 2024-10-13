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

// OPMCPSParams defines the "CPS:" extension that we're supporting in OPM files. The parameters
// are used in later version of the CPS1 sound driver and enable more control over operator volume.
struct OPMCPSParams {
  uint8_t vol;            // a volume value that is converted to attenuation via 0x7F - vol
  uint8_t vol_key_scale;  // an index to a table that will
  uint8_t extra_atten;
};

struct OPMPatch {
  std::string name;
  uint8_t number;
  OPMLFOParams lfoParams;
  OPMChannelParams channelParams;
  OPMOpParams opParams[4];
  OPMCPSParams cpsParams[4];
};

class OPMFileLoader {
public:


public:
  OPMFileLoader();
  ~OPMFileLoader();

  static std::vector<OPMPatch> parseOpmFile(const std::string& fileName);
  static std::vector<OPMPatch> parseOpmString(const std::string& content);
  static std::vector<OPMPatch> parseOpmStream(std::istream& inputStream);
private:
  static void parseOperatorParams(const std::string& line, OPMOpParams& op);
  static void parseChannelParams(const std::string& line, OPMChannelParams& ch);
  static void parseLFOParams(const std::string& line, OPMLFOParams& lfo);
  static void parseCPSParams(const std::string& line, OPMCPSParams* cps);
};