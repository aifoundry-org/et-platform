/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_DEVICE_APIS_MESSAGE_TYPES_H
#define ET_DEVICE_APIS_MESSAGE_TYPES_H

#include <stdint.h>

/* WARNING: The following data-structures are part of the device-api data exchanged between
 host and device. The requirement is that fields are naturally aligned. To easily enforce this
 make sure that member fields are ordered by decreasing size. All the structures here must be
 64-bit aligned. cmn_header_t structure is common to command and response and should always be
 kept in the start of command/response header. */

typedef uint16_t tag_id_t;
typedef uint16_t msg_id_t;
typedef uint16_t msg_size_t;

/*! \struct cmn_header_t
    \brief Common header, common to command, response, and events
    \warning Not to be used directly in device-api message. Must be aligned to 64-bits when used.
*/
struct cmn_header_t {
    msg_size_t size;                 /**< size of payload that follows the message header */
    tag_id_t tag_id;                 /**< unique ID to correlate commands/responses across Host-> Device */
    msg_id_t msg_id;                 /**< unique ID to differentiate commands/responses/events generated from host */
    uint16_t flags;                  /**< This bit mask with the following fields
                                           Bit [0] - Command barrier - allows all command prior to it to complete
                                           Bit [1] - Kernel Launch will include a pointer to a Compute Kernel trace buffer
                                           Bit [2:3] - DMA buffer type : Encoding:
                                               - 00 Host Managed (Default)
                                               - 01 MEM_OPS_MMFW_TRACE
                                               - 10 MEM_OPS_CMFW_TRACE
                                               - 11 MEM_OPS_{MM+CM}_TRACE
                                           Bit [4] - Flush L3 prior to launching Kernel on Compute Minions 
                                           Bit [5:8] - Scale factor for Command (Note today only DMA/Kernel) Execution timeout 
                                                       is supported. The scale will be multiplied with the base period-> 100 mS. 
                                                       So example if 300 mS is required, Scale[8:5] = 0x3 */
} __attribute__((packed, aligned(8)));

/*! \struct cmd_header_t
    \brief Command header for all commands host to device
*/
struct cmd_header_t {
    struct cmn_header_t cmd_hdr;     /**< Command header */
} __attribute__((packed, aligned(8)));

/*! \struct rsp_header_t
    \brief Response header for all command responses from device to host
*/
struct rsp_header_t {
    struct cmn_header_t rsp_hdr;     /**< Response header */
} __attribute__((packed, aligned(8)));

/*! \struct dev_mgmt_rsp_hdr_extn_t
    \brief Response header extension for all Device Management responses
*/
struct dev_mgmt_rsp_hdr_extn_t {
    uint64_t device_latency_usec;    /**< Device cycle Latency to execute a command */
    int32_t status;                  /**< Status of the Command execution */
    uint8_t pad[4];                  /**< Padding to make it align to 64-bits */
} __attribute__((packed, aligned(8)));

/**<  @brief DM request header. This header is attached to each request */
typedef struct cmd_header_t dev_mgmt_cmd_header_t;

/*! \struct dev_mgmt_rsp_header_t
    \brief DM response header. This header is attached to each response
*/
struct dev_mgmt_rsp_header_t {
    struct cmn_header_t rsp_hdr;                 /**< Response header */
    struct dev_mgmt_rsp_hdr_extn_t rsp_hdr_ext;  /**< Response header extension */
} __attribute__((packed, aligned(8)));

/*! \struct evt_header_t
    \brief Event header for all events from the device to the host
*/
struct evt_header_t {
    uint16_t size;        /**< size of the event message */
    uint16_t event_id;    /**< unique ID to differentiate event class(enum) from host */
    uint8_t pad[4];       /**< Padding to make it align to 64-bits */
} __attribute__((packed, aligned(8)));

#endif // ET_DEVICE_APIS_MESSAGE_TYPES_H
