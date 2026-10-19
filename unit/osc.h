#pragma once
/*
 *  File: osc.h
 *
 *  CHIPTUNE APU oscillator: adapts chiptune::Apu to the NTS-1 mkII Processor API.
 *  Written against the dummy-osc template (processor.h / unit_osc.h).
 */
#include "processor.h"
#include "unit_osc.h"

#include "apu_core.h"
#include "presets.h"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // osc units get no SDRAM

  enum
  {
    VIB = 0U, // knob A
    DETN,     // knob B
    PRESET,
    CHIP,
    ARP,
    SUB,
    NOISE,
    DECAY,
    QUANT,
    INTVL,
    NUM_PARAMS
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    ensureInit();
    // A preset overwrites the whole parameter set. The SDK re-sends every saved value on
    // load, so a value only counts as an edit when it differs from the last one seen.
    if (index < NUM_PARAMS)
    {
      const bool changed = value != seen_[index];
      seen_[index] = value;
      if (!changed) return;
    }
    switch (index)
    {
    case PRESET:
      p_ = chiptune::preset(value < 0 ? 0 : (value >= chiptune::kNumPresets ? chiptune::kNumPresets - 1 : value));
      break;
    case DETN:
      p_.detune = static_cast<float>(value) * (16.f / 1023.f); // register steps (non-SID)
      p_.cutoff = static_cast<float>(value) * (1.f / 1023.f);  // filter closure (SID)
      break;
    case CHIP:
      p_.chip = static_cast<chiptune::Chip>(value < 0 ? 0 : (value > 12 ? 12 : value));
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
    static const char *const chip[] = {"NES", "GB", "SEGA", "ATARI", "SID", "PCE", "POKEY", "VRC6", "5B", "TIA", "N163", "FDS", "PAULA"};
    static const char *const arp[] = {"OFF", "MAJ", "MIN", "OCT", "5TH", "SUS4"};
    static const char *const subAy[] = {"OFF", "-1OCT", "UNI", "BZSAW", "BZTRI"};
    static const char *const subSid[] = {"OFF", "-1OCT", "UNI", "SYNC", "RING"};
    static const char *const subPce[] = {"OFF", "-1OCT", "UNI", "FM", "UNI"};
    static const char *const subPokey[] = {"OFF", "-1OCT", "UNI", "HPF", "UNI"};
    static const char *const subPaula[] = {"OFF", "-1OCT", "UNI", "FM", "AM"};
    static const char *const subOther[] = {"OFF", "-1OCT", "UNI", "UNI", "UNI"};
    static const char *const noise[] = {"OFF", "LOW", "MID", "HIGH", "METAL", "BUZZ"};
    static const char *const quant[] = {"OFF", "ON"};
    static const char *const intvl[] = {"UNI", "MIN3", "MAJ3", "4TH", "5TH", "OCT"};

    const char *const *sub = subOther;
    switch (p_.chip)
    {
    case chiptune::kChipAY:
    case chiptune::kChipS5B:    sub = subAy; break;
    case chiptune::kChipSID:    sub = subSid; break;
    case chiptune::kChipPCE:    sub = subPce; break;
    case chiptune::kChipPaula:  sub = subPaula; break;
    case chiptune::kChipPOKEY:  sub = subPokey; break;
    default: break;
    }

    switch (index)
    {
    case PRESET: return value >= 0 && value < chiptune::kNumPresets ? chiptune::presetName(value) : nullptr;
    case CHIP:  return value >= 0 && value <= 12 ? chip[value] : nullptr;
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
  void setShapeLfo(float lfo)
  {
    ensureInit();
    apu_.setShapeMod(lfo);
  }

  void noteOn(uint8_t, uint8_t velo)
  {
    ensureInit();
    apu_.noteOn(velo);
  }
  void noteOff(uint8_t) { apu_.noteOff(); }
  void allNoteOff() { apu_.noteOff(); }

  void setTempo(float) {}
  void tempo4ppqnTick(uint32_t) {}
  // Assumes a 14-bit bend centred on 8192; +-2 semitones.
  void pitchBend(uint16_t bend)
  {
    const float b = (static_cast<float>(bend) - 8192.f) * (1.f / 8192.f);
    bend_ = b < -1.f ? -1.f : (b > 1.f ? 1.f : b);
  }
  void channelPressure(uint8_t) {}
  void aftertouch(uint8_t, uint8_t) {}

  void process(const float *__restrict, float *__restrict out, uint32_t frames) override final
  {
    ensureInit();
    apu_.setFrequency(w0_ * static_cast<float>(getSampleRate()) * std::pow(2.f, bend_ * (2.f / 12.f)));
    for (const float *end = out + frames; out != end; ++out)
      *out = apu_.render();
  }

private:
  void ensureInit()
  {
    if (ready_) return;
    ready_ = true;
    p_ = chiptune::ApuParams();
    for (int i = 0; i < NUM_PARAMS; ++i) seen_[i] = 0;
    seen_[PRESET] = -1; // so the default preset 0 is applied
    seen_[QUANT] = 1;   // header default
    apu_.init(static_cast<float>(getSampleRate()));
  }

  chiptune::Apu apu_;
  chiptune::ApuParams p_;
  float w0_;
  int32_t seen_[NUM_PARAMS];
  float bend_; // -1..1, zero from BSS so a bend before the first render survives
  bool ready_;
};
