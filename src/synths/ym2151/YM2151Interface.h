#pragma once

#include "ymfm_opm.h"
#include "OPMFileLoader.h"

class YM2151Interface : public ymfm::ymfm_interface {
public:
//  YM2151Interface();
//  ~YM2151Interface();

  YM2151Interface():
        m_chip(*this),
        m_timers{0, 0},
        m_busy_timer{ 0 },
        m_irq_status{ false }
  { }
  ~YM2151Interface() { }

  virtual void ymfm_sync_mode_write(uint8_t data) override {
    m_engine->engine_mode_write(data);
  }

  virtual void ymfm_sync_check_interrupts() override {
    m_engine->engine_check_interrupts();
  }

  virtual void ymfm_set_timer(uint32_t tnum, int32_t duration_in_clocks) override {
    if (tnum >= 2) return;
    m_timers[tnum] = duration_in_clocks;
  }

  virtual void ymfm_set_busy_end(uint32_t clocks) override {
    m_busy_timer = clocks;
  }

  virtual bool ymfm_is_busy() override {
    return m_busy_timer > 0;
  }

  virtual void ymfm_update_irq(bool asserted) override {
    m_irq_status = asserted;
  }

  void update_clocks(int cycles) {
    m_busy_timer = std::max(0, m_busy_timer - (64 * cycles));
    for (int i = 0; i < 2; ++i) {
      if (m_timers[i] > 0) {
        m_timers[i] = std::max(0, m_timers[i] - (64 * cycles));
        if (m_timers[i] <= 0) {
          m_engine->engine_timer_expired(i);
        }
      }
    }
  }

  void expire_engine_timer() {
    for (int i = 0; i < 2; ++i) {
      m_engine->engine_timer_expired(i);
      m_timers[i] = 0;
    }
    m_busy_timer = 0;
  }

  void write(uint8_t addr, uint8_t value) {
    if (ymfm_is_busy()) {
      expire_engine_timer();
    }
    m_chip.write_address(addr);
    m_chip.write_data(value);
    expire_engine_timer();
  }

  void changePreset(OPMPatch patch, int chan) {
    write(0x0F, (patch.channelParams.NE << 7) | (patch.lfoParams.NFRQ));
    write(0x18, patch.lfoParams.LFRQ);
    write(0x19, patch.lfoParams.PMD | 0x80);
    write(0x19, patch.lfoParams.AMD & 0x7F);
    write(0x1B, patch.lfoParams.WF);
    write(0x20 + chan, patch.channelParams.PAN | (patch.channelParams.FL << 3) | patch.channelParams.CON);
    write(0x38 + chan, (patch.channelParams.PMS << 4) | patch.channelParams.AMS);

    for (size_t i = 0; i < sizeof(patch.opParams) / sizeof(patch.opParams[0]); i++) {
      OPMOpParams& op = patch.opParams[i];
      write(0x40 + (i*8) + chan, (op.DT1 << 4) | op.MUL);
      write(0x60 + (i*8) + chan, op.TL);
      write(0x80 + (i*8) + chan, (op.KS << 6) | op.AR);
      write(0xA0 + (i*8) + chan, op.AMS_EN | op.D1R);
      write(0xC0 + (i*8) + chan, (op.DT2 << 6) | op.D2R);
      write(0xE0 + (i*8) + chan, (op.D1L << 4) | op.RR);
    }

    // TODO: Need to consider how to handle SLOT_MASK
  }

//  void generate(int16_t* output, uint32_t numsamples) {
//    int s = 0;
//    int ls, rs;
//    update_clocks(numsamples);
//    for (uint32_t i = 0; i < numsamples; i++) {
//      m_chip.generate(&opm_out);
//      ls = opm_out.data[0];
//      rs = opm_out.data[1];
//      if (ls < -32768) ls = -32768;
//      if (ls > 32767) ls = 32767;
//      if (rs < -32768) rs = -32768;
//      if (rs > 32767) rs = 32767;
//      output[s++] = ls;
//      output[s++] = rs;
//    }
//  }

  void generate(float* out[], int numSamples) {
    constexpr int maxInt = 32767;
    update_clocks(numSamples);
    for (uint32_t i = 0; i < numSamples; ++i) {
      m_chip.generate(&opm_out);
      for (int c=0; c<2; ++c) {
        if (opm_out.data[c] > 32767 || opm_out.data[c] < -32768) {
          printf("NEED TO CLIP");
        }
        float normalizedSample = (static_cast<float>(opm_out.data[c]) / maxInt);
        out[c][i] = normalizedSample;
      }
    }
  }

  uint32_t sample_rate(uint32_t input_clock) const {
    return m_chip.sample_rate(input_clock);
  }

  // Function to convert an array of 32-bit unsigned ints to 32-bit floats
//  std::vector<float> convertToFloat(int numSamples, const int32_t input[], float output[]) {
////    std::vector<float> output;
////    output.reserve(input.size());
//
//    const float maxInt = static_cast<float>(UINT32_MAX); // Maximum value for a 32-bit unsigned int
//
//    for (auto sample : input) {
//      // Normalize the unsigned integer and then convert it to a float in the range -1.0 to 1.0
//      float normalizedSample = (static_cast<float>(sample) / maxInt) * 2.0f - 1.0f;
//      output.push_back(normalizedSample);
//    }
//
//    return output;
//  }


  uint8_t read_status() {
    return m_chip.read_status();
  }

  bool irq() {
    return m_irq_status;
  }

private:
  ymfm::ym2151 m_chip;
  int32_t m_timers[2];
  int32_t m_busy_timer;
  bool m_irq_status;

  ymfm::ym2151::output_data opm_out;
};
