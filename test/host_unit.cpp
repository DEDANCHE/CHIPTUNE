// Adapter regressions. Build with -Itest/support -Isrc (no SDK needed).
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../unit/osc.h"

static int failures = 0;
static void check(bool ok, const char *name) {
  std::printf("%s: %s\n", name, ok ? "ok" : "FAIL");
  if (!ok) ++failures;
}

static void flush(Osc &osc) {
  float sample;
  osc.process(nullptr, &sample, 1);
}

int main() {
  for (int preset = 0; preset < chiptune::kNumPresets; ++preset) {
    Osc source{};
    source.setParameter(Osc::PRESET, preset);
    flush(source);
    source.setParameter(Osc::COLOR, 10);
    source.setParameter(Osc::DETN, 333);
    source.setParameter(Osc::ARP, 0);
    flush(source);
    int32_t saved[Osc::NUM_PARAMS];
    for (int i = 0; i < Osc::NUM_PARAMS; ++i) saved[i] = source.getParameterValue(i);

    Osc restored{};
    for (int reverse = 0; reverse < 2; ++reverse) {
      // Reuse the same instance too: recall must not depend on previous values.
      for (int j = 0; j < Osc::NUM_PARAMS; ++j) {
        const int i = reverse ? Osc::NUM_PARAMS - 1 - j : j;
        restored.setParameter(i, saved[i]);
        (void)restored.getParameterValue(i); // A read must not consume the batch.
      }
      flush(restored);
      bool equal = true;
      for (int i = 0; i < Osc::NUM_PARAMS; ++i)
        equal = equal && restored.getParameterValue(i) == saved[i];
      check(equal, reverse ? "reverse-order preset recall" : "index-order preset recall");
    }
  }

  Osc edit{};
  edit.setParameter(Osc::PRESET, 1);
  flush(edit);
  edit.setParameter(Osc::ARP, 0);
  flush(edit);
  check(edit.getParameterValue(Osc::ARP) == 0, "ARP OFF after NES ARP");
  edit.setParameter(Osc::PRESET, 6);
  check(std::strcmp(edit.getParameterStrValue(Osc::SUB, 3), "SYNC") == 0,
        "pending SID preset uses SID labels");

  {
    Osc osc{};
    osc.setParameter(Osc::COLOR, 900); // COLOR arrives before CHIP on recall.
    osc.setParameter(Osc::CHIP, chiptune::kChipPCE);
    osc.setParameter(Osc::ARP, 19); // MAJ, tempo-synced sixteenth notes
    osc.setParameter(Osc::SUB, 7); // wave only
    flush(osc);
    check(osc.getParameterValue(Osc::COLOR) == 900, "COLOR survives family change");
    check(osc.getParameterValue(Osc::ARP) == 19, "tempo division round-trips");
    check(osc.getParameterValue(Osc::SUB) == 7, "wave solo round-trips");
    osc.setParameter(Osc::QUANT, 3);
    check(osc.getParameterValue(Osc::QUANT) == 3, "tuning and resonance are independent");
    osc.setTempo(120.f);
    osc.tempo4ppqnTick(0);
  }

  {
    const int legacy[] = {8, 9, 10, 11, 12};
    const int families[] = {3, 6, 5, 5, 5};
    for (int i = 0; i < 5; ++i) {
      Osc osc{};
      osc.setParameter(Osc::CHIP, legacy[i]);
      check(osc.getParameterValue(Osc::CHIP) == families[i], "legacy ID maps to its documented family");
    }
    Osc env{};
    env.setParameter(Osc::PRESET, 65);
    flush(env);
    check(env.getParameterValue(Osc::SUB) == 18, "pure YM envelope retains its voicing");
    env.setParameter(Osc::SUB, 18);
    flush(env);
    check(env.getParameterValue(Osc::SUB) == 18, "pure YM envelope recall");
  }

  {
    for (int chip = 0; chip < chiptune::kNumPrimaryChips; ++chip) {
      for (int value = 0; value < 18; ++value) {
        Osc osc{};
        osc.setParameter(Osc::CHIP, chip);
        osc.setParameter(Osc::SUB, value);
        osc.setParameter(Osc::COLOR, 1023);
        flush(osc);
        check(osc.getParameterValue(Osc::SUB) == value, "primary-family voice mode round-trip");
        check(osc.getParameterStrValue(Osc::SUB, value) != nullptr, "voice mode has a label");
      }
    }
  }

  const int presets[] = {0, 6}; // event engine and SID, including its FIR/filter history
  for (int preset : presets) {
    Osc osc{};
    osc.setParameter(Osc::PRESET, preset);
    osc.setParameter(Osc::COLOR, 10);
    osc.setPitch(220.f / 48000.f);
    osc.noteOn(60, 127);
    float samples[4800];
    osc.process(nullptr, samples, 4800);
    for (int suspend = 0; suspend < 2; ++suspend) {
      if (suspend) osc.suspend(); else osc.reset();
      osc.process(nullptr, samples, 4800);
      float peak = 0.f;
      for (float y : samples) peak = std::fmax(peak, std::fabs(y));
      check(peak == 0.f, suspend ? "suspend silences the voice" : "reset silences the voice");
      check(osc.getParameterValue(Osc::COLOR) == 10 && osc.getParameterValue(Osc::PRESET) == preset,
            "reset preserves COLOR and preset");
      osc.noteOn(60, 127);
      osc.process(nullptr, samples, 4800);
      peak = 0.f;
      for (float y : samples) peak = std::fmax(peak, std::fabs(y));
      check(peak > 0.03f, "voice restarts after reset");
    }
  }
  return failures ? 1 : 0;
}
