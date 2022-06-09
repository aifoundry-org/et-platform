/*
 * Copyright (c) 2018-2020, University of Southampton.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include <cstdint>

#include <utils.h>
#include <device-layer/IDeviceLayer.h>
#include <deviceManagement/DeviceManagement.h>
#include <DebugDataConfigParser.hpp>

using namespace dev;
using namespace device_mgmt_api;
using namespace device_management;

#define RISCV64_REG_SIZE        8 //bytes
#define RISCV64_NUM_GPRS        32 //registers
#define MAX_RSP_PKT_SIZE        512 //bytes
#define RISCV_PC_INDEX          32

#define HARTS_PER_NEIGH     16
#define NUM_NEIGH_PER_SHIRE 4
#define NUM_HARTS_PER_SHIRE (NUM_NEIGH_PER_SHIRE*HARTS_PER_NEIGH)

#define MDI_MEM_READ_LENGTH_BYTES_8     8
#define MDI_MEM_READ_LENGTH_BYTES_4     4
#define MDI_MEM_READ_LENGTH_BYTES_2     2
#define MDI_MEM_READ_LENGTH_BYTES_1     1

#define MDI_MEM_WRITE_LENGTH_BYTES_8     8
#define MDI_MEM_WRITE_LENGTH_BYTES_4     4
#define MDI_MEM_WRITE_LENGTH_BYTES_2     2
#define MDI_MEM_WRITE_LENGTH_BYTES_1     1

/* Target state */
enum target_state {
  TGT_RUNNING,
  TGT_HALTED,
  TGT_EXCEPTION,
  TGT_ERROR
};

/**
 * @brief MinionDebugInterface Interface to control and interact with
 * simulation from an independent outside program, e.g. from a debug server.
 */
class MinionDebugInterface {
 public:
  /* ------ Public methods ------ */

  MinionDebugInterface(uint8_t device_idx, uint64_t shire_mask, uint64_t thread_mask, uint64_t bp_timeout);

  ~MinionDebugInterface();

  // Control and report
  /**
   * @brief Kill simulation.
   */
  void kill();

  /**
   * @brief Reset simulation.
   */
  void reset();

  /**
   * @brief stall execution.
   */
  void stall();

  /**
   * @brief unstall execution.
   */
  void unstall();

  /**
   * @brief check if execution is stalled.
   * @retval true if execution is stalled, false otherwise.
   */
  bool isStalled();

  /**
   * @brief waitForCommand Wait for a new command. Target may call this after
   * single-stepping, hitting a breakpoint etc.
   */
  // virtual void waitForCommand();

  /**
   * @brief waitForTargetStalled Wait for the target to stall. Controller (e.g.
   * gdb server) may call this after issuing a single-step command, or to wait
   * for the target to hit a breakpoint, etc.
   */
  // virtual void waitForTargetStalled();

  /**
   * @brief step execute a single instruction, then stall
   */
  void step();

  // Breakpoints
  /**
   * @brief insert a breakpoint, causing stall when PC=addr
   * @param addr address to stall at
   */
  void insertBreakpoint(uint64_t addr);

  /**
   * @brief Remove breakpoint at addr
   * @param addr address of breakpoint to remove.
   */
  void removeBreakpoint(uint64_t addr);

  // ------ Register access ------
  /**
   * @brief readReg Read the contents of a general purpose register.
   * @param num register number.
   * @retval Content of register <num>.
   */
  uint64_t readReg(std::uint32_t num);

   /**
   * @brief readCSRReg Read the contents of a CSR
   * @param num register number.
   * @retval Content of register <num>.
   */
  uint64_t readCSRReg(std::uint32_t num);

  /**
   * @brief writeReg Set the contents of a general purpose register.
   * @param num register number
   * @param value value to write
   */
  void writeReg(std::uint32_t num, uint64_t value);

