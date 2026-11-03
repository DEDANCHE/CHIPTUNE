#pragma once
// C/C++ shared control contract: symbol, maximum, default, display type, label.
#define CHIPAPU_PARAMETERS(X) \
  X(COLOR, 1023, 384, k_unit_param_type_none, "COLOR") \
  X(DETN, 1023, 0, k_unit_param_type_none, "DETN/FLT") \
  X(PRESET, 76, 0, k_unit_param_type_strings, "PRESET") \
  X(CHIP, 12, 0, k_unit_param_type_strings, "CHIP") \
  X(ARP, 35, 0, k_unit_param_type_strings, "ARP/RATE") \
  X(SUB, 22, 0, k_unit_param_type_strings, "VOICE") \
  X(NOISE, 5, 0, k_unit_param_type_strings, "NOISE") \
  X(DECAY, 15, 0, k_unit_param_type_none, "DECAY") \
  X(QUANT, 3, 1, k_unit_param_type_strings, "TUNE/RES") \
  X(INTVL, 5, 0, k_unit_param_type_strings, "INTVL")
