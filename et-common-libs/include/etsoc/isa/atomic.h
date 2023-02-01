/***********************************************************************/
/*! \copyright
  Copyright (C) 2018 Esperanto Technologies Inc.
  The copyright to the computer program(s) herein is the
  property of Esperanto Technologies, Inc. All Rights Reserved.
  The program(s) may be used and/or copied only with
  the written permission of Esperanto Technologies and
  in accordance with the terms and conditions stipulated in the
  agreement/contract under which the program(s) have been supplied.
*/
/***********************************************************************/
/*! \file atomic.h
    \brief A C header that defines the atomic load/store services available.
*/
/***********************************************************************/
#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*! \fn  static inline uint8_t atomic_load_local_8(volatile const uint8_t *address)
    \brief  This function load a byte using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_local_8 api
*/
static inline uint8_t atomic_load_local_8(volatile const uint8_t *address);

/*! \fn static inline uint16_t atomic_load_local_16(volatile const uint16_t *address)
    \brief  This function load 2-bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_local_16 api
*/
static inline uint16_t atomic_load_local_16(volatile const uint16_t *address);

/*! \fn  static inline uint32_t atomic_load_local_32(volatile const uint32_t *address)
    \brief  This function load 4-bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_local_32 api
*/
static inline uint32_t atomic_load_local_32(volatile const uint32_t *address);

/*! \fn static inline uint64_t atomic_load_local_64(volatile const uint64_t *address)
    \brief  This function load 8-bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_local_64 api
*/
static inline uint64_t atomic_load_local_64(volatile const uint64_t *address);

/*! \fn static inline uint8_t atomic_load_global_8(volatile const uint8_t *address)
    \brief  This function load 1-byte using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_global_8 api
*/
static inline uint8_t atomic_load_global_8(volatile const uint8_t *address);

/*! \fn static inline uint16_t atomic_load_global_16(volatile const uint16_t *address)
    \brief  This function load 2-bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_global_16 api
*/
static inline uint16_t atomic_load_global_16(volatile const uint16_t *address);

/*! \fn static inline uint32_t atomic_load_global_32(volatile const uint32_t *address)
    \brief  This function load 4-bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_global_32 api
*/
static inline uint32_t atomic_load_global_32(volatile const uint32_t *address);

/*! \fn static inline uint64_t atomic_load_global_64(volatile const uint64_t *address)
    \brief  This function load 8-bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_global_64 api
*/
static inline uint64_t atomic_load_global_64(volatile const uint64_t *address);

/*! \fn static inline int8_t atomic_load_signed_local_8(volatile const int8_t *address)
    \brief  This function load 1 signed byte using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_local_8 api
*/
static inline int8_t atomic_load_signed_local_8(volatile const int8_t *address);

/*! \fn static inline int16_t atomic_load_signed_local_16(volatile const int16_t *address)
    \brief  This function load 2 signed bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_local_16 api
*/
static inline int16_t atomic_load_signed_local_16(volatile const int16_t *address);

/*! \fn static inline int32_t atomic_load_signed_local_32(volatile const int32_t *address)
    \brief  This function load 4 signed bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_local_32 api
*/
static inline int32_t atomic_load_signed_local_32(volatile const int32_t *address);

/*! \fn static inline int64_t atomic_load_signed_local_64(volatile const int64_t *address)
    \brief  This function load 8 signed bytes using local atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_local_64 api
*/
static inline int64_t atomic_load_signed_local_64(volatile const int64_t *address);

/*! \fn static inline int8_t atomic_load_signed_global_8(volatile const int8_t *address)
    \brief  This function load 1 signed byte using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_global_8 api
*/
static inline int8_t atomic_load_signed_global_8(volatile const int8_t *address);

/*! \fn static inline int16_t atomic_load_signed_global_16(volatile const int16_t *address)
    \brief  This function load 2 signed bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_global_16 api
*/
static inline int16_t atomic_load_signed_global_16(volatile const int16_t *address);

/*! \fn static inline int32_t atomic_load_signed_global_32(volatile const int32_t *address)
    \brief  This function load 4 signed bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_global_32 api
*/
static inline int32_t atomic_load_signed_global_32(volatile const int32_t *address);

