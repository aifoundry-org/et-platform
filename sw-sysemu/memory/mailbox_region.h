/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAILBOX_REGION_H
#define BEMU_MAILBOX_REGION_H

#include <array>
#include <cstdint>
#include <functional>
#include "emu_defines.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"
#include "dense_region.h"
#include "sparse_region.h"

// From emu.cpp
extern unsigned current_thread;
extern void pu_plic_interrupt_pending_set(uint32_t source_id);
extern void pu_plic_interrupt_pending_clear(uint32_t source_id);

namespace bemu {


extern typename MemoryRegion::value_type memory_reset_value;

template <unsigned long long Base, unsigned long long N>
struct PU_TRG_MMin_Region : public MemoryRegion
{
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        MMM_INT_INC = 0x4,
        PCI_INT_DEC = 0x8,
        PCI_MMM_CNT = 0xC,
    };

    void read(size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case MMM_INT_INC:
            *result32 = 0;
            break;
        case PCI_INT_DEC:
            *result32 = 0;
            break;
        case PCI_MMM_CNT:
            *result32 = counter;
            break;
        }
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case MMM_INT_INC:
            break;
        case PCI_INT_DEC:
            counter -= *source32 & 1;
            check_trigger_int();
            break;
        case PCI_MMM_CNT:
            break;
        }
    }

    void init(size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::MailboxRegion::PU_TRG_MMin_Region::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    void interrupt_inc() { ++counter; check_trigger_int(); }

protected:
    void check_trigger_int() {
        // PU_PLIC_PCIE_MESSAGE_INTR_ID
        if (counter > 0) {
            pu_plic_interrupt_pending_set(33);
        } else {
            pu_plic_interrupt_pending_clear(33);
        }
    }

    uint32_t counter = 0;
};

template<unsigned long long Base, unsigned long long N>
struct MailboxRegion : public MemoryRegion {
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

    void read(size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            std::fill_n(result, n, memory_reset_value);
            return;
        }
        elem->read(pos - elem->first(), n, result);
    }

    void write(size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(pos, n);
        if (elem) {
            try {
                elem->write(pos - elem->first(), n, source);
            } catch (const memory_error&) {
                throw memory_error(first() + pos);
            }
        }
    }

    void init(size_type pos, size_type n, const_pointer source) {
        const auto elem = search(pos, n);
        if (!elem)
            throw std::runtime_error("bemu::MailboxRegion::init()");
        elem->init(pos - elem->first(), n, source);
    }

    addr_type first() const { return Base; }
    addr_type last() const { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // Members
    PU_TRG_MMin_Region <pu_trg_mmin_pos, 8_KiB>       pu_trg_mmin{};
    DenseRegion        <pu_trg_mmin_sp_pos, 8_KiB>    pu_trg_mmin_sp{};
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
    DenseRegion        <pu_trg_pcie_pos, 8_KiB>       pu_trg_pcie{};
    DenseRegion        <pu_trg_pcie_sp_pos , 8_KiB>   pu_trg_pcie_sp{};

protected:
    static inline bool above(const MemoryRegion* lhs, size_type rhs) {
        return lhs->last() < rhs;
    }

    template<size_t M>
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

    MemoryRegion* search(size_type pos, size_type n) const {
        return (current_thread == EMU_IO_SHIRE_SP_THREAD)
                ? search(spio_regions, pos, n)
                : search(minion_regions, pos, n);
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
};


} // namespace bemu

#endif // BEMU_MAILBOX_REGION_H
