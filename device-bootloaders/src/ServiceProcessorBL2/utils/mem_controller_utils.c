/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

#define DEBUG_INFO  1                   // true to enable preliminary debug codes

#include <stdint.h>
#include <unistd.h>
#include "mem_controller.h"
#include "log.h"

#include "hwinc/hal_device.h"
#include "hwinc/ddrc_reg_def.h"
#include "hal_ddr_init.h"

/*
** Private functions/macros used only in this file
*/
#define GET_MEMSHIRE_ESR(memshire, reg) \
        ((((uint64_t)memshire&0x7)|0xe8) << 22 | reg)      // 0xe8=232

static inline uint32_t ms_peek_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value, uint32_t wait_mask)
{
    uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
    return (value ^ wait_value) & wait_mask;
}

static inline volatile void *get_ddrc_address(uint32_t memshire, uint32_t block, uint64_t reg)
{
  if(block == 1)
    reg |= 0x1000;
  return (volatile void *) (R_SHIRE_LPDDR_BASEADDR | ((uint64_t)memshire & 0x7) << 26 | reg);
}

typedef enum
{
  TRAINING_1D,
  TRAINING_2D
}training_stage;

#if DEBUG_INFO
static const char* get_training_status_text(uint8_t train_msg)
{
  switch(train_msg) {
    case 0x07:
      return "training has run successfully ... firmware complete";
    case 0xff:
      return "training has failed ... firmware complete";
    case 0x00:
      return "end of initialization";
    case 0x01:
      return "end of fine write leveling";
    case 0x02:
      return "end of read enable training";
    case 0x03:
      return "end of read delay center optimization";
    case 0x04:
      return "end of write delay center optimization";
    case 0x05:
      return "end of 2D read delay/voltage center optimization";
    case 0x06:
      return "end of 2D write delay /voltage center optimization";
    case 0x09:
      return "end of max read latency training";
    case 0x0a:
      return "end of read dq deskew training";
    case 0x0b:
      return "reserved";
    case 0x0c:
      return "end of all DB training ... MREP/DWL/MRD/MWD complete";
    case 0x0d:
      return "end of CA training";
    case 0xfd:
      return "end of MPR read delay center optimization";
    case 0xfe:
      return "end of write leveling coarse delay";
    default:
      return "unknown";
  }
}

static void log_training_error(training_stage stage, uint32_t memshire, uint8_t train_msg)
{
  if(stage == TRAINING_1D)
    Log_Write(LOG_LEVEL_INFO, "DDR TRAIN (S%d) received DDRC PHY message 0x%02x (%s)", memshire+232, train_msg, get_training_status_text(train_msg));
  else
    Log_Write(LOG_LEVEL_INFO, "DDR TRAIN 2d (S%d) received DDRC PHY message 0x%02x (%s)", memshire+232, train_msg, get_training_status_text(train_msg));
}

#define LOG_TRAINING(stage, memshire, train_msg)    log_training_error(stage, memshire, train_msg)

#else   // DEBUG_INFO

#define LOG_TRAINING(stage, memshire, train_msg)    (stage = stage)     // to suppress compilation warning

#endif  // DEBUG_INFO

static void wait_for_training_internal(training_stage stage, uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  uint8_t train_msg;
  uint8_t number_of_shire_completed;
  uint8_t shire_completed[NUMBER_OF_MEMSHIRE] = { 0 };

  // memshire is not define outside, use it as a local variable
  memshire = 0;
  number_of_shire_completed = 0;
  while(number_of_shire_completed < NUMBER_OF_MEMSHIRE) {

    if(memshire == NUMBER_OF_MEMSHIRE) {
      // usleep(train_poll_iteration_delay);      // delay between each cycle
      memshire = 0;
    }

    if(shire_completed[memshire] || ms_peek_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x0, 0x1) != 0) {
      ++memshire;
      continue;
    }

    train_msg = (uint8_t)(ms_read_ddrc_reg(memshire, 2, APBONLY0_UctWriteOnlyShadow) & 0xff);     // read message

    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000000);                            // ack message
    ms_poll_ddrc_reg(memshire, 2, APBONLY0_UctShadowRegs, 0x00000001, 0x1, 100, 1);               // wait for handshake
    ms_write_ddrc_reg(memshire, 2, APBONLY0_DctWriteProt, 0x00000001);                            // ack handshake

    LOG_TRAINING(stage, memshire, train_msg);
    if(train_msg == 0x7 || train_msg == 0xff || train_msg == 0x0b) {
      if(train_msg == 0xff || train_msg == 0x0b) {
        // report error if needed
      }
      shire_completed[memshire] = 1;
      ++number_of_shire_completed;
    }

    ++memshire;

  }

  (void)train_poll_max_iterations;    // suppress compilation warning
  (void)train_poll_iteration_delay;   // suppress compilation warning
  return;
}

