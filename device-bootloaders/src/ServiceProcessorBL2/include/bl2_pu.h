/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
#ifndef __BL2_PU__
#define __BL2_PU__

#include <stdint.h>

/*! \fn int PU_Uart_Initialize(uint8_t uart_id)
    \brief This function initializes the peripheral unit UART driver.
    \param uart_id id of the peripheral unit
    \return Status indicating success or negative error
*/
int PU_Uart_Initialize(uint8_t uart_id);

/*! \fn uint32_t PU_SRAM_Read(uint32_t *address)
    \brief This function reads SRAM word.
    \param address pointer to address to read from
    \return Status indicating success or negative error
*/
uint32_t PU_SRAM_Read(uint32_t *address);

/*! \fn uint32_t PU_SRAM_Write(uint32_t *address, uint32_t data)
    \brief This function writes a word to SRAM.
    \param address pointer to address to read from
    \param data data to be written
    \return Status indicating success or negative error
*/
int PU_SRAM_Write(uint32_t *address, uint32_t data);

#endif