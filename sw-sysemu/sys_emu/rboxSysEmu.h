#ifndef _RBOX_SYSEMU_H_
#define _RBOX_SYSEMU_H_

#include "common/main_memory.h"
#include "emu.h"
#include <queue>
#include <header.h>

#define NUM_SHIRE_MIN 64
#define NUM_THREADS_SHIRE 128

typedef struct
{
  unsigned   minion;   // minion in shire
  unsigned   thread;   // thread in minion
  t_PSdesc4  data;     // PS descriptor
  unsigned   state_index;
} quad_transaction;

class rboxSysEmu
{
  typedef void (*func_ptr_write_msg_port_data) (uint32_t thread, uint32_t port_id, uint32_t *data);

 public:

  rboxSysEmu(unsigned shireId, main_memory *mem, func_ptr_write_msg_port_data write_msg_port);
  int tick();
  void dataRequest(unsigned thread);
  bool dataQuery(unsigned thread);
  void threadDisabled(unsigned thread) { enabled_[thread] = false; }
  bool done();
 private:

  main_memory *mem_;
  const unsigned id_;
  bool started_;
  bool current_thread_[NUM_SHIRE_MIN];
  unsigned state_idx_[NUM_THREADS_SHIRE*2];                                     // State per thread
  bool done_;
  unsigned remaining_quads_;
  std::queue<quad_transaction> queue_;                                  // queue that contains quad fragments created by the rbox
  std::queue<quad_transaction> queuePerThread_[NUM_THREADS_SHIRE];      // queue of every thread in the shire

  bool enabled_[NUM_THREADS_SHIRE];

  func_ptr_write_msg_port_data write_msg_port_;
};

#endif