/*! \fn static inline int64_t atomic_load_signed_global_64(volatile const int64_t *address)
    \brief  This function load 8 signed bytes using global atomics.
    \param address  address to load data from
    \return value on given address
    \memops Implementation of atomic_load_signed_global_64 api
*/
static inline int64_t atomic_load_signed_global_64(volatile const int64_t *address);

/*! \fn static inline void atomic_store_local_8(volatile uint8_t *address, uint8_t value)
    \brief  This function store 1 byte to given address using local atomics.
    \param address  address to load data from
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_local_8 api
*/
static inline void atomic_store_local_8(volatile uint8_t *address, uint8_t value);

/*! \fn static inline void atomic_store_local_16(volatile uint16_t *address, uint16_t value)
    \brief  This function store 2 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_local_16 api
*/
static inline void atomic_store_local_16(volatile uint16_t *address, uint16_t value);

/*! \fn static inline void atomic_store_local_32(volatile uint32_t *address, uint32_t value)
    \brief  This function store 4 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_local_32 api
*/
static inline void atomic_store_local_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline void atomic_store_local_64(volatile uint64_t *address, uint64_t value)
    \brief  This function store 8 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_local_64 api
*/
static inline void atomic_store_local_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline void atomic_store_global_8(volatile uint8_t *address, uint8_t value)
    \brief  This function store 1 byte to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_global_8 api
*/
static inline void atomic_store_global_8(volatile uint8_t *address, uint8_t value);

/*! \fn static inline void atomic_store_global_16(volatile uint16_t *address, uint16_t value)
    \brief  This function store 2 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_global_16 api
*/
static inline void atomic_store_global_16(volatile uint16_t *address, uint16_t value);

/*! \fn static inline void atomic_store_global_32(volatile uint32_t *address, uint32_t value)
    \brief  This function store 4 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_global_32 api
*/
static inline void atomic_store_global_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline void atomic_store_global_64(volatile uint64_t *address, uint64_t value)
    \brief  This function store 8 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_global_64 api
*/
static inline void atomic_store_global_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline void atomic_store_signed_local_8(volatile int8_t *address, int8_t value)
    \brief  This function store signed 1 byte to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_local_8 api
*/
static inline void atomic_store_signed_local_8(volatile int8_t *address, int8_t value);

/*! \fn static inline void atomic_store_signed_local_16(volatile int16_t *address, int16_t value)
    \brief  This function store signed 2 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_local_16 api
*/
static inline void atomic_store_signed_local_16(volatile int16_t *address, int16_t value);

/*! \fn static inline void atomic_store_signed_local_32(volatile int32_t *address, int32_t value)
    \brief  This function store signed 4 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_local_32 api
*/
static inline void atomic_store_signed_local_32(volatile int32_t *address, int32_t value);

/*! \fn static inline void atomic_store_signed_local_64(volatile int64_t *address, int64_t value)
    \brief  This function store signed 8 bytes to given address using local atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_local_64 api
*/
static inline void atomic_store_signed_local_64(volatile int64_t *address, int64_t value);

/*! \fn static inline void atomic_store_signed_global_8(volatile int8_t *address, int8_t value)
    \brief  This function store signed 1 byte to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_global_8 api
*/
static inline void atomic_store_signed_global_8(volatile int8_t *address, int8_t value);

/*! \fn static inline void atomic_store_signed_global_16(volatile int16_t *address, int16_t value)
    \brief  This function store signed 2 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_global_16 api
*/
static inline void atomic_store_signed_global_16(volatile int16_t *address, int16_t value);

/*! \fn static inline void atomic_store_signed_global_32(volatile int32_t *address, int32_t value)
    \brief  This function store signed 4 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_global_32 api
*/
static inline void atomic_store_signed_global_32(volatile int32_t *address, int32_t value);

/*! \fn static inline void atomic_store_signed_global_64(volatile int64_t *address, int64_t value)
    \brief  This function store signed 8 bytes to given address using global atomics.
    \param address  address to store data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_store_signed_global_64 api
*/
static inline void atomic_store_signed_global_64(volatile int64_t *address, int64_t value);

