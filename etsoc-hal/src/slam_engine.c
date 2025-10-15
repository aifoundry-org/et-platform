/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 77867a77416cb9dd48aeec75dd61e97b919cad1e in slam_engine repository
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "slam_engine.h"

// #define DEBUG

#ifdef DEBUG

#  define DPRINTF(...)    printf(__VA_ARGS__)

#  define DEBUG_CSR_DETAIL(slam_mode, skip_n_register, last_value, offset, mask, value)                  \
     DPRINTF("Mode=%d, Skip=%d, Buffer=0x%016lx, Offset=0x%08x, Mask=0x%08x, Value=0x%08x\n",            \
       slam_mode, skip_n_register, last_value, offset, mask, value)
#  define DEBUG_CSR_READ_INFO(address, value)       DPRINTF("  CSR Read  [%p] = 0x%08x\n", (volatile void*)address, (uint32_t)value)
#  define DEBUG_CSR_WRITE_INFO(address, value)      DPRINTF("  CSR Write [%p] = 0x%08x\n", (volatile void*)address, (uint32_t)value)

#  define DEBUG_ESR_DETAIL(slam_mode, skip_n_register, last_value, offset, mask, value)                  \
     DPRINTF("Mode=%d, Skip=%d, Buffer=0x%016lx, Offset=0x%016lx, Mask=0x%016lx, Value=0x%016lx\n",      \
       slam_mode, skip_n_register, last_value, offset, mask, value)
#  define DEBUG_ESR_READ_INFO(address, value)       DPRINTF("  ESR Read  [%p] = 0x%016lx\n", (volatile void*)address, (uint64_t)value)
#  define DEBUG_ESR_WRITE_INFO(address, value)      DPRINTF("  ESR Write [%p] = 0x%016lx\n", (volatile void*)address, (uint64_t)value)

#  define DEBUG_BUFFER_INFO(value)                  DPRINTF("  Buffer changed to 0x%016lx\n", value);

#else

#  define DPRINTF(...)
#  define DEBUG_CSR_DETAIL(slam_mode, skip_n_register, last_value, offset, mask, value)
#  define DEBUG_CSR_READ_INFO(address, value)
#  define DEBUG_CSR_WRITE_INFO(address, value)
#  define DEBUG_ESR_DETAIL(slam_mode, skip_n_register, last_value, offset, mask, value)
#  define DEBUG_ESR_READ_INFO(address, value)
#  define DEBUG_ESR_WRITE_INFO(address, value)
#  define DEBUG_BUFFER_INFO(value)

#endif



/*
** helper functions for slam engine
*/

// dummy function returning void
static void __dummy_void(uint32_t usec)
{
  DPRINTF("Default helper functions, weak linked\n");
  (void) usec;
}

// default us_delay().  Could be overwritten.
void us_delay(uint32_t usec) __attribute__ ((weak, alias("__dummy_void")));

/*
** real stuff for slam engine
*/

#define CSR_SLAM_OPS_MASK                           (~(uint32_t)NUMBER_OF_SLAM_TABLE_OPS)
#define IS_CSR_SLAM_OPS(x)                          ( (x & CSR_SLAM_OPS_MASK) == CSR_SLAM_OPS_MASK)

#define ESR_SLAM_OPS_MASK                           (~(uint64_t)NUMBER_OF_SLAM_TABLE_OPS)
#define IS_ESR_SLAM_OPS(x)                          ( (x & ESR_SLAM_OPS_MASK) == ESR_SLAM_OPS_MASK)

typedef enum {
  SLAM_MODE_READ = SWITCH_MODE_TO_READ,
  SLAM_MODE_WRITE = SWITCH_MODE_TO_WRITE,
  SLAM_MODE_READ_MODIFY_WRITE = SWITCH_MODE_TO_READ_MODIFY_WRITE,
} SLAM_MODE;
#define SLAM_MODE_DEFAULT SLAM_MODE_READ_MODIFY_WRITE

typedef struct {
  uint64_t             last_value;
  bool                 store_into_next_register;
  uint32_t             skip_n_register;
  SLAM_MODE            slam_mode;
} CONTROL_PARAMS;

static void process_ops(uint32_t control, uint64_t op1, uint64_t op2, CONTROL_PARAMS *params)
{
  switch(control) {
    case (uint32_t)STO_SWITCH_MODE:
      if(op1 == SWITCH_MODE_TO_READ)
        params->slam_mode = SLAM_MODE_READ;
      else if(op1 == SWITCH_MODE_TO_WRITE)
        params->slam_mode = SLAM_MODE_WRITE;
      else if(op1 == SWITCH_MODE_TO_READ_MODIFY_WRITE)
        params->slam_mode = SLAM_MODE_READ_MODIFY_WRITE;
      else
        assert(0);
      break;
    case (uint32_t)STO_NOP:
      break;
    case (uint32_t)STO_US_DELAY:
      us_delay((uint32_t)op1);
      break;
    case (uint32_t)STO_MODIFY_BUFFER_STORE_NEXT:
      params->last_value = (params->last_value & (~op1)) | op2;
      DEBUG_BUFFER_INFO(params->last_value);
      params->store_into_next_register = true;
      if(params->slam_mode == SLAM_MODE_READ)
        params->slam_mode = SLAM_MODE_READ_MODIFY_WRITE;        // switch back to read/modify_write
      break;
    case (uint32_t)STO_IF_BUFFER_EQUAL_EXEC_NEXT:
      params->store_into_next_register = false;
      if(params->last_value != op1)
        params->skip_n_register = 1;
      break;
    case (uint32_t)STO_IF_BUFFER_RANGE_EXEC_NEXT:
      params->store_into_next_register = false;
      if(params->last_value < op1 || params->last_value > op2)
        params->skip_n_register = 1;
      break;
    case (uint32_t)STO_IF_BUFFER_EQUAL_SKIP_N:
      params->store_into_next_register = false;
      if(params->last_value == op1)
        params->skip_n_register = (uint32_t)op2;
      break;
    default:
      assert(0);
      break;
  }
}

