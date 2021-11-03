/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

// Local defines
#include "l1_scp_checker.h"
#include "emu_gio.h"

// Logging macros
#define L1_SCP_CHECKER_LOG(minion, cmd) \
  { if((minion == 0xFFFFFFFF) || (log_minion == 0xFFFFFFFF) || (minion == log_minion)) \
    { \
      cmd; \
    } \
  }

/*! \brief Scp directory constructor
 *
 *  This function creates a new object of the type L1 Scp checker
 */
l1_scp_checker::l1_scp_checker(bemu::System* chip) : bemu::Agent(chip)
{
  // Marks all L1 entries as invalid
  for(uint32_t minion = 0; minion < EMU_NUM_MINIONS; minion++)
  {
    for(uint32_t entry = 0 ; entry < L1_SCP_ENTRIES; entry++)
    {
      minion_scp_info[minion].l1_scp_line_status[entry] = l1_scp_status::Invalid;
      minion_scp_info[minion].l1_scp_line_id[entry] = -1;
    }
  }
}

/*! \brief Fills an L1 scp entry
 *
 *  Sets an entry of the L1 scp of a specific minion as filled and sets
 *  also the id related to the fill
 */
void l1_scp_checker::l1_scp_fill(uint32_t thread, uint32_t idx, uint32_t id)
{
    // Ignore TENB entries
    if(idx >= L1_SCP_ENTRIES)
    {
        return;
    }

    uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
    uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
    uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;

    auto &status = minion_scp_info[minion_id].l1_scp_line_status[idx];
    if(status == l1_scp_status::InUse)
    {
        LOG_AGENT(FTL, *this, "l1_scp_checker::l1_scp_fill => line state is InUse!! shire: %i, minion %i, line: %i", shire, minion, idx);
    }

    // If no one was writing, move to fill
    if((status == l1_scp_status::Invalid) || (status == l1_scp_status::Valid) || (status == l1_scp_status::Unknown))
        status = l1_scp_status::Fill;
    // Someone was writing already, move to fill with unknown results
    else
        status = l1_scp_status::FillUnknown;
    minion_scp_info[minion_id].l1_scp_line_id[idx]     = id;
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::l1_scp_fill => l1 scp entry set to %s for shire: %i, minion: %i, line: %i, id: %i", to_string(status).c_str(), shire, minion, idx, id));
}

/*! \brief Read data from L1 scp
 *
 *  Data is read from L1 scp. If data is not valid an error is raised
 */
void l1_scp_checker::l1_scp_read(uint32_t thread, uint32_t idx, tensor_op_type type)
{
    uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
    uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
    uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;

    auto &status = minion_scp_info[minion_id].l1_scp_line_status[idx];
    if(  (status != l1_scp_status::Valid)
      && (status != l1_scp_status::InUse))
    {
        LOG_AGENT(FTL, *this, "l1_scp_checker::l1_scp_read => line state is not Valid/InUse!! It is %s for shire: %i, minion %i, line: %i", to_string(minion_scp_info[minion_id].l1_scp_line_status[idx]).c_str(), shire, minion, idx);
    }
    
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::l1_scp_read => l1 scp entry set to InUse for shire: %i, minion: %i, line: %i", shire, minion, idx));
    
    // Looks for the first alive tensor of the same type starting by the end
    auto end = minion_scp_info[minion_id].alive_tensor_ops.end();
    auto it = end;
    it--;
    while(it != end)
    {
        if(it->type == type)
        {
            break;
        }
      it--;
    }
    if(it == end)
    {
        LOG_AGENT(FTL, *this, "l1_scp_checker::l1_scp_read => couldn't find any outstanding op of type %s for shire: %i, minion %i, line: %i", to_string(type).c_str(), shire, minion, idx);
    }
    tensor_op &op = *it;
    // Flags that line as being in use
    minion_scp_info[minion_id].l1_scp_line_status[idx] = l1_scp_status::InUse;
    minion_scp_info[minion_id].l1_scp_line_consumers[idx].push_back(&op);
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::l1_scp_read => adding consumer shire: %i, minion %i, line: %i, consumer: %p", shire, minion, idx, (void *) &op));
}

/*! \brief A new tensor operation starts
 *
 *  Flags a new tensor operation is starting.
 */
void l1_scp_checker::tensor_op_start(uint32_t thread, tensor_op_type type, bool uses_fp_regs)
{
    uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
    uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
    uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;

    // Before pushing the new tensor to the structs, drain all the tensor ops that must have
    // finished in the minion before this instruction can actually start
    drain_tensor_op_for_type(thread, type, false);

    // Creates the tensor op
    tensor_op op;
    op.type         = type;
    op.uses_fp_regs = uses_fp_regs;

    // Adds it to the list of alive ops
    minion_scp_info[minion_id].alive_tensor_ops.push_back(op);

    tensor_op &op_ref = minion_scp_info[minion_id].alive_tensor_ops.back();
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::tensor_op_start => shire: %i, minion: %i, op: %s, fp regs: %i, ptr: %p", shire, minion, to_string(type).c_str(), uses_fp_regs, (void *) &op_ref));

    //for (auto & op : minion_scp_info[minion_id].alive_tensor_ops) {
    //    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "  l1_scp_checker::tensor_op_start dump alive_tensor_ops queue => shire: %i, minion: %i, op: %s, fp regs: %i, ptr: %p", shire, minion, to_string(op.type).c_str(), op.uses_fp_regs, &op));
    //}
}

