#ifndef _IPC_H
#define _IPC_H

#include "emu_defines.h"

void ipc_init(const char *type, int debug);
void ipc_init_xreg(xreg dst);
void ipc_init_mreg(mreg dst);
void ipc_int(opcode opc, xreg dst, xreg src1, xreg src2,char *dis);
void ipc_f2x(opcode opc, xreg dst, freg src1, char *dis);
void ipc_f2x(opcode opc, xreg dst, freg src1, freg src2, char *dis);
void ipc_ld(opcode opc, xreg dst, xreg src1, uint64_t addr,char *dis);
void ipc_ld(opcode opc, int count, int size, freg dst, xreg src1, uint64_t addr,char *dis);
void ipc_st(opcode opc, int count, int size, freg src1, xreg base, uint64_t addr,char *dis);
void ipc_st(opcode opc, int count, int size, xreg src1, xreg base, uint64_t addr,char *dis);
void ipc_gt(opcode opc, int count, int size, freg dst, freg src1, xreg base, uint64_t addr,char *dis, int idx);
void ipc_sc(opcode opc, int count, int size, freg src1, freg src2, xreg base, uint64_t addr,char *dis);
void ipc_pi(opcode opc, int count, freg dst, freg src1, freg src2, freg src3,char *dis);
void ipc_ps(opcode opc, int count, freg dst, freg src1, freg src2, freg src3,char *dis);
void ipc_msk(opcode opc, mreg dst, freg src1, freg src2,char *dis);
void ipc_msk(opcode opc, mreg dst, mreg src1, mreg src2,char *dis);
void ipc_texsnd(xreg src1, xreg src2, freg fsrc, char *dis);
void ipc_texrcv(freg dst, char *dis);
void ipc_print_stats(const char *type);

#endif // _IPC_H
