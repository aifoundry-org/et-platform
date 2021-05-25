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
#include "bl2_cache_control.h"

/* The driver can populate this structure with the defaults that will be used during the init
    phase.*/
static struct sram_event_control_block  event_control_block __attribute__((section(".data")));

int32_t sram_error_control_init(dm_event_isr_callback event_cb)
{
    event_control_block.event_cb = event_cb;
    return  0;
}

int32_t sram_error_control_deinit(void)
{
    return  0;
}

int32_t sram_enable_uce_interrupt(void)
{
    return  0;
}

int32_t sram_disable_ce_interrupt(void)
{
    return  0;
}

int32_t sram_disable_uce_interrupt(void)
{
    return  0;
}

int32_t sram_set_ce_threshold(uint32_t ce_threshold) 
{
    /* set countable errors threshold */
    event_control_block.ce_threshold = ce_threshold;
    return 0;
}

int32_t sram_get_ce_count(uint32_t *ce_count) 
{
    /* get correctable errors count */
    *ce_count = event_control_block.ce_count;
    return 0;
}

int32_t sram_get_uce_count(uint32_t *uce_count) 
{
    /* get un-correctable errors count */
    *uce_count = event_control_block.uce_count;
    return 0;
}

void sram_error_threshold_isr(void) 
{
    /* TODO: This is just an example implementation.
       The final driver implementation will read these values from the
       hardware, create a message and invoke call back with message and error type as parameters.
    */
    uint8_t error_type = CORRECTABLE;

    if((error_type == UNCORRECTABLE) || 
        (++event_control_block.ce_count > event_control_block.ce_threshold)) {
            
            struct event_message_t message;

            /* add details in message header and fill payload */ 
            FILL_EVENT_HEADER(&message.header, SRAM_UCE,
                    sizeof(struct event_message_t) - sizeof(struct cmn_header_t));
            FILL_EVENT_PAYLOAD(&message.payload, CRITICAL, 1024, 1, 0);
        
            /* call the callback function and post message */
            event_control_block.event_cb(CORRECTABLE, &message);
    }

}
