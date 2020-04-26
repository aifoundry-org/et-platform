/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _LOG_H
#define _LOG_H

#include <cstdint>

// There is no checker so we make this an empty interface
inline void clearlogstate() {}
inline void log_pc_update(uint64_t) {}
inline void log_xreg_write(uint8_t, uint64_t) {}
inline void log_xreg_late_write(uint8_t, uint64_t) {}
inline void log_freg_write(uint8_t, const mreg_t&, const freg_t&) {}
inline void log_freg_load(uint8_t, const mreg_t&, const freg_t&) {}
inline void log_mreg_write(uint8_t, const mreg_t&) {}
inline void log_fflags_write(uint64_t) {}
inline void log_mem_write(bool, int, uint64_t, uint64_t, uint64_t) {}
inline void log_mem_read(bool, int, uint64_t, uint64_t) {}
inline void log_mem_read_write(bool, int, uint64_t, uint64_t, uint64_t) {}
inline void log_trap( uint64_t, uint64_t, uint64_t, uint64_t) {}
inline void log_gsc_progress(uint64_t) {}

// tensor_error
inline void log_tensor_error_value(uint16_t) {}

// TensorLoad
inline void log_tensor_load(uint8_t, uint8_t, uint8_t, uint64_t) {}
inline void log_tensor_load_scp_write(uint8_t, const uint64_t*) {}

// TensorFMA
inline void log_tensor_fma_new_pass() {}
inline void log_tensor_fma_write(uint8_t, bool, uint8_t, int, uint32_t) {}

// TensorQuant
inline void log_tensor_quant_new_transform(bool = false) {}
inline void log_tensor_quant_write(uint8_t, uint8_t, const mreg_t&, const freg_t&) {}

// TensorReduce
inline void log_tensor_reduce(bool, uint8_t, uint8_t) {}
inline void log_tensor_reduce_write(uint8_t, const freg_t&) {}

// TensorStore
inline void log_tensor_store(bool, uint8_t, uint8_t, uint8_t) {}
inline void log_tensor_store_write(uint64_t, uint32_t) {}
inline void log_tensor_store_error(uint16_t) {}

#endif // _LOG_H
