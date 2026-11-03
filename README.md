# CHIPTUNE

An APU-style chiptune oscillator for the **Korg NTS-1 digital kit mkII**, built with
the logue SDK. Version 2.1.0 includes eight primary chip families and explicit
compatibility entries for legacy engines. Hardware listening and CPU profiling
remain pending.

## Installation

Load [dist/chipapu.nts1mkiiunit](dist/chipapu.nts1mkiiunit) using the Korg Kontrol
Editor. This package targets the NTS-1 **mkII**; it is not an NTS-1 mkI unit.

## Controls and presets

- [Oscillators and controls](docs/oscillators.md): chip families, the ten controls,
  voice modes, tuning, and engine details.
- [Preset catalog](docs/presets.md): the available patches and how edits are recalled.

Knob A controls COLOR. Knob B controls detune, SID filter closure, or FM depth,
depending on the selected engine.

## Architecture and review findings

A technical review of the codebase highlights several notable DSP and embedded engineering patterns, along with recent hardening applied:

### DSP and embedded highlights

- **Zero-heap deterministic design**: Memory usage is strictly bounded and static, with no dynamic memory allocations (`malloc`/`new`), guaranteeing deterministic real-time audio thread execution on the ARM Cortex-M7.
- **Dual-engine topology**:
  - *Event Engine* (NES, GB, SN76489, YM2149, PCE, POKEY, VRC6): Simulates registers in their native chip clock domain. Discontinuities are placed as fractional-sample windowed-sinc impulse steps into a 64-sample circular integrator (`BlipBuf`, 16 taps, 64 phases), achieving anti-aliased output without multi-megahertz oversampling.
  - *Analog Engine* (SID 6581/8580): Operates at 2x oversampling (96 kHz) with PolyBLEP anti-aliased oscillators, naive step cancellation for hard sync, a 31-tap decimation FIR filter, and a 2-pole Chamberlin state-variable filter (SVF).
- **Smooth dynamic range control**: A rational soft-knee saturator (`softLimit`) provides $C^1$-continuous limiting above $0.75$, maintaining level consistency across disparate chip architectures without clipping artifacts.
- **Transactional parameter recall**: Korg Kontrol Editor parameter streams are staged through `pendingMask_` and resolved atomically via `resolvedParams()`, preventing intermediate parameter readbacks from corrupting engine models during preset selection.
- **Transient click mitigation**: Switching engines, wavetable banks, or voice modes triggers a 48-sample (1 ms) linear crossfade from the latched previous sample, eliminating DC pops without running dual active engines.
- **Strict ELF footprint control**: The packaged binary occupies 43,412 bytes of the 49,152-byte (48 KiB) PT_LOAD hardware limit (~12% headroom).

### Hardening and fixes applied

- **Registered Developer ID**: Assigned FourCC developer identifier `0x44443732U` (`'DD72'`) in `platform/nts1_mkii/header.c` to prevent identifier collisions in the Korg Kontrol Editor.
- **Deterministic scalar initialization**: Explicit in-class default member initializers added to `Osc` (`w0_`, `pending_`, `pendingMask_`, `presetIndex_`, `bend_`, `ready_`, `notePending_`, `pendingVelocity_`), eliminating undefined behavior risks for non-BSS (stack/heap) instantiations.
- **Thread/ISR safety**: Qualified `pendingMask_` as `volatile` to prevent register caching across audio DMA interrupt and UI callback contexts.
- **Strictly standard modular indexing**: Replaced negative signed bitwise operations in `src/apu_core.h` delay buffers with safe positive modular offsets (`+ 32`), guaranteeing standard-compliant modular arithmetic across all C++ compilers.
- **Sanitized test suite pass**: All 1,820 test assertions across adapter, feature, runtime, and core audio suites pass with undefined-behavior sanitization (`-fsanitize=undefined -fno-sanitize-recover=all`).
- **Curatorial preset volume leveling**: Calibrated channel levels (`pulse1Level`, `pulse2Level`, `waveLevel`) on hot sustained presets (`[2] NES BASS`, `[3] GB WAVE MIX`, `[40] NES OCT ARP`, `[71] NES MAJ 16TH`) by ~-1.7 to -2.0 dB, bringing raw RMS into the -9 to -10 dBFS range and matching the rest of the factory catalog without flattening dynamics into the soft limiter.
- **Dedicated solo noise articulation**: Enabled Knob A (`COLOR`) on noise solo patches (`VOICE` NOISE / 4):
  - On Atari 2600 (`X TIA`): sweeps across the four hardware polynomial distortion modes (pure tone divider, 4-bit poly crunch, 5-bit poly metal, 9-bit poly white noise snare).
  - On NES, Game Boy, and SN76489: values in the upper half (`COLOR >= 512`) engage metallic / periodic short-sequence LFSR noise while the lower half provides authentic white noise.

