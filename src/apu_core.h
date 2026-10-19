#pragma once
// Event-driven chiptune APU core: chip-clock-domain channels rendered through a
// band-limited step buffer (blip-style). Header-only, no heap, no libc state.
// Portable: builds on the host for tests and on Cortex-M7 for the NTS-1 mkII.

#include <cmath>
#include <cstdint>

namespace chiptune {

namespace detail {
constexpr float kNesNoise[16] = {4,   8,   16,  32,  64,  96,   128,  160,
                                 202, 254, 380, 508, 762, 1016, 2034, 4068};
constexpr float kGbNoise[16] = {8,    16,   32,   64,    128,   256,   512,    1024,
                                2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144};
constexpr uint8_t kDuty[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 1, 1, 0, 0, 0},
                                 {1, 0, 0, 1, 1, 1, 1, 1}};
constexpr int kNumIntervals = 6;
constexpr int8_t kIntervals[kNumIntervals] = {0, 3, 4, 5, 7, 12};
constexpr int8_t kArp[6][3] = {{0, 0, 0}, {0, 4, 7}, {0, 3, 7}, {0, 12, 0}, {0, 7, 12}, {0, 5, 7}};
constexpr uint8_t kArpLen[6] = {1, 3, 3, 2, 3, 3};
// noise period index per NOISE setting (off, low, mid, high, metal, buzz)
constexpr uint8_t kNoiseIdx[6] = {0, 11, 7, 3, 4, 11};
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

private:
  float kernel_[kPhases][kTaps];
  float buf_[kSize];
  int pos_ = 0;
  float acc_ = 0.f;
};

enum Chip : uint8_t { kChipNES = 0, kChipGB = 1, kNumChips = 2 };

struct ApuParams {
  Chip chip = kChipNES;
  uint8_t duty = 2;      // 0..3: 12.5 / 25 / 50 / 75 %  (GB: also picks wave table)
  uint8_t arp = 0;       // 0 off, 1 maj, 2 min, 3 oct, 4 5th+oct, 5 sus4
  uint8_t sub = 0;       // 0 off, 1 -12 st, 2 unison (triangle / wave channel)
  uint8_t noise = 0;     // 0 off, 1 low, 2 mid, 3 high, 4 metal, 5 buzz
  uint8_t decay = 0;     // 0 hold, 1..15 envelope step = decay/240 s
  uint8_t interval = 0;  // pulse 2 interval index, see detail::kIntervals
  uint8_t vibrato = 0;   // 0..15
  bool quantize = true;  // round to the chip's period registers
  float detune = 0.f;    // pulse 2 offset in register steps
};

class Apu {
public:
  // Explicitly resets all state; safe even if static constructors never ran.
  void init(float sampleRate) {
    sr_ = sampleRate;
    p_ = ApuParams();
    p1_ = p2_ = sub_ = noise_ = Chan();
    freq_ = 440.f;
    vol_ = 15.f;
    level_ = dc_ = 0.f;
    vibPhase_ = vibValue_ = 0.f;
    envTicks_ = arpTick_ = arpStep_ = vibDelay_ = 0;
    gate_ = false;
    blip_.init();
    buildWaves();
    ctrlPeriod_ = sr_ / 240.f;
    setChip(p_.chip);
    dirty_ = false;
  }

  void setParams(const ApuParams &p) {
    const bool chipChanged = p.chip != p_.chip;
    p_ = p;
    if (chipChanged) setChip(p.chip);
    sub_.on = p_.sub != 0 && gate_;
    noise_.on = p_.noise != 0 && gate_;
    dirty_ = true;
  }

  void setFrequency(float hz) { freq_ = hz; }