/*! \fn static inline uint32_t atomic_exchange_local_32(volatile uint32_t *address, uint32_t value)
    \brief  This function exchanges 4 bytes to given address using local atomics.
    \param address  address to exchange data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_exchange_local_32 api
*/
static inline uint32_t atomic_exchange_local_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_exchange_local_64(volatile uint64_t *address, uint64_t value)
    \brief  This function exchanges 8 bytes to given address using local atomics.
    \param address  address to exchange data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_exchange_local_64 api
*/
static inline uint64_t atomic_exchange_local_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_exchange_global_32(volatile uint32_t *address, uint32_t value)
    \brief  This function exchanges 2 bytes to given address using global atomics.
    \param address  address to exchange data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_exchange_global_32 api
*/
static inline uint32_t atomic_exchange_global_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_exchange_global_64(volatile uint64_t *address, uint64_t value)
    \brief  This function exchanges 8 bytes to given address using global atomics.
    \param address  address to exchange data
    \param value  value to store
    \return value on given address
    \memops Implementation of atomic_exchange_global_64 api
*/
static inline uint64_t atomic_exchange_global_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_add_local_32(volatile uint32_t *address, uint32_t value)
    \brief  This function adds 4 bytes to value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_local_32 api
*/
static inline uint32_t atomic_add_local_32(volatile uint32_t *address, uint32_t value);
/*! \fn static inline uint64_t atomic_add_local_64(volatile uint64_t *address, uint64_t value)
    \brief  This function adds 8 bytes to value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_local_64 api
*/
static inline uint64_t atomic_add_local_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_add_global_32(volatile uint32_t *address, uint32_t value)
    \brief  This function adds 4 bytes to value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_global_32 api
*/
static inline uint32_t atomic_add_global_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_add_global_64(volatile uint64_t *address, uint64_t value)
    \brief  This function adds 8 bytes to value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_global_64 api
*/
static inline uint64_t atomic_add_global_64(volatile uint64_t *address, uint64_t value);

/*! \fn  static inline int32_t atomic_add_signed_local_32(volatile int32_t *address, int32_t value)
    \brief  This function adds signed 4 bytes to value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_signed_local_32 api
*/
static inline int32_t atomic_add_signed_local_32(volatile int32_t *address, int32_t value);

/*! \fn static inline int64_t atomic_add_signed_local_64(volatile int64_t *address, int64_t value)
    \brief  This function adds signed 8 bytes to value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_signed_local_64 api
*/
static inline int64_t atomic_add_signed_local_64(volatile int64_t *address, int64_t value);

/*! \fn static inline int32_t atomic_add_signed_global_32(volatile int32_t *address, int32_t value)
    \brief  This function adds signed 4 bytes to value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_signed_global_32 api
*/
static inline int32_t atomic_add_signed_global_32(volatile int32_t *address, int32_t value);

/*! \fn static inline int64_t atomic_add_signed_global_64(volatile int64_t *address, int64_t value)
    \brief  This function adds signed 8 bytes to value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to add
    \return value on given address
    \memops Implementation of atomic_add_signed_global_64 api
*/
static inline int64_t atomic_add_signed_global_64(volatile int64_t *address, int64_t value);

/*! \fn static inline uint32_t atomic_and_local_32(volatile uint32_t *address, uint32_t value)
    \brief  This function performs AND operation on 4 bytes value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to perform AND with
    \return value on given address
    \memops Implementation of atomic_and_local_32 api
*/
static inline uint32_t atomic_and_local_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_and_local_64(volatile uint64_t *address, uint64_t value)
    \brief  This function performs AND operation on 8 bytes value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to perform AND with
    \return value on given address
    \memops Implementation of atomic_and_local_64 api
*/
static inline uint64_t atomic_and_local_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_and_global_32(volatile uint32_t *address, uint32_t value)
    \brief  This function performs AND operation on 4 bytes value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to perform AND with
    \return value on given address
    \memops Implementation of atomic_and_global_32 api
