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

  ~EtStream() { assert(actions_.empty()); }

  bool isBlocking() { return is_blocking_; }
  /// @brief Add a command to execute in the command Queue.
  void addCommand(et_runtime::EtAction *action);
  /// @brief Return True iff the command queue is empty
  bool noCommands() const { return actions_.empty(); }
  /// @brief Return pointer to the command in the front of the queue
  et_runtime::EtAction *frontCommand() { return actions_.front(); }
  /// @brief Remove front command
  void popCommand() { actions_.pop(); }

private:
  std::queue<et_runtime::EtAction *> actions_;
};

#endif // ETTEE_ET_STREAM_H
