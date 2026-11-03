# CHIPTUNE

An APU-style chiptune oscillator for the **Korg NTS-1 digital kit mkII**, built with
the logue SDK. Version 2.0.2 includes eight primary chip families and explicit
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

## Building and testing

The portable core and host tests target C++11. Run these PowerShell commands from
the repository root with Zig available on PATH:

```powershell
New-Item -ItemType Directory -Force build/host, build/audio | Out-Null
zig c++ -std=c++11 -Wall -Wextra -O2 tests/core_audio_test.cpp -o build/host/core_audio_test.exe
& ./build/host/core_audio_test.exe build/audio
zig c++ -std=c++11 -Wall -Wextra -O2 -Itests/sdk_stubs -Isrc tests/adapter_test.cpp -o build/host/adapter_test.exe
& ./build/host/adapter_test.exe
```

The core suite tests synthesis and writes WAV files. The adapter suite uses SDK
stubs and tests parameter recall, note timing, engine changes, and reset behavior.
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
   `platform/nts1_mkii/`. Copy `src/apu_core.h` and `src/presets.h` alongside them.
3. Set the project name in `config.mk`, then run `build nts-1_mkii/<project>`
   in the SDK Docker environment.
4. Load the resulting unit with the Korg Kontrol Editor.

The custom `unit.cc` is required for parameter recall. When preparing a release,
copy the tested package from `build/unit/` to `dist/`.

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
