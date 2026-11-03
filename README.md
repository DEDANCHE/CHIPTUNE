# CHIPTUNE

Exploration of a chiptune oscillator unit for the **Korg NTS-1 digital kit mkII**
(logue SDK) built on an APU-style engine instead of a classic wavetable/PolyBLEP oscillator.

- Design notes, engine comparison and open questions: [docs/DESIGN.md](docs/DESIGN.md)
- [Oscillator revision 2](docs/OSCILLATORS_V2.md) – eight primary families, legacy IDs, controls, and migration
- `src/apu_core.h` – portable, header-only core: NES, GB, SN76489, YM2149, SID, PCE/WT, POKEY, VRC6; experimental/legacy models remain available through presets
- `src/presets.h` / [docs/PRESETS.md](docs/PRESETS.md) – 74 presets (original positions retained); knob A is COLOR, knob B is DETN/FLT/FM depth
- `unit/` – NTS-1 mkII unit glue (`header.c`, `osc.h`, `unit.cc`), written against the SDK `dummy-osc` template
- `test/host_render.cpp` – host test + WAV renderer

## Status

Revision 2 was built and host-tested on 2026-11-03. This major revision changes the parameter layout: reselect a preset and recreate v1 edits before saving a v2 patch. Hardware listening and CPU profiling remain pending.

| Piece | State |
|-------|-------|
| APU core | Compiles with `-Wall -Wextra -std=c++11`; all 309 host renderer checks and 461 adapter checks pass |
| Unit glue | **Builds with the logue SDK `nts-1_mkii` makefiles** (arm-none-eabi-gcc 14.2.1, no warnings) into a 42,244-byte `.nts1mkiiunit`; not yet run on hardware |
| SHAPE_LFO / pitch bend | Implemented; ranges assumed (bend 14-bit centred 8192, ±2 semitones). Audio input not used: all 10 parameter slots are taken |
| Size / RAM | ELF sections: text 37,963 bytes, data 3,028 bytes, bss 6,444 bytes. CPU on Cortex-M7 is not yet measured |

## Host test

The host renderer targets C++11 (the original revision was verified with `zig c++`):

```
zig c++ -std=c++11 -Wall -Wextra -O2 test/host_render.cpp -o build/host_render
build/host_render build      # prints checks, writes WAVs into build/
```

Adapter regression source is in `test/host_unit.cpp`, using host SDK substitutes from `test/support` (include paths: `test/support` and `src`). It covers presets in both recall orders, family aliases, COLOR/voice/rate parameters, repeated edits, and reset/suspend behavior. The renderer adds musical-tuning, solo, zero-depth FM and tempo cases. All 770 checks pass.

```
zig c++ -std=c++11 -Wall -Wextra -O2 -Itest/support -Isrc test/host_unit.cpp -o build/host_unit
build/host_unit
```

## Building the unit

The rebuilt `dist/chipapu.nts1mkiiunit` contains revision 2 and the review fixes. It has not yet been tested on hardware.

Windows shortcut (logue-sdk checkout + arm-none-eabi-gcc + GNU make + Git for Windows):

```
$env:LOGUE_SDK = "C:\path\to\logue-sdk"
powershell -File tools\build_unit.ps1      # -> build\unit\chipapu.nts1mkiiunit
```

The script uses the SDK's CMSIS checkout when present, otherwise fetches the headers into `build/CMSIS`. It stages the project in `build/unit` and runs `make install` with the SDK makefiles. The SDK targets gcc-arm-none-eabi 10.3; this revision was built with 14.2.1.

Manual / Docker route:

1. Copy `logue-sdk/platform/nts-1_mkii/dummy-osc` to a new project folder (keeps SDK `Makefile` and `config.mk`).
2. Replace its `header.c`, `osc.h` and `unit.cc` with the files from `unit/`, and copy both `src/apu_core.h` and `src/presets.h` next to them. The custom `unit.cc` is required for correct parameter recall.
3. Set the project name in `config.mk`, then `build nts-1_mkii/<project>` in the SDK Docker environment.
4. Load the resulting `.nts1mkiiunit` with the Korg Kontrol Editor.
