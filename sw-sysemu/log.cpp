#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"
#include "emu.h"

inst_state_change * log_info = NULL;

void clearlogstate()
{
    if (log_info == NULL) return;
    memset(log_info, 0, sizeof(inst_state_change));
}

void setlogstate(inst_state_change * log_info_)
{
    log_info = log_info_;
}

void log_pc_update(uint64_t new_pc)
{
    assert(~new_pc & 1);
    if (log_info == NULL) return;
    log_info->pc_mod = true;
    log_info->pc = new_pc;
}

void log_xreg_write(int xdst)
{
    assert(xdst < 32);
    if (log_info == NULL) return;
    log_info->int_reg_mod = true;
    log_info->int_reg_rd = xdst;
    log_info->int_reg_data = XREGS[xdst].x;
}

void log_freg_write(int fdst)
{
    assert(fdst < 32);
    if (log_info == NULL) return;
    log_info->fp_reg_mod = true;
    log_info->fp_reg_rd = fdst;
    for(int i = 0; i < (VL/2); i++)
        log_info->fp_reg_data[i] = FREGS[fdst].x[i];
}

void log_mreg_write(int mdst)
{
    assert(mdst < 8);
    if (log_info == NULL) return;
    log_info->m_reg_mod[mdst] = true;
    for(int i = 0; i < VL; i++)
        log_info->m_reg_data[mdst][i] = MREGS[mdst].b[i];
}

void log_mem_write(int pos, int size, uint64_t addr, uint64_t val)
{
    assert(pos < VL);
    if (log_info == NULL) return;
    log_info->mem_mod[pos] = true;
    log_info->mem_size[pos] = size;
    log_info->mem_addr[pos] = addr;
    log_info->mem_data[pos] = val;
}

void log_fflags_write(uint64_t new_flags)
{
    if (log_info == NULL) return;
    log_info->fflags_mod = true;
    log_info->fflags_value = new_flags;
}

void log_tensor_load(int trans)
{
    if (log_info == NULL) return;
    log_info->tensor_mod = true;
    log_info->tl_transform = trans;
}

void log_tensor_fma_new_pass()
{
    if (log_info == NULL) return;
    log_info->tensorfma_passes++;
}

void log_tensor_fma_skip_row(int pass, int row)
{
    if (log_info == NULL) return;
    for (int freg = row*TFMA_REGS_PER_ROW; freg < (row+1) * TFMA_REGS_PER_ROW; ++freg)
    {
        for (int elem = 0; elem < VL; ++elem)
        {
            log_info->tensorfma_skip[pass][freg][elem] = true;
        }
    }
}

void log_tensor_fma_skip_elem(int pass, int freg, int elem)
{
    if (log_info == NULL) return;
    log_info->tensorfma_skip[pass][freg][elem] = true;
}

void log_tensor_fma_write(int pass, int freg, int elem, uint32_t value)
{
    if (log_info == NULL) return;
    log_info->tensorfma_data[pass][freg][elem] = value;
    log_info->tensorfma_mod[pass] |= 1u << freg;
}
