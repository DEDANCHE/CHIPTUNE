#pragma once
// Chiptune APU core. Two engines share one voice front-end (note, arp, vibrato):
//   * event engine  (NES, GB, SN76489, YM2149, HuC6280, POKEY): channels run in the chip clock
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
constexpr int8_t kIntervals[kNumIntervals] = {0, 3, 4, 5, 7, 12};
constexpr int8_t kArp[6][3] = {{0, 0, 0}, {0, 4, 7}, {0, 3, 7}, {0, 12, 0}, {0, 7, 12}, {0, 5, 7}};
constexpr uint8_t kArpLen[6] = {1, 3, 3, 2, 3, 3};
// NOISE setting (off, low, mid, high, metal, buzz) -> noise table index, per chip
constexpr uint8_t kNoiseIdx[7][6] = {{0, 11, 7, 3, 4, 11},  // NES
                                     {0, 11, 7, 3, 4, 11},  // GB
                                     {0, 2, 1, 0, 0, 0},    // SN76489
                                     {0, 13, 9, 4, 1, 13},  // YM2149
                                     {0, 0, 0, 0, 0, 0},    // SID (unused)
                                     {0, 0, 1, 3, 5, 0},    // HuC6280
                                     {0, 6, 4, 2, 0, 7}};   // POKEY
// HuC6280 noise: 64 * (32 - n) chip clocks per LFSR step
constexpr float kPceNoise[16] = {2048, 1024, 512, 256, 128, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};
// POKEY noise divider: (N + 1) * 28 chip clocks (64 kHz base clock)
constexpr float kPokeyNoise[16] = {56,  112,  224,  448,  896,  1792, 3584, 5600,
                                   5600, 5600, 5600, 5600, 5600, 5600, 5600, 5600};
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
  kNumChips = 7
};

struct ApuParams {
  Chip chip = kChipNES;
  uint8_t duty = 2;      // 0..3. NES/GB: pulse duty 12.5/25/50/75 %, GB: wave table.
                         //       SID: waveform tri / saw / pulse(PWM) / saw&pulse.
                         //       PCE: wave table. POKEY: pure / 4-bit / 5-bit / 5&4-bit poly.
  uint8_t arp = 0;       // 0 off, 1 maj, 2 min, 3 oct, 4 5th+oct, 5 sus4
  uint8_t sub = 0;       // 0 off, 1 -12 st, 2 unison, 3/4 YM2149 envelope buzz (saw/tri)
  uint8_t noise = 0;     // 0 off, 1 low, 2 mid, 3 high, 4 metal, 5 buzz
  uint8_t decay = 0;     // 0 hold, 1..15 decay speed
  uint8_t interval = 0;  // voice 2 interval index, see detail::kIntervals
  uint8_t vibrato = 0;   // 0..15
  bool quantize = true;  // round to the chip's period registers
  float detune = 0.f;    // voice 2 offset in register steps (not SID)
  float cutoff = 0.f;    // SID only: 0 = filter open .. 1 = closed
  bool resonant = false; // SID only: high filter resonance
};

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
    for (int i = 0; i < 4; ++i) sv_[i] = SidVoice();
    sidState_ = kIdle;
    sidEnv_ = sidLow_ = sidBand_ = pwPhase_ = 0.f;
    blip_.init();
    buildWaves();
    ctrlPeriod_ = sr_ / 240.f;
    setChip(p_.chip);
    updateSidCoefs();
    dirty_ = false;
  }

  void setParams(const ApuParams &p) {
    const bool chipChanged = p.chip != p_.chip;
    p_ = p;
    if (chipChanged) setChip(p.chip);
    applyOn();
    updateSidCoefs();
    dirty_ = true;
  }

  void setFrequency(float hz) { freq_ = hz; }

  void noteOn(uint8_t velocity) {
    const bool wasIdle = sidState_ == kIdle;
    gate_ = true;
    vol_ = static_cast<float>(1 + (velocity * 14) / 127);
    envTicks_ = 0;
    arpTick_ = 0;
    arpStep_ = 0;
    vibPhase_ = 0.f;
    vibValue_ = 0.f;
    vibDelay_ = 48;
    p1_ = p2_ = sub_ = noise_ = env_ = Chan();
    noise_.lfsr = p_.chip == kChipPSG ? 0x8000u : (p_.chip == kChipPOKEY ? 0x1FFFFu : 1u);
    env_.level = 15;
    applyOn();
    updatePeriods();
    p1_.timer = p1_.period;
    p2_.timer = p2_.period;
    sub_.timer = sub_.period;
    noise_.timer = noise_.period;
    env_.timer = env_.period;
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
    const bool pokey = p_.chip == kChipPOKEY;
    const Kind k12 = pce ? Kind::kWave : (pokey ? Kind::kPokey : Kind::kPulse);
    const Kind ks = pokey ? Kind::kPokey : (sq ? Kind::kPulse : Kind::kWave);
    const Kind kn = pokey ? Kind::kPokey : Kind::kNoise;
    const uint8_t d1 = sq ? 2 : p_.duty;
    const uint8_t d2 = sq ? 2 : static_cast<uint8_t>((p_.duty + 1) & 3);
    const uint8_t ds = pce ? p_.duty : (pokey ? 0 : 2);
    const uint8_t dn = pokey ? 4 : 0;
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
      if (p2_.on) fired |= tick(p2_, dt, k12, d2);
      if (sub_.on) fired |= tick(sub_, dt, ks, ds);
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
    return y > 1.f ? 1.f : (y < -1.f ? -1.f : y);
  }

