#include "log.h"
#include "print_exception.h"
#include "riscv_encoding.h"

#include <inttypes.h>

static void print_regs(uint64_t mepc, uint64_t mtval, uint64_t mstatus);

void print_exception(uint64_t mcause, uint64_t mepc, uint64_t mtval, uint64_t mstatus,
                     uint64_t hart_id)
{
    log_write(LOG_LEVEL_CRITICAL, "H%04" PRIu64 ": ", hart_id);

    switch (mcause) {
    case EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED:
        log_write(LOG_LEVEL_CRITICAL, "Instruction address misaligned exception ");
        break;

    case EXCEPTION_INSTRUCTION_ACCESS_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Instruction access fault exception ");
        break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
        log_write(LOG_LEVEL_CRITICAL, "Illegal instruction exception ");
        break;

    case EXCEPTION_BREAKPOINT:
        log_write(LOG_LEVEL_CRITICAL, "Breakpoint exception ");
        break;

    case EXCEPTION_LOAD_ADDRESS_MISALIGNED:
        log_write(LOG_LEVEL_CRITICAL, "Load address misaligned exception ");
        break;

    case EXCEPTION_LOAD_ACCESS_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Load access fault exception ");
        break;

    case EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED:
        log_write(LOG_LEVEL_CRITICAL, "Store AMO address misaligned exception ");
        break;

    case EXCEPTION_STORE_AMO_ACCESS_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Store AMO access fault exception ");
        break;

    case EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE:
    case EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE:
    case EXCEPTION_ENVIRONMENT_CALL_FROM_M_MODE:
        log_write(LOG_LEVEL_CRITICAL, "Environment call exception mcause=%016" PRIx64 " ", mcause);
        break;

    case EXCEPTION_INSTRUCTION_PAGE_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Instruction page fault exception ");
        break;

    case EXCEPTION_LOAD_PAGE_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Load page fault exception ");
        break;

    case EXCEPTION_STORE_AMO_PAGE_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Store AMO page fault exception ");
        break;

    case EXCEPTION_FETCH_BUS_ERROR:
        log_write(LOG_LEVEL_CRITICAL, "Fetch bus error exception ");
        break;
    case EXCEPTION_FETCH_ECC_ERROR:
        log_write(LOG_LEVEL_CRITICAL, "Fetch ECC error exception ");
        break;
    case EXCEPTION_LOAD_PAGE_SPLIT_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Load page split exception ");
        break;
    case EXCEPTION_STORE_PAGE_SPLIT_FAULT:
        log_write(LOG_LEVEL_CRITICAL, "Store page split exception ");
        break;
    case EXCEPTION_BUS_ERROR:
        log_write(LOG_LEVEL_CRITICAL, "Bus error exception ");
        break;
    case EXCEPTION_MCODE_INSTRUCTION:
        log_write(LOG_LEVEL_CRITICAL, "Mcode instruction exception ");
        break;
    case EXCEPTION_TXFMA_OFF:
        log_write(LOG_LEVEL_CRITICAL, "TXFMA off exception ");
        break;

    case EXCEPTION_RESERVED_A:
    case EXCEPTION_RESERVED_E:
    case EXCEPTION_UNKOWN_10:
    case EXCEPTION_UNKOWN_11:
    case EXCEPTION_UNKOWN_12:
    case EXCEPTION_UNKOWN_13:
    case EXCEPTION_UNKOWN_14:
    case EXCEPTION_UNKOWN_15:
    case EXCEPTION_UNKOWN_16:
    case EXCEPTION_UNKOWN_17:
    case EXCEPTION_UNKOWN_18:
    default:
        log_write(LOG_LEVEL_CRITICAL, "Unknown exception mcause=%016" PRIx64 " ", mcause);
        break;
    }

    print_regs(mepc, mtval, mstatus);
}

static void print_regs(uint64_t mepc, uint64_t mtval, uint64_t mstatus)
{
    const uint64_t pc = mepc - 4U;

    log_write(LOG_LEVEL_CRITICAL, "PC=0x%010" PRIx64 " mtval=0x%016" PRIx64 " mstatus=0x%016" PRIx64 " priv=", pc, mtval,
           mstatus);

    // Check mstatus.MPP to determine what privilege the trap came from
    switch ((mstatus & 0x1800U) >> 11U) {
    case 0:
        log_write(LOG_LEVEL_CRITICAL, "U\r\n");
        break;

    case 1:
        log_write(LOG_LEVEL_CRITICAL, "S\r\n");
        break;

    case 3:
        log_write(LOG_LEVEL_CRITICAL, "M\r\n");
        break;

    default:
        log_write(LOG_LEVEL_CRITICAL, "X\r\n");
        break;
    }
}
