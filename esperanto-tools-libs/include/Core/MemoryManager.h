//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_MANAGER_H
#define ET_RUNTIME_MEMORY_MANAGER_H

#include <cstddef>
#include <map>
#include <memory>

class EtDevice;

namespace et_runtime {

namespace device {

// Region of host or device memory region.
struct EtMemRegion {
  void *region_base;
  size_t region_size;
  std::map<const void *, size_t>
      alloced_ptrs; // alloced ptr -> size of alloced area

  EtMemRegion(void *ptr, size_t size) : region_base(ptr), region_size(size) {}

  static constexpr size_t kAlign = 1 << 20; // 1M
  bool isPtrAlloced(const void *ptr);
  void *alloc(size_t size);
  void free(void *ptr);
  void print();
  bool isPtrInRegion(const void *ptr) {
    return ptr >= region_base &&
           (uintptr_t)ptr < (uintptr_t)region_base + region_size;
  }
};

/// @brief MemoryManager, responsible for tracking device memory use.
class MemoryManager {
public:
  static constexpr size_t kHostMemRegionSize = (1 << 20) * 256;

  MemoryManager(EtDevice &dev) : device_(dev) { initMemRegions(); }
  ~MemoryManager() { uninitMemRegions(); }

  // private:
  void initMemRegions();
  void uninitMemRegions();
  std::unique_ptr<EtMemRegion> host_mem_region;
  std::unique_ptr<EtMemRegion> dev_mem_region;
  std::unique_ptr<EtMemRegion> kernels_dev_mem_region;
  EtDevice &device_;
};
} // namespace device
} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_MANAGER_H
