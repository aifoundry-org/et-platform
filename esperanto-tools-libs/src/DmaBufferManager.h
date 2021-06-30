/*-------------------------------------------------------------------------
 * Copyright (C) 2021, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/

#pragma once
#include "DmaBufferImp.h"
#include "utils.h"
#include <device-layer/IDeviceLayer.h>

namespace rt {
class DmaBufferManager {
public:
  explicit DmaBufferManager(dev::IDeviceLayer* deviceLayer, int device)
    : deviceLayer_(deviceLayer)
    , device_(device) {
  }
  std::unique_ptr<DmaBuffer> allocate(size_t size, bool writeable);
  // check if this region of memory falls into an already allocated dmaBuffer
  bool isDmaBuffer(const std::byte* address) const;
  void release(DmaBufferImp* dmaBuffer);

private:
  std::vector<DmaBufferImp> inUseBuffers_;
  dev::IDeviceLayer* deviceLayer_;
  mutable std::mutex mutex_;
  const int device_;
};

} // namespace rt