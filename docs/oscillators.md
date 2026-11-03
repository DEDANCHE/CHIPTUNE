# Oscillators

Current controls and engine behavior for version 2.1.0. See [validation](#validation)
for build results and remaining hardware checks.

## Lineup

The CHIP menu starts with eight primary families, with their original numeric IDs unchanged:

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

Version 2.0.1 exposes IDs 8..12 as explicit compatibility entries so saved patches
retain their engine and clock variant:

- **8 (YM 5B)** selects the YM2149 engine's Sunsoft clock/divider variant.
- **9 (TIA)** remains an experimental engine. Its curated presets are now noise percussion, prefixed `X TIA`.
- **10 (N163)** becomes the PCE/WT short four-bit wave bank. It is not a cycle-exact N163 emulator.
- **11 (FDS)** remains experimental, prefixed `X FDS`, with a revised table modulator.
- **12 (OLD PCM)** selects the legacy generated-wave Paula model, also reachable through its original presets. A new signed-PCM sample instrument is deferred; it is not presented as part of this release.

CHIP saves the actual engine: 8 for YM 5B, 9 for TIA, 11 for FDS, and 12 for OLD PCM.
WT4 uses the PCE engine (saved as 5) with COLOR selecting its four-bit bank. Selecting
a primary family directly replaces a legacy model, even when it belongs to that family.
The original preset ID still supplies hidden settings such as vibrato; an explicit CHIP
override takes precedence during recall. Legacy preset positions remain unchanged.

## Ten controls

| Slot | Control | Values |
|---|---|---|
| 0 | COLOR | 0..1023; chip-specific timbre |
| 1 | DETN/FLT | 0..1023; detune, SID filter, FM depth, solo vibrato, or noise rate |
| 2 | PRESET | 0..76; original indices 0..73 retained, three appended |
| 3 | CHIP | 0..7 primary families; 8..12 compatibility entries |
| 4 | ARP/RATE | Six patterns for each of six rates |
| 5 | VOICE | 0..22: layers, solos, balances, YM envelopes, kick/tom, slow PWM |
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
Values 18/19 select pure YM saw/triangle envelopes independently of COLOR; layered
buzz modes retain all eight COLOR shapes. Models without a noise or wave channel cannot
produce that unavailable source. Four-bit GB tables remain amplitude-stepped.

VOICE 20/21 select kick/tom articulation with a falling pitch envelope of 24/12
semitones respectively. NES/GB/VRC6 use their wave layer; other families use P1.
DECAY still controls amplitude. VOICE 22 selects slow/deep SID PWM (0.25 Hz, depth
0.35), independently of COLOR's coupled speed/depth. COLOR still selects the waveform;
choose its pulse zone for PWM. This articulation has no special effect on other chips.

On non-SID P1/wave solos without FM, DETN adds up to half a semitone of delayed
vibrato. On event-engine noise solos, it increases the noise clock up to 16x,
bounded by the engine's minimum period. SID retains filter control. These mappings
make previously inactive knobs useful without adding a parameter slot.

MIDI channel pressure and poly pressure for the active note are combined using
their maximum value. Pressure opens SID's filter or adds register/FM/noise/solo
modulation elsewhere, through the smoothed SHAPE modulation path. With no pressure
messages there is no added expression. Reset clears pressure; releasing a note
clears its poly pressure. Channel pressure persists across notes until changed.

Engine, voice and COLOR-zone edits blend from the previous output for 48 samples
(1 ms). This bounds the first discontinuity without allocating a second engine.
It is not a full dual-engine tail crossfade. Clock callbacks arriving after a pending
note-on are applied after the voice starts and before the first output sample.

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

## Migration

Revision 2 is a major version change; the current package is `2.1.0`. A v1 patch's slot 0 was VIB; it is COLOR now.
The old SID QUANT switch also has different meaning from the new TUNE/RES field.
Do not replay a v1 raw parameter array as a v2 patch. Reselect its retained preset ID,
restore intended timbre/voice edits using the new controls, and save again. Stale values
written by the old adapter cannot be recovered automatically.

Version 2.0.0 stored only the parent family for experimental engines. When loading an
older legacy patch, reselect its actual CHIP entry (YM 5B, X TIA, X FDS, or OLD PCM)
and save again. The old parameter array cannot distinguish a retained experimental
engine from one deliberately replaced by its parent family.

Version 2.0.2 corrects preset 21's duplicated envelope into a triangle (COLOR 256);
preset 42 retains the saw. Names now describe sustained harmonic/FM sounds, mixed
noise voices and gentle PWM accurately. All IDs remain unchanged. A saved patch
that explicitly stores the old COLOR value keeps that edit; reselect preset 21 to
load the corrected default. See [preset review](preset-review.md) for the changes
and the remaining audition-dependent proposals.

Version 2.1.0 keeps preset IDs 0..73 and appends 74..76. Original preset defaults
were sample-identical at 220 Hz for 0.5 seconds in a comparison against v2.0.2.
Saved edits with nonzero DETN on previously inactive solo/noise voices can now
sound different: set DETN to zero to retain the former unmodulated solo behavior.
Pressure messages now affect synthesis. Pure YM envelopes are normalized to P1
solo when switching away from YM, preventing hidden FM state from being lost on
recall. Engine and timbre transitions use the new short blend.

## Validation

Built and host-tested on 2026-11-03: 341 core audio, 766 adapter, 613 feature, and
95 runtime callback checks passed with undefined-behavior sanitization.
The NTS-1 mkII unit builds with ARM GCC 14.2.1 without warnings. The packaged binary is
37,688 bytes; ELF sections are text 33,487, data 2,944 and bss 6,472 bytes. The actual
PT_LOAD span is 43,124 bytes, leaving 6,028 bytes below the 48 KiB load limit.
All 17 required callbacks remain exported. The release checksum and dependency
revision are recorded in `dist/manifest.json`.

Executed host regression cases cover recall order and audio identity, explicit legacy
engine selection, COLOR, voice solos, low-note tuning, zero-depth FM, tempo behavior,
reset, release, held-note engine transitions, pitch at note attack, distinct YM envelope
modes, and finite output at extreme settings. Feature tests add pressure, pitch
envelopes, transition bounds, and every voice/chip combination in both recall orders.
Note-on starts after the next render supplies pitch; engine changes rebuild
channel timers without requiring a new note.
All 1,815 checks passed. The rebuilt `dist/chipapu.nts1mkiiunit` contains this revision.
The instrumented profiling build also passes memory/export checks; host profiling
and raw/matched audition generation were exercised. See [development](development.md)
for the reproducible commands and hardware measurement procedure.
CPU headroom, listening quality and hardware compatibility remain unmeasured on the NTS-1.
Hardware profiling is required before increasing oversampling or promoting FDS to the main selector.
