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
/*! \file plic.h
    \brief A C header that defines the PLIC Driver's public
    interfaces
*/
/***********************************************************************/
#ifndef PLIC_H
#define PLIC_H

#include <stdint.h>
#include "etsoc_hal/inc/pu_plic_intr_device.h"

/*! \fn void PLIC_Init(void)
    \brief Initialize PLIC driver
    \returns none
*/
void PLIC_Init(void);

/*! \fn void PLIC_RegisterHandler(uint32_t intID, uint32_t priority, void (*handler)(uint32_t intID))
    \brief Registers a handler for a given interrupt source
    \param intID Interrupt ID
    \param priority Interrupt priority
    \param handler function pointer to handler routine
    \return none
*/
void PLIC_RegisterHandler(uint32_t intID, uint32_t priority, void (*handler)(uint32_t intID));

/*! \fn void PLIC_UnregisterHandler(uint32_t intID)
    \brief Unregisters a handler for a given interrupt source
    \param intID Interrupt ID
    \return none
*/
void PLIC_UnregisterHandler(uint32_t intID);

/*! \fn void PLIC_Dispatch(void)
    \brief Dispatches all pending interrupts
    \return none
*/
void PLIC_Dispatch(void);

#endif /* INTERRUPTS_DEFS_H */
