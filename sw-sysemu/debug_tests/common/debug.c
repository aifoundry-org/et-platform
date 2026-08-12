#include "debug.h"

#include "common.h"
#include "encoding.h"
#include "esrs.h"

#define MAX_RETRIES 5

#define T0 5

bool Setup_Debug(void)
{
    write_dmctrl(DMACTIVE);
    write_dmctrl(0);
    write_dmctrl(DMACTIVE);
    TRY_UNTIL(dmactive, MAX_RETRIES)
    {
        uint32_t dmctrl = read_dmctrl();
        if (dmctrl & DMACTIVE)
            break;
    }
    return !!dmactive;
}


void Select_Harts(uint64_t shire_id, uint64_t thread_mask)
{
    for (int neigh = 0; neigh < 4; ++neigh) {
        uint64_t mask   = thread_mask & 0xFFFF;
        uint64_t hactrl = read_hactrl(shire_id, neigh);
        hactrl |= (mask | (mask << 16));
        write_hactrl(shire_id, neigh, hactrl);
        thread_mask >>= 16;
    }
}


void Unselect_Harts(uint64_t shire_id, uint64_t thread_mask)
{
    for (int neigh = 0; neigh < 4; ++neigh) {
        uint64_t mask   = thread_mask & 0xFFFF;
        uint64_t hactrl = read_hactrl(shire_id, neigh);
        hactrl &= ~(mask | (mask << 16));
        write_hactrl(shire_id, neigh, hactrl);
        thread_mask >> 16;
    }
}


void Halt_On_Reset(bool value)
{
    uint32_t dmctrl = read_dmctrl();
    write_dmctrl(dmctrl | (value ? SETRESETHALTREQ : CLRRESETHALTREQ));
}


bool Check_Running()
{
    uint32_t tree = read_andortreel2();
    return !!(tree & ALLRUNNING);
}


bool Check_Halted()
{
    uint32_t tree = read_andortreel2();
    return !!(tree & ALLHALTED);
}


bool Have_Reset()
{
    uint32_t tree = read_andortreel2();
    return !!(tree & ALLHAVERESET);
}


void Ack_Have_Reset(void)
{
    uint32_t dmctrl = read_dmctrl();
    write_dmctrl(dmctrl | ACKHAVERESET);
}


bool Halt_Harts(void)
{
    uint32_t dmctrl = read_dmctrl();
    write_dmctrl(dmctrl | HALTREQ);
    TRY_UNTIL(halted, MAX_RETRIES)
    {
        if (Check_Halted())
            break;
    }
    write_dmctrl(dmctrl);
    return !!halted;
}


bool Resume_Harts(void)
{
    uint32_t dmctrl = read_dmctrl();
    write_dmctrl(dmctrl | RESUMEREQ);
    TRY_UNTIL(resumed, MAX_RETRIES)
    {
        uint32_t tree = read_andortreel2();
        if (tree & ALLRESUMEACK)
            break;
    }
    write_dmctrl(dmctrl);
    return !!resumed;
}


bool Reset_Harts(void)
{
    uint32_t dmctrl = read_dmctrl();
    write_dmctrl(dmctrl | HARTRESET);
    write_dmctrl(dmctrl);
    TRY_UNTIL(reset, MAX_RETRIES)
    {
        if (Have_Reset())
            break;
    }
    return !!reset;
}


static inline bool wait_progbuf(uint64_t hart_id)
{
    const uint64_t shire = hart_id / HARTS_PER_SHIRE;
    const uint64_t neigh = hart_id / HARTS_PER_NEIGH;
    const uint64_t hart  = hart_id % HARTS_PER_SHIRE;
    const uint64_t busy  = 1ull << (hart_id % HARTS_PER_NEIGH);
    const uint64_t error = (busy << 32) | (busy << 16);

    uint64_t hastatus1;
    TRY_UNTIL(ready, MAX_RETRIES * 4)
    {
        hastatus1 = read_hastatus1(shire, neigh);
        if (!(hastatus1 & busy))
            break;
    }
    return ready && (!(hastatus1 & error));
}


