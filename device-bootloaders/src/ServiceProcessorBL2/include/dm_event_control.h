#ifndef __DM_EVENT_CONTROL_H__
#define __DM_EVENT_CONTROL_H__

/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/*! \file dm_events.h
    \brief A C header that defines the the DM events interface, control
            blocks and functions.
*/
/***********************************************************************/

#include "dm_event_def.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "config/mgmt_build_config.h"

/*!
 * @struct struct event_control_block_t
 * @brief structure defining the max error fields and OS objects for
 *      error handling.
 */
struct max_error_count_t
{
    uint32_t pcie_ce_max_count;
    uint32_t pcie_uce_max_count;
    uint32_t ddr_ce_max_count;
    uint32_t ddr_uce_max_count;
    uint32_t sram_ce_max_count;
    uint32_t sram_uce_max_count;
};

/* function prototype to get global max error control block */
volatile struct max_error_count_t  *get_soc_max_control_block(void);

/* event control init function */
int32_t dm_event_control_init(void);

/* Callback prototypes */
void pcie_event_callback(enum error_type type, struct event_message_t *msg);
void sram_event_callback(enum error_type type, struct event_message_t *msg);
void ddr_event_callback(enum error_type type, struct event_message_t *msg);
void power_event_callback(enum error_type type, struct event_message_t *msg);
void wdog_timeout_callback(enum error_type type, struct event_message_t *msg);
void minion_event_callback(enum error_type type, struct event_message_t *msg);
void pmic_event_callback(enum error_type type, struct event_message_t *msg);

#define DM_EVENT_QUEUE_LENGTH           8
#define DM_EVENT_QUEUE_ITEM_SIZE sizeof (struct event_message_t)
#define DM_EVENT_QUEUE_SIZE             DM_EVENT_QUEUE_LENGTH * DM_EVENT_QUEUE_ITEM_SIZE

#define TEST_EVENT_GEN
#ifdef TEST_EVENT_GEN

/* resume events generation task */
void start_test_events(tag_id_t tag_id, msg_id_t msg_id);
void generate_test_event(const struct event_message_t *msg);
#endif

#endif
