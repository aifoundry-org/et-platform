#ifndef __DM_EVENT_DEF_H__
#define __DM_EVENT_DEF_H__

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

/*! \file event_control.h
    \brief A C header that defines the the error control interface
           SP BL2
*/
/***********************************************************************/

#include <stdint.h>
#include "dm.h"
#include "sp_mm_comms_spec.h"

/*!
 * @enum enum event_class
 * @brief Enum defining error severity class
 */
enum event_class {
    INFO,           /**< Information. */
    WARNING,        /**< Warning */
    CRITICAL,       /**< Critical */
    FATAL,          /**< FATAL */
};

/*!
 * @enum event_ids
 * @brief Enum defining event ids. Event IDs 0 - 255 are used for OPS events and
 *        256 - 512 are used for management events
 */
enum event_ids {
    PCIE_CE = 256,         /**< Correctable PCIE error. */
    PCIE_UCE,              /**< Uncorrectable PCIE error. */
    DRAM_CE,               /**< Correctable DRAM error. */
    DRAM_UCE,              /**< Uncorrectable DRAM error. */
    SRAM_CE,               /**< Correctable SRAM error. */
    SRAM_UCE,              /**< Uncorrectable SRAM error. */
    THERMAL_LOW,           /**< Lower thermal threshold exceeded. */
    PMIC_ERROR,            /**<  */
    WDOG_INTERNAL_TIMEOUT, /**< Internal WDT Interrupt. */
    WDOG_EXTERNAL_TIMEOUT, /**< External PMIC Reset. */
    FW_BOOT,               /**< FW boot error. */
    MINION_EXCEPT_TH,      /**< Minion error count threshold exceeded. */
    MINION_HANG_TH,        /**< Minion hang threshold exceeded. */
    THROTTLE_TIME,         /** < Event for time in throttling state */
    MAX_ERROR_EVENT = 512, /**< Max limit for error IDs. */ 
};

/*!
 * @enum error_type
 * @brief Enum defining event/error type
 */
enum error_type {
    CORRECTABLE,
    UNCORRECTABLE,
};

/*!
 * @struct struct event_message_t
 * @brief structure defining the event message format
 */
struct event_payload {
    uint16_t class_count; /**< payload containing the event class[1:0], event count[15:2] */
    uint64_t syndrome[2]; /**< Hardware defined syndrome info */
} __attribute__((__packed__));

/*!
 * @struct struct event_message_t
 * @brief structure defining the event message format
 */
struct event_message_t {
    struct cmn_header_t header; /**< See struct cmn_header_t. */
    struct event_payload payload; /**< See struct event_payload */
} __attribute__((packed, aligned(8)));


#define EVENT_CLASS_MASK       0x3
#define EVENT_ERROR_COUNT_MASK 0x3FFF

#define FILL_EVENT_HEADER(header, id, sz) \
                                (header)->msg_id = id; \
                                (header)->size = sz;

#define FILL_EVENT_PAYLOAD(payload, class, count, syndrome1, syndrome2) \
                                    (payload)->class_count =  (((count)&EVENT_ERROR_COUNT_MASK) << 2) | \
                                     ((class) & EVENT_CLASS_MASK); \
                                    (payload)->syndrome[0] = (syndrome1); \
                                    (payload)->syndrome[1] = (syndrome2); \

#define EVENT_PAYLOAD_GET_EVENT_CLASS(payload) (((payload)->class_count) & EVENT_ERROR_CLASS_MASK)
#define EVENT_PAYLOAD_GET_ERROR_COUNT(payload) ((((payload)->class_count) >> 2) & EVENT_ERROR_COUNT_MASK)

typedef void (*dm_event_isr_callback)(enum error_type, struct event_message_t *msg);

#endif
