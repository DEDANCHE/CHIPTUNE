#pragma once
// CHIPTUNE NTS-1 mkII oscillator adapter. Copyright (C) 2026 DEDANCHE.
// Licensed under the GNU General Public License v3.0 (see LICENSE).
/*
 *  File: osc.h
 *
 *  CHIPTUNE APU oscillator: adapts chiptune::Apu to the NTS-1 mkII Processor API.
 *  Written against the dummy-osc template (processor.h / unit_osc.h).
 */
#include "processor.h"
#include "unit_osc.h"
#include <cmath>
#include <cstdint>
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif

#include "apu_core.h"
#include "presets.h"
#include "parameters.h"

class Osc : public Processor
{
public:
  uint32_t getBufferSize() const override final { return 0; } // osc units get no SDRAM

  enum
  {
#define PARAM_ENUM(symbol, maximum, initial, type, label) symbol,
    CHIPAPU_PARAMETERS(PARAM_ENUM)
#undef PARAM_ENUM
    NUM_PARAMS
  };

  void setParameter(uint8_t index, int32_t value) override final
  {
    ensureInit();
    if (index >= NUM_PARAMS) return;
#define PARAM_MAX(symbol, maximum, initial, type, label) maximum,
    static constexpr int32_t maxValue[NUM_PARAMS] = {CHIPAPU_PARAMETERS(PARAM_MAX)};
#undef PARAM_MAX
    static_assert(maxValue[PRESET] + 1 == chiptune::kNumPresets, "Preset range mismatch");
    value = value < 0 ? 0 : (value > maxValue[index] ? maxValue[index] : value);
    pending_[index] = value;
    pendingMask_ |= static_cast<uint16_t>(1U << index);
  }

  // Resolve a callback batch without consuming it: parameter reads during recall
  // must not split a preset and its overrides into separate transactions.
  int32_t getParameterValue(uint8_t index)
  {
    ensureInit();
    const chiptune::ApuParams p = resolvedParams();
    switch (index)
    {
    case PRESET: return pendingMask_ & (1U << PRESET) ? clampPreset(pending_[PRESET]) : presetIndex_;
    case COLOR: return p.color;
    case DETN: {
      const float knob = p.chip == chiptune::kChipSID ? p.cutoff : p.detune * (1.f / 16.f);
      return static_cast<int32_t>(std::floor((knob < 0.f ? 0.f : (knob > 1.f ? 1.f : knob)) * 1023.f + 0.5f));
    }
    case CHIP: return chipValue(p);
    case ARP: return p.arp + 6 * p.arpRate;
    case SUB:
      if (p.articulation) return 19 + p.articulation;
      if (p.chip == chiptune::kChipAY && p.voice == 1 && p.sub >= 3) return 18 + p.sub - 3;
      return p.voice ? p.voice + 4 : p.sub;
    case NOISE: return p.noise;
    case DECAY: return p.decay;
    case QUANT: return (p.quantize ? 1 : 0) | (p.resonant ? 2 : 0);
    case INTVL: return p.interval;
    default: return 0;
    }
  }

private:
  static int chipValue(const chiptune::ApuParams &p)
  {
    return p.chip == chiptune::kChipAY && p.ay5b ? chiptune::kChipS5B : p.chip;
  }

  static int clampPreset(int value)
  {
    return value < 0 ? 0 : (value >= chiptune::kNumPresets ? chiptune::kNumPresets - 1 : value);
  }

