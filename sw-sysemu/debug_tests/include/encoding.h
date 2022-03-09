#pragma once


#define CSR_TDATA1 0x7a1
#define CSR_TDATA2 0x7a2
#define CSR_DPC    0x7b1
#define CSR_DDATA0 0x7b8

#define EBREAK 0x100073ull

#define CSRRW(rd, csr, rs1)  ((csr << 20) | (rs1 << 15) | (rd << 7) | 0x1073ull)
#define CSRRS(rd, csr, rs1)  ((csr << 20) | (rs1 << 15) | (rd << 7) | 0x2073ull)
#define CSRRC(rd, csr, rs1)  ((csr << 20) | (rs1 << 15) | (rd << 7) | 0x3073ull)
#define CSRRWI(rd, csr, imm) ((csr << 20) | (imm << 15) | (rd << 7) | 0x5073ull)
#define CSRRSI(rd, csr, imm) ((csr << 20) | (imm << 15) | (rd << 7) | 0x6073ull)
#define CSRRCI(rd, csr, imm) ((csr << 20) | (imm << 15) | (rd << 7) | 0x7073ull)

#define CSRR(rd, csr)   CSRRS(rd, csr, 0)
#define CSRW(csr, rs1)  CSRRW(0, csr, rs1)
#define CSRS(csr, rs)   CSRRS(0, csr, rs1)
#define CSRWI(csr, imm) CSRRWI(0, csr, imm)
#define CSRSI(csr, imm) CSRRSI(0, csr, imm)
#define CSRCI(csr, imm) CSRRCI(0, csr, imm)
