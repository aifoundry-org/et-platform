#ifndef MOCK_ETSOC_H
#define MOCK_ETSOC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Model registers as global variables. */
extern uint64_t reg_mhartid;
extern uint64_t reg_hpmcounter3;
extern uint64_t reg_hpmcounter4;
extern uint64_t reg_hpmcounter5;
extern uint64_t reg_hpmcounter6;
extern uint64_t reg_hpmcounter7;
extern uint64_t reg_hpmcounter8;

void etsoc_reset(void); /* Reset regs to 0 */

#define get_hart_id() reg_mhartid

#ifdef __cplusplus
}
#endif

#endif
