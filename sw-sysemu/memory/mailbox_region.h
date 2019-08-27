/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MAILBOX_REGION_H
#define BEMU_MAILBOX_REGION_H

#include <array>
#include <cstdint>
#include <functional>
#include "dense_region.h"
#include "emu_defines.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"
#include "sparse_region.h"

extern uint32_t current_thread;

namespace bemu {


extern typename MemoryRegion::value_type memory_reset_value;


template<unsigned long long Base, size_t N>
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
    DenseRegion  <pu_trg_mmin_pos, 8_KiB>       pu_trg_mmin{};
    DenseRegion  <pu_trg_mmin_sp_pos, 8_KiB>    pu_trg_mmin_sp{};
    DenseRegion  <pu_sram_mm_mx_pos, 4_KiB>     pu_sram_mm_mx{};
    DenseRegion  <pu_mbox_mm_mx_pos, 4_KiB>     pu_mbox_mm_mx{};
    DenseRegion  <pu_mbox_mm_sp_pos, 4_KiB>     pu_mbox_mm_sp{};
    DenseRegion  <pu_mbox_pc_mm_pos, 4_KiB>     pu_mbox_pc_mm{};
    SparseRegion <pu_sram_pos, 224_KiB, 16_KiB> pu_sram{};
    DenseRegion  <pu_mbox_mx_sp_pos, 4_KiB>     pu_mbox_mx_sp{};
    DenseRegion  <pu_mbox_pc_mx_pos, 4_KiB>     pu_mbox_pc_mx{};
    DenseRegion  <pu_mbox_spare_pos, 4_KiB>     pu_mbox_spare{};
    DenseRegion  <pu_mbox_pc_sp_pos, 4_KiB>     pu_mbox_pc_sp{};
    DenseRegion  <pu_trg_max_pos, 8_KiB>        pu_trg_max{};
    DenseRegion  <pu_trg_max_sp_pos, 8_KiB>     pu_trg_max_sp{};
    DenseRegion  <pu_trg_pcie_pos, 8_KiB>       pu_trg_pcie{};
    DenseRegion  <pu_trg_pcie_sp_pos , 8_KiB>   pu_trg_pcie_sp{};

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
