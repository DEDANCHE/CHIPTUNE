# Development and release workflow

Run commands from the repository root. Dependencies: Zig, Node.js, PowerShell;
ARM GCC, GNU make, Git for Windows, and a local logue-sdk checkout for firmware.

## Tests and release

```powershell
powershell -File tools/test.ps1
powershell -File tools/release.ps1 -Sdk C:\path\to\logue-sdk
```

`test.ps1` compiles four suites with undefined-behavior sanitization by default:
adapter, features, runtime callbacks, and core audio. `-NoSanitize` is available
for a faster development pass. Logs go to `build/host/`; test WAVs to `build/audio/`.
Runtime tests compile the real `unit.cc` and `header.c` against minimal host SDK
substitutes. They validate callback behavior, not the target ABI or hardware timing.

The release script stops on test/build failure. It validates the packaged ELF's
PT_LOAD address span, package size, and all 17 required callback exports before
copying the unit to `dist/`. Both limits are 49,152 bytes. `dist/manifest.json`
records SHA-256, memory headroom, compiler, version, and CMSIS revision. Actual
hardware loading and listening remain separate acceptance checks.

CMSIS is pinned to `f25cf2c15304ad872a5b9761651d3f57d6202906` from
`https://github.com/ARM-software/CMSIS.git`. The build verifies the SDK checkout's
revision when using it. If headers are absent, it fetches that exact commit into
`build/CMSIS`. The build rejects a different existing revision rather than silently
changing the user's SDK. ARM compiler and SDK revisions are otherwise locally
selected; this workflow does not promise identical output across toolchains.

## Preset auditions

```powershell
powershell -File tools/audio.ps1 -Mode audition
# In a PowerShell session, select arbitrary IDs or all presets:
./tools/audio.ps1 -Presets 0,6,74,75,76
./tools/audio.ps1 -Presets all
```

The default set is 0, 6, 9, 61, 74, 75, 76. Each preset renders identical bass,
melody, and percussion note patterns at 120 BPM and velocity 110. Every phrase has
a raw and a matched WAV. Matching targets RMS 0.15 with a peak ceiling of 0.95;
`audition.csv` records the actual gain and achieved RMS. Peak-limited phrases may
not reach the target. This is RMS matching, not perceptual loudness normalization.
Files go to `build/audition/`, or `-OutputDirectory`.

## Profiling

```powershell
./tools/audio.ps1 -Mode profile -OutputDirectory build/profile
./tools/build_unit.ps1 -Sdk C:\path\to\logue-sdk -Profile
```

The first command measures host block time for every chip ID at 55, 440, and
12,000 Hz, with 48- and 96-frame blocks and active modulation. Its CSV is useful
for relative host regressions; it cannot establish Cortex-M7 CPU headroom.

The second command builds an **instrumented debug unit** in `build/unit/`. It
enables the Cortex-M7 DWT cycle counter and exposes `chipapu_profile` for a debugger:
block count, last/max cycles and matching frame counts, and total cycles/frames.
Read a stable snapshot while execution is halted. Enable it only on the intended
ARM target. Do not ship it as the normal release; `release.ps1` rebuilds without it.

To measure on hardware:

1. Load the instrumented unit and attach a debugger capable of reading its symbols.
2. Use the actual core clock, not the host PC speed. A block's budget is
   `core_clock_hz * frames / 48000` cycles. Compare last/max measurements only with
   their matching frame counts. Average cost is totalCycles / totalFrames.
3. Exercise high notes, SID sync, FDS modulation, maximum noise rates, pressure,
   clock events, and repeated preset transitions. Include the actual hardware
   effect/routing configuration and check for audible underruns.
4. Record firmware version, actual clock, block geometry, patch, peak/average
   utilization, and audible behavior. DWT instrumentation adds a small overhead.

The maximum records the block with most raw cycles, not necessarily the highest
utilization if block sizes vary; use fixed block geometry for worst-case testing.
Hardware measurements have not been performed in this development environment.

## Data and compatibility

`platform/nts1_mkii/parameters.h` supplies enum order, ranges, defaults, and header
labels. `src/presets.h` supplies explicit defaults and named stable IDs. Numeric
IDs 0–73 are retained; 74–76 are appended. Preset rows store cutoff as an integer
percentage to reduce ROM/data size; edit-time cutoff still uses the full knob range.

Private implementation symbols are hidden in the firmware to reduce dynamic
symbol/relocation overhead. SDK imports and runtime callbacks retain external
visibility; the packaged ELF export check guards that boundary.