/*! \brief Tensor Wait operation. Used for fills to L1 scp to finish and wait for
 * tensor ops.
 *
 *  Waits for L1 scp fills of a given minion to finish. Only the fills associated
 *  to the request ID will be marked as valid
 */
void l1_scp_checker::tensor_wait(uint32_t thread, bemu::Hart::Waiting id)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::tensor_wait => shire: %i, minion: %i, id: %i (%s)", shire, minion, (int) id, to_string(id).c_str()));

  // For TensorLoad waits
  if((id == bemu::Hart::Waiting::tload_0) || (id == bemu::Hart::Waiting::tload_1))
  {
    uint32_t internal_id = (id == bemu::Hart::Waiting::tload_0) ? 0 : 1;
    // For all the entries of the minion
    for(uint32_t entry = 0 ; entry < L1_SCP_ENTRIES; entry++)
    {
      // Entry has an outstanding fill and same id
      if((minion_scp_info[minion_id].l1_scp_line_status[entry] == l1_scp_status::Fill) && (minion_scp_info[minion_id].l1_scp_line_id[entry] == internal_id))
      {
        minion_scp_info[minion_id].l1_scp_line_status[entry] = l1_scp_status::Valid;
        minion_scp_info[minion_id].l1_scp_line_id[entry]     = -1;
        L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::tensor_wait => l1 scp entry set to Valid for for shire: %i, minion: %i, line: %i, id: %i", shire, minion, entry, internal_id));
      }
      // Entry has an outstanding fill unkown and same id
      if((minion_scp_info[minion_id].l1_scp_line_status[entry] == l1_scp_status::FillUnknown) && (minion_scp_info[minion_id].l1_scp_line_id[entry] == internal_id))
      {
        minion_scp_info[minion_id].l1_scp_line_status[entry] = l1_scp_status::Unknown;
        minion_scp_info[minion_id].l1_scp_line_id[entry]     = -1;
        L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::tensor_wait => l1 scp entry set to Unknown for for shire: %i, minion: %i, line: %i, id: %i", shire, minion, entry, internal_id));
      }
    }
  }
  // Tensor*
  else if ((id == bemu::Hart::Waiting::tfma)
        || (id == bemu::Hart::Waiting::tstore)
        || (id == bemu::Hart::Waiting::reduce)
        || (id == bemu::Hart::Waiting::tquant))
  {
    // Tensor instructions have implicit dependencies. There's a dependency either through
    // instruction type or through FP regs
    tensor_op_type type;
    if      (id == bemu::Hart::Waiting::tfma)   { type = tensor_op_type::TensorFMA; }
    else if (id == bemu::Hart::Waiting::tstore) { type = tensor_op_type::TensorStore; }
    else if (id == bemu::Hart::Waiting::reduce) { type = tensor_op_type::TensorReduce; }
    else                                        { type = tensor_op_type::TensorQuant; }

    drain_tensor_op_for_type(thread, type, false);
  }
}

// This function drains from the alive tensor ops all the ops that must be done
// before a specific op type gets executed after them.
void l1_scp_checker::drain_tensor_op_for_type(uint32_t thread, tensor_op_type type, bool force_fp_regs)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;

  bool fp_regs = force_fp_regs; // By default do not drain FP ops
  auto end = minion_scp_info[minion_id].alive_tensor_ops.end();
  auto it  = end;
  it--;

  // Traverses the list of alive ops in revers
  while(it != end)
  {
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::drain_tensor_op_for_type => checking op %s with fp regs %i", to_string(it->type).c_str(), it->uses_fp_regs));
    L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::drain_tensor_op_for_type => current type %s, fp regs %i", to_string(type).c_str(), fp_regs));
    // Checks if the instruction is affected either by type or FP
    if ((type == it->type) || (fp_regs && it->uses_fp_regs))
    {
      L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "%s", "l1_scp_checker::drain_tensor_op_for_type => op is finished"));

      // Set the FP regs dependency if this one uses it
      fp_regs |= it->uses_fp_regs;
      // If the instruction is TensorFMA and uses FP regs, the TensorFMA without FP regs need to start being affected
      if ((it->type == tensor_op_type::TensorFMA) && it->uses_fp_regs)
      {
        // Switching the type to tensorFMA guarantees that they are affected, the other
        // type of tensor ops will be affected through FP regs
        type = tensor_op_type::TensorFMA;
      }

      // Remove the tensor op
      finish_tensor_op(thread, &(*it));

      it = minion_scp_info[minion_id].alive_tensor_ops.erase(it);
      it--;

      //for (auto & op : minion_scp_info[minion_id].alive_tensor_ops) {
      //  L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "  l1_scp_checker::drain_tensor_op_for_type dump alive_tensor_ops queue => shire: %i, minion: %i, op: %s, fp regs: %i, ptr: %p", shire, minion, to_string(op.type).c_str(), op.uses_fp_regs, &op));
      //}
    }
    else
    {
      it--;
    }
  }
}

