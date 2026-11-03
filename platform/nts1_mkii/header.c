/*
 *  File: header.c
 *
 *  CHIPTUNE APU oscillator unit header (NTS-1 mkII).
 *  Layout follows the SDK's dummy-osc template; see README for how to build.
 */

#include "unit_osc.h"
#include "parameters.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x44443732U, // 'DD72' (DEDANCHE) developer ID, see logue-sdk/developer_ids.md
    .unit_id = 0x0U,
    .version = 0x00020100U,
    .name = "chipapu",
    .num_params = 10,
    .params = {
#define PARAM_HEADER(symbol, maximum, initial, type, label) {0, maximum, 0, initial, type, 0, 0, 0, {label}},
        CHIPAPU_PARAMETERS(PARAM_HEADER)
#undef PARAM_HEADER
    },
};