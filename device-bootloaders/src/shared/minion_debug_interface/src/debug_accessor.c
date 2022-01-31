/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*

************************************************************************/

#include "debug_accessor.h"
#include "minion_run_control.h"
#include "minion_state_inspection.h"

static uint16_t SELECTED_MINSHIRES[NUM_SHIRES][4] = { 0 };
static uint64_t WORKARROUND_RESUME_SELECTED_HARTS[NUM_SHIRES] = { 0 };
static uint64_t WORKARROUND_RESUME_ENABLED_HARTS[NUM_SHIRES] = { 0 };

uint64_t read_andortreel2(void)
{
    return ioread32(R_SP_MISC_BASEADDR | SPIO_MISC_ESR_ANDORTREEL2_ADDRESS);
}

uint32_t read_dmctrl(void)
{
    return ioread32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_DMCTRL_ADDRESS);
}

void write_dmctrl(uint32_t data)
{
    return iowrite32((R_SP_MISC_BASEADDR | SPIO_MISC_ESR_DMCTRL_ADDRESS), data);
}

static void enable_shire_harts(uint64_t shire_id, uint64_t harts)
{
    WRITE_THREAD0_DISABLE(shire_id, harts & THREAD_0_MASK);
    WRITE_THREAD1_DISABLE(shire_id, harts & THREAD_1_MASK);
}

static void disable_shire_harts(uint64_t shire_id, uint64_t harts)
{
    WRITE_THREAD0_DISABLE(shire_id, harts & ~THREAD_0_MASK);
    WRITE_THREAD1_DISABLE(shire_id, harts & ~THREAD_1_MASK);
}

static uint64_t get_enabled_harts(uint64_t shire_id)
{
    uint32_t th0 = (uint32_t)(READ_THREAD0_DISABLE(shire_id));
    uint32_t th1 = (uint32_t)(READ_THREAD1_DISABLE(shire_id));
    uint64_t th0_enabled = ~th0;
    uint64_t th1_enabled = ~th1;
    return th0_enabled | th1_enabled << 32;
}

void select_hart_op(uint64_t shire_id, uint64_t neigh_id, uint16_t hart_mask)
{
    uint64_t hactrl = READ_HACTRL(shire_id, neigh_id);
    hactrl = SELECT_HART_OP(hactrl, (uint64_t)hart_mask);
    WRITE_HACTRL(shire_id, neigh_id, hactrl);
}

void unselect_hart_op(uint64_t shire_id, uint64_t neigh_id, uint16_t hart_mask)
{
    uint64_t hactrl = READ_HACTRL(shire_id, neigh_id);
    hactrl = UNSELECT_HART_OP(hactrl, (uint64_t)hart_mask);
    WRITE_HACTRL(shire_id, neigh_id, hactrl);
}

bool wait_till_core_halt(void)
{
    uint32_t rtry = MAX_RETRIES;
    while (!Check_Halted() && rtry != 0)
    {
        rtry--;
    }
    return (rtry != 0);
}

void assert_halt(void)
{
    uint32_t current_dmctrl = read_dmctrl() | DMCTRL_HALT_MASK;
    write_dmctrl(current_dmctrl);
}

void deassert_halt(void)
{
    uint32_t current_dmctrl = read_dmctrl() | DMCTRL_HALT_MASK;
    write_dmctrl(current_dmctrl & ~HALTREQ);
}

