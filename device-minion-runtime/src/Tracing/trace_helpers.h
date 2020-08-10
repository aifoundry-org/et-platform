/*------------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 ------------------------------------------------------------------------------ */

#ifndef ET_TRACE_HELPERS_H
#define ET_TRACE_HELPERS_H

#ifdef __cplusplus
namespace device_api {
#endif

/// @brief Initializes trace subsystem, sets up control region, sets all the
/// knobs to default states, and does all the required cache maintainance
void TRACE_init(void);

/// @brief Initializes trace buffer for logging, which includes setting up
/// ring buffer indexes and hart-id of the logging thread in buffer header
void TRACE_init_buffer(void);

/// @brief Does all the cache maintainance required, to allow master to consume
/// the buffer
void TRACE_evict_buffer(void);

/// @brief Does all the cache maintainance to keep the trace control region
/// updated.
///
/// Function serves two purposes. Master is the trace control region writer
/// and in case of Master, this function evicts the dirty cache entries to L3
/// which master has updated. In case of Workers, this function invalidates
/// the trace control region to get the updated value.
/// Note: Worker must never update trace control region.
///
void TRACE_update_control(void);

#ifdef __cplusplus
} // namespace device_api
#endif

#endif // ET_TRACE_HELPERS_H
