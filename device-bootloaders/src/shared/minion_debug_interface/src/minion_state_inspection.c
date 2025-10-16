/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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

uint64_t Minion_Local_Atomic_Read(uint64_t hart_id, uint64_t addr)
{
    /* Write addr to register(a0) */
    Write_GPR(hart_id, GPR_REG_INDEX_A0, addr);

    uint32_t read_minion_local_atomic[NUM_INST_MINION_LCL_ATOMIC_READ_SEQ] = {
        MINION_LCL_ATOMIC_READ_SEQ()
    };

    /* Execute minion memory read (local) atomically */
    /* LCL_ATOMIC_READ macro is defined to save the result in GPR(a0) */
    execute_instructions(hart_id, read_minion_local_atomic, NUM_INST_MINION_LCL_ATOMIC_READ_SEQ);

    /* Return the read value(from the memory) available in the register */
    return Read_GPR(hart_id, GPR_REG_INDEX_A0);
}

uint64_t Minion_Global_Atomic_Read(uint64_t hart_id, uint64_t addr)
{
    /* Write addr to register(a0) */
    Write_GPR(hart_id, GPR_REG_INDEX_A0, addr);

    uint32_t read_minion_global_atomic[NUM_INST_MINION_GLB_ATOMIC_READ_SEQ] = {
        MINION_GLB_ATOMIC_READ_SEQ()
    };

    /* Execute minion memory read (global) atomically */
    /* GLB_ATOMIC_READ macro is defined to save the result in GPR(a0) */
    execute_instructions(hart_id, read_minion_global_atomic, NUM_INST_MINION_GLB_ATOMIC_READ_SEQ);

    /* Return the read value(from the memory) available in the register */
    return Read_GPR(hart_id, GPR_REG_INDEX_A0);
}

uint64_t Minion_Memory_Read(uint64_t hart_id, uint64_t addr)
{
    /* Write addr to register(a0) */
    Write_GPR(hart_id, GPR_REG_INDEX_A0, addr);

    uint32_t read_minion_mem[NUM_INST_MINION_MEM_READ_SEQ] = { MINION_MEM_READ_SEQ() };

    /* Execute minion memory read instruction sequence */
    execute_instructions(hart_id, read_minion_mem, NUM_INST_MINION_MEM_READ_SEQ);

    /* Return the read value(from the memory) available in the register */
    return Read_GPR(hart_id, GPR_REG_INDEX_A0);
}