void csr_slam_engine_with_base_address(CSR_SLAM_TABLE *table, uint64_t base_address)
{
  CSR_SLAM_TABLE_ENTRY *entry;
  volatile uint32_t    *register_address;
  CONTROL_PARAMS       params;

  assert(table != NULL);
  assert(table->id == SLAM_TABLE_ID_CSR_SLAM_TABLE);
  assert(table->version >= SLAM_TABLE_VERSION);

  DPRINTF("id = 0x%08x\n", table->id);
  DPRINTF("version = 0x%08x\n", table->version);
  DPRINTF("attribute = 0x%08x\n", table->attribute);
  DPRINTF("reserved = 0x%08x\n", table->reserved);
  DPRINTF("base_address = 0x%016lx\n", table->base_address);

  params.last_value = 0;
  params.store_into_next_register = false;
  params.skip_n_register = 0;
  params.slam_mode = SLAM_MODE_DEFAULT;

  if(base_address == 0) {
    base_address = table->base_address;
  }
  else {
    DPRINTF("Overwriting base_address in table with 0x%016lx\n", base_address);
  }

  entry = table->entries;
  while(entry->register_offset != (uint32_t)STO_SLAM_TABLE_END) {
    DEBUG_CSR_DETAIL(params.slam_mode, params.skip_n_register, params.last_value, entry->register_offset, entry->mask, entry->value);
    if(params.skip_n_register > 0) {
      --params.skip_n_register;
      ++entry;
      continue;
    }
    if(IS_CSR_SLAM_OPS(entry->register_offset)) {
      process_ops(entry->register_offset, entry->mask, entry->value, &params);
    }
    else {
      register_address = (volatile uint32_t *) (base_address + entry->register_offset);
      if(!params.store_into_next_register) {
        if(params.slam_mode == SLAM_MODE_READ || params.slam_mode == SLAM_MODE_READ_MODIFY_WRITE) {
          params.last_value = *register_address;
          DEBUG_CSR_READ_INFO(register_address, params.last_value);
          params.last_value = (params.last_value & (~entry->mask)) | entry->value;
        }
        else if (params.slam_mode == SLAM_MODE_WRITE) {
          params.last_value = entry->value & ~entry->mask;
        }
      }
      params.store_into_next_register = false;
      if(params.slam_mode == SLAM_MODE_WRITE || params.slam_mode == SLAM_MODE_READ_MODIFY_WRITE) {
        *register_address = (uint32_t)params.last_value;
        DEBUG_CSR_WRITE_INFO(register_address, params.last_value);
      }
    }

    ++entry;
  }
}

void esr_slam_engine_with_base_address(ESR_SLAM_TABLE *table, uint64_t base_address)
{
  ESR_SLAM_TABLE_ENTRY *entry;
  volatile uint64_t    *register_address;
  CONTROL_PARAMS       params;

  assert(table != NULL);
  assert(table->id == SLAM_TABLE_ID_ESR_SLAM_TABLE);
  assert(table->version >= SLAM_TABLE_VERSION);

  DPRINTF("id = 0x%08x\n", table->id);
  DPRINTF("version = 0x%08x\n", table->version);
  DPRINTF("attribute = 0x%08x\n", table->attribute);
  DPRINTF("reserved = 0x%08x\n", table->reserved);
  DPRINTF("base_address = 0x%016lx\n", table->base_address);

  params.last_value = 0;
  params.store_into_next_register = false;
  params.skip_n_register = 0;
  params.slam_mode = SLAM_MODE_DEFAULT;

  if(base_address == 0) {
    base_address = table->base_address;
  }
  else {
    DPRINTF("Overwriting base_address in table with 0x%016lx\n", base_address);
  }

  entry = table->entries;
  while(entry->register_offset != (uint64_t)STO_SLAM_TABLE_END) {
    DEBUG_ESR_DETAIL(params.slam_mode, params.skip_n_register, params.last_value, entry->register_offset, entry->mask, entry->value);
    if(params.skip_n_register > 0) {
      --params.skip_n_register;
      ++entry;
      continue;
    }
    if(IS_ESR_SLAM_OPS(entry->register_offset)) {
      process_ops((uint32_t)entry->register_offset, entry->mask, entry->value, &params);
    }
    else {
      register_address = (volatile uint64_t *) (base_address + entry->register_offset);
      if(!params.store_into_next_register) {
        if(params.slam_mode == SLAM_MODE_READ || params.slam_mode == SLAM_MODE_READ_MODIFY_WRITE) {
          params.last_value = *register_address;
          DEBUG_ESR_READ_INFO(register_address, params.last_value);
          params.last_value = (params.last_value & (~entry->mask)) | entry->value;
        }
        else if (params.slam_mode == SLAM_MODE_WRITE) {
          params.last_value = entry->value & ~entry->mask;
        }
      }
      params.store_into_next_register = false;
      if(params.slam_mode == SLAM_MODE_WRITE || params.slam_mode == SLAM_MODE_READ_MODIFY_WRITE) {
        *register_address = params.last_value;
          DEBUG_ESR_WRITE_INFO(register_address, params.last_value);
      }
    }

    ++entry;
  }
}
