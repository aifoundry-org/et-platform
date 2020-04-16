/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PERIPHERAL_REGION_H
#define BEMU_PERIPHERAL_REGION_H

#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include "devices/plic.h"
#include "devices/pu_uart.h"
#include "literals.h"
#include "memory_error.h"
#include "memory_region.h"

namespace bemu {


extern typename MemoryRegion::reset_value_type memory_reset_value;


template<unsigned long long Base, unsigned long long N>
struct PeripheralRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 256_MiB, "bemu::PeripheralRegion has illegal size");

    enum : unsigned long long {
        // base addresses for the various regions of the address space
        pu_plic_base    = 0x00000000,
        pu_uart_base    = 0x02002000,
        pu_uart1_base   = 0x02007000,
    };

    void read(size_type pos, size_type n, pointer result) override {
        const auto elem = search(pos, n);
        if (!elem) {
            default_value(result, n, memory_reset_value, pos);
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

    void init(size_type pos, size_type n, const_pointer source) override {
        const auto elem = search(pos, n);
        if (!elem)
            throw std::runtime_error("bemu::PeripheralRegion::init()");
        elem->init(pos - elem->first(), n, source);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    // Members
    PU_PLIC <pu_plic_base,  32_MiB>  pu_plic{};
    PU_Uart <pu_uart_base,   4_KiB>  pu_uart{};
    PU_Uart <pu_uart1_base,  4_KiB>  pu_uart1{};

protected:
    static inline bool above(const MemoryRegion* lhs, size_type rhs) {
        return lhs->last() < rhs;
    }

    MemoryRegion* search(size_type pos, size_type n) const {
        auto lo = std::lower_bound(regions.cbegin(), regions.cend(), pos, above);
        if ((lo == regions.cend()) || ((*lo)->first() > pos))
            return nullptr;
        if (pos+n-1 > (*lo)->last())
            throw std::out_of_range("bemu::PeripheralRegion::search()");
        return *lo;
    }

    // These arrays must be sorted by region offset
    std::array<MemoryRegion*,3> regions = {{
        &pu_plic,
        &pu_uart,
        &pu_uart1,
    }};
};


} // namespace bemu

#endif // BEMU_PERIPHERAL_REGION_H
