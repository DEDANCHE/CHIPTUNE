# Reference presets

Knob A (PRESET, 0..32) loads these on the unit; the values below are what each preset sets.
They are also defined in `src/presets.h` (`chiptune::preset(i)`), and the host test renders each
one to `build/preset_<i>.wav`. Unlisted parameters are at their defaults (0 / off).

A preset overwrites the whole parameter set, duty included (there is no separate DUTY knob). The edit
menu is not refreshed, so it can show stale values after a preset change; moving a menu item or
knob B afterwards edits that one parameter on top of the preset. Menu values that still equal the
last value seen are ignored, so a patch reloaded with a preset keeps the preset's sound.

Raw values: DUTY 0..3, DETN in register steps (knob B 0..1023 maps to 0..16), CHIP 0..9
(NES, GB, SEGA, ATARI, SID, PCE, POKEY, VRC6, 5B, TIA). For SID, knob B is the filter cutoff
(0..1 of the knob range) and QUANT is resonance.

| # | Name | CHIP | DUTY | SUB | Other |
|---|------|------|------|-----|-------|
| 0 | NES LEAD | NES | 1 | off | VIB 6, DETN 2 |
| 1 | NES ARP | NES | 2 | off | ARP MAJ, DECAY 4 |
| 2 | NES BASS | NES | 3 | -1OCT | |
| 3 | GB WAVE | GB | 2 | -1OCT | INTVL 5TH |
| 4 | SEGA BUZZ | SEGA | 0 | -1OCT | NOISE BUZZ, DECAY 6 |
| 5 | ATARI BUZZ | ATARI | 0 | BZSAW | NOISE HIGH, DECAY 8 |
| 6 | C64 PWM | SID | 2 | -1OCT | cutoff 0.4, resonance on |
| 7 | C64 SYNC | SID | 1 | SYNC | cutoff 0.2 |
| 8 | C64 RING | SID | 0 | RING | INTVL 5TH |
| 9 | PCE FM | PCE | 0 | FM | INTVL OCT, DETN 6 |
| 10 | POKEY BASS | POKEY | 1 | -1OCT | |
| 11 | POKEY HPF | POKEY | 0 | HPF | DETN 4 |
| 12 | VRC6 LEAD | VRC6 | 0 | -1OCT (saw) | VIB 5 |
| 13 | 5B BUZZ | 5B | 0 | BZTRI | NOISE LOW, DECAY 4 |
| 14 | TIA BASS | TIA | 1 | -1OCT | play above about 1 kHz for accurate pitch |
| 15 | NES CHORUS | NES | 2 | off | DETN 4 |
| 16 | NES SNARE | NES | 2 | off | NOISE HIGH, DECAY 2 |
| 17 | NES MINOR | NES | 0 | off | ARP MIN |
| 18 | GB SUB BASS | GB | 0 | -1OCT | |
| 19 | GB HIHAT | GB | 3 | off | NOISE MID, DECAY 6 |
| 20 | SEGA LEAD | SEGA | 0 | off | INTVL 5TH, VIB 4, DETN 2 |
| 21 | SEGA SNARE | SEGA | 0 | off | NOISE HIGH, DECAY 3 |
| 22 | ATARI LEAD | ATARI | 0 | UNI | DETN 3, VIB 4 |
| 23 | ATARI TRI | ATARI | 0 | BZTRI | |
| 24 | C64 BASS | SID | 1 | -1OCT | cutoff 0.5, resonance on |
| 25 | C64 NOISE | SID | 2 | off | NOISE MID, DECAY 6, cutoff 0.3 |
| 26 | C64 ARP | SID | 2 | UNI | ARP MAJ, cutoff 0.2 |
| 27 | PCE ORGAN | PCE | 0 | off | INTVL OCT, DETN 2 |
| 28 | POKEY LEAD | POKEY | 0 | off | DETN 2, VIB 4 |
| 29 | POKEY NOISE | POKEY | 2 | off | NOISE MID, DECAY 5 |
| 30 | VRC6 ARP | VRC6 | 1 | -1OCT (saw) | ARP MAJ |
| 31 | 5B LEAD | 5B | 0 | UNI | DETN 2, VIB 4 |
| 32 | TIA PERC | TIA | 3 | off | DECAY 4 |
The SUB names depend on the CHIP: the menu shows SYNC / RING for SID, FM for PCE, HPF for POKEY and
BZSAW / BZTRI for ATARI and 5B.
