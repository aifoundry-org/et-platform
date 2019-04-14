#ifndef ETTEE_ET_STREAM_H
#define ETTEE_ET_STREAM_H

#include <cassert>
#include <queue>

namespace et_runtime {
class EtAction;
}

/*
 * Steam of actions executed asynchronously on device thread.
 */
class EtStream {
  bool is_blocking_;

public:
  EtStream(bool is_blocking) : is_blocking_(is_blocking) { init(); }

  void init();

  ~EtStream() { assert(actions.empty()); }

  bool isBlocking() { return is_blocking_; }

  std::queue<et_runtime::EtAction *> actions;
};

#endif // ETTEE_ET_STREAM_H
