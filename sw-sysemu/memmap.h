/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MEMMAP_H
#define BEMU_MEMMAP_H

// ET-SoC Memory map
//
// +---------------------+---------------------------------+-----------------+
// |   Address range     |      Address range (hex)        |                 |
// |  From    |    To    |      From      |      To        | Maps to         |
// +----------+----------+----------------+----------------+-----------------+
// |     0G   |     1G   | 0x00_0000_0000 | 0x00_3fff_ffff | IO region       |
// |     1G   |     2G   | 0x00_4000_0000 | 0x00_7fff_ffff | SP region       |
// |     1G   |  1G+128K | 0x00_4000_0000 | 0x00_4001_ffff |     SP/ROM      |
// |   1G+1M  |   1G+2M  | 0x00_4040_0000 | 0x00_404f_ffff |     SP/SRAM     |
// | 1G+288M+164K ...    | 0x00_5202_9000 | 0x00_5202_9FFF |     SP/SP_MISC  |
// |     2G   |     4G   | 0x00_8000_0000 | 0x00_ffff_ffff | SCP region      |
// |     4G   |     8G   | 0x01_0000_0000 | 0x01_ffff_ffff | ESR region      |
// |     8G   |   256G   | 0x02_0000_0000 | 0x3f_ffff_ffff | Reserved        |
// |   256G   |   512G   | 0x40_0000_0000 | 0x7f_ffff_ffff | PCIe region     |
// |   512G   |  512G+8M | 0x80_0000_0000 | 0x80_007f_ffff | DRAM/Mbox       |
// |   512G   |  512G+2M | 0x80_0000_0000 | 0x80_001f_ffff |     DRAM/Mcode  |
// |  512G+2M |  512G+8M | 0x80_0020_0000 | 0x80_007f_ffff |     DRAM/Mdata  |
// |  512G+8M | 512G+64M | 0x80_0080_0000 | 0x80_03ff_ffff | DRAM/Sbox       |
// |  512G+8M | 512G+16M | 0x80_0080_0000 | 0x80_00ff_ffff |     DRAM/Scode  |
// | 512G+16M | 512G+64M | 0x80_0100_0000 | 0x80_03ff_ffff |     DRAM/Sdata  |
// | 512G+64M |   516G   | 0x80_0400_0000 | 0x80_ffff_ffff | DRAM/OSbox      |
// |   516G   |   544G   | 0x81_0000_0000 | 0x87_ffff_ffff | DRAM/Other      |
// +----------+----------+----------------+----------------+-----------------+

#include <cstdint>

namespace bemu {


inline bool paddr_is_maxion_space(uint64_t addr)
{ return addr < 0x0010000000ULL; }


inline bool paddr_is_io_space(uint64_t addr)
{ return addr < 0x0040000000ULL; }


inline bool paddr_is_sp_space(uint64_t addr)
{ return (addr >= 0x0040000000ULL) && (addr < 0x0080000000ULL); }


inline bool paddr_is_sp_rom(uint64_t addr)
{ return (addr >= 0x0040000000ULL) && (addr < 0x0040020000ULL); }


inline bool paddr_is_sp_sram(uint64_t addr)
{ return (addr >= 0x0040400000ULL) && (addr < 0x0040500000ULL); }


inline bool paddr_is_sp_sram_code(uint64_t addr)
{ return (addr >= 0x0040400000ULL) && (addr < 0x0040480000ULL); }


inline bool paddr_is_sp_sram_data(uint64_t addr)
{ return (addr >= 0x0040480000ULL) && (addr < 0x0040500000ULL); }


inline bool paddr_is_sp_misc(uint64_t addr)
{ return (addr >= 0x0052029000ULL) && (addr < 0x005202A000ULL); }


inline bool paddr_is_scratchpad(uint64_t addr)
{ return (addr >= 0x0080000000ULL) && (addr < 0x0100000000ULL); }


inline bool paddr_is_esr_space(uint64_t addr)
{ return (addr >= 0x0100000000ULL) && (addr < 0x0200000000ULL); }


inline bool paddr_is_reserved(uint64_t addr)
{ return (addr >= 0x0200000000ULL) && (addr < 0x4000000000ULL); }


inline bool paddr_is_pcie_space(uint64_t addr)
{ return (addr >= 0x4000000000ULL) && (addr < 0x8000000000ULL); }


inline bool paddr_is_dram_mbox(uint64_t addr)
{ return (addr >= 0x8000000000ULL) && (addr < 0x8000800000ULL); }


inline bool paddr_is_dram_mcode(uint64_t addr)
{ return (addr >= 0x8000000000ULL) && (addr < 0x8000200000ULL); }


inline bool paddr_is_dram_mdata(uint64_t addr)
{ return (addr >= 0x8000200000ULL) && (addr < 0x8000800000ULL); }


inline bool paddr_is_dram_sbox(uint64_t addr)
{ return (addr >= 0x8000800000ULL) && (addr < 0x8004000000ULL); }


inline bool paddr_is_dram_scode(uint64_t addr)
{ return (addr >= 0x8000800000ULL) && (addr < 0x8001000000ULL); }


inline bool paddr_is_dram_sdata(uint64_t addr)
{ return (addr >= 0x8001000000ULL) && (addr < 0x8004000000ULL); }


inline bool paddr_is_dram_osbox(uint64_t addr)
{ return (addr >= 0x8004000000ULL) && (addr < 0x8100000000ULL); }


inline bool paddr_is_dram_other(uint64_t addr)
{ return addr >= 0x8100000000ULL; }


inline bool paddr_is_dram(uint64_t addr)
{ return addr >= 0x8000000000ULL; }


inline bool paddr_is_dram_uncacheable(uint64_t addr)
{ return (addr >= 0x8000000000ULL) && ((addr & 0x4000000000ULL) != 0); }


} // namespace bemu

#endif // BEMU_MEMMAP_H
