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

namespace bemu {


struct MemoryRegion
{
    typedef unsigned long long    addr_type;
    typedef unsigned long long    size_type;
    typedef unsigned char         value_type;
    typedef value_type            reset_value_type[MEM_RESET_PATTERN_SIZE];
    typedef value_type*           pointer;
    typedef const value_type*     const_pointer;
  
    virtual ~MemoryRegion() {}

    // Copies @n bytes starting from offset @pos into @result
    virtual void read(size_type pos, size_type n, pointer result) = 0;

    // Copies @n bytes from @source starting into offset @pos
    virtual void write(size_type pos, size_type n, const_pointer source) = 0;

    // Initialized @n bytes starting at offset @pos from values in @source
    virtual void init(size_type pos, size_type n, const_pointer source) = 0;

    // Returns the first valid address of this region
    virtual addr_type first() const = 0;

    // Returns the last valid address of this region
    virtual addr_type last() const = 0;

    // Outputs region data to a stream
    virtual void dump_data(std::ostream& os, size_type pos, size_type n) const = 0;

    static void default_value(pointer result, size_type n, reset_value_type pattern, size_type offset){
      for ( unsigned i = 0 ; i < n; i++)
        result[i] = pattern[(i + offset) % MEM_RESET_PATTERN_SIZE];
    }
};


} // namespace bemu

#endif // BEMU_MEMORY_REGION_H
