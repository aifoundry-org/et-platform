/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_H
#define BEMU_MAIN_MEMORY_H

#include <array>
#include <cstdint>
#include <functional>
#include "dense_region.h"
#include "devices/pu_uart.h"
#include "emu_defines.h"
#include "literals.h"
#include "memory_region.h"
#include "null_region.h"
#include "scratch_region.h"
#include "sparse_region.h"
#include "state.h"
#include "sysreg_region.h"
#include "traps.h"

namespace bemu {


// ET-SoC Memory map
//
// +----------------------+---------------------------------+-----------------+
// |     Address range    |      Address range (hex)        |                 |
// |  From     |   To     |      From      |      To        | Maps to         |
// +-----------+----------+----------------+----------------+-----------------+
// |      0G   |     1G   | 0x00_1000_0000 | 0x00_1FFF_FFFF | PU region       |
// |      0G   |   256M   | 0x00_0000_0000 | 0x00_0FFF_FFFF |   Maxion        |
// |    256M   |   512M   | 0x00_1000_0000 | 0x00_1FFF_FFFF |   IO            |
// |    512M   |     1G   | 0x00_2000_0000 | 0x00_3FFF_FFFF |   Mailbox       |
// |  512M+32K |  512+64K | 0x00_2000_8000 | 0x00_2000_FFFF |     PU/SRAM_LO  |
// |  512M+64K | 512+128K | 0x00_2001_0000 | 0x00_2001_FFFF |     PU/SRAM_MID |
// | 512M+128K | 512+256K | 0x00_2002_0000 | 0x00_2003_FFFF |     PU/SRAM_HI  |
// |      1G   |     2G   | 0x00_4000_0000 | 0x00_7FFF_FFFF | SP region       |
// |      1G   |  1G+64K  | 0x00_4000_0000 | 0x00_4000_FFFF |   SP/ROM        |
// |    1G+4M  |   1G+5M  | 0x00_4040_0000 | 0x00_404F_FFFF |   SP/SRAM       |
// |      2G   |     4G   | 0x00_8000_0000 | 0x00_FFFF_FFFF | SCP region      |
// |      4G   |     8G   | 0x01_0000_0000 | 0x01_FFFF_FFFF | ESR region      |
// |      8G   |   256G   | 0x02_0000_0000 | 0x3F_FFFF_FFFF | Reserved        |
// |    256G   |   512G   | 0x40_0000_0000 | 0x7F_FFFF_FFFF | PCIe region     |
// |    512G   |     1T   | 0x80_0000_0000 | 0xFF_FFFF_FFFF | DRAM region     |
// |    512G   |  512G+2M | 0x80_0000_0000 | 0x80_001F_FFFF |   DRAM/Mcode    |
// |   512G+2M |   516G   | 0x80_0020_0000 | 0x80_FFFF_FFFF |   DRAM/OS       |
// |    516G   |   544G   | 0x81_0000_0000 | 0x87_FFFF_FFFF |   DRAM/Other    |
// |    544G   |   768G   | 0x88_0000_0000 | 0xBF_FFFF_FFFF |   Reserved      |
// |    768G   |  768G+2M | 0xC0_0000_0000 | 0xC0_001F_FFFF |   DRAM/Mcode    |
// |   768G+2M |   762G   | 0xC0_0020_0000 | 0xC0_FFFF_FFFF |   DRAM/OS       |
// |    762G   |   800G   | 0xC1_0000_0000 | 0xC7_FFFF_FFFF |   DRAM/Other    |
// |    768G   |    1T    | 0xC8_0000_0000 | 0xFF_FFFF_FFFF |   Reserved      |
// +-----------+----------+----------------+----------------+-----------------+
//
// The DRAM sub-regions with address[39:38]==0b11 alias the DRAM sub-regions
// with address[39:38]==0b10 for some agents.  MainMemory is agnostic to this
// and it considers addresses with address[39:38]==0b11 to be reserved memory.
// It is the responsibility of the requesting agent to map address with
// address[39:38]==0b11 to address[39:38]==0b10 before calling MainMemory.


struct MainMemory {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    // base addresses and sizes for the various regions of the address space
    enum : unsigned long long {
        //pu_base        =   0_GiB,
        //pu_size        =   1_GiB,
        //pu_maxion_base =   0_MiB,
        //pu_maxion_size = 256_MiB,
        //pu_io_base     = 256_MiB,
        //pu_io_size     = 256_MiB,
        //pu_mbox_base   = 512_MiB,
        //pu_mbox_size   = 512_MiB,
        pu_dev0_base     =   0_GiB,
        pu_dev0_size     = 288_MiB + 8_KiB,
        pu_uart_base     = 0x0012002000ULL,
        pu_uart_size     = 4_KiB,
        pu_dev1_base     = 0x0012003000ULL,
        pu_dev1_size     = 16_KiB,
        pu_uart1_base    = 0x0012007000ULL,
        pu_uart1_size    = 4_KiB,
        pu_dev2_base     = 0x0012008000ULL,
        pu_dev2_size     = 224_MiB,
        pu_sram_base     = 0x0020008000ULL,
        pu_sram_size     = 224_KiB,
        pu_dev3_base     = 512_MiB + 256_KiB,
        pu_dev3_size     = 512_MiB - 256_KiB,
        //sp_base        =   1_GiB,
        //sp_size        =   1_GiB,
        sp_rom_base      =   1_GiB,
        sp_rom_size      = 128_KiB,
        sp_dev0_base     =   1_GiB + 128_KiB,
        sp_dev0_size     =   4_MiB - 128_KiB,
        sp_sram_base     =   1_GiB + 4_MiB,
        sp_sram_size     =   1_MiB,
        sp_dev1_base     =   1_GiB + 5_MiB,
        sp_dev1_size     =   1_GiB - 5_MiB,
        scp_base         =   2_GiB,
        scp_size         =   2_GiB,
        sysreg_base      =   4_GiB,
        sysreg_size      =   4_GiB,
        //rsvd0_base     =   8_GiB,
        //rsvd0_size     = 248_GiB,
        pcie_base        = 256_GiB,
        pcie_size        = 256_GiB,
        dram_base        = 512_GiB,
        dram_size        =  32_GiB,
    };

