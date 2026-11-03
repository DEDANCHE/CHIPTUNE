#pragma once
// Chiptune APU core. Two engines share one voice front-end (note, arp, vibrato):
//   * event engine  (NES, GB, SN76489, YM2149, HuC6280, POKEY, VRC6, 5B, TIA, N163, FDS, Paula): channels run in the chip clock
//     domain; each output change is a band-limited step (blip-style).
//   * analog engine (SID): per-sample phase accumulators with polyBLEP,
//     ADSR and a state-variable low-pass.
// Header-only, no heap. Builds on the host for tests and on Cortex-M7.

#include <cmath>
#include <cstdint>

namespace chiptune {

namespace detail {
constexpr float kNesNoise[16] = {4,   8,   16,  32,  64,  96,   128,  160,
                                 202, 254, 380, 508, 762, 1016, 2034, 4068};
constexpr float kGbNoise[16] = {8,    16,   32,   64,    128,   256,   512,    1024,
                                2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144};
// SN76489 shift rates: clk/512, clk/1024, clk/2048 -> LFSR period in chip clocks (x2)
constexpr float kPsgNoise[16] = {256, 512, 1024, 1024, 1024, 1024, 1024, 1024,
                                 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024};
// YM2149 noise period register N (1..31) -> 16 * N chip clocks
constexpr float kAyNoise[16] = {16,  32,  48,  64,  96,  128, 160, 192,
                                224, 256, 320, 384, 448, 496, 496, 496};
constexpr uint8_t kDuty[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 1, 1, 0, 0, 0},
                                 {1, 0, 0, 1, 1, 1, 1, 1}};
constexpr int kNumIntervals = 6;

// Output stage: chips are mixed at their hardware-relative levels (the NES mixer and the GB / PCE
// wave channels sit well below the PSG-style chips), so each chip gets make-up gain, then a soft
// knee keeps peaks below full scale instead of hard clipping.
inline float softLimit(float y) {
  constexpr float knee = 0.75f;
  const float a = y < 0.f ? -y : y;
  if (a <= knee) return y;
  const float t = (a - knee) * (1.f / (1.f - knee));
  const float o = knee + (1.f - knee) * t / (1.f + t);
  return y < 0.f ? -o : o;
}
constexpr int8_t kIntervals[kNumIntervals] = {0, 3, 4, 5, 7, 12};
constexpr int8_t kArp[6][3] = {{0, 0, 0}, {0, 4, 7}, {0, 3, 7}, {0, 12, 0}, {0, 7, 12}, {0, 5, 7}};
constexpr uint8_t kArpLen[6] = {1, 3, 3, 2, 3, 3};
// NOISE setting (off, low, mid, high, metal, buzz) -> noise table index, per chip
constexpr uint8_t kNoiseIdx[13][6] = {{0, 11, 7, 3, 4, 11},  // NES
                                     {0, 11, 7, 3, 4, 11},  // GB
                                     {0, 2, 1, 0, 0, 0},    // SN76489
                                     {0, 13, 9, 4, 1, 13},  // YM2149
                                     {0, 0, 0, 0, 0, 0},    // SID (unused)
                                     {0, 0, 1, 3, 5, 0},    // HuC6280
                                     {0, 6, 4, 2, 0, 7},    // POKEY
                                     {0, 0, 0, 0, 0, 0},    // VRC6 (no noise channel)
                                     {0, 13, 9, 4, 1, 13},  // Sunsoft 5B
                                     {0, 13, 8, 3, 0, 15},  // TIA
                                     {0, 0, 0, 0, 0, 0},    // N163 (no noise channel)
                                     {0, 0, 0, 0, 0, 0},    // FDS (no noise channel)
                                     {0, 13, 9, 4, 1, 13}}; // Paula (noise sample)
// HuC6280 noise: 64 * (32 - n) chip clocks per LFSR step
constexpr float kPceNoise[16] = {2048, 1024, 512, 256, 128, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};
// POKEY noise divider: (N + 1) * 28 chip clocks (64 kHz base clock)
constexpr float kPokeyNoise[16] = {56,  112,  224,  448,  896,  1792, 3584, 5600,
                                   5600, 5600, 5600, 5600, 5600, 5600, 5600, 5600};
// TIA noise divider: N * 114 chip clocks (31.4 kHz base clock)
constexpr float kTiaNoise[16] = {114,  228,  342,  456,  570,  684,  798,  912,
                                 1140, 1368, 1710, 2052, 2394, 2736, 3192, 3648};
// TIA DUTY -> poly mode used by pokeyStep: pure, 4-bit, 5-bit, 9-bit noise
constexpr uint8_t kTiaMode[4] = {0, 1, 2, 4};
// VRC6 pulse: high steps out of 16 for the four DUTY zones
constexpr uint8_t kVrc6Duty[4] = {2, 4, 8, 12};
// POKEY distortion mode (DUTY) -> divider fires per audible cycle
constexpr float kPokeySteps[5] = {2.f, 15.f, 31.f, 31.f, 2.f};
constexpr float kSidNoiseHz[6] = {0.f, 150.f, 800.f, 3500.f, 9000.f, 40.f};
} // namespace detail

// Band-limited step synthesis. Every change of the chip's output level is
// added as a windowed-sinc impulse at its exact fractional sample position;
// reading integrates the impulses back into a band-limited step.
class BlipBuf {
public:
  static constexpr int kTaps = 16;
  static constexpr int kPhases = 64;
  static constexpr int kSize = 64; // power of two, > kTaps + 1

  void init() {
    const float pi = 3.14159265358979f;
    const float cutoff = 0.92f; // fraction of Nyquist
    for (int p = 0; p < kPhases; ++p) {
      const float f = static_cast<float>(p) / kPhases;
      float sum = 0.f;
      for (int k = 0; k < kTaps; ++k) {
        const float x = static_cast<float>(k - (kTaps / 2 - 1)) - f;
        const float sx = x * cutoff;
        const float s = (sx > -1e-6f && sx < 1e-6f) ? 1.f : std::sin(pi * sx) / (pi * sx);
        const float w = 0.42f + 0.5f * std::cos(pi * x / (kTaps / 2)) +
                        0.08f * std::cos(2.f * pi * x / (kTaps / 2));
        kernel_[p][k] = s * w;
        sum += s * w;
      }
      for (int k = 0; k < kTaps; ++k) kernel_[p][k] /= sum;
    }
    clear();
  }

  void clear() {
    for (int i = 0; i < kSize; ++i) buf_[i] = 0.f;
    pos_ = 0;
    acc_ = 0.f;
  }

  // t: time of the step inside the sample about to be read, in samples [0, 1).
  void addStep(float t, float delta) {
    int phase = static_cast<int>(t * kPhases);
    if (phase < 0) phase = 0;
    if (phase >= kPhases) phase = kPhases - 1;
    const float *h = kernel_[phase];
    const int base = pos_ + 1;
    for (int k = 0; k < kTaps; ++k) buf_[(base + k) & (kSize - 1)] += delta * h[k];
  }

  float read() {
    acc_ += buf_[pos_];
    buf_[pos_] = 0.f;
    pos_ = (pos_ + 1) & (kSize - 1);
    return acc_;
  }

  void zeroAcc() { acc_ = 0.f; }

  // Reuse the event engine's kernel for SID reset discontinuities.
  const float *kernel(float t) const {
    const int p = static_cast<int>(t * kPhases);
    return kernel_[p < 0 ? 0 : (p >= kPhases ? kPhases - 1 : p)];
  }

private:
  float kernel_[kPhases][kTaps];
  float buf_[kSize];
  int pos_ = 0;
  float acc_ = 0.f;
};

enum Chip : uint8_t {
  kChipNES = 0,
  kChipGB = 1,
  kChipPSG = 2, // Sega SN76489
  kChipAY = 3,  // Atari ST YM2149 (AY-3-8910 family)
  kChipSID = 4, // Commodore 64 SID
  kChipPCE = 5,   // PC Engine HuC6280
  kChipPOKEY = 6, // Atari 8-bit POKEY
  kChipVRC6 = 7,  // Konami VRC6: 2 pulses with 16-step duty + saw
  kChipS5B = 8,   // Sunsoft 5B (YM2149 core at 1.79 MHz)
  kChipTIA = 9,   // Atari 2600 TIA: 5-bit dividers, poly-counter waveforms
  kChipN163 = 10,  // Namco 163: 16-sample 4-bit wavetables
  kChipFDS = 11,   // Experimental FDS: 64-sample carrier and table-driven modulation counter
  kChipPaula = 12, // Amiga Paula: 32-sample 8-bit samples, period-register tuning
  kNumChips = 13
};

