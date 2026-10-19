#pragma once
// Reference presets for the CHIPTUNE unit. The NTS-1 mkII unit API has no preset
// slot, so these double as a table of parameter values to dial in by hand
// (see docs/PRESETS.md) and as test material for the host renderer.

#include "apu_core.h"

namespace chiptune {

constexpr int kNumPresets = 15;

inline const char *presetName(int index) {
  static const char *const names[kNumPresets] = {
      "NES LEAD",   "NES ARP",    "NES BASS",   "GB WAVE",     "SEGA BUZZ",
      "ATARI BUZZ", "C64 PWM",    "C64 SYNC",   "C64 RING",    "PCE FM",
      "POKEY BASS", "POKEY HPF",  "VRC6 LEAD",  "5B BUZZ",     "TIA BASS"};
  return names[index < 0 ? 0 : (index >= kNumPresets ? kNumPresets - 1 : index)];
}

inline ApuParams preset(int index) {
  ApuParams p;
  switch (index) {
  case 0: // NES LEAD: 25 % pulse with delayed vibrato
    p.chip = kChipNES; p.duty = 1; p.vibrato = 6; p.interval = 0; p.detune = 2.f;
    break;
  case 1: // NES ARP: tracker-style major arpeggio
    p.chip = kChipNES; p.duty = 2; p.arp = 1; p.decay = 4;
    break;
  case 2: // NES BASS: pulse plus triangle an octave down
    p.chip = kChipNES; p.duty = 3; p.sub = 1; p.decay = 0;
    break;
  case 3: // GB WAVE: organ wave table with a fifth above
    p.chip = kChipGB; p.duty = 2; p.interval = 4; p.sub = 1;
    break;
  case 4: // SEGA BUZZ: periodic noise that tracks the note
    p.chip = kChipPSG; p.noise = 5; p.sub = 1; p.decay = 6;
    break;
  case 5: // ATARI BUZZ: YM2149 envelope buzzer with noise
    p.chip = kChipAY; p.sub = 3; p.noise = 3; p.decay = 8;
    break;
  case 6: // C64 PWM: pulse, resonant low-pass
    p.chip = kChipSID; p.duty = 2; p.sub = 1; p.cutoff = 0.4f; p.resonant = true;
    break;
  case 7: // C64 SYNC: hard sync, slave swept against the master
    p.chip = kChipSID; p.duty = 1; p.sub = 3; p.cutoff = 0.2f;
    break;
  case 8: // C64 RING: ring-modulated triangle at a fifth
    p.chip = kChipSID; p.sub = 4; p.interval = 4;
    break;
  case 9: // PCE FM: channel 2 an octave up frequency-modulates channel 1
    p.chip = kChipPCE; p.duty = 0; p.sub = 3; p.interval = 5; p.detune = 6.f;
    break;
  case 10: // POKEY BASS: 4-bit poly buzz with a clean sub
    p.chip = kChipPOKEY; p.duty = 1; p.sub = 1; p.decay = 0;
    break;
  case 11: // POKEY HPF: high-pass mode, detuned sampling clock
    p.chip = kChipPOKEY; p.duty = 0; p.sub = 3; p.detune = 4.f;
    break;
  case 12: // VRC6 LEAD: 12.5 % pulse, saw bass layer
    p.chip = kChipVRC6; p.duty = 0; p.sub = 1; p.vibrato = 5;
    break;
  case 13: // 5B BUZZ: Gimmick-style envelope buzz
    p.chip = kChipS5B; p.sub = 4; p.noise = 1; p.decay = 4;
    break;
  default: // TIA BASS: 4-bit poly, coarse tuning
    p.chip = kChipTIA; p.duty = 1; p.sub = 1;
    break;
  }
  return p;
}

} // namespace chiptune
