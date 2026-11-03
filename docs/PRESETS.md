# Revision 2 presets

The PRESET menu contains 74 entries. Original indices 0..59 are reserved; fourteen
new patches are appended at 60..73. Names and several voicings have changed deliberately.
Knob A is COLOR; knob B is detune, SID filter closure, or FM depth.

See [Oscillator revision 2](OSCILLATORS_V2.md) for the control map, migration rules,
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
keep the retired generated-wave model reachable without adding it to the main selector.

| ID | Preset | Family / bank |
|---|---|---|
| 0 | NES LEAD | NES |
| 1 | NES ARP | NES |
| 2 | NES BASS | NES |
| 3 | GB WAVE | GB |
| 4 | SEGA BUZZ | SN76489 |
| 5 | ATARI BUZZ | YM2149 |
| 6 | C64 PWM | SID |
| 7 | C64 SYNC | SID |
| 8 | C64 RING | SID |
| 9 | PCE FM | PCE/WT |
| 10 | POKEY BASS | POKEY |
| 11 | POKEY HPF | POKEY |
| 12 | VRC6 LEAD | VRC6 |
| 13 | YM 5B BUZZ | YM2149 / 5B variant |
| 14 | X TIA KICK | Experimental TIA percussion |
| 15 | NES CHORUS | NES |
| 16 | NES SNARE | NES |
| 17 | NES MINOR | NES |
| 18 | GB HIHAT | GB |
| 19 | SEGA LEAD | SN76489 |
| 20 | ATARI LEAD | YM2149 |
| 21 | ATARI TRI | YM2149 |
| 22 | C64 BASS | SID |
| 23 | C64 NOISE | SID |
| 24 | C64 ARP | SID |
| 25 | PCE ORGAN | PCE/WT |
| 26 | POKEY LEAD | POKEY |
| 27 | VRC6 ARP | VRC6 |
| 28 | X TIA SNARE | Experimental TIA percussion |
| 29 | WT4 BELL | PCE/WT four-bit bank |
| 30 | WT4 SAW | PCE/WT four-bit bank |
| 31 | WT4 ORGAN | PCE/WT four-bit bank |
| 32 | WT4 BUZZ | PCE/WT four-bit bank |
| 33 | X FDS BELL | Experimental FDS |
| 34 | X FDS BASS | Experimental FDS |
| 35 | X FDS WOBBLE | Experimental FDS |
| 36 | OLD PCM SAW | Retired PCM compatibility |
| 37 | OLD PCM FM | Retired PCM compatibility |
| 38 | OLD PCM AM | Retired PCM compatibility |
| 39 | NES FIFTH | NES |
| 40 | NES OCT ARP | NES |
| 41 | SEGA BASS | SN76489 |
| 42 | ATARI SAW | YM2149 |
| 43 | ATARI ARP | YM2149 |
| 44 | C64 TRI | SID |
| 45 | C64 HOLLOW | SID |
| 46 | PCE BASS | PCE/WT |
| 47 | PCE ARP | PCE/WT |
| 48 | PCE BELL | PCE/WT |
| 49 | POKEY BUZZ | POKEY |
| 50 | YM 5B BASS | YM2149 / 5B variant |
| 51 | X TIA HAT | Experimental TIA percussion |
| 52 | X TIA METAL | Experimental TIA percussion |
| 53 | WT4 ARP | PCE/WT four-bit bank |
| 54 | X FDS BRASS | Experimental FDS |
| 55 | X FDS KEYS | Experimental FDS |
| 56 | X FDS ARP | Experimental FDS |
| 57 | OLD PCM BASS | Retired PCM compatibility |
| 58 | OLD PCM CHORD | Retired PCM compatibility |
| 59 | OLD PCM BELL | Retired PCM compatibility |
| 60 | NES SOLO | NES |
| 61 | NES TRI | NES |
| 62 | GB WAVE SOLO | GB |
| 63 | GB NOISE | GB |
| 64 | PSG METAL | SN76489 |
| 65 | YM ENV SOLO | YM2149 |
| 66 | SID SLOW PWM | SID |
| 67 | PCE WAVE | PCE/WT |
| 68 | WT4 SOLO | PCE/WT four-bit bank |
| 69 | POKEY POLY | POKEY |
| 70 | VRC6 SAW | VRC6 |
| 71 | NES SYNC ARP | NES |
| 72 | PCE SYNC ARP | PCE/WT |
| 73 | X FDS TABLE | Experimental FDS |
