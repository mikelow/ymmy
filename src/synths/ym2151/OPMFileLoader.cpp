#include "OPMFileLoader.h"

OPMFileLoader::OPMFileLoader() {}
OPMFileLoader::~OPMFileLoader() {}

bool tryParseUInt8(std::istringstream& iss, uint8_t& value) {
  int temp;
  if (iss >> temp && temp >= 0 && temp <= 255) {
    value = static_cast<uint8_t>(temp);
    return true;
  }
  return false;
}

void trimNewline(std::string& str) {
  if (!str.empty() && str.back() == '\r') {
    str.pop_back();
  }
}

void OPMFileLoader::parseOperatorParams(const std::string& line, OPMOpParams& op) {
  std::istringstream iss(line.substr(3)); // Skip the "LFO:" part
  if (tryParseUInt8(iss, op.AR) &&
      tryParseUInt8(iss, op.D1R) &&
      tryParseUInt8(iss, op.D2R) &&
      tryParseUInt8(iss, op.RR) &&
      tryParseUInt8(iss, op.D1L) &&
      tryParseUInt8(iss, op.TL) &&
      tryParseUInt8(iss, op.KS) &&
      tryParseUInt8(iss, op.MUL) &&
      tryParseUInt8(iss, op.DT1) &&
      tryParseUInt8(iss, op.DT2) &&
      tryParseUInt8(iss, op.AMS_EN)) {
    // Successfully parsed all LFO parameters
  } else {
    std::cerr << "Error parsing LFO parameters: " << line << std::endl;
    // Handle the error appropriately
  }
}

void OPMFileLoader::parseChannelParams(const std::string& line, OPMChannelParams& ch) {
//  std::istringstream iss(line);
//  iss >> ch.PAN >> ch.FL >> ch.CON >> ch.AMS >> ch.PMS >> ch.SLOT_MASK >> ch.NE;

  std::istringstream iss(line.substr(3)); // Skip the "LFO:" part
  if (tryParseUInt8(iss, ch.PAN) &&
      tryParseUInt8(iss, ch.FL) &&
      tryParseUInt8(iss, ch.CON) &&
      tryParseUInt8(iss, ch.AMS) &&
      tryParseUInt8(iss, ch.PMS) &&
      tryParseUInt8(iss, ch.SLOT_MASK) &&
      tryParseUInt8(iss, ch.NE)) {
    // Successfully parsed all LFO parameters
  } else {
    std::cerr << "Error parsing LFO parameters: " << line << std::endl;
  }
}

void OPMFileLoader::parseLFOParams(const std::string& line, OPMLFOParams& lfo) {
  std::istringstream iss(line.substr(4)); // Skip the "LFO:" part
  if (tryParseUInt8(iss, lfo.LFRQ) &&
      tryParseUInt8(iss, lfo.AMD) &&
      tryParseUInt8(iss, lfo.PMD) &&
      tryParseUInt8(iss, lfo.WF) &&
      tryParseUInt8(iss, lfo.NFRQ)) {
    // Successfully parsed all LFO parameters
  } else {
    std::cerr << "Error parsing LFO parameters: " << line << std::endl;
  }
}

void OPMFileLoader::parseCPSParams(const std::string& line, OPMCPSParams* cps) {
  std::istringstream iss(line.substr(4)); // Skip the "LFO:" part
  for (int i = 0; i < 4; i++) {
    if (tryParseUInt8(iss, cps[i].vol) &&
        tryParseUInt8(iss, cps[i].vol_key_scale) &&
        tryParseUInt8(iss, cps[i].extra_atten)) {
      // Successfully parsed all LFO parameters
    } else {
      std::cerr << "Error parsing LFO parameters: " << line << std::endl;
    }
  }
}


std::vector<OPMPatch> OPMFileLoader::parseOpmFile(const std::string& fileName) {
  std::ifstream file(fileName);
  if (!file.is_open()) {
    std::cerr << "Failed to open file: " << fileName << std::endl;
    return {};  // Return empty vector on failure
  }

  return parseOpmStream(file);
}

std::vector<OPMPatch> OPMFileLoader::parseOpmString(const std::string& content) {
  std::istringstream stream(content);
  return parseOpmStream(stream);
}

std::vector<OPMPatch> OPMFileLoader::parseOpmStream(std::istream& inputStream) {
  std::string line;
  std::vector<OPMPatch> patches;
  OPMPatch currentPatch;

  while (getline(inputStream, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '/')
      continue;

    auto substr = line.substr(0, 3);

    if (substr[0] == '@' && substr[1] == ':') {
      if (!currentPatch.name.empty()) {
        patches.push_back(currentPatch);
        currentPatch = OPMPatch();
      }
      std::istringstream iss(line.substr(2));  // Remove '@' and parse
      if (!tryParseUInt8(iss, currentPatch.number)) {
        return {};
      }
      // Skip any additional whitespace after the number
      iss >> std::ws;

      // Get the remainder of the line as the patch name
      std::getline(iss, currentPatch.name);
      trimNewline(currentPatch.name); // Trim the newline character if present
    } else if (substr == "LFO") {
      parseLFOParams(line, currentPatch.lfoParams);
    } else if (substr == "CH:") {
      parseChannelParams(line, currentPatch.channelParams);
    } else if (substr == "M1:") {
      parseOperatorParams(line, currentPatch.opParams[0]);
    } else if (substr == "C1:") {
      parseOperatorParams(line, currentPatch.opParams[2]);
    } else if (substr == "M2:") {
      parseOperatorParams(line, currentPatch.opParams[1]);
    } else if (substr == "C2:") {
      parseOperatorParams(line, currentPatch.opParams[3]);
    } else if (substr == "CPS") {
      parseCPSParams(line, currentPatch.cpsParams);
    }
  }

  if (!currentPatch.name.empty()) {
    patches.push_back(currentPatch);
  }

  return patches;
}
