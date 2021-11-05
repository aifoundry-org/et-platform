/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _L1_SCP_CHECKER_H_
#define _L1_SCP_CHECKER_H_

// Global
#include <list>

// Local
#include "emu_defines.h"
#include "cache.h"
#include "agent.h"
#include "processor.h"

// TensorOp type
enum class tensor_op_type
{
  TensorFMA,
  TensorQuant,
  TensorReduce,
  TensorStore
};

class l1_scp_checker : public bemu::Agent
{
  public:
    // Constructor
    l1_scp_checker(bemu::System* chip = nullptr);

    // Copy/Move
    l1_scp_checker(const l1_scp_checker&) = default;
    l1_scp_checker& operator=(const l1_scp_checker&) = default;
    l1_scp_checker(l1_scp_checker&&) = default;
    l1_scp_checker& operator=(l1_scp_checker&&) = default;

    std::string name() const { return "L1-SCP-Checker"; }

    // Accessors
    void l1_scp_fill(uint32_t thread, uint32_t idx, uint32_t id);
    void l1_scp_read(uint32_t thread, uint32_t idx, tensor_op_type type);
    void tensor_op_start(uint32_t thread, tensor_op_type type, bool uses_fp_regs);
    void tensor_wait(uint32_t thread, bemu::Hart::Waiting id);

    // Logging variables
    uint32_t log_minion = 2048; // None by default

  private:
    // L1 Scp status
    enum class l1_scp_status
    {
      Invalid,     // Not initialized, no one reading or writing
      Fill,        // TensorLoad filling it
      Valid,       // Known contents in line, no one reading or writing
      FillUnknown, // More than one tensorload inflight, unknown contents at the end
      Unknown,     // Unknown contents in line, no one reading or writing
      InUse,       // TensorOp is reading the contents
    };

    // TensorWait defines
    enum class tensor_wait_type
    {
      TensorLoad_0 = 0,
      TensorLoad_1 = 1,
      TensorLoadL2_0 = 2,
      TensorLoadL2_1 = 3,
      Prefetch_0 = 4,
      Prefetch_1 = 5,
      CacheOp = 6,
      TensorFMA = 7,
      TensorStore = 8,
      TensorReduce = 9,
      TensorQuant = 10
    };

    // This struct stores information of alive tensor op instructions
    struct tensor_op
    {
      tensor_op_type type;         // Type of tensor operation
      bool           uses_fp_regs; // The operation uses FP regs or not (there's an implicit order barrier when using FP regs)
    };
    
    struct minion_scp_info_t
    {
        l1_scp_status          l1_scp_line_status[L1_SCP_ENTRIES];    // Per scratchpad cache line status
        std::list<tensor_op *> l1_scp_line_consumers[L1_SCP_ENTRIES]; // Per scratchpad cache line list of alive consumers
        uint32_t               l1_scp_line_id[L1_SCP_ENTRIES];        // TensorLoad/TensorWait Id for each scratchpad cache line
        std::list<tensor_op>   alive_tensor_ops;                      // List of alive ops in the minion
    };

  private:
    void drain_tensor_op_for_type(uint32_t thread, tensor_op_type type, bool force_fp_regs);
    void finish_tensor_op(uint32_t thread, tensor_op * op);
    std::string to_string(l1_scp_status status) const;
    std::string to_string(tensor_op_type type) const;
    std::string to_string(bemu::Hart::Waiting type) const;

  private:
    minion_scp_info_t minion_scp_info[EMU_NUM_MINIONS];
};

#endif

