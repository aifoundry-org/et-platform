/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MAILBOX_REGION_H
#define BEMU_MAILBOX_REGION_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include "emu_gio.h"
#include "literals.h"
#include "processor.h"
#include "system.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "memory/dense_region.h"
#include "memory/sparse_region.h"

namespace bemu {

enum : unsigned long long {
    IPI_TRIGGER = 0x0,
    MMM_INT_INC = 0x4,
    PCI_INT_DEC = 0x8,
    PCI_MMM_CNT = 0xC,
};

enum : unsigned long long { // Starting at +0x2000
    IPI_CLEAR = 0x0,
};

struct IMailboxInterrupts {
    virtual void pcie_interrupt_counter_inc(System*) = 0;
    virtual void pcie_interrupt_counter_dec(System*) = 0;
    virtual uint32_t pcie_interrupt_counter_get() const = 0;

    virtual void mm_to_sp_interrupt_ipi_trigger_write(System*, uint32_t value) = 0;
    virtual void mm_to_sp_interrupt_ipi_clear_write(System*, uint32_t value) = 0;
    virtual uint32_t mm_to_sp_interrupt_reg_get() const = 0;

    virtual void host_to_sp_interrupt_ipi_trigger_write(System*, uint32_t value) = 0;
    virtual void host_to_sp_interrupt_ipi_clear_write(System*, uint32_t value) = 0;
    virtual uint32_t host_to_sp_interrupt_reg_get() const = 0;
};

template <unsigned long long Base, unsigned long long N>
struct PU_TRG_MMin : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    PU_TRG_MMin(IMailboxInterrupts &mb) : mb_(mb) {}

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_TRIGGER:
            *result32 = mb_.mm_to_sp_interrupt_reg_get();
            break;
        case PCI_INT_DEC:
            *result32 = 0;
            break;
        case PCI_MMM_CNT:
            *result32 = mb_.pcie_interrupt_counter_get();
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PU_TRG_MMin::write(pos=0x%llx, data32 = 0x%" PRIx32 ")", pos, *source32);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_TRIGGER:
            mb_.mm_to_sp_interrupt_ipi_trigger_write(agent.chip, *source32);
            break;
        case PCI_INT_DEC:
            if (*source32 & 1)
                mb_.pcie_interrupt_counter_dec(agent.chip);
            break;
        case PCI_MMM_CNT:
            // Read-only
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PU_TRG_MMin::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    IMailboxInterrupts& mb_;
};

template <unsigned long long Base, unsigned long long N>
struct PU_TRG_MMin_SP : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    PU_TRG_MMin_SP(IMailboxInterrupts &mb) : mb_(mb) {}

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PU_TRG_MMin_SP::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_CLEAR:
            *result32 = mb_.mm_to_sp_interrupt_reg_get();
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PU_TRG_MMin_SP::write(pos=0x%llx, data32 = 0x%" PRIx32 ")", pos, *source32);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_CLEAR:
            mb_.mm_to_sp_interrupt_ipi_clear_write(agent.chip, *source32);
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PU_TRG_MMin_SP::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    IMailboxInterrupts& mb_;
};

template <unsigned long long Base, unsigned long long N>
struct PU_TRG_PCIe : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    PU_TRG_PCIe(IMailboxInterrupts &mb) : mb_(mb) {}

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_TRIGGER:
            *result32 = mb_.host_to_sp_interrupt_reg_get();
            break;
        case MMM_INT_INC:
            *result32 = 0;
            break;
        case PCI_MMM_CNT:
            *result32 = mb_.pcie_interrupt_counter_get();
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PU_TRG_PCIe::write(pos=0x%llx, data32 = 0x%" PRIx32 ")", pos, *source32);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_TRIGGER:
            mb_.host_to_sp_interrupt_ipi_trigger_write(agent.chip, *source32);
            break;
        case MMM_INT_INC:
            if (*source32 & 1)
                mb_.pcie_interrupt_counter_inc(agent.chip);
            break;
        case PCI_MMM_CNT:
            // Read-only
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PU_TRG_PCIe::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    IMailboxInterrupts& mb_;
};

