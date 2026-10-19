# CHIPTUNE

Exploration of a chiptune oscillator unit for the **Korg NTS-1 digital kit mkII**
(logue SDK) built on an APU-style engine instead of a classic wavetable/PolyBLEP oscillator.

- Design notes, engine comparison and open questions: [docs/DESIGN.md](docs/DESIGN.md)
- `src/apu_core.h` – portable, header-only APU core (NES, Game Boy, Sega SN76489, Atari ST YM2149 and C64 SID models)
- `unit/` – NTS-1 mkII unit glue (`header.c`, `osc.h`), written against the SDK `dummy-osc` template
- `test/host_render.cpp` – host test + WAV renderer

## Status

| Piece | State |
|-------|-------|
| APU core | Compiles clean with `-Wall -Wextra -std=c++11`; host checks pass for all five chips (pitch, alias level, filter, release) |
| Unit glue | Written, **not yet built with the logue SDK toolchain or run on hardware** |
| CPU / RAM budget on Cortex-M7 | Not measured |

## Host test

Any C++11 compiler works (verified with `zig c++`):

```
zig c++ -std=c++11 -Wall -Wextra -O2 test/host_render.cpp -o build/host_render
build/host_render build      # prints checks, writes WAVs into build/
```

## Building the unit

1. Copy `logue-sdk/platform/nts-1_mkii/dummy-osc` to a new project folder (keeps SDK `Makefile`, `config.mk`, `unit.cc`).
2. Replace its `header.c` and `osc.h` with `unit/header.c` and `unit/osc.h`, and copy `src/apu_core.h` next to them.
3. Set the project name in `config.mk`, then `build nts-1_mkii/<project>` in the SDK Docker environment.
4. Load the resulting `.nts1mkiiunit` with the Korg Kontrol Editor.
