/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_PCIE_DBI_SLV_H
#define BEMU_PCIE_DBI_SLV_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif

#define ETSOC_CX_ATU_NUM_INBOUND_REGIONS 32

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct PcieDbiSlvRegion : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::value_type     value_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    enum : unsigned long long {
        PF0_TYPE0_HDR_STATUS_COMMAND_REG = 0x04,
        PF0_TYPE0_HDR_BAR0_REG_ADDRESS   = 0x10,
        PF0_TYPE0_HDR_BAR1_REG_ADDRESS   = 0x14,
        PF0_TYPE0_HDR_BAR2_REG_ADDRESS   = 0x18,
        PF0_TYPE0_HDR_BAR3_REG_ADDRESS   = 0x1c,
        PF0_TYPE0_HDR_BAR4_REG_ADDRESS   = 0x20,
        PF0_TYPE0_HDR_BAR5_REG_ADDRESS   = 0x24,
        PF0_MSI_CAP                      = 0x50,
        PF0_MSIX_CAP                     = 0xb0,
        PF0_ATU_CAP_ADDRESS              = 0x300000,
    };

    enum : unsigned long long {
        PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_i_OFFSET     = 0x00,
        PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_i_OFFSET     = 0x04,
        PF0_ATU_CAP_IATU_LWR_BASE_ADDR_OFF_INBOUND_i_OFFSET     = 0x08,
        PF0_ATU_CAP_IATU_UPPER_BASE_ADDR_OFF_INBOUND_i_OFFSET   = 0x0c,
        PF0_ATU_CAP_IATU_LIMIT_ADDR_OFF_INBOUND_i_OFFSET        = 0x10,
        PF0_ATU_CAP_IATU_LWR_TARGET_ADDR_OFF_INBOUND_i_OFFSET   = 0x14,
        PF0_ATU_CAP_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_i_OFFSET = 0x18,
        PF0_ATU_CAP_IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_i_OFFSET   = 0x20,
    };

    void read(const Agent&, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "PcieDbiSlvRegion::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case PF0_TYPE0_HDR_STATUS_COMMAND_REG:
            *result32 = (1u << 1); // MEM_SPACE_EN
            break;
        case PF0_MSI_CAP:
            *result32 = (1u << 16) | (0u << 20); /* PCI_MSI_ENABLE = 1, PCI_MSI_MULTIPLE_MSG_EN = 0 */
            break;
        case PF0_MSIX_CAP:
            *result32 = (1u << 31); /* PCI_MSIX_ENABLE = 1 */
            break;
        default:
            // ATU subregion
            if (pos >= PF0_ATU_CAP_ADDRESS && pos < PF0_ATU_CAP_ADDRESS + 0x80000) {
                pos -= PF0_ATU_CAP_ADDRESS;
                // Inbound ATUs
                if (pos & 0x100) {
                    uint32_t idx = (pos - 0x100) >> 9;
                    assert(idx < iatus.size());

                    switch (pos & 0xFF) {
                    case PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].ctrl_1;
                        break;
                    case PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].ctrl_2;
                        break;
                    case PF0_ATU_CAP_IATU_LWR_BASE_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].lwr_base_addr;
                        break;
                    case PF0_ATU_CAP_IATU_UPPER_BASE_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].upper_base_addr;
                        break;
                    case PF0_ATU_CAP_IATU_LIMIT_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].limit_addr;
                        break;
                    case PF0_ATU_CAP_IATU_LWR_TARGET_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].lwr_target_addr;
                        break;
                    case PF0_ATU_CAP_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].upper_target_addr;
                        break;
                    case PF0_ATU_CAP_IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_i_OFFSET:
                        *result32 = iatus[idx].uppr_limit_addr;
                        break;
                    default:
                        abort();
                    }
                } else { // Outbound ATUs
                    *result32 = 0;
                }
            // BAR subregion
            } else if (pos >= PF0_TYPE0_HDR_BAR0_REG_ADDRESS &&
                       pos <= PF0_TYPE0_HDR_BAR5_REG_ADDRESS) {
                *result32 = bar_regs[(pos - PF0_TYPE0_HDR_BAR0_REG_ADDRESS) / 4];
            }
            break;
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PcieDbiSlvRegion::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        default:
            // ATU subregion
            if (pos >= PF0_ATU_CAP_ADDRESS && pos < PF0_ATU_CAP_ADDRESS + 0x80000) {
                pos -= PF0_ATU_CAP_ADDRESS;
                // Inbound ATUs
                if (pos & 0x100) {
                    uint32_t idx = (pos - 0x100) >> 9;
                    assert(idx < iatus.size());

                    switch (pos & 0xFF) {
                    case PF0_ATU_CAP_IATU_REGION_CTRL_1_OFF_INBOUND_i_OFFSET:
                        iatus[idx].ctrl_1 = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_REGION_CTRL_2_OFF_INBOUND_i_OFFSET:
                        iatus[idx].ctrl_2 = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_LWR_BASE_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].lwr_base_addr = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_UPPER_BASE_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].upper_base_addr = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_LIMIT_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].limit_addr = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_LWR_TARGET_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].lwr_target_addr = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].upper_target_addr = *source32;
                        break;
                    case PF0_ATU_CAP_IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_i_OFFSET:
                        iatus[idx].uppr_limit_addr = *source32;
                        break;
                    default:
                        abort();
                    }
                } else { // Outbound ATUs
                    // Not implemented
                }
            // BAR subregion
            } else if (pos >= PF0_TYPE0_HDR_BAR0_REG_ADDRESS &&
                       pos <= PF0_TYPE0_HDR_BAR5_REG_ADDRESS) {
                bar_regs[(pos - PF0_TYPE0_HDR_BAR0_REG_ADDRESS) / 4] = *source32;
            }
            break;
        }
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        write(agent, pos, n, source);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(std::ostream&, size_type, size_type) const override { }

    struct iatu_info_t {
        uint32_t ctrl_1;
        uint32_t ctrl_2;
        uint32_t lwr_base_addr;
        uint32_t upper_base_addr;
        uint32_t limit_addr;
        uint32_t lwr_target_addr;
        uint32_t upper_target_addr;
        uint32_t uppr_limit_addr;
    };

    std::array<uint32_t, 6> bar_regs{};
    std::array<iatu_info_t, ETSOC_CX_ATU_NUM_INBOUND_REGIONS> iatus{};
};

} // namespace bemu

#endif // BEMU_PCIE_DBI_SLV_H
