#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdint.h>

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

#define atomic_store_template(scope, type, cscope, size)                                                  \
static inline void atomic_store_##cscope##_##size(volatile uint##size##_t *address, uint##size##_t value) \
{                                                                                                         \
    asm volatile(                                                                                         \
        "amoswap" #scope "." #type " x0, %[val], %[addr]"                                                 \
        : [addr] "=A" (*address)                                                                          \
        : [val]   "r" (value)                                                                             \
    );                                                                                                    \
}

#define atomic_op_template(name, op, scope, type, cscope, size)                                                        \
static inline uint##size##_t atomic_##name##_##cscope##_##size(volatile uint##size##_t *address, uint##size##_t value) \
{                                                                                                                      \
    uint##size##_t result;                                                                                             \
    asm volatile(                                                                                                      \
        "amo" #op "" #scope "." #type " %[res], %[val], %[addr]"                                                       \
        : [res]  "=r" (result),                                                                                        \
          [addr] "+A" (*address)                                                                                       \
        : [val]   "r" (value)                                                                                          \
    );                                                                                                                 \
    return result;                                                                                                     \
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

#endif
