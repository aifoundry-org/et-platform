#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"
#include "emu.h"

inst_state_change * log_info = NULL;

void clearlogstate()
{
    if (!log_info) return;
    memset(log_info, 0, sizeof(inst_state_change));
}

void setlogstate(inst_state_change * log_info_)
{
    log_info = log_info_;
}

void log_pc_update(uint64_t new_pc)
{
    assert(~new_pc & 1);
    if (!log_info) return;
    log_info->pc_mod = true;
    log_info->pc = new_pc;
}

void log_xreg_write(int xdst, uint64_t xval)
{
    assert(xdst < MAXXREG);
    if (!log_info) return;
    log_info->int_reg_mod = true;
    log_info->int_reg_rd = xdst;
    log_info->int_reg_data = xval;
}

void log_freg_write(int fdst, const freg_t& fval)
{
    assert(fdst < MAXFREG);
    if (!log_info) return;
    log_info->fp_reg_mod = true;
    log_info->fp_reg_rd = fdst;
    for(size_t i = 0; i < (VL/2); i++)
        log_info->fp_reg_data[i] = fval.u64[i];
}

void log_mreg_write(int mdst, const mreg_t& mval)
{
    assert(mdst < MAXMREG);
    if (!log_info) return;
    log_info->m_reg_mod[mdst] = true;
    for(size_t i = 0; i < VL; i++)
        log_info->m_reg_data[mdst][i] = mval[i];
}

void log_mem_write(int pos, int size, uint64_t addr, uint64_t val)
{
    assert(unsigned(pos) < VL);
    if (!log_info) return;
    log_info->mem_mod[pos] = true;
    log_info->mem_size[pos] = size;
    log_info->mem_addr[pos] = addr;
    log_info->mem_data[pos] = val;
}

void log_fflags_write(uint64_t new_flags)
{
    if (!log_info) return;
    log_info->fflags_mod = true;
    log_info->fflags_value = new_flags;
}

void log_tensor_load(uint8_t trans, uint8_t first, uint8_t count, uint64_t conv_mask)
{
    assert(trans < 8);
    assert(conv_mask <= 0xFFFF);
    if (!log_info) return;
    log_info->tensor_mod = true;
    log_info->tl_scp_first = first;
    log_info->tl_scp_count = count;
    switch (trans)
    {
        case 1: case 2:
            log_info->tl_transform = 2;
            break;
        case 5: case 6: case 7:
            log_info->tl_transform = 1;
            break;
        default:
            log_info->tl_transform = 0;
            break;
    }
    log_info->tl_conv_skip = ~conv_mask & 0xFFFF;
}

void log_tensor_load_scp_write(uint8_t line, const uint64_t* data)
{
    assert(line < 16);
    if (!log_info) return;
    for (int j = 0; j < (L1D_LINE_SIZE/4); j += 2)
    {
        log_info->tensordata[0][2*line + (j+0)/VL][(j+0)%VL] = data[j/2] & 0xffffffff;
        log_info->tensordata[0][2*line + (j+1)/VL][(j+1)%VL] = (data[j/2] >> 32) & 0xffffffff;
    }
}

void log_tensor_fma_new_pass()
{
    if (!log_info) return;
    uint8_t pass = log_info->tensorfma_passes++;
    assert(pass < TFMA_MAX_ACOLS);
}

void log_tensor_fma_write(int pass, int freg, int elem, uint32_t value)
{
    assert(pass < TFMA_MAX_ACOLS);
    assert(freg < MAXFREG);
    assert(unsigned(elem) < VL);
    if (!log_info) return;
    log_info->tensorfma_mask[pass][freg][elem] = true;
    log_info->tensorfma_mod[pass] |= 1u << freg;
    log_info->tensordata[pass][freg][elem] = value;
}

void log_tensor_quant_new_transform(bool skip_write)
{
    if (!log_info) return;
    uint8_t trans = log_info->tensorquant_trans++;
    assert(trans < TQUANT_MAX_TRANS);
    log_info->tensorquant_skip_write[trans] = skip_write;
}

void log_tensor_quant_write(int trans, int freg, int elem, uint32_t value)
{
    assert(trans < TQUANT_MAX_TRANS);
    assert(freg < MAXFREG);
    assert(unsigned(elem) < VL);
    if (!log_info) return;
    log_info->tensorquant_mask[trans][freg][elem] = true;
    log_info->tensorquant_mod[trans] |= 1u << freg;
    log_info->tensordata[trans][freg][elem] = value;
}

void log_tensor_reduce(uint8_t first, uint8_t count)
{
    assert(first < MAXFREG);
    assert(count <= MAXFREG);
    if (!log_info) return;
    log_info->tensorreduce_first = first;
    log_info->tensorreduce_count = count;
}

void log_tensor_reduce_write(uint8_t freg, uint8_t elem, uint32_t value)
{
    assert(freg < MAXFREG);
    assert(elem < VL);
    if (!log_info) return;
    log_info->tensordata[0][freg][elem] = value;
}

void log_trap()
{
    if (!log_info) return;
    log_info->trap_mod = true;
}

void log_gsc_progress(uint64_t gsc_progress, bool success)
{
    if (!log_info) return;
    LOG(DEBUG, "log_gsc_progress(%lu,%d)", gsc_progress, success);
    log_info->gsc_progress_mod = true;
    log_info->gsc_progress = gsc_progress;
}
