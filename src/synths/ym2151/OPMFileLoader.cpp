#include "OPMFileLoader.h"

OPMFileLoader::OPMFileLoader() {}
OPMFileLoader::~OPMFileLoader() {}

void OPMFileLoader::parseOperatorParams(const std::string& line, OPMOpParams& op) {
  std::istringstream iss(line);
  iss >> op.AR >> op.D1R >> op.D2R >> op.RR >> op.D1L >> op.TL >> op.KS >> op.MUL >> op.DT1 >>
      op.DT2 >> op.AMS_EN;
}

void OPMFileLoader::parseChannelParams(const std::string& line, OPMChannelParams& ch) {
  std::istringstream iss(line);
  iss >> ch.PAN >> ch.FL >> ch.CON >> ch.AMS >> ch.PMS >> ch.SLOT >> ch.NE;
}

void OPMFileLoader::parseLFOParams(const std::string& line, OPMLFOParams& lfo) {
  std::istringstream iss(line);
  iss >> lfo.LFRQ >> lfo.AMD >> lfo.PMD >> lfo.WF >> lfo.WF >> lfo.NFRQ;
}

std::vector<OPMPatch> OPMFileLoader::readOpmFile(const std::string& fileName) {
  std::ifstream file(fileName);
  std::string line;
  std::vector<OPMPatch> patches;
  OPMPatch currentPatch;

  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << fileName << std::endl;
    return patches;  // Return empty vector on failure
  }

  while (getline(file, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '/')
      continue;

    auto substr = line.substr(0, 4);

    if (substr[0] == '@' && substr[1] == ':') {
      if (!currentPatch.name.empty()) {
        patches.push_back(currentPatch);
        currentPatch = OPMPatch();
      }
      std::istringstream iss(line.substr(2));  // Remove '@' and parse
      iss >> currentPatch.number >> currentPatch.name;
    } else if (substr == "LFO") {
      parseLFOParams(line.substr(4), currentPatch.lfoParams);
    } else if (substr == "CH:") {
      parseChannelParams(line.substr(3), currentPatch.channelParams);
    } else if (substr == "M1:") {
      parseOperatorParams(line.substr(3), currentPatch.opParams[0]);
    } else if (substr == "C1:") {
      parseOperatorParams(line.substr(3), currentPatch.opParams[1]);
    } else if (substr == "M2:") {
      parseOperatorParams(line.substr(3), currentPatch.opParams[2]);
    } else if (substr == "C2:") {
      parseOperatorParams(line.substr(3), currentPatch.opParams[3]);
    }
  }

  if (!currentPatch.name.empty()) {
    patches.push_back(currentPatch);
  }

  return patches;
}