struct ApuParams {
  uint8_t articulation = 0; // 0 normal, 1 kick, 2 tom, 3 slow/deep SID PWM
  Chip chip = kChipNES;
  uint8_t duty = 2;      // 0..3. NES/GB: pulse duty 12.5/25/50/75 %, GB: wave table.
                         //       SID: waveform tri / saw / pulse(PWM) / saw&pulse.
                         //       PCE: wave table. POKEY: pure / 4-bit / 5-bit / 5&4-bit poly.
                         //       TIA: pure / 4-bit / 5-bit / 9-bit noise. VRC6: pulse duty.
                         //       N163 / FDS / Paula: wave table (bell|sine / saw / organ / buzz).
  uint8_t arp = 0;       // 0 off, 1 maj, 2 min, 3 oct, 4 5th+oct, 5 sus4
  uint8_t sub = 0;       // 0 off, 1 -12 st, 2 unison. 3/4: YM2149/5B envelope buzz (saw/tri),
                         //   SID 3 = hard sync, 4 = ring mod, PCE 3 = LFO FM, POKEY 3 = high-pass (4 = unison on both),
                         //   Paula 3 = FM, 4 = AM (voice 2 modulates voice 1)
  uint8_t noise = 0;     // 0 off, 1 low, 2 mid, 3 high, 4 metal, 5 buzz
  uint8_t decay = 0;     // 0 hold, 1..15 control ticks per envelope step
  uint8_t interval = 0;  // voice 2 interval index, see detail::kIntervals
  uint8_t vibrato = 0;   // 0..15
  bool quantize = true;  // round to the chip's period registers
  float detune = 0.f;    // voice 2 offset in register steps (not SID)
  float cutoff = 0.f;    // SID only: 0 = filter open .. 1 = closed
  bool resonant = false; // SID only: high filter resonance
  uint16_t color = 512; // knob A: four zones, or eight waves/duties on GB/PCE/VRC6
  bool colorEnabled = false; // old callers can still set duty directly
  uint8_t voice = 0; // 0 layered, 1 P1, 2 P2, 3 wave, 4 noise; 5..7 P2, 8..10 wave, 11..13 noise balance
  uint8_t arpRate = 0; // tracker 60 Hz, quarter/eighth/16th/32nd, tracker 30 Hz
  bool ay5b = false; // Sunsoft clock variant within the YM2149 family
  uint8_t waveBank = 0; // PCE: 0 five-bit waves, 1 short four-bit waves
  float pwmDepth = 0.4f;
  float pwmRate = 0.8f;
  float pulse1Level = 1.f, pulse2Level = 1.f, waveLevel = 1.f, noiseLevel = 1.f;
};

constexpr int kNumPrimaryChips = 8;
inline Chip primaryChip(Chip c) {
  return c == kChipS5B ? kChipAY : (c == kChipTIA ? kChipPOKEY :
      (c == kChipN163 || c == kChipFDS || c == kChipPaula ? kChipPCE : c));
}

inline void applyColor(ApuParams &p, uint16_t value) {
  p.color = value > 1023 ? 1023 : value;
  p.colorEnabled = true;
  const bool wave = p.chip == kChipGB || p.chip == kChipPCE || p.chip == kChipN163;
  p.duty = static_cast<uint8_t>((p.color >> (wave ? 7 : 8)) & 3);
  if (p.chip == kChipPCE) p.waveBank = p.color >= 512 ? 1 : 0;
  if (p.chip == kChipSID) {
    const float macro = (p.color & 255) * (1.f / 255.f);
    p.pwmDepth = 0.45f * macro;
    p.pwmRate = 0.05f + 7.95f * macro;
  }
  if (p.chip == kChipPSG) p.pulse2Level = 1.f - (p.color & 511) * (1.f / 511.f);
}

class Apu {
public:
  // Explicitly resets all state; safe even if static constructors never ran.
  void init(float sampleRate) {
    sr_ = sampleRate;
    p_ = ApuParams();
    p1_ = p2_ = sub_ = noise_ = env_ = Chan();
    freq_ = 440.f;
    vol_ = 15.f;
    level_ = dc_ = 0.f;
    vibPhase_ = vibValue_ = 0.f;
    envTicks_ = arpTick_ = arpStep_ = vibDelay_ = 0;
    gate_ = false;
    hpLatch_ = false;
    shapeMod_ = 0.f;
    shapeTarget_ = 0.f;
    tempo_ = 120.f;
    detuneCurrent_ = 0.f;
    sidInc2_ = 0;
    p1Base_ = 1.0e9f;
    for (int i = 0; i < 4; ++i) sv_[i] = SidVoice();
    sidState_ = kIdle;
    sidEnv_ = sidLow_ = sidBand_ = pwPhase_ = 0.f;
    blip_.init();
    buildWaves();
    ctrlPeriod_ = sr_ / 240.f;
    setChip(p_.chip);
    updateSidCoefs();
    reset();
  }

  // Clear voice and filter history without rebuilding tables or changing the patch.
  void reset() {
    pitchEnv_ = 0.f;
    shapeMod_ = shapeTarget_ = 0.f;
    detuneCurrent_ = p_.detune;
    updateSidCoefs();
    p1_ = p2_ = sub_ = noise_ = env_ = Chan();
    vol_ = 0.f;
    level_ = dc_ = ctrlAcc_ = 0.f;
    vibPhase_ = vibValue_ = 0.f;
    envTicks_ = arpTick_ = arpStep_ = vibDelay_ = 0;
    gate_ = false;
    hpLatch_ = false;
    p1Base_ = 1.0e9f;
    sidInc2_ = 0;
    for (int i = 0; i < 4; ++i) sv_[i] = SidVoice();
    sidState_ = kIdle;
    sidEnv_ = sidLow_ = sidBand_ = pwPhase_ = 0.f;
    for (int i = 0; i < 32; ++i) sidHist_[i] = 0.f;
    sidHistPos_ = 0;
    for (int i = 0; i < 32; ++i) sidSyncBuf_[i] = sidDryHist_[i] = 0.f;
    sidSyncPos_ = 0;
    sidF_ = sidFTarget_;
    sidQ_ = sidQTarget_;
    sidInputGain_ = p_.resonant ? 0.45f : 1.f;
    sidWet_ = sidBypass_ ? 0.f : 1.f;
    pwmDepthCurrent_ = p_.pwmDepth;
    pwmRateCurrent_ = p_.pwmRate;
    arpPhase_ = 0.f;
    fdsCounter_ = 0;
    fdsBasePitch_ = 1.f;
    blip_.clear();
    dirty_ = false;
  }

  void setParams(const ApuParams &p) {
    const Chip oldChip = p_.chip;
    const bool oldVariant = p_.ay5b;
    p_ = p;
    sanitize();
    const bool changedChip = p_.chip != oldChip || p_.ay5b != oldVariant;
    if (changedChip) setChip(p_.chip);
    if (!gate_) detuneCurrent_ = p_.detune;
    applyOn();
    updateSidCoefs();
    if (changedChip) {
      // Timers belong to the old chip clock; SID leaves event timers stopped.
      // Rebuild voice history while retaining the held gate and envelope level.
      const bool held = gate_;
      const float volume = vol_;
      reset();
      gate_ = held;
      vol_ = volume;
      restartChannels();
      if (held) sidState_ = kAttack;
    }
    dirty_ = true;
  }

  void setFrequency(float hz) {
    const float hi = 0.45f * sr_;
    freq_ = !(hz >= 8.f) ? 8.f : (hz > hi ? hi : hz); // also maps NaN to 8 Hz
  }

  // Shape LFO / external modulation, -1..1: SID filter cutoff, register detune elsewhere.
  void setShapeMod(float m) {
    const float c = !(m >= -1.f) ? -1.f : (m > 1.f ? 1.f : m);
    shapeTarget_ = c;
  }

  void setTempo(float bpm) {
    if (bpm >= 20.f && bpm <= 400.f) tempo_ = bpm;
  }

  void tempo4ppqnTick(uint32_t counter) {
    if (!gate_ || p_.arpRate < 1 || p_.arpRate > 4) return;
    const float position = static_cast<float>(counter % 48U) * (1U << (p_.arpRate - 1)) * 0.25f;
    const unsigned step = static_cast<unsigned>(position);
    arpStep_ = static_cast<uint8_t>(step % detail::kArpLen[p_.arp]);
    arpPhase_ = position - step;
    updatePeriods();
  }

  void noteOn(uint8_t velocity) {
    pitchEnv_ = p_.articulation == 1 ? 24.f : (p_.articulation == 2 ? 12.f : 0.f);
    const bool wasIdle = sidState_ == kIdle;
    gate_ = true;
    vol_ = static_cast<float>(1 + (velocity * 14) / 127);
    envTicks_ = 0;
    arpTick_ = 0;
    arpStep_ = 0;
    arpPhase_ = 0.f;
    fdsCounter_ = 0;
    detuneCurrent_ = p_.detune;
    vibPhase_ = 0.f;
    vibValue_ = 0.f;
    vibDelay_ = 48;
    restartChannels();
    if (wasIdle) {
      for (int i = 0; i < 4; ++i) sv_[i] = SidVoice();
      updatePeriods();
    }
    sidState_ = kAttack;
    dirty_ = true;
  }

  void noteOff() {
    gate_ = false;
    applyOn();
    if (sidState_ != kIdle) sidState_ = kRelease;
    dirty_ = true;
  }

