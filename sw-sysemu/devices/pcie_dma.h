/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PCIE_DMA_H
#define BEMU_PCIE_DMA_H

#include <array>
#include <cstdint>
#include "agent.h"
#include "emu_defines.h"

namespace bemu {

template<bool wrch>
struct PcieDma {
    void go(const Agent& agent);

    int chan_id;
    uint32_t ch_control1 = 0;
    uint32_t llp_low = 0;
    uint32_t llp_high = 0;
    uint32_t engine_en = 0;

private:
    void trigger_done_int(const Agent& agent);

    bool liep = false;
};

} // namespace bemu

#endif // BEMU_PCIE_DMA_H
