#pragma once
#include <stdint.h>
// Opt-in debugger instrumentation. Release builds perform no MMIO or bookkeeping.
#ifdef CHIPAPU_PROFILE
struct ChipapuProfile { uint32_t blocks, maxCycles, maxFrames, lastCycles, lastFrames; uint64_t totalCycles, totalFrames; };
extern "C" { volatile ChipapuProfile chipapu_profile = {}; }
inline volatile uint32_t &profile_reg(uint32_t address) { return *reinterpret_cast<volatile uint32_t *>(address); }
inline void chipapu_profile_init() {
  profile_reg(0xE000EDFC) |= 1u << 24; // DEMCR.TRCENA
  profile_reg(0xE0001FB0) = 0xC5ACCE55; // DWT lock access on Cortex-M7
  profile_reg(0xE0001004) = 0;
  profile_reg(0xE0001000) |= 1u;
}
inline uint32_t chipapu_profile_begin() { return profile_reg(0xE0001004); }
inline void chipapu_profile_end(uint32_t begin, uint32_t frames) {
  const uint32_t cycles = profile_reg(0xE0001004) - begin;
  ++chipapu_profile.blocks;
  chipapu_profile.lastCycles = cycles; chipapu_profile.lastFrames = frames;
  chipapu_profile.totalCycles += cycles; chipapu_profile.totalFrames += frames;
  if (cycles > chipapu_profile.maxCycles) { chipapu_profile.maxCycles = cycles; chipapu_profile.maxFrames = frames; }
}
#else
inline void chipapu_profile_init() {}
inline uint32_t chipapu_profile_begin() { return 0; }
inline void chipapu_profile_end(uint32_t, uint32_t) {}
#endif