  float render() {
    if (++ctrlAcc_ >= ctrlPeriod_) {
      ctrlAcc_ -= ctrlPeriod_;
      controlTick();
    }
    if (p_.chip == kChipSID) return renderSid();

    if (dirty_) {
      syncLevel(0.f);
      dirty_ = false;
    }

    const bool sq = squareChip();
    const bool pce = p_.chip == kChipPCE;
    const bool vrc6 = p_.chip == kChipVRC6;
    const bool pokey = pokeyLike();
    const Kind k12 = (pce || wavetable()) ? Kind::kWave : (vrc6 ? Kind::kPulse16 : (pokey ? Kind::kPokey : Kind::kPulse));
    const Kind ks = pokey ? Kind::kPokey : (vrc6 ? Kind::kSaw : (sq ? Kind::kPulse : Kind::kWave));
    const Kind k2 = p_.chip == kChipFDS ? Kind::kFdsMod : (paulaFm() ? Kind::kFmMod : k12);
    const Kind kn = pokey ? Kind::kPokey : Kind::kNoise;
    const uint8_t pm = pokey ? pokeyMode() : 0;
    const uint8_t table = waveIndex();
    const uint8_t d1 = pokey ? pm : (sq ? 2 : (pce ? table : p_.duty));
    const uint8_t d2 = pokey ? pm : (sq ? 2 : (pce ? static_cast<uint8_t>((table + 1) & 7) : static_cast<uint8_t>((p_.duty + 1) & 3)));
    const uint8_t ds = pce ? table : (pokey ? 0 : 2);
    const uint8_t dn = pokey ? 4 : 0;
    const bool lfo = fmMode();
    const bool hp = hpMode();
    float remaining = cps_;
    for (;;) {
      float dt = remaining;
      if (p1_.on && p1_.timer < dt) dt = p1_.timer;
      if (p2_.on && p2_.timer < dt) dt = p2_.timer;
      if (sub_.on && sub_.timer < dt) dt = sub_.timer;
      if (noise_.on && noise_.timer < dt) dt = noise_.timer;
      if (env_.on && env_.timer < dt) dt = env_.timer;
      remaining -= dt;

      bool fired = false;
      if (p1_.on) fired |= tick(p1_, dt, k12, d1);
      if (p2_.on) {
        const bool f2 = tick(p2_, dt, k2, d2);
        if (f2 && p_.chip == kChipFDS) setCarrierPeriod(fdsPeriod());
        else if (f2 && lfo)
          setCarrierPeriod(p1Base_ * (1.f + lfoDepth() * (static_cast<float>(p2_.level) - 15.5f) * (1.f / 15.5f)));
        fired |= f2;
      }
      if (sub_.on) {
        const bool fs = tick(sub_, dt, ks, ds);
        if (fs && hp) hpLatch_ = p1_.bit != 0; // POKEY high-pass: ch3 clock samples ch1
        fired |= fs;
      }
      if (noise_.on) fired |= tick(noise_, dt, kn, dn);
      if (env_.on) fired |= tick(env_, dt, Kind::kEnv, 0);
      if (fired) syncLevel((cps_ - remaining) / cps_);
      if (remaining <= 0.f) break;
    }

    // The integrator is exact; DC is removed here so release does not undershoot.
    float y = blip_.read();
    if (gate_) {
      dc_ += (y - dc_) * (1.f / 3300.f);
    } else {
      dc_ *= 0.99f;
      if (level_ == 0.f && y < 1e-5f && y > -1e-5f) blip_.zeroAcc(); // shed float drift
    }
    y -= dc_;
    return detail::softLimit(y * outGain_);
  }

private:
  enum class Kind { kPulse, kWave, kNoise, kEnv, kPokey, kPulse16, kSaw, kFmMod, kFdsMod };
  enum SidState : uint8_t { kIdle, kAttack, kDecay, kSustain, kRelease };

  struct Chan {
    float timer = 0.f;
    float period = 1.0e9f;
    uint32_t lfsr = 1;
    uint8_t seq = 0;
    uint8_t bit = 0;
    uint8_t level = 0; // wave / envelope output, 0..15 (0..31 HuC6280, 0..63 FDS, 0..255 Paula)
    uint8_t p5 = 0x1F; // POKEY 5-bit poly counter
    uint8_t p4 = 0xF;  // POKEY 4-bit poly counter
    bool on = false;
  };

  struct SidVoice {
    uint32_t acc = 0;
    uint32_t inc = 0;
    uint32_t lfsr = 0x7FFFF8u;
    float noise = 0.f;
  };

  struct ChipSpec {
    float clk;
    float pulseUnit; // chip clocks per register unit, pulse / square
    float waveUnit;  // chip clocks per register unit, triangle / wave
    const float *noiseTable;
    float mixGain;
    float regMax;
  };

  static uint8_t lim(uint8_t v, uint8_t hi) { return v > hi ? hi : v; }
  static float unitLevel(float v) { return !(v >= 0.f) ? 0.f : (v > 1.f ? 1.f : v); }

  // Keeps every parameter inside the range the lookup tables are sized for.
  void sanitize() {
    p_.articulation = lim(p_.articulation, 3);
    if (static_cast<unsigned>(p_.chip) >= static_cast<unsigned>(kNumChips)) p_.chip = kChipNES;
    if (p_.chip == kChipS5B) { p_.chip = kChipAY; p_.ay5b = true; }
    if (p_.chip == kChipN163) {
      p_.chip = kChipPCE; p_.waveBank = 1;
      if (p_.colorEnabled) p_.color = static_cast<uint16_t>((p_.color & 511) | 512);
    }
    p_.duty = lim(p_.duty, 3);
    p_.arp = lim(p_.arp, 5);
    p_.sub = lim(p_.sub, 4);
    p_.noise = lim(p_.noise, 5);
    p_.decay = lim(p_.decay, 15);
    p_.interval = lim(p_.interval, detail::kNumIntervals - 1);
    p_.vibrato = lim(p_.vibrato, 15);
    p_.color = p_.color > 1023 ? 1023 : p_.color;
    p_.voice = lim(p_.voice, 13);
    p_.arpRate = lim(p_.arpRate, 5);
    p_.waveBank = lim(p_.waveBank, 1);
    p_.pwmDepth = 0.45f * unitLevel(p_.pwmDepth / 0.45f);
    p_.pwmRate = !(p_.pwmRate >= 0.05f) ? 0.05f : (p_.pwmRate > 12.f ? 12.f : p_.pwmRate);
    p_.pulse1Level = unitLevel(p_.pulse1Level);
    p_.pulse2Level = unitLevel(p_.pulse2Level);
    p_.waveLevel = unitLevel(p_.waveLevel);
    p_.noiseLevel = unitLevel(p_.noiseLevel);
    p_.cutoff = unitLevel(p_.cutoff);
    p_.detune = 16.f * unitLevel(p_.detune / 16.f);
  }

  bool ayLike() const { return p_.chip == kChipAY || p_.chip == kChipS5B; }
  bool squareChip() const { return p_.chip == kChipPSG || ayLike(); }
  bool buzz() const { return ayLike() && p_.sub >= 3; }
  uint8_t envelopeShape() const {
    // Pure envelope VOICE modes choose the waveform explicitly. Layered buzz
    // modes retain all eight COLOR shapes.
    if (p_.voice == 1 && buzz()) return p_.sub == 4 ? 2 : 0;
    return p_.colorEnabled ? static_cast<uint8_t>(p_.color >> 7) : (p_.sub == 4 ? 2 : 0);
  }
  bool pokeyLike() const { return p_.chip == kChipPOKEY || p_.chip == kChipTIA; }
  uint8_t pokeyMode() const { return p_.chip == kChipTIA ? detail::kTiaMode[p_.duty] : p_.duty; }
  bool hpMode() const { return p_.chip == kChipPOKEY && p_.sub == 3; }
  bool pceLfo() const { return p_.chip == kChipPCE && p_.sub == 3; }
  bool paulaFm() const { return p_.chip == kChipPaula && p_.sub == 3; }
  bool paulaAm() const { return p_.chip == kChipPaula && p_.sub == 4; }
  // voice 2 is an FM modulator for voice 1 (knob B = depth, INTVL = ratio)
  bool fmMode() const { return pceLfo() || p_.chip == kChipFDS || paulaFm(); }
  bool wavetable() const { return p_.chip >= kChipN163 && p_.chip <= kChipPaula; }
  float waveSteps() const { return p_.chip == kChipN163 ? 16.f : (p_.chip == kChipFDS ? 64.f : 32.f); }
  float detuneEff() const { return detuneCurrent_ + shapeMod_ * 8.f; }
  uint8_t waveIndex() const {
    if (p_.colorEnabled) return static_cast<uint8_t>(p_.color >> 7);
    return p_.duty + (p_.chip == kChipPCE && p_.waveBank ? 4 : 0);
  }
  bool waveOnMain() const { return p_.chip == kChipPCE || wavetable() || p_.chip == kChipSID; }
  float gain1() const { return (p_.voice == 0 || p_.voice == 1 || p_.voice >= 5 || (p_.voice == 3 && waveOnMain())) ? p_.pulse1Level : 0.f; }
  float gain2() const {
    if (!(p_.voice == 0 || p_.voice == 2 || p_.voice >= 5)) return 0.f;
    return p_.pulse2Level * (p_.voice >= 5 && p_.voice <= 7 ? (p_.voice - 4) * 0.25f : 1.f);
  }
  float gainWave() const {
    if (!(p_.voice == 0 || p_.voice == 3 || p_.voice >= 5)) return 0.f;
    return p_.waveLevel * (p_.voice >= 8 && p_.voice <= 10 ? (p_.voice - 7) * 0.25f : 1.f);
  }
  float gainNoise() const {
    if (!(p_.voice == 0 || p_.voice == 4 || p_.voice >= 5)) return 0.f;
    return p_.noiseLevel * (p_.voice >= 11 ? (p_.voice - 10) * 0.25f : 1.f);
  }
  float lfoDepth() const {
    const float d = detuneEff() * (1.f / 16.f);
    const float c = d < 0.f ? 0.f : (d > 1.f ? 1.f : d);
    return 0.7f * c; // Every FM model has a genuinely unmodulated zero-depth setting.
  }

