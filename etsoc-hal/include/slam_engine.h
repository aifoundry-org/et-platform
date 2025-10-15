/* Copyright (c) 2025 Ainekko, Co. */
/* SPDX-License-Identifier: Apache-2.0 */
/*                                                                         */
// From commit 77867a77416cb9dd48aeec75dd61e97b919cad1e in slam_engine repository
#ifndef __SLAM_ENGINE_H
#define __SLAM_ENGINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DO_PRAGMA(x)  _Pragma(#x)

/*
** For global definition
*/
#define SLAM_TABLE_ID_CSR_SLAM_TABLE                0x5f435352      // '_CSR'
#define SLAM_TABLE_ID_ESR_SLAM_TABLE                0x5f455352      // '_ESR'

#define SLAM_TABLE_MAJOR_VERSION                    1               // Major version has to bump if number of type of tables increases
#define SLAM_TABLE_MINOR_VERSION                    1               // Minor version has to bump if structure or STO_xxxx changes
#define SLAM_TABLE_VERSION                          ((SLAM_TABLE_MAJOR_VERSION << 16) | SLAM_TABLE_MINOR_VERSION)
#define GET_TABLE_MAJOR_VERSION(x)                  ((x >> 16) & 0xffff)
#define GET_TABLE_MINOR_VERSION(x)                  (x & 0xffff)

/*
** All Slam Table Operations
**
**   These are defined as negative numbers on purpose in order to be used in both 32 bits and 64 bits tables
*/
#define NUMBER_OF_SLAM_TABLE_OPS                    0xf       // needs to be continuous bits from lsb

#define STO_SLAM_TABLE_END                          (-1)      // mark the end of a table
#define STO_SWITCH_MODE                             (-2)      // op1: mode to switch to (defined below)
#define STO_NOP                                     (-3)
#define STO_US_DELAY                                (-4)      // op1: micro seconds to delay
#define STO_MODIFY_BUFFER_STORE_NEXT                (-5)      // op1: mask, op2: value
#define STO_IF_BUFFER_EQUAL_EXEC_NEXT               (-6)      // op1: value to compare
#define STO_IF_BUFFER_RANGE_EXEC_NEXT               (-7)      // op1: minimum in range to compare, op2: maximum in range to compare
#define STO_IF_BUFFER_EQUAL_SKIP_N                  (-8)      // op1: value to compare, op2: number of entries to skip

// Options for STO_SWITCH_MODE
#define SWITCH_MODE_TO_READ                         1         // In READ  mode: buffer <- [register]; buffer <- (buffer & ~op1) | op2;
#define SWITCH_MODE_TO_WRITE                        2         // In WRITE mode: buffer <- op2; [register] <- buffer;
#define SWITCH_MODE_TO_READ_MODIFY_WRITE            3         // In R/M/W mode: buffer <- [register]; buffer <- (buffer & ~op1) | op2; [register] <- buffer;

/*
** For CSR
*/
typedef struct {
  uint32_t register_offset;
  uint32_t mask;
  uint32_t value;
} CSR_SLAM_TABLE_ENTRY;

typedef struct {
  uint32_t             id;
  uint32_t             version;
  uint32_t             attribute;
  uint32_t             reserved;
  uint64_t             base_address;
  CSR_SLAM_TABLE_ENTRY entries[];
} CSR_SLAM_TABLE;
typedef CSR_SLAM_TABLE *CSR_SLAM_TABLE_PTR;
#define CSR_SLAM_TABLE_PTR_NULL ((CSR_SLAM_TABLE*)NULL)

#define CSR_SLAM_TABLE_START(name, attribute, base)                     \
DO_PRAGMA(GCC diagnostic push)                                          \
DO_PRAGMA(GCC diagnostic ignored "-Wpedantic")                          \
DO_PRAGMA(GCC diagnostic ignored "-Wsign-conversion")                   \
CSR_SLAM_TABLE name = {                                                 \
  SLAM_TABLE_ID_CSR_SLAM_TABLE,                                         \
  SLAM_TABLE_VERSION,                                                   \
  attribute,                                                            \
  0,                                                                    \
  base,                                                                 \
  {
    /*
    ** entries of table will go here
    */
#define CSR_SLAM_TABLE_END(name)                                        \
    { STO_SLAM_TABLE_END, STO_SLAM_TABLE_END, STO_SLAM_TABLE_END }      \
  }                                                                     \
};                                                                      \
DO_PRAGMA(GCC diagnostic pop)

#define CSR_SLAM_ENGINE(name)                  csr_slam_engine_with_base_address(name, (uint64_t)0)
#define CSR_SLAM_ENGINE_WITH_BASE(name, base)  csr_slam_engine_with_base_address(name, (uint64_t)base)

void csr_slam_engine_with_base_address(CSR_SLAM_TABLE *table, uint64_t base_address);

/*
** For ESR
*/
typedef struct {
  uint64_t register_offset;
  uint64_t mask;
  uint64_t value;
} ESR_SLAM_TABLE_ENTRY;

typedef struct {
  uint32_t             id;
  uint32_t             version;
  uint32_t             attribute;
  uint32_t             reserved;
  uint64_t             base_address;
  ESR_SLAM_TABLE_ENTRY entries[];
} ESR_SLAM_TABLE;
typedef ESR_SLAM_TABLE *ESR_SLAM_TABLE_PTR;
#define ESR_SLAM_TABLE_PTR_NULL ((ESR_SLAM_TABLE*)NULL)

#define ESR_SLAM_TABLE_START(name, attribute, base)                     \
DO_PRAGMA(GCC diagnostic push)                                          \
DO_PRAGMA(GCC diagnostic ignored "-Wpedantic")                          \
DO_PRAGMA(GCC diagnostic ignored "-Wsign-conversion")                   \
ESR_SLAM_TABLE name = {                                                 \
  SLAM_TABLE_ID_ESR_SLAM_TABLE,                                         \
  SLAM_TABLE_VERSION,                                                   \
  attribute,                                                            \
  0,                                                                    \
  base,                                                                 \
  {
    /*
    ** entries of table will go here
    */
#define ESR_SLAM_TABLE_END(name)                                        \
    { STO_SLAM_TABLE_END, STO_SLAM_TABLE_END, STO_SLAM_TABLE_END }      \
  }                                                                     \
};                                                                      \
DO_PRAGMA(GCC diagnostic pop)

#define ESR_SLAM_ENGINE(name)                     esr_slam_engine_with_base_address(name, (uint64_t)0)
#define ESR_SLAM_ENGINE_WITH_BASE(name, base)     esr_slam_engine_with_base_address(name, (uint64_t)base)

void esr_slam_engine_with_base_address(ESR_SLAM_TABLE *table, uint64_t base_address);

#define SLAM_ENGINE(name) _Generic((name), \
  CSR_SLAM_TABLE*: csr_slam_engine_with_base_address((void*)name, (uint64_t)0), \
  ESR_SLAM_TABLE*: esr_slam_engine_with_base_address((void*)name, (uint64_t)0))

#define SLAM_ENGINE_WITH_BASE(name, base) _Generic((name), \
  CSR_SLAM_TABLE*: csr_slam_engine_with_base_address((void*)name, (uint64_t)base), \
  ESR_SLAM_TABLE*: esr_slam_engine_with_base_address((void*)name, (uint64_t)base))

/*
** Helper functions from external
*/

// us_delay: Delay for 1 micro second
//   required only if using STO_US_DELAY
void us_delay(uint32_t usec);

#ifdef __cplusplus
}
#endif

#endif //__SLAM_ENGINE_H
