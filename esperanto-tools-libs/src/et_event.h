#ifndef ETTEE_ET_EVENT_H
#define ETTEE_ET_EVENT_H

#include "et_stream.h"
#include "utils.h"
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stddef.h>

/*
 * Event object like Event in CUDA Runtime.
 */
class EtEvent {
  bool disable_timing_;
  bool blocking_sync_;
  EtActionEvent *action_event_ = nullptr;

public:
  EtEvent(bool disable_timing, bool blocking_sync)
      : disable_timing_(disable_timing), blocking_sync_(blocking_sync) {}

  ~EtEvent() { assert(action_event_ == nullptr); }

  EtActionEvent *getAction() { return action_event_; }
  void resetAction(EtActionEvent *action = nullptr);

  bool isDisableTiming() { return disable_timing_; }
  bool isBlockingSync() { return blocking_sync_; }
};

#endif // ETTEE_ET_EVENT_H