  void applyOn() {
    const bool g = gate_ && vol_ > 0.f;
    const bool b = buzz();
    const bool mod = fmMode() && lfoDepth() > 0.f && gain1() > 0.f;
    p1_.on = g && !b && gain1() > 0.f;
    p2_.on = g && (fmMode() ? mod : (paulaAm() ? gain1() > 0.f : gain2() > 0.f));
    sub_.on = g && p_.sub != 0 && gainWave() > 0.f && !(p_.voice == 3 && waveOnMain()) && !b && !pceLfo() && !paulaFm() && !paulaAm();
    noise_.on = g && p_.noise != 0 && gainNoise() > 0.f && p_.chip != kChipVRC6 && p_.chip != kChipN163 && p_.chip != kChipFDS;
    env_.on = g && b && gain1() > 0.f;
  }

  void restartChannels() {
    p1_ = p2_ = sub_ = noise_ = env_ = Chan();
    noise_.lfsr = p_.chip == kChipPSG ? 0x8000u
                : (p_.chip == kChipPOKEY ? 0x1FFFFu : (p_.chip == kChipTIA ? 0x1FFu : 1u));
    hpLatch_ = false;
    env_.level = 15;
    applyOn();
    updatePeriods();
    p1_.timer = p1_.period;
    p2_.timer = p2_.period;
    sub_.timer = sub_.period;
    noise_.timer = noise_.period;
    env_.timer = env_.period;
    if (hpMode()) sub_.timer = sub_.period * 1.5f;
  }

  void setCarrierPeriod(float period) {
    const float phaseLeft = p1_.period > 0.f ? unitLevel(p1_.timer / p1_.period) : 1.f;
    p1_.period = period;
    p1_.timer = phaseLeft * period;
  }

  bool tick(Chan &c, float dt, Kind kind, uint8_t arg) {
    c.timer -= dt;
    if (c.timer > 0.f) return false;
    c.timer += c.period;
    switch (kind) {
    case Kind::kPulse:
      c.seq = (c.seq + 1) & 7;
      c.bit = detail::kDuty[arg][c.seq];
      break;
    case Kind::kWave:
      c.seq = (c.seq + 1) & (p_.chip == kChipFDS ? 63 : 31);
      if (p_.chip == kChipNES)
        c.level = c.seq < 16 ? 15 - c.seq : c.seq - 16;
      else if (p_.chip == kChipPCE)
        c.level = pceWave_[arg & 7][c.seq];
      else if (p_.chip == kChipN163)
        c.level = n163Wave_[arg & 3][c.seq & 15];
      else if (p_.chip == kChipFDS) {
        c.seq &= 63;
        c.level = fdsWave_[arg & 3][c.seq];
      } else if (p_.chip == kChipPaula)
        c.level = paulaWave_[arg & 3][c.seq];
      else
        c.level = gbWave_[waveIndex() & 7][c.seq];
      break;
    case Kind::kFmMod:
      c.seq = (c.seq + 1) & 31;
      c.level = pceWave_[0][c.seq];
      break;
    case Kind::kFdsMod: {
      // Each 3-bit table entry is clocked twice; the counter wraps at seven bits.
      static const uint8_t pattern[4][32] = {
        {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,7,7,7,7,7,7,7,7,0,0,0,0,0,0,0,0},
        {2,2,2,2,0,0,0,0,6,6,6,6,0,0,0,0,2,2,2,2,0,0,0,0,6,6,6,6,0,0,0,0},
        {3,0,0,0,5,0,0,0,3,0,0,0,5,0,0,0,3,0,0,0,5,0,0,0,3,0,0,0,5,0,0,0},
        {1,1,2,0,7,7,6,0,1,1,2,0,7,7,6,0,1,1,2,0,7,7,6,0,1,1,2,0,7,7,6,0}};
      static const int8_t adjust[8] = {0,1,2,4,0,-4,-2,-1};
      const uint8_t op = pattern[p_.duty][c.seq >> 1];
      int next = op == 4 ? 0 : fdsCounter_ + adjust[op];
      next = (next + 128) & 127;
      fdsCounter_ = static_cast<int8_t>(next >= 64 ? next - 128 : next);
      c.seq = (c.seq + 1) & 63;
      break;
    }
    case Kind::kNoise:
      stepNoise(c);
      break;
    case Kind::kPokey:
      pokeyStep(c, arg);
      break;
    case Kind::kPulse16:
      c.seq = (c.seq + 1) & 15;
      c.bit = c.seq < (p_.colorEnabled ? 1 + (p_.color >> 7) : detail::kVrc6Duty[arg & 3]);
      break;
    case Kind::kSaw: // VRC6 saw: 7 steps, accumulator rate 42, top 5 bits
      c.seq = static_cast<uint8_t>((c.seq + 1) % 7);
      c.level = static_cast<uint8_t>((c.seq * 42) >> 3);
      break;
    case Kind::kEnv: {
      const uint8_t shape = envelopeShape();
      c.seq = static_cast<uint8_t>((c.seq + 1) & 31);
      const uint8_t ramp = c.seq & 15;
      const uint8_t triangle = c.seq < 16 ? 15 - ramp : ramp;
      switch (shape) {
      case 0: c.level = 15 - ramp; break;
      case 1: c.level = ramp; break;
      case 2: c.level = triangle; break;
      case 3: c.level = 15 - triangle; break;
      case 4: c.level = c.seq < 16 ? 15 : 0; break;
      case 5: c.level = ramp >> 1 << 1; break;
      case 6: c.level = triangle >> 1 << 1; break;
      default: c.level = ramp < 8 ? 15 - 2 * ramp : 0; break;
      }
      break;
    }
    }
    return true;
  }

  // POKEY output flip-flop. Poly counters step per divider fire (the real ones run
  // at the base clock); modes: 0 pure, 1 4-bit, 2 5-bit gate, 3 5-bit gate + 4-bit, 4 17-bit.
  void pokeyStep(Chan &c, uint8_t mode) {
    if (mode == 2 || mode == 3) {
      c.p5 = static_cast<uint8_t>(((c.p5 << 1) | (((c.p5 >> 4) ^ (c.p5 >> 2)) & 1)) & 0x1F);
      if (!(c.p5 & 1)) return;
    }
    if (mode == 1 || mode == 3) {
      c.p4 = static_cast<uint8_t>(((c.p4 << 1) | (((c.p4 >> 3) ^ (c.p4 >> 2)) & 1)) & 0xF);
      c.bit = c.p4 & 1;
    } else if (mode == 4 && p_.chip == kChipTIA) { // 9-bit poly
      c.lfsr = ((c.lfsr << 1) | (((c.lfsr >> 8) ^ (c.lfsr >> 4)) & 1)) & 0x1FFu;
      if (c.lfsr == 0) c.lfsr = 1;
      c.bit = c.lfsr & 1;
    } else if (mode == 4) {
      c.lfsr = ((c.lfsr << 1) | (((c.lfsr >> 16) ^ (c.lfsr >> 11)) & 1)) & 0x1FFFFu;
      if (c.lfsr == 0) c.lfsr = 1;
      c.bit = c.lfsr & 1;
    } else {
      c.bit ^= 1;
    }
  }

  void stepNoise(Chan &c) {
    const bool shortMode = p_.noise == 4 || p_.noise == 5 || (p_.chip == kChipPSG && p_.colorEnabled && p_.color >= 512);
    if (p_.chip == kChipPSG) {
      const uint32_t fb = shortMode ? (c.lfsr & 1) : ((c.lfsr ^ (c.lfsr >> 3)) & 1);
      c.lfsr = (c.lfsr >> 1) | (fb << 15);
      c.bit = c.lfsr & 1;
    } else if (ayLike()) {
      const uint32_t fb = (c.lfsr ^ (c.lfsr >> 3)) & 1;
      c.lfsr = (c.lfsr >> 1) | (fb << 16);
      c.bit = c.lfsr & 1;
    } else {
      const uint32_t fb = (c.lfsr ^ (c.lfsr >> (shortMode ? 6 : 1))) & 1;
      c.lfsr = (c.lfsr >> 1) | (fb << 14);
      c.bit = (c.lfsr & 1) ? 0 : 1;
    }
  }

