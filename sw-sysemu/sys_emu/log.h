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
#include "processor.h"

// Run control
inline void notify_pc_update(const bemu::Hart&, uint64_t) {}
inline void notify_trap(const bemu::Hart&,  uint64_t, uint64_t, uint64_t, uint64_t) {}

// General purpose registers (late writes are operations that take more than one cycle)
inline void notify_xreg_write(const bemu::Hart&, uint8_t, uint64_t) {}
inline void notify_xreg_late_write(const bemu::Hart&, uint8_t, uint64_t) {}

// Different flavors of writes to the VPU register file (or fregs)
inline void notify_freg_load(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) {}
inline void notify_freg_intmv(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) {}
inline void notify_freg_write(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) {}
inline void notify_freg_read(const bemu::Hart&, uint8_t) {}

// Memory write backs
inline void notify_mem_write(const bemu::Hart&, bool, int, uint64_t, uint64_t, uint64_t) {}
inline void notify_mem_read(const bemu::Hart&, bool, int, uint64_t, uint64_t) {}
inline void notify_mem_read_write(const bemu::Hart&, bool, int, uint64_t, uint64_t, uint64_t) {}

// Mask registers and misc CSRs
inline void notify_mreg_write(const bemu::Hart&, uint8_t, const bemu::mreg_t&) {}
inline void notify_fflags_write(const bemu::Hart&, uint64_t) {}
inline void notify_gsc_progress(const bemu::Hart&, uint64_t) {}

// TensorError
inline void notify_tensor_error_value(const bemu::Hart&, uint16_t) {}

// TensorLoad
inline void notify_tensor_load(const bemu::Hart&, uint8_t, uint8_t, uint8_t, uint64_t) {}
inline void notify_tensor_load_scp_write(const bemu::Hart&, uint8_t, const uint64_t*) {}

// TensorFMA
inline void notify_tensor_fma_new_pass(const bemu::Hart&) {}
inline void notify_tensor_fma_write(const bemu::Hart&, uint8_t, bool, uint8_t, int, uint32_t) {}

// TensorQuant
inline void notify_tensor_quant_new_transform(const bemu::Hart&, bool = false) {}
inline void notify_tensor_quant_write(const bemu::Hart&, uint8_t, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) {}

// TensorReduce
inline void notify_tensor_reduce(const bemu::Hart&, bool, uint8_t, uint8_t) {}
inline void notify_tensor_reduce_write(const bemu::Hart&, uint8_t, const bemu::freg_t&) {}

// TensorStore
inline void notify_tensor_store(const bemu::Hart&, bool, uint8_t, uint8_t, uint8_t) {}
inline void notify_tensor_store_write(const bemu::Hart&, uint64_t, uint32_t) {}
inline void notify_tensor_store_error(const bemu::Hart&, uint16_t) {}

#endif // _LOG_H
