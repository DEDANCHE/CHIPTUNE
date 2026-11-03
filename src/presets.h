#pragma once
// CHIPTUNE presets. Copyright (C) 2026 DEDANCHE.
// Licensed under the GNU General Public License v3.0 (see LICENSE).
// Stable preset IDs. All sound-defining defaults are explicit in the table.
#include "apu_core.h"
namespace chiptune {
enum PresetId {
  kPresetNesLead = 0,
  kPresetNesArp = 1,
  kPresetNesBass = 2,
  kPresetGbWaveMix = 3,
  kPresetPsgBuzz = 4,
  kPresetYmNoiseEnv = 5,
  kPresetSidPwm = 6,
  kPresetSidSync = 7,
  kPresetSidRing = 8,
  kPresetPceFm = 9,
  kPresetPokeyBass = 10,
  kPresetPokeyHpf = 11,
  kPresetVrc6Lead = 12,
  kPresetYm5bEnvHit = 13,
  kPresetXTiaThud = 14,
  kPresetNesChorus = 15,
  kPresetNesSnare = 16,
  kPresetNesMinRun = 17,
  kPresetGbNoiseHit = 18,
  kPresetPsgFifth = 19,
  kPresetYmLead = 20,
  kPresetYmTriEnv = 21,
  kPresetSidBass = 22,
  kPresetSidPulseHit = 23,
  kPresetSidArp = 24,
  kPresetPceOctMix = 25,
  kPresetPokeyLead = 26,
  kPresetVrc6Arp = 27,
  kPresetXTiaSnare = 28,
  kPresetWt4Fifth = 29,
  kPresetWt4SawMix = 30,
  kPresetWt4Organ = 31,
  kPresetWt4Hybrid = 32,
  kPresetXFdsHarm = 33,
  kPresetXFdsBass = 34,
  kPresetXFdsWobble = 35,
  kPresetOldPcmSaw = 36,
  kPresetOldPcmFm = 37,
  kPresetOldPcmAm = 38,
  kPresetNesFifth = 39,
  kPresetNesOctArp = 40,
  kPresetPsgBass = 41,
  kPresetYmSawEnv = 42,
  kPresetYmSusArp = 43,
  kPresetSidTri = 44,
  kPresetSidHollow = 45,
  kPresetPceBass = 46,
  kPresetPceMajTrk = 47,
  kPresetPceFmFifth = 48,
  kPresetPokeyBuzz = 49,
  kPresetYm5bBass = 50,
  kPresetXTiaLowNz = 51,
  kPresetXTiaMetal = 52,
  kPresetWt4Arp = 53,
  kPresetXFdsSaw = 54,
  kPresetXFdsKeys = 55,
  kPresetXFdsArp = 56,
  kPresetOldPcmBass = 57,
  kPresetOldPcmMin = 58,
  kPresetOldPcmBell = 59,
  kPresetNesSolo = 60,
  kPresetNesTri = 61,
  kPresetGbWaveSolo = 62,
  kPresetGbHighHit = 63,
  kPresetPsgMetal = 64,
  kPresetYmSawSolo = 65,
  kPresetSidGentlePwm = 66,
  kPresetPceWave = 67,
  kPresetWt4Solo = 68,
  kPresetPokeyPoly = 69,
  kPresetVrc6Saw = 70,
  kPresetNesMaj16th = 71,
  kPresetPceMin16th = 72,
  kPresetXFdsTable = 73,
  kPresetSidSlowDeep = 74,
  kPresetNesKick = 75,
  kPresetPceTom = 76,
  kNumPresets = 77
};
namespace detail {
struct PresetDef {
  const char *name;
  Chip chip;
  uint16_t color;
  uint8_t sub, noise, arp, interval, vibrato, decay;
  uint8_t detune, cutoffPercent;
  bool resonant;
  uint8_t voice, arpRate;
  bool ay5b;
  uint8_t articulation;
};
inline const PresetDef *presetTable() {
  static const PresetDef t[kNumPresets] = {
    // name, chip, COLOR, sub, noise, arp, interval, vibrato, decay, detune, cutoff, resonance, voice, arp rate, 5B, articulation
    {"NES LEAD", kChipNES, 384, 0, 0, 0, 0, 6, 0, 2, 0, false, 0, 0, false, 0},
    {"NES ARP", kChipNES, 640, 0, 0, 1, 0, 0, 4, 0, 0, false, 0, 0, false, 0},
    {"NES BASS", kChipNES, 896, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"GB WAVE MIX", kChipGB, 320, 1, 0, 0, 4, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"PSG BUZZ", kChipPSG, 128, 1, 5, 0, 0, 0, 6, 0, 0, false, 0, 0, false, 0},
    {"YM NOISE ENV", kChipAY, 128, 3, 3, 0, 0, 0, 8, 0, 0, false, 0, 0, false, 0},
    {"SID PWM", kChipSID, 640, 1, 0, 0, 0, 0, 0, 0, 40, true, 0, 0, false, 0},
    {"SID SYNC", kChipSID, 384, 3, 0, 0, 0, 0, 0, 0, 20, false, 0, 0, false, 0},
    {"SID RING", kChipSID, 128, 4, 0, 0, 4, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"PCE FM", kChipPCE, 64, 3, 0, 0, 5, 0, 0, 6, 0, false, 0, 0, false, 0},
    {"POKEY BASS", kChipPOKEY, 384, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"POKEY HPF", kChipPOKEY, 128, 3, 0, 0, 0, 0, 0, 4, 0, false, 0, 0, false, 0},
    {"VRC6 LEAD", kChipVRC6, 128, 1, 0, 0, 0, 5, 0, 0, 0, false, 0, 0, false, 0},
    {"YM5B ENV HIT", kChipAY, 128, 4, 1, 0, 0, 0, 4, 0, 0, false, 0, 0, true, 0},
    {"X TIA THUD", kChipTIA, 384, 1, 2, 0, 0, 0, 4, 0, 0, false, 4, 0, false, 0},
    {"NES CHORUS", kChipNES, 640, 0, 0, 0, 0, 0, 0, 4, 0, false, 0, 0, false, 0},
    {"NES SNARE", kChipNES, 128, 0, 3, 0, 0, 0, 2, 0, 0, false, 4, 0, false, 0},
    {"NES MIN RUN", kChipNES, 128, 0, 0, 2, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"GB NOISE HIT", kChipGB, 128, 0, 2, 0, 0, 0, 6, 0, 0, false, 4, 0, false, 0},
    {"PSG FIFTH", kChipPSG, 128, 0, 0, 0, 4, 4, 0, 2, 0, false, 0, 0, false, 0},
    {"YM LEAD", kChipAY, 128, 2, 0, 0, 0, 4, 0, 3, 0, false, 0, 0, false, 0},
    {"YM TRI ENV", kChipAY, 256, 4, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"SID BASS", kChipSID, 384, 1, 0, 0, 0, 0, 0, 0, 50, true, 0, 0, false, 0},
    {"SID PULSE HIT", kChipSID, 640, 0, 2, 0, 0, 0, 6, 0, 30, false, 0, 0, false, 0},
    {"SID ARP", kChipSID, 640, 2, 0, 1, 0, 0, 0, 0, 20, false, 0, 0, false, 0},
    {"PCE OCT MIX", kChipPCE, 64, 0, 0, 0, 5, 0, 0, 2, 0, false, 0, 0, false, 0},
    {"POKEY LEAD", kChipPOKEY, 128, 0, 0, 0, 0, 4, 0, 2, 0, false, 0, 0, false, 0},
    {"VRC6 ARP", kChipVRC6, 384, 1, 0, 1, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"X TIA SNARE", kChipTIA, 896, 0, 4, 0, 0, 0, 4, 0, 0, false, 4, 0, false, 0},
    {"WT4 FIFTH", kChipPCE, 576, 0, 0, 0, 4, 0, 0, 2, 0, false, 0, 0, false, 0},
    {"WT4 SAW MIX", kChipPCE, 704, 0, 0, 0, 0, 5, 0, 0, 0, false, 0, 0, false, 0},
    {"WT4 ORGAN", kChipPCE, 832, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"WT4 HYBRID", kChipPCE, 960, 0, 0, 0, 0, 0, 0, 3, 0, false, 0, 0, false, 0},
    {"X FDS HARM", kChipFDS, 640, 0, 0, 0, 5, 0, 0, 4, 0, false, 0, 0, false, 0},
    {"X FDS BASS", kChipFDS, 128, 1, 0, 0, 0, 0, 0, 10, 0, false, 0, 0, false, 0},
    {"X FDS WOBBLE", kChipFDS, 896, 0, 0, 0, 2, 4, 0, 12, 0, false, 0, 0, false, 0},
    {"OLD PCM SAW", kChipPaula, 128, 2, 0, 0, 0, 0, 0, 2, 0, false, 0, 0, false, 0},
    {"OLD PCM FM", kChipPaula, 640, 3, 0, 0, 4, 0, 0, 8, 0, false, 0, 0, false, 0},
    {"OLD PCM AM", kChipPaula, 896, 4, 0, 0, 5, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"NES FIFTH", kChipNES, 640, 0, 0, 0, 4, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"NES OCT ARP", kChipNES, 384, 0, 0, 3, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"PSG BASS", kChipPSG, 128, 1, 0, 0, 5, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"YM SAW ENV", kChipAY, 128, 3, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"YM SUS ARP", kChipAY, 128, 2, 0, 5, 0, 0, 0, 2, 0, false, 0, 0, false, 0},
    {"SID TRI", kChipSID, 128, 0, 0, 0, 0, 4, 0, 0, 35, false, 0, 0, false, 0},
    {"SID HOLLOW", kChipSID, 640, 0, 0, 0, 0, 0, 0, 0, 55, true, 0, 0, false, 0},
    {"PCE BASS", kChipPCE, 192, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"PCE MAJ TRK", kChipPCE, 448, 0, 0, 1, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"PCE FM FIFTH", kChipPCE, 64, 3, 0, 0, 4, 0, 0, 12, 0, false, 0, 0, false, 0},
    {"POKEY BUZZ", kChipPOKEY, 896, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"YM5B BASS", kChipAY, 128, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, true, 0},
    {"X TIA LOW NZ", kChipTIA, 128, 0, 1, 0, 0, 0, 4, 0, 0, false, 4, 0, false, 0},
    {"X TIA METAL", kChipTIA, 640, 0, 3, 1, 0, 0, 4, 0, 0, false, 4, 0, false, 0},
    {"WT4 ARP", kChipPCE, 832, 0, 0, 2, 0, 0, 3, 0, 0, false, 0, 0, false, 0},
    {"X FDS SAW", kChipFDS, 384, 0, 0, 0, 0, 0, 0, 6, 0, false, 0, 0, false, 0},
    {"X FDS KEYS", kChipFDS, 128, 0, 0, 0, 3, 0, 5, 5, 0, false, 0, 0, false, 0},
    {"X FDS ARP", kChipFDS, 640, 0, 0, 1, 5, 0, 0, 3, 0, false, 0, 0, false, 0},
    {"OLD PCM BASS", kChipPaula, 128, 1, 0, 0, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"OLD PCM MIN", kChipPaula, 640, 2, 0, 2, 0, 0, 0, 0, 0, false, 0, 0, false, 0},
    {"OLD PCM BELL", kChipPaula, 640, 0, 0, 0, 4, 0, 6, 0, 0, false, 0, 0, false, 0},
    {"NES SOLO", kChipNES, 384, 0, 0, 0, 0, 5, 0, 0, 0, false, 1, 0, false, 0},
    {"NES TRI", kChipNES, 640, 2, 0, 0, 0, 0, 0, 0, 0, false, 3, 0, false, 0},
    {"GB WAVE SOLO", kChipGB, 320, 2, 0, 0, 0, 0, 0, 0, 0, false, 3, 0, false, 0},
    {"GB HIGH HIT", kChipGB, 64, 0, 3, 0, 0, 0, 2, 0, 0, false, 4, 0, false, 0},
    {"PSG METAL", kChipPSG, 128, 0, 5, 0, 0, 0, 4, 0, 0, false, 4, 0, false, 0},
    {"YM SAW SOLO", kChipAY, 128, 3, 0, 0, 0, 0, 0, 0, 0, false, 1, 0, false, 0},
    {"SID GENTLE PWM", kChipSID, 544, 0, 0, 0, 0, 0, 0, 0, 30, true, 0, 0, false, 0},
    {"PCE WAVE", kChipPCE, 448, 0, 0, 0, 0, 0, 0, 0, 0, false, 1, 0, false, 0},
    {"WT4 SOLO", kChipPCE, 576, 0, 0, 0, 0, 0, 0, 0, 0, false, 1, 0, false, 0},
    {"POKEY POLY", kChipPOKEY, 896, 0, 0, 0, 0, 0, 0, 0, 0, false, 1, 0, false, 0},
    {"VRC6 SAW", kChipVRC6, 640, 2, 0, 0, 0, 0, 0, 0, 0, false, 3, 0, false, 0},
    {"NES MAJ 16TH", kChipNES, 384, 0, 0, 1, 0, 0, 0, 0, 0, false, 0, 3, false, 0},
    {"PCE MIN 16TH", kChipPCE, 448, 0, 0, 2, 0, 0, 0, 0, 0, false, 0, 3, false, 0},
    {"X FDS TABLE", kChipFDS, 896, 0, 0, 0, 4, 0, 0, 5, 0, false, 0, 0, false, 0},
    {"SID SLOW DEEP", kChipSID, 640, 0, 0, 0, 0, 0, 0, 0, 30, true, 0, 0, false, 3},
    {"NES KICK", kChipNES, 640, 2, 0, 0, 0, 0, 4, 0, 0, false, 3, 0, false, 1},
    {"PCE TOM", kChipPCE, 64, 0, 0, 0, 0, 0, 6, 0, 0, false, 1, 0, false, 2}
  };
  return t;
}
} // namespace detail
inline int presetIndex(int index) { return index < 0 ? 0 : (index >= kNumPresets ? kNumPresets - 1 : index); }
inline const char *presetName(int index) { return detail::presetTable()[presetIndex(index)].name; }
inline ApuParams preset(int index) {
  const int idx = presetIndex(index);
  const detail::PresetDef &d = detail::presetTable()[idx];
  ApuParams p;
  p.chip=d.chip; p.sub=d.sub; p.noise=d.noise; p.arp=d.arp;
  p.interval=d.interval; p.vibrato=d.vibrato; p.decay=d.decay;
  p.detune=d.detune; p.cutoff=d.cutoffPercent / 100.f; p.resonant=d.resonant;
  p.voice=d.voice; p.arpRate=d.arpRate; p.ay5b=d.ay5b; p.articulation=d.articulation;
  applyColor(p,d.color);
  if (idx == kPresetNesBass) { p.pulse1Level = 0.8f; p.pulse2Level = 0.8f; p.waveLevel = 0.82f; }
  else if (idx == kPresetGbWaveMix) { p.pulse1Level = 0.8f; p.pulse2Level = 0.8f; p.waveLevel = 0.85f; }
  else if (idx == kPresetNesOctArp || idx == kPresetNesMaj16th) { p.pulse1Level = 0.82f; p.pulse2Level = 0.82f; }
  return p;
}
} // namespace chiptune
