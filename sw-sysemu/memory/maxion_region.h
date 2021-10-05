/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MAXION_REGION_H
#define BEMU_MAXION_REGION_H

#include <algorithm>
#include <stdexcept>
#include "emu_defines.h"
#include "literals.h"
#include "processor.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {


template<unsigned long long Base, unsigned long long N>
struct MaxionRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 256_MiB, "bemu::MaxionRegion has illegal size");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        try {
            const Hart& cpu = dynamic_cast<const Hart&>(agent);
            if (cpu.mhartid != IO_SHIRE_SP_HARTID)
                throw memory_error(first() + pos);
        }
        catch (const std::bad_cast&) {
            throw memory_error(first() + pos);
        }
        default_value(result, n, agent.chip->memory_reset_value, pos);
    }

    void write(const Agent& agent, size_type pos, size_type, const_pointer) override {
        try {
            const Hart& cpu = dynamic_cast<const Hart&>(agent);
            if (cpu.mhartid != IO_SHIRE_SP_HARTID)
                throw memory_error(first() + pos);
        }
        catch (const std::bad_cast&) {
            throw memory_error(first() + pos);
        }
    }

    void init(const Agent&, size_type, size_type, const_pointer) override {
        throw std::runtime_error("bemu::MaxionRegion::init()");
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_MAXION_REGION_H
