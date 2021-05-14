/*-------------------------------------------------------------------------
* Copyright (C) 2018, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

#ifndef __SYSTEM_REGISTERS_H__
#define __SYSTEM_REGISTERS_H__

#include <stdint.h>

#define SYSTEM_REGISTER_GET_MISA(val) \
    __asm__ __volatile__("csrr    %[result], misa" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MVENDORID(val) \
    __asm__ __volatile__("csrr    %[result], mvendorid" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MARCHID(val) \
    __asm__ __volatile__("csrr    %[result], marchid" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MIMPID(val) \
    __asm__ __volatile__("csrr    %[result], mimpid" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MHARTID(val) \
    __asm__ __volatile__("csrr    %[result], mhartid" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MSTATUS(val) \
    __asm__ __volatile__("csrr    %[result], mstatus" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MTVEC(val) \
    __asm__ __volatile__("csrr    %[result], mtvec" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MTIME(val) \
    __asm__ __volatile__("csrr    %[result], time" : [result] "=r"(val) : :);

#define SYSTEM_REGISTER_GET_MTIMECMP(val) \
    __asm__ __volatile__("csrr    %[result], timecmp" : [result] "=r"(val) : :);

uint64_t system_register_get_mtvec(void);
uint64_t system_register_set_mtvec(void);
uint64_t system_register_get_medeleg(void);
uint64_t system_register_set_medeleg(void);
uint64_t system_register_get_mideleg(void);
uint64_t system_register_set_mideleg(void);
uint64_t system_register_get_mip(void);
uint64_t system_register_set_mip(void);
uint64_t system_register_get_mie(void);
uint64_t system_register_set_mie(void);
uint64_t system_register_get_mtime(void);
uint64_t system_register_set_mtime(void);
uint64_t system_register_get_mtimecmp(void);
uint64_t system_register_set_mtimecmp(void);
uint64_t system_register_get_minstret(void);
uint64_t system_register_set_minstret(void);
uint64_t system_register_get_mcycle(void);
uint64_t system_register_set_mcycle(void);
uint64_t system_register_get_mcounteren(void);
uint64_t system_register_set_mcounteren(void);
uint64_t system_register_get_mscratch(void);
uint64_t system_register_set_mscratch(void);
uint64_t system_register_get_mepc(void);
uint64_t system_register_set_mepc(void);
uint64_t system_register_get_mcause(void);
uint64_t system_register_set_mcause(void);
uint64_t system_register_get_mtval(void);
uint64_t system_register_set_mtval(void);

uint64_t system_register_get_mhpmcounter(uint32_t index);
uint64_t system_register_get_mhpmevent(uint32_t index);

#endif