  void setChip(Chip c) {
    p_.chip = c;
    switch (c) {
    case kChipNES: spec_ = {1789773.f, 2.f, 1.f, detail::kNesNoise, 1.4f, 2048.f}; break;
    case kChipGB:  spec_ = {4194304.f, 4.f, 2.f, detail::kGbNoise, 0.7f, 2048.f}; break;
    case kChipPSG: spec_ = {3579545.f, 4.f, 4.f, detail::kPsgNoise, 0.2f, 1023.f}; break;
    case kChipAY:  spec_ = {p_.ay5b ? 1789773.f : 2000000.f, p_.ay5b ? 4.f : 2.f, p_.ay5b ? 4.f : 2.f, detail::kAyNoise, 0.2f, 4095.f}; break;
    case kChipPCE: spec_ = {3579545.f, 1.f, 1.f, detail::kPceNoise, 0.25f, 4095.f}; break;
    case kChipPOKEY: spec_ = {1789773.f, 1.f, 1.f, detail::kPokeyNoise, 0.2f, 65535.f}; break;
    case kChipVRC6: spec_ = {1789773.f, 1.f, 2.f, detail::kAyNoise, 0.2f, 4095.f}; break;
    case kChipS5B: spec_ = {1789773.f, 4.f, 4.f, detail::kAyNoise, 0.2f, 4095.f}; break;
    case kChipTIA: spec_ = {3579545.f, 114.f, 114.f, detail::kTiaNoise, 0.2f, 32.f}; break;
    case kChipN163: spec_ = {1789773.f, 0.125f, 0.125f, detail::kAyNoise, 0.3f, 65535.f}; break;
    case kChipFDS: spec_ = {1789773.f, 0.125f, 0.125f, detail::kAyNoise, 0.3f, 65535.f}; break;
    case kChipPaula: spec_ = {3546895.f, 1.f, 1.f, detail::kAyNoise, 0.3f, 65535.f}; break;
    default:       spec_ = {sr_, 1.f, 1.f, detail::kNesNoise, 1.f, 2048.f}; break;
    }
    switch (c) { // make-up gain, about +3..12 dB, see detail::softLimit
    case kChipNES:   outGain_ = 4.0f; break;
    case kChipGB:    outGain_ = 3.55f; break;
    case kChipPCE:   outGain_ = 3.5f; break;
    case kChipVRC6:  outGain_ = 2.4f; break;
    case kChipN163:  outGain_ = 3.0f; break;
    case kChipFDS:   outGain_ = 3.5f; break;
    case kChipPaula: outGain_ = 2.8f; break;
    case kChipSID:   outGain_ = 2.0f; break;
    default:         outGain_ = 1.78f; break;
    }
    for (int v = 0; v < 16; ++v) {
      if (v == 0)
        amp_[v] = 0.f;
      else if (c == kChipPSG)
        amp_[v] = std::pow(10.f, -(15 - v) * 0.1f); // 2 dB per step
      else if (c == kChipPOKEY || c == kChipTIA)
        amp_[v] = static_cast<float>(v) * (1.f / 15.f); // linear 4-bit volume
      else
        amp_[v] = std::pow(2.f, -(15 - v) * 0.5f);  // 3 dB per step
    }
    cps_ = spec_.clk / sr_;
    ctrlAcc_ = 0.f;
    dirty_ = true;
  }

  void buildWaves() {
    const float tau = 6.28318530718f;
    // 31-tap Hamming low-pass (about 21 kHz at 96 kHz) decimating the 2x SID engine
    float fsum = 0.f;
    for (int k = 0; k < 31; ++k) {
      const float n = static_cast<float>(k - 15);
      const float w = tau * 0.21875f * n;
      const float s = n == 0.f ? 1.f : std::sin(w) / w;
      sidFir_[k] = s * (0.54f + 0.46f * std::cos(tau * 0.5f * n / 15.5f));
      fsum += sidFir_[k];
    }
    for (int k = 0; k < 31; ++k) sidFir_[k] /= fsum;
    auto q =  [](float v) { // -1..1 -> 0..15
      const int n = static_cast<int>((v * 0.5f + 0.5f) * 15.f + 0.5f);
      return static_cast<uint8_t>(n < 0 ? 0 : (n > 15 ? 15 : n));
    };
    auto qn = [](float v, float top) { // -1..1 -> 0..top
      const int n = static_cast<int>((v * 0.5f + 0.5f) * top + 0.5f);
      const int hi = static_cast<int>(top);
      return static_cast<uint8_t>(n < 0 ? 0 : (n > hi ? hi : n));
    };
    auto q5 = [](float v) { // -1..1 -> 0..31
      const int n = static_cast<int>((v * 0.5f + 0.5f) * 31.f + 0.5f);
      return static_cast<uint8_t>(n < 0 ? 0 : (n > 31 ? 31 : n));
    };
    for (int i = 0; i < 32; ++i) {
      const float ph = static_cast<float>(i) / 32.f;
      const float s = std::sin(tau * ph);
      gbWave_[0][i] = q(s);
      gbWave_[1][i] = q(2.f * ph - 1.f);
      gbWave_[2][i] = q(0.6f * s + 0.4f * std::sin(2.f * tau * ph));
      gbWave_[3][i] = q(ph < 0.5f ? -1.f + 4.f * ph : 3.f - 4.f * ph);
      gbWave_[4][i] = q(ph < 0.25f ? 1.f : -1.f);
      gbWave_[5][i] = q(0.7f * s + 0.3f * std::sin(3.f * tau * ph));
      gbWave_[6][i] = q((std::sin(tau * ph) + std::sin(4.f * tau * ph)) * 0.5f);
      gbWave_[7][i] = q(0.5f * s + (ph < 0.125f ? 0.5f : -0.5f));
      pceWave_[0][i] = q5(s);
      pceWave_[1][i] = q5(2.f * ph - 1.f);
      pceWave_[2][i] = q5(ph < 0.25f ? 1.f : -1.f);
      pceWave_[3][i] = q5(ph < 0.5f ? -1.f + 4.f * ph : 3.f - 4.f * ph);
      paulaWave_[0][i] = qn(2.f * ph - 1.f, 255.f);
      paulaWave_[1][i] = qn(ph < 0.25f ? 1.f : -1.f, 255.f);
      paulaWave_[2][i] = qn((s + 0.5f * std::sin(2.f * tau * ph) + 0.33f * std::sin(3.f * tau * ph)) * 0.6f, 255.f);
      paulaWave_[3][i] = qn(0.6f * (2.f * ph - 1.f) + 0.4f * std::sin(2.f * tau * ph), 255.f);
    }
    for (int i = 0; i < 16; ++i) { // Namco 163: 16-sample, 4-bit
      const float ph = static_cast<float>(i) / 16.f;
      const float th = tau * ph;
      n163Wave_[0][i] = q(0.7f * std::sin(th) + 0.3f * std::sin(3.f * th));
      n163Wave_[1][i] = q(2.f * ph - 1.f);
      n163Wave_[2][i] = q(0.6f * (std::sin(th) + 0.5f * std::sin(2.f * th) + 0.25f * std::sin(4.f * th)));
      n163Wave_[3][i] = q(0.5f * std::sin(th) + (ph < 0.25f ? 0.5f : -0.5f));
      for (int w = 0; w < 4; ++w) {
        const uint8_t v = static_cast<uint8_t>((n163Wave_[w][i] * 31 + 7) / 15);
        pceWave_[w + 4][2 * i] = pceWave_[w + 4][2 * i + 1] = v;
      }
    }
    for (int i = 0; i < 64; ++i) { // FDS: 64-sample, 6-bit
      const float ph = static_cast<float>(i) / 64.f;
      const float th = tau * ph;
      fdsWave_[0][i] = qn(std::sin(th), 63.f);
      fdsWave_[1][i] = qn(2.f * ph - 1.f, 63.f);
      fdsWave_[2][i] = qn(0.7f * std::sin(th) + 0.3f * std::sin(5.f * th), 63.f);
      fdsWave_[3][i] = qn(ph < 0.5f ? -1.f + 4.f * ph : 3.f - 4.f * ph, 63.f);
    }
  }

  // Musical mode keeps the requested fundamental. Hardware mode folds and rounds
  // to representable registers; detune remains an explicit period offset in both.
  float stepFor(float unit, float stepsPerCycle, float hz, float detuneRegs, float regMax) const {
    float reg = spec_.clk / (unit * stepsPerCycle * hz);
    if (!p_.quantize) {
      reg += detuneRegs;
      const float minReg = spec_.clk / (unit * stepsPerCycle * 0.45f * sr_);
      return (reg < minReg ? minReg : reg) * unit;
    }
    while (reg > regMax) reg *= 0.5f;
    if (p_.quantize) reg = std::floor(reg + 0.5f);
    reg += detuneRegs;
    if (reg < 1.f) reg = 1.f;
    if (reg > regMax) reg = regMax;
    return reg * unit;
  }

