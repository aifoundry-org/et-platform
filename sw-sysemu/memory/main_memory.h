/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MAIN_MEMORY_H
#define BEMU_MAIN_MEMORY_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include "agent.h"
#include "literals.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

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
// | 0x80_0000_0000 | 0x80_007F_FFFF |    2MiB  |   DRAM/Mcode      |
// | 0x80_0080_0000 | 0x80_FFFF_FFFF |    4GiB  |   DRAM/OS         |
// | 0x81_0000_0000 | 0x87_FFFF_FFFF |   28GiB  |   DRAM/Other      |
// | 0x88_0000_0000 | 0xBF_FFFF_FFFF |  224GiB  |   Reserved        |
// | 0xC0_0000_0000 | 0xC0_007F_FFFF |    2MiB  |   DRAM/Mcode      |
// | 0xC0_0080_0000 | 0xC0_FFFF_FFFF |    4GiB  |   DRAM/OS         |
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

    // ----- Types -----

    struct pcie_iatu_info_t {
        uint32_t ctrl_1;
        uint32_t ctrl_2;
        uint32_t lwr_base_addr;
        uint32_t upper_base_addr;
        uint32_t limit_addr;
        uint32_t lwr_target_addr;
        uint32_t upper_target_addr;
        uint32_t uppr_limit_addr;
    };

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        pu_maxion_base      = 0x0000000000ULL,
        pu_io_base          = 0x0010000000ULL,
        pu_mbox_base        = 0x0020000000ULL,
        spio_base           = 0x0040000000ULL,
        scp_base            = 0x0080000000ULL,
        sysreg_base         = 0x0100000000ULL,
#ifdef SYS_EMU
        pcie_base           = 0x4000000000ULL,
#endif
        dram_base           = 0x8000000000ULL,
    };

    // ----- Public methods -----

    void reset();

    void read(const Agent& agent, addr_type addr, size_type n, void* result) {
        const auto elem = search(addr, n);
        elem->read(agent, addr - elem->first(), n, reinterpret_cast<pointer>(result));
    }

    void write(const Agent& agent, addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->write(agent, addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    void init(const Agent& agent, addr_type addr, size_type n, const void* source) {
        auto elem = search(addr, n);
        elem->init(agent, addr - elem->first(), n, reinterpret_cast<const_pointer>(source));
    }

    addr_type first() const { return regions.front()->first(); }
    addr_type last() const { return regions.back()->last(); }

    void dump_data(const Agent& agent, std::ostream& os, addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        auto hi = std::lower_bound(regions.cbegin(), regions.cend(), addr+n-1, above);
        if (hi == regions.cend())
            throw std::out_of_range("bemu::MainMemory::dump_data()");
        size_type pos = addr - (*lo)->first();
        while (lo != hi) {
            (*lo)->dump_data(agent, os, pos, (*lo)->last() - (*lo)->first() - pos + 1);
            ++lo;
            pos = 0;
        }
        (*lo)->dump_data(agent, os, pos, addr + n - (*lo)->first() - pos);
    }

    // Access the PLICs
    void pu_plic_interrupt_pending_set(const Agent&, uint32_t source);
    void pu_plic_interrupt_pending_clear(const Agent&, uint32_t source);
    void sp_plic_interrupt_pending_set(const Agent&, uint32_t source);
    void sp_plic_interrupt_pending_clear(const Agent&, uint32_t source);

    // Access the UARTs
    void pu_uart0_set_rx_fd(int fd);
    void pu_uart1_set_rx_fd(int fd);
    int pu_uart0_get_rx_fd() const;
    int pu_uart1_get_rx_fd() const;
    void spio_uart0_set_rx_fd(int fd);
    void spio_uart1_set_rx_fd(int fd);
    int spio_uart0_get_rx_fd() const;
    int spio_uart1_get_rx_fd() const;
    void pu_uart0_set_tx_fd(int fd);
    void pu_uart1_set_tx_fd(int fd);
    int pu_uart0_get_tx_fd() const;
    int pu_uart1_get_tx_fd() const;
    void spio_uart0_set_tx_fd(int fd);
    void spio_uart1_set_tx_fd(int fd);
    int spio_uart0_get_tx_fd() const;
    int spio_uart1_get_tx_fd() const;

    // Access the RISC-V timers
    bool pu_rvtimer_is_active() const;
    uint64_t pu_rvtimer_read_mtime() const;
    uint64_t pu_rvtimer_read_mtimecmp() const;
    void pu_rvtimer_update(const Agent&, uint64_t cycle);
    void pu_rvtimer_write_mtime(const Agent&, uint64_t cycle);
    void pu_rvtimer_write_mtimecmp(const Agent&, uint64_t cycle);
    bool spio_rvtimer_is_active() const;
    void spio_rvtimer_update(const Agent&, uint64_t cycle);

    // Access the DW APB timers
    void pu_apb_timers_update(System& chip, uint64_t cycle);
    void spio_apb_timers_update(System& chip, uint64_t cycle);

    // Access the Mailboxes
    void pc_mm_mailbox_read(const Agent& agent, addr_type offset, size_type n, void* result);
    void pc_mm_mailbox_write(const Agent& agent, addr_type addr, size_type n, const void* source);
    void pc_sp_mailbox_read(const Agent& agent, addr_type offset, size_type n, void* result);
    void pc_sp_mailbox_write(const Agent& agent, addr_type addr, size_type n, const void* source);

    // Access PCIe
    void pu_trg_pcie_mmm_int_inc(const Agent& agent);
    void pu_trg_pcie_ipi_trigger(const Agent& agent);
    void pcie0_dbi_slv_trigger_done_int(const Agent&, bool wrch, int channel);
#ifdef SYS_EMU
    std::array<pcie_iatu_info_t, ETSOC_CX_ATU_NUM_INBOUND_REGIONS>& pcie0_get_iatus();
#endif

protected:
    static inline bool above(const std::unique_ptr<MemoryRegion>& lhs, addr_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(addr_type addr, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), addr, above);
        if ((lo == regions.cend()) || ((*lo)->first() > addr))
            throw memory_error(addr);
        if (addr+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::MainMemory::search()");
        return lo->get();
    }

    // This array must be sorted by region base address
#ifdef SYS_EMU
    std::array<std::unique_ptr<MemoryRegion>, 8> regions{};
#else
    std::array<std::unique_ptr<MemoryRegion>, 7> regions{};
#endif
};


} // namespace bemu

#endif // BEMU_MAIN_MEMORY_H
