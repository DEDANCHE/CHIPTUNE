# Design exploration: APU-style oscillator for NTS-1 mkII

## Platform constraints (from the logue SDK `nts-1_mkii` README)

- Cortex-M7 (STM32H725), hard-float; units are ELF shared objects built with gcc-arm-none-eabi 10.3.
- `osc` units: 16 slots, ~48 KB unit size / 48 KB RAM load, **no allocatable external memory**.
- At most 10 parameters: 2 knobs (A/B) + 8 edit-menu entries.
- Oscillator is **monophonic**: one note, pitch delivered per render as `w0`; input stereo, output mono.
- Units are not binary compatible with the NTS-1 mkI (Cortex-M4); sources are portable.

Consequence: a chip emulator must be tiny (no sample ROM, no big tables) and treat the
chip as *one voice made of several channels*.

## Engine alternatives considered

| Engine | Idea | Pros | Cons |
|--------|------|------|------|
| Naive stepped waveforms | Phase accumulator + 4-bit tables | Trivial | Heavy aliasing at the NTS pitch range |
| PolyBLEP/wavetable pulse | Classic analog-style osc with chip waveforms | Cheap, clean | Not chip-like: no register quantisation, nonlinear mixing, LFSR noise |
| **Event-driven APU + band-limited steps (chosen)** | Channels run in the chip clock domain; each output change is a windowed-sinc step at its fractional sample time | Authentic timing/quantisation/mixing, alias-free, cost scales with transitions | Heavier per-sample than a phase accumulator; needs a small kernel table (4 KB) |
| Cycle-exact chip emulation + decimation | Step the chip every clock (1.79 MHz for NES) | Perfect | ~37 chip clocks per output sample per channel; wasteful versus events |

## Chosen architecture (`src/apu_core.h`)

```
note (w0, velocity) ─► Apu::control tick @240 Hz: envelope, 60 Hz arp, vibrato, period registers
                         │
   pulse1 ─┐             ▼
   pulse2 ─┼─► chip mixer (NES nonlinear / GB linear) ─► level change? ─► BlipBuf.addStep(t, Δ)
   tri/wave┤                                                            ─► integrate ─► DC trim ─► out
   noise  ─┘ (15/7-bit LFSR)
```

- **Period registers**: pitch is converted to the chip's 11-bit register (`QUANT` on), so high notes detune
  the way real hardware does; `DETN` offsets pulse 2 in register steps for the classic phase-beating chorus.
  Registers that overflow fold up by octaves so low notes stay in tune.
- **Mixers**: NES uses the documented non-linear pulse and triangle/noise formulas; GB is linear.
- **Control rate**: 240 Hz tick drives a decay envelope (NES-style 15→0 steps), 60 Hz tracker arpeggio and delayed vibrato.
- **Mono mapping**: the held note drives pulse 1; pulse 2 follows at `INTVL`; `SUB` plays the triangle/wave channel
  at −1 oct or unison; `NOISE` adds the LFSR channel (short-mode "metal"/"buzz" settings).

### Parameter map (10 slots)

| # | Name | Meaning |
|---|------|---------|
| 0 (knob A) | PRESET | 0..32, loads `chiptune::preset(i)` (all parameters, including the duty field: pulse duty zone 12.5/25/50/75 %; GB / PCE: wave table; SID: waveform; POKEY / TIA: distortion) |
| 1 (knob B) | DETN/FLT | pulse 2 detune, 0..16 register steps; SID: filter cutoff (open..closed); SHAPE_LFO adds to it |
| 2 | CHIP | NES / GB / SEGA (SN76489) / ATARI (YM2149) / SID (C64) / PCE (HuC6280) / POKEY / VRC6 / 5B / TIA |
| 3 | ARP | off, maj, min, oct, 5th+oct, sus4 |
| 4 | SUB | off, −1 oct, unison, then per chip: ATARI / 5B envelope buzz saw / triangle; SID hard sync / ring mod; PCE LFO FM; POKEY high-pass |
| 5 | NOISE | off, low, mid, high, metal, buzz |
| 6 | DECAY | 0 hold, 1–15 envelope speed |
| 7 | QUANT | register quantisation on/off; SID: filter resonance on/off |
| 8 | INTVL | pulse 2 interval: unison, m3, M3, P4, P5, oct |
| 9 | VIB | vibrato depth 0–15 |

## Chip models

| CHIP | Chip | Engine | Notes |
|------|------|--------|-------|
| NES | 2A03 | event-driven | 2 pulses, triangle, LFSR noise, non-linear mixer |
| GB | LR35902 APU | event-driven | 2 pulses, 4-bit wave RAM, linear mixer |
| SEGA | SN76489 | event-driven | 10-bit dividers, 2 dB attenuation, white / periodic noise (setting 5 is pitched by the note) |
| ATARI | YM2149 (Atari ST) | event-driven | 12-bit dividers, 3 dB steps, 17-bit LFSR noise, envelope "buzzer" via `SUB` = BZSAW / BZTRI, tuned to the note |
| PCE | HuC6280 (PC Engine) | event-driven | Three 32-sample 5-bit wave channels (sine / saw / 25 % pulse / triangle; channel 2 uses the next table), 15-bit LFSR noise, 3 dB volume steps |
| POKEY | Atari 8-bit POKEY | event-driven | 8-bit dividers on the 64 kHz base clock, 16-bit pair on 1.79 MHz for low notes, distortion via the preset duty field; voice 3 stays pure; 17-bit poly noise |
| VRC6 | Konami VRC6 | event-driven | 2 pulses with 8 duty steps (4 zones via the preset duty field), 7-step saw as the `SUB` channel, no noise |
| 5B | Sunsoft 5B (YM2149 variant) | event-driven | Same core as ATARI at the 1.79 MHz NES clock, including the envelope buzzer |
| TIA | Atari 2600 TIA | event-driven | 5-bit divider on a 114-clock base, 4-bit / 5-bit / 9-bit poly distortion via the preset duty field; registers above 32 fold up by octaves, so notes below about 1 kHz are coarsely tuned |
| SID | MOS 6581/8580 (C64) | continuous | Saw / triangle / pulse (auto PWM) voices, 23-bit LFSR noise, ADSR-style envelope, state-variable low-pass |