*/
static inline uint32_t atomic_and_global_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_and_global_64(volatile uint64_t *address, uint64_t value)
    \brief  This function performs AND operation on 8 bytes value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to perform AND with
    \return value on given address
    \memops Implementation of atomic_and_global_64 api
*/
static inline uint64_t atomic_and_global_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_or_local_32(volatile uint32_t *address, uint32_t value)
    \brief  This function performs OR operation on 4 bytes value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to perform OR with
    \return value on given address
    \memops Implementation of atomic_or_local_32 api
*/
static inline uint32_t atomic_or_local_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_or_local_64(volatile uint64_t *address, uint64_t value)
    \brief  This function performs OR operation on 8 bytes value on given address using local atomics.
    \param address  address to exchange data
    \param value  value to perform OR with
    \return value on given address
    \memops Implementation of atomic_or_local_64 api
*/
static inline uint64_t atomic_or_local_64(volatile uint64_t *address, uint64_t value);

/*! \fn static inline uint32_t atomic_or_global_32(volatile uint32_t *address, uint32_t value)
    \brief  This function performs OR operation on 4 bytes value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to perform OR with
    \return value on given address
    \memops Implementation of atomic_or_global_32 api
*/
static inline uint32_t atomic_or_global_32(volatile uint32_t *address, uint32_t value);

/*! \fn static inline uint64_t atomic_or_global_64(volatile uint64_t *address, uint64_t value)
    \brief  This function performs OR operation on 8 bytes value on given address using global atomics.
    \param address  address to exchange data
    \param value  value to perform OR with
    \return value on given address
    \memops Implementation of atomic_or_global_64 api
*/
static inline uint64_t atomic_or_global_64(volatile uint64_t *address, uint64_t value);

/*! \fn  static inline uint32_t atomic_compare_and_exchange_local_32(
    volatile uint32_t *address, uint32_t expected, uint32_t desired)
    \brief This function compares expected value on given address and replace it with desired value if comparison 
    is successful.
    \param address  address to exchange data
    \param expected  expected value
    \param desired  desired value
    \return value on given address
    \memops Implementation of atomic_compare_and_exchange_local_32 api
*/
static inline uint32_t atomic_compare_and_exchange_local_32(
    volatile uint32_t *address, uint32_t expected, uint32_t desired);

/*! \fn  static inline uint64_t atomic_compare_and_exchange_local_64(
    volatile uint64_t *address, uint64_t expected, uint64_t desired)
    \brief This function compares expected value on given address and replace it with desired value if comparison 
    is successful.
    \param address  address to exchange data
    \param expected  expected value
    \param desired  desired value
    \return value on given address
    \memops Implementation of atomic_compare_and_exchange_local_64 api
*/
static inline uint64_t atomic_compare_and_exchange_local_64(
    volatile uint64_t *address, uint64_t expected, uint64_t desired);

/*! \fn static inline uint32_t atomic_compare_and_exchange_global_32(
    volatile uint32_t *address, uint32_t expected, uint32_t desired)
    \brief This function compares expected value on given address and replace it with desired value if comparison 
    is successful.
    \param address  address to exchange data
    \param expected  expected value
    \param desired  desired value
    \return value on given address
    \memops Implementation of atomic_compare_and_exchange_global_32 api
*/
static inline uint32_t atomic_compare_and_exchange_global_32(
    volatile uint32_t *address, uint32_t expected, uint32_t desired);

/*! \fn static inline uint64_t atomic_compare_and_exchange_global_64(
    volatile uint64_t *address, uint64_t expected, uint64_t desired)
    \brief This function compares expected value on given address and replace it with desired value if comparison 
    is successful.
    \param address  address to exchange data
    \param expected  expected value
    \param desired  desired value
    \return value on given address
    \memops Implementation of atomic_compare_and_exchange_global_64 api
*/
static inline uint64_t atomic_compare_and_exchange_global_64(
    volatile uint64_t *address, uint64_t expected, uint64_t desired);

/*! \example atomics.c
    Example(s) of using atomic functions.
*/

#include "etsoc/isa/atomic-impl.h"

#ifdef __cplusplus
}
#endif

#endif
