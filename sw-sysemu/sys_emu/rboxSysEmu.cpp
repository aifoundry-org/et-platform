#include "rboxSysEmu.h"
#include "trace.h"

rboxSysEmu::rboxSysEmu(unsigned shireId, main_memory *mem,
                       func_ptr_write_msg_port_data write_msg_port) :
  mem_(mem),
  id_(shireId)
{
  started_ = false;
  write_msg_port_ = write_msg_port;  

  // Enable all threads in shire
  for ( int i = 0;i < NUM_THREADS_SHIRE;i++)
     enabled_[i] = true;

  bzero(current_thread_, sizeof(current_thread_));
  memset(state_idx_,  0xffffff, sizeof(state_idx_));

  // IMPORTANT: This code assumes that all the quads are to be executed in the same shire.
  t_Trace trace;
  init_t_Trace(&trace);
  trace.loadTraceFromFile(&trace);
    
  quad_transaction qt;
  
  for (unsigned i = 0; i< trace.numQuads; i++)
  {
    qt.minion = trace.quads[i].minion_id % NUM_SHIRE_MIN;   // Minion ID
    memcpy(&qt.data, &trace.quads[i].dPS, 64);              // Copy PS desc
    qt.thread = current_thread_[qt.minion];                 // Thread number 0-1
    qt.state_index = trace.quads[i].state_index;
    queue_.push(qt);
    current_thread_[qt.minion] = !current_thread_[qt.minion];
  }
  
  remaining_quads_ = 0;
}


int rboxSysEmu::tick()
{
  // do not start until minion 0 is ready (when it has credits)
  if ( !started_ && mem_->getRboxCredit( id_ * NUM_SHIRE_MIN) > 0 )
  {
    started_ = true;
  }

  if (!started_ || queue_.empty()) return -1;

  // rbox process start
   
  // we want to send to minion M, thread t... its credit is:
  unsigned threadNr = (queue_.front().minion << 1)  | queue_.front().thread;

  int credit = mem_ -> getRboxCredit ( threadNr + id_ * NUM_SHIRE_MIN);

  if(credit > 0)
  {
    // decrement credit
    mem_ -> decRboxCredit ( threadNr + id_ * NUM_SHIRE_MIN);

    // Add set state packets in queue if change state
    if(state_idx_[threadNr] != queue_.front().state_index)
    {
        quad_transaction change_state_packet;

        change_state_packet.minion = queue_.front().minion;
        change_state_packet.data.mask = 0;
        change_state_packet.data.xUL = queue_.front().state_index;
        change_state_packet.data.yUL = 0;                
        change_state_packet.thread = queue_.front().thread;

        queuePerThread_[threadNr].push(change_state_packet);
        remaining_quads_++;
        state_idx_[threadNr] = queue_.front().state_index;
    }
    else
    {
        // Send quad fragment and remove from queue
        queuePerThread_[threadNr].push(queue_.front());
        remaining_quads_++;
        queue_.pop();
    }

    //This code is executed in both cases
    // and notify if thread was disabled
    if (enabled_[threadNr] == false)
    {
      enabled_[threadNr] = true;
      return threadNr;
    }
    else return -1;
  }
  else
  {
    return -1;
  } 
}



void rboxSysEmu::dataRequest(unsigned thread)
{
    // send to minion msg port
  uint8_t oob = 0;
  write_msg_port_(thread, 0, (uint32_t*)(&(queuePerThread_[thread].front().data)),oob);
  queuePerThread_[thread].pop();
  remaining_quads_--;
}

bool rboxSysEmu::dataQuery(unsigned thread)
{
  return queuePerThread_[thread].size() > 0;
  
}

bool rboxSysEmu::done()
{
    return remaining_quads_ == 0;
}