    void read(addr_type addr, size_type n, void* result) const {
        const auto elem = search(addr, n);
        elem->read(addr - elem->first(), n, reinterpret_cast<pointer>(result));
    }

    void write(addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->write(addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    void init(addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->init(addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    addr_type first() const { return 0; }
    addr_type last() const { return dram_base + dram_size - 1; }

    void dump_data(std::ostream& os, addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        auto hi = std::lower_bound(regions.cbegin(), regions.cend(), addr+n-1, above);
        if (hi == regions.cend())
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        size_type pos = addr - (*lo)->first();
        while (lo != hi) {
            (*lo)->dump_data(os, pos, (*lo)->last() - (*lo)->first() - pos + 1);
            ++lo;
            pos = 0;
        }
        (*lo)->dump_data(os, pos, addr + n - (*lo)->first() - pos);
    }

    // Members

    NullRegion    <pu_dev0_base, pu_dev0_size>         pu_dev0_space;
    PU_Uart       <pu_uart_base, pu_uart_size>         pu_uart_space;
    NullRegion    <pu_dev1_base, pu_dev1_size>         pu_dev1_space;
    PU_Uart       <pu_uart1_base, pu_uart1_size>       pu_uart1_space;
    NullRegion    <pu_dev2_base, pu_dev2_size>         pu_dev2_space;
    SparseRegion  <pu_sram_base, pu_sram_size, 16_KiB> pu_sram_space;
    NullRegion    <pu_dev3_base, pu_dev3_size>         pu_dev3_space;
    DenseRegion   <sp_rom_base, sp_rom_size, false>    sp_rom_space;
    NullRegion    <sp_dev0_base, sp_dev0_size>         sp_dev0_space;
    SparseRegion  <sp_sram_base, sp_sram_size, 64_KiB> sp_sram_space;
    NullRegion    <sp_dev1_base, sp_dev1_size>         sp_dev1_space;
    ScratchRegion <scp_base, 4_MiB, EMU_NUM_SHIRES>    scp_space;
    SysregRegion  <sysreg_base, sysreg_size>           sysreg_space;
    NullRegion    <pcie_base, pcie_size>               pcie_space;
    SparseRegion  <dram_base, dram_size, 16_MiB>       dram_space;

protected:
    static inline bool above(const MemoryRegion* lhs, addr_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw trap_bus_error(addr);
        if (addr+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::MainMemory::search()");
        return *lo;
    }

    // This array must be sorted by region base address
    std::array<MemoryRegion*,15> regions = {{
        &pu_dev0_space,
        &pu_uart_space,
        &pu_dev1_space,
        &pu_uart1_space,
        &pu_dev2_space,
        &pu_sram_space,
        &pu_dev3_space,
        &sp_rom_space,
        &sp_dev0_space,
        &sp_sram_space,
        &sp_dev1_space,
        &scp_space,
        &sysreg_space,
        &pcie_space,
        &dram_space
    }};
};


} // namespace bemu

#endif // BEMU_MAIN_MEMORY_H
