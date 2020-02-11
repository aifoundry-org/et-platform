/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _RBOX_SYSEMU_H_
#define _RBOX_SYSEMU_H_

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
  typedef void (*func_ptr_write_msg_port_data) (uint32_t thread, uint32_t port_id, uint32_t *data, uint8_t oob);

 public:

  rboxSysEmu(unsigned shireId, func_ptr_write_msg_port_data write_msg_port);
  int tick();
  void dataRequest(unsigned thread);
  bool dataQuery(unsigned thread);
  void threadDisabled(unsigned thread) { enabled_[thread] = false; }
  bool done();
 private:

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
