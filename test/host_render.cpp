// Host test: renders APU scenarios to 32-bit float mono WAV files.
//   host_render <outdir>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/apu_core.h"

using namespace chiptune;

static void writeWav(const std::string &path, const std::vector<float> &s, int sr) {
  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f) {
    std::perror(path.c_str());
    std::exit(1);
  }
  const uint32_t dataBytes = static_cast<uint32_t>(s.size() * 4);
  const uint32_t riff = 36 + dataBytes, fmtSize = 16, byteRate = sr * 4;
  const uint16_t fmt = 3, ch = 1, align = 4, bits = 32;
  std::fwrite("RIFF", 1, 4, f);
  std::fwrite(&riff, 4, 1, f);
  std::fwrite("WAVEfmt ", 1, 8, f);
  std::fwrite(&fmtSize, 4, 1, f);
  std::fwrite(&fmt, 2, 1, f);
  std::fwrite(&ch, 2, 1, f);
  const uint32_t usr = sr;
  std::fwrite(&usr, 4, 1, f);
  std::fwrite(&byteRate, 4, 1, f);
  std::fwrite(&align, 2, 1, f);
  std::fwrite(&bits, 2, 1, f);
  std::fwrite("data", 1, 4, f);
  std::fwrite(&dataBytes, 4, 1, f);
  std::fwrite(s.data(), 4, s.size(), f);
  std::fclose(f);
}

static std::vector<float> note(const ApuParams &p, float hz, float seconds, float hold) {
  Apu apu;
  apu.init(48000.f);
  apu.setParams(p);
  apu.setFrequency(hz);
  apu.noteOn(127);
  std::vector<float> out;
  const int n = static_cast<int>(seconds * 48000), off = static_cast<int>(hold * 48000);
  for (int i = 0; i < n; ++i) {
    if (i == off) apu.noteOff();
    out.push_back(apu.render());
  }
  return out;
}

static int g_failures = 0;

static void check(bool ok, const char *what, double value) {
  std::printf("%-44s %10.3f  %s\n", what, value, ok ? "ok" : "FAIL");
  if (!ok) ++g_failures;
}

// Goertzel magnitude of `hz` over s[from, to).
static double mag(const std::vector<float> &s, size_t from, size_t to, double hz) {
  const double w = 2.0 * 3.14159265358979 * hz / 48000.0, c = 2.0 * std::cos(w);
  double a = 0, b = 0;
  for (size_t i = from; i < to; ++i) {
    const double n = s[i] + c * a - b;
    b = a;
    a = n;
  }
  return std::sqrt(a * a + b * b - c * a * b) * 2.0 / (to - from);
}

static double peakAbs(const std::vector<float> &s, size_t from, size_t to) {
  double m = 0;
  for (size_t i = from; i < to; ++i) m = std::fmax(m, std::fabs(s[i]));
  return m;
}

