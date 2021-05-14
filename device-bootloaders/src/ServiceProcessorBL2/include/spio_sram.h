#ifndef __SPIO_SRAM__
#define __SPIO_SRAM__

#include <stdint.h>

/*! \fn uint32_t SPIO_SRAM_Read_Word (uint64_t *address)
    \brief This function reads a word from SRAM and return it to caller.
    \param address address to read word from
    \return data on the address provided
*/

uint32_t SPIO_SRAM_Read_Word (uint64_t *address);

/*! \fn int8_t SPIO_SRAM_Write_Word (uint64_t *address, uint32_t data)
    \brief This function write a word to SRAM on supplied address. 
    \param address address to write word data 
    \param data    data to be written to sram
    \return Status
*/

int SPIO_SRAM_Write_Word (uint64_t *address, uint32_t data);

#endif
