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
#include "emu_gio.h"
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "devices/pcie_dma.h"

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
        PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_LOW_OFF_ADDRESS = 0x940,
        PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_HIGH_OFF_ADDRESS = 0x944,
        PF0_ATU_CAP_ADDRESS              = 0x300000,
        PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS  = 0x38000c,
        PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS   = 0x380010,
        PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS   = 0x38002c,
        PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS    = 0x380030,
        PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS = 0x38004c,
        PF0_DMA_CAP_DMA_WRITE_INT_MASK_OFF_ADDRESS   = 0x380054,
        PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS  = 0x380058,
        PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS  = 0x3800a0,
        PF0_DMA_CAP_DMA_READ_INT_MASK_OFF_ADDRESS    = 0x3800a8,
        PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS   = 0x3800ac,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS = 0x380200,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS     = 0x38021c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS    = 0x380220,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS = 0x380300,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS     = 0x38031c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS    = 0x380320,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS = 0x380400,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_1_ADDRESS     = 0x38041c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_1_ADDRESS    = 0x380420,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS = 0x380500,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_1_ADDRESS     = 0x38051c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_1_ADDRESS    = 0x380520,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS = 0x380600,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_2_ADDRESS     = 0x38061c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_2_ADDRESS    = 0x380620,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS = 0x380700,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_2_ADDRESS     = 0x38071c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_2_ADDRESS    = 0x380720,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS = 0x380800,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_3_ADDRESS     = 0x38081c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_3_ADDRESS    = 0x380820,
        PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS = 0x380900,
        PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_3_ADDRESS     = 0x38091c,
        PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_3_ADDRESS    = 0x380920,
    };

    enum : uint32_t {
        PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_EN_HSHAKE_CH0_LSB = 16,
    };

    enum : uint32_t {
        PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_EN_HSHAKE_CH0_LSB = 16,
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

    enum : unsigned long long {
        SPIO_PLIC_PSHIRE_PCIE0_EDMA0_INTR_ID = 96,
    };

    PcieDbiSlvRegion(std::array<PcieDma<true>,  ETSOC_CC_NUM_DMA_WR_CHAN> &dma_wrch,
                     std::array<PcieDma<false>, ETSOC_CC_NUM_DMA_RD_CHAN> &dma_rdch) :
        dma_wrch_(dma_wrch), dma_rdch_(dma_rdch) {}

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
            *result32 = msi_cap;
            break;
        case PF0_MSIX_CAP:
            *result32 = msix_cap;
            break;
        case PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_LOW_OFF_ADDRESS:
            *result32 = msix_match_low;
            break;
        case PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_HIGH_OFF_ADDRESS:
            *result32 = msix_match_high;
            break;
        case PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS:
            *result32 = dma_write_engine_en;
            break;
        case PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS:
            *result32 = dma_write_doorbell;
            break;
        case PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS:
            *result32 = dma_read_engine_en;
            break;
        case PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS:
            *result32 = dma_read_doorbell;
            break;
        case PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS:
            *result32 = dma_write_int_status;
            break;
        case PF0_DMA_CAP_DMA_WRITE_INT_MASK_OFF_ADDRESS:
            *result32 = dma_write_int_mask;
            break;
        case PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS:
            *result32 = 0;
            break;
        case PF0_DMA_CAP_DMA_READ_INT_MASK_OFF_ADDRESS:
            *result32 = dma_read_int_mask;
            break;
        case PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS:
            *result32 = dma_read_int_status;
            break;
        case PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS:
            *result32 = 0;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS:
            *result32 = dma_wrch_[0].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS:
            *result32 = dma_wrch_[0].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS:
            *result32 = dma_wrch_[0].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS:
            *result32 = dma_rdch_[0].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS:
            *result32 = dma_rdch_[0].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS:
            *result32 = dma_rdch_[0].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS:
            *result32 = dma_wrch_[1].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_1_ADDRESS:
            *result32 = dma_wrch_[1].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_1_ADDRESS:
            *result32 = dma_wrch_[1].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS:
            *result32 = dma_rdch_[1].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_1_ADDRESS:
            *result32 = dma_rdch_[1].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_1_ADDRESS:
            *result32 = dma_rdch_[1].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS:
            *result32 = dma_wrch_[2].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_2_ADDRESS:
            *result32 = dma_wrch_[2].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_2_ADDRESS:
            *result32 = dma_wrch_[2].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS:
            *result32 = dma_rdch_[2].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_2_ADDRESS:
            *result32 = dma_rdch_[2].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_2_ADDRESS:
            *result32 = dma_rdch_[2].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS:
            *result32 = dma_wrch_[3].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_3_ADDRESS:
            *result32 = dma_wrch_[3].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_3_ADDRESS:
            *result32 = dma_wrch_[3].llp_high;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS:
            *result32 = dma_rdch_[3].ch_control1;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_3_ADDRESS:
            *result32 = dma_rdch_[3].llp_low;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_3_ADDRESS:
            *result32 = dma_rdch_[3].llp_high;
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

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "PcieDbiSlvRegion::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case PF0_MSI_CAP:
            msi_cap = *source32;
            break;
        case PF0_MSIX_CAP:
            msix_cap = *source32;
            break;
        case PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_LOW_OFF_ADDRESS:
            msix_match_low = *source32;
            break;
        case PF0_PORT_LOGIC_MSIX_ADDRESS_MATCH_HIGH_OFF_ADDRESS:
            msix_match_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_ADDRESS:
            dma_write_engine_en = *source32;
            break;
        case PF0_DMA_CAP_DMA_WRITE_DOORBELL_OFF_ADDRESS: {
            dma_write_doorbell = *source32;
            uint32_t wr_doorbell_num = dma_write_doorbell & ((1u << ETSOC_CC_NUM_DMA_WR_CHAN) - 1);
            uint32_t hshake_bit = PF0_DMA_CAP_DMA_WRITE_ENGINE_EN_OFF_DMA_WRITE_ENGINE_EN_HSHAKE_CH0_LSB
                                  + wr_doorbell_num;
            /* In non-Handshake mode, we can start the DMA */
            if (!(dma_write_engine_en & (1u << hshake_bit))) {
                dma_wrch_[wr_doorbell_num].go(agent);
            }
            break;
        }
        case PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_ADDRESS:
            dma_read_engine_en = *source32;
            break;
        case PF0_DMA_CAP_DMA_READ_DOORBELL_OFF_ADDRESS: {
            dma_read_doorbell = *source32;
            uint32_t rd_doorbell_num = dma_read_doorbell & ((1u << ETSOC_CC_NUM_DMA_RD_CHAN) - 1);
            uint32_t hshake_bit = PF0_DMA_CAP_DMA_READ_ENGINE_EN_OFF_DMA_READ_ENGINE_EN_HSHAKE_CH0_LSB
                                  + rd_doorbell_num;
            /* In non-Handshake mode, we can start the DMA */
            if (!(dma_read_engine_en & (1u << hshake_bit))) {
                dma_rdch_[rd_doorbell_num].go(agent);
            }
            break;
        }
        case PF0_DMA_CAP_DMA_WRITE_INT_STATUS_OFF_ADDRESS:
            dma_write_int_status = *source32;
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_WRITE_INT_MASK_OFF_ADDRESS:
            dma_write_int_mask = *source32 & ((1u << ETSOC_CC_NUM_DMA_WR_CHAN) - 1);
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_WRITE_INT_CLEAR_OFF_ADDRESS:
            dma_write_int_status &= ~(*source32 & 0xFFu); // DONE bits
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_READ_INT_STATUS_OFF_ADDRESS:
            dma_read_int_status = *source32;
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_READ_INT_MASK_OFF_ADDRESS:
            dma_read_int_mask = *source32 & ((1u << ETSOC_CC_NUM_DMA_RD_CHAN) - 1);
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_READ_INT_CLEAR_OFF_ADDRESS:
            dma_read_int_status &= ~(*source32 & 0xFFu); // DONE bits
            edma_trigger_check(agent);
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_0_ADDRESS:
            dma_wrch_[0].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_0_ADDRESS:
            dma_wrch_[0].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_0_ADDRESS:
            dma_wrch_[0].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_0_ADDRESS:
            dma_rdch_[0].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_0_ADDRESS:
            dma_rdch_[0].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_0_ADDRESS:
            dma_rdch_[0].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_1_ADDRESS:
            dma_wrch_[1].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_1_ADDRESS:
            dma_wrch_[1].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_1_ADDRESS:
            dma_wrch_[1].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_1_ADDRESS:
            dma_rdch_[1].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_1_ADDRESS:
            dma_rdch_[1].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_1_ADDRESS:
            dma_rdch_[1].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_2_ADDRESS:
            dma_wrch_[2].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_2_ADDRESS:
            dma_wrch_[2].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_2_ADDRESS:
            dma_wrch_[2].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_2_ADDRESS:
            dma_rdch_[2].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_2_ADDRESS:
            dma_rdch_[2].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_2_ADDRESS:
            dma_rdch_[2].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_WRCH_3_ADDRESS:
            dma_wrch_[3].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_WRCH_3_ADDRESS:
            dma_wrch_[3].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_WRCH_3_ADDRESS:
            dma_wrch_[3].llp_high = *source32;
            break;
        case PF0_DMA_CAP_DMA_CH_CONTROL1_OFF_RDCH_3_ADDRESS:
            dma_rdch_[3].ch_control1 = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_LOW_OFF_RDCH_3_ADDRESS:
            dma_rdch_[3].llp_low = *source32;
            break;
        case PF0_DMA_CAP_DMA_LLP_HIGH_OFF_RDCH_3_ADDRESS:
            dma_rdch_[3].llp_high = *source32;
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

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

    void trigger_done_int(const Agent& agent, bool wrch, int chan_id) {
        if (wrch) {
            assert(chan_id < ETSOC_CC_NUM_DMA_WR_CHAN);
            dma_write_int_status |= 1u << chan_id;
        } else {
            assert(chan_id < ETSOC_CC_NUM_DMA_RD_CHAN);
            dma_read_int_status |= 1u << chan_id;
        }
        edma_trigger_check(agent);
    }

    void edma_trigger_check(const Agent& agent) {
        for (int i = 0; i < ETSOC_CC_NUM_DMA_WR_CHAN; i++) {
            uint32_t plic_source = SPIO_PLIC_PSHIRE_PCIE0_EDMA0_INTR_ID + i;
            if ((dma_write_int_status & dma_write_int_mask) & (1u << i)) {
                agent.chip->sp_plic_interrupt_pending_set(plic_source);
            } else {
                agent.chip->sp_plic_interrupt_pending_clear(plic_source);
            }
        }

        for (int i = 0; i < ETSOC_CC_NUM_DMA_RD_CHAN; i++) {
            uint32_t plic_source = SPIO_PLIC_PSHIRE_PCIE0_EDMA0_INTR_ID + ETSOC_CC_NUM_DMA_WR_CHAN + i;
            if ((dma_read_int_status & dma_read_int_mask) & (1u << i)) {
                agent.chip->sp_plic_interrupt_pending_set(plic_source);
            } else {
                agent.chip->sp_plic_interrupt_pending_clear(plic_source);
            }
        }
    }

    std::array<uint32_t, 6> bar_regs{};
    std::array<MainMemory::pcie_iatu_info_t, ETSOC_CX_ATU_NUM_INBOUND_REGIONS> iatus{};
    uint32_t msi_cap = (1u << 16) | (0u << 20); /* PCI_MSI_ENABLE = 1, PCI_MSI_MULTIPLE_MSG_EN = 0 */
    uint32_t msix_cap = (1u << 31); /* PCI_MSIX_ENABLE = 1 */
    uint32_t msix_match_low = 0;
    uint32_t msix_match_high = 0;
    uint32_t dma_write_engine_en = 0;
    uint32_t dma_write_doorbell = 0;
    uint32_t dma_read_engine_en = 0;
    uint32_t dma_read_doorbell = 0;
    uint32_t dma_write_int_status = 0;
    uint32_t dma_write_int_mask = ETSOC_CC_NUM_DMA_WR_CHAN == 0 ? 0 : (1u << ETSOC_CC_NUM_DMA_WR_CHAN) - 1;
    uint32_t dma_read_int_status = 0;
    uint32_t dma_read_int_mask = ETSOC_CC_NUM_DMA_RD_CHAN == 0 ? 0 : (1u << ETSOC_CC_NUM_DMA_RD_CHAN) - 1;
    std::array<PcieDma<true>,  ETSOC_CC_NUM_DMA_WR_CHAN> &dma_wrch_;
    std::array<PcieDma<false>, ETSOC_CC_NUM_DMA_RD_CHAN> &dma_rdch_;
};

} // namespace bemu

#endif // BEMU_PCIE_DBI_SLV_H