void l1_scp_checker::finish_tensor_op(uint32_t thread, tensor_op * op)
{
  uint32_t minion_id = thread / EMU_THREADS_PER_MINION;
  uint32_t minion    = minion_id % EMU_MINIONS_PER_SHIRE;
  uint32_t shire     = thread / EMU_THREADS_PER_SHIRE;
  L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::finish_tensor_op => shire: %i, minion: %i, op: %s",
        shire, minion, to_string(op->type).c_str()));

  // Runs through all the lines and removes the ops
  for (size_t idx = 0; idx < L1_SCP_ENTRIES; idx++)
  {
    // Runs through all the consumers of the line and remove the current op
    bool mod = false;
    auto consumer = minion_scp_info[minion_id].l1_scp_line_consumers[idx].begin();
    while (consumer != minion_scp_info[minion_id].l1_scp_line_consumers[idx].end())
    {
      if (op == *consumer)
      {
        L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::finish_tensor_op => removing consumer shire: %i, minion: %i, line: %i, consumer: %p",
              shire, minion, (int) idx, (void *) *consumer));
        consumer = minion_scp_info[minion_id].l1_scp_line_consumers[idx].erase(consumer);
        mod = true;
      }
      else
      {
        consumer++;
      }
    }

    // If there are no consumers, mark the line as valid
    if (mod && (minion_scp_info[minion_id].l1_scp_line_consumers[idx].size() == 0))
    {
      L1_SCP_CHECKER_LOG(minion_id, LOG_AGENT(DEBUG, *this, "l1_scp_checker::finish_tensor_op => l1 scp entry set to Valid for shire: %i, minion: %i, line: %i", shire, minion, (int) idx));
      minion_scp_info[minion_id].l1_scp_line_status[idx] = l1_scp_status::Valid;
    }
  }
}

std::string l1_scp_checker::to_string(l1_scp_status status) const
{
  std::string ret = "";
  switch (status)
  {
    case l1_scp_status::Invalid:
      ret = "Invalid";
      break;
    case l1_scp_status::Fill:
      ret = "Fill";
      break;
    case l1_scp_status::Valid:
      ret = "Valid";
      break;
    case l1_scp_status::FillUnknown:
      ret = "FillUnknown";
      break;
    case l1_scp_status::Unknown:
      ret = "Unknown";
      break;
    case l1_scp_status::InUse:
      ret = "InUse";
      break;
  }

  return ret;
}

std::string l1_scp_checker::to_string(tensor_op_type type) const {
  std::string ret = "";
  switch (type)
  {
    case tensor_op_type::TensorFMA:
      ret = "TensorFMA";
      break;
    case tensor_op_type::TensorQuant:
      ret = "TensorQuant";
      break;
    case tensor_op_type::TensorReduce:
      ret = "TensorReduce";
      break;
    case tensor_op_type::TensorStore:
      ret = "TensorStore";
      break;
  }

  return ret;
}

std::string l1_scp_checker::to_string(bemu::Hart::Waiting type) const {
  std::string ret = "";
  switch (type)
  {
    case bemu::Hart::Waiting::tload_0:
      ret = "TensorLoad_0";
      break;
    case bemu::Hart::Waiting::tload_1:
      ret = "TensorLoad_1";
      break;
    case bemu::Hart::Waiting::tload_L2_0:
      ret = "TensorLoadL2_0";
      break;
    case bemu::Hart::Waiting::tload_L2_1:
      ret = "TensorLoadL2_1";
      break;
    case bemu::Hart::Waiting::prefetch_0:
      ret = "Prefetch_0";
      break;
    case bemu::Hart::Waiting::prefetch_1:
      ret = "Prefetch_1";
      break;
    case bemu::Hart::Waiting::cacheop:
      ret = "CacheOp";
      break;
    case bemu::Hart::Waiting::tfma:
      ret = "TensorFMA";
      break;
    case bemu::Hart::Waiting::tstore:
      ret = "TensorStore";
      break;
    case bemu::Hart::Waiting::reduce:
      ret = "TensorReduce";
      break;
    case bemu::Hart::Waiting::tquant:
      ret = "TensorQuant";
      break;
    default:
      assert(false && "Shouldn't get here");
      break;
  }

  return ret;
}

