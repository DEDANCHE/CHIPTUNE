#pragma once
// Reference presets for the CHIPTUNE unit. The NTS-1 mkII unit API has no preset
// slot, so these double as a table of parameter values to dial in by hand
// (see docs/PRESETS.md) and as test material for the host renderer.

#include "apu_core.h"

namespace chiptune {

constexpr int kNumPresets = 45;

inline const char *presetName(int index) {
  static const char *const names[kNumPresets] = {
      "NES LEAD",   "NES ARP",    "NES BASS",   "GB WAVE",     "SEGA BUZZ",
      "ATARI BUZZ", "C64 PWM",    "C64 SYNC",   "C64 RING",    "PCE FM",
      "POKEY BASS", "POKEY HPF",  "VRC6 LEAD",  "5B BUZZ",     "TIA BASS",
      "NES CHORUS", "NES SNARE",  "NES MINOR",  "GB SUB BASS", "GB HIHAT",
      "SEGA LEAD",  "SEGA SNARE", "ATARI LEAD", "ATARI TRI",   "C64 BASS",
      "C64 NOISE",  "C64 ARP",    "PCE ORGAN",  "POKEY LEAD",  "POKEY NOISE",
      "VRC6 ARP",   "5B LEAD",    "TIA PERC",   "N163 BELL",   "N163 SAW",
      "N163 ORGAN", "N163 BUZZ",  "FDS BELL",   "FDS BASS",    "FDS SAW",
      "FDS WOBBLE", "AMIGA SAW",  "AMIGA PWM",  "AMIGA FM",    "AMIGA AM"};
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
  case 14: // TIA BASS: 4-bit poly, coarse tuning
    p.chip = kChipTIA; p.duty = 1; p.sub = 1;
    break;
  case 15: // NES CHORUS: two pulses detuned for phase beating
    p.chip = kChipNES; p.duty = 2; p.detune = 4.f;
    break;
  case 16: // NES SNARE: pulse with a fast-decaying high noise
    p.chip = kChipNES; p.duty = 2; p.noise = 3; p.decay = 2;
    break;
  case 17: // NES MINOR: minor arpeggio, 12.5 % pulse
    p.chip = kChipNES; p.duty = 0; p.arp = 2;
    break;
  case 18: // GB SUB BASS: sine wave table an octave down
    p.chip = kChipGB; p.duty = 0; p.sub = 1;
    break;
  case 19: // GB HIHAT: short mid noise burst
    p.chip = kChipGB; p.duty = 3; p.noise = 2; p.decay = 6;
    break;
  case 20: // SEGA LEAD: square with a fifth above and vibrato
    p.chip = kChipPSG; p.interval = 4; p.vibrato = 4; p.detune = 2.f;
    break;
  case 21: // SEGA SNARE: white noise burst
    p.chip = kChipPSG; p.noise = 3; p.decay = 3;
    break;
  case 22: // ATARI LEAD: detuned squares, unison sub
    p.chip = kChipAY; p.sub = 2; p.detune = 3.f; p.vibrato = 4;
    break;
  case 23: // ATARI TRI: triangle envelope buzzer
    p.chip = kChipAY; p.sub = 4;
    break;
  case 24: // C64 BASS: resonant saw, low cutoff
    p.chip = kChipSID; p.duty = 1; p.sub = 1; p.cutoff = 0.5f; p.resonant = true;
    break;
  case 25: // C64 NOISE: filtered noise hit
    p.chip = kChipSID; p.duty = 2; p.noise = 2; p.decay = 6; p.cutoff = 0.3f;
    break;
  case 26: // C64 ARP: pulse arpeggio, gentle filter
    p.chip = kChipSID; p.duty = 2; p.arp = 1; p.sub = 2; p.cutoff = 0.2f;
    break;
  case 27: // PCE ORGAN: sine wave table, octave above, slight detune
    p.chip = kChipPCE; p.duty = 0; p.interval = 5; p.detune = 2.f;
    break;
  case 28: // POKEY LEAD: pure tone with vibrato
    p.chip = kChipPOKEY; p.duty = 0; p.detune = 2.f; p.vibrato = 4;
    break;
  case 29: // POKEY NOISE: 17-bit poly noise burst
    p.chip = kChipPOKEY; p.duty = 2; p.noise = 2; p.decay = 5;
    break;
  case 30: // VRC6 ARP: 25 % pulse arpeggio with saw layer
    p.chip = kChipVRC6; p.duty = 1; p.arp = 1; p.sub = 1;
    break;
  case 31: // 5B LEAD: squares with unison sub and vibrato
    p.chip = kChipS5B; p.sub = 2; p.detune = 2.f; p.vibrato = 4;
    break;
  case 32: // TIA PERC: 9-bit poly noise burst
    p.chip = kChipTIA; p.duty = 3; p.decay = 4;
    break;
  case 33: // N163 BELL: 4-bit sine plus third harmonic, fifth above
    p.chip = kChipN163; p.duty = 0; p.interval = 4; p.detune = 2.f;
    break;
  case 34: // N163 SAW: 16-step saw with vibrato
    p.chip = kChipN163; p.duty = 1; p.vibrato = 5;
    break;
  case 35: // N163 ORGAN: additive organ, octave layer
    p.chip = kChipN163; p.duty = 2; p.sub = 1;
    break;
  case 36: // N163 BUZZ: rough pulse-sine blend, detuned
    p.chip = kChipN163; p.duty = 3; p.detune = 3.f;
    break;
  case 37: // FDS BELL: sine carrier, 5th-harmonic wave, light FM
    p.chip = kChipFDS; p.duty = 2; p.interval = 5; p.detune = 4.f;
    break;
  case 38: // FDS BASS: sine carrier with deep FM, octave down
    p.chip = kChipFDS; p.duty = 0; p.interval = 0; p.detune = 10.f; p.sub = 1;
    break;
  case 39: // FDS SAW: 6-bit saw with arpeggio
    p.chip = kChipFDS; p.duty = 1; p.arp = 1;
    break;
  case 40: // FDS WOBBLE: triangle carrier, slow-ish heavy FM
    p.chip = kChipFDS; p.duty = 3; p.interval = 2; p.detune = 12.f; p.vibrato = 4;
    break;
  case 41: // AMIGA SAW: 8-bit saw, unison sub
    p.chip = kChipPaula; p.duty = 0; p.sub = 2; p.detune = 2.f;
    break;
  case 42: // AMIGA PWM: 25 % pulse sample, octave down
    p.chip = kChipPaula; p.duty = 1; p.sub = 1;
    break;
  case 43: // AMIGA FM: organ sample frequency-modulated by voice 2
    p.chip = kChipPaula; p.duty = 2; p.sub = 3; p.interval = 4; p.detune = 8.f;
    break;
  default: // AMIGA AM: saw-plus-octave sample amplitude-modulated (Paula channel modulation)
    p.chip = kChipPaula; p.duty = 3; p.sub = 4; p.interval = 5;
    break;
  }
  return p;
}

} // namespace chiptune
