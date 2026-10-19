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
| 0 (knob A) | DUTY | pulse duty zone 12.5/25/50/75 %; GB: wave table |
| 1 (knob B) | DETN | pulse 2 detune, 0..16 register steps |
| 2 | CHIP | NES / GB |
| 3 | ARP | off, maj, min, oct, 5th+oct, sus4 |
| 4 | SUB | off, −1 oct, unison |
| 5 | NOISE | off, low, mid, high, metal, buzz |
| 6 | DECAY | 0 hold, 1–15 envelope speed |
| 7 | QUANT | register quantisation on/off |
| 8 | INTVL | pulse 2 interval: unison, m3, M3, P4, P5, oct |
| 9 | VIB | vibrato depth 0–15 |

## Measured so far (host, 48 kHz)

- 50 % pulse at 3150 Hz: folded 9th-harmonic alias at **−62 dB** relative to the fundamental (a naive pulse is about −19 dB).
- 440 Hz pitch: fundamental ≥ 77× the neighbouring bin.
- Note-off returns to silence (initial DC-blocker version undershot by −0.36 and drooped; replaced by an exact integrator plus a gate-aware DC trim).

## Other APUs to add later (same primitives)

| Chip | What it needs beyond the core |
|------|------------------------------|
| SN76489 (Master System) | 3 squares with 10-bit dividers + white/periodic noise, 2 dB attenuators |
| PC Engine (HuC6280) | 6 × 32-sample 5-bit wave channels, LFO, noise on 2 channels |
| POKEY (Atari) | Poly-counter distortion modes, 16-bit pairing, high-pass filter |
| SID | Analog filter and ring-mod; needs a different (continuous) engine path |

## Open questions / next steps

1. Build with the SDK Docker image; check unit size (<48 KB) and RAM (<48 KB; kernel table is ~4 KB, built lazily on first use).
2. Measure CPU on hardware; worst case is the high-pitched noise/pulse event rate. Candidates: smaller kernel (8 taps), fewer phases.
3. Verify whether static constructors run in loaded units; glue uses lazy init to be safe.
4. Decide on use of `SHAPE_LFO`, pitch bend and the audio inputs (currently ignored).
5. Pick a developer ID (`dev_id` in `header.c`) before distribution.
