/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
/*! \file mm_to_cm_iface.h
    \brief A C header that define the MM to CM Interface.
*/
/***********************************************************************/
#ifndef MM_TO_CM_IFACE_DEFS_H
#define MM_TO_CM_IFACE_DEFS_H

typedef struct {
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t scause;
    uint64_t *regs;
} __attribute__((packed)) internal_execution_context_t;

/*! \fn void MM_To_CM_Iface_Init(void)
    \brief This function initialize the MM to CM message handler.
    \return None
*/
void MM_To_CM_Iface_Init(void);

/*! \fn void MM_To_CM_Iface_Multicast_Receive(void *const optional_arg)
    \brief This function handles MM unicast messages.
    \param optional_arg Pointer to optional arguments (if needed)
    \return None
*/
void MM_To_CM_Iface_Multicast_Receive(void *const optional_arg);

/*! \fn void MM_To_CM_Iface_Main_Loop(void)
    \brief This function starts MM message handler on CM side.
    \return None
*/
void __attribute__((noreturn)) MM_To_CM_Iface_Main_Loop(void);

#endif /* MM_TO_CM_IFACE_DEFS_H */
