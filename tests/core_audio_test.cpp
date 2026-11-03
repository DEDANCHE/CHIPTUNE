// Host test: renders APU scenarios to 32-bit float mono WAV files.
//   core_audio_test <outdir>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/apu_core.h"
#include "../src/presets.h"

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
    // Enabling the AY envelope while a tone is held must retain an audible fundamental.
    Apu apu;
    apu.init(48000.f);
    ApuParams q;
    q.chip = kChipAY; q.voice = 1; q.quantize = false;
    apu.setParams(q); apu.setFrequency(220.f); apu.noteOn(127);
    for (int i = 0; i < 9600; ++i) apu.render();
    q.sub = 3;
    apu.setParams(q);
    std::vector<float> s;
    for (int i = 0; i < 48000; ++i) s.push_back(apu.render());
    const double fundamental = mag(s, 4800, 48000, 220.0);
    check(fundamental > 0.03, "held AY tone can enable its envelope", fundamental);
  }

  {
    // A triangle reference must not recall the same envelope as the saw reference.
    for (float hz : {55.f, 220.f, 880.f}) {
      ApuParams tri = preset(21), saw = preset(42);
      tri.quantize = saw.quantize = false;
      const auto a = note(tri, hz, 1.f, 1.f), b = note(saw, hz, 1.f, 1.f);
      double difference = 0;
      for (size_t i = 4800; i < a.size(); ++i) difference += std::fabs(a[i] - b[i]);
      check(difference / 43200 > 0.01, "YM triangle and saw presets differ", difference / 43200);
      check(mag(a, 4800, 48000, hz) > 0.02, "YM triangle preset keeps its fundamental", mag(a, 4800, 48000, hz));
    }
  }

  {
    // Every YM envelope shape must retain the note's fundamental, including
    // the quantized triangle (shape 6), whose cycle spans 32 envelope ticks.
    for (bool sunsoft : {false, true}) {
      for (int shape = 0; shape < 8; ++shape) {
        ApuParams q;
        q.chip = kChipAY; q.ay5b = sunsoft;
        q.sub = 3; q.voice = 0; q.pulse2Level = 0.f; q.quantize = false;
        applyColor(q, static_cast<uint16_t>(shape * 128));
        const auto s = note(q, 440.f, 1.f, 1.f);
        const double fundamental = mag(s, 4800, 48000, 440.0);
        const double octaveBelow = mag(s, 4800, 48000, 220.0);
        check(fundamental > 0.03 && fundamental > 10 * octaveBelow,
              "YM envelope shape preserves 440 Hz pitch", fundamental);
      }
    }
  }

  {
    // Musical tuning must preserve low fundamentals beyond original register limits.
    const Chip chips[] = {kChipNES, kChipGB, kChipPSG, kChipAY, kChipTIA};
    for (Chip chip : chips) {
      ApuParams q;
      q.chip = chip;
      q.voice = 1;
      q.duty = chip == kChipTIA ? 0 : 2;
      q.quantize = false;
      const auto s = note(q, 20.f, 1.f, 1.f);
      const double fundamental = mag(s, 4800, 48000, 20.0);
      check(fundamental > 0.03, "musical mode preserves 20 Hz fundamental", fundamental);
    }
  }

  {
    // Muted channels must contribute nothing, even when their old state is nonzero.
    ApuParams a, b;
    a.voice = b.voice = 1;
    b.pulse2Level = 0.f;
    b.waveLevel = b.noiseLevel = 0.f;
    const auto sa = note(a, 220.f, 0.5f, 0.5f), sb = note(b, 220.f, 0.5f, 0.5f);
    double difference = 0;
    for (size_t i = 0; i < sa.size(); ++i) difference += std::fabs(sa[i] - sb[i]);
    check(difference == 0, "P1 solo excludes all other channels", difference);

    ApuParams wave;
    wave.chip = kChipGB;
    wave.voice = 3;
    wave.sub = 2;
    const auto full = note(wave, 220.f, 0.5f, 0.5f);
    wave.waveLevel = 0.f;
    const auto muted = note(wave, 220.f, 0.5f, 0.5f);
    check(peakAbs(full, 4800, 24000) > 0.03 && peakAbs(muted, 0, 24000) == 0.f,
          "GB wave solo and independent level", peakAbs(muted, 0, 24000));
  }

  {
    // FM's zero-depth path must equal the carrier-only wave, not a weak FM tone.
    ApuParams a, b;
    a.chip = b.chip = kChipPCE;
    a.voice = b.voice = 1;
    a.quantize = b.quantize = false;
    b.sub = 3;
    const auto sa = note(a, 220.f, 0.5f, 0.5f), sb = note(b, 220.f, 0.5f, 0.5f);
    double difference = 0;
    for (size_t i = 0; i < sa.size(); ++i) difference = std::fmax(difference, std::fabs(sa[i] - sb[i]));
    check(difference < 1e-6, "PCE zero FM depth is a pure carrier", difference);
  }

  {
    // Tempo divisions must respond to tempo; legacy tracker speed must not.
    for (int synced = 0; synced < 2; ++synced) {
      Apu a, b;
      a.init(48000.f); b.init(48000.f);
      ApuParams q;
      q.arp = 1; q.arpRate = synced ? 3 : 0; q.quantize = false;
      a.setParams(q); b.setParams(q);
      a.setTempo(60.f); b.setTempo(120.f);
      a.noteOn(127); b.noteOn(127);
      double difference = 0;
      for (int i = 0; i < 24000; ++i) difference += std::fabs(a.render() - b.render());
      check(synced ? difference > 1 : difference == 0,
            synced ? "tempo-synced arp follows BPM" : "tracker arp ignores BPM", difference);
    }
  }

  {
    // Changing FDS depth must not strand the carrier on its stopped-period sentinel.
    Apu apu;
    apu.init(48000.f);
    ApuParams q;
    q.chip = kChipFDS; q.duty = 3; q.detune = 16.f; q.interval = 4;
    apu.setParams(q); apu.noteOn(127);
    bool finite = true;
    float finalPeak = 0.f;
    for (int block = 0; block < 4; ++block) {
      q.detune = block == 1 ? 0.f : 16.f;
      apu.setParams(q);
      for (int i = 0; i < 12000; ++i) {
        const float y = apu.render();
        finite = finite && std::isfinite(y) && std::fabs(y) <= 1.f;
        if (block == 3 && i >= 6000) finalPeak = std::fmax(finalPeak, std::fabs(y));
      }
    }
    check(finite && finalPeak > 0.03f, "FDS modulation stops and resumes safely", finalPeak);
  }

  {
    // Exercise sync's wrap/reset/pulse-edge correction for every waveform at high pitches.
    bool finite = true;
    for (int wave = 0; wave < 4; ++wave) {
      for (float hz : {3150.f, 12000.f}) {
        ApuParams q;
        q.chip = kChipSID; q.duty = static_cast<uint8_t>(wave); q.sub = 3;
        const auto s = note(q, hz, 0.1f, 0.1f);
        for (float y : s) finite = finite && std::isfinite(y) && std::fabs(y) <= 1.f;
      }
    }
    check(finite, "SID sync correction remains finite at high pitches", finite);
  }

  {
    // The NES triangle / GB wave layer must follow velocity and decay too.
    const Chip chips[] = {kChipNES, kChipGB};
    for (Chip chip : chips) {
      ApuParams q;
      q.chip = chip;
      q.sub = 1;
      q.decay = 1;
      const auto decayed = note(q, 220.f, 1.f, 1.f);
      check(peakAbs(decayed, 24000, 48000) < 1e-3,
            chip == kChipNES ? "NES sub follows decay" : "GB wave follows decay",
            peakAbs(decayed, 24000, 48000));
      q.decay = 0;
      Apu loud, quiet;
      loud.init(48000.f);
      quiet.init(48000.f);
      loud.setParams(q);
      quiet.setParams(q);
      loud.setFrequency(220.f);
      quiet.setFrequency(220.f);
      loud.noteOn(127);
      quiet.noteOn(1);
      double loudEnergy = 0, quietEnergy = 0;
      for (int i = 0; i < 24000; ++i) {
        const float a = loud.render(), b = quiet.render();
        if (i >= 4800) { loudEnergy += a * a; quietEnergy += b * b; }
      }
      check(quietEnergy < loudEnergy * 0.05,
            chip == kChipNES ? "NES sub follows velocity" : "GB wave follows velocity",
            quietEnergy / loudEnergy);
    }
  }

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
    // Voice 2 must use the same distortion mode as voice 1, or it plays out of tune.
    q.noise = 0;
    q.duty = 2;
    const auto p5 = note(q, 220.f, 1.f, 1.f);
    const double f5 = mag(p5, 4800, 48000, 220.0), o5 = mag(p5, 4800, 48000, 233.0);
    check(f5 > 10 * o5, "POKEY 5-bit poly pitch (fund / 233 Hz)", f5 / o5);
  }
  {
    // Konami VRC6: pulse pitch, alias, saw layer.
    ApuParams q;
    q.chip = kChipVRC6;
    q.quantize = false;
    q.duty = 2;
    const auto vr = note(q, 440.f, 1.f, 1.f);
    const double vf = mag(vr, 4800, 48000, 440.0), vo = mag(vr, 4800, 48000, 466.0);
    check(vf > 10 * vo, "VRC6 440 Hz pitch (fund / 466 Hz)", vf / vo);
    const auto vrHi = note(q, 3150.f, 1.f, 1.f);
    const double va = 20 * std::log10(mag(vrHi, 4800, 48000, 19650.0) / mag(vrHi, 4800, 48000, 3150.0));
    check(va < -40.0, "VRC6 alias @19650 Hz rel. fund (dB)", va);
    q.sub = 1;
    const auto vs = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(vs, 2000, 12000) > 0.05, "VRC6 sounding with saw", peakAbs(vs, 2000, 12000));
    check(peakAbs(vs, 14000, 48000) < 1e-3, "VRC6 silent after note-off", peakAbs(vs, 14000, 48000));
  }
  {
    // Sunsoft 5B: AY clone at the NES clock.
    ApuParams q;
    q.chip = kChipS5B;
    q.quantize = false;
    const auto sb = note(q, 440.f, 1.f, 1.f);
    const double sf = mag(sb, 4800, 48000, 440.0), so = mag(sb, 4800, 48000, 466.0);
    check(sf > 10 * so, "5B 440 Hz pitch (fund / 466 Hz)", sf / so);
    q.sub = 3;
    const auto bz = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(bz, 2000, 12000) > 0.05, "5B buzz sounding", peakAbs(bz, 2000, 12000));
    check(peakAbs(bz, 14000, 48000) < 1e-3, "5B silent after note-off", peakAbs(bz, 14000, 48000));
  }
  {
    // Atari 2600 TIA: 5-bit divider, tested above 1 kHz where registers do not fold.
    ApuParams q;
    q.chip = kChipTIA;
    q.quantize = false;
    q.duty = 0;
    const auto ti = note(q, 1000.f, 1.f, 1.f);
    const double tf = mag(ti, 4800, 48000, 1000.0);
    check(tf > 0.05, "TIA 1 kHz fundamental", tf);
    q.duty = 1;
    q.noise = 2;
    const auto tn = note(q, 220.f, 1.f, 0.25f);
    check(peakAbs(tn, 2000, 12000) > 0.05, "TIA sounding", peakAbs(tn, 2000, 12000));
    check(peakAbs(tn, 14000, 48000) < 1e-3, "TIA silent after note-off", peakAbs(tn, 14000, 48000));
  }
  {
    // New voicings: each must sound, differ from its base voicing, and release to silence.
    struct Case { const char *name; ApuParams base, mod; } cases[3];
    cases[0].name = "SID hard sync";
    cases[0].base.chip = cases[0].mod.chip = kChipSID;
    cases[0].base.duty = cases[0].mod.duty = 1;
    cases[0].mod.sub = 3;
    cases[1].name = "SID ring mod";
    cases[1].base.chip = cases[1].mod.chip = kChipSID;
    cases[1].base.duty = cases[1].mod.duty = 0;
    cases[1].mod.sub = 4;
    cases[1].mod.interval = 4;
    cases[2].name = "PCE LFO FM";
    cases[2].base.chip = cases[2].mod.chip = kChipPCE;
    cases[2].mod.sub = 3;
    cases[2].mod.interval = 5;
    cases[2].mod.detune = 8.f;
    // The POKEY high-pass needs its own base: the plain voice with the same duty.
    for (const Case &c : cases) {
      const auto a = note(c.base, 220.f, 1.5f, 0.5f);
      const auto b = note(c.mod, 220.f, 1.5f, 0.5f);
      double diff = 0;
      for (size_t i = 4800; i < 24000; ++i) diff += std::fabs(a[i] - b[i]);
      char buf[96];
      std::snprintf(buf, sizeof buf, "%s sounds", c.name);
      check(peakAbs(b, 4800, 24000) > 0.05, buf, peakAbs(b, 4800, 24000));
      std::snprintf(buf, sizeof buf, "%s differs from plain (mean |d|)", c.name);
      check(diff / 19200 > 0.01, buf, diff / 19200);
      std::snprintf(buf, sizeof buf, "%s silent after release", c.name);
      check(peakAbs(b, 60000, 72000) < 1e-3, buf, peakAbs(b, 60000, 72000));
    }
    ApuParams a, b;
    a.chip = b.chip = kChipPOKEY;
    a.duty = b.duty = 0;
    b.sub = 3;
    const auto pa = note(a, 220.f, 1.f, 1.f), pb = note(b, 220.f, 1.f, 1.f);
    double diff = 0;
    for (size_t i = 4800; i < 24000; ++i) diff += std::fabs(pa[i] - pb[i]);
    check(peakAbs(pb, 4800, 24000) > 0.02, "POKEY high-pass sounds", peakAbs(pb, 4800, 24000));
    check(diff / 19200 > 0.01, "POKEY high-pass differs from plain (mean |d|)", diff / 19200);

    // SID UNI: the third voice is detuned -7 cent, so a partial appears below the fundamental.
    ApuParams su, s0;
    su.chip = s0.chip = kChipSID;
    su.duty = s0.duty = 0;
    su.noise = s0.noise = 0;
    su.sub = 2;
    s0.sub = 0;
    const auto ua = note(su, 1760.f, 1.f, 1.f), u0 = note(s0, 1760.f, 1.f, 1.f);
    const double lo = 1760.0 * 0.99595;
    const double mu = mag(ua, 4800, 48000, lo), m0 = mag(u0, 4800, 48000, lo);
    check(mu > 4.0 * m0 && mu > 0.02, "SID UNI adds a -7 cent partial (ratio to OFF)", mu / (m0 + 1e-9));
  }
  {
    // Wavetable chips: pitch accuracy, wave selection, voicing modes, release.
    const Chip wt[3] = {kChipN163, kChipFDS, kChipPaula};
    const char *const nm[3] = {"N163", "FDS", "Paula"};
    for (int k = 0; k < 3; ++k) {
      ApuParams q;
      q.chip = wt[k];
      q.duty = 1; // saw: strong fundamental
      q.quantize = false;
      const auto s = note(q, 440.f, 1.f, 1.f);
      const double f0 = mag(s, 4800, 48000, 440.0), off = mag(s, 4800, 48000, 466.0);
      char buf[96];
      std::snprintf(buf, sizeof buf, "%s 440 Hz pitch accuracy (fund / 466 Hz)", nm[k]);
      check(f0 > 10 * off, buf, f0 / off);
      const auto hi = note(q, 3150.f, 1.f, 1.f);
      const double hf = mag(hi, 4800, 48000, 3150.0);
      std::snprintf(buf, sizeof buf, "%s 3150 Hz fundamental", nm[k]);
      check(hf > 0.02, buf, hf);

      ApuParams w = q;
      w.duty = 0;
      const auto sw = note(w, 440.f, 0.5f, 0.5f);
      double d = 0;
      for (size_t i = 4800; i < 24000; ++i) d += std::fabs(s[i] - sw[i]);
      std::snprintf(buf, sizeof buf, "%s wave select changes timbre (mean |d|)", nm[k]);
      check(d / 19200 > 0.01, buf, d / 19200);

      const auto rel = note(q, 220.f, 1.5f, 0.25f);
      std::snprintf(buf, sizeof buf, "%s silent after note-off", nm[k]);
      check(peakAbs(rel, 20000, 72000) < 1e-3, buf, peakAbs(rel, 20000, 72000));
    }
    {
      ApuParams a, b;
      a.chip = b.chip = kChipFDS;
      a.duty = b.duty = 0;
      b.detune = 8.f; // FM depth
      b.interval = 5;
      a.interval = 5;
      const auto sa = note(a, 220.f, 0.5f, 0.5f), sb = note(b, 220.f, 0.5f, 0.5f);
      double d = 0;
      for (size_t i = 4800; i < 24000; ++i) d += std::fabs(sa[i] - sb[i]);
      check(d / 19200 > 0.01, "FDS FM depth changes the sound (mean |d|)", d / 19200);
    }
    for (int sub = 3; sub <= 4; ++sub) {
      ApuParams a, b;
      a.chip = b.chip = kChipPaula;
      a.duty = b.duty = 2;
      a.sub = 2;
      b.sub = static_cast<uint8_t>(sub);
      b.interval = 4;
      b.detune = 8.f;
      const auto sa = note(a, 220.f, 1.f, 0.5f), sb = note(b, 220.f, 1.f, 0.5f);
      double d = 0;
      for (size_t i = 4800; i < 24000; ++i) d += std::fabs(sa[i] - sb[i]);
      char buf[96];
      std::snprintf(buf, sizeof buf, "Paula %s sounds", sub == 3 ? "FM" : "AM");
      check(peakAbs(sb, 4800, 24000) > 0.05, buf, peakAbs(sb, 4800, 24000));
      std::snprintf(buf, sizeof buf, "Paula %s differs from unison (mean |d|)", sub == 3 ? "FM" : "AM");
      check(d / 19200 > 0.01, buf, d / 19200);
      std::snprintf(buf, sizeof buf, "Paula %s silent after note-off", sub == 3 ? "FM" : "AM");
      check(peakAbs(sb, 36000, 48000) < 1e-3, buf, peakAbs(sb, 36000, 48000));
    }
  }
  {
    // SUB = 4 is plain unison on PCE and POKEY (3 is FM / high-pass), as the menu says.
    const Chip chips[2] = {kChipPCE, kChipPOKEY};
    for (Chip c : chips) {
      ApuParams a, b;
      a.chip = b.chip = c;
      a.sub = 2;
      b.sub = 4;
      const auto sa = note(a, 220.f, 0.5f, 0.5f), sb = note(b, 220.f, 0.5f, 0.5f);
      double d = 0;
      for (size_t i = 0; i < sa.size(); ++i) d = std::fmax(d, std::fabs(sa[i] - sb[i]));
      check(d < 1e-6, c == kChipPCE ? "PCE SUB 4 equals unison (max |d|)" : "POKEY SUB 4 equals unison (max |d|)", d);
    }
  }
  {
    // Presets: every one sounds and decays to silence after note-off.
    for (int i = 0; i < kNumPresets; ++i) {
      const auto s = note(preset(i), 220.f, 2.f, 0.5f);
      char buf[96];
      std::snprintf(buf, sizeof buf, "preset %-10s sounds", presetName(i));
      check(peakAbs(s, 2000, 24000) > 0.03, buf, peakAbs(s, 2000, 24000));
      std::snprintf(buf, sizeof buf, "preset %-10s silent after release", presetName(i));
      check(peakAbs(s, 84000, 96000) < 1e-3, buf, peakAbs(s, 84000, 96000));
      std::snprintf(buf, sizeof buf, "preset %-10s peak < 1", presetName(i));
      check(peakAbs(s, 0, s.size()) < 1.0, buf, peakAbs(s, 0, s.size()));
      writeWav(dir + "/preset_" + std::to_string(i) + ".wav", s, 48000);
    }
  }
  {
    // Robustness: every chip, extreme frequencies and out-of-range parameters.
    static Apu a;
    a.init(48000.f);
    bool ok = true;
    float worst = 0.f;
    const float freqs[] = {0.f, 1.f, 20.f, 440.f, 12000.f, 30000.f, 1e9f};
    for (int c = 0; c < 15; ++c) { // 13 and 14 are out of range on purpose
      for (int variant = 0; variant < 4; ++variant) {
        for (float hz : freqs) {
          ApuParams q;
          q.chip = static_cast<Chip>(c);
          q.duty = static_cast<uint8_t>(variant * 3);
          q.sub = static_cast<uint8_t>(variant + 1);
          q.noise = static_cast<uint8_t>(variant == 3 ? 200 : variant + 2);
          q.arp = static_cast<uint8_t>(variant + 1);
          q.interval = static_cast<uint8_t>(variant * 7);
          q.vibrato = 15;
          q.decay = static_cast<uint8_t>(variant * 5);
          q.cutoff = variant * 0.4f;
          q.resonant = variant & 1;
          q.quantize = variant != 1;
          q.detune = variant * 5.f;
          if (variant) applyColor(q, static_cast<uint16_t>(variant * 341));
          q.voice = static_cast<uint8_t>(variant);
          a.setParams(q);
          a.setShapeMod(variant == 1 ? -1.f : (variant == 2 ? 1.f : 0.f));
          a.setFrequency(hz);
          a.noteOn(127);
          for (int i = 0; i < 6000; ++i) {
            const float y = a.render();
            if (!(y >= -1.f && y <= 1.f)) ok = false;
            if (y > worst) worst = y;
          }
          a.noteOff();
          for (int i = 0; i < 2000; ++i) {
            const float y = a.render();
            if (!(y >= -1.f && y <= 1.f)) ok = false;
          }
        }
      }
    }
    check(ok, "fuzz: output finite and within [-1, 1] for all chips/params", worst);
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
