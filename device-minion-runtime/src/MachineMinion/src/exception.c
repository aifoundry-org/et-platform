#include <stdint.h>

//                                   csr       |     rs1      |    funct3   | opcode
#define INST_CSRRx_MASK      ((0xFFFULL << 20) | (0x1f << 15) | (0x7 << 12) | (0x7f))

//                                 mhartid     |    rs1=0     |           csrrs
#define INST_CSRRS_MHARTID   ((0xF14ULL << 20) | (   0 << 15) | (0x2 << 12) | (0x73))

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t* const reg);
static void write_reg(uint64_t* const reg, uint64_t rd, uint64_t val);

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t* const reg)
{
    if (mcause == 0x2) // illegal instruction
    {
        // Allow reads to mhartid from supervisor/user to complete
        if ((mtval & INST_CSRRx_MASK) == INST_CSRRS_MHARTID)
        {
            const uint64_t rd = (mtval >> 7) & 0x1F;
            uint64_t temp;

            asm volatile ("csrrs %0, mhartid, x0" : "=&r" (temp));
            write_reg(reg, rd, temp);
        }
        else
        {
            (void)mepc;
        }
    }
}

static void write_reg(uint64_t* const reg, uint64_t rd, uint64_t val)
{
    switch(rd)
    {
        case 0: // x0 has no effect and would write to unused stack
        break;

        case 1:
            reg[rd] = val;
        break;

        case 2: // x2 is sp - write to mscratch to give them what they asked for
            asm volatile ("csrw mscratch, %0" : : "r" (val));
        break;

        case 3: // x2/sp isn't saved, so adjust index -1
            reg[rd-1] = val;
        break;

        case 4: // x4/tp isn't saved, so can't index to it.
        break;

        case 5 ... 31: // x2 and x4 aren't saved, so adjust index -2
            reg[rd-2] = val;
        break;

        default:
        break;
    }
}