bool Execute_Insns(uint64_t hart_id, uint32_t* insns, unsigned n_insns)
{
    const uint64_t shire = hart_id / HARTS_PER_SHIRE;
    const uint64_t hart  = hart_id % HARTS_PER_SHIRE;
    const unsigned nsets = (n_insns >> 2) << 2;

    for (unsigned i = 0; i < nsets; ++i) {
        write_nxprogbuf1(shire, hart, insns[i + 3]);
        write_nxprogbuf0(shire, hart, insns[i + 2]);
        write_abscmd(shire, hart, ((uint64_t)insns[i + 1]) << 32 | insns[i]);
        if (!wait_progbuf(hart_id))
            return false;
    }

    switch (n_insns % 4) {
    case 0: return true;
    case 3:
        write_nxprogbuf1(shire, hart, EBREAK);
        write_nxprogbuf0(shire, hart, insns[nsets + 2]);
        write_abscmd(shire, hart, ((uint64_t)insns[nsets + 1] << 32) | insns[nsets]);
        break;
    case 2:
        write_nxprogbuf0(shire, hart, EBREAK);
        write_abscmd(shire, hart, ((uint64_t)insns[nsets + 1] << 32) | insns[nsets]);
        break;
    case 1: write_abscmd(shire, hart, EBREAK << 32 | insns[nsets]); break;
    }
    return wait_progbuf(hart_id);
}


uint64_t Read_GPR(uint64_t hart_id, unsigned reg)
{
    if (reg >= 32)
        return 0;
    uint64_t shire = hart_id / HARTS_PER_SHIRE;
    uint64_t hart  = hart_id % HARTS_PER_SHIRE;
    write_abscmd(shire, hart, EBREAK << 32 | CSRW(CSR_DDATA0, reg));
    return wait_progbuf(hart_id) ? read_nxdata1(shire, hart) << 32 | read_nxdata0(shire, hart) : 0;
}


void Read_All_GPR(uint64_t hart_id, uint64_t* regs)
{
    for (int i = 0; i < 32; ++i) {
        regs[i] = Read_GPR(hart_id, i);
    }
}


void Write_GPR(uint64_t hart_id, unsigned reg, uint64_t value)
{
    if (reg >= 32)
        return;
    uint64_t shire = hart_id / HARTS_PER_SHIRE;
    uint64_t hart  = hart_id % HARTS_PER_SHIRE;
    write_nxdata0(shire, hart, value & 0xFFFFFFFF);
    write_nxdata1(shire, hart, value >> 32);
    write_abscmd(shire, hart, EBREAK << 32 | CSRR(reg, CSR_DDATA0));
    wait_progbuf(hart_id);
}


uint64_t Read_CSR(uint64_t hart_id, unsigned reg)
{
    uint64_t shire   = hart_id / HARTS_PER_SHIRE;
    uint64_t hart    = hart_id % HARTS_PER_SHIRE;
    uint64_t insns[] = {
        CSRW(CSR_DDATA0, T0),
        CSRR(T0, reg),
        CSRRW(T0, CSR_DDATA0, T0),
        EBREAK,
    };
    write_nxprogbuf1(shire, hart, insns[3]);
    write_nxprogbuf0(shire, hart, insns[2]);
    write_abscmd(shire, hart, insns[1] << 32 | insns[0]);
    return wait_progbuf(hart_id) ? read_nxdata1(shire, hart) << 32 | read_nxdata0(shire, hart) : 0;
}


void Write_CSR(uint64_t hart_id, unsigned reg, uint64_t value)
{
    uint64_t shire   = hart_id / HARTS_PER_SHIRE;
    uint64_t hart    = hart_id % HARTS_PER_SHIRE;
    uint64_t insns[] = {
        CSRRW(T0, CSR_DDATA0, T0),
        CSRW(reg, T0),
        CSRR(T0, CSR_DDATA0),
        EBREAK,
    };
    write_nxdata0(shire, hart, value & 0xFFFFFFFF);
    write_nxdata1(shire, hart, value >> 32);
    write_nxprogbuf1(shire, hart, insns[3]);
    write_nxprogbuf0(shire, hart, insns[2]);
    write_abscmd(shire, hart, insns[1] << 32 | insns[0]);
    wait_progbuf(hart_id);
}
