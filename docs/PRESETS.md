# Reference presets

The PRESET menu item (0..59, just above CHIP; knob A is VIB, knob B is DETN/FLT) loads these on the unit; the values below are what each preset sets.
They are also defined in `src/presets.h` (`chiptune::preset(i)`), and the host test renders each
one to `build/preset_<i>.wav`. Unlisted parameters are at their defaults (0 / off).

A preset overwrites the whole parameter set, duty included (there is no separate DUTY knob). The edit
menu is not refreshed, so it can show stale values after a preset change; moving a menu item,
knob A or knob B afterwards edits that one parameter on top of the preset. Menu values that still equal the
last value seen are ignored, so a patch reloaded with a preset keeps the preset's sound.

Raw values: DUTY 0..3, DETN in register steps (knob B 0..1023 maps to 0..16), CHIP 0..12
(NES, GB, SEGA, ATARI, SID, PCE, POKEY, VRC6, 5B, TIA, N163, FDS, PAULA). For SID, knob B is the filter cutoff
(0..1 of the knob range) and QUANT is resonance.

| # | Name | CHIP | DUTY | SUB | Other |
|---|------|------|------|-----|-------|
| 0 | NES LEAD | NES | 1 | off | DETN 2, VIB 6 |
| 1 | NES ARP | NES | 2 | off | ARP MAJ, DECAY 4 |
| 2 | NES BASS | NES | 3 | -1OCT |  |
| 3 | GB WAVE | GB | 2 | -1OCT | INTVL P5 |
| 4 | SEGA BUZZ | SEGA | 0 | -1OCT | NOISE BUZZ, DECAY 6 |
| 5 | ATARI BUZZ | ATARI | 0 | BZSAW | NOISE HIGH, DECAY 8 |
| 6 | C64 PWM | SID | 2 | -1OCT | cutoff 0.4, RES |
| 7 | C64 SYNC | SID | 1 | SYNC | cutoff 0.2 |
| 8 | C64 RING | SID | 0 | RING | INTVL P5 |
| 9 | PCE FM | PCE | 0 | FM | INTVL oct, DETN 6 (FM depth) |
| 10 | POKEY BASS | POKEY | 1 | -1OCT |  |
| 11 | POKEY HPF | POKEY | 0 | HPF | DETN 4 |
| 12 | VRC6 LEAD | VRC6 | 0 | -1OCT | VIB 5 |
| 13 | 5B BUZZ | 5B | 0 | BZTRI | NOISE LOW, DECAY 4 |
| 14 | TIA BASS | TIA | 1 | -1OCT |  |
| 15 | NES CHORUS | NES | 2 | off | DETN 4 |
| 16 | NES SNARE | NES | 2 | off | NOISE HIGH, DECAY 2 |
| 17 | NES MINOR | NES | 0 | off | ARP MIN |
| 18 | GB HIHAT | GB | 3 | off | NOISE MID, DECAY 6 |
| 19 | SEGA LEAD | SEGA | 0 | off | INTVL P5, DETN 2, VIB 4 |
| 20 | ATARI LEAD | ATARI | 0 | UNI | DETN 3, VIB 4 |
| 21 | ATARI TRI | ATARI | 0 | BZTRI |  |
| 22 | C64 BASS | SID | 1 | -1OCT | cutoff 0.5, RES |
| 23 | C64 NOISE | SID | 2 | off | NOISE MID, DECAY 6, cutoff 0.3 |
| 24 | C64 ARP | SID | 2 | UNI | ARP MAJ, cutoff 0.2 |
| 25 | PCE ORGAN | PCE | 0 | off | INTVL oct, DETN 2 |
| 26 | POKEY LEAD | POKEY | 0 | off | DETN 2, VIB 4 |
| 27 | VRC6 ARP | VRC6 | 1 | -1OCT | ARP MAJ |
| 28 | TIA PERC | TIA | 3 | off | DECAY 4 |
| 29 | N163 BELL | N163 | 0 | off | INTVL P5, DETN 2 |
| 30 | N163 SAW | N163 | 1 | off | VIB 5 |
| 31 | N163 ORGAN | N163 | 2 | -1OCT |  |
| 32 | N163 BUZZ | N163 | 3 | off | DETN 3 |
| 33 | FDS BELL | FDS | 2 | off | INTVL oct, DETN 4 (FM depth) |
| 34 | FDS BASS | FDS | 0 | -1OCT | DETN 10 (FM depth) |
| 35 | FDS WOBBLE | FDS | 3 | off | INTVL M3, DETN 12 (FM depth), VIB 4 |
| 36 | AMIGA SAW | PAULA | 0 | UNI | DETN 2 |
| 37 | AMIGA FM | PAULA | 2 | FM | INTVL P5, DETN 8 (FM depth) |
| 38 | AMIGA AM | PAULA | 3 | AM | INTVL oct |
| 39 | NES FIFTH | NES | 2 | off | INTVL P5 |
| 40 | NES OCT ARP | NES | 1 | off | ARP OCT |
| 41 | SEGA BASS | SEGA | 0 | -1OCT | INTVL oct |
| 42 | ATARI SAW | ATARI | 0 | BZSAW |  |
| 43 | ATARI ARP | ATARI | 0 | UNI | ARP SUS4, DETN 2 |
| 44 | C64 TRI | SID | 0 | off | VIB 4, cutoff 0.35 |
| 45 | C64 HOLLOW | SID | 2 | off | cutoff 0.55, RES |
| 46 | PCE BASS | PCE | 1 | -1OCT |  |
| 47 | PCE ARP | PCE | 3 | off | ARP MAJ |
| 48 | PCE BELL | PCE | 0 | FM | INTVL P5, DETN 12 (FM depth) |
| 49 | POKEY BUZZ | POKEY | 3 | -1OCT |  |
| 50 | 5B BASS | 5B | 0 | -1OCT |  |
| 51 | TIA LEAD | TIA | 0 | off |  |
| 52 | TIA ARP | TIA | 2 | off | ARP MAJ |
| 53 | N163 ARP | N163 | 2 | off | ARP MIN, DECAY 3 |
| 54 | FDS BRASS | FDS | 1 | off | DETN 6 (FM depth) |
| 55 | FDS KEYS | FDS | 0 | off | INTVL P4, DETN 5 (FM depth), DECAY 5 |
| 56 | FDS ARP | FDS | 2 | off | ARP MAJ, INTVL oct, DETN 3 (FM depth) |
| 57 | AMIGA BASS | PAULA | 0 | -1OCT |  |
| 58 | AMIGA CHORD | PAULA | 2 | UNI | ARP MIN |
| 59 | AMIGA BELL | PAULA | 2 | off | INTVL P5, DECAY 6 |

Six earlier presets that sounded within about 6 dB (log-spectrum distance) of another were removed (GB SUB BASS, SEGA SNARE, POKEY NOISE, 5B LEAD, FDS SAW, AMIGA PWM), so numbers above 17 shifted. The closest remaining pair is about 6.6 dB apart.

The SUB names depend on the CHIP: the menu shows SYNC / RING for SID, FM for PCE, HPF for POKEY, FM / AM for PAULA and
BZSAW / BZTRI for ATARI and 5B.
