/*-------------------------------------------------------------------------
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#include <cinttypes>

#include "system.h"
#include "processor.h"
#include "emu_gio.h"

namespace bemu {

void System::apply_boot_protocol(uint64_t payload_pc, uint64_t payload_sp)
{
    constexpr uint64_t MRAM_BASE = 0x40000000ULL;
    constexpr uint64_t BOOT_PROT_SIZE = 0x100;
    constexpr uint64_t PC_OFFSET = 0x28;
    constexpr uint64_t SP_OFFSET = 0x20;

    // Write PC and SP to MRAM boot vector addresses
    memory.init(noagent, MRAM_BASE + PC_OFFSET, sizeof(payload_pc), &payload_pc);
    memory.init(noagent, MRAM_BASE + SP_OFFSET, sizeof(payload_sp), &payload_sp);

    uint64_t pc = payload_pc;
    uint64_t sp = payload_sp;

    // Sanity checks
    if (pc >= MRAM_BASE && pc < MRAM_BASE + BOOT_PROT_SIZE)
        LOG_AGENT(WARN, noagent, "Payload PC (0x%" PRIx64 ") points into boot protocol region", pc);
    if (sp >= MRAM_BASE && sp < MRAM_BASE + BOOT_PROT_SIZE)
        LOG_AGENT(WARN, noagent, "Payload SP (0x%" PRIx64 ") points into boot protocol region", sp);

    // Set enabled harts
    for (unsigned h = 0; h < EMU_NUM_THREADS; ++h) {
        if (cpu[h].is_nonexistent() || cpu[h].is_unavailable())
            continue;
        cpu[h].pc = pc;
        cpu[h].npc = pc;
        cpu[h].xregs[2] = sp;
    }
}

} // namespace bemu