  // POKEY chip clocks per divider fire. Uses the 8-bit divider on the 64 kHz base
  // clock when the register fits, else the 16-bit pair on the 1.79 MHz clock.
  float pokeyPeriod(float hz, float detuneRegs, float steps) const {
    const float target = spec_.clk / (steps * hz);
    if (!p_.quantize) {
      const float period = target + detuneRegs * 28.f;
      const float minimum = spec_.clk / (steps * 0.45f * sr_);
      return period < minimum ? minimum : period;
    }
    float r = target / 28.f;
    if (r >= 1.f && r <= 256.f) {
      if (p_.quantize) r = std::floor(r + 0.5f);
      r += detuneRegs;
      return (r < 1.f ? 1.f : (r > 256.f ? 256.f : r)) * 28.f;
    }
    float n = target - 7.f;
    if (p_.quantize) n = std::floor(n + 0.5f);
    n += detuneRegs * (n > 128.f ? n * (1.f / 128.f) : 1.f);
    return (n < 1.f ? 1.f : (n > 65535.f ? 65535.f : n)) + 7.f;
  }

  // TIA: 5-bit divider (1..32) on the 31.4 kHz audio clock. Out-of-range registers
  // fold up by octaves, so low notes keep their pitch class.
  float tiaPeriod(float hz, float detuneRegs, float steps) const {
    float r = spec_.clk / (steps * hz) / 114.f;
    if (!p_.quantize) {
      r += detuneRegs;
      const float minimum = spec_.clk / (steps * 0.45f * sr_ * 114.f);
      return (r < minimum ? minimum : r) * 114.f;
    }
    while (r > 32.f) r *= 0.5f;
    if (p_.quantize) r = std::floor(r + 0.5f);
    r += detuneRegs;
    return (r < 1.f ? 1.f : (r > 32.f ? 32.f : r)) * 114.f;
  }

  uint32_t sidInc(float hz) const {
    float v = hz / (2.f * sr_) * 16777216.f; // voices run at 2x and are decimated
    if (v > 8388607.f) v = 8388607.f;
    return static_cast<uint32_t>(v);
  }

  float fdsPitch(float hz) const {
    float pitch = hz * (4194304.f / 1789773.f);
    if (p_.quantize) {
      while (pitch > 4095.f) pitch *= 0.5f;
      pitch = std::floor(pitch + 0.5f);
    }
    return pitch < 1.f ? 1.f : pitch;
  }

  float fdsPeriod() const {
    // Seven-bit counter * six-bit gain, FDS's asymmetric rounding and pitch scaling.
    const int gain = static_cast<int>(unitLevel(detuneEff() / 16.f) * 63.f + 0.5f);
    const int product = fdsCounter_ * gain;
    int scaled = static_cast<int>(std::floor(product / 16.f));
    if ((product & 15) && !(scaled & 128)) scaled += fdsCounter_ < 0 ? -1 : 2;
    while (scaled >= 192) scaled -= 256;
    while (scaled < -64) scaled += 256;
    const float offset = std::floor((fdsBasePitch_ * scaled + 32.f) / 64.f);
    const float pitch = fdsBasePitch_ + offset;
    // A stopped carrier has no scheduled events; retain a finite timer sentinel.
    return pitch <= 0.f ? 1.0e9f : 65536.f / pitch;
  }

  void updatePeriods() {
    const float arpSemi = static_cast<float>(detail::kArp[p_.arp][arpStep_]);
    const float semi2 = static_cast<float>(detail::kIntervals[p_.interval]);
    const float f1 = freq_ * std::pow(2.f, (arpSemi + vibValue_ + pitchEnv_) / 12.f);
    const float f2 = freq_ * std::pow(2.f, (arpSemi + semi2 + vibValue_ + pitchEnv_) / 12.f);
    const float fs = freq_ * std::pow(2.f, (vibValue_ + pitchEnv_ + (p_.sub == 1 ? -12.f : 0.f)) / 12.f);

    if (p_.chip == kChipSID) {
      sv_[0].inc = sidInc(f1);
      sv_[1].inc = sidInc(f2 * 1.00406f); // fixed +7 cent spread
      sidInc2_ = sv_[1].inc;
      sv_[2].inc = sidInc(p_.sub == 2 ? fs * 0.99595f : fs); // UNI: -7 cent, else it would just double voice 1
      sv_[3].inc = sidInc(detail::kSidNoiseHz[p_.noise]);
      return;
    }

    const float det = detuneEff();
    if (p_.chip == kChipFDS) {
      fdsBasePitch_ = fdsPitch(f1);
      p1Base_ = 65536.f / fdsBasePitch_;
      setCarrierPeriod(fdsPeriod());
      p2_.period = 65536.f / fdsPitch(f2);
      sub_.period = 65536.f / fdsPitch(fs);
      return;
    }
    if (pokeyLike()) {
      const bool tia = p_.chip == kChipTIA;
      const float st = detail::kPokeySteps[pokeyMode()];
      auto per = [&](float hz, float d, float s) {
        return tia ? tiaPeriod(hz, d, s) : pokeyPeriod(hz, d, s);
      };
      p1_.period = per(f1, 0.f, st);
      p2_.period = per(f2, det, st);
      // the bass voice stays a pure tone; in high-pass mode it is the sampling clock
      sub_.period = hpMode() ? per(f1, det, st) : per(fs, 0.f, 2.f);
      noise_.period = noisePeriod();
      p1Base_ = p1_.period;
      return;
    }

    const float rm = spec_.regMax;
    if (p_.chip == kChipVRC6) {
      p1_.period = stepFor(1.f, 16.f, f1, 0.f, rm);
      p2_.period = stepFor(1.f, 16.f, f2, det, rm);
      sub_.period = stepFor(2.f, 7.f, fs, 0.f, rm);
      p1Base_ = p1_.period;
      return;
    }
    const float ws = waveSteps();
    const float steps = wavetable() ? ws : (p_.chip == kChipPCE ? 32.f : 8.f);
    const float steps2 = (p_.chip == kChipFDS || paulaFm()) ? 32.f : steps; // sine modulator
    p1_.period = stepFor(spec_.pulseUnit, steps, f1, 0.f, rm);
    p2_.period = stepFor(spec_.pulseUnit, steps2, f2, fmMode() ? 0.f : det, rm);
    p1Base_ = p1_.period;
    sub_.period = squareChip() ? stepFor(spec_.pulseUnit, 8.f, fs, 0.f, rm)
                               : stepFor(spec_.waveUnit, ws, fs, 0.f, rm);
    noise_.period = noisePeriod();
    if (p_.chip == kChipPSG && p_.noise == 5) { // pitched periodic noise
      const float per = spec_.clk / (16.f * f1) * noiseScale();
      noise_.period = per < 4.f ? 4.f : per;
    }
    if (ayLike()) {
      const uint8_t shape = envelopeShape();
      const bool longCycle = (shape >= 2 && shape <= 4) || shape == 6;
      env_.period = stepFor(16.f, longCycle ? 32.f : 16.f, f1, 0.f, 65535.f);
    }
  }

  float noiseScale() const { return p_.voice == 4 ? std::pow(2.f, -detuneEff() * 0.25f) : 1.f; }
  float noisePeriod() const {
    const float period = spec_.noiseTable[detail::kNoiseIdx[p_.chip][p_.noise]] * noiseScale();
    return period < 4.f ? 4.f : period;
  }

  void controlTick() {
    pitchEnv_ *= p_.articulation == 1 ? 0.65f : 0.88f;
    if (pitchEnv_ < 0.001f) pitchEnv_ = 0.f;
    detuneCurrent_ += (p_.detune - detuneCurrent_) * 0.25f;
    shapeMod_ += (shapeTarget_ - shapeMod_) * 0.25f;
    if (std::fabs(p_.detune - detuneCurrent_) < 1e-4f) detuneCurrent_ = p_.detune;
    if (std::fabs(shapeTarget_ - shapeMod_) < 1e-4f) shapeMod_ = shapeTarget_;
    if (p_.chip == kChipSID) updateSidCoefs();
    applyOn();
    if (!gate_) return;
    if (p_.decay && p_.chip != kChipSID && ++envTicks_ >= p_.decay) {
      envTicks_ = 0;
      if (vol_ > 0.f) vol_ -= 1.f;
      dirty_ = true;
    }
    const float rate = p_.arpRate == 0 ? 60.f : (p_.arpRate == 5 ? 30.f : tempo_ * (1U << (p_.arpRate - 1)) / 60.f);
    arpPhase_ += rate / 240.f;
    if (arpPhase_ >= 1.f) {
      arpPhase_ -= 1.f;
      ++arpStep_;
    }
    if (arpStep_ >= detail::kArpLen[p_.arp]) arpStep_ = 0;
    if (vibDelay_ > 0) {
      --vibDelay_;
      vibValue_ = 0.f;
    } else {
      vibPhase_ += 6.f / 240.f;
      if (vibPhase_ >= 1.f) vibPhase_ -= 1.f;
      const float tri = vibPhase_ < 0.5f ? 4.f * vibPhase_ - 1.f : 3.f - 4.f * vibPhase_;
      const float soloDepth = (p_.voice == 1 || p_.voice == 3) && !fmMode() && p_.chip != kChipSID ? detuneEff() / 32.f : 0.f;
      vibValue_ = tri * (static_cast<float>(p_.vibrato) / 30.f + soloDepth);
    }
    updatePeriods();
  }

