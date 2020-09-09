#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

static inline uint8_t  atomic_load_local_8(volatile const uint8_t *address);
static inline uint16_t atomic_load_local_16(volatile const uint16_t *address);
static inline uint32_t atomic_load_local_32(volatile const uint32_t *address);
static inline uint64_t atomic_load_local_64(volatile const uint64_t *address);
static inline uint8_t  atomic_load_global_8(volatile const uint8_t *address);
static inline uint16_t atomic_load_global_16(volatile const uint16_t *address);
static inline uint32_t atomic_load_global_32(volatile const uint32_t *address);
static inline uint64_t atomic_load_global_64(volatile const uint64_t *address);

static inline void atomic_store_local_8(volatile uint8_t *address, uint8_t value);
static inline void atomic_store_local_16(volatile uint16_t *address, uint16_t value);
static inline void atomic_store_local_32(volatile uint32_t *address, uint32_t value);
static inline void atomic_store_local_64(volatile uint64_t *address, uint64_t value);
static inline void atomic_store_global_8(volatile uint8_t *address, uint8_t value);
static inline void atomic_store_global_16(volatile uint16_t *address, uint16_t value);
static inline void atomic_store_global_32(volatile uint32_t *address, uint32_t value);
static inline void atomic_store_global_64(volatile uint64_t *address, uint64_t value);

static inline uint32_t atomic_exchange_local_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_exchange_local_64(volatile uint64_t *address, uint64_t value);
static inline uint32_t atomic_exchange_global_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_exchange_global_64(volatile uint64_t *address, uint64_t value);

static inline uint32_t atomic_add_local_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_add_local_64(volatile uint64_t *address, uint64_t value);
static inline uint32_t atomic_add_global_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_add_global_64(volatile uint64_t *address, uint64_t value);

static inline uint32_t atomic_and_local_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_and_local_64(volatile uint64_t *address, uint64_t value);
static inline uint32_t atomic_and_global_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_and_global_64(volatile uint64_t *address, uint64_t value);

static inline uint32_t atomic_or_local_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_or_local_64(volatile uint64_t *address, uint64_t value);
static inline uint32_t atomic_or_global_32(volatile uint32_t *address, uint32_t value);
static inline uint64_t atomic_or_global_64(volatile uint64_t *address, uint64_t value);

#include "atomic-impl.h"

#endif
