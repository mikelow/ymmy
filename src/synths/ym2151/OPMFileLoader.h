#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "OPMTypes.h"

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
};