template <unsigned long long Base, unsigned long long N>
struct PU_TRG_PCIe_SP : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    PU_TRG_PCIe_SP(IMailboxInterrupts &mb) : mb_(mb) {}

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PU_TRG_PCIe_SP::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_CLEAR:
            *result32 = mb_.host_to_sp_interrupt_reg_get();
            break;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PU_TRG_PCIe_SP::write(pos=0x%llx, data32 = 0x%" PRIx32 ")", pos, *source32);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case IPI_CLEAR:
            mb_.host_to_sp_interrupt_ipi_clear_write(agent.chip, *source32);
            break;
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::PU_TRG_PCIe_SP::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    IMailboxInterrupts& mb_;
};

template<unsigned long long Base, unsigned long long N>
struct MailboxRegion : public MemoryRegion, public IMailboxInterrupts
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 512_MiB, "bemu::MailboxRegion has illegal size");

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        pu_trg_mmin_pos    = 0x00000000,
        pu_trg_mmin_sp_pos = 0x00002000,
        pu_sram_mm_mx_pos  = 0x00004000,
        pu_mbox_mm_mx_pos  = 0x00005000,
        pu_mbox_mm_sp_pos  = 0x00006000,
        pu_mbox_pc_mm_pos  = 0x00007000,
        pu_sram_pos        = 0x00008000,
        pu_mbox_mx_sp_pos  = 0x10000000,
        pu_mbox_pc_mx_pos  = 0x10001000,
        pu_mbox_spare_pos  = 0x10002000,
        pu_mbox_pc_sp_pos  = 0x10003000,
        pu_trg_max_pos     = 0x10004000,
        pu_trg_max_sp_pos  = 0x10006000,
        pu_trg_pcie_pos    = 0x10008000,
        pu_trg_pcie_sp_pos = 0x1000A000,
    };

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        const auto elem = search(agent, pos, n);
        if (!elem) {
            default_value(result, n, agent.chip->memory_reset_value, pos);
            return;
        }
        elem->read(agent, pos - elem->first(), n, result);
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(agent, pos, n);
        if (elem) {
            try {
                elem->write(agent, pos - elem->first(), n, source);
            } catch (const memory_error&) {
                throw memory_error(first() + pos);
            }
        }
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(agent, pos, n);
        if (!elem)
            throw std::runtime_error("bemu::MailboxRegion::init()");
        elem->init(agent, pos - elem->first(), n, source);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    void pcie_interrupt_counter_inc(System* system) override {
        pcie_interrupt_counter++;
        pcie_interrupt_check_trigger(system);
    }

    void pcie_interrupt_counter_dec(System* system) override {
        pcie_interrupt_counter--;
        pcie_interrupt_check_trigger(system);
    }

    uint32_t pcie_interrupt_counter_get() const override {
        return pcie_interrupt_counter;
    }

    void mm_to_sp_interrupt_ipi_trigger_write(System* system, uint32_t value) override {
        mm_to_sp_interrupt_reg |= value;
        mm_to_sp_interrupt_check_trigger(system);
    }

    void mm_to_sp_interrupt_ipi_clear_write(System* system, uint32_t value) override {
        mm_to_sp_interrupt_reg &= ~value;
        mm_to_sp_interrupt_check_trigger(system);
    }

    uint32_t mm_to_sp_interrupt_reg_get() const override {
        return mm_to_sp_interrupt_reg;
    }

    void host_to_sp_interrupt_ipi_trigger_write(System* system, uint32_t value) override {
        host_to_sp_interrupt_reg |= value;
        host_to_sp_interrupt_check_trigger(system);
    }

    void host_to_sp_interrupt_ipi_clear_write(System* system, uint32_t value) override {
        host_to_sp_interrupt_reg &= ~value;
        host_to_sp_interrupt_check_trigger(system);
    }

    uint32_t host_to_sp_interrupt_reg_get() const override {
        return host_to_sp_interrupt_reg;
    }

    // Members
    PU_TRG_MMin        <pu_trg_mmin_pos, 8_KiB>       pu_trg_mmin{*this};
    PU_TRG_MMin_SP     <pu_trg_mmin_sp_pos, 8_KiB>    pu_trg_mmin_sp{*this};
    DenseRegion        <pu_sram_mm_mx_pos, 4_KiB>     pu_sram_mm_mx{};
    DenseRegion        <pu_mbox_mm_mx_pos, 4_KiB>     pu_mbox_mm_mx{};
    DenseRegion        <pu_mbox_mm_sp_pos, 4_KiB>     pu_mbox_mm_sp{};
    DenseRegion        <pu_mbox_pc_mm_pos, 4_KiB>     pu_mbox_pc_mm{};
    SparseRegion       <pu_sram_pos, 224_KiB, 16_KiB> pu_sram{};
    DenseRegion        <pu_mbox_mx_sp_pos, 4_KiB>     pu_mbox_mx_sp{};
    DenseRegion        <pu_mbox_pc_mx_pos, 4_KiB>     pu_mbox_pc_mx{};
    DenseRegion        <pu_mbox_spare_pos, 4_KiB>     pu_mbox_spare{};
    DenseRegion        <pu_mbox_pc_sp_pos, 4_KiB>     pu_mbox_pc_sp{};
    DenseRegion        <pu_trg_max_pos, 8_KiB>        pu_trg_max{};
    DenseRegion        <pu_trg_max_sp_pos, 8_KiB>     pu_trg_max_sp{};
    PU_TRG_PCIe        <pu_trg_pcie_pos, 8_KiB>       pu_trg_pcie{*this};
    PU_TRG_PCIe_SP     <pu_trg_pcie_sp_pos , 8_KiB>   pu_trg_pcie_sp{*this};

