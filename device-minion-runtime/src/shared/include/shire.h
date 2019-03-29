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

static inline unsigned int __attribute__((always_inline)) get_hart_id(void)
{
    unsigned int ret;

    __asm__ __volatile__ ("csrr %0, mhartid" : "=r" (ret) );

    return ret;
}

static inline unsigned int __attribute__((always_inline)) get_shire_id(void)
{
    return get_hart_id() >> 6;
}

static inline unsigned int __attribute__((always_inline)) get_minion_id(void)
{
    return get_hart_id() >> 1;
}

static inline unsigned int __attribute__((always_inline)) get_thread_id(void)
{
    return get_hart_id() & 1;
}
