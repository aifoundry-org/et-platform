/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "decode.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "processor.h"
#include "traps.h"

namespace bemu {


extern std::array<Hart,EMU_NUM_THREADS> cpu;


void insn_fence_i(insn_t inst)
{
    DISASM_NOARG("fence.i");
    throw trap_mcode_instruction(inst.bits);
}


} // namespace bemu
