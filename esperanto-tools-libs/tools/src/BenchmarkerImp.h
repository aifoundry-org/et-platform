/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------*/

#pragma once
#include <runtime/IRuntime.h>
#include <tools/IBenchmarker.h>
namespace rt {
class BenchmarkerImp : public IBenchmarker {
public:
  explicit BenchmarkerImp(IRuntime* runtime);
  ~BenchmarkerImp();
  SummaryResults run(Options options, DeviceMask deviceMask) override;

private:
  IRuntime* runtime_;
};
} // namespace rt