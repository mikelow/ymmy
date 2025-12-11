#include "YM2151Driver.h"

#include "Cps1YM2151Driver.h"
#include "YM2151Synth.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace {
std::string toLower(const std::string& value) {
  std::string lowered(value.size(), '\0');
  std::transform(value.begin(), value.end(), lowered.begin(), [](unsigned char c) { return std::tolower(c); });
  return lowered;
}
}  // namespace

bool parseDriverMetadataLine(const std::string& line, OPMDriverData& driverData) {
  if (line.rfind("DRIVER:", 0) == 0) {
    std::istringstream iss(line.substr(7));
    if (!(iss >> driverData.name)) {
      std::cerr << "Error parsing DRIVER name: " << line << std::endl;
      driverData = {};
      return true;
    }

    driverData.dataBytes.clear();
    uint8_t byte{};
    while (iss >> std::ws && iss.good()) {
      int temp = -1;
      if (iss >> temp && temp >= 0 && temp <= 255) {
        driverData.dataBytes.push_back(static_cast<uint8_t>(temp));
      } else {
        break;
      }
    }
    return true;
  }

  auto colonPos = line.find(':');
  if (colonPos != std::string::npos) {
    const auto header = line.substr(0, colonPos + 1);
    if (header.size() > 1 &&
        std::all_of(header.begin(), header.end(), [](unsigned char c) { return std::isupper(c) || c == ':'; })) {
      driverData.name = toLower(header.substr(0, header.size() - 1));
      driverData.dataBytes.clear();

      std::istringstream iss(line.substr(colonPos + 1));
      int temp = -1;
      while (iss >> temp) {
        if (temp >= 0 && temp <= 255) {
          driverData.dataBytes.push_back(static_cast<uint8_t>(temp));
        }
      }
      return true;
    }
  }

  return false;
}

void DefaultYM2151Driver::assignPatchToChannel(const OPMPatch& patch,
                                               int channel,
                                               YM2151DriverHost& host,
                                               YM2151MidiChannelState& channelState) {
  channelState.driverData = patch.driver.dataBytes;
}

void DefaultYM2151Driver::updateChannelVolume(int channel,
                                              YM2151DriverHost& host,
                                              YM2151MidiChannelState& channelState) {
  host.defaultChannelVolumeUpdate(channel);
}

std::unique_ptr<YM2151Driver> createDriver(const std::string& name) {
  auto lowered = toLower(name);
  if (lowered == "cps") {
    return std::make_unique<Cps1YM2151Driver>();
  }
  return std::make_unique<DefaultYM2151Driver>();
}

