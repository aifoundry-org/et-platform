/*-------------------------------------------------------------------------
 * Copyright (C) 2020, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "ITarget.h"
#include "KernelParametersCache.h"
#include "NewCore/EventManager.h"
#include <atomic>
#include <thread>

namespace rt {
class MailboxReader {
public:
  explicit MailboxReader(ITarget* target, KernelParametersCache* kernelParametersCache, EventManager* eventManager);
  ~MailboxReader();

  void stop();

private:
  std::thread reader_;
  std::atomic<bool> run_;
  ITarget* target_;
  EventManager* eventManager_;
  KernelParametersCache* kernelParametersCache_;
};
} // namespace rt