private:
  enum class Kind { kPulse, kWave, kNoise, kEnv, kPokey };
  enum SidState : uint8_t { kIdle, kAttack, kDecay, kSustain, kRelease };

  struct Chan {
    float timer = 0.f;
    float period = 1.0e9f;
    uint32_t lfsr = 1;
    uint8_t seq = 0;
    uint8_t bit = 0;
    uint8_t level = 0; // wave / envelope output, 0..15 (0..31 on HuC6280)
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

  bool squareChip() const { return p_.chip == kChipPSG || p_.chip == kChipAY; }
  bool buzz() const { return p_.chip == kChipAY && p_.sub >= 3; }

  void applyOn() {
    const bool g = gate_;
    const bool b = buzz();
    p1_.on = g && !b;
    p2_.on = g;
    sub_.on = g && p_.sub != 0 && !b;
    noise_.on = g && p_.noise != 0;
    env_.on = g && b;
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
      c.seq = (c.seq + 1) & 31;
      if (p_.chip == kChipNES)
        c.level = c.seq < 16 ? 15 - c.seq : c.seq - 16;
      else if (p_.chip == kChipPCE)
        c.level = pceWave_[arg & 3][c.seq];
      else
        c.level = gbWave_[p_.duty][c.seq];
      break;
    case Kind::kNoise:
      stepNoise(c);
      break;
    case Kind::kPokey:
      pokeyStep(c, arg);
      break;
    case Kind::kEnv: {
      const bool tri = p_.sub == 4;
      c.seq = static_cast<uint8_t>((c.seq + 1) % (tri ? 32 : 16));
      c.level = tri ? (c.seq < 16 ? 15 - c.seq : c.seq - 16) : 15 - c.seq;
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
    } else if (mode == 4) {
      c.lfsr = ((c.lfsr << 1) | (((c.lfsr >> 16) ^ (c.lfsr >> 11)) & 1)) & 0x1FFFFu;
      c.bit = c.lfsr & 1;
    } else {
      c.bit ^= 1;
    }
  }

  void stepNoise(Chan &c) {
    const bool shortMode = p_.noise == 4 || p_.noise == 5;
    if (p_.chip == kChipPSG) {
      const uint32_t fb = shortMode ? (c.lfsr & 1) : ((c.lfsr ^ (c.lfsr >> 3)) & 1);
      c.lfsr = (c.lfsr >> 1) | (fb << 15);
      c.bit = c.lfsr & 1;
    } else if (p_.chip == kChipAY) {
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
    case kChipPSG: spec_ = {3579545.f, 4.f, 4.f, detail::kPsgNoise, 0.25f, 1023.f}; break;
    case kChipAY:  spec_ = {2000000.f, 2.f, 2.f, detail::kAyNoise, 0.25f, 4095.f}; break;
    case kChipPCE: spec_ = {3579545.f, 1.f, 1.f, detail::kPceNoise, 0.25f, 4095.f}; break;
    case kChipPOKEY: spec_ = {1789773.f, 1.f, 1.f, detail::kPokeyNoise, 0.25f, 65535.f}; break;
    default:       spec_ = {sr_, 1.f, 1.f, detail::kNesNoise, 1.f, 2048.f}; break;
    }
    for (int v = 0; v < 16; ++v) {
      if (v == 0)
        amp_[v] = 0.f;
      else if (c == kChipPSG)
        amp_[v] = std::pow(10.f, -(15 - v) * 0.1f); // 2 dB per step
      else if (c == kChipPOKEY)
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
      pceWave_[0][i] = q5(s);
      pceWave_[1][i] = q5(2.f * ph - 1.f);
      pceWave_[2][i] = q5(ph < 0.25f ? 1.f : -1.f);
      pceWave_[3][i] = q5(ph < 0.5f ? -1.f + 4.f * ph : 3.f - 4.f * ph);
    }
  }

  // Chip clocks per step for a note frequency. Folds the register into range
  // by octaves and rounds to the chip's period registers when quantising.
  float stepFor(float unit, float stepsPerCycle, float hz, float detuneRegs, float regMax) const {
    float reg = spec_.clk / (unit * stepsPerCycle * hz);
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

  uint32_t sidInc(float hz) const {
    float v = hz / (2.f * sr_) * 16777216.f; // voices run at 2x and are decimated
    if (v > 8388607.f) v = 8388607.f;
    return static_cast<uint32_t>(v);
  }

  void updatePeriods() {
    const float arpSemi = static_cast<float>(detail::kArp[p_.arp][arpStep_]);
    const float semi2 = static_cast<float>(detail::kIntervals[p_.interval]);
    const float f1 = freq_ * std::pow(2.f, (arpSemi + vibValue_) / 12.f);
    const float f2 = freq_ * std::pow(2.f, (arpSemi + semi2 + vibValue_) / 12.f);
    const float fs = freq_ * std::pow(2.f, (vibValue_ + (p_.sub == 1 ? -12.f : 0.f)) / 12.f);

    if (p_.chip == kChipSID) {
      sv_[0].inc = sidInc(f1);
      sv_[1].inc = sidInc(f2 * 1.00406f); // fixed +7 cent spread
      sv_[2].inc = sidInc(fs);
      sv_[3].inc = sidInc(detail::kSidNoiseHz[p_.noise]);
      return;
    }

    if (p_.chip == kChipPOKEY) {
      const float st = detail::kPokeySteps[p_.duty];
      p1_.period = pokeyPeriod(f1, 0.f, st);
      p2_.period = pokeyPeriod(f2, p_.detune, st);
      sub_.period = pokeyPeriod(fs, 0.f, 2.f); // the bass voice stays a pure tone
      noise_.period = spec_.noiseTable[detail::kNoiseIdx[p_.chip][p_.noise]];
      return;
    }

    const float rm = spec_.regMax;
    const float steps = p_.chip == kChipPCE ? 32.f : 8.f;
    p1_.period = stepFor(spec_.pulseUnit, steps, f1, 0.f, rm);
    p2_.period = stepFor(spec_.pulseUnit, steps, f2, p_.detune, rm);
    sub_.period = squareChip() ? stepFor(spec_.pulseUnit, 8.f, fs, 0.f, rm)
                               : stepFor(spec_.waveUnit, 32.f, fs, 0.f, rm);
    noise_.period = spec_.noiseTable[detail::kNoiseIdx[p_.chip][p_.noise]];
    if (p_.chip == kChipPSG && p_.noise == 5) { // pitched periodic noise
      const float per = spec_.clk / (16.f * f1);
      noise_.period = per < 4.f ? 4.f : per;
    }
    if (p_.chip == kChipAY)
      env_.period = stepFor(16.f, p_.sub == 4 ? 32.f : 16.f, f1, 0.f, 65535.f);
  }

  void controlTick() {
    if (!gate_) return;
    if (p_.decay && p_.chip != kChipSID && ++envTicks_ >= p_.decay) {
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
    int v = static_cast<int>(vol_);
    v = v < 0 ? 0 : (v > 15 ? 15 : v);
    switch (p_.chip) {
    case kChipNES:
    case kChipGB: {
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
    case kChipPCE: {
      float s = static_cast<float>(p1_.level + p2_.level);
      if (sub_.on) s += static_cast<float>(sub_.level);
      if (noise_.on && noise_.bit) s += 31.f;
      return s * (1.f / 31.f) * amp_[v] * spec_.mixGain;
    }
    default: { // PSG, AY, POKEY
      const float amp = amp_[v];
      float a = p1_.bit ? amp : 0.f;
      if (buzz()) a = amp_[env_.level] * (vol_ * (1.f / 15.f));
      const float b = p2_.bit ? amp : 0.f;
      const float c = (sub_.on && sub_.bit) ? amp : 0.f;
      const float n = (noise_.on && noise_.bit) ? amp : 0.f;
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
    sidBypass_ = p_.cutoff < 0.01f;
    const float fc = 9000.f * std::pow(0.00667f, p_.cutoff); // 9 kHz .. 60 Hz
    sidF_ = 2.f * std::sin(pi * fc / sr_);
    sidQ_ = p_.resonant ? 0.3f : 1.f;
  }

  float sidVoice(SidVoice &v, float pw) {
    const float k = 1.f / 16777216.f;
    const float dt = static_cast<float>(v.inc) * k;
    v.acc = (v.acc + v.inc) & 0xFFFFFFu;
    const float p = static_cast<float>(v.acc) * k;
    switch (p_.duty) {
    case 0:
      return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
    case 1:
      return 2.f * p - 1.f - polyBlep(p, dt);
    default: {
      float q = p - pw;
      if (q < 0.f) q += 1.f;
      float s = (p < pw ? 1.f : -1.f) + polyBlep(p, dt) - polyBlep(q, dt);
      if (p_.duty == 3) s *= 0.5f * ((2.f * p - 1.f - polyBlep(p, dt)) + 1.f);
      return s;
    }
    }
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

    pwPhase_ += 0.8f / sr_;
    if (pwPhase_ >= 1.f) pwPhase_ -= 1.f;
    const float pw = 0.5f + 0.4f * (pwPhase_ < 0.5f ? 4.f * pwPhase_ - 1.f : 3.f - 4.f * pwPhase_);

    for (int s = 0; s < 2; ++s) {
      float m = sidVoice(sv_[0], pw) + sidVoice(sv_[1], pw);
      if (p_.sub) m += sidVoice(sv_[2], pw);
      if (p_.noise) m += sidNoiseVoice(sv_[3]);
      sidHistPos_ = (sidHistPos_ + 1) & 31;
      sidHist_[sidHistPos_] = m;
    }
    float mix = 0.f;
    for (int k = 0; k < 31; ++k) mix += sidFir_[k] * sidHist_[(sidHistPos_ - k) & 31];
    mix *= 0.22f * sidEnv_ * vol_ * (1.f / 15.f);

    float y = mix;
    if (!sidBypass_) {
      sidLow_ += sidF_ * sidBand_;
      const float high = mix - sidLow_ - sidQ_ * sidBand_;
      sidBand_ += sidF_ * high;
      y = sidLow_;
    }
    return y > 1.f ? 1.f : (y < -1.f ? -1.f : y);
  }

  BlipBuf blip_;
  ApuParams p_;
  ChipSpec spec_{1789773.f, 2.f, 1.f, detail::kNesNoise, 1.4f, 2048.f};
  Chan p1_, p2_, sub_, noise_, env_;
  uint8_t gbWave_[4][32] = {};
  uint8_t pceWave_[4][32] = {};
  float amp_[16] = {};
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
  float sidQ_ = 1.f;
  float sidLow_ = 0.f;
  float sidBand_ = 0.f;
  float pwPhase_ = 0.f;
  bool sidBypass_ = true;
};

} // namespace chiptune
