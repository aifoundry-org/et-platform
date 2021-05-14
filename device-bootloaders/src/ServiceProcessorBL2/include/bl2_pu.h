#ifndef __BL2_PU__
#define __BL2_PU__

#include <stdint.h>

int PU_Uart_Initialize(uint8_t uart_id);
uint32_t PU_SRAM_Read(uint32_t *address);
int PU_SRAM_Write(uint32_t *address, uint32_t data);

#endif