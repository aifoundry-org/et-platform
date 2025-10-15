/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "emu_gio.h"
#include "esrs.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "log.h"
#include "processor.h"
#include "system.h"
#include "utility.h"

namespace bemu {


static inline uint_fast16_t bitmixb(uint_fast16_t sel, uint_fast16_t val0, uint_fast16_t val1)
{
    uint_fast16_t val = 0;
    for (unsigned pos = 0; pos < 16; ++pos) {
        if (sel & 1) {
            val |= ((val1 & 1) << pos);
            val1 >>= 1;
        } else {
            val |= ((val0 & 1) << pos);
            val0 >>= 1;
        }
        sel >>= 1;
    }
    return val;
}


void insn_packb(Hart& cpu)
{
    DISASM_RD_RS1_RS2("packb");
    WRITE_RD( (RS1 & 0xff) | ((RS2 & 0xff) << 8) );
}


void insn_bitmixb(Hart& cpu)
{
    require_feature_gfx();
    DISASM_RD_RS1_RS2("bitmixb");
    WRITE_RD( bitmixb(uint16_t(RS1), uint16_t(RS2), uint16_t(RS2 >> 8)) );
}


} // namespace bemu
