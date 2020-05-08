/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <algorithm>
#include <fstream>
#include "elfio/elfio.hpp"
#include "emu_gio.h"
#include "load.h"
#include "main_memory.h"

namespace bemu {


void load_elf(MainMemory& mem, const char* filename)
{
    std::ifstream file;
    ELFIO::elfio elf;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::in | std::ios::binary);
    elf.load(file);
    for (const ELFIO::segment* seg : elf.segments) {
        if (!(seg->get_type() & PT_LOAD))
            continue;

        LOG_NOTHREAD(INFO, "Segment[%d] VA: 0x%" PRIx64 "\tType: 0x%" PRIx32 " (LOAD)",
                     seg->get_index(), seg->get_virtual_address(), seg->get_type());

        uint64_t vma_offset = seg->get_virtual_address() - seg->get_physical_address();

        for (ELFIO::Elf_Half idx = 0; idx < seg->get_sections_num(); ++idx) {
            const ELFIO::section* sec =
                    elf.sections[seg->get_section_index_at(idx)];

            if (!sec->get_size())
                continue;

            if (sec->get_type() == SHT_NOBITS)
                continue;

            uint64_t vma = sec->get_address();
            uint64_t lma = vma - vma_offset;
            LOG_NOTHREAD(INFO, "Section[%d] %s\tVMA: 0x%" PRIx64 "\tLMA: 0x%" PRIx64 "\tSize: 0x%" PRIx64
                         "\tType: 0x%" PRIx32 "\tFlags: 0x%" PRIx64,
                         idx, sec->get_name().c_str(), vma, lma, sec->get_size(),
                         sec->get_type(), sec->get_flags());

            if (lma >= MainMemory::dram_base)
                lma &= ~0x4000000000ULL;

            mem.init(Noshire_agent{}, lma, sec->get_size(), sec->get_data());
        }
    }
}


void load_raw(MainMemory& mem, const char* filename, unsigned long long addr)
{
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::in | std::ios::binary);
    file.exceptions(std::ifstream::badbit);

    char fbuf[65536];
    while (true) {
        file.read(fbuf, 65536);
        std::streamsize count = file.gcount();
        if (count <= 0)
            break;
        if (addr >= MainMemory::dram_base)
            addr &= ~0x4000000000ULL;
        mem.init(Noshire_agent{}, addr, count, reinterpret_cast<MainMemory::const_pointer>(fbuf));
        addr += count;
    }
}


} // bemu
