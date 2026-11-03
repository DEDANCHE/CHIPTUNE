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
    COLOR = 0U, // knob A
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
    if (index >= NUM_PARAMS) return;
    static const int32_t maxValue[NUM_PARAMS] = {1023, 1023, chiptune::kNumPresets - 1, 12, 35, 19, 5, 15, 3, 5};
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
    apu_.reset();
  }

  void suspend() override final { reset(); }

  void setPitch(float w0) { w0_ = w0; }
  void setShapeLfo(float lfo)
  {
    ensureInit();
    apu_.setShapeMod(lfo);
  }

  void noteOn(uint8_t, uint8_t velo)
  {
    ensureInit();
    commitParameters();
    // Runtime pitch arrives with the next render. Start the voice only after
    // that pitch is applied, rather than scheduling edges at the previous note.
    pendingVelocity_ = velo;
    notePending_ = true;
  }
  void noteOff(uint8_t) { notePending_ = false; apu_.noteOff(); }
  void allNoteOff() { noteOff(0); }

  void setTempo(float bpm) { ensureInit(); apu_.setTempo(bpm); }
  void tempo4ppqnTick(uint32_t counter) { ensureInit(); apu_.tempo4ppqnTick(counter); }
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
    commitParameters();
    apu_.setFrequency(w0_ * static_cast<float>(getSampleRate()) * std::pow(2.f, bend_ * (2.f / 12.f)));
    if (notePending_) {
      apu_.noteOn(pendingVelocity_);
      notePending_ = false;
    }
    for (const float *end = out + frames; out != end; ++out)
      *out = apu_.render();
  }

private:
  chiptune::ApuParams resolvedParams() const
  {
    chiptune::ApuParams p = pendingMask_ & (1U << PRESET)
        ? chiptune::preset(clampPreset(pending_[PRESET])) : p_;
    // Family must resolve before COLOR, which has chip-specific semantics.
    if (pendingMask_ & (1U << CHIP)) {
      applyParameter(p, CHIP, pending_[CHIP]);
      chiptune::applyColor(p, p.color);
    }
    for (uint8_t i = 0; i < NUM_PARAMS; ++i)
      if (i != PRESET && i != CHIP && (pendingMask_ & (1U << i))) applyParameter(p, i, pending_[i]);
    return p;
  }

  void commitParameters()
  {
    if (!pendingMask_) return;
    p_ = resolvedParams();
    if (pendingMask_ & (1U << PRESET)) presetIndex_ = clampPreset(pending_[PRESET]);
    pendingMask_ = 0;
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
    apu_.init(static_cast<float>(getSampleRate()));
    apu_.setParams(p_);
  }

  chiptune::Apu apu_;
  chiptune::ApuParams p_;
  float w0_;
  int32_t pending_[NUM_PARAMS];
  uint16_t pendingMask_;
  int presetIndex_;
  float bend_; // -1..1, zero from BSS so a bend before the first render survives
  bool ready_;
  bool notePending_;
  uint8_t pendingVelocity_;
};
