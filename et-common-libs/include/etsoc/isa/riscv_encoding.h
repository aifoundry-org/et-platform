#ifndef RISCV_ENCODING_H
#define RISCV_ENCODING_H

#ifndef PRV_U
#define PRV_U 0
#endif
#ifndef PRV_S
#define PRV_S 1
#endif
#ifndef PRV_M
#define PRV_M 3
#endif

#define MSTATUS_SIE        0x00000002ul
#define MSTATUS_SPIE_SHIFT 5
#define MSTATUS_SPIE       (1ul << MSTATUS_SPIE_SHIFT)
#define MSTATUS_SPP_SHIFT  8
#define MSTATUS_SPP        (1ul << MSTATUS_SPP_SHIFT)
#define MSTATUS_MPP_SHIFT  11
#define MSTATUS_MPP        (3ul << MSTATUS_MPP_SHIFT)

#define EXCEPTION_INSTRUCTION_ADDRESS_MISALIGNED 0
#define EXCEPTION_INSTRUCTION_ACCESS_FAULT       1
#define EXCEPTION_ILLEGAL_INSTRUCTION            2
#define EXCEPTION_BREAKPOINT                     3
#define EXCEPTION_LOAD_ADDRESS_MISALIGNED        4
#define EXCEPTION_LOAD_ACCESS_FAULT              5
#define EXCEPTION_STORE_AMO_ADDRESS_MISALIGNED   6
#define EXCEPTION_STORE_AMO_ACCESS_FAULT         7
#define EXCEPTION_ENVIRONMENT_CALL_FROM_U_MODE   8
#define EXCEPTION_ENVIRONMENT_CALL_FROM_S_MODE   9
#define EXCEPTION_RESERVED_A                     10
#define EXCEPTION_ENVIRONMENT_CALL_FROM_M_MODE   11
#define EXCEPTION_INSTRUCTION_PAGE_FAULT         12
#define EXCEPTION_LOAD_PAGE_FAULT                13
#define EXCEPTION_RESERVED_E                     14
#define EXCEPTION_STORE_AMO_PAGE_FAULT           15
#define EXCEPTION_UNKOWN_10                      16
#define EXCEPTION_UNKOWN_11                      17
#define EXCEPTION_UNKOWN_12                      18
#define EXCEPTION_UNKOWN_13                      19
#define EXCEPTION_UNKOWN_14                      20
#define EXCEPTION_UNKOWN_15                      21
#define EXCEPTION_UNKOWN_16                      22
#define EXCEPTION_UNKOWN_17                      23
#define EXCEPTION_UNKOWN_18                      24
#define EXCEPTION_FETCH_BUS_ERROR                25
#define EXCEPTION_FETCH_ECC_ERROR                26
#define EXCEPTION_LOAD_PAGE_SPLIT_FAULT          27
#define EXCEPTION_STORE_PAGE_SPLIT_FAULT         28
#define EXCEPTION_BUS_ERROR                      29
#define EXCEPTION_MCODE_INSTRUCTION              30
#define EXCEPTION_TXFMA_OFF                      31

/* Interrupts have highest bit (MXLEN-1) set to 1 */
#define SUPERVISOR_SOFTWARE_INTERRUPT            1
#define MACHINE_SOFTWARE_INTERRUPT               3
#define SUPERVISOR_TIMER_INTERRUPT               5
#define MACHINE_TIMER_INTERRUPT                  7
#define SUPERVISOR_EXTERNAL_INTERRUPT            9
#define MACHINE_EXTERNAL_INTERRUPT               11

#define SUPERVISOR_PENDING_INTERRUPTS(sip)       asm volatile("csrr %0, sip" : "=r"(sip))

//                                   csr       |     rs1      |    funct3   | opcode
#define INST_CSRRx_MASK ((0xFFFULL << 20) | (0x1f << 15) | (0x7 << 12) | (0x7f))

//                                 mhartid     |    rs1=0     |           csrrs
#define INST_CSRRS_MHARTID ((0xF14ULL << 20) | (0 << 15) | (0x2 << 12) | (0x73))

/* Macros to access RISC-V registers */
#define CSR_READ_SCAUSE(value)     asm volatile("csrr %0, scause\n" : "=r"(value));
#define CSR_READ_SSTATUS(value)    asm volatile("csrr %0, sstatus\n" : "=r"(value));
#define CSR_READ_SEPC(value)       asm volatile("csrr %0, sepc\n" : "=r"(value));
#define CSR_READ_STVAL(value)      asm volatile("csrr %0, stval\n" : "=r"(value));

#endif
