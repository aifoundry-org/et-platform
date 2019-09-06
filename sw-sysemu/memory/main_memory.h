/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAIN_MEMORY_H
#define BEMU_MAIN_MEMORY_H

#include <array>
#include <cstdint>
#include <functional>
#include "emu_defines.h"
#include "literals.h"
#include "mailbox_region.h"
#include "maxion_region.h"
#include "memory_error.h"
#include "memory_region.h"
#include "null_region.h"
#include "peripheral_region.h"
#include "scratch_region.h"
#include "sparse_region.h"
#include "svcproc_region.h"
#include "sysreg_region.h"

namespace bemu {


// ET-SoC Memory map
//
// +---------------------------------+----------+-------------------+
// |      Address range (hex)        |          |                   |
// |      From      |      To        |   Size   | Maps to           |
// +----------------+----------------+----------+-------------------+
// | 0x00_0000_0000 | 0x00_3FFF_FFFF |    1GiB  | PU region         |
// | 0x00_0000_0000 | 0x00_0FFF_FFFF |  256MiB  |   Maxion          |
// | 0x00_1000_0000 | 0x00_1FFF_FFFF |  256MiB  |   IO              |
// | 0x00_1200_2000 | 0x00_1200_2FFF |    4KiB  |     PU/UART       |
// | 0x00_1200_7000 | 0x00_1200_7FFF |    4KiB  |     PU/UART1      |
// | 0x00_2000_0000 | 0x00_3FFF_FFFF |  512MiB  |   Mailbox         |
// | 0x00_2000_4000 | 0x00_2000_4FFF |    4KiB  |     PU/SRAM_MM_MX |
// | 0x00_2000_5000 | 0x00_2000_5FFF |    4KiB  |     PU/MBOX_MM_MX |
// | 0x00_2000_6000 | 0x00_2000_6FFF |    4KiB  |     PU/MBOX_MM_SP |
// | 0x00_2000_7000 | 0x00_2000_7FFF |    4KiB  |     PU/MBOX_PC_MM |
// | 0x00_2000_8000 | 0x00_2000_FFFF |   32KiB  |     PU/SRAM_LO    |
// | 0x00_2001_0000 | 0x00_2001_FFFF |   64KiB  |     PU/SRAM_MID   |
// | 0x00_2002_0000 | 0x00_2003_FFFF |  128KiB  |     PU/SRAM_HI    |
// | 0x00_3000_0000 | 0x00_3000_0FFF |    4KiB  |     PU/MBOX_MX_SP |
// | 0x00_3000_1000 | 0x00_3000_1FFF |    4KiB  |     PU/MBOX_PC_MX |
// | 0x00_3000_2000 | 0x00_3000_2FFF |    4KiB  |     PU/MBOX_SPARE |
// | 0x00_3000_3000 | 0x00_3000_3FFF |    4KiB  |     PU/MBOX_PC_SP |
// | 0x00_4000_0000 | 0x00_7FFF_FFFF |    1GiB  | SP region         |
// | 0x00_4000_0000 | 0x00_4001_FFFF |  128KiB  |   SP/ROM          |
// | 0x00_4040_0000 | 0x00_404F_FFFF |    1MiB  |   SP/SRAM         |
// | 0x00_8000_0000 | 0x00_FFFF_FFFF |    2GiB  | SCP region        |
// | 0x01_0000_0000 | 0x01_FFFF_FFFF |    4GiB  | ESR region        |
// | 0x02_0000_0000 | 0x3F_FFFF_FFFF |  256GiB  | Reserved          |
// | 0x40_0000_0000 | 0x7F_FFFF_FFFF |  512GiB  | PCIe region       |
// | 0x80_0000_0000 | 0xFF_FFFF_FFFF |  512GiB  | DRAM region       |
// | 0x80_0000_0000 | 0x80_001F_FFFF |    2MiB  |   DRAM/Mcode      |
// | 0x80_0020_0000 | 0x80_FFFF_FFFF |    4GiB  |   DRAM/OS         |
// | 0x81_0000_0000 | 0x87_FFFF_FFFF |   28GiB  |   DRAM/Other      |
// | 0x88_0000_0000 | 0xBF_FFFF_FFFF |  224GiB  |   Reserved        |
// | 0xC0_0000_0000 | 0xC0_001F_FFFF |    2MiB  |   DRAM/Mcode      |
// | 0xC0_0020_0000 | 0xC0_FFFF_FFFF |    4GiB  |   DRAM/OS         |
// | 0xC1_0000_0000 | 0xC7_FFFF_FFFF |   28GiB  |   DRAM/Other      |
// | 0xC8_0000_0000 | 0xFF_FFFF_FFFF |  224GiB  |   Reserved        |
// +----------------+----------------+----------+-------------------+
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

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        pu_maxion_base      = 0x0000000000ULL,
        pu_io_base          = 0x0010000000ULL,
        pu_mbox_base        = 0x0020000000ULL,
        spio_base           = 0x0040000000ULL,
        scp_base            = 0x0080000000ULL,
        sysreg_base         = 0x0100000000ULL,
        pcie_base           = 0x4000000000ULL,
        dram_base           = 0x8000000000ULL,
    };

    void read(addr_type addr, size_type n, void* result) {
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

    addr_type first() const { return pu_maxion_space.first(); }
    addr_type last() const { return dram_space.last(); }

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
    MaxionRegion     <pu_maxion_base, 256_MiB>          pu_maxion_space{};
    PeripheralRegion <pu_io_base, 256_MiB>              pu_io_space{};
    MailboxRegion    <pu_mbox_base, 512_MiB>            pu_mbox_space{};
    SvcProcRegion    <spio_base, 1_GiB>                 spio_space{};
    ScratchRegion    <scp_base, 4_MiB, EMU_NUM_SHIRES>  scp_space{};
    SysregRegion     <sysreg_base, 4_GiB>               sysreg_space{};
    NullRegion       <pcie_base, 256_GiB>               pcie_space{};
    SparseRegion     <dram_base, EMU_DRAM_SIZE, 16_MiB> dram_space{};

protected:
    static inline bool above(const MemoryRegion* lhs, addr_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw memory_error(addr);
        if (addr+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::MainMemory::search()");
        return *lo;
    }

    // This array must be sorted by region base address
    std::array<MemoryRegion*,8> regions = {{
        &pu_maxion_space,
        &pu_io_space,
        &pu_mbox_space,
        &spio_space,
        &scp_space,
        &sysreg_space,
        &pcie_space,
        &dram_space
    }};
};


} // namespace bemu

#endif // BEMU_MAIN_MEMORY_H
