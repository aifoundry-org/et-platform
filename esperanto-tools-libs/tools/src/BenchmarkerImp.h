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
#include <device-layer/IDeviceLayer.h>
#include <runtime/IRuntime.h>
#include <tools/IBenchmarker.h>

namespace rt {
class BenchmarkerImp : public IBenchmarker {
public:
  explicit BenchmarkerImp(dev::IDeviceLayer* deviceLayer, const std::string& kernelsDirPath);
  ~BenchmarkerImp();
  IRuntime* getRuntime() override {
    return runtime_.get();
  }
  Results run(Options options) override;

private:
  dev::IDeviceLayer* deviceLayer_;
  std::byte* dH2DBuffer_ = nullptr;
  std::byte* dD2HBuffer_ = nullptr;
  DeviceId device_;
  RuntimePtr runtime_;
  KernelId jumpLoop_;
};
} // namespace rt