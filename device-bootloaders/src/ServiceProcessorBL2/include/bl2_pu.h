/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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