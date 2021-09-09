/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_MEMORY_REGION_H
#define BEMU_MEMORY_REGION_H

#include <cstddef>
#include <iosfwd>
#include "agent.h"
#include "emu_defines.h"

namespace bemu {


struct MemoryRegion
{
    using addr_type         = unsigned long long;
    using size_type         = unsigned long long;
    using value_type        = unsigned char;
    using reset_value_type  = value_type[MEM_RESET_PATTERN_SIZE];
    using pointer           = value_type*;
    using const_pointer     = const value_type*;

    virtual ~MemoryRegion() {}

    // Copies @n bytes starting from offset @pos into @result
    virtual void read(const Agent& agent, size_type pos, size_type n, pointer result) = 0;

    // Copies @n bytes from @source starting into offset @pos
    virtual void write(const Agent& agent, size_type pos, size_type n, const_pointer source) = 0;

    // Initialized @n bytes starting at offset @pos from values in @source
    virtual void init(const Agent& agent, size_type pos, size_type n, const_pointer source) = 0;

    // Returns the first valid address of this region
    virtual addr_type first() const = 0;

    // Returns the last valid address of this region
    virtual addr_type last() const = 0;

    // Outputs region data to a stream
    virtual void dump_data(const Agent& agent, std::ostream& os, size_type pos, size_type n) const = 0;

    static void default_value(pointer result, size_type n,
                              const reset_value_type& pattern, size_type offset)
    {
        for (unsigned i = 0 ; i < n; ++i) {
            result[i] = pattern[(i + offset) % MEM_RESET_PATTERN_SIZE];
        }
    }
};


} // namespace bemu

#endif // BEMU_MEMORY_REGION_H
