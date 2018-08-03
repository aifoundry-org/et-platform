#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "log.h"

// Data from EMU
extern xdata xregs[EMU_NUM_THREADS][32];
extern fdata fregs[EMU_NUM_THREADS][32];
extern mdata mregs[EMU_NUM_THREADS][8];
extern uint32_t current_thread;

inst_state_change * log_info = NULL;

// Clears the log
void clearlogstate()
{
    log_info->pc_mod = false;
    log_info->pc = 0;
    log_info->int_reg_mod = false;
    log_info->int_reg_rd = 0;
    log_info->int_reg_data = 0;
    for(int m = 0; m < 8; m++)
    {
        log_info->m_reg_mod[m] = 0;
        for(int i = 0; i < 8; i++)
        {
           log_info->m_reg_data[m][i] = 0;
        }
    }
    log_info->fp_reg_mod = false;
    log_info->fp_reg_rd = 0;
    log_info->fp_reg_data[0] = 0;
    log_info->fp_reg_data[1] = 0;
    for(int i = 0; i < 4; i++)
    {
        log_info->mem_mod[i] = false;
        log_info->mem_size[i] = 0;
        log_info->mem_addr[i] = 0;
        log_info->mem_data[i] = 0;
    }
}

// Sets the log info pointer
void setlogstate(inst_state_change * log_info_)
{
    log_info = log_info_;
}

// Jump
void logpcchange(uint64_t new_pc)
{
    if(log_info == NULL) return;
    // As we support the C extension the PC must be aligned to 2B
    assert((new_pc & 1ULL) == 0ULL);
    log_info->pc_mod = true;
    log_info->pc = new_pc;
}

// Adds an int register change
void logxregchange(int xdst)
{
    if(log_info == NULL) return;
    log_info->int_reg_mod = true;
    log_info->int_reg_rd = xdst;
    log_info->int_reg_data = XREGS[xdst].x;
}

// Adds a float register change
void logfregchange(int fdst)
{
    if(log_info == NULL) return;
    log_info->fp_reg_mod = true;
    log_info->fp_reg_rd = fdst;
    log_info->fp_reg_data[0] = FREGS[fdst].x[0];
    log_info->fp_reg_data[1] = FREGS[fdst].x[1];
}

// Adds a mask register change
void logmregchange(int mdst)
{
    if(log_info == NULL) return;
    log_info->m_reg_mod[mdst] = true;
    for(int i = 0; i < 8; i++)
        log_info->m_reg_data[mdst][i] = MREGS[mdst].b[i];
}

void logmemwchange(int pos, int size, uint64_t addr, uint64_t val)
{
    if(log_info == NULL) return;
    log_info->mem_mod[pos] = true;
    log_info->mem_size[pos] = size;
    log_info->mem_addr[pos] = addr;
    log_info->mem_data[pos] = val;
}

