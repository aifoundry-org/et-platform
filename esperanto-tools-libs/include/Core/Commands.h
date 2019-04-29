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

class CardProxy;

namespace et_runtime {
class EtAction {
  std::atomic<int> ref_counter;

public:
  EtAction() : ref_counter(1) {}
  virtual ~EtAction() {
    assert(ref_counter.load(std::memory_order_relaxed) == 0);
  }
  virtual void execute(CardProxy *card_proxy) {
    THROW("Unexpected EtAction::execute()");
  }
  virtual bool readyForExecution() { return true; }
  void incRefCounter() { ref_counter.fetch_add(1); }
  static void decRefCounter(EtAction *act) {
    if (act->ref_counter.fetch_sub(1) == 1) {
      delete act;
    }
  }
};

class EtActionEvent : public EtAction {
  bool executed = false;
  std::mutex observer_mutex;
  std::condition_variable observer_cond_var;

public:
  virtual void execute(CardProxy *card_proxy);
  void observerWait();
  bool isExecuted() {
    std::lock_guard<std::mutex> lk(observer_mutex);
    return executed;
  }
};

class EtActionEventWaiter : public EtAction {
  EtActionEvent *event_to_wait_;

public:
  EtActionEventWaiter(EtActionEvent *event) : event_to_wait_(event) {
    assert(event_to_wait_ != nullptr);
    event_to_wait_->incRefCounter();
  }
  ~EtActionEventWaiter() { EtAction::decRefCounter(event_to_wait_); }
  virtual bool readyForExecution() { return event_to_wait_->isExecuted(); }
  virtual void execute(CardProxy *card_proxy) {}
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
  virtual void execute(CardProxy *card_proxy);
  bool isLocalMode() { return res_is_local_mode; }
};

class EtActionRead : public EtAction {
  void *dstHostPtr;
  const void *srcDevPtr;
  size_t count;

public:
  EtActionRead(void *dstHostPtr, const void *srcDevPtr, size_t count)
      : dstHostPtr(dstHostPtr), srcDevPtr(srcDevPtr), count(count) {}
  virtual void execute(CardProxy *card_proxy);
};

class EtActionWrite : public EtAction {
  void *dstDevPtr;
  const void *srcHostPtr;
  size_t count;

public:
  EtActionWrite(void *dstDevPtr, const void *srcHostPtr, size_t count)
      : dstDevPtr(dstDevPtr), srcHostPtr(srcHostPtr), count(count) {}
  virtual void execute(CardProxy *card_proxy);
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
  virtual void execute(CardProxy *card_proxy);
};

} // namespace et_runtime

#endif // ET_RUNTIME_MEMORY_COMMANDS_H
