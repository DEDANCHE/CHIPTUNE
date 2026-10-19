# Reference presets

The NTS-1 mkII unit API has no preset slot, so these are parameter values to dial in by hand.
They are also defined in `src/presets.h` (`chiptune::preset(i)`), and the host test renders each
one to `build/preset_<i>.wav`. Unlisted parameters are at their defaults (0 / off).

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

The SUB names depend on the CHIP: the menu shows SYNC / RING for SID, FM for PCE, HPF for POKEY and
BZSAW / BZTRI for ATARI and 5B.
