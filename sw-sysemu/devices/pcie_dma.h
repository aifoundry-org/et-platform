/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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
