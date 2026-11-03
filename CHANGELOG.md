# Changelog

All notable changes to the **CHIPTUNE** APU oscillator for the Korg NTS-1 mkII will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.1.0] - 2026-11-03

### Added
- **Developer ID Registration**: Assigned and registered official Korg Logue SDK FourCC developer identifier `0x44443732U` (`'DD72'`) in `platform/nts1_mkii/header.c`, preventing identifier collisions in the Korg Kontrol Editor.
- **Dedicated Solo Noise Articulation**:
  - **Atari 2600 TIA**: Knob A (`COLOR`) on solo noise patches (`VOICE` NOISE / 4) now dynamically modulates between the four authentic hardware polynomial distortion modes: pure tone divider rumble (`[51] X TIA LOW NZ`), 4-bit poly crunch (`[14] X TIA THUD`), 5-bit poly metal (`[52] X TIA METAL`), and 9-bit poly white noise snare (`[28] X TIA SNARE`).
  - **NES, Game Boy, and SN76489**: Solo noise patches activate metallic/periodic short-cycle LFSR mode when `COLOR >= 512` (93-step on NES, 127-step on Game Boy, periodic shift on SN76489), while values below 512 provide smooth 15-bit white noise.
- **New Presets**: Appended three new expressive sound presets (catalog now 77 presets, indices `0` through `76`):
  - `[74] SID SLOW DEEP`: Fixed 0.25 Hz slow rate, 0.35 depth pulse-width modulation.
  - `[75] NES KICK`: Isolated triangle channel with a 24-semitone falling pitch envelope.
  - `[76] PCE TOM`: Isolated wavetable channel with a 12-semitone falling pitch envelope.
- **Expression & Modulation Routing**:
  - Real-time Channel Pressure and Polyphonic Aftertouch modulation support: opens the Chamberlin low-pass filter on SID patches and deepens detune/modulation depth on dual-voice and FM patches via smoothed `shapeMod_`.
  - Shape LFO support via `setShapeLfo()`.
- **Automated Verification & Release Tooling**:
  - Implemented standalone Python ELF32 validator `tools/check_unit.py` for headless environments.
  - Added dual Node.js / Python fallback in `tools/release.ps1` for reproducible unit packaging and validation.
  - Integrated Clang format specification (`.clang-format`) and code cleanup tooling in `tools/audio_tools.cpp`.

### Changed
- **Curatorial Volume Leveling**: Calibrated channel levels (`pulse1Level`, `pulse2Level`, `waveLevel`) on hot sustained presets (`[ 2] NES BASS`, `[ 3] GB WAVE MIX`, `[40] NES OCT ARP`, `[71] NES MAJ 16TH`) by ~-1.7 to -2.0 dB, bringing raw RMS into the -9 to -10 dBFS range and matching the rest of the factory bank without flattening dynamics.
- **Acoustic Optimization of Solo Noise Presets**: Set factory `COLOR` default to 128 (Zone 0) on `[16] NES SNARE` and `[18] GB NOISE HIT` so preset recall retains authentic full-sequence white noise while enabling Knob A to morph into metallic buzz on demand.
- **Math Optimization**: Replaced `std::pow(2.f, ...)` with hardware single-precision `std::exp2(...)` in `src/apu_core.h` and `platform/nts1_mkii/osc.h`.

### Fixed
- **Legato Note-Off Termination**: Corrected note-off voice termination in `platform/nts1_mkii/osc.h` (`if (note != activeNote_) return;`) so releasing a previously held note during legato transitions does not silence the active voice.
- **Pitch Bend Zero-Bypass**: Added zero-check bypass in `platform/nts1_mkii/osc.h` to skip exponential pitch calculations when pitch bend is centered (8192), eliminating unnecessary CPU overhead on idle notes.
- **Modular Delay Indexing**: Replaced negative signed bitwise operations in SID delay ring buffers in `src/apu_core.h` with positive modular offsets (`+ 32`), ensuring strict C++ standard compliance across all compilers.
- **Adapter Scalar Initialization**: Added in-class default initializers to scalar members in `Osc` (`w0_`, `pending_`, `pendingMask_`, `presetIndex_`, `bend_`, `ready_`, `notePending_`, `pendingVelocity_`).
- **Interrupt/DMA Safety**: Qualified `pendingMask_` as `volatile` to prevent register caching across audio DMA interrupt and UI callback contexts.

---

## [2.0.2] - 2026-11-03

### Added
- **Transient Click Crossfade**: Added a 48-sample (1 ms) linear crossfade (`transitionLeft_`, `transitionFrom_`) triggered on engine, wavetable bank, or voice mode changes, eliminating DC pops during live switching.
- **Engine State Normalization**: Pure YM envelope states normalize to P1 solo when switching away from the YM engine, preventing hidden FM/sync states from corrupting other chip engines.

### Changed
- **Preset Nomenclature & Accuracy**: Corrected preset names to accurately reflect underlying acoustic behavior (distinguishing sustained harmonic/FM presets, mixed noise hits, and gentle PWM).
- **Triangle Envelope Repair**: Repaired preset `[21] YM TRI ENV` into an authentic repeating triangle envelope (`COLOR = 256`), while preserving saw envelope on preset `[42] YM SAW ENV`.

### Fixed
- **Preset Round-Trip Recall**: Parameter reads during recall resolve pending state atomically via `resolvedParams()` without consuming the batch, ensuring parameter saves reflect accurate engine states.

---

## [2.0.0] - 2026-11-02

### Added
- **Multi-Engine APU Synthesizer Core**:
  - Integrated 8 primary chip families: Nintendo NES (RP2A03), Game Boy (DMG), Sega Master System (SN76489), Atari ST (YM2149), Commodore 64 (SID 6581/8580), PC Engine (HuC6280), Atari 8-bit (POKEY), and Konami VRC6.
  - Added 5 experimental/extended models: Sunsoft 5B, Atari 2600 (TIA), Namco 163 (N163), Famicom Disk System (FDS), and Amiga (Paula).
- **Band-Limited Step Synthesis (`BlipBuf`)**: Fractional-sample windowed-sinc impulse integration (16 taps, 64 phases) for anti-aliased square, wavetable, and envelope waveforms without multi-megahertz oversampling.
- **SID 6581/8580 Engine**: 2x oversampled (96 kHz) PolyBLEP oscillators, hard sync step cancellation, 31-tap decimation FIR filter, and 2-pole Chamberlin state-variable filter.
- **Dynamic Limiting (`softLimit`)**: $C^1$-continuous rational soft-knee saturator keeping signal peaks bounded below 0.955.
- **Unified 10-Control Interface**: Standardized NTS-1 mkII mapping across `COLOR`, `DETN/FLT`, `PRESET`, `CHIP`, `ARP/RATE`, `VOICE`, `NOISE`, `DECAY`, `TUNE/RES`, and `INTVL`.
- **Expanded Preset Catalog**: Expanded factory presets to 74 presets covering all 13 chip models.

---

## [1.0.0] - 2026-10-25

### Added
- Initial release of the CHIPTUNE APU oscillator for Korg NTS-1 mkII.
- Basic chiptune waveform synthesis (pulse, triangle, noise).
- Initial 33 factory presets.
