/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "devices/spio_misc_region.h"
#include "system.h"
#include <stdexcept>


// IOshire SP addresses
#define ESR_DMCTRL              0x8ULL
#define ESR_AND_OR_TREEL2       0xCULL


namespace bemu {


void SpioMiscRegion::read(const Agent& agent, size_type pos,
                          size_type n, pointer result)
{
    if (n != 4) {
        throw memory_error(first() + pos);
    }
    uint32_t value = 0;
    switch (pos) {
    case ESR_DMCTRL:
        value = agent.chip->read_dmctrl();
        break;
    case ESR_AND_OR_TREEL2:
        value = agent.chip->read_andortree2();
        break;
    default:
        throw memory_error(first() + pos);
    }
    *reinterpret_cast<uint32_t*>(result) = value;
}


void SpioMiscRegion::write(const Agent& agent, size_type pos,
                           size_type n, const_pointer source)
{
    if (n != 4) {
        throw memory_error(first() + pos);
    }
    if (pos != ESR_DMCTRL) {
        // The only writeable ESR in this region is DMCTRL
        throw memory_error(first() + pos);
    }
    agent.chip->write_dmctrl(*reinterpret_cast<const uint32_t*>(source));

}


void SpioMiscRegion::init(const Agent&, size_type, size_type, const_pointer)
{
    throw std::runtime_error("bemu::SpioMiscRegion::init()");
}


} // namespace bemu
