// Adapter regressions. Build with -Itests/sdk_stubs -Isrc (no SDK needed).
#include <cstdio>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include "../platform/nts1_mkii/osc.h"

static int failures = 0;
static void check(bool ok, const char *name) {
  std::printf("%s: %s\n", name, ok ? "ok" : "FAIL");
  if (!ok) ++failures;
}

static void flush(Osc &osc) {
  float sample;
  osc.process(nullptr, &sample, 1);
}

static void renderFresh(Osc &osc, float *out, int frames) {
  osc.setPitch(220.f / 48000.f);
  flush(osc);
  osc.reset();
  osc.noteOn(57, 127);
  osc.process(nullptr, out, frames);
}

static double audioDifference(Osc &a, Osc &b) {
  float x[4800], y[4800];
  renderFresh(a, x, 4800); renderFresh(b, y, 4800);
  double difference = 0;
  for (int i = 0; i < 4800; ++i) difference += std::fabs(x[i] - y[i]);
  return difference / 4800;
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
    const int savedIds[] = {8, 9, 5, 11, 12}; // WT4 is encoded by PCE + COLOR.
    for (int i = 0; i < 5; ++i) {
      Osc osc{};
      osc.setParameter(Osc::CHIP, legacy[i]);
      check(osc.getParameterValue(Osc::CHIP) == savedIds[i], "legacy engine identity survives parameter reads");
      check(osc.getParameterStrValue(Osc::CHIP, legacy[i]) != nullptr, "legacy chip has a menu label");
    }
    // A bare legacy N163 selection must choose the short four-bit bank;
    // an explicit COLOR in the same recall batch still takes precedence.
    for (int color : {0, 384, 700, 1023}) {
      Osc alias{}, explicitBank{};
      alias.setParameter(Osc::COLOR, color);
      flush(alias);
      alias.setParameter(Osc::CHIP, chiptune::kChipN163);
      const int expected = (color & 511) | 512;
      check(alias.getParameterValue(Osc::COLOR) == expected, "N163 alias selects upper COLOR bank");
      explicitBank.setParameter(Osc::CHIP, chiptune::kChipPCE);
      explicitBank.setParameter(Osc::COLOR, expected);
      alias.setPitch(220.f / 48000.f);
      explicitBank.setPitch(220.f / 48000.f);
      flush(alias); flush(explicitBank);
      alias.reset(); explicitBank.reset();
      alias.noteOn(57, 127); explicitBank.noteOn(57, 127);
      float actual[4800], reference[4800];
      alias.process(nullptr, actual, 4800);
      explicitBank.process(nullptr, reference, 4800);
      check(std::memcmp(actual, reference, sizeof actual) == 0, "N163 alias renders the four-bit bank");
    }
    for (int reverse = 0; reverse < 2; ++reverse) {
      Osc alias{};
      alias.setParameter(reverse ? Osc::COLOR : Osc::CHIP, reverse ? 100 : chiptune::kChipN163);
      alias.setParameter(reverse ? Osc::CHIP : Osc::COLOR, reverse ? chiptune::kChipN163 : 100);
      flush(alias);
      check(alias.getParameterValue(Osc::COLOR) == 100, "explicit COLOR overrides N163 alias bank in either recall order");
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

  {
    // Engine identity must survive recall both before and after replacing a
    // legacy preset's engine with its primary family. Compare sound, not labels.
    const int legacyPresets[] = {13, 14, 33, 36};
    const int parentChips[] = {3, 6, 5, 5};
    for (int k = 0; k < 4; ++k) {
      for (int replace = 0; replace < 2; ++replace) {
        Osc source{};
        source.setParameter(Osc::PRESET, legacyPresets[k]); flush(source);
        if (replace) { source.setParameter(Osc::CHIP, parentChips[k]); flush(source); }
        source.setParameter(Osc::DETN, 333); flush(source);
        for (int reverse = 0; reverse < 2; ++reverse) {
          Osc restored{};
          for (int j = 0; j < Osc::NUM_PARAMS; ++j) {
            const int i = reverse ? Osc::NUM_PARAMS - 1 - j : j;
            restored.setParameter(i, source.getParameterValue(i));
          }
          check(audioDifference(source, restored) < 1e-7, "legacy or replaced engine recalls identical audio");
        }
        if (replace) check(source.getParameterValue(Osc::CHIP) == parentChips[k], "parent chip replaces legacy engine directly");
      }
    }
  }

  {
    // All event/SID transitions must keep a held note audible, without retrigger.
    const int heldPresets[] = {0, 3, 19, 20, 6, 25, 26, 12, 65, 33, 36};
    for (int from : heldPresets) {
      for (int to : heldPresets) {
        Osc osc{};
        osc.setParameter(Osc::PRESET, from);
        float block[4800]; renderFresh(osc, block, 4800);
        osc.setParameter(Osc::PRESET, to);
        osc.process(nullptr, block, 4800);
        osc.process(nullptr, block, 4800);
        float peak = 0;
        for (float y : block) peak = std::fmax(peak, std::fabs(y));
        check(peak > 0.02f, "held note remains audible across engine changes");
        osc.noteOff(57);
        osc.setParameter(Osc::PRESET, from);
        for (int i = 0; i < 5; ++i) osc.process(nullptr, block, 4800);
        peak = 0;
        for (float y : block) peak = std::fmax(peak, std::fabs(y));
        check(peak < 1e-3f, "engine change after note-off stays silent");
      }
    }
  }

  {
    // Both callback orders must start the new pitch, not the previous 8 Hz note.
    for (int chip : {0, 4, 5}) {
      for (int pitchFirst = 0; pitchFirst < 2; ++pitchFirst) {
        Osc source{}, reference{};
        for (Osc *osc : {&source, &reference}) {
          osc->setParameter(Osc::CHIP, chip);
          osc->setParameter(Osc::QUANT, 0);
          osc->setParameter(Osc::SUB, 5);
          osc->setPitch(8.f / 48000.f); flush(*osc); osc->reset();
        }
        if (pitchFirst) source.setPitch(440.f / 48000.f);
        source.noteOn(69, 127);
        if (!pitchFirst) source.setPitch(440.f / 48000.f);
        reference.setPitch(440.f / 48000.f); flush(reference); reference.reset();
        reference.noteOn(69, 127);
        float actual[1000], expected[1000];
        source.process(nullptr, actual, 1000); reference.process(nullptr, expected, 1000);
        check(std::memcmp(actual, expected, sizeof actual) == 0, "new note attack uses current render pitch in either callback order");
        source.reset(); source.noteOn(69, 127); source.noteOff(69);
        source.process(nullptr, actual, 1000);
        float peak = 0;
        for (float y : actual) peak = std::fmax(peak, std::fabs(y));
        check(peak == 0.f, "note-off cancels an unrendered note-on");
      }
    }
  }

  {
    for (int chip : {3, 8}) {
      for (int color : {0, 128, 384, 800, 1023}) {
        Osc saw{}, tri{};
        for (Osc *osc : {&saw, &tri}) {
          osc->setParameter(Osc::CHIP, chip);
          osc->setParameter(Osc::COLOR, color);
        }
        saw.setParameter(Osc::SUB, 18); tri.setParameter(Osc::SUB, 19);
        check(audioDifference(saw, tri) > 0.01, "YM pure saw and triangle have distinct audio regardless of COLOR");
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
