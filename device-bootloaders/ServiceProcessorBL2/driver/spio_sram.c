#include "spio_sram.h"
#include "etsoc_hal/inc/hal_device.h"

uint32_t SPIO_SRAM_Read_Word (uint64_t *address)
{
    /* check if address range is valid */
    if (((uintptr_t)address > (uintptr_t)R_SP_SRAM_BASEADDR) && 
        ((uintptr_t)address < (uintptr_t)(R_SP_SRAM_BASEADDR + R_SP_SRAM_SIZE))) {
        
        return (uint32_t)*address;
    }

    return 0;

}

int SPIO_SRAM_Write_Word (uint64_t *address, uint32_t data)
{
    /* check if address range is valid */
    if (((uintptr_t)address > (uintptr_t)R_SP_SRAM_BASEADDR) && 
        ((uintptr_t)address < (uintptr_t)(R_SP_SRAM_BASEADDR + R_SP_SRAM_SIZE))) {
        
        *address = data;
    }

    return 0;
}