  static void applyParameter(chiptune::ApuParams &p, uint8_t index, int32_t value)
  {
    switch (index)
    {
    case DETN:
      p.detune = static_cast<float>(value) * (16.f / 1023.f); // register steps (non-SID)
      p.cutoff = static_cast<float>(value) * (1.f / 1023.f);  // filter closure (SID)
      break;
    case CHIP:
      // Store engine identity explicitly so a preset cannot resurrect a model
      // that the player has replaced with its primary family.
      if (value != chipValue(p)) {
        p.chip = static_cast<chiptune::Chip>(value);
        p.ay5b = value == chiptune::kChipS5B;
        p.waveBank = value == chiptune::kChipN163 ? 1 : 0;
        p.pulse1Level = p.pulse2Level = p.waveLevel = p.noiseLevel = 1.f;
        if (value == chiptune::kChipS5B) p.chip = chiptune::kChipAY;
        if (value == chiptune::kChipN163) {
          p.chip = chiptune::kChipPCE;
          // Keep the alias bank when the family change reapplies COLOR.
          p.color = static_cast<uint16_t>((p.color & 511) | 512);
        }
      }
      break;
    case ARP:
      p.arp = static_cast<uint8_t>(value % 6);
      p.arpRate = static_cast<uint8_t>(value / 6);
      break;
    case SUB:
      p.articulation = 0;
      if (value >= 20) {
        p.articulation = static_cast<uint8_t>(value - 19);
        p.voice = p.articulation == 3 ? 0 : (p.chip == chiptune::kChipNES || p.chip == chiptune::kChipGB || p.chip == chiptune::kChipVRC6 ? 3 : 1);
        p.sub = p.voice == 3 ? 2 : 0;
        break;
      }
      if (value >= 18) { p.voice = 1; p.sub = static_cast<uint8_t>(value - 15); break; }
      p.voice = value >= 5 ? static_cast<uint8_t>(value - 4) : 0;
      if (value < 5) p.sub = static_cast<uint8_t>(value);
      else {
        p.sub = value == 7 || (value >= 12 && value <= 14) ? 2 : 0;
        if (value == 8 && p.noise == 0) p.noise = 3;
      }
      break;
    case NOISE:
      p.noise = static_cast<uint8_t>(value);
      break;
    case DECAY:
      p.decay = static_cast<uint8_t>(value);
      break;
    case QUANT:
      p.quantize = (value & 1) != 0;
      p.resonant = (value & 2) != 0;
      break;
    case INTVL:
      p.interval = static_cast<uint8_t>(value);
      break;
    case COLOR:
      chiptune::applyColor(p, static_cast<uint16_t>(value));
      break;
    default:
      return;
    }
  }

public:
  const char *getParameterStrValue(uint8_t index, int32_t value) const override final
  {
    static const char *const chip[] = {"NES", "GB", "SN76489", "YM2149", "SID", "PCE/WT", "POKEY", "VRC6", "YM 5B", "X TIA", "WT4", "X FDS", "OLD PCM"};
    static const char *const arp[] = {"OFF", "MAJ", "MIN", "OCT", "5TH", "SUS4"};
    static const char *const subAy[] = {"OFF", "-1OCT", "UNI", "BZSAW", "BZTRI"};
    static const char *const subSid[] = {"OFF", "-1OCT", "UNI", "SYNC", "RING"};
    static const char *const subPce[] = {"OFF", "-1OCT", "UNI", "FM", "UNI"};
    static const char *const subPokey[] = {"OFF", "-1OCT", "UNI", "HPF", "UNI"};
    static const char *const subPaula[] = {"OFF", "-1OCT", "UNI", "FM", "AM"};
    static const char *const subOther[] = {"OFF", "-1OCT", "UNI", "UNI", "UNI"};
    static const char *const noise[] = {"OFF", "LOW", "MID", "HIGH", "METAL", "BUZZ"};
    static const char *const quant[] = {"MUSIC", "CHIP", "MUS+RES", "CHP+RES"};
    static const char *const voice[] = {"P1 ONLY", "P2 ONLY", "WAVE", "NOISE", "P2 25%", "P2 50%", "P2 75%", "WAV25%", "WAV50%", "WAV75%", "NZ 25%", "NZ 50%", "NZ 75%", "ENV SAW", "ENV TRI"};
    static const char *const intvl[] = {"UNI", "MIN3", "MAJ3", "4TH", "5TH", "OCT"};

    const char *const *sub = subOther;
    switch (resolvedParams().chip)
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
    case CHIP:  return value >= 0 && value < chiptune::kNumChips ? chip[value] : nullptr;
    case ARP: {
      static char label[16];
      static const char *const rate[] = {"60H", "1/4", "1/8", "1/16", "1/32", "30H"};
      if (value < 0 || value > 35) return nullptr;
      const char *a = arp[value % 6], *b = rate[value / 6];
      int i = 0;
      while (*a) label[i++] = *a++;
      label[i++] = ' ';
      while (*b) label[i++] = *b++;
      label[i] = 0;
      return label;
    }
    case SUB:
      if (value >= 20 && value <= 22) {
        static const char *const articulation[] = {"KICK", "TOM", "SLOW PWM"};
        return articulation[value - 20];
      }
      if (value >= 18 && value <= 19 && resolvedParams().chip != chiptune::kChipAY) return "YM ONLY";
      return value >= 0 && value <= 4 ? sub[value] : (value >= 5 && value <= 19 ? voice[value - 5] : nullptr);
    case NOISE: return value >= 0 && value <= 5 ? noise[value] : nullptr;
    case QUANT: return value >= 0 && value <= 3 ? quant[value] : nullptr;
    case INTVL: return value >= 0 && value <= 5 ? intvl[value] : nullptr;
    default:    return nullptr;
    }
  }

  // The unit adapter calls init(); lazy initialization also supports direct use.
  void init(float *) override final { ensureInit(); }

  void reset() override final
  {
    ensureInit();
    commitParameters();
    notePending_ = false;
    tickPending_ = false;
    channelPressure_ = polyPressure_ = 0;
    transitionLeft_ = 0;
    lastSample_ = 0.f;
    apu_.reset();
  }

  void suspend() override final { reset(); }

  void setPitch(float w0) { w0_ = w0; }
  void setShapeLfo(float lfo)
  {
    ensureInit();
    shapeLfo_ = lfo;
  }

  void noteOn(uint8_t note, uint8_t velo)
  {
    ensureInit();
    commitParameters();
    // Runtime pitch arrives with the next render. Start the voice only after
    // that pitch is applied, rather than scheduling edges at the previous note.
    pendingVelocity_ = velo;
    notePending_ = true;
    tickPending_ = false;
    activeNote_ = note;
    polyPressure_ = 0;
  }
  void noteOff(uint8_t note) { if (note != activeNote_) return; notePending_ = tickPending_ = false; polyPressure_ = 0; apu_.noteOff(); }
  void allNoteOff() { notePending_ = tickPending_ = false; polyPressure_ = 0; apu_.noteOff(); }

  void setTempo(float bpm) { ensureInit(); apu_.setTempo(bpm); }
  void tempo4ppqnTick(uint32_t counter) {
    ensureInit();
    if (notePending_) { pendingTick_ = counter; tickPending_ = true; }
    else { commitParameters(); apu_.tempo4ppqnTick(counter); }
  }
  // Assumes a 14-bit bend centred on 8192; +-2 semitones.
  void pitchBend(uint16_t bend)
  {
    if (bend >= 8190 && bend <= 8194) { bend_ = 0.f; return; }
    const float b = (static_cast<float>(bend) - 8192.f) * (1.f / 8192.f);
    bend_ = b < -1.f ? -1.f : (b > 1.f ? 1.f : b);
  }
  void channelPressure(uint8_t value) { ensureInit(); channelPressure_ = value > 127 ? 127 : value; }
  void aftertouch(uint8_t note, uint8_t value) { ensureInit(); if (note == activeNote_) polyPressure_ = value > 127 ? 127 : value; }

  void process(const float *__restrict, float *__restrict out, uint32_t frames) override final
  {
    ensureInit();
    commitParameters();
    apu_.setFrequency(w0_ * static_cast<float>(getSampleRate()) * (bend_ == 0.f ? 1.f : std::exp2(bend_ * (1.f / 6.f))));
    const float pressure = (channelPressure_ > polyPressure_ ? channelPressure_ : polyPressure_) * (1.f / 127.f);
    apu_.setShapeMod(shapeLfo_ + (p_.chip == chiptune::kChipSID ? -pressure : pressure));
    if (notePending_) {
      apu_.noteOn(pendingVelocity_);
      notePending_ = false;
      if (tickPending_) { apu_.tempo4ppqnTick(pendingTick_); tickPending_ = false; }
    }
    for (const float *end = out + frames; out != end; ++out) {
      float sample = apu_.render();
      if (transitionLeft_) {
        sample += (transitionFrom_ - sample) * (transitionLeft_ * (1.f / 48.f));
        --transitionLeft_;
      }
      *out = lastSample_ = sample;
    }
  }

