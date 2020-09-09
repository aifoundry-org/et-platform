#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

static inline uint32_t atomic_load_local_32(volatile const uint32_t *address);
static inline uint64_t atomic_load_local_64(volatile const uint64_t *address);
static inline uint32_t atomic_load_global_32(volatile const uint32_t *address);
static inline uint64_t atomic_load_global_64(volatile const uint64_t *address);

static inline void atomic_store_local_32(volatile uint32_t *address, uint32_t value);
static inline void atomic_store_local_64(volatile uint64_t *address, uint64_t value);
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

#define atomic_load_template(scope, type, cscope, size)                                            \
static inline uint##size##_t atomic_load_##cscope##_##size(volatile const uint##size##_t *address) \
{                                                                                                  \
    uint##size##_t result;                                                                         \
    asm volatile(                                                                                  \
        "amoor" #scope "." #type " %[res], x0, %[addr]"                                            \
        : [res]  "=r" (result)                                                                     \
        : [addr]  "A" (*address)                                                                   \
    );                                                                                             \
    return result;                                                                                 \
}

#define atomic_store_template(scope, type, cscope, size)                            \
static inline void atomic_store_##cscope##_##size(volatile uint##size##_t *address, \
                                                  uint##size##_t value)             \
{                                                                                   \
    asm volatile(                                                                   \
        "amoswap" #scope "." #type " x0, %[val], %[addr]"                           \
        : [addr] "=A" (*address)                                                    \
        : [val]   "r" (value)                                                       \
    );                                                                              \
}

#define atomic_op_template(name, op, scope, type, cscope, size)                                  \
static inline uint##size##_t atomic_##name##_##cscope##_##size(volatile uint##size##_t *address, \
                                                               uint##size##_t value)             \
{                                                                                                \
    uint##size##_t result;                                                                       \
    asm volatile(                                                                                \
        "amo" #op "" #scope "." #type " %[res], %[val], %[addr]"                                 \
        : [res]  "=r" (result),                                                                  \
          [addr] "+A" (*address)                                                                 \
        : [val]   "r" (value)                                                                    \
    );                                                                                           \
    return result;                                                                               \
}

#define atomic_define_variants(func)           \
    atomic_##func##_template(l, w, local,  32) \
    atomic_##func##_template(l, d, local,  64) \
    atomic_##func##_template(g, w, global, 32) \
    atomic_##func##_template(g, d, global, 64)

#define atomic_define_op_variants(name, op)        \
    atomic_op_template(name, op, l, w, local,  32) \
    atomic_op_template(name, op, l, d, local,  64) \
    atomic_op_template(name, op, g, w, global, 32) \
    atomic_op_template(name, op, g, d, global, 64)

atomic_define_variants(load)
atomic_define_variants(store)
atomic_define_op_variants(exchange, swap)
atomic_define_op_variants(add, add)
atomic_define_op_variants(and, and)
atomic_define_op_variants(or, or)

#undef atomic_load_template
#undef atomic_store_template
#undef atomic_op_template
#undef atomic_define_variants
#undef atomic_define_op_variants

#endif
