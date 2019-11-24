/**
 * Copyright (c) 2018-present, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ET_RUNTIME_SUPPORT_MEMORY_RANGE_H
#define ET_RUNTIME_SUPPORT_MEMORY_RANGE_H

#include <chrono>

namespace et_runtime {
namespace support {
/// @brief Struct holding the begining and size of an allocated buffer
///
/// This struct will be used to indentify the allocated memory regions
/// It will provide a comparator operator that can be used for using this
/// structure in an associative container.
struct MemoryRange {
  MemoryRange() = default;
  // The less operator will return true iff this range is not overlapping
  // and to the left of the "other" one
  bool operator<(const MemoryRange other) const {
    return (addr_ + size_) <= other.addr_;
  }
  uintptr_t end() const { return addr_ + size_; }
  uintptr_t addr_ = 0;
  size_t size_ = 0;
};

} // namespace support

} // namespace et_runtime
#endif // ET_RUNTIME_SUPPORT_MEMORY_RANGE_H
