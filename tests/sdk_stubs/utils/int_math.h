#pragma once
#include <stdint.h>
inline int32_t clipminmaxi32(int32_t lo,int32_t value,int32_t hi) { return value < lo ? lo : (value > hi ? hi : value); }
