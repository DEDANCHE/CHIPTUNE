/*
 *  File: header.c
 *
 *  CHIPTUNE APU oscillator unit header (NTS-1 mkII).
 *  Copyright (C) 2026 DEDANCHE
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
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