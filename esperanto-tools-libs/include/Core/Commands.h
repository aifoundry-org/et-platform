//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the2
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

#ifndef ET_RUNTIME_MEMORY_COMMANDS_H
#define ET_RUNTIME_MEMORY_COMMANDS_H

#include "Support/HelperMacros.h"

#include "etrt-bin.h"
//#include "utils.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <vector>


namespace et_runtime {

class Device;

class EtAction {
public:
  EtAction() = default;
  virtual ~EtAction() = default;
  virtual void execute(et_runtime::Device *device_target) = 0;
  virtual bool readyForExecution() { return true; }
};

class EtActionEvent : public EtAction {
  bool executed = false;
  std::mutex observer_mutex;
  std::condition_variable observer_cond_var;

public:
  virtual void execute(et_runtime::Device *device_target);
  void observerWait();
  bool isExecuted() {
    std::lock_guard<std::mutex> lk(observer_mutex);
    return executed;
  }
};

class EtActionEventWaiter : public EtAction {
public:
  EtActionEventWaiter(std::shared_ptr<EtActionEvent> event)
      : event_to_wait_(event) {
    assert(event_to_wait_.get() != nullptr);
  }
  ~EtActionEventWaiter() = default;
  bool readyForExecution() override {
    auto event = dynamic_cast<EtActionEvent *>(event_to_wait_.get());
    assert(event != nullptr);
    return event_to_wait_->isExecuted();
  }
  void execute(et_runtime::Device *device_target) override {}

private:
  std::shared_ptr<EtActionEvent> event_to_wait_;
};

class EtActionConfigure : public EtAction {
  const void *devMemRegionPtr;
  size_t devMemRegionSize;
  const void *kernelsDevMemRegionPtr;
  size_t kernelsDevMemRegionSize;
  bool res_is_local_mode = false;

public:
  EtActionConfigure(const void *devMemRegionPtr, size_t devMemRegionSize,
                    const void *kernelsDevMemRegionPtr,
                    size_t kernelsDevMemRegionSize)
      : devMemRegionPtr(devMemRegionPtr), devMemRegionSize(devMemRegionSize),
        kernelsDevMemRegionPtr(kernelsDevMemRegionPtr),
        kernelsDevMemRegionSize(kernelsDevMemRegionSize) {}
  virtual void execute(et_runtime::Device *device_target);
  bool isLocalMode() { return res_is_local_mode; }
};

class EtActionRead : public EtAction {
  void *dstHostPtr;
  const void *srcDevPtr;
  size_t count;

public:
  EtActionRead(void *dstHostPtr, const void *srcDevPtr, size_t count)
      : dstHostPtr(dstHostPtr), srcDevPtr(srcDevPtr), count(count) {}
  virtual void execute(et_runtime::Device *device_target);
};

class EtActionWrite : public EtAction {
  void *dstDevPtr;
  const void *srcHostPtr;
  size_t count;

public:
  EtActionWrite(void *dstDevPtr, const void *srcHostPtr, size_t count)
      : dstDevPtr(dstDevPtr), srcHostPtr(srcHostPtr), count(count) {}
  virtual void execute(et_runtime::Device *device_target);
};

class EtActionLaunch : public EtAction {
  dim3 gridDim;
  dim3 blockDim;
  std::vector<uint8_t> args_buff;
  uintptr_t kernel_pc;
  std::string kernel_name;

public:
  EtActionLaunch(dim3 gridDim, dim3 blockDim,
                 const std::vector<uint8_t> &args_buff, uintptr_t kernel_pc,
                 const std::string &kernel_name)
      : gridDim(gridDim), blockDim(blockDim), args_buff(args_buff),
        kernel_pc(kernel_pc), kernel_name(kernel_name) {}
  virtual void execute(et_runtime::Device *device_target);
};

} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_COMMANDS_H