  void noteOn(uint8_t velocity) {
    gate_ = true;
    vol_ = static_cast<float>(1 + (velocity * 14) / 127);
    envTicks_ = 0;
    arpTick_ = 0;
    arpStep_ = 0;
    vibPhase_ = 0.f;
    vibValue_ = 0.f;
    vibDelay_ = 48;
    p1_ = Chan();
    p2_ = Chan();
    sub_ = Chan();
    noise_ = Chan();
    p1_.on = p2_.on = true;
    sub_.on = p_.sub != 0;
    noise_.on = p_.noise != 0;
    updatePeriods();
    p1_.timer = p1_.period;
    p2_.timer = p2_.period;
    sub_.timer = sub_.period;
    noise_.timer = noise_.period;
    dirty_ = true;
  }

  void noteOff() {
    gate_ = false;
    p1_.on = p2_.on = sub_.on = noise_.on = false;
    dirty_ = true;
  }

  float render() {
    if (++ctrlAcc_ >= ctrlPeriod_) {
      ctrlAcc_ -= ctrlPeriod_;
      controlTick();
    }
    if (dirty_) {
      syncLevel(0.f);
      dirty_ = false;
    }

    float remaining = cps_;
    for (;;) {
      float dt = remaining;
      if (p1_.on && p1_.timer < dt) dt = p1_.timer;
      if (p2_.on && p2_.timer < dt) dt = p2_.timer;
      if (sub_.on && sub_.timer < dt) dt = sub_.timer;
      if (noise_.on && noise_.timer < dt) dt = noise_.timer;
      remaining -= dt;

      bool fired = false;
      if (p1_.on) fired |= tick(p1_, dt, Kind::kPulse, p_.duty);
      if (p2_.on) fired |= tick(p2_, dt, Kind::kPulse, (p_.duty + 1) & 3);
      if (sub_.on) fired |= tick(sub_, dt, Kind::kWave, 0);
      if (noise_.on) fired |= tick(noise_, dt, Kind::kNoise, 0);
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
    return y > 1.f ? 1.f : (y < -1.f ? -1.f : y);
  }

private:
  enum class Kind { kPulse, kWave, kNoise };

  struct Chan {
    float timer = 0.f;
    float period = 1.0e9f;
    uint8_t seq = 0;
    uint8_t bit = 0;
    uint8_t level = 0; // wave channel output, 0..15
    uint16_t lfsr = 1;
    bool on = false;
  };

  struct ChipSpec {
    float clk;
    float pulseUnit; // chip clocks per register unit, pulse
    float waveUnit;  // chip clocks per register unit, triangle / wave
    const float *noiseTable;
    float mixGain;
  };

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
      c.seq = (c.seq + 1) & 31;
      if (p_.chip == kChipNES)
        c.level = c.seq < 16 ? 15 - c.seq : c.seq - 16;
      else
        c.level = gbWave_[p_.duty][c.seq];
      break;
    case Kind::kNoise: {
      const bool shortMode = p_.noise == 4 || p_.noise == 5;
      const uint16_t fb = (c.lfsr ^ (c.lfsr >> (shortMode ? 6 : 1))) & 1;
      c.lfsr = static_cast<uint16_t>((c.lfsr >> 1) | (fb << 14));
      c.bit = (c.lfsr & 1) ? 0 : 1;
      break;
    }
    }
    return true;
  }

  void setChip(Chip c) {
    p_.chip = c;
    spec_ = c == kChipNES ? ChipSpec{1789773.f, 2.f, 1.f, detail::kNesNoise, 1.4f}
                          : ChipSpec{4194304.f, 4.f, 2.f, detail::kGbNoise, 0.7f};
    cps_ = spec_.clk / sr_;
    ctrlAcc_ = 0.f;
    dirty_ = true;
  }

  void buildWaves() {
    const float tau = 6.28318530718f;
    auto q = [](float v) { // -1..1 -> 0..15
      const int n = static_cast<int>((v * 0.5f + 0.5f) * 15.f + 0.5f);
      return static_cast<uint8_t>(n < 0 ? 0 : (n > 15 ? 15 : n));
    };
    for (int i = 0; i < 32; ++i) {
      const float ph = static_cast<float>(i) / 32.f;
      const float s = std::sin(tau * ph);
      gbWave_[0][i] = q(s);
      gbWave_[1][i] = q(2.f * ph - 1.f);
      gbWave_[2][i] = q(0.6f * s + 0.4f * std::sin(2.f * tau * ph));
      gbWave_[3][i] = q(ph < 0.5f ? -1.f + 4.f * ph : 3.f - 4.f * ph);
    }
  }

  // Chip clocks per step for a note frequency. Folds the register into range
  // by octaves and rounds to the chip's period registers when quantising.
  float stepFor(float unit, float stepsPerCycle, float hz, float detuneRegs) const {
    float reg = spec_.clk / (unit * stepsPerCycle * hz);
    while (reg > 2048.f) reg *= 0.5f;
    if (p_.quantize) reg = std::floor(reg + 0.5f);
    reg += detuneRegs;
    if (reg < 1.f) reg = 1.f;
    if (reg > 2048.f) reg = 2048.f;
    return reg * unit;
  }

  void updatePeriods() {
    const float arpSemi = static_cast<float>(detail::kArp[p_.arp][arpStep_]);
    const float semi2 = static_cast<float>(detail::kIntervals[p_.interval]);
    const float f1 = freq_ * std::pow(2.f, (arpSemi + vibValue_) / 12.f);
    const float f2 = freq_ * std::pow(2.f, (arpSemi + semi2 + vibValue_) / 12.f);
    const float fs = freq_ * std::pow(2.f, (vibValue_ + (p_.sub == 1 ? -12.f : 0.f)) / 12.f);
    p1_.period = stepFor(spec_.pulseUnit, 8.f, f1, 0.f);
    p2_.period = stepFor(spec_.pulseUnit, 8.f, f2, p_.detune);
    sub_.period = stepFor(spec_.waveUnit, 32.f, fs, 0.f);
    noise_.period = spec_.noiseTable[detail::kNoiseIdx[p_.noise]];
  }

  void controlTick() {
    if (!gate_) return;
    if (p_.decay && ++envTicks_ >= p_.decay) {
      envTicks_ = 0;
      if (vol_ > 0.f) vol_ -= 1.f;
      dirty_ = true;
    }
    if (++arpTick_ >= 4) { // 60 Hz tracker rate
      arpTick_ = 0;
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
      vibValue_ = tri * static_cast<float>(p_.vibrato) / 30.f;
    }
    updatePeriods();
  }

  float mixLevel() const {
    if (!gate_) return 0.f;
    const float a = p1_.bit ? vol_ : 0.f;
    const float b = p2_.bit ? vol_ : 0.f;
    const float w = sub_.on ? static_cast<float>(sub_.level) : 0.f;
    const float n = (noise_.on && noise_.bit) ? vol_ : 0.f;
    if (p_.chip == kChipNES) {
      const float ps = a + b;
      const float po = ps > 0.f ? 95.88f / (8128.f / ps + 100.f) : 0.f;
      const float tnd = w / 8227.f + n / 12241.f;
      const float to = tnd > 0.f ? 159.79f / (1.f / tnd + 100.f) : 0.f;
      return (po + to) * spec_.mixGain;
    }
    return (a + b + w + n) / 60.f * spec_.mixGain;
  }

  void syncLevel(float t) {
    const float lvl = mixLevel();
    if (lvl != level_) {
      blip_.addStep(t < 0.99999f ? t : 0.99999f, lvl - level_);
      level_ = lvl;
    }
  }

  BlipBuf blip_;
  ApuParams p_;
  ChipSpec spec_{1789773.f, 2.f, 1.f, detail::kNesNoise, 1.4f};
  Chan p1_, p2_, sub_, noise_;
  uint8_t gbWave_[4][32] = {};
  float sr_ = 48000.f;
  float cps_ = 37.28f;
  float ctrlPeriod_ = 200.f;
  float ctrlAcc_ = 0.f;
  float freq_ = 440.f;
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
};

} // namespace chiptune