## Building and testing

The portable core and host tests target C++11. Run the full suite from the
repository root with Zig available on PATH:

```powershell
powershell -File tools/test.ps1
```

The core suite tests synthesis and writes WAV files. The adapter suite uses SDK
stubs and tests parameter recall, note timing, engine changes, and reset behavior.
Feature tests cover all voice/chip recall combinations, pressure and new articulations;
runtime tests exercise the actual unit callbacks. See the [development guide](docs/development.md)
for one-command releases, audition WAVs, host timing, and instrumented hardware builds.
See [validation results](docs/oscillators.md#validation) for test counts, compiler
versions, and package size. Generated files stay under the ignored `build/` folder.

To build the unit on Windows, install ARM GCC, GNU make, and Git for Windows, and
provide a local logue-sdk checkout:

```powershell
$env:LOGUE_SDK = "C:\path\to\logue-sdk"
powershell -File tools/build_unit.ps1
# Output: build/unit/chipapu.nts1mkiiunit
```

The script stages sources from `platform/nts1_mkii/` and `src/` in `build/unit/`,
then runs the SDK makefiles. It uses the SDK's CMSIS checkout when available,
otherwise fetching headers into `build/CMSIS`. The SDK targets ARM GCC 10.3;
the compiler used for the current package is recorded in the validation results.

For a manual or SDK Docker build:

1. Copy `logue-sdk/platform/nts-1_mkii/dummy-osc` into a new project directory,
   retaining its `Makefile` and `config.mk`.
2. Replace `header.c`, `osc.h`, and `unit.cc` with the files from
   `platform/nts1_mkii/`, including `parameters.h` and `profiling.h`. Copy
   `src/apu_core.h` and `src/presets.h` alongside them.
3. Set the project name in `config.mk`, then run `build nts-1_mkii/<project>`
   in the SDK Docker environment.
4. Load the resulting unit with the Korg Kontrol Editor.

The custom `unit.cc` is required for parameter recall. Prepare a checked release with
`powershell -File tools/release.ps1 -Sdk C:\path\to\logue-sdk`; it updates `dist/`
only after tests, the ARM build, ELF memory limits and callback exports pass.

## Migration

For v1 patches, reselect a preset and recreate edits using the v2 control layout.
For v2.0.0 legacy patches, reselect the actual engine in CHIP and save again.
See the [migration notes](docs/oscillators.md#migration) for details.

## Repository layout and history

| Directory | Contents |
|---|---|
| `src/` | Portable synthesis engine and preset definitions |
| `platform/nts1_mkii/` | SDK callbacks, oscillator adapter, and unit metadata |
| `tests/` | Core audio and adapter regression suites |
| `tests/sdk_stubs/` | Minimal SDK substitutes for host tests |
| `tools/` | Unit build script |
| `docs/` | Current controls, presets, migration, and validation |
| `docs/archive/` | Historical design notes |
| `dist/` | Installable unit package |
| `build/` | Ignored build products, test executables, and rendered audio |

The [v1 design archive](docs/archive/design-v1.md) preserves the original engine
comparison and measurements. Its parameter tables describe revision 1.
