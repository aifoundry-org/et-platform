#include "exception.h"
#include "exception_codes.h"
#include "print_exception.h"
#include "message.h"
#include "printf.h"
#include "shire.h"
#include "sync.h"

#include <stdbool.h>

//                                   csr       |     rs1      |    funct3   | opcode
#define INST_CSRRx_MASK      ((0xFFFULL << 20) | (0x1f << 15) | (0x7 << 12) | (0x7f))

//                                 mhartid     |    rs1=0     |           csrrs
#define INST_CSRRS_MHARTID   ((0xF14ULL << 20) | (   0 << 15) | (0x2 << 12) | (0x73))

static void write_reg(uint64_t* const reg, uint64_t rd, uint64_t val);
static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus, uint64_t hart_id);

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t* const reg)
{
    bool returnFromException = false;

    // Instruction emulation goes here
    if ((mcause == EXCEPTION_ILLEGAL_INSTRUCTION) && ((mtval & INST_CSRRx_MASK) == INST_CSRRS_MHARTID))
    {
        const uint64_t rd = (mtval >> 7) & 0x1F;
        uint64_t temp;

        asm volatile ("csrrs %0, mhartid, x0" : "=&r" (temp));
        write_reg(reg, rd, temp);
        returnFromException = true;
    }
    else
    {
        const uint64_t hart_id = get_hart_id();
        uint64_t mstatus;

        asm volatile ("csrr %0, mstatus" : "=r" (mstatus));

        if (hart_id == 2048)
        {
            print_exception(mcause, mepc, mtval, mstatus, hart_id);
        }
        else
        {
            send_exception_message(mcause, mepc, mtval, mstatus, hart_id);
        }
    }

    if (!returnFromException)
    {
        asm volatile ("wfi");
    }
}

static void write_reg(uint64_t* const reg, uint64_t rd, uint64_t val)
{
    switch (rd)
    {
        case 0: // x0 has no effect
        break;

        case 1:
            reg[0] = val;
        break;

        case 2: // x2 is sp - write to mscratch to give them what they asked for
            asm volatile ("csrw mscratch, %0" : : "r" (val));
        break;

        case 3: // x2/sp isn't saved, so adjust index -1
            reg[1] = val;
        break;

        case 4: // x4/tp isn't saved, so can't index to it.
        break;

        case 5 ... 31: // x2 and x4 aren't saved, so adjust index -2
            reg[rd-3] = val;
        break;

        default:
        break;
    }
}

static void send_exception_message(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus, uint64_t hart_id)
{
    static message_t message;

    message.id = MESSAGE_ID_EXCEPTION;
    message.data[0] = hart_id;
    message.data[1] = mcause;
    message.data[2] = mepc;
    message.data[3] = mtval;
    message.data[4] = mstatus;

    message_send_worker(get_shire_id(), hart_id, &message);
}