  // ------ Memory access ------
  /**
   * @brief readMem read memory from target.
   * @param out output buffer
   * @param addr start address to read from
   * @param len number of bytes to read.
   */
  int readMem(uint8_t *out, uint64_t addr, std::uint32_t len);

  /**
   * @brief writeMem write memory to target.
   * @param src source buffer
   * @param addr address to write to
   * @param len number of bytes to write
   * @retval bool true if success, 0 otherwise.
   */
  int writeMem(uint8_t *src, uint64_t addr, std::uint32_t len);

  // ------ Target info ------
  /**
   * @brief pc_regnum get the register number of the program counter, i.e. which
   * register is the PC.
   * @retval register number of the program counter.
   */
  uint32_t pcRegNum();

  /**
   * @brief n_regs get the number of general purpose registers.
   * @retval how many general purpose registers the target has (including PC,
   * SP, LR etc.)
   */
  uint32_t nRegs();

  /**
   * @brief wordSize
   * @retval word size of target platform (in bytes)
   */
  uint32_t wordSize();

  // ------ Control debugger ------

  /**
   * @brief stopServer Stop control server thread
   */
  void stopServer();

  /**
   * @brief shouldStopServer Check if controller's server thread should stop.
   * @retval true if control thread should stop, false otherwise.
   */
  bool shouldStopServer();

  /**
   * @brief isServerRunning Check if debug server is running.
   */
  bool isServerRunning();

  /**
   * @brief setServerRunning set running status of debug server.
   * @param status Should be true if running, false otherwise.
   */
  void setServerRunning(bool status);

  // ------ Target utilities ------
  /**
   * @brief htotl Convert value from host to target endianness
   * @param hostVal The value in host endianness
   * @return The value in target endianness
   */
  uint64_t htotl(uint64_t hostVal);

  /**
   * @brief ttohl Convert value from target to host endianness
   * @param targetVal The value in target endianness
   * @return The value in host endianness
   */
  uint64_t ttohl(uint64_t targetVal);

  /**
   * @brief selectHart Select a Hart using shire_id and thread_mask
   * @param shire_id
   * @param thread_mask
   * @return Return status of select hart.
   */
  uint64_t selectHart(uint64_t shire_id, uint64_t thread_mask);

   /**
   * @brief unselectHart UnSelect a Hart using shire_id and thread_mask
   * @param shire_id
   * @param thread_mask
   * @return Return status of unselect hart.
   */
  uint64_t unselectHart(uint64_t shire_id, uint64_t thread_mask);

    /**
   * @brief haltHart Halt a Hart
   * @return None
   */
  void haltHart();

    /**
   * @brief resumeHart Resume a Hart
   * @return None
   */
  void resumeHart();

  /**
   * @brief hartStatus Get Hart Status
   * @return None
   */
   uint32_t hartStatus();

  /**
   * @brief getShireID Accessor for Shire ID
   * @return Shire ID
   */
   uint64_t getShireID();

     /**
   * @brief getThreadMask Accessor for Thread Mask
   * @return Thread Mask
   */
   uint64_t getThreadMask();

  /**
   * @brief closeMDI Deletes the MDI Object
   * @return None
   */
  void closeMDI();


  /**
   * @brief uint64_bytes_swap Swap the bytes in the input
   * @return None
   */
  uint64_t uint64BytesSwap(uint64_t val);

  uint8_t     device_idx; 
  uint64_t    bp_timeout;
  bool        tgt_state;
  void*       handle_;
  std::unique_ptr<IDeviceLayer>   devLayer_;
  getDM_t     dmi_;
  uint64_t    shire_id;
  uint64_t    thread_mask;

  private:

  /**
   * @brief Verify if valid DMLib is loaded and available for access
   */
  int verifyDMLib();

  /**
   * @brief Verify if valid DMLib is loaded and available for access
   */
  int invokeDmServiceRequest(uint8_t code, const char* input_buff, const uint32_t input_size,
    char* output_buff, const uint32_t output_size);

  uint32_t    current_hart_;
  bool        server_state;
  bool        should_stop_server;
};
