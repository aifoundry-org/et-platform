/*-------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include <cereal/cereal.hpp>
#include <cstddef>
#include <limits>
#include <stdint.h>
namespace rt {
namespace Request {

// these requests corresponds to first version of the protocol, is expected to add more versions here in the future
enum class Type : uint32_t {
  VERSION,
  MALLOC,
  FREE,
  MEMCPY_H2D,
  MEMCPY_D2H,
  CREATE_STREAM,
  DESTROY_STREAM,
  LOAD_CODE,
  UNLOAD_CODE,
  KERNEL_LAUNCH,
  GET_DEVICES,
  ABORT_STREAM
};

struct Header {
  uint32_t size_;
  Type type_;
};

struct Version {
  uint64_t value_;
  template <typename T> void archive(T& archive) {
    archive(value_);
  }
};

struct Malloc {
  size_t size_;
  int device_;
  uint32_t alignment_;
  template <typename T> void archive(T& archive) {
    archive(size_, device_, alignment_);
  }
};

struct Free {
  int device_;
  std::byte* address_;
  template <typename T> void archive(T& archive) {
    archive(device_, address_);
  }
};

struct AbortStream {
  int streamId_;
  template <typename T> void archive(T& archive) {
    archive(streamId_);
  }
};

struct Memcpy {};

} // namespace Request

namespace Response {
enum class Type : uint32_t {
  VERSION = 0x7FFFFFFF,
  MALLOC,
  FREE,
  MEMCPY_H2D,
  MEMCPY_D2H,
  CREATE_STREAM,
  DESTROY_STREAM,
  LOAD_CODE,
  UNLOAD_CODE,
  KERNEL_LAUNCH,
  GET_DEVICES,
  ABORT_STREAM
};

struct Header {
  uint32_t size_;
  Type type_;
};

} // namespace Response
} // namespace rt