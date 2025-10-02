#pragma once

#include <stdint.h>

#define AMO_IMPL(OP, L, W, T)                                       \
    static inline T amo##OP##L##W(uint64_t addr, T data)            \
    {                                                               \
        T rv;                                                       \
        asm volatile("amo" #OP #L "." #W " %[rv],%[data],(%[addr])" \
                     : [ rv ] "=r"(rv)                              \
                     : [ data ] "r"(data), [ addr ] "r"(addr)       \
                     : "memory");                                   \
        return rv;                                                  \
    }

#define AMO_CMPSWAP(L, W, T)                                                              \
    static inline T amocmpswap##L##W(uint64_t addr, T cmp, T data)                        \
    {                                                                                     \
        T rv;                                                                             \
        asm volatile("add x31,x0,%[cmp]; amocmpswap" #L "." #W " %[rv],%[data],(%[addr])" \
                     : [ rv ] "=r"(rv)                                                    \
                     : [ cmp ] "r"(cmp), [ data ] "r"(data), [ addr ] "r"(addr)           \
                     : "x31", "memory");                                                  \
        return rv;                                                                        \
    }

/* Global 32-bit */
AMO_IMPL(swap, g, w, uint32_t)
AMO_IMPL(and, g, w, uint32_t)
AMO_IMPL(or, g, w, uint32_t)
AMO_IMPL(xor, g, w, uint32_t)
AMO_IMPL(add, g, w, uint32_t)
AMO_IMPL(min, g, w, uint32_t)
AMO_IMPL(max, g, w, uint32_t)
AMO_IMPL(minu, g, w, uint32_t)
AMO_IMPL(maxu, g, w, uint32_t)
AMO_CMPSWAP(g, w, uint32_t)

/* Local 32-bit */
AMO_IMPL(swap, l, w, uint32_t)
AMO_IMPL(and, l, w, uint32_t)
AMO_IMPL(or, l, w, uint32_t)
AMO_IMPL(xor, l, w, uint32_t)
AMO_IMPL(add, l, w, uint32_t)
AMO_IMPL(min, l, w, uint32_t)
AMO_IMPL(max, l, w, uint32_t)
AMO_IMPL(minu, l, w, uint32_t)
AMO_IMPL(maxu, l, w, uint32_t)
AMO_CMPSWAP(l, w, uint32_t)

/* Global 64-bit */
AMO_IMPL(swap, g, d, uint64_t)
AMO_IMPL(and, g, d, uint64_t)
AMO_IMPL(or, g, d, uint64_t)
AMO_IMPL(xor, g, d, uint64_t)
AMO_IMPL(add, g, d, uint64_t)
AMO_IMPL(min, g, d, uint64_t)
AMO_IMPL(max, g, d, uint64_t)
AMO_IMPL(minu, g, d, uint64_t)
AMO_IMPL(maxu, g, d, uint64_t)
AMO_CMPSWAP(g, d, uint64_t)

/* Local 64-bit */
AMO_IMPL(swap, l, d, uint64_t)
AMO_IMPL(and, l, d, uint64_t)
AMO_IMPL(or, l, d, uint64_t)
AMO_IMPL(xor, l, d, uint64_t)
AMO_IMPL(add, l, d, uint64_t)
AMO_IMPL(min, l, d, uint64_t)
AMO_IMPL(max, l, d, uint64_t)
AMO_IMPL(minu, l, d, uint64_t)
AMO_IMPL(maxu, l, d, uint64_t)
AMO_CMPSWAP(l, d, uint64_t)

/* Pthread-like mutex. Return zero on success */
static inline int mutex_trylock(uint64_t addr) { return !amoorgw(addr, 1); }
static inline int mutex_unlock(uint64_t addr) { return !amoandgw(addr, 0); }
static inline int mutex_lock(uint64_t addr)
{
    while (mutex_trylock(addr))
        ;
    return 0;
}


static inline void barrier_wait(uint64_t addr, uint32_t max)
{
    uint32_t val = amoaddgw(addr, 1);
    if (val == max - 1) { /* last */
        amoandgw(addr, 0);
        amoorgw(addr + 4, max - 1);
    }
    else {
        while (!amoorgw(addr + 4, 0))
            ;
        amoaddgw(addr + 4, -1);
    }
}
