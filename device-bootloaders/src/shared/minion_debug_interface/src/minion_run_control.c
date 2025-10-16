/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*

************************************************************************/
/* This file contains group of functions to setup/coordinate debug features within the ETSOC */

#include "minion_run_control.h"

bool Halt_Harts()
{
    assert_halt();
    bool all_halted = WAIT(check_halted());
    deassert_halt();
    return all_halted;
}

bool Resume_Harts()
{
    uint32_t current_dmctrl = read_dmctrl() | DMCTRL_RES_MASK;
    write_dmctrl(current_dmctrl);
    return WAIT(check_running());
}

void Select_Harts(uint8_t shire_id, uint8_t neigh_id)
{
    select_hart_op(shire_id, neigh_id, ALL_MINIONS_MASK);
}

void Unselect_Harts(uint8_t shire_id, uint8_t neigh_id)
{
    unselect_hart_op(shire_id, neigh_id, ALL_MINIONS_MASK);
}

void Set_PC(uint64_t hart_id, uint64_t pc)
{
    Write_CSR(hart_id, MINION_CSR_DPC_OFFSET, pc);
}

/* BreakPoint Control */
/* Sets breakpoint on a PC. Other breapoints (on data or on PC) in the hart  are overriten */
void Set_PC_Breakpoint(uint64_t hart_id, uint64_t pc, priv_mask_e mode)
{
    Write_CSR(hart_id, MINION_CSR_TDATA1_OFFSET, TDATA1(mode));
    Write_CSR(hart_id, MINION_CSR_TDATA2_OFFSET, pc);
}

void Unset_PC_Breakpoint(uint64_t hart_id)
{
    Set_PC_Breakpoint(hart_id, 0, PRIV_MASK_PRIV_NONE);
}
