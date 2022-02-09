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
/* This file contains group of functions to setup/coordinate debug features within the ETSOC */

#include "minion_run_control.h"

bool Halt_Harts()
{
    assert_halt();
    bool all_halted = WAIT(Check_Halted());
    deassert_halt();
    return all_halted;
}

bool Resume_Harts()
{
    uint32_t current_dmctrl = read_dmctrl() | DMCTRL_RES_MASK;
    write_dmctrl(current_dmctrl);
    return WAIT(Check_Running());
}

bool Check_Halted()
{
    uint32_t treel2 = read_andortreel2();
    bool no_harts_enabled = HALTED(treel2) && RUNNING(treel2);
    return (HALTED(treel2) && (!no_harts_enabled));
}

bool Check_Running()
{
    uint32_t treel2 = read_andortreel2();
    bool no_harts_enabled = HALTED(treel2) && RUNNING(treel2);
    return (RUNNING(treel2) && (!no_harts_enabled));
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
    Write_CSR(hart_id, MINION_CSR_TDATA1_OFFSET, pc);
}

void Unset_PC_Breakpoint(uint64_t hart_id)
{
    Set_PC_Breakpoint(hart_id, 0, PRIV_MASK_PRIV_NONE);
}
