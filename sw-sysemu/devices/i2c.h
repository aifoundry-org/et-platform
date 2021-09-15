/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_I2C_H
#define BEMU_I2C_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"
#include "literals.h"
#include "emu_gio.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N, int ID>
struct I2c : public MemoryRegion {
    using addr_type     = typename MemoryRegion::addr_type;
    using size_type     = typename MemoryRegion::size_type;
    using pointer       = typename MemoryRegion::pointer;
    using const_pointer = typename MemoryRegion::const_pointer;

    enum : unsigned long long {
        I2C_IC_DATA_CMD = 0x10,
        I2C_IC_RXFLR = 0x78
    };

    static_assert(N == 4_KiB, "bemu::I2c has illegal size");

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_AGENT(DEBUG, agent, "I2c%d::read(pos=0x%llx)", ID, pos);

        if (n != 4)
            throw memory_error(first() + pos);

        switch (pos) {
        case I2C_IC_RXFLR:
            *result32 = n; 
            break;
        case I2C_IC_DATA_CMD:
            *result32 = pmic_read(agent, i2c_reg_addr);
            break;
        default:
            *result32 = 0;
        }
    }

    void write(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);
        (void) source32;

        LOG_AGENT(DEBUG, agent, "I2c%d::write(pos=0x%llx)", ID, pos);

        if((pos ==  I2C_IC_DATA_CMD) && ((*source32 & 0x00000100) == 0))
        {
            i2c_reg_addr = *source32 & 0x000000ff;
            pmic_write(i2c_reg_addr);
        }

        if (n != 4)
            throw memory_error(first() + pos);
    }

    void init(const Agent& agent, size_type pos, size_type n, const_pointer source) override {
        const uint8_t *src = reinterpret_cast<const uint8_t *>(source);
        (void) src;
        LOG_AGENT(DEBUG, agent, "I2c%d::init(pos=0x%llx, n=0x%llx)", ID, pos, n);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

protected:
    enum : uint8_t 
    {
         PMIC_AVG_POWER_ADDR = 0x29,
         PMIC_TEMP_ADDR = 0x7,
         PMIC_INT_CAUSE_ADDR = 0x4
    };

    uint8_t i2c_reg_addr;
    // Default Power to 25 W
    uint16_t average_power = 25;
    // Default Temperature to 25 C
    uint8_t current_temp = 25;

    void pmic_write(uint8_t reg_addr) {
       (void)reg_addr;
        // Add support for update PMIC Register
    }

    uint32_t pmic_read(const Agent& agent, uint8_t reg_addr) {
        uint32_t result;
        uint32_t plic_source = SPIO_PLIC_GPIO_INTR_ID;

        switch (reg_addr) {
        case PMIC_AVG_POWER_ADDR:
            average_power += rand() % 5;
            result = average_power << 2;
            // If Power is over HW Threshold, send interrupt to ET SOC
            if(average_power > 40) {
                agent.chip->sp_plic_interrupt_pending_set(plic_source);
            }
            // Reset the Power if above an arbitrary threshold
            average_power = (average_power < 50) ? average_power : 10;
            break;
        case PMIC_TEMP_ADDR:
            current_temp += rand() % 5;
            result = current_temp;
            // If Temperature is over HW Threshold, send interrupt to ET SOC
            if(current_temp > 80) {
                agent.chip->sp_plic_interrupt_pending_set(plic_source);
            }
            current_temp = (current_temp < 85) ? current_temp : 25;
            break;
        case PMIC_INT_CAUSE_ADDR:
            // Provide the respective Int Cause
            result = (current_temp > 80) ? 1: (( average_power > 40) ? 2: 0);
            agent.chip->sp_plic_interrupt_pending_clear(plic_source);
            break;
        default: 
            result = 0;
            break;
        }
        return result;
    }

};

} // namespace bemu

#endif // BEMU_I2C_H
