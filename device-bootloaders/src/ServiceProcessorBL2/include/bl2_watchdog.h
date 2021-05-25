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
#ifndef __BL2_WATCHDOG_H__
#define __BL2_WATCHDOG_H__

#include <stdint.h>
#include "dm_event_def.h"

/*!
 * @struct struct watchdog_control_block
 * @brief Watchdog features control block
 */
struct watchdog_control_block {
    uint32_t timeout_msec; /**< Current timeout in msecs. */
    uint32_t max_timeout_msec; /**< Max possible timeout in msecs. */
    dm_event_isr_callback event_cb; /**< Event callback handler. */
};

/*! \fn int32_t watchdog_init(uint32_t timeout_msec)
    \brief This function initializes watchdog with the
           given timeout, enables interrupt handler
           for the SPIO_WDT_INTR, enable PMIC Interface
           to handle second interrupt and reset the system
           , spio_wdt_sys_rstn and starts the watch dog
    \param timeout_msec expiration period in msecs
    \return Status indicating success or negative error
*/
int32_t watchdog_init(uint32_t timeout_msec);

/*! \fn int32_t watchdog_error_init(dm_event_isr_callback event_cb)
    \brief This function registers callback for wdog expiration
           ISR and any other hardware configuration needed.
    \param event_cb pointer to the error call back function
    \return Status indicating success or negative error
*/
int32_t watchdog_error_init(dm_event_isr_callback event_cb);

/*! \fn int32_t watchdog_start(void)
    \brief This function starts the watchdog.
    \param NONE
    \return Status indicating success or negative error
*/
int32_t watchdog_start(void);

/*! \fn int32_t watchdog_stop(void)
    \brief This function stops the running watchdog
    \param NONE
    \return Status indicating success or negative error
*/
int32_t watchdog_stop(void);

/*! \fn void watchdog_kick(void)
    \brief This function feeds the watchdog to keep it running
    \param NONE
    \return NONE
*/
void watchdog_kick(void);

/*! \fn int32_t get_watchdog_timeout(uint32_t timeout_msec)
    \brief This function provides current timeout value set
           for watchdog.
    \param timeout_msec pointer to variable for holding the
            timeout value
    \return Status indicating success or negative error
*/
int32_t get_watchdog_timeout(uint32_t *timeout_msec);

/*! \fn int32_t get_watchdog_max_timeout(uint32_t timeout_msec)
    \brief This function provides maximum timeout value that can
           be used for watchdog.
    \param timeout_msec pointer to variable for holding the
            timeout value
    \return Status indicating success or negative error
*/
int32_t get_watchdog_max_timeout(uint32_t *timeout_msec);

//TODO : Make this static with the driver implementation - SW-6751
void watchdog_isr(void); 

#endif
