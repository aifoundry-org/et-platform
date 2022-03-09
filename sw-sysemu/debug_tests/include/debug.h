#pragma once

#include <stdint.h>
#include <stdbool.h>

/* FIXME(cabul): Not required, revise BEMU */
bool Setup_Debug(void); /* custom */

void Select_Harts(uint64_t shire_id, uint64_t thread_mask);
void Unselect_Harts(uint64_t shire_id, uint64_t thread_mask);

void Halt_On_Reset(bool value); /* custom */

bool Check_Running(void);
bool Check_Halted(void);
bool Have_Reset(void);     /* custom */
void Ack_Have_Reset(void); /* custom */

bool Halt_Harts(void);
bool Resume_Harts(void);
bool Reset_Harts(void); /* custom */

bool Execute_Insns(uint64_t hart_id, uint32_t* insns, unsigned n_insns); /* custom */

uint64_t Read_GPR(uint64_t hart_id, unsigned reg);
void     Read_All_GPR(uint64_t hart_id, uint64_t* regs);
void     Write_GPR(uint64_t hart_id, unsigned reg, uint64_t value);

uint64_t Read_CSR(uint64_t hart_id, unsigned reg);
void     Write_CSR(uint64_t hart_id, unsigned reg, uint64_t value);
