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
/*
This file contains group of functions to extract/view state of the Minion state.
These will be consumed by DM Minion Debug Interface commands.
*/

#include "minion_state_inspection.h"

/* dev_registers is used to return the GPRs in Read_All_GPR */
struct dev_context_registers_t dev_registers = { 0 };

uint64_t Read_GPR(uint64_t hart_id, uint32_t reg)
{
    uint32_t read_reg_list[NUM_INST_GPR_READ_SEQ] = { READ_GPR_SEQ(reg) };
    execute_instructions(hart_id, read_reg_list, NUM_INST_GPR_READ_SEQ);
    return read_ddata(hart_id);
}

void Write_GPR(uint64_t hart_id, uint32_t reg, uint64_t data)
{
    uint32_t write_reg_list[NUM_INST_GPR_WRITE_SEQ] = { WRITE_GPR_SEQ(reg) };
    write_ddata(hart_id, data);
    execute_instructions(hart_id, write_reg_list, NUM_INST_GPR_WRITE_SEQ);
}

uint64_t *Read_All_GPR(uint64_t hart_id)
{
    for (uint32_t reg = 0; reg < 31; reg += 1)
    {
        dev_registers.gpr[reg] = Read_GPR(hart_id, reg);
    }
    return dev_registers.gpr;
}

uint64_t Read_CSR(uint64_t hart_id, uint32_t csr)
{
    uint32_t read_reg_list[NUM_INST_CSR_READ_SEQ] = { READ_CSR_SEQ(csr) };
    execute_instructions(hart_id, read_reg_list, NUM_INST_CSR_READ_SEQ);
    return read_ddata(hart_id);
}

void Write_CSR(uint64_t hart_id, uint32_t csr, uint64_t data)
{
    uint32_t write_reg_list[NUM_INST_CSR_WRITE_SEQ] = { WRITE_CSR_SEQ(csr) };
    write_ddata(hart_id, data);
    execute_instructions(hart_id, write_reg_list, NUM_INST_CSR_WRITE_SEQ);
}

void VPU_RF_Init(uint64_t hart_id)
{
    uint32_t vpu_rf_init_list[NUM_INST_VPU_RF_INIT_SEQ] = { VPU_RF_INIT_SEQ() };
    execute_instructions(hart_id, vpu_rf_init_list, NUM_INST_VPU_RF_INIT_SEQ);
}

