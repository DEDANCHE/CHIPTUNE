/*
 *  File: header.c
 *
 *  CHIPTUNE APU oscillator unit header (NTS-1 mkII).
 *  Layout follows the SDK's dummy-osc template; see README for how to build.
 */

#include "unit_osc.h"

const __unit_header unit_header_t unit_header = {
    .header_size = sizeof(unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_osc,
    .api = UNIT_API_VERSION,
    .dev_id = 0x0U, // TODO: request a developer ID, see logue-sdk/developer_ids.md
    .unit_id = 0x0U,
    .version = 0x00020000U,
    .name = "chipapu",
    .num_params = 10,
    .params = {
        // min, max, center (unused), default, type, frac. bits, frac. mode, <reserved>, name

        // A knob: chip-specific timbre (v2 parameter layout)
        {0, 1023, 0, 384, k_unit_param_type_none, 0, 0, 0, {"COLOR"}},
        // B knob: pulse 2 detune in period-register steps (0..16); on SID the filter cutoff (open..closed)
        {0, 1023, 0, 0, k_unit_param_type_none, 0, 0, 0, {"DETN/FLT"}},

        // Original preset and chip IDs are reserved; experimental models live in presets.
        {0, 73, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"PRESET"}},
        {0, 7, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"CHIP"}},
        {0, 35, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"ARP/RATE"}},
        {0, 19, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"VOICE"}},
        {0, 5, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"NOISE"}},
        {0, 15, 0, 0, k_unit_param_type_none, 0, 0, 0, {"DECAY"}},
        {0, 3, 0, 1, k_unit_param_type_strings, 0, 0, 0, {"TUNE/RES"}},
        {0, 5, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"INTVL"}}},
};
