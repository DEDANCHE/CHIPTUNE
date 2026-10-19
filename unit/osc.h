#pragma once
/*
 *  File: osc.h
 *
 *  CHIPTUNE APU oscillator: adapts chiptune::Apu to the NTS-1 mkII Processor API.
 *  Written against the dummy-osc template (processor.h / unit_osc.h); not yet
 *  built with the SDK toolchain.
 */
#include "processor.h"
#include "unit_osc.h"

#include "apu_core.h"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // osc units get no SDRAM

  enum
  {
    DUTY = 0U,
    DETN,
    CHIP,
    ARP,
    SUB,
    NOISE,
    DECAY,
    QUANT,
    INTVL,
    VIB,
    NUM_PARAMS
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    ensureInit();
    switch (index)
    {
    case DUTY:
      p_.duty = static_cast<uint8_t>(value >> 8); // 0..1023 -> 0..3 (POKEY: distortion mode)
      break;
    case DETN:
      p_.detune = static_cast<float>(value) * (16.f / 1023.f); // register steps (non-SID)
      p_.cutoff = static_cast<float>(value) * (1.f / 1023.f);  // filter closure (SID)
      break;
    case CHIP:
      p_.chip = static_cast<chiptune::Chip>(value < 0 ? 0 : (value > 6 ? 6 : value));
      break;
    case ARP:
      p_.arp = static_cast<uint8_t>(value);
      break;
    case SUB:
      p_.sub = static_cast<uint8_t>(value);
      break;
    case NOISE:
      p_.noise = static_cast<uint8_t>(value);
      break;
    case DECAY:
      p_.decay = static_cast<uint8_t>(value);
      break;
    case QUANT:
      p_.quantize = value != 0; // period-register quantise (non-SID)
      p_.resonant = value != 0; // filter resonance (SID)
      break;
    case INTVL:
      p_.interval = static_cast<uint8_t>(value);
      break;
    case VIB:
      p_.vibrato = static_cast<uint8_t>(value);
      break;
    default:
      return;
    }
    apu_.setParams(p_);
  }

  const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    static const char *const chip[] = {"NES", "GB", "SEGA", "ATARI", "SID", "PCE", "POKEY"};
    static const char *const arp[] = {"OFF", "MAJ", "MIN", "OCT", "5TH", "SUS4"};
    static const char *const sub[] = {"OFF", "-1OCT", "UNI", "BZSAW", "BZTRI"};
    static const char *const noise[] = {"OFF", "LOW", "MID", "HIGH", "METAL", "BUZZ"};
    static const char *const quant[] = {"OFF", "ON"};
    static const char *const intvl[] = {"UNI", "MIN3", "MAJ3", "4TH", "5TH", "OCT"};

    switch (index)
    {
    case CHIP:  return value >= 0 && value <= 6 ? chip[value] : nullptr;
    case ARP:   return value >= 0 && value <= 5 ? arp[value] : nullptr;
    case SUB:   return value >= 0 && value <= 4 ? sub[value] : nullptr;
    case NOISE: return value >= 0 && value <= 5 ? noise[value] : nullptr;
    case QUANT: return value >= 0 && value <= 1 ? quant[value] : nullptr;
    case INTVL: return value >= 0 && value <= 5 ? intvl[value] : nullptr;
    default:    return nullptr;
    }
  }

  // The SDK never calls init() from the dummy-osc unit.cc, so all entry points
  // that touch the APU go through ensureInit().
  void init(float *) override final { ensureInit(); }

  void setPitch(float w0) { w0_ = w0; }
  void setShapeLfo(float) {}

  void noteOn(uint8_t, uint8_t velo)
  {
    ensureInit();
    apu_.noteOn(velo);
  }
  void noteOff(uint8_t) { apu_.noteOff(); }
  void allNoteOff() { apu_.noteOff(); }

  void setTempo(float) {}
  void tempo4ppqnTick(uint32_t) {}
  void pitchBend(uint16_t) {}
  void channelPressure(uint8_t) {}
  void aftertouch(uint8_t, uint8_t) {}

  void process(const float *__restrict, float *__restrict out, uint32_t frames) override final
  {
    ensureInit();
    apu_.setFrequency(w0_ * static_cast<float>(getSampleRate()));
    for (const float *end = out + frames; out != end; ++out)
      *out = apu_.render();
  }

private:
  void ensureInit()
  {
    if (ready_) return;
    ready_ = true;
    p_ = chiptune::ApuParams();
    apu_.init(static_cast<float>(getSampleRate()));
  }

  chiptune::Apu apu_;
  chiptune::ApuParams p_;
  float w0_;
  bool ready_;
};