private:
  chiptune::ApuParams resolvedParams() const { return resolvedParams(pendingMask_); }
  chiptune::ApuParams resolvedParams(uint16_t mask) const
  {
    chiptune::ApuParams p = mask & (1U << PRESET)
        ? chiptune::preset(clampPreset(pending_[PRESET])) : p_;
    // Family must resolve before COLOR, which has chip-specific semantics.
    if (mask & (1U << CHIP)) {
      applyParameter(p, CHIP, pending_[CHIP]);
      chiptune::applyColor(p, p.color);
    }
    for (uint8_t i = 0; i < NUM_PARAMS; ++i)
      if (i != PRESET && i != CHIP && (mask & (1U << i))) applyParameter(p, i, pending_[i]);
    // Pure YM envelope states are not portable FM/sync states on other chips.
    if (p.chip != chiptune::kChipAY && p.voice == 1 && p.sub >= 3) p.sub = 0;
    return p;
  }

  void commitParameters()
  {
    const uint16_t mask = pendingMask_;
    if (!mask) return;
    pendingMask_ &= ~mask;
    const auto next = resolvedParams(mask);
    if (lastSample_ != 0.f && (next.chip != p_.chip || next.ay5b != p_.ay5b || (next.color >> 7) != (p_.color >> 7) || next.voice != p_.voice)) {
      transitionFrom_ = lastSample_;
      transitionLeft_ = 48;
    }
    p_ = next;
    if (mask & (1U << PRESET)) presetIndex_ = clampPreset(pending_[PRESET]);

    apu_.setParams(p_);
  }

  void ensureInit()
  {
    if (ready_) return;
    ready_ = true;
    p_ = chiptune::preset(0);
    pendingMask_ = 0;

    presetIndex_ = 0;
    notePending_ = false;
    const float sr = static_cast<float>(getSampleRate());
    apu_.init(sr >= 8000.f ? sr : 48000.f);
    apu_.setParams(p_);
  }

  chiptune::Apu apu_;
  chiptune::ApuParams p_;
  float w0_ = 0.f;
  int32_t pending_[NUM_PARAMS] = {};
  volatile uint16_t pendingMask_ = 0;
  int presetIndex_ = 0;
  float bend_ = 0.f; // -1..1, zero from BSS so a bend before the first render survives
  bool ready_ = false;
  bool notePending_ = false;
  uint8_t pendingVelocity_ = 0;
  bool tickPending_ = false;
  uint32_t pendingTick_ = 0;
  uint8_t activeNote_ = 0, channelPressure_ = 0, polyPressure_ = 0;
  float shapeLfo_ = 0.f, lastSample_ = 0.f, transitionFrom_ = 0.f;
  uint8_t transitionLeft_ = 0;
};
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