C64 and SID are the same chip, and the Atari ST uses the YM2149, so five models cover the first four names requested; PC Engine and POKEY were added afterwards.
SID is a different engine path because its voices are continuous analog-style oscillators: 24-bit phase
accumulators with polyBLEP edges, run at 2x the sample rate and decimated by a 31-tap FIR. Unlike the others its
note keeps sounding through an exponential release after note-off.

PCE and POKEY notes: the SN76489 was already covered by the SEGA model. The POKEY poly counters step once per divider fire instead of at the 1.79 MHz base clock, and each mode retunes the divider so the pattern repeats at the note pitch (the 4-bit and 5-bit modes would otherwise sound far below the note). PCE `SUB` = FM: channel 2 frequency-modulates channel 1 (the HuC6280 LFO idea; depth follows `DETN`). POKEY `SUB` = HPF: voice 3 acts as the sampling clock for a high-pass latch on voice 1. Not implemented: PCE direct-DAC mode and per-channel balance, POKEY 15 kHz base clock and channel linking.

SID `SUB` = SYNC: voice 2 restarts on every wrap of voice 1 (slave ratio sweeps with the PWM LFO); `SUB` = RING: voice 1 triangle multiplied by the MSB of voice 2. The sync reset is not band-limited and ring mod is approximate. Not implemented for SID: band-pass/high-pass modes, per-voice routing.
The 8-bit noise and combined waveforms are approximations rather than the real DAC curves.

## Measured so far (host, 48 kHz)

- NES 50 % pulse at 3150 Hz: folded 9th-harmonic alias at **-62 dB** relative to the fundamental (a naive pulse is about -19 dB).
- SEGA 3150 Hz square: **-60.6 dB**. SID 3150 Hz saw: **-75 dB** (a first 11-tap decimator only reached -34 dB; the 31-tap one fixes it).
- 440 Hz pitch: fundamental at least 38x the neighbouring bin on every chip; the YM2149 buzzer at 220 Hz is 47x.
- SID low-pass (`cutoff` 0.6) cuts 6.6 kHz by more than 40 dB relative to open.
- PCE saw: alias at 19.65 kHz **-61 dB**; POKEY pure square: **-56 dB**. POKEY 82 Hz uses the 16-bit pair (30x over the neighbouring bin); the 4-bit poly mode holds pitch at 39x.
- VRC6 pulse alias **-63 dB**; 5B pitch 76x over the neighbouring bin. SID sync, ring, PCE FM and POKEY HPF each differ audibly from the plain voicing and release to silence.
- Fuzz (12 chip values incl. 2 out of range, extreme frequencies, wild parameters, SHAPE_LFO ±1): output stays finite within +-1 (peak 0.98 after the make-up gain).
- Note-off returns to silence on all chips (SID after its release tail).
- Level: the chips mix at hardware-relative levels, which left the NES / GB / PCE voicings near -23 dBFS RMS. A per-chip make-up gain (+3 to +12 dB) and a soft-knee limiter (knee 0.75) now put a plain voicing at about -9 to -13 dBFS RMS, with attack peaks of -6 to -2 dBFS on a single note; presets peak at 0.44 to 0.97.

## Modulation and presets

- `SHAPE_LFO`: SID filter cutoff += 0.5 × LFO; on event chips it modulates pulse 2 detune by up to ±8 register steps.
- Pitch bend: ±2 semitones, applied in `process`.
- `src/presets.h` holds 33 reference presets (`chiptune::preset(i)`); knob A (`PRESET`) selects one, see [docs/PRESETS.md](PRESETS.md).

## Other APUs to add later (same primitives)

| Chip | What it needs beyond the core |
|------|------------------------------|
| Namco N163, FM chips (YM2612, OPLL, VRC7) | Wave RAM channels; FM operators need a continuous path like SID |
| PCE / POKEY extras | HuC6280 6th channel, POKEY channel linking |
| SID extras | polyBLEP at the sync discontinuity, real DAC/combined waveform tables |
## Open questions / next steps

1. ~~Build with the SDK~~ Done with `tools/build_unit.ps1` (gcc 14.2.1): text 27.0 KB, data 1.1 KB, bss 5.4 KB, all well under 48 KB. Still to do: build with the SDK Docker image (gcc 10.3) and load on hardware.
2. Measure CPU on hardware; worst case is the high-pitched noise/pulse event rate. Candidates: smaller kernel (8 taps), fewer phases.
3. Verify whether static constructors run in loaded units; glue uses lazy init to be safe.
4. Verify the assumed ranges: pitch bend is treated as 14-bit centred on 8192 (±2 semitones), `SHAPE_LFO` as −1..1. Audio inputs are ignored; all ten parameter slots are used, so there is no switch for them.
5. Pick a developer ID (`dev_id` in `header.c`) before distribution.
