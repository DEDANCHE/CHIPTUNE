#pragma once
// Reference presets for the CHIPTUNE unit. The PRESET menu item loads one of these;
// the table also documents the parameter values (see docs/presets.md) and is
// the test material for the host renderer.
//
// Original IDs are reserved for migration. Revision-1 spectral-distance measurements
// do not describe revised voicings; host regression results are recorded in the docs.
// Hardware audition and CPU profiling remain pending.

#include "apu_core.h"

namespace chiptune {

constexpr int kNumPresets = 74;

namespace detail {
struct PresetDef {
  const char *name;
  Chip chip;
  uint8_t duty, sub, noise, arp, interval, vibrato, decay;
  float detune, cutoff;
  bool resonant;
};

inline const PresetDef *presetTable() {
  static const PresetDef t[kNumPresets] = {
      //  name          chip        duty sub nz arp itv vib dec  detn  cut   res
      {"NES LEAD",    kChipNES,    1, 0, 0, 0, 0, 6, 0, 2.f, 0.f,   false}, // 25 % pulse, delayed vibrato
      {"NES ARP",     kChipNES,    2, 0, 0, 1, 0, 0, 4, 0.f, 0.f,   false}, // tracker-style major arpeggio
      {"NES BASS",    kChipNES,    3, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // pulse plus triangle an octave down
      {"GB WAVE MIX",     kChipGB,     2, 1, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // organ wave table with a fifth above
      {"PSG BUZZ",   kChipPSG,    0, 1, 5, 0, 0, 0, 6, 0.f, 0.f,   false}, // periodic noise that tracks the note
      {"YM NOISE ENV",  kChipAY,     0, 3, 3, 0, 0, 0, 8, 0.f, 0.f,   false}, // YM2149 envelope buzzer with noise
      {"SID PWM",     kChipSID,    2, 1, 0, 0, 0, 0, 0, 0.f, 0.4f,  true},  // pulse, resonant low-pass
      {"SID SYNC",    kChipSID,    1, 3, 0, 0, 0, 0, 0, 0.f, 0.2f,  false}, // hard sync
      {"SID RING",    kChipSID,    0, 4, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // ring-modulated triangle at a fifth
      {"PCE FM",      kChipPCE,    0, 3, 0, 0, 5, 0, 0, 6.f, 0.f,   false}, // channel 2 FMs channel 1
      {"POKEY BASS",  kChipPOKEY,  1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 4-bit poly buzz with a clean sub
      {"POKEY HPF",   kChipPOKEY,  0, 3, 0, 0, 0, 0, 0, 4.f, 0.f,   false}, // high-pass mode
      {"VRC6 LEAD",   kChipVRC6,   0, 1, 0, 0, 0, 5, 0, 0.f, 0.f,   false}, // 12.5 % pulse, saw bass layer
      {"YM5B ENV HIT",     kChipS5B,    0, 4, 1, 0, 0, 0, 4, 0.f, 0.f,   false}, // Gimmick-style envelope buzz
      {"X TIA THUD",    kChipTIA,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 4-bit poly, coarse tuning
      {"NES CHORUS",  kChipNES,    2, 0, 0, 0, 0, 0, 0, 4.f, 0.f,   false}, // two pulses detuned
      {"NES SNARE",   kChipNES,    2, 0, 3, 0, 0, 0, 2, 0.f, 0.f,   false}, // fast-decaying high noise
      {"NES MIN RUN",   kChipNES,    0, 0, 0, 2, 0, 0, 0, 0.f, 0.f,   false}, // minor arpeggio, 12.5 % pulse
      {"GB NOISE HIT",    kChipGB,     3, 0, 2, 0, 0, 0, 6, 0.f, 0.f,   false}, // short mid noise burst
      {"PSG FIFTH",   kChipPSG,    0, 0, 0, 0, 4, 4, 0, 2.f, 0.f,   false}, // square with a fifth and vibrato
      {"YM LEAD",  kChipAY,     0, 2, 0, 0, 0, 4, 0, 3.f, 0.f,   false}, // detuned squares, unison sub
      {"YM TRI ENV",   kChipAY,     0, 4, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // triangle envelope buzzer
      {"SID BASS",    kChipSID,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.5f,  true},  // resonant saw, low cutoff
      {"SID PULSE HIT",   kChipSID,    2, 0, 2, 0, 0, 0, 6, 0.f, 0.3f,  false}, // filtered noise hit
      {"SID ARP",     kChipSID,    2, 2, 0, 1, 0, 0, 0, 0.f, 0.2f,  false}, // pulse arpeggio, gentle filter
      {"PCE OCT MIX",   kChipPCE,    0, 0, 0, 0, 5, 0, 0, 2.f, 0.f,   false}, // sine table, octave above
      {"POKEY LEAD",  kChipPOKEY,  0, 0, 0, 0, 0, 4, 0, 2.f, 0.f,   false}, // pure tone with vibrato
      {"VRC6 ARP",    kChipVRC6,   1, 1, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // 25 % pulse arpeggio, saw layer
      {"X TIA SNARE",    kChipTIA,    3, 0, 0, 0, 0, 0, 4, 0.f, 0.f,   false}, // 9-bit poly noise burst
      {"WT4 FIFTH",   kChipN163,   0, 0, 0, 0, 4, 0, 0, 2.f, 0.f,   false}, // sine plus third harmonic, fifth above
      {"WT4 SAW MIX",    kChipN163,   1, 0, 0, 0, 0, 5, 0, 0.f, 0.f,   false}, // 16-step saw with vibrato
      {"WT4 ORGAN",  kChipN163,   2, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // additive organ, octave layer
      {"WT4 HYBRID",   kChipN163,   3, 0, 0, 0, 0, 0, 0, 3.f, 0.f,   false}, // rough pulse-sine blend, detuned
      {"X FDS HARM",    kChipFDS,    2, 0, 0, 0, 5, 0, 0, 4.f, 0.f,   false}, // 5th-harmonic wave, light FM
      {"X FDS BASS",    kChipFDS,    0, 1, 0, 0, 0, 0, 0, 10.f, 0.f,  false}, // sine with deep FM, octave down
      {"X FDS WOBBLE",  kChipFDS,    3, 0, 0, 0, 2, 4, 0, 12.f, 0.f,  false}, // triangle, heavy FM, vibrato
      {"OLD PCM SAW",   kChipPaula,  0, 2, 0, 0, 0, 0, 0, 2.f, 0.f,   false}, // 8-bit saw, unison sub
      {"OLD PCM FM",    kChipPaula,  2, 3, 0, 0, 4, 0, 0, 8.f, 0.f,   false}, // organ sample FM
      {"OLD PCM AM",    kChipPaula,  3, 4, 0, 0, 5, 0, 0, 0.f, 0.f,   false}, // saw + octave sample AM
      // added in the second pass
      {"NES FIFTH",   kChipNES,    2, 0, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // power-chord pulses
      {"NES OCT ARP", kChipNES,    1, 0, 0, 3, 0, 0, 0, 0.f, 0.f,   false}, // octave-jump arpeggio
      {"PSG BASS",   kChipPSG,    0, 1, 0, 0, 5, 0, 0, 0.f, 0.f,   false}, // square, octave layers
      {"YM SAW ENV",   kChipAY,     0, 3, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // clean envelope saw buzz
      {"YM SUS ARP",   kChipAY,     0, 2, 0, 5, 0, 0, 0, 2.f, 0.f,   false}, // sus4 arpeggio, detuned unison
      {"SID TRI",     kChipSID,    0, 0, 0, 0, 0, 4, 0, 0.f, 0.35f, false}, // soft filtered triangle
      {"SID HOLLOW",  kChipSID,    2, 0, 0, 0, 0, 0, 0, 0.f, 0.55f, true},  // resonant pulse
      {"PCE BASS",    kChipPCE,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 5-bit saw and octave down
      {"PCE MAJ TRK",     kChipPCE,    3, 0, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // triangle arpeggio
      {"PCE FM FIFTH",    kChipPCE,    0, 3, 0, 0, 4, 0, 0, 12.f, 0.f,  false}, // deep FM at a fifth
      {"POKEY BUZZ",  kChipPOKEY,  3, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // rough distortion with sub
      {"YM5B BASS",     kChipS5B,    0, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // square with octave sub
      {"X TIA LOW NZ",    kChipTIA,    0, 0, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // pure divider tone
      {"X TIA METAL",     kChipTIA,    2, 0, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // coarse poly arpeggio
      {"WT4 ARP",    kChipN163,   2, 0, 0, 2, 0, 0, 3, 0.f, 0.f,   false}, // organ minor arpeggio, plucked
      {"X FDS SAW",   kChipFDS,    1, 0, 0, 0, 0, 0, 0, 6.f, 0.f,   false}, // saw carrier, FM at unison
      {"X FDS KEYS",    kChipFDS,    0, 0, 0, 0, 3, 0, 5, 5.f, 0.f,   false}, // electric-piano style FM pluck
      {"X FDS ARP",     kChipFDS,    2, 0, 0, 1, 5, 0, 0, 3.f, 0.f,   false}, // FM bell arpeggio
      {"OLD PCM BASS",  kChipPaula,  0, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // sample bass with octave down
      {"OLD PCM MIN", kChipPaula,  2, 2, 0, 2, 0, 0, 0, 0.f, 0.f,   false}, // organ minor arpeggio, unison
      {"OLD PCM BELL",  kChipPaula,  2, 0, 0, 0, 4, 0, 6, 0.f, 0.f,   false}, // organ sample with a fifth, decaying
      // v2: appended IDs; the original 0..59 are never renumbered.
      {"NES SOLO",    kChipNES,    1, 0, 0, 0, 0, 5, 0, 0.f, 0.f, false},
      {"NES TRI",     kChipNES,    2, 2, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"GB WAVE SOLO",kChipGB,     2, 2, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"GB HIGH HIT",    kChipGB,     0, 0, 3, 0, 0, 0, 2, 0.f, 0.f, false},
      {"PSG METAL",   kChipPSG,    0, 0, 5, 0, 0, 0, 4, 0.f, 0.f, false},
      {"YM SAW SOLO", kChipAY,     0, 3, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"SID GENTLE PWM",kChipSID,    2, 0, 0, 0, 0, 0, 0, 0.f, 0.3f, true},
      {"PCE WAVE",    kChipPCE,    3, 0, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"WT4 SOLO",    kChipN163,   0, 0, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"POKEY POLY",  kChipPOKEY,  3, 0, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"VRC6 SAW",    kChipVRC6,   2, 2, 0, 0, 0, 0, 0, 0.f, 0.f, false},
      {"NES MAJ 16TH",kChipNES,    1, 0, 0, 1, 0, 0, 0, 0.f, 0.f, false},
      {"PCE MIN 16TH",kChipPCE,    3, 0, 0, 2, 0, 0, 0, 0.f, 0.f, false},
      {"X FDS TABLE", kChipFDS,    3, 0, 0, 0, 4, 0, 0, 5.f, 0.f, false},
  };
  return t;
}
} // namespace detail

inline const char *presetName(int index) {
  return detail::presetTable()[index < 0 ? 0 : (index >= kNumPresets ? kNumPresets - 1 : index)].name;
}

inline ApuParams preset(int index) {
  const detail::PresetDef &d = detail::presetTable()[index < 0 ? 0 : (index >= kNumPresets ? kNumPresets - 1 : index)];
  ApuParams p;
  p.chip = d.chip;
  p.duty = d.duty;
  p.sub = d.sub;
  p.noise = d.noise;
  p.arp = d.arp;
  p.interval = d.interval;
  p.vibrato = d.vibrato;
  p.decay = d.decay;
  p.detune = d.detune;
  p.cutoff = d.cutoff;
  p.resonant = d.resonant;
  if (p.chip == kChipS5B) { p.chip = kChipAY; p.ay5b = true; }
  if (p.chip == kChipN163) { p.chip = kChipPCE; p.waveBank = 1; }
  const bool wave = p.chip == kChipGB || p.chip == kChipPCE;
  const uint16_t color = static_cast<uint16_t>(d.duty * (wave ? 128 : 256) + (wave ? 64 : 128) + (p.waveBank ? 512 : 0));
  applyColor(p, color);
  // The layered triangle preset must select COLOR's triangle zone explicitly.
  // SUB=4 alone is insufficient when COLOR chooses the layered envelope shape.
  if (index == 21) applyColor(p, 256);
  if (index == 16 || index == 18 || index == 63 || index == 64) p.voice = 4;
  if (index == 60 || index == 69) p.voice = 1;
  if (index == 61 || index == 62 || index == 70) p.voice = 3;
  if (index == 65) p.voice = 1;
  if (index == 66) applyColor(p, 2 * 256 + 32);
  if (index == 67 || index == 68) p.voice = 1;
  if (index == 71 || index == 72) p.arpRate = 3;
  if (p.chip == kChipTIA) {
    p.voice = 4;
    p.noise = static_cast<uint8_t>(1 + d.duty);
    p.decay = d.decay ? d.decay : 4;
  }
  return p;
}

} // namespace chiptune
