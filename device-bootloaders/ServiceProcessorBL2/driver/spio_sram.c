#include "spio_sram.h"


int SPIO_SRAM_Read_Word (uint64_t address)
{
    /* SRAM read word interface to TF*/
    (void)address;
    return 0;

}

int SPIO_SRAM_Write_Word (uint64_t address, uint32_t data)
{
    /* SRAM write word interface to TF*/
    (void)address;
    (void)data;
    return 0;
}
