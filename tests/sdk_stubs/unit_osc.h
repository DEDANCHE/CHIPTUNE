#pragma once
// Minimal runtime stand-in for callback integration tests; not an ABI emulator.
#include <stdint.h>
#define UNIT_TARGET_PLATFORM 0x100
#define k_unit_module_osc 1
#define UNIT_API_VERSION 1
#define UNIT_API_IS_COMPAT(api) ((api) == UNIT_API_VERSION)
#define __unit_header
#ifdef __cplusplus
#include <cmath>
#define __unit_callback extern "C"
#else
#define __unit_callback
#endif
enum { k_unit_param_type_none, k_unit_param_type_strings };
enum { k_unit_err_none=0, k_unit_err_undef=-1, k_unit_err_target=-2,
       k_unit_err_api_version=-3, k_unit_err_samplerate=-4, k_unit_err_geometry=-5 };
typedef struct { int32_t min,max,center,initial,type,bits,mode,reserved; char name[16]; } unit_param_t;
typedef struct { uint32_t header_size,target,api,dev_id,unit_id,version; char name[16]; uint32_t num_params; unit_param_t params[10]; } unit_header_t;
typedef struct { int32_t shape_lfo; uint16_t pitch; } unit_runtime_osc_context_t;
typedef struct { uint32_t target,api,samplerate,input_channels,output_channels;
  struct { void *runtime_context; } hooks;
} unit_runtime_desc_t;
#ifdef __cplusplus
extern "C" { extern const unit_header_t unit_header; }
inline float osc_w0f_for_note(uint8_t note, uint8_t fraction) {
  return 440.f / 48000.f * std::pow(2.f, (note + fraction / 256.f - 69.f) / 12.f);
}
#endif
