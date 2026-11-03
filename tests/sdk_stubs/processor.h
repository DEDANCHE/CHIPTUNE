#pragma once
#include <cstdint>

// Minimal host substitute for the SDK Processor interface used by Osc.
class Processor {
public:
  static constexpr float getSampleRate() { return 48000.f; }
  virtual uint32_t getBufferSize() const { return 0; }
  virtual void init(float *) = 0;
  virtual void reset() {}
  virtual void suspend() {}
  virtual void setParameter(uint8_t, int32_t) = 0;
  virtual const char *getParameterStrValue(uint8_t, int32_t) const { return nullptr; }
  virtual void process(const float *, float *, uint32_t) = 0;
};