  float mixLevel() const {
    if (!gate_) return 0.f;
    int v = static_cast<int>(vol_);
    v = v < 0 ? 0 : (v > 15 ? 15 : v);
    switch (p_.chip) {
    case kChipNES:
    case kChipGB: {
      const float a = p1_.on && p1_.bit ? vol_ * gain1() : 0.f;
      const float b = p2_.on && p2_.bit ? vol_ * gain2() : 0.f;
      const float w = sub_.on ? static_cast<float>(sub_.level) * (vol_ * (1.f / 15.f)) * gainWave() : 0.f;
      const float n = (noise_.on && noise_.bit) ? vol_ * gainNoise() : 0.f;
      if (p_.chip == kChipNES) {
        const float ps = a + b;
        const float po = ps > 0.f ? 95.88f / (8128.f / ps + 100.f) : 0.f;
        const float tnd = w / 8227.f + n / 12241.f;
        const float to = tnd > 0.f ? 159.79f / (1.f / tnd + 100.f) : 0.f;
        return (po + to) * spec_.mixGain;
      }
      return (a + b + w + n) / 60.f * spec_.mixGain;
    }
    case kChipPCE: {
      float s = p1_.on ? static_cast<float>(p1_.level) * gain1() : 0.f;
      if (p2_.on && !pceLfo()) s += static_cast<float>(p2_.level) * gain2();
      if (sub_.on) s += static_cast<float>(sub_.level) * gainWave();
      if (noise_.on && noise_.bit) s += 31.f * gainNoise();
      return s * (1.f / 31.f) * amp_[v] * spec_.mixGain;
    }
    case kChipN163:
    case kChipFDS:
    case kChipPaula: {
      const float full = p_.chip == kChipFDS ? 63.f : (p_.chip == kChipPaula ? 255.f : 15.f);
      float s;
      if (paulaAm()) { // voice 2 amplitude-modulates voice 1
        s = p1_.on && p2_.on ? 1.5f * (static_cast<float>(p1_.level) / full) * (static_cast<float>(p2_.level) / full) * gain1() : 0.f;
      } else {
        s = p1_.on ? static_cast<float>(p1_.level) / full * gain1() : 0.f;
        if (p2_.on && !fmMode()) s += static_cast<float>(p2_.level) / full * gain2();
        if (sub_.on) s += static_cast<float>(sub_.level) / full * gainWave();
        if (noise_.on && noise_.bit) s += gainNoise();
      }
      return s * amp_[v] * spec_.mixGain;
    }
    case kChipVRC6: {
      const float a = p1_.on && p1_.bit ? vol_ * gain1() : 0.f;
      const float b = p2_.on && p2_.bit ? vol_ * gain2() : 0.f;
      const float w = sub_.on ? static_cast<float>(sub_.level) * (vol_ * (1.f / 15.f)) * gainWave() : 0.f;
      return (a + b + w) * (1.f / 61.f) * 0.8f;
    }
    default: { // PSG, AY, 5B, POKEY, TIA
      const float amp = amp_[v];
      const bool b1 = hpMode() ? (p1_.bit != 0) != hpLatch_ : p1_.bit != 0;
      const bool ayGate = !ayLike() || !p_.colorEnabled || !noise_.on || noise_.bit;
      float a = p1_.on && b1 && ayGate ? amp * gain1() : 0.f;
      if (env_.on) a = amp_[env_.level] * (vol_ * (1.f / 15.f)) * gain1() * (ayGate ? 1.f : 0.f);
      const float b = p2_.on && p2_.bit && ayGate ? amp * gain2() : 0.f;
      const float c = (sub_.on && sub_.bit && !hpMode() && ayGate) ? amp * gainWave() : 0.f;
      const bool separateNoise = !ayLike() || !p_.colorEnabled || p_.voice == 4;
      const float n = (separateNoise && noise_.on && noise_.bit) ? amp * gainNoise() : 0.f;
      return (a + b + c + n) * spec_.mixGain;
    }
    }
  }

  void syncLevel(float t) {
    const float lvl = mixLevel();
    if (lvl != level_) {
      blip_.addStep(t < 0.99999f ? t : 0.99999f, lvl - level_);
      level_ = lvl;
    }
  }

  // ---- SID (analog engine) -------------------------------------------------