/*
** Low-level support functions
*/
uint64_t ms_read_esr(uint32_t memshire, uint64_t reg)
{
  return *(volatile uint64_t*)GET_MEMSHIRE_ESR(memshire, reg);
}

void ms_write_esr(uint32_t memshire, uint64_t reg, uint64_t value)
{
  *(volatile uint64_t*)GET_MEMSHIRE_ESR(memshire, reg) = value;
}

uint32_t ms_read_reg(uint32_t memshire, uint64_t reg)
{
  return *(volatile uint32_t*)GET_MEMSHIRE_ESR(memshire, reg);
}

void ms_write_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*)GET_MEMSHIRE_ESR(memshire, reg) = value;
}

uint32_t ms_read_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg)
{
  return *(volatile uint32_t*) get_ddrc_address(memshire, blk, reg);
}

void ms_write_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, blk, reg) = value;
}

void ms_write_both_ddrc_reg(uint32_t memshire, uint64_t reg, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, 0, reg) = value;
  *(volatile uint32_t*) get_ddrc_address(memshire, 1, reg) = value;
}

void ms_write_ddrc_addr(uint32_t memshire, uint64_t addr_in, uint32_t value)
{
  *(volatile uint32_t*) get_ddrc_address(memshire, 0, addr_in) = value;
}

uint32_t ms_poll_pll_reg(uint32_t memshire, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries)
{
  while(1) {
    uint32_t value = ms_read_reg(memshire, reg);
    if( ((value ^ wait_value) & wait_mask) == 0)
      break;

    timeout_tries--;
    if(timeout_tries == 0) {
      break;
    }
    // usleep(1);
  }

  return timeout_tries;
}

uint32_t ms_poll_ddrc_reg(uint32_t memshire, uint32_t blk, uint64_t reg, uint32_t wait_value, uint32_t wait_mask, uint32_t timeout_tries, uint32_t wait_count)
{
  while(1) {
    uint32_t value = ms_read_ddrc_reg(memshire, blk, reg);
    if( ((value ^ wait_value) & wait_mask) == 0)
      break;

    timeout_tries--;
    if(timeout_tries == 0) {
      break;
    }
    // usleep(wait_count);
    (void)wait_count;
  }

  return timeout_tries;
}

/*
** Training related functions
** - Reading training info from external storage, either file or flash.
** - Writing into DDR memory controllers
*/
void ms_ddr_phy_1d_train_from_file(uint32_t mem_config, uint32_t memshire)
{
  // TODO Now nothing but supress compile warnings
  (void)mem_config;
  (void)memshire;
}

void ms_wait_for_training(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  wait_for_training_internal(TRAINING_1D, memshire, train_poll_max_iterations, train_poll_iteration_delay);
}

void ms_ddr_phy_2d_train_from_file(uint32_t mem_config, uint32_t memshire)
{
  // TODO Now nothing but supress compile warnings
  (void)mem_config;
  (void)memshire;
}

void ms_wait_for_training_2d(uint32_t memshire, uint32_t train_poll_max_iterations, uint32_t train_poll_iteration_delay)
{
  wait_for_training_internal(TRAINING_2D, memshire, train_poll_max_iterations, train_poll_iteration_delay);
}
