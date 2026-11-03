# Revision 2 presets

The PRESET menu contains 74 entries. Original indices 0..59 are reserved; fourteen
new patches are appended at 60..73. Names and several voicings have changed deliberately.
Version 2.0.2 repairs preset 21 into a triangle envelope and corrects misleading names.
All other preset audio definitions remain unchanged by this preset refinement pass.
Knob A is COLOR; knob B is detune, SID filter closure, or FM depth.

See [Oscillators](oscillators.md) for the control map, migration rules,
engine changes and validation status. Vibrato remains stored in the preset.
The previous measured spectral-distance claims do not apply to these revised sounds.

The custom unit adapter resolves a preset before explicit edits in the same callback
batch, then commits before rendering or note-on. Parameter reads resolve the same pending
state without consuming it. They return the current sound's values for saving and recall.
CHIP family and COLOR are resolved before the other controls, irrespective of callback order.
DETN/FLT saves at ten-bit resolution. Repeated setter values are valid edits.

A v1 raw parameter array is not a v2 patch: slot 0 and TUNE/RES have changed. Reselect the
retained preset ID, recreate the desired edits using v2 controls, and save again.
Legacy saved data may contain stale menu values that cannot be recovered automatically.

The eight primary families occupy CHIP 0..7. YM 5B presets retain a clock variant;
WT4 presets use the short four-bit bank; X TIA/X FDS are experimental; OLD PCM entries
keep the legacy generated-wave model reachable. CHIP 8..12 also expose the compatibility
models explicitly. CHIP saves the actual engine/clock variant, with WT4 represented by
PCE plus its COLOR bank. A primary CHIP override replaces the preset's legacy model
and survives saving and recall. See the revision notes for migrating v2.0.0 legacy patches.

| ID | Preset | Family / bank |
|---|---|---|
| 0 | NES LEAD | NES |
| 1 | NES ARP | NES |
| 2 | NES BASS | NES |
| 3 | GB WAVE MIX | GB |
| 4 | PSG BUZZ | SN76489 |
| 5 | YM NOISE ENV | YM2149 |
| 6 | SID PWM | SID |
| 7 | SID SYNC | SID |
| 8 | SID RING | SID |
| 9 | PCE FM | PCE/WT |
| 10 | POKEY BASS | POKEY |
| 11 | POKEY HPF | POKEY |
| 12 | VRC6 LEAD | VRC6 |
| 13 | YM5B ENV HIT | YM2149 / 5B variant |
| 14 | X TIA THUD | Experimental TIA percussion |
| 15 | NES CHORUS | NES |
| 16 | NES SNARE | NES |
| 17 | NES MIN RUN | NES |
| 18 | GB NOISE HIT | GB |
| 19 | PSG FIFTH | SN76489 |
| 20 | YM LEAD | YM2149 |
| 21 | YM TRI ENV | YM2149 |
| 22 | SID BASS | SID |
| 23 | SID PULSE HIT | SID |
| 24 | SID ARP | SID |
| 25 | PCE OCT MIX | PCE/WT |
| 26 | POKEY LEAD | POKEY |
| 27 | VRC6 ARP | VRC6 |
| 28 | X TIA SNARE | Experimental TIA percussion |
| 29 | WT4 FIFTH | PCE/WT four-bit bank |
| 30 | WT4 SAW MIX | PCE/WT four-bit bank |
| 31 | WT4 ORGAN | PCE/WT four-bit bank |
| 32 | WT4 HYBRID | PCE/WT four-bit bank |
| 33 | X FDS HARM | Experimental FDS |
| 34 | X FDS BASS | Experimental FDS |
| 35 | X FDS WOBBLE | Experimental FDS |
| 36 | OLD PCM SAW | Retired PCM compatibility |
| 37 | OLD PCM FM | Retired PCM compatibility |
| 38 | OLD PCM AM | Retired PCM compatibility |
| 39 | NES FIFTH | NES |
| 40 | NES OCT ARP | NES |
| 41 | PSG BASS | SN76489 |
| 42 | YM SAW ENV | YM2149 |
| 43 | YM SUS ARP | YM2149 |
| 44 | SID TRI | SID |
| 45 | SID HOLLOW | SID |
| 46 | PCE BASS | PCE/WT |
| 47 | PCE MAJ TRK | PCE/WT |
| 48 | PCE FM FIFTH | PCE/WT |
| 49 | POKEY BUZZ | POKEY |
| 50 | YM5B BASS | YM2149 / 5B variant |
| 51 | X TIA LOW NZ | Experimental TIA percussion |
| 52 | X TIA METAL | Experimental TIA percussion |
| 53 | WT4 ARP | PCE/WT four-bit bank |
| 54 | X FDS SAW | Experimental FDS |
| 55 | X FDS KEYS | Experimental FDS |
| 56 | X FDS ARP | Experimental FDS |
| 57 | OLD PCM BASS | Retired PCM compatibility |
| 58 | OLD PCM MIN | Retired PCM compatibility |
| 59 | OLD PCM BELL | Retired PCM compatibility |
| 60 | NES SOLO | NES |
| 61 | NES TRI | NES |
| 62 | GB WAVE SOLO | GB |
| 63 | GB HIGH HIT | GB |
| 64 | PSG METAL | SN76489 |
| 65 | YM SAW SOLO | YM2149 |
| 66 | SID GENTLE PWM | SID |
| 67 | PCE WAVE | PCE/WT |
| 68 | WT4 SOLO | PCE/WT four-bit bank |
| 69 | POKEY POLY | POKEY |
| 70 | VRC6 SAW | VRC6 |
| 71 | NES MAJ 16TH | NES |
| 72 | PCE MIN 16TH | PCE/WT |
| 73 | X FDS TABLE | Experimental FDS |
