#include "interrupt.h"

void (*vectorTable[PU_PLIC_INTR_CNT])(void);
void* pullVectorTable = vectorTable;
