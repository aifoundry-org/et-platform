/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_DW_APB_TIMERS_H
#define BEMU_DW_APB_TIMERS_H

#include <array>
#include <cstdint>
#include "memory/memory_error.h"
#include "memory/memory_region.h"

namespace bemu {

template<unsigned long long Base, unsigned long long N>
struct DW_apb_timers : public MemoryRegion {
    typedef typename MemoryRegion::addr_type      addr_type;
    typedef typename MemoryRegion::size_type      size_type;
    typedef typename MemoryRegion::pointer        pointer;
    typedef typename MemoryRegion::const_pointer  const_pointer;

    static_assert(N == 4_KiB, "bemu::DW_apb_timers has illegal size");

    /*
     * NOTE: Proper TIM_NEWMODE TimerNLoadCount2 with HIGH/LOW PWM capabilities (TimerNControlReg[4])
     *       is not implemented!
     */

    enum : size_type {
        NUM_TIMERS = 8,
        TIMER_WIDTH = 32,
    };

    // Stride in bytes between registers of different timers
    enum : size_type {
        REG_STRIDE = 0x14u,
    };

    // Registers
    enum : size_type {
        // Per-timer registers
        LOADCOUNT_OFFSET = 0x0u,
        CURRENTVAL_OFFSET = 0x4u,
        CONTROLREG_OFFSET = 0x8u,
        EOI_OFFSET = 0xCu,
        INTSTAT_OFFSET = 0x10u,
        // "Global" registers
        TIMERSINTSTATUS_ADDRESS = 0xa0u,
        TIMERSEOI_ADDRESS = 0xa4u,
        TIMERSRAWINTSTATUS_ADDRESS = 0xa8u,
        TIMERS_COMP_VERSION_ADDRESS = 0xacu,
        TIMER1LOADCOUNT2_ADDRESS = 0xb0u,
        TIMER2LOADCOUNT2_ADDRESS = 0xb4u,
        TIMER3LOADCOUNT2_ADDRESS = 0xb8u,
        TIMER4LOADCOUNT2_ADDRESS = 0xbcu,
        TIMER5LOADCOUNT2_ADDRESS = 0xc0u,
        TIMER6LOADCOUNT2_ADDRESS = 0xc4u,
        TIMER7LOADCOUNT2_ADDRESS = 0xc8u,
        TIMER8LOADCOUNT2_ADDRESS = 0xccu
    };

    // TimerNControlReg bit fields
    #define CONTROLREG_TIMER_ENABLE_GET(x)         ((x) & 0x00000001ul)
    #define CONTROLREG_TIMER_ENABLE_SET(x)         ((x) & 0x00000001ul)
    #define CONTROLREG_TIMER_MODE_GET(x)           (((x) & 0x00000002ul) >> 1)
    #define CONTROLREG_TIMER_MODE_SET(x)           (((x) << 1) & 0x00000002ul)
    #define CONTROLREG_TIMER_INTERRUPT_MASK_GET(x) (((x) & 0x00000004ul) >> 2)
    #define CONTROLREG_TIMER_INTERRUPT_MASK_SET(x) (((x) << 2) & 0x00000004ul)

    // TimerNIntStatus bit fields
    #define INTSTAT_TIMERNINTSTATUS_GET(x)         ((x) & 0x00000001ul)
    #define INTSTAT_TIMERNINTSTATUS_SET(x)         ((x) & 0x00000001ul)

    void read(const Agent& agent, size_type pos, size_type n, pointer result) override {
        uint32_t *result32 = reinterpret_cast<uint32_t *>(result);

        LOG_NOTHREAD(DEBUG, "DW_apb_timers::read(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos < (NUM_TIMERS * REG_STRIDE)) {
            size_type timer = pos / REG_STRIDE;
            size_type timer_pos = pos % REG_STRIDE;

            switch (timer_pos) {
            case LOADCOUNT_OFFSET:
                *result32 = loadcount[timer];
                break;
            case CURRENTVAL_OFFSET:
                // A “0” is always read back when the timer is not enabled; otherwise, the current
                // value of the timer (TimerNCurrentValue register) is read back
                if (CONTROLREG_TIMER_ENABLE_GET(controlreg[timer])) {
                    *result32 = currentvalue[timer];
                } else {
                    *result32 = 0;
                }
                break;
            case CONTROLREG_OFFSET:
                *result32 = controlreg[timer];
                break;
            case EOI_OFFSET:
                // Reading from this register returns all zeroes and clears the interrupt from Timer N
                end_of_interrupt(*agent.chip, timer);
                *result32 = 0;
                break;
            case INTSTAT_OFFSET:
                *result32 = intstatus[timer];
                break;
            }
        } else {
            switch (pos) {
            case TIMERSINTSTATUS_ADDRESS:
                *result32 = 0;
                for (size_type i = 0; i < NUM_TIMERS; i++) {
                    if (INTSTAT_TIMERNINTSTATUS_GET(intstatus[i])) {
                        *result32 |= 1u << i;
                    }
                }
                break;
            case TIMERSRAWINTSTATUS_ADDRESS:
                *result32 = rawintstatus;
                break;
            case TIMERSEOI_ADDRESS:
                // Reading this register returns all zeroes and clears all active interrupts
                for (size_type i = 0; i < NUM_TIMERS; i++) {
                    end_of_interrupt(*agent.chip, i);
                }
                *result32 = 0;
                break;
            }
        }
    }

