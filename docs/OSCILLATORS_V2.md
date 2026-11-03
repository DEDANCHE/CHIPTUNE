# Oscillator revision 2

Built and host-tested on 2026-11-03: 309 renderer checks and 461 adapter checks passed.
The NTS-1 mkII unit builds with ARM GCC 14.2.1 without warnings. The packaged binary is
42,244 bytes; ELF sections are text 37,963, data 3,028 and bss 6,444 bytes.
Listening tests and CPU profiling on an actual NTS-1 remain pending.

## Lineup

The main CHIP menu contains eight families, with their original numeric IDs unchanged:

| ID | Family | COLOR | Main additions |
|---|---|---|---|
| 0 | NES | Four pulse duty zones | Pulse solos, isolated triangle and noise, independent channel gains |
| 1 | GB | Eight four-bit wave tables | Wave-only operation and wave balance choices; pulse duty follows pairs of wave zones |
| 2 | SN76489 | Voice-2 balance; upper half selects periodic noise | Square/noise solos and pitched periodic noise |
| 3 | YM2149 | Eight repeating envelope shapes | Pure envelope modes, tone/noise gating, Sunsoft clock variant |
| 4 | SID | Four waveform zones; within a zone PWM depth/rate rise together | Sync step correction, PWM/filter smoothing, voice/noise solos |
| 5 | PCE/WT | Four five-bit waves followed by four short four-bit waves | N163 wave bank, wave-only operation, zero-depth FM |
| 6 | POKEY | Four distortion zones | Distortion/noise solos and high-pass voicing |
| 7 | VRC6 | Eight pulse widths, 1..8 high steps out of 16 | Saw-only operation and channel balance |

IDs 8..12 remain reserved in the core and are accepted by the unit's callback adapter:

- **8 (5B)** becomes the YM2149 engine's Sunsoft clock/divider variant. Select a YM 5B preset to retain that variant in a v2 patch.
- **9 (TIA)** remains an experimental engine. Its curated presets are now noise percussion, prefixed `X TIA`.
- **10 (N163)** becomes the PCE/WT short four-bit wave bank. It is not a cycle-exact N163 emulator.
- **11 (FDS)** remains experimental, prefixed `X FDS`, with a revised table modulator.
- **12 (Paula)** remains reachable by the original preset IDs, prefixed `OLD PCM`. The generated-wave engine is retired from the main menu. A new signed-PCM sample instrument is deferred; it is not presented as part of this release.

Selecting a legacy preset displays its parent family in CHIP. Replaying that same parent
does not discard the preset's experimental model. To leave it explicitly, select a main-family
preset or select another family first. Legacy engine IDs and preset positions are reserved,
but the sound and raw v1 parameter layout are not guaranteed to be identical.

## Ten controls

| Slot | Control | Values |
|---|---|---|
| 0 | COLOR | 0..1023; chip-specific timbre |
| 1 | DETN/FLT | 0..1023; period detune, SID filter closure, or FM depth |
| 2 | PRESET | 0..73; original indices 0..59 retained, fourteen appended |
| 3 | CHIP | 0..7 primary families |
| 4 | ARP/RATE | Six patterns for each of six rates |
| 5 | VOICE | Original sub/special modes, solos, balance choices, pure YM envelopes |
| 6 | NOISE | Off, low, mid, high, metal, buzz |
| 7 | DECAY | 0 hold, 1 fast .. 15 slow |
| 8 | TUNE/RES | MUSIC, CHIP, MUS+RES, CHP+RES |
| 9 | INTVL | Unison, minor third, major third, fourth, fifth, octave |

Vibrato remains a preset property; COLOR replaces its dedicated knob. SID's COLOR macro
couples PWM depth and rate to fit the ten-slot interface. The portable core also exposes
separate `pwmDepth`, `pwmRate`, `pulse1Level`, `pulse2Level`, `waveLevel`, and `noiseLevel` fields.

VOICE 0..4 preserve the original off/sub/unison/special voicings. Values 5..8 select P1,
P2, wave, or noise alone. PCE/FDS/SID use their main wave voice for wave-only mode;
NES/GB/VRC6 use the triangle/wave/saw layer. Values 9..11 balance P2 at 25/50/75 percent;
12..14 add a unison wave layer at 25/50/75 percent; 15..17 balance noise at those levels.
Values 18/19 are pure YM envelope modes. Models without a noise or wave channel cannot
produce that unavailable source. Four-bit GB tables remain amplitude-stepped.

ARP/RATE uses `pattern + 6 * rate`. Patterns are OFF/MAJ/MIN/OCT/5TH/SUS4. Rates are
60 Hz tracker, quarter notes, eighth notes, sixteenths, thirty-seconds, and 30 Hz tracker.
`setTempo` sets the internal tempo (default 120 BPM); 4-PPQN callbacks align the musical
arpeggio phase. Fixed tracker rates ignore tempo. Envelope and vibrato control remain at 240 Hz.

MUSIC keeps the requested fundamental across original register limits, with fractional
periods and an upper frequency bound. CHIP retains register rounding and octave folding.
SID has fine phase-accumulator pitch in both modes; RES independently enables its resonant filter.
Period detune remains audible in MUSIC mode when deliberately requested.

## Engine work

Muted channels stop generating events and their stale levels are excluded from mixing.
Zero-depth PCE/FDS modulation stops the modulator. Detune and external modulation settle
at the control rate; SID cutoff, resonance, bypass, PWM depth and PWM rate are smoothed.
FM period changes preserve remaining phase, including recovery from a stopped FDS carrier.

SID sync uses the event engine's existing 16-tap kernel to correct slave wrap, pulse-edge,
and master-reset steps. A common eight-oversample delay aligns the raw signal and correction;
the existing 2x engine and 31-tap decimator remain. This targets step discontinuities;
triangle slope changes, moving-PWM edge cases and approximate ring modulation still need listening
and spectral validation. No new alias-rejection figure is claimed.

FDS now uses four 32-entry three-bit adjustment tables, repeated entries over 64 clocks,
a wrapping signed seven-bit modulation counter, six-bit modulation gain, FDS-style rounding,
and carrier/modulator pitch registers. It remains a musical approximation: selectable ROM-like
tables and a shared envelope replace a full register interface and hardware envelope units.
The modulation behavior was checked against the technical descriptions in
[NESdev](https://www.nesdev.org/wiki/FDS_audio) and the independent
[NSFPlay implementation](https://github.com/bbbradsmith/nsfplay/blob/master/xgm/devices/Sound/nes_fds.cpp).

## Migration and validation

This is a major version change (`2.0.0`). A v1 patch's slot 0 was VIB; it is COLOR now.
The old SID QUANT switch also has different meaning from the new TUNE/RES field.
Do not replay a v1 raw parameter array as a v2 patch. Reselect its retained preset ID,
restore intended timbre/voice edits using the new controls, and save again. Stale values
written by the old adapter cannot be recovered automatically. Custom legacy-clock edits
should be recreated from the corresponding YM 5B preset.

Executed host regression cases cover recall order, legacy-family mapping, COLOR, voice solos,
low-note tuning, zero-depth FM, tempo behavior, reset, release and finite output at extreme settings.
All 770 checks passed. The rebuilt `dist/chipapu.nts1mkiiunit` contains this revision.
CPU headroom, listening quality and hardware compatibility remain unmeasured on the NTS-1.
Hardware profiling is required before increasing oversampling or promoting FDS to the main selector.