int main(int argc, char **argv) {
  const std::string dir = argc > 1 ? argv[1] : ".";
  ApuParams p;

  {
    // 50% pulse at 3150 Hz: 9th harmonic (28350 Hz) folds to 19650 Hz.
    // A naive pulse puts it near -19 dB re fundamental.
    ApuParams q;
    q.quantize = false;
    const auto hi = note(q, 3150.f, 1.f, 1.f);
    const double fund = mag(hi, 4800, 48000, 3150.0);
    const double alias = mag(hi, 4800, 48000, 19650.0);
    check(fund > 0.05, "3150 Hz fundamental magnitude", fund);
    check(20 * std::log10(alias / fund) < -40.0, "alias @19650 Hz rel. fundamental (dB)",
          20 * std::log10(alias / fund));

    const auto lo = note(q, 440.f, 1.f, 1.f);
    const double f440 = mag(lo, 4800, 48000, 440.0), off = mag(lo, 4800, 48000, 466.0);
    check(f440 > 10 * off, "440 Hz pitch accuracy (fund / 466 Hz)", f440 / off);

    ApuParams r;
    r.noise = 1;
    const auto rel = note(r, 440.f, 1.f, 0.25f);
    check(peakAbs(rel, 0, 12000) > 0.05, "sounding before note-off", peakAbs(rel, 0, 12000));
    check(peakAbs(rel, 14000, 48000) < 1e-3, "silent after note-off", peakAbs(rel, 14000, 48000));
  }

  {
    // Sega SN76489: square at 440 Hz, 3-register-step quantisation off.
    ApuParams q;
    q.chip = kChipPSG;
    q.quantize = false;
    const auto psg = note(q, 440.f, 1.f, 1.f);
    const double pf = mag(psg, 4800, 48000, 440.0), po = mag(psg, 4800, 48000, 466.0);
    check(pf > 10 * po, "PSG 440 Hz pitch (fund / 466 Hz)", pf / po);
    const auto psgHi = note(q, 3150.f, 1.f, 1.f);
    const double pa = 20 * std::log10(mag(psgHi, 4800, 48000, 19650.0) / mag(psgHi, 4800, 48000, 3150.0));
    check(pa < -40.0, "PSG alias @19650 Hz rel. fund (dB)", pa);
    q.noise = 5; // pitched periodic noise tracks the note
    const auto buzzN = note(q, 220.f, 1.f, 1.f);
    check(peakAbs(buzzN, 4800, 48000) > 0.05, "PSG periodic noise sounds", peakAbs(buzzN, 4800, 48000));
  }
  {
    // Atari ST YM2149: tone pitch, and envelope buzzer tuned to the note.
    ApuParams q;
    q.chip = kChipAY;
    q.quantize = false;
    const auto ay = note(q, 440.f, 1.f, 1.f);
    const double af = mag(ay, 4800, 48000, 440.0), ao = mag(ay, 4800, 48000, 466.0);
    check(af > 10 * ao, "YM2149 440 Hz pitch (fund / 466 Hz)", af / ao);
    q.sub = 3;
    const auto bz = note(q, 220.f, 1.f, 1.f);
    const double bf = mag(bz, 4800, 48000, 220.0), bo = mag(bz, 4800, 48000, 233.0);
    check(bf > 10 * bo && bf > 0.02, "YM2149 envelope buzz pitch", bf / bo);
    const auto rel = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(rel, 14000, 48000) < 1e-3, "YM2149 silent after note-off", peakAbs(rel, 14000, 48000));
  }
  {
    // C64 SID: polyBLEP saw alias level, pitch, filter, release tail.
    ApuParams q;
    q.chip = kChipSID;
    q.duty = 1;
    const auto saw = note(q, 440.f, 1.f, 1.f);
    const double sf = mag(saw, 4800, 48000, 440.0), so = mag(saw, 4800, 48000, 466.0);
    check(sf > 10 * so, "SID saw 440 Hz pitch (fund / 466 Hz)", sf / so);
    const auto hi = note(q, 3150.f, 1.f, 1.f);
    const double sa = 20 * std::log10(mag(hi, 4800, 48000, 19650.0) / mag(hi, 4800, 48000, 3150.0));
    check(sa < -60.0, "SID saw alias @19650 Hz rel. fund (dB)", sa);
    q.cutoff = 0.6f;
    const auto lp = note(q, 440.f, 1.f, 1.f);
    const double hOpen = mag(saw, 4800, 48000, 6600.0), hLp = mag(lp, 4800, 48000, 6600.0);
    check(hLp < 0.3 * hOpen, "SID low-pass cuts 6.6 kHz (ratio)", hLp / hOpen);
    q.cutoff = 0.f;
    q.duty = 2;
    q.noise = 2;
    q.sub = 1;
    q.decay = 6;
    const auto full = note(q, 220.f, 1.5f, 0.25f);
    check(peakAbs(full, 2000, 12000) > 0.05, "SID sounding", peakAbs(full, 2000, 12000));
    check(peakAbs(full, 40000, 72000) < 1e-3, "SID silent after release", peakAbs(full, 40000, 72000));
  }
  {
    // PC Engine HuC6280: 32-sample wave channels.
    ApuParams q;
    q.chip = kChipPCE;
    q.quantize = false;
    q.duty = 1;
    const auto pc = note(q, 440.f, 1.f, 1.f);
    const double cf = mag(pc, 4800, 48000, 440.0), co = mag(pc, 4800, 48000, 466.0);
    check(cf > 10 * co, "PCE 440 Hz pitch (fund / 466 Hz)", cf / co);
    const auto pcHi = note(q, 3150.f, 1.f, 1.f);
    const double ca = 20 * std::log10(mag(pcHi, 4800, 48000, 19650.0) / mag(pcHi, 4800, 48000, 3150.0));
    check(ca < -40.0, "PCE alias @19650 Hz rel. fund (dB)", ca);
    q.noise = 3;
    q.sub = 1;
    const auto pn = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(pn, 2000, 12000) > 0.05, "PCE sounding with noise", peakAbs(pn, 2000, 12000));
    check(peakAbs(pn, 14000, 48000) < 1e-3, "PCE silent after note-off", peakAbs(pn, 14000, 48000));
  }
  {
    // Atari POKEY: pure tone, 4-bit poly buzz tuned to the note, 17-bit noise.
    ApuParams q;
    q.chip = kChipPOKEY;
    q.quantize = false;
    q.duty = 0;
    const auto pk = note(q, 440.f, 1.f, 1.f);
    const double kf = mag(pk, 4800, 48000, 440.0), ko = mag(pk, 4800, 48000, 466.0);
    check(kf > 10 * ko, "POKEY 440 Hz pitch (fund / 466 Hz)", kf / ko);
    const auto pkHi = note(q, 3150.f, 1.f, 1.f);
    const double ka = 20 * std::log10(mag(pkHi, 4800, 48000, 19650.0) / mag(pkHi, 4800, 48000, 3150.0));
    check(ka < -40.0, "POKEY alias @19650 Hz rel. fund (dB)", ka);
    const auto low = note(q, 82.f, 1.f, 1.f); // needs the 16-bit divider pair
    const double lf = mag(low, 4800, 48000, 82.0), lo = mag(low, 4800, 48000, 90.0);
    check(lf > 10 * lo, "POKEY 82 Hz pitch via 16-bit pair (fund / 90 Hz)", lf / lo);
    q.duty = 1;
    const auto p4 = note(q, 220.f, 1.f, 1.f);
    const double bf = mag(p4, 4800, 48000, 220.0), bo = mag(p4, 4800, 48000, 233.0);
    check(bf > 10 * bo, "POKEY 4-bit poly pitch (fund / 233 Hz)", bf / bo);
    q.noise = 2;
    const auto pn = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(pn, 2000, 12000) > 0.05, "POKEY sounding with noise", peakAbs(pn, 2000, 12000));
    check(peakAbs(pn, 14000, 48000) < 1e-3, "POKEY silent after note-off", peakAbs(pn, 14000, 48000));
  }

  p.duty = 2;
  p.quantize = false;
  writeWav(dir + "/nes_pulse_440.wav", note(p, 440.f, 1.f, 1.f), 48000);
  writeWav(dir + "/nes_pulse_3150.wav", note(p, 3150.f, 1.f, 1.f), 48000);

  p = ApuParams();
  p.arp = 1;
  p.sub = 1;
  p.interval = 5;
  p.detune = 2.f;
  p.noise = 3;
  p.decay = 4;
  p.vibrato = 6;
  writeWav(dir + "/nes_full.wav", note(p, 220.f, 1.5f, 1.f), 48000);

  p = ApuParams();
  p.chip = kChipGB;
  p.sub = 2;
  p.noise = 2;
  p.decay = 8;
  writeWav(dir + "/gb_full.wav", note(p, 330.f, 1.5f, 1.f), 48000);

  p = ApuParams();
  p.chip = kChipPSG;
  p.sub = 2;
  p.noise = 5;
  p.decay = 5;
  writeWav(dir + "/psg_full.wav", note(p, 220.f, 1.5f, 1.f), 48000);

  p = ApuParams();
  p.chip = kChipAY;
  p.sub = 3;
  p.noise = 3;
  p.decay = 8;
  writeWav(dir + "/ym2149_buzz.wav", note(p, 110.f, 1.5f, 1.f), 48000);

  p = ApuParams();
  p.chip = kChipSID;
  p.duty = 2;
  p.sub = 1;
  p.cutoff = 0.5f;
  p.resonant = true;
  p.arp = 1;
  writeWav(dir + "/sid_pwm.wav", note(p, 110.f, 1.5f, 1.2f), 48000);

  p = ApuParams();
  p.chip = kChipPCE;
  p.duty = 1;
  p.detune = 3.f;
  p.noise = 2;
  p.decay = 6;
  writeWav(dir + "/pce_full.wav", note(p, 165.f, 1.5f, 1.f), 48000);

  p = ApuParams();
  p.chip = kChipPOKEY;
  p.duty = 1;
  p.sub = 1;
  p.arp = 1;
  p.decay = 5;
  writeWav(dir + "/pokey_buzz.wav", note(p, 110.f, 1.5f, 1.f), 48000);

  // Silence check: gate off must return to zero and stay there.
  p = ApuParams();
  p.noise = 1;
  writeWav(dir + "/nes_release.wav", note(p, 440.f, 1.f, 0.25f), 48000);
  return g_failures ? 1 : 0;
}
