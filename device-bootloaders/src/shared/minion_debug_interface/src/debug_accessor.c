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


uint32_t read_andortreel2(void)
{
    return ioread32(R_SP_MISC_BASEADDR | SPIO_MISC_ESR_ANDORTREEL2_BYTE_ADDRESS);
}

uint32_t read_dmctrl(void)
{
    return ioread32(R_SP_MISC_BASEADDR + SPIO_MISC_ESR_DMCTRL_BYTE_ADDRESS);
}

void write_dmctrl(uint32_t data)
{
    return iowrite32((R_SP_MISC_BASEADDR | SPIO_MISC_ESR_DMCTRL_BYTE_ADDRESS), data);
}

void disable_shire_threads(uint8_t shire_id)
{
    WRITE_THREAD0_DISABLE(shire_id, 0xFFFFFFFF);
    WRITE_THREAD1_DISABLE(shire_id, 0xFFFFFFFF);
}

void enable_shire_threads(uint8_t shire_id)
{
    WRITE_THREAD0_DISABLE(shire_id, 0x0);
    WRITE_THREAD1_DISABLE(shire_id, 0x0);
}

uint64_t get_enabled_harts(uint8_t shire_id)
{
    uint32_t th0_enabled = ~READ_THREAD0_DISABLE(shire_id);
    uint32_t th1_enabled = ~READ_THREAD1_DISABLE(shire_id);
    uint64_t shire_enabled_harts = 0;
    for (uint32_t i = 0; i < HARTS_PER_SHIRE; ++i)
    {
        uint32_t th_enabled = i%2 == 0 ? th0_enabled : th1_enabled;
        shire_enabled_harts |= ((th_enabled >> i/2) & 0x1) << i;
    }
    return shire_enabled_harts;
}

void select_hart_op(uint8_t shire_id, uint8_t neigh_id, uint16_t hart_mask)
{
    uint64_t hactrl = READ_HACTRL(shire_id, neigh_id);
    hactrl = SELECT_HART_OP(hactrl, (uint64_t)hart_mask);
    WRITE_HACTRL(shire_id, neigh_id, hactrl);
}

void unselect_hart_op(uint8_t shire_id, uint8_t neigh_id, uint16_t hart_mask)
{
    uint64_t hactrl = READ_HACTRL(shire_id, neigh_id);
    hactrl = UNSELECT_HART_OP(hactrl, (uint64_t)hart_mask);
    WRITE_HACTRL(shire_id, neigh_id, hactrl);
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

/* Functions used by minion state inspection APIs */
static void write_abscmd(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, ABSCMD_BYTE, data, thread_id);
}

uint64_t read_nxdata0(uint64_t hart_id)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    return read_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA0_BYTE, thread_id);
}

void write_nxdata0(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA0_BYTE, data, thread_id);
}

uint64_t read_nxdata1(uint64_t hart_id)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    return read_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA1_BYTE, thread_id);
}

void write_nxdata1(uint64_t hart_id, uint64_t data)
{
    uint8_t shire_id = (uint8_t)(GET_SHIRE_ID(hart_id));
    uint8_t minion_id = (uint8_t)(GET_MINION_ID(hart_id));
    uint8_t thread_id = (uint8_t)(GET_THREAD_ID(hart_id));
    write_esr_new(PP_MESSAGES, shire_id, REGION_MINION, minion_id, NXDATA1_BYTE, data, thread_id);
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
            busy = HART_BUSY_STATUS(hart_id);
            --rtry;

            bool xcpt = HART_EXCEPTION_STATUS(hart_id);
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

/* This function returns True if selected harts are halted */
bool check_halted()
{
    uint32_t treel2 = read_andortreel2();
    bool no_harts_enabled = HALTED(treel2) && RUNNING(treel2);
    return (HALTED(treel2) && (!no_harts_enabled));
}

/* This function returns True if selected harts are running. It DOESN'T implement the workaround for
   Errata 1.18 so minion 0 on each neigh needs to be selected and enabled when this function is called*/
bool check_running()
{
    uint32_t treel2 = read_andortreel2();
    bool no_harts_enabled = HALTED(treel2) && RUNNING(treel2);
    return (RUNNING(treel2) && (!no_harts_enabled));
}