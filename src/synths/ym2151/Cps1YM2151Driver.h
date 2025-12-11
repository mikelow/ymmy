#pragma once

#include "YM2151Driver.h"

#include <optional>

struct OPMCPS425VolData {
  // increases or decreases attenuation based on the pitch of the note to be played. Used in conjunction with
  // ascendingKeyScaleTable and descendingKeyScaleTable. Bit 7 determines which table to use.
  uint8_t key_scale_sensitivity;
  // additional attenuation that is applied except when value is set by event 0x19. We bake it into the TL
  // value already, as event 0x19 is rarely used and we haven't added support for this condition. May remove it.
  uint8_t extra_atten;
};

// OPMCPSParams defines the cps driver data that we're adding to the "DRIVER" field of OPM files.
struct OPMCPSParams {
  bool enable_lfo;     // boolean indicating whether to enable the LFO when the instr is loaded
  bool reset_lfo;      // boolean indicating whether to reset the LFO phase on note on
  OPMCPS425VolData vol_data[4];
};

class Cps1YM2151Driver : public YM2151Driver {
public:
  std::string getName() const override { return "cps"; }
  void assignPatchToChannel(const OPMPatch& patch,
                            int channel,
                            YM2151DriverHost& host,
                            YM2151MidiChannelState& channelState) override;
  void updateChannelVolume(int channel,
                           YM2151DriverHost& host,
                           YM2151MidiChannelState& channelState) override;
  bool shouldResetLFOOnNoteOn(int channel) const override;
  bool enableLFO(int channel) const override;

private:
  bool parseCpsParams(const std::vector<uint8_t>& data, OPMCPSParams& outParams);

  std::array<std::optional<OPMCPSParams>, 8> channelParams{};
};
