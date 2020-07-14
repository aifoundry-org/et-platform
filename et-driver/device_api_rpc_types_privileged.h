/*------------------------------------------------------------------------------
 * Copyright (C) 2019, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

// WARNING: this file is auto-generated do not edit directly

#ifndef ET_DEVICE_API_RPC_TYPES_PRIVILEGED_H
#define ET_DEVICE_API_RPC_TYPES_PRIVILEGED_H

#include "device_api_message_types.h"

// DeviceAPI Privileged Enumerations
typedef uint32_t et_dma_chan_id_e;

/// @brief IDs of the different DMA channels we have on the device.
enum ET_DMA_CHAN_ID {
  ET_DMA_CHAN_ID_READ_0 = 0,
  ET_DMA_CHAN_ID_READ_1 = 1,
  ET_DMA_CHAN_ID_READ_2 = 2,
  ET_DMA_CHAN_ID_READ_3 = 3,
  ET_DMA_CHAN_ID_WRITE_0 = 4,
  ET_DMA_CHAN_ID_WRITE_1 = 5,
  ET_DMA_CHAN_ID_WRITE_2 = 6,
  ET_DMA_CHAN_ID_WRITE_3 = 7,

};

typedef uint32_t et_dma_state_e;

/// @brief State of a DMA
enum ET_DMA_STATE {
  ET_DMA_STATE_IDLE = 0,
  ET_DMA_STATE_ACTIVE = 1,
  ET_DMA_STATE_DONE = 2,
  ET_DMA_STATE_ABORTING = 3,
  ET_DMA_STATE_ABORTED = 4,

};



// DeviceAPI Privileged structs
// Structs that are being used in other tests



// The real  Privileged RPC messages that we exchange

/// @brief Request the version of the device_api privileged-level supported by the target, advertise the one we support
struct device_api_version_cmd_t {
  struct command_header_t command_info;
  uint64_t  major; /// Sem Version Major
  uint64_t  minor; /// Sem Version Minor
  uint64_t  patch; /// Sem Version Patch
  uint64_t  api_hash; /// HASH of the API specification. Unique identifier of the API contents.
};

/// @brief Return the version implemented by the target
struct device_api_version_rsp_t {
  struct response_header_t response_info;
  uint64_t  major; /// Sem Version Major
  uint64_t  minor; /// Sem Version Minor
  uint64_t  patch; /// Sem Version Patch
  uint64_t  api_hash; /// HASH of the API specification. Unique identifier of the API contents implemented by the target.
  uint8_t  accept; /// Set to true by the target if it can support the version provided by the requestor.
};

/// @brief Execute the reflect test cmd
struct reflect_cmd_t {
  struct command_header_t command_info;
  int32_t  dummy; /// Dummy field
};

/// @brief Response to reflect test
struct reflect_rsp_t {
  struct response_header_t response_info;
  int32_t  dummy; /// Dummy field
};

/// @brief Execute a DMA on channel until it completes
struct dma_run_to_done_cmd_t {
  struct command_header_t command_info;
  et_dma_chan_id_e  chan; /// DMA channel
};

/// @brief Response marking the commpletion of a DMA
struct dma_run_to_done_rsp_t {
  struct response_header_t response_info;
  et_dma_chan_id_e  chan; /// DMA channel
  et_dma_state_e  status; /// Status of the DMA operation
};


#endif // ET_DEVICE_API_RPC_TYPES_PRIVILEGED_H