    void write(const Agent&, size_type pos, size_type n, const_pointer source) override {
        const uint32_t *source32 = reinterpret_cast<const uint32_t *>(source);

        LOG_NOTHREAD(DEBUG, "DW_apb_timers::write(pos=0x%llx)", pos);

        if (n != 4)
            throw memory_error(first() + pos);

        if (pos >= (NUM_TIMERS * REG_STRIDE))
            return;

        size_type timer = pos / REG_STRIDE;
        size_type timer_pos = pos % REG_STRIDE;

        switch (timer_pos) {
        case LOADCOUNT_OFFSET:
            loadcount[timer] = *source32;
            break;
        case CONTROLREG_OFFSET:
            if (CONTROLREG_TIMER_ENABLE_GET(*source32) &&
               !CONTROLREG_TIMER_ENABLE_GET(controlreg[timer])) {
                // When a timer counter is enabled after being reset or disabled,
                // the count value is loaded from the TimerNLoadCount register
                currentvalue[timer] = loadcount[timer];
            } else if (CONTROLREG_TIMER_ENABLE_GET(controlreg[timer]) &&
                      !CONTROLREG_TIMER_ENABLE_GET(*source32)) {
                // When the timer enable bit is de-asserted and the timer stops running,
                // the timer counter and any associated registers in the timer clock domain,
                // such as the toggle register, are asynchronously reset
                currentvalue[timer] = 0;
            }
            controlreg[timer] = *source32;
            break;
        }
    }

    void clock_tick(System& chip) {
        for (size_type i = 0; i < NUM_TIMERS; i++) {
            if (!CONTROLREG_TIMER_ENABLE_GET(controlreg[i])) {
                continue;
            }
            assert(currentvalue[i] > 0);
            // Timer counted down to 0
            if (--currentvalue[i] == 0) {
                if (CONTROLREG_TIMER_MODE_GET(controlreg[i]) == 1) {
                    // User-defined count mode: loads the current value of the TimerNLoadCount
                    currentvalue[i] = loadcount[i];
                } else {
                    // Free-running mode: loads the maximum value (dependent on the timer width)
                    currentvalue[i] = (uint32_t)((1ull << TIMER_WIDTH) - 1);
                }

                // Update raw interrupt status (non-masked)
                rawintstatus |= 1u << i;
                // Generate interrupt if not masked
                if (!CONTROLREG_TIMER_INTERRUPT_MASK_GET(controlreg[i])) {
                    // If raising edge, send interrupt to PLIC
                    if (!intstatus[i]) {
                        chip.pu_plic_interrupt_pending_set(PU_PLIC_TIMER0_INTR_ID + i);
                    }
                    intstatus[i] = INTSTAT_TIMERNINTSTATUS_SET(1);
                }
            }
        }
    }

    void init(const Agent&, size_type pos, size_type n, const_pointer) override {
        LOG_NOTHREAD(DEBUG, "DW_apb_timers::init(pos=0x%llx, n=0x%llx)", pos, n);
    }

    addr_type first() const override { return Base; }
    addr_type last() const override { return Base + N - 1; }

    void dump_data(const Agent&, std::ostream&, size_type, size_type) const override { }

private:
    void end_of_interrupt(System& chip, uint32_t timer) {
        // Clears the interrupt from Timer N
        if (intstatus[timer]) {
            // Falling edge, clear interrupt to PLIC
            chip.pu_plic_interrupt_pending_clear(PU_PLIC_TIMER0_INTR_ID + timer);
        }
        intstatus[timer] = 0;
        rawintstatus &= ~(1u << timer);
    }

    std::array<uint32_t, NUM_TIMERS> loadcount;
    std::array<uint32_t, NUM_TIMERS> loadcount2;
    std::array<uint32_t, NUM_TIMERS> currentvalue;
    std::array<uint32_t, NUM_TIMERS> controlreg;
    std::array<uint32_t, NUM_TIMERS> intstatus;
    uint32_t rawintstatus;
};

} // namespace bemu

#endif // BEMU_DW_APB_TIMERS_H
