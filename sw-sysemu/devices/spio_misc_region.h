/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_SPIO_MISC_REGION_H
#define BEMU_SPIO_MISC_REGION_H

#include "memory/memory_region.h"

namespace bemu {


struct SpioMiscRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override;
    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override;

    void init(const Agent&, size_type, size_type, const_pointer) override;

    addr_type first() const override { return 0x12029000ULL; }
    addr_type last() const override { return first() + 4096 - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }
};


} // namespace bemu

#endif // BEMU_SPIO_MISC_REGION_H
