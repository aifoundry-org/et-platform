/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
#ifndef MM_TO_CM_IFACE_DEFS_H
#define MM_TO_CM_IFACE_DEFS_H

typedef struct {
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t scause;
    uint64_t *regs;
} __attribute__((packed)) swi_execution_context_t;

void MM_To_CM_Iface_Init(void);
void MM_To_CM_Iface_Multicast_Receive(void *const optional_arg);
void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void);

#endif /* MM_TO_CM_IFACE_DEFS_H */
