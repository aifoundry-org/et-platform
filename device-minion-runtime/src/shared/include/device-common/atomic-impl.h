/*
 * DON'T INCLUDE THIS FILE DIRECTLY
 */

#define atomic_store_small_template(scope, type, cscope, size)                      \
static inline void atomic_store_##cscope##_##size(volatile uint##size##_t *address, \
                                                  uint##size##_t value)             \
{                                                                                   \
    asm volatile(                                                                   \
        "s" #type #scope " %[val], %[addr]"                                         \
        : [addr] "=A" (*address)                                                    \
        : [val]   "r" (value)                                                       \
    );                                                                              \
}

#define atomic_load_small_template(cscope, size)                                                   \
static inline uint##size##_t atomic_load_##cscope##_##size(volatile const uint##size##_t *address) \
{                                                                                                  \
    uint32_t result, offset, *address32;                                                           \
    offset = (uintptr_t)address & 3ull;                                                            \
    address32 = (uint32_t *)((uintptr_t)address & ~3ull);                                          \
    result = atomic_load_##cscope##_32(address32);                                                 \
    return (result >> (offset * 8)) & ((1ul << (sizeof(uint##size##_t) * 8)) - 1);                 \
}

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

#define atomic_load_signed_template(cscope, size)                                                         \
static inline int##size##_t atomic_load_signed_##cscope##_##size(volatile const int##size##_t *address)   \
{                                                                                                         \
    return (int##size##_t)atomic_load_##cscope##_##size((volatile const uint##size##_t*)address);         \
}

#define atomic_store_signed_template(cscope, size)                                         \
static inline void atomic_store_signed_##cscope##_##size(volatile int##size##_t *address,  \
                                                         int##size##_t value)              \
{                                                                                          \
    atomic_store_##cscope##_##size((volatile uint##size##_t*)address,                      \
                                   (uint##size##_t)value);                                 \
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

#define atomic_signed_op_template(name, op, scope, type, cscope, size)                                  \
static inline int##size##_t atomic_##name##_signed_##cscope##_##size(volatile int##size##_t *address,   \
                                                               int##size##_t value)                     \
{                                                                                                       \
    int##size##_t result;                                                                               \
    asm volatile(                                                                                       \
        "amo" #op "" #scope "." #type " %[res], %[val], %[addr]"                                        \
        : [res]  "=r" (result),                                                                         \
          [addr] "+A" (*address)                                                                        \
        : [val]   "r" (value)                                                                           \
    );                                                                                                  \
    return result;                                                                                      \
}

#define atomic_define_variants(func)           \
    atomic_##func##_template(l, w, local,  32) \
    atomic_##func##_template(l, d, local,  64) \
    atomic_##func##_template(g, w, global, 32) \
    atomic_##func##_template(g, d, global, 64)

#define atomic_define_signed_variants(func)     \
    atomic_##func##_signed_template(local,   8) \
    atomic_##func##_signed_template(local,  16) \
    atomic_##func##_signed_template(local,  32) \
    atomic_##func##_signed_template(local,  64) \
    atomic_##func##_signed_template(global,  8) \
    atomic_##func##_signed_template(global, 16) \
    atomic_##func##_signed_template(global, 32) \
    atomic_##func##_signed_template(global, 64)

#define atomic_define_op_variants(name, op)        \
    atomic_op_template(name, op, l, w, local,  32) \
    atomic_op_template(name, op, l, d, local,  64) \
    atomic_op_template(name, op, g, w, global, 32) \
    atomic_op_template(name, op, g, d, global, 64)

#define atomic_define_signed_op_variants(name, op)        \
    atomic_signed_op_template(name, op, l, w, local,  32) \
    atomic_signed_op_template(name, op, l, d, local,  64) \
    atomic_signed_op_template(name, op, g, w, global, 32) \
    atomic_signed_op_template(name, op, g, d, global, 64)

atomic_load_small_template(local, 8)
atomic_load_small_template(local, 16)
atomic_load_small_template(global, 8)
atomic_load_small_template(global, 16)

atomic_store_small_template(l, b, local, 8)
atomic_store_small_template(l, h, local, 16)
atomic_store_small_template(g, b, global, 8)
atomic_store_small_template(g, h, global, 16)

atomic_define_variants(load)
atomic_define_variants(store)
atomic_define_signed_variants(load)
atomic_define_signed_variants(store)
atomic_define_op_variants(exchange, swap)
atomic_define_op_variants(add, add)
atomic_define_op_variants(and, and)
atomic_define_op_variants(or, or)
atomic_define_signed_op_variants(add, add)

#undef atomic_store_small_template
#undef atomic_load_small_template
#undef atomic_load_template
#undef atomic_store_template
#undef atomic_load_signed_template
#undef atomic_store_signed_template
#undef atomic_op_template
#undef atomic_signed_op_template
#undef atomic_define_variants
#undef atomic_define_signed_variants
#undef atomic_define_op_variants
#undef atomic_define_signed_op_variants