  static float polyBlep(float t, float dt) {
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.f;
    }
    if (t > 1.f - dt) {
      t = (t - 1.f) / dt;
      return t * t + t + t + 1.f;
    }
    return 0.f;
  }

  void updateSidCoefs() {
    const float pi = 3.14159265358979f;
    sidAttackInc_ = 1.f / (0.002f * sr_);
    sidDecayCoef_ = p_.decay ? std::exp(-6.9f / (p_.decay * 0.0625f * sr_)) : 1.f;
    sidRelCoef_ = std::exp(-6.9f / (0.15f * sr_));
    float cut = p_.cutoff + 0.5f * shapeMod_;
    cut = cut < 0.f ? 0.f : (cut > 1.f ? 1.f : cut);
    sidBypass_ = cut < 0.01f;
    const float fc = 9000.f * std::pow(0.00667f, cut); // 9 kHz .. 60 Hz
    sidFTarget_ = 2.f * std::sin(pi * fc / sr_);
    sidQTarget_ = p_.resonant ? 0.3f : 1.f;
  }

  float sidVoice(SidVoice &v, float pw, uint8_t wave) {
    const float k = 1.f / 16777216.f;
    const float dt = static_cast<float>(v.inc) * k;
    v.acc = (v.acc + v.inc) & 0xFFFFFFu;
    return sidWaveAt(static_cast<float>(v.acc) * k, pw, wave, dt);
  }

  static float sidWaveAt(float p, float pw, uint8_t wave, float dt) {
    switch (wave) {
    case 0:
      return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
    case 1:
      return 2.f * p - 1.f - polyBlep(p, dt);
    default: {
      float q = p - pw;
      if (q < 0.f) q += 1.f;
      float s = (p < pw ? 1.f : -1.f) + polyBlep(p, dt) - polyBlep(q, dt);
      if (wave == 3) s *= 0.5f * ((2.f * p - 1.f - polyBlep(p, dt)) + 1.f);
      return s;
    }
    }
  }

  void addSidReset(float t, float delta) {
    const float *kernel = blip_.kernel(t);
    float step = 0.f;
    for (int i = 0; i < BlipBuf::kTaps; ++i) {
      step += kernel[i];
      // Subtract the delayed naive step to leave a finite correction impulse.
      sidSyncBuf_[(sidSyncPos_ + i + 1) & 31] += delta * (step - (i + 1 >= 8 ? 1.f : 0.f));
    }
  }

  static float sidRawWave(float phase, float pw, uint8_t wave) {
    if (wave == 0) return phase < 0.5f ? 4.f * phase - 1.f : 3.f - 4.f * phase;
    if (wave == 1) return 2.f * phase - 1.f;
    const float pulse = phase < pw ? 1.f : -1.f;
    return wave == 3 ? pulse * phase : pulse;
  }

  float sidSyncSegment(float phase, float distance, float time, float duration, float pw) {
    if (distance <= 0.f) return phase;
    const float end = phase + distance;
    const bool pulse = p_.duty >= 2;
    const float pulseJump = p_.duty == 3 ? -2.f * pw : -2.f;
    if (pulse && phase < pw && end >= pw)
      addSidReset(time + duration * (pw - phase) / distance, pulseJump * gain2());
    if (end >= 1.f) {
      const float wrapJump = p_.duty == 0 ? 0.f : (p_.duty == 1 ? -2.f : (p_.duty == 2 ? 2.f : 1.f));
      addSidReset(time + duration * (1.f - phase) / distance, wrapJump * gain2());
      if (pulse && end - 1.f >= pw)
        addSidReset(time + duration * (1.f + pw - phase) / distance, pulseJump * gain2());
    }
    return end >= 1.f ? end - 1.f : end;
  }

  float sidSyncedVoice(SidVoice &v, float masterTime, float pw) {
    const float unit = 1.f / 16777216.f;
    const float distance = v.inc * unit;
    float phase = v.acc * unit;
    if (masterTime >= 0.f) {
      phase = sidSyncSegment(phase, distance * masterTime, 0.f, masterTime, pw);
      const float jump = sidRawWave(0.f, pw, p_.duty) - sidRawWave(phase, pw, p_.duty);
      addSidReset(masterTime, jump * gain2());
      phase = sidSyncSegment(0.f, distance * (1.f - masterTime), masterTime, 1.f - masterTime, pw);
    } else {
      phase = sidSyncSegment(phase, distance, 0.f, 1.f, pw);
    }
    v.acc = static_cast<uint32_t>(phase * 16777216.f) & 0xFFFFFFu;
    return sidRawWave(phase, pw, p_.duty);
  }

  float sidNoiseVoice(SidVoice &v) {
    const uint32_t before = v.acc >> 20;
    v.acc = (v.acc + v.inc) & 0xFFFFFFu;
    uint32_t edges = ((v.acc >> 20) - before) & 0xFu;
    while (edges--) {
      const uint32_t fb = ((v.lfsr >> 22) ^ (v.lfsr >> 17)) & 1;
      v.lfsr = ((v.lfsr << 1) | fb) & 0x7FFFFFu;
      const uint32_t bits = ((v.lfsr >> 20) & 1) << 7 | ((v.lfsr >> 18) & 1) << 6 |
                            ((v.lfsr >> 14) & 1) << 5 | ((v.lfsr >> 11) & 1) << 4 |
                            ((v.lfsr >> 9) & 1) << 3 | ((v.lfsr >> 5) & 1) << 2 |
                            ((v.lfsr >> 2) & 1) << 1 | (v.lfsr & 1);
      v.noise = static_cast<float>(bits) * (1.f / 127.5f) - 1.f;
    }
    return v.noise;
  }

  float renderSid() {
    switch (sidState_) {
    case kAttack:
      sidEnv_ += sidAttackInc_;
      if (sidEnv_ >= 1.f) {
        sidEnv_ = 1.f;
        sidState_ = p_.decay ? kDecay : kSustain;
      }
      break;
    case kDecay:
      sidEnv_ *= sidDecayCoef_;
      if (sidEnv_ < 1e-3f) {
        sidEnv_ = 0.f;
        sidState_ = kSustain;
      }
      break;
    case kRelease:
      sidEnv_ *= sidRelCoef_;
      if (sidEnv_ < 1e-4f) {
        sidEnv_ = 0.f;
        sidState_ = kIdle;
      }
      break;
    default:
      break;
    }
    if (sidState_ == kIdle || sidEnv_ == 0.f) {
      sidLow_ = sidBand_ = 0.f;
      return 0.f;
    }

    pwmDepthCurrent_ += ((p_.articulation == 3 ? 0.35f : p_.pwmDepth) - pwmDepthCurrent_) * 0.002f;
    pwmRateCurrent_ += ((p_.articulation == 3 ? 0.25f : p_.pwmRate) - pwmRateCurrent_) * 0.002f;
    pwPhase_ += pwmRateCurrent_ / sr_;
    if (pwPhase_ >= 1.f) pwPhase_ -= 1.f;
    const float pw = 0.5f + pwmDepthCurrent_ * (pwPhase_ < 0.5f ? 4.f * pwPhase_ - 1.f : 3.f - 4.f * pwPhase_);

    const bool sync = p_.sub == 3 && gain1() > 0.f && gain2() > 0.f;
    const bool ring = p_.sub == 4 && gain1() > 0.f && gain2() > 0.f;
    if (sync) { // sweep the slave ratio so the sync timbre moves
      const float tri = pwPhase_ < 0.5f ? 4.f * pwPhase_ - 1.f : 3.f - 4.f * pwPhase_;
      const float inc = static_cast<float>(sidInc2_) * (1.5f + 0.9f * tri);
      sv_[1].inc = static_cast<uint32_t>(inc > 8388607.f ? 8388607.f : inc);
    }
    for (int s = 0; s < 2; ++s) {
      float m;
      if (ring) { // triangle of voice 1 x MSB of voice 2
        const float t = sidVoice(sv_[0], pw, 0);
        sidVoice(sv_[1], pw, 0);
        m = 1.6f * t * (sv_[1].acc < 0x800000u ? 1.f : -1.f) * gain1();
      } else {
        const uint32_t a0 = sv_[0].acc;
        m = gain1() > 0.f ? sidVoice(sv_[0], pw, p_.duty) * gain1() : 0.f;
        if (sync) {
          const float masterTime = sv_[0].acc < a0 ? 1.f - static_cast<float>(sv_[0].acc) / sv_[0].inc : -1.f;
          m += sidSyncedVoice(sv_[1], masterTime, pw) * gain2();
        } else if (gain2() > 0.f) {
          m += sidVoice(sv_[1], pw, p_.duty) * gain2();
        }
        if (p_.voice != 3 && (p_.sub == 1 || p_.sub == 2) && gainWave() > 0.f) m += sidVoice(sv_[2], pw, p_.duty) * gainWave();
      }
      if (p_.noise && gainNoise() > 0.f) m += sidNoiseVoice(sv_[3]) * gainNoise();
      sidDryHist_[sidSyncPos_] = m;
      m = sidDryHist_[(sidSyncPos_ - 8) & 31] + sidSyncBuf_[sidSyncPos_];
      sidSyncBuf_[sidSyncPos_] = 0.f;
      sidSyncPos_ = (sidSyncPos_ + 1) & 31;
      sidHistPos_ = (sidHistPos_ + 1) & 31;
      sidHist_[sidHistPos_] = m;
    }
    float mix = 0.f;
    for (int k = 0; k < 31; ++k) mix += sidFir_[k] * sidHist_[(sidHistPos_ - k) & 31];
    mix *= 0.17f * sidEnv_ * vol_ * (1.f / 15.f);

    sidF_ += (sidFTarget_ - sidF_) * 0.002f;
    sidQ_ += (sidQTarget_ - sidQ_) * 0.002f;
    sidInputGain_ += ((p_.resonant ? 0.45f : 1.f) - sidInputGain_) * 0.002f;
    sidWet_ += ((sidBypass_ ? 0.f : 1.f) - sidWet_) * 0.002f;
    float y = mix;
    {
      sidLow_ += sidF_ * sidBand_;
      const float in = sidInputGain_ * mix;
      const float high = in - sidLow_ - sidQ_ * sidBand_;
      sidBand_ += sidF_ * high;
      y += sidWet_ * (sidLow_ - mix);
    }
    return detail::softLimit(y * outGain_);
  }

  BlipBuf blip_;
  ApuParams p_;
  float outGain_ = 1.f;
  ChipSpec spec_{1789773.f, 2.f, 1.f, detail::kNesNoise, 1.4f, 2048.f};
  Chan p1_, p2_, sub_, noise_, env_;
  uint8_t gbWave_[8][32] = {};
  uint8_t pceWave_[8][32] = {};
  uint8_t n163Wave_[4][16] = {};
  uint8_t fdsWave_[4][64] = {};
  uint8_t paulaWave_[4][32] = {};
  float amp_[16] = {};
  float sr_ = 48000.f;
  float cps_ = 37.28f;
  float ctrlPeriod_ = 200.f;
  float ctrlAcc_ = 0.f;
  float freq_ = 440.f;
  float pitchEnv_ = 0.f;
  float vol_ = 15.f;
  float level_ = 0.f;
  float dc_ = 0.f;
  float vibPhase_ = 0.f;
  float vibValue_ = 0.f;
  uint8_t envTicks_ = 0;
  uint8_t arpTick_ = 0;
  uint8_t arpStep_ = 0;
  uint8_t vibDelay_ = 0;
  bool gate_ = false;
  bool dirty_ = false;
  bool hpLatch_ = false;
  float shapeMod_ = 0.f;
  float shapeTarget_ = 0.f;
  float detuneCurrent_ = 0.f;
  float tempo_ = 120.f;
  float arpPhase_ = 0.f;
  float fdsBasePitch_ = 1.f;
  int8_t fdsCounter_ = 0;
  float p1Base_ = 1.0e9f;
  uint32_t sidInc2_ = 0;

  SidVoice sv_[4];
  SidState sidState_ = kIdle;
  float sidEnv_ = 0.f;
  float sidAttackInc_ = 0.f;
  float sidDecayCoef_ = 1.f;
  float sidRelCoef_ = 0.f;
  float sidHist_[32] = {};
  float sidFir_[31] = {};
  int sidHistPos_ = 0;
  float sidF_ = 0.f;
  float sidFTarget_ = 0.f;
  float sidWet_ = 0.f;
  float sidSyncBuf_[32] = {};
  float sidDryHist_[32] = {};
  int sidSyncPos_ = 0;
  float sidQ_ = 1.f;
  float sidQTarget_ = 1.f;
  float sidInputGain_ = 1.f;
  float pwmDepthCurrent_ = 0.f;
  float pwmRateCurrent_ = 0.8f;
  float sidLow_ = 0.f;
  float sidBand_ = 0.f;
  float pwPhase_ = 0.f;
  bool sidBypass_ = true;
};

} // namespace chiptune
