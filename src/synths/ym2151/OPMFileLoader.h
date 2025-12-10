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

struct OPMCPS425VolData {
  // increases or decreases attenuation based on the pitch of the note to be played. Used in conjunction with
  // ascendingKeyScaleTable and descendingKeyScaleTable. Bit 7 determines which table to use.
  uint8_t key_scale_sensitivity;
  // additional attenuation that is applied except when value is set by event 0x19. We bake it into the TL
  // value already, as event 0x19 is rarely used and we haven't added support for this condition. May remove it.
  uint8_t extra_atten;
};

// OPMCPSParams defines the "CPS:" extension that we're adding and supporting in OPM files.
struct OPMCPSParams {
  bool enable_lfo;     // boolean indicating whether to enable the LFO when the instr is loaded
  bool reset_lfo;      // boolean indicating whether to reset the LFO phase on note on
  OPMCPS425VolData vol_data[4];
};

struct OPMPatch {
  std::string name;
  uint16_t number;
  OPMLFOParams lfoParams;
  OPMChannelParams channelParams;
  OPMOpParams opParams[4];
  OPMCPSParams cpsParams;
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
  static void parseCPSParams(const std::string& line, OPMCPSParams& cps);
};