protected:

    void pcie_interrupt_check_trigger(System* system) {
        if (pcie_interrupt_counter > 0) {
            system->pu_plic_interrupt_pending_set(PU_PLIC_PCIE_MESSAGE_INTR_ID);
        } else {
            system->pu_plic_interrupt_pending_clear(PU_PLIC_PCIE_MESSAGE_INTR_ID);
        }
    }

    void mm_to_sp_interrupt_check_trigger(System* system) {
        if (mm_to_sp_interrupt_reg != 0) {
            system->sp_plic_interrupt_pending_set(SPIO_PLIC_MBOX_MMIN_INTR_ID);
        } else {
            system->sp_plic_interrupt_pending_clear(SPIO_PLIC_MBOX_MMIN_INTR_ID);
        }
    }

    void host_to_sp_interrupt_check_trigger(System* system) {
        if (host_to_sp_interrupt_reg != 0) {
            system->sp_plic_interrupt_pending_set(SPIO_PLIC_MBOX_HOST_INTR_ID);
        } else {
            system->sp_plic_interrupt_pending_clear(SPIO_PLIC_MBOX_HOST_INTR_ID);
        }
    }

    static inline bool above(const MemoryRegion* lhs, size_type rhs) {
        return lhs->last() < rhs;
    }

    template<std::size_t M>
    MemoryRegion* search(const std::array<MemoryRegion*,M>& cont,
                         size_type pos, size_type n) const
    {
        auto lo = std::lower_bound(cont.cbegin(), cont.cend(), pos, above);
        if ((lo == cont.cend()) || ((*lo)->first() > pos))
            return nullptr;
        if (pos+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::MailboxRegion::search()");
        return *lo;
    }

    //
    // The specification says that access is controlled as follows:
    // Mailbox:
    // ----------------+----+----+------------+------------+----+----+----+-----
    // Region name     | SvcProc |         Minion          |  Maxion | PCIe host
    //                 | Rd | Wr | Rd         | Wr         | Rd | Wr | Rd | Wr
    // ----------------+----+----+------------+------------+----+----+----+-----
    // R_PU_MBOX_MM_MX | Y  | Y  | mprot[1:0] | mprot[1:0] | Y  | Y  | N  | N
    // R_PU_MBOX_MM_SP | Y  | Y  | mprot[1:0] | mprot[1:0] | N  | N  | N  | N
    // R_PU_MBOX_PC_MM | Y  | Y  | mprot[1:0] | mprot[1:0] | N  | N  | Y  | Y
    // R_PU_MBOX_MX_SP | Y  | Y  | N          | N          | Y  | Y  | N  | N
    // R_PU_MBOX_PC_MX | Y  | Y  | N          | N          | Y  | Y  | Y  | Y
    // R_PU_MBOX_SPARE | Y  | Y  | N          | N          | N  | N  | N  | N
    // R_PU_MBOX_PC_SP | Y  | Y  | N          | N          | N  | N  | Y  | Y
    // ----------------+----+----+------------+------------+----+----+----+-----
    //
    // Trigger:
    // -----------------+-----------------------------
    // Region           | Access
    // -----------------+-----------------------------
    // R_PU_TRG_MMIN    | Master/mprot[1:0] (+SvcProc)
    // R_PU_TRG_MMIN_SP | SvcProc
    // R_PU_TRG_MAX     | Maxion (+SvcProc)
    // R_PU_TRG_MAX_SP  | SvcProc
    // R_PU_TRG_PCIE    | PCIe Host (+SvcProc)
    // R_PU_TRG_PCIE_SP | SvcProc
    // -----------------+-----------------------------
    //

    MemoryRegion* search(const Agent& agent, size_type pos, size_type n) const {
        try {
            const Hart& cpu = dynamic_cast<const Hart&>(agent);
            return (cpu.mhartid == IO_SHIRE_SP_HARTID)
                ? search(spio_regions, pos, n)
                : search(minion_regions, pos, n);
        }
        catch (const std::bad_cast&) {
            // Assume non-Hart Agent is PCIe/Host
            // TODO: GDB stub will not be able to access all the regions...
            return search(pcie_regions, pos, n);
        }
    }

    // These arrays must be sorted by region offset
    std::array<MemoryRegion*,15> spio_regions = {{
        &pu_trg_mmin,
        &pu_trg_mmin_sp,
        &pu_sram_mm_mx,
        &pu_mbox_mm_mx,
        &pu_mbox_mm_sp,
        &pu_mbox_pc_mm,
        &pu_sram,
        &pu_mbox_mx_sp,
        &pu_mbox_pc_mx,
        &pu_mbox_spare,
        &pu_mbox_pc_sp,
        &pu_trg_max,
        &pu_trg_max_sp,
        &pu_trg_pcie,
        &pu_trg_pcie_sp,
    }};
    std::array<MemoryRegion*,6> minion_regions = {{
        &pu_trg_mmin,
        &pu_sram_mm_mx,
        &pu_mbox_mm_mx,
        &pu_mbox_mm_sp,
        &pu_mbox_pc_mm,
        &pu_sram,
    }};
    std::array<MemoryRegion*,5> pcie_regions = {{
        &pu_mbox_pc_mm,
        &pu_sram,
        &pu_mbox_pc_mx,
        &pu_mbox_pc_sp,
        &pu_trg_pcie,
    }};

    std::atomic<uint32_t> pcie_interrupt_counter{0};
    std::atomic<uint32_t> mm_to_sp_interrupt_reg{0};
    std::atomic<uint32_t> host_to_sp_interrupt_reg{0};
};


} // namespace bemu

#endif // BEMU_MAILBOX_REGION_H
