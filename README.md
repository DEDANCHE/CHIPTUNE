# CHIPTUNE

Exploration of a chiptune oscillator unit for the **Korg NTS-1 digital kit mkII**
(logue SDK) built on an APU-style engine instead of a classic wavetable/PolyBLEP oscillator.

- Design notes, engine comparison and open questions: [docs/DESIGN.md](docs/DESIGN.md)
- `src/apu_core.h` – portable, header-only APU core (NES, Game Boy, Sega SN76489, Atari ST YM2149, C64 SID, PC Engine HuC6280, Atari POKEY, Konami VRC6, Sunsoft 5B, Atari 2600 TIA, Namco N163, Famicom Disk System and Amiga Paula models)
- `src/presets.h` / [docs/PRESETS.md](docs/PRESETS.md) – 60 presets, selected with knob A
- `unit/` – NTS-1 mkII unit glue (`header.c`, `osc.h`), written against the SDK `dummy-osc` template
- `test/host_render.cpp` – host test + WAV renderer

## Status

| Piece | State |
|-------|-------|
| APU core | Compiles clean with `-Wall -Wextra -std=c++11`; host checks pass for all thirteen chips (pitch, alias level, filter, release, sync / ring / FM / high-pass voicings, presets, fuzz) |
| Unit glue | **Builds with the logue SDK `nts-1_mkii` makefiles** (arm-none-eabi-gcc 14.2.1, no warnings) into a 32 KB `.nts1mkiiunit`; not yet run on hardware |
| SHAPE_LFO / pitch bend | Implemented; ranges assumed (bend 14-bit centred 8192, ±2 semitones). Audio input not used: all 10 parameter slots are taken |
| Size / RAM | Measured from the ELF: text 28.9 KB, data 2.5 KB, bss 5.8 KB (limits 48 KB each). CPU on Cortex-M7 not measured; on a host PC the FDS voice at 3.5 kHz costs about 3x a NES voice, so it is the heaviest case |

## Host test

Any C++11 compiler works (verified with `zig c++`):

```
zig c++ -std=c++11 -Wall -Wextra -O2 test/host_render.cpp -o build/host_render
build/host_render build      # prints checks, writes WAVs into build/
```

## Building the unit

A prebuilt `dist/chipapu.nts1mkiiunit` (built from this source, not yet tested on hardware) is included; load it with the Korg Kontrol Editor.

Windows shortcut (logue-sdk checkout + arm-none-eabi-gcc + GNU make + Git for Windows):

```
$env:LOGUE_SDK = "C:\path\to\logue-sdk"
powershell -File tools\build_unit.ps1      # -> build\unit\chipapu.nts1mkiiunit
```

The script fetches the CMSIS headers (the SDK submodule) into `build/CMSIS`, stages the project in `build/unit` and runs `make install` with the SDK makefiles. The SDK targets gcc-arm-none-eabi 10.3; other versions are untested beyond the build above.

Manual / Docker route:

1. Copy `logue-sdk/platform/nts-1_mkii/dummy-osc` to a new project folder (keeps SDK `Makefile`, `config.mk`, `unit.cc`).
2. Replace its `header.c` and `osc.h` with `unit/header.c` and `unit/osc.h`, and copy `src/apu_core.h` next to them.
3. Set the project name in `config.mk`, then `build nts-1_mkii/<project>` in the SDK Docker environment.
4. Load the resulting `.nts1mkiiunit` with the Korg Kontrol Editor.