bool workarround_resume_pre(void)
{
    for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id += 1)
    {
        uint64_t enabled_harts = get_enabled_harts(shire_id);
        uint64_t harts_to_enable = 0;
        uint64_t harts_to_select = 0;
        for (uint64_t neigh_id = 0; neigh_id < NUM_NEIGH_PER_SHIRE; neigh_id += 1)
        {
            uint64_t neigh_mask = SELECTED_MINSHIRES[shire_id][neigh_id];
            if (((neigh_mask & 3) != 3) && (neigh_mask != 0))
            {
                uint64_t m0_requires_enable = ~(enabled_harts >> HARTS_PER_NEIGH) & 3;
                uint64_t m0_requires_select = ~(neigh_mask & 3) & 3;
                harts_to_select = harts_to_select | m0_requires_select
                                                        << neigh_id * HARTS_PER_NEIGH;
                harts_to_enable = harts_to_enable | m0_requires_enable
                                                        << neigh_id * HARTS_PER_NEIGH;
            }
        }
        WORKARROUND_RESUME_ENABLED_HARTS[shire_id] = harts_to_enable;
        WORKARROUND_RESUME_SELECTED_HARTS[shire_id] = harts_to_select;
        if (harts_to_select != 0)
        {
            Select_Harts(shire_id, harts_to_select);
            assert_halt();
            if (harts_to_enable != 0)
            {
                enable_shire_harts(shire_id, harts_to_enable);
            }
            bool all_halted = wait_till_core_halt();
            if (!all_halted)
            {
                return false;
            }
            deassert_halt();
            const uint64_t NUM_M0_PER_SHIRE = 8;
            const uint64_t m0_local_hart_id[8] = {
                HARTS_PER_SHIRE * shire_id + (0 * HARTS_PER_NEIGH + 0),
                HARTS_PER_SHIRE * shire_id + (0 * HARTS_PER_NEIGH + 1),
                HARTS_PER_SHIRE * shire_id + (1 * HARTS_PER_NEIGH + 0),
                HARTS_PER_SHIRE * shire_id + (1 * HARTS_PER_NEIGH + 1),
                HARTS_PER_SHIRE * shire_id + (2 * HARTS_PER_NEIGH + 0),
                HARTS_PER_SHIRE * shire_id + (2 * HARTS_PER_NEIGH + 1),
                HARTS_PER_SHIRE * shire_id + (3 * HARTS_PER_NEIGH + 0),
                HARTS_PER_SHIRE * shire_id + (3 * HARTS_PER_NEIGH + 1)
            };

            for (uint64_t m0_idx = 0; m0_idx < NUM_M0_PER_SHIRE; m0_idx += 1)
            {
                uint64_t hart_id = m0_local_hart_id[m0_idx];
                Set_PC_Breakpoint(hart_id, Read_CSR(hart_id, MINION_CSR_DPC_OFFSET),
                                  PRIV_MASK_PRIV_ALL);
            }
        }
    }
    return true;
}

bool workarround_resume_post(void)
{
    for (uint64_t shire_id = 0; shire_id < NUM_SHIRES; shire_id += 1)
    {
        uint64_t harts_to_disable = WORKARROUND_RESUME_ENABLED_HARTS[shire_id];
        uint64_t harts_to_unselect = WORKARROUND_RESUME_SELECTED_HARTS[shire_id];
        Unselect_Harts(shire_id, harts_to_unselect);
        disable_shire_harts(shire_id, harts_to_disable);
        WORKARROUND_RESUME_ENABLED_HARTS[shire_id] = 0;
        WORKARROUND_RESUME_SELECTED_HARTS[shire_id] = 0;
    }
    return true;
}

/* Functions used by minion state inspection APIs */
static void write_abscmd(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, ABSCMD, data, thread_id);
}

uint64_t read_nxdata0(uint64_t hart_id)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    return read_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA0, thread_id);
}

void write_nxdata0(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA0, data, thread_id);
}

uint64_t read_nxdata1(uint64_t hart_id)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    return read_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA1, thread_id);
}

void write_nxdata1(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA0, data, thread_id);
}

void execute_instructions(uint64_t hart_id, const uint32_t *inst_list, uint32_t num_inst)
{
    for (uint64_t inst_pair_idx = 0; inst_pair_idx < num_inst; inst_pair_idx += 2)
    {
        uint64_t inst0 = inst_list[inst_pair_idx];
        uint64_t inst1 = inst_list[inst_pair_idx + 1];
        uint64_t inst_pair = inst0 | inst1 << 32;
        write_abscmd(hart_id, inst_pair);
        uint32_t rtry = MAX_RETRIES;
        bool busy = true;
        uint64_t hastatus1 = ~BITS64_ALLF_MASK;
        while (busy && rtry)
        {
            busy = HART_BUSY_STATUS(hart_id)
            --rtry;

            bool xcpt = HART_EXCEPTION_STATUS(hart_id)
            if (xcpt)
            {
                uint64_t xcpt_mask =
                    ~(((uint64_t)1 << HARTS_PER_NEIGH) + hart_id % HARTS_PER_NEIGH) &
                    BITS64_ALLF_MASK;
                WRITE_HASTATUS1(GET_SHIRE_ID(hart_id), GET_NEIGH_ID(hart_id), hastatus1 & xcpt_mask);

                /* Add logging capability. ERROR: There was an exception while executing instructions */
            }
        }

        if (busy)
        {
            /* Add logging capability. Hart did not finish executing instructions */
            return;
        }
    }
}

uint64_t read_ddata(uint64_t hart_id)
{
    return (read_nxdata0(hart_id) | (read_nxdata1(hart_id) << 32));
}

void write_ddata(uint64_t hart_id, uint64_t data)
{
    write_nxdata0(hart_id, (data & 0xffffffff));
    write_nxdata1(hart_id, ((data >> 32) & 0xffffffff));
}
