/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include "emu_defines.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "insn_util.h"
#include "processor.h"
#include "traps.h"

namespace bemu {


void insn_fence_i(Hart& cpu)
{
    DISASM_NOARG("fence.i");
    throw trap_mcode_instruction(cpu.inst.bits);
}


} // namespace bemu
