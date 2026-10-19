#pragma once
// Reference presets for the CHIPTUNE unit. Knob A (PRESET) loads one of these;
// the table also documents the parameter values (see docs/PRESETS.md) and is
// the test material for the host renderer.
//
// Presets that measured within about 6 dB (log-spectrum RMS over four time slices
// at 220 and 880 Hz) of an existing one were dropped; new ones were kept only
// when they were at least that far from every other preset.

#include "apu_core.h"

namespace chiptune {

constexpr int kNumPresets = 60;

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
      {"GB WAVE",     kChipGB,     2, 1, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // organ wave table with a fifth above
      {"SEGA BUZZ",   kChipPSG,    0, 1, 5, 0, 0, 0, 6, 0.f, 0.f,   false}, // periodic noise that tracks the note
      {"ATARI BUZZ",  kChipAY,     0, 3, 3, 0, 0, 0, 8, 0.f, 0.f,   false}, // YM2149 envelope buzzer with noise
      {"C64 PWM",     kChipSID,    2, 1, 0, 0, 0, 0, 0, 0.f, 0.4f,  true},  // pulse, resonant low-pass
      {"C64 SYNC",    kChipSID,    1, 3, 0, 0, 0, 0, 0, 0.f, 0.2f,  false}, // hard sync
      {"C64 RING",    kChipSID,    0, 4, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // ring-modulated triangle at a fifth
      {"PCE FM",      kChipPCE,    0, 3, 0, 0, 5, 0, 0, 6.f, 0.f,   false}, // channel 2 FMs channel 1
      {"POKEY BASS",  kChipPOKEY,  1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 4-bit poly buzz with a clean sub
      {"POKEY HPF",   kChipPOKEY,  0, 3, 0, 0, 0, 0, 0, 4.f, 0.f,   false}, // high-pass mode
      {"VRC6 LEAD",   kChipVRC6,   0, 1, 0, 0, 0, 5, 0, 0.f, 0.f,   false}, // 12.5 % pulse, saw bass layer
      {"5B BUZZ",     kChipS5B,    0, 4, 1, 0, 0, 0, 4, 0.f, 0.f,   false}, // Gimmick-style envelope buzz
      {"TIA BASS",    kChipTIA,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 4-bit poly, coarse tuning
      {"NES CHORUS",  kChipNES,    2, 0, 0, 0, 0, 0, 0, 4.f, 0.f,   false}, // two pulses detuned
      {"NES SNARE",   kChipNES,    2, 0, 3, 0, 0, 0, 2, 0.f, 0.f,   false}, // fast-decaying high noise
      {"NES MINOR",   kChipNES,    0, 0, 0, 2, 0, 0, 0, 0.f, 0.f,   false}, // minor arpeggio, 12.5 % pulse
      {"GB HIHAT",    kChipGB,     3, 0, 2, 0, 0, 0, 6, 0.f, 0.f,   false}, // short mid noise burst
      {"SEGA LEAD",   kChipPSG,    0, 0, 0, 0, 4, 4, 0, 2.f, 0.f,   false}, // square with a fifth and vibrato
      {"ATARI LEAD",  kChipAY,     0, 2, 0, 0, 0, 4, 0, 3.f, 0.f,   false}, // detuned squares, unison sub
      {"ATARI TRI",   kChipAY,     0, 4, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // triangle envelope buzzer
      {"C64 BASS",    kChipSID,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.5f,  true},  // resonant saw, low cutoff
      {"C64 NOISE",   kChipSID,    2, 0, 2, 0, 0, 0, 6, 0.f, 0.3f,  false}, // filtered noise hit
      {"C64 ARP",     kChipSID,    2, 2, 0, 1, 0, 0, 0, 0.f, 0.2f,  false}, // pulse arpeggio, gentle filter
      {"PCE ORGAN",   kChipPCE,    0, 0, 0, 0, 5, 0, 0, 2.f, 0.f,   false}, // sine table, octave above
      {"POKEY LEAD",  kChipPOKEY,  0, 0, 0, 0, 0, 4, 0, 2.f, 0.f,   false}, // pure tone with vibrato
      {"VRC6 ARP",    kChipVRC6,   1, 1, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // 25 % pulse arpeggio, saw layer
      {"TIA PERC",    kChipTIA,    3, 0, 0, 0, 0, 0, 4, 0.f, 0.f,   false}, // 9-bit poly noise burst
      {"N163 BELL",   kChipN163,   0, 0, 0, 0, 4, 0, 0, 2.f, 0.f,   false}, // sine plus third harmonic, fifth above
      {"N163 SAW",    kChipN163,   1, 0, 0, 0, 0, 5, 0, 0.f, 0.f,   false}, // 16-step saw with vibrato
      {"N163 ORGAN",  kChipN163,   2, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // additive organ, octave layer
      {"N163 BUZZ",   kChipN163,   3, 0, 0, 0, 0, 0, 0, 3.f, 0.f,   false}, // rough pulse-sine blend, detuned
      {"FDS BELL",    kChipFDS,    2, 0, 0, 0, 5, 0, 0, 4.f, 0.f,   false}, // 5th-harmonic wave, light FM
      {"FDS BASS",    kChipFDS,    0, 1, 0, 0, 0, 0, 0, 10.f, 0.f,  false}, // sine with deep FM, octave down
      {"FDS WOBBLE",  kChipFDS,    3, 0, 0, 0, 2, 4, 0, 12.f, 0.f,  false}, // triangle, heavy FM, vibrato
      {"AMIGA SAW",   kChipPaula,  0, 2, 0, 0, 0, 0, 0, 2.f, 0.f,   false}, // 8-bit saw, unison sub
      {"AMIGA FM",    kChipPaula,  2, 3, 0, 0, 4, 0, 0, 8.f, 0.f,   false}, // organ sample FM
      {"AMIGA AM",    kChipPaula,  3, 4, 0, 0, 5, 0, 0, 0.f, 0.f,   false}, // saw + octave sample AM
      // added in the second pass
      {"NES FIFTH",   kChipNES,    2, 0, 0, 0, 4, 0, 0, 0.f, 0.f,   false}, // power-chord pulses
      {"NES OCT ARP", kChipNES,    1, 0, 0, 3, 0, 0, 0, 0.f, 0.f,   false}, // octave-jump arpeggio
      {"SEGA BASS",   kChipPSG,    0, 1, 0, 0, 5, 0, 0, 0.f, 0.f,   false}, // square, octave layers
      {"ATARI SAW",   kChipAY,     0, 3, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // clean envelope saw buzz
      {"ATARI ARP",   kChipAY,     0, 2, 0, 5, 0, 0, 0, 2.f, 0.f,   false}, // sus4 arpeggio, detuned unison
      {"C64 TRI",     kChipSID,    0, 0, 0, 0, 0, 4, 0, 0.f, 0.35f, false}, // soft filtered triangle
      {"C64 HOLLOW",  kChipSID,    2, 0, 0, 0, 0, 0, 0, 0.f, 0.55f, true},  // resonant pulse
      {"PCE BASS",    kChipPCE,    1, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // 5-bit saw and octave down
      {"PCE ARP",     kChipPCE,    3, 0, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // triangle arpeggio
      {"PCE BELL",    kChipPCE,    0, 3, 0, 0, 4, 0, 0, 12.f, 0.f,  false}, // deep FM at a fifth
      {"POKEY BUZZ",  kChipPOKEY,  3, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // rough distortion with sub
      {"5B BASS",     kChipS5B,    0, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // square with octave sub
      {"TIA LEAD",    kChipTIA,    0, 0, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // pure divider tone
      {"TIA ARP",     kChipTIA,    2, 0, 0, 1, 0, 0, 0, 0.f, 0.f,   false}, // coarse poly arpeggio
      {"N163 ARP",    kChipN163,   2, 0, 0, 2, 0, 0, 3, 0.f, 0.f,   false}, // organ minor arpeggio, plucked
      {"FDS BRASS",   kChipFDS,    1, 0, 0, 0, 0, 0, 0, 6.f, 0.f,   false}, // saw carrier, FM at unison
      {"FDS KEYS",    kChipFDS,    0, 0, 0, 0, 3, 0, 5, 5.f, 0.f,   false}, // electric-piano style FM pluck
      {"FDS ARP",     kChipFDS,    2, 0, 0, 1, 5, 0, 0, 3.f, 0.f,   false}, // FM bell arpeggio
      {"AMIGA BASS",  kChipPaula,  0, 1, 0, 0, 0, 0, 0, 0.f, 0.f,   false}, // sample bass with octave down
      {"AMIGA CHORD", kChipPaula,  2, 2, 0, 2, 0, 0, 0, 0.f, 0.f,   false}, // organ minor arpeggio, unison
      {"AMIGA BELL",  kChipPaula,  2, 0, 0, 0, 4, 0, 6, 0.f, 0.f,   false}, // organ sample with a fifth, decaying
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
  return p;
}

} // namespace chiptune
