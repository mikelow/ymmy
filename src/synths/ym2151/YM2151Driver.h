#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "OPMTypes.h"

struct ChannelState;
enum class RLSetting : uint8_t;

class YM2151DriverHost {
public:
  virtual ~YM2151DriverHost() = default;

  virtual void defaultCCHandler(int channel, uint8_t controllerNum) = 0;
  virtual void defaultChannelTLUpdate(int channel) = 0;
  virtual void defaultChannelPanUpdate(int channel) = 0;

  virtual void setChannelTL(int channel, const std::array<uint8_t, 4>& atten) = 0;
  virtual void setChannelRL(int channel, RLSetting lr) = 0;
};

class YM2151Driver {
public:
  virtual ~YM2151Driver() = default;

  virtual std::string getName() const = 0;
  virtual void assignPatchToChannel(const OPMPatch& patch,
                                    int channel,
                                    YM2151DriverHost& host,
                                    ChannelState& channelState) {}
  virtual bool handleCC(int channel,
                        int controllerNum,
                        YM2151DriverHost& host,
                        ChannelState& channelState) { return false; }
  virtual bool updateChannelTL(int channel,
                               YM2151DriverHost& host,
                               ChannelState& channelState) { return false; }
  virtual bool shouldResetLFOOnNoteOn(int channel) const { return false; }
  virtual bool enableLFO(int channel) const { return false; }
};

class DefaultYM2151Driver : public YM2151Driver {
public:
  std::string getName() const override { return "default"; }
};

std::unique_ptr<YM2151Driver> createDriver(const std::string& name);
bool parseDriverMetadataLine(const std::string& line, OPMDriverData& driverData);
