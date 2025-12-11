#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "OPMTypes.h"

class YM2151DriverHost {
public:
  virtual ~YM2151DriverHost() = default;

  virtual void defaultChannelVolumeUpdate(int channel) = 0;
  virtual void applyOperatorAttenuation(int channel, const std::array<uint8_t, 4>& attenuation) = 0;
};

struct YM2151MidiChannelState;

class YM2151Driver {
public:
  virtual ~YM2151Driver() = default;

  virtual std::string getName() const = 0;
  virtual void assignPatchToChannel(const OPMPatch& patch,
                                    int channel,
                                    YM2151DriverHost& host,
                                    YM2151MidiChannelState& channelState) = 0;
  virtual void updateChannelVolume(int channel,
                                   YM2151DriverHost& host,
                                   YM2151MidiChannelState& channelState) = 0;
  virtual bool shouldResetLFOOnNoteOn(int channel) const { return false; }
  virtual bool enableLFO(int channel) const { return false; }
};

class DefaultYM2151Driver : public YM2151Driver {
public:
  std::string getName() const override { return "default"; }
  void assignPatchToChannel(const OPMPatch& patch,
                            int channel,
                            YM2151DriverHost& host,
                            YM2151MidiChannelState& channelState) override;
  void updateChannelVolume(int channel,
                           YM2151DriverHost& host,
                           YM2151MidiChannelState& channelState) override;
};

std::unique_ptr<YM2151Driver> createDriver(const std::string& name);
bool parseDriverMetadataLine(const std::string& line, OPMDriverData& driverData);
