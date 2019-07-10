#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

// Generic atomic_read wrapper
#define atomic_read(x) _Generic((x), \
    volatile uint64_t*: read_u64, \
    uint64_t*: read_u64, \
    volatile int64_t*: read_64, \
    int64_t*: read_64 \
)(x)

// Generic atomic_write wrapper
#define atomic_write(x, y) _Generic((x), \
    volatile uint64_t*: write_u64, \
    uint64_t*: write_u64, \
    volatile int64_t*: write_64, \
    int64_t*: write_64 \
)(x, y)

// Generic atomic_add wrapper
#define atomic_add(x, y) _Generic((x), \
    volatile uint64_t*: add_u64, \
    uint64_t*: add_u64, \
    volatile int64_t*: add_64, \
    int64_t*: add_64 \
)(x, y)

static inline __attribute__((always_inline)) uint64_t read_u64(const volatile uint64_t* const address)
{
    uint64_t result;

    // Description: Global atomic 64-bit or operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // The original value in memory is returned in integer register 'rd'.
    // Assembly: amoorg.d rd, rs2, (rs1)
    asm volatile (
        "amoorg.d %0, x0, %1"
        : "=r" (result)
        : "m" (*address)
    );

    return result;
}

static inline __attribute__((always_inline)) int64_t read_64(const volatile int64_t* const address)
{
    int64_t result;

    asm volatile (
        "amoorg.d %0, x0, %1"
        : "=r" (result)
        : "m" (*address)
    );

    return result;
}

static inline __attribute__((always_inline)) void write_u64(volatile uint64_t* const address, uint64_t data)
{
    // Description: Global atomic 64-bit swap operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // Assembly: amoswapg.d rd, rs2, (rs1)
    asm volatile (
        "amoswapg.d x0, %1, %0 \n"
        : "=m" (*address)
        : "r" (data)
    );
}

static inline __attribute__((always_inline)) void write_64(volatile int64_t* const address, int64_t data)
{
    asm volatile (
        "amoswapg.d x0, %1, %0 \n"
        : "=m" (*address)
        : "r" (data)
    );
}

static inline __attribute__((always_inline)) uint64_t add_u64(volatile uint64_t* const address, uint64_t data)
{
    uint64_t prev;

    // Description: Global atomic 64-bit addition operation between the value in integer register ‘rs2’
    // and the value in the memory address pointed by integer register ‘rs1’.
    // The original value in memory is returned in integer register ‘rd’.
    // Assembly: amoaddg.d rd, rs2, (rs1)
    asm volatile (
        "amoaddg.d %0, %2, %1 \n"
        : "=r" (prev), "+m" (*address)
        : "r" (data)
    );

    return prev;
}

static inline __attribute__((always_inline)) int64_t add_64(volatile int64_t* const address, int64_t data)
{
    int64_t prev;

    asm volatile (
        "amoaddg.d %0, %2, %1 \n"
        : "=r" (prev), "+m" (*address)
        : "r" (data)
    );

    return prev;
}

#endif
