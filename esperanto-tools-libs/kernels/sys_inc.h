#ifndef SYS_INC_H
#define SYS_INC_H

#define BOOTROM_START_IP 0x8000001000

// Special addresses in ESR region (4G-8G)
#define ATOMIC_REGION 0x013FF40100ULL
#define FCC_THREAD0   0x013FF400C0ULL
#define FCC_THREAD1   0x013FF400D0ULL
#define IPI_NET       0x0108000800ULL
#define FCC0_MASTER   0x01083400C0ULL
#define FCC1_MASTER   0x01083400C8ULL

#define FLB_SWI     0
#define FLB_FINAL   1
#define FLB_BOOT    2

#define MINIONS_PER_SHIRE 32
#define THREADS_PER_MINION 2
#define SHIRES_COUNT 32
#define MINIONS_COUNT (SHIRES_COUNT*MINIONS_PER_SHIRE)

#define CACHE_LINE_SIZE 64

//ETSOC map
#define ELF_ADDR_START    0x8000000000
#define LAYER_ADDR_START_1  0x8040000000
#define DATA_ADDR_START   0x8080000000
#define STACK_ADDR_START  0x8100000000
#define MASTER_ADDR_START 0x8200000000

// RAM memory region
//#define RAM_MEMORY_REGION 0x8100000000
#define RAM_MEMORY_REGION 0x8000000000

#define BLOCK_SHARED_REGION                 (RAM_MEMORY_REGION + 0x100000000)  // +4GB
#define BLOCK_SHARED_REGION_SIZE_PER_SHIRE  (256*1024)
#define BLOCK_SHARED_REGION_TOTAL_SIZE      (BLOCK_SHARED_REGION_SIZE_PER_SHIRE * SHIRES_COUNT)

//#define LAUNCH_PARAMS_AREA_BASE (RAM_MEMORY_REGION + 0x200000000) // +8G
#define LAUNCH_PARAMS_AREA_BASE LAYER_ADDR_START_1
#define LAUNCH_PARAMS_AREA_SIZE 0x10000

//#define STACK_REGION                    (RAM_MEMORY_REGION + 0x300000000) // +12GB
#define STACK_REGION STACK_ADDR_START
#define STACK_REGION_SIZE_PER_THREAD    0x1000
#define STACK_REGION_TOTAL_SIZE         (STACK_REGION_SIZE_PER_THREAD * MINIONS_COUNT * 2)

//#define GLOBAL_MEM_REGION_BASE  (RAM_MEMORY_REGION + 0x400000000)      // +16GB
#define GLOBAL_MEM_REGION_BASE DATA_ADDR_START
#define GLOBAL_MEM_REGION_SIZE  (512*1024*1024) // + 512MB
#define EXEC_MEM_REGION_BASE    (GLOBAL_MEM_REGION_BASE + GLOBAL_MEM_REGION_SIZE)
#define EXEC_MEM_REGION_SIZE    (128*1024*1024) // + 128MB


#define CAUSE_ILLEGAL_INSTRUCTION 2

//                                  mhartid  |    rs1=0     |        csrrs    
#define INST_READ_MHARTID_MASK  ((0xFFFULL<<20) | (0x1f << 15) | (0x7<<12) | (0x7f))
#define INST_READ_MHARTID_MATCH ((0xF14ULL<<20) | (0<<15)      | (0x2<<12) | (0x73))

#define INST_WRITE_TXSLEEP27_MASK  ((0xFFFULL<<20) | (0x7<<12) | (0x7f))
#define INST_WRITE_TXSLEEP27_MATCH ((0x7D1ULL<<20) | (0x1<<12) | (0x73))


#define CACHEOP_DEST_LVL_MEM    0x0c00000000000000ull
#define CACHEOP_DEST_LVL_L3     0x0800000000000000ull
#define CACHEOP_DEST_LVL_L2     0x0400000000000000ull

#ifndef __ASSEMBLER__
#ifndef INCLUDE_FOR_HOST

// Auxiliary functions
#define AUX_FUN_ATTRS static inline __attribute__((always_inline))

AUX_FUN_ATTRS unsigned int get_hart_id()
{
    int ret;
    __asm__ __volatile__ (
        "csrr %[ret], 0xcd0\n"
      : [ret] "=r" (ret)
      :
      :
    );
    return ret;
}

AUX_FUN_ATTRS unsigned int get_minion_id() { return get_hart_id() >> 1; }
AUX_FUN_ATTRS unsigned int get_thread_id() { return get_hart_id() & 1; }
AUX_FUN_ATTRS unsigned int get_shire_id() { return get_minion_id() / MINIONS_PER_SHIRE; }
AUX_FUN_ATTRS unsigned int get_lane_id() { return get_minion_id() % MINIONS_PER_SHIRE; }

AUX_FUN_ATTRS void infinite_sleep() { asm volatile("1:\n" "wfi\n" "j 1b\n"); }
AUX_FUN_ATTRS void assert_unreachable() { asm volatile ("lb x31, 0(x0)" : : : "x31"); infinite_sleep(); };
#define ASSERT(cond) do { if (!(cond)) { assert_unreachable(); } } while (0)

// alignment up with not necessarily power of 2
template < typename T >
T align_up(T val, T align)
{
    return (val + align - 1) / align * align;
}

// alignment down with not necessarily power of 2
template < typename T >
T align_down(T val, T align)
{
    return (val) / align * align;
}

// Send IPI to maxion.
AUX_FUN_ATTRS void send_ipi_to_maxion()
{
    *(volatile uint64_t*)IPI_NET = 1;
}

// Inform maxion about initialization completion.
AUX_FUN_ATTRS void send_ready_to_maxion()
{
    // TODO: enable
    //*(volatile uint64_t*)FCC1_MASTER = 1;
}

// Evict cacheline for addr without waiting
AUX_FUN_ATTRS void evict_cacheline_nowait(void* virtual_addr, unsigned long long dest_value)
{
    asm volatile("csrw 0x89f, %0\n" // cache_op EvictVa into RAM
                 : : "r" ((unsigned long long)virtual_addr | dest_value));
}

// Evict cachelines of given address range and wait for completion
AUX_FUN_ATTRS void evict_range_wait(void* virtual_addr, unsigned len, unsigned long long dest_value)
{
    unsigned long long aligned_addr = align_down( (unsigned long long)virtual_addr, (unsigned long long)CACHE_LINE_SIZE );
    unsigned long long end_addr = (unsigned long long)virtual_addr + len;

    for ( unsigned long long addr = aligned_addr ; addr < end_addr ; addr += CACHE_LINE_SIZE )
    {
        evict_cacheline_nowait( (void*)addr, dest_value );
    }

    asm volatile("csrw 0x830, %0\n" // tensor_wait for cache_op
                 : : "r" (6));
}

// Sleep on FCC (Fast Credit Counter).
AUX_FUN_ATTRS void intra_shire_sleep()
{
    asm volatile("csrw 0x821, zero");
}

/*
 * Send FCC-Event to first @nr_active_t0 t0-threads and @nr_active_t1 t1-threads in shire but self.
 */
AUX_FUN_ATTRS void intra_shire_wake_but_self(unsigned nr_active_t0, // number of participating t0-threads
                                             unsigned nr_active_t1) // number of participating t1-threads
{
    uint64_t fcc_mask_t0 = !nr_active_t0 ? 0 : (uint64_t)-1 >> (64 - nr_active_t0); // low nr_active_t0 bits are ones
    uint64_t fcc_mask_t1 = !nr_active_t1 ? 0 : (uint64_t)-1 >> (64 - nr_active_t1); // low nr_active_t1 bits are ones
    // exclude self from one of the masks
    if (get_thread_id() == 0) {
        fcc_mask_t0 &= ~(1ull << get_lane_id());
    } else {
        fcc_mask_t1 &= ~(1ull << get_lane_id());
    }

    // Wake t0-threads.
    if (fcc_mask_t0) {
        *(volatile uint64_t*)FCC_THREAD0 = fcc_mask_t0;
    }
    // Wake t1-threads.
    if (fcc_mask_t1) {
        *(volatile uint64_t*)FCC_THREAD1 = fcc_mask_t1;
    }
}

/*
 * Synchronize first @nr_active_t0 t0-threads and @nr_active_t1 t1-threads in shire.
 * Each participating thread must call this function, and only such.
 *
 * After synchronization:
 * - the last guy returns immediately with true in return value,
 * - all other threads sleep on FCC and will return false when-or-if will receive FCC-Event.
 */
AUX_FUN_ATTRS bool intra_shire_barrier(unsigned barrier_idx,  // index of the barrier
                                       unsigned nr_active_t0, // number of participating t0-threads
                                       unsigned nr_active_t1) // number of participating t1-threads
{
    uint64_t flbarrier_descr = (barrier_idx                       << 0)
                             | ((nr_active_t0 + nr_active_t1 - 1) << 5);
    uint64_t last_guy;

    asm volatile("fence\n"
                 "csrrw %[last_guy], 0x820, %[flbarrier_descr]\n"
                 : [last_guy] "=r" (last_guy)
                 : [flbarrier_descr] "r" (flbarrier_descr)
                 :
    );
    if (!last_guy) {
        intra_shire_sleep();
    }
    return last_guy;
}

AUX_FUN_ATTRS uint64_t AmoAdd64Global(uint64_t addr, uint64_t val)
{
    uint64_t ret;
    asm volatile("amoaddg.d  %[ret], %[addr], %[val]\n"
                 : [ret] "=r" (ret)
                 : [addr] "r" (addr),
                   [val] "r" (val)
                 :
    );
    return ret;
}

AUX_FUN_ATTRS uint64_t AmoLoad64Global(uint64_t addr)
{
    uint64_t ret;
    asm volatile("amoorg.d  %[ret], %[addr], zero\n"
                 : [ret] "=r" (ret)
                 : [addr] "r" (addr)
                 :
    );
    return ret;
}

AUX_FUN_ATTRS void AmoStore32Global(uint64_t addr, uint32_t val)
{
    asm volatile("amoswapg.w zero, %[addr], %[val]\n"
                 :
                 : [addr] "r" (addr),
                   [val] "r" (val)
                 :
    );
}

AUX_FUN_ATTRS void AmoStore64Global(uint64_t addr, uint64_t val)
{
    asm volatile("amoswapg.d zero, %[addr], %[val]\n"
                 :
                 : [addr] "r" (addr),
                   [val] "r" (val)
                 :
    );
}

AUX_FUN_ATTRS void AmoStore64Local(uint64_t addr, uint64_t val)
{
    asm volatile("amoswapl.d zero, %[addr], %[val]\n"
                 :
                 : [addr] "r" (addr),
                   [val] "r" (val)
                 :
    );
}

/*
 * Reduce arguments with FADD-operation from all t0/t1-threads in shire.
 * Each thread in shire must call this function.
 *
 * Only return value from first thread in shire holds the desired result.
 * All other threads return garbage and should not be used.
 */
AUX_FUN_ATTRS float intra_shire_treduce_fadd(float a)
{
    uint64_t treduce_descr = (3ull/*REDUCE*/    << 0)
                           | (0ull/*level*/     << 3)
                           | (0ull/*FADD*/      << 24)
                           | (0ull/*start_reg*/ << 57)
                           | (1ull/*num_reg*/   << 16);
    // will be performed treduce for levels in [0..4] i.e. rely that shire have 2^(4+1) = 32 minions
    ASSERT(MINIONS_PER_SHIRE == 32);

    float res;
    asm volatile("fmv.s f0, %[a]\n"
                 "add   x31, zero, %[treduce_descr]\n"
                 "csrw  0x800, x31\n"
                 "addi  x31, x31, 8\n"
                 "csrw  0x800, x31\n"
                 "addi  x31, x31, 8\n"
                 "csrw  0x800, x31\n"
                 "addi  x31, x31, 8\n"
                 "csrw  0x800, x31\n"
                 "addi  x31, x31, 8\n"
                 "csrw  0x800, x31\n"
                 "fmv.s %[res], f0\n"
                 : [res] "=f" (res)
                 : [a] "f" (a),
                   [treduce_descr] "r" (treduce_descr)
                 : "x31", "f0"
    );
    return res;
}

enum class TensorLoadTrans
{
    None                 = 0,
    Interleave8          = 1,
    TransposeInterleave8 = 3,
    Transpose8           = 5,
    Transpose32          = 7,
};

AUX_FUN_ATTRS void tensor_load(unsigned scp_line, const void *mem, uint64_t stride, int rows, TensorLoadTrans trans)
{
    uint64_t tload_descr = (uint64_t(scp_line)                   << 53)
                         | (uint64_t(mem)                        << 0)
                         | (uint64_t(rows - 1)                   << 0)
                         | (uint64_t(trans)                      << 59);

    asm volatile("add   x30, zero, %[tload_descr]\n"
                 "add   x31, zero, %[stride]\n"
                 "csrw  0x83f, x30\n"
                 :
                 : [tload_descr] "r" (tload_descr),
                   [stride] "r" (stride)
                 : "x30", "x31"
    );
}

enum class TensorFmaType
{
    F32 = 0,
    I8I32 = 3,
};

AUX_FUN_ATTRS void tensor_fma(bool is_first_pass, unsigned scp_line_a, unsigned scp_line_b, int arows, int acols, int bcols, TensorFmaType type)
{
    bcols = align_up(bcols, 4);
    uint64_t tfma_descr = (uint64_t(is_first_pass ? 1 : 0)      << 0)
                        | (uint64_t(type)                       << 1)
                        | (uint64_t(scp_line_a)                 << 4)
                        | (uint64_t(scp_line_b)                 << 12)
                        | (uint64_t(acols - 1)                  << 47)
                        | (uint64_t(arows - 1)                  << 51)
                        | (uint64_t(bcols/4 - 1)                << 55);

    asm volatile("add   x30, zero, %[tfma_descr]\n"
                 "csrw  0x801, x30\n"
                 :
                 : [tfma_descr] "r" (tfma_descr)
                 : "x30",
                   "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
                   "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    );
}

AUX_FUN_ATTRS void tensor_store(void *mem, uint64_t stride, int rows, int cols)
{
    ASSERT(cols % 4 == 0);
    uint64_t tstore_descr = (uint64_t(mem)                        << 0)
                          | (uint64_t(rows - 1)                   << 51)
                          | (uint64_t(cols/4 - 1)                 << 55);

    asm volatile("add   x30, zero, %[tstore_descr]\n"
                 "add   x31, zero, %[stride]\n"
                 "csrw  0x87f, x30\n"
                 :
                 : [tstore_descr] "r" (tstore_descr),
                   [stride] "r" (stride)
                 : "x30", "x31",
                   "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
                   "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    );
}

AUX_FUN_ATTRS void tensor_store_emulation(void *mem, uint64_t stride, int rows, int cols)
{
    uint64_t mask = (1 << cols) - 1;
    uint64_t mask0 = mask & 0xFF;
    uint64_t mask1 = mask >> 8;

    asm volatile("mova.x.m  x31\n"
                 "mv        x30, %[addr]\n"

#define STORE_MATRIX_ROW(f_0, f_1) \
                 "mov.m.x   m0, %[mask0], 0\n" \
                 "fsw.ps  " #f_0 ", 0(x30)\n" \
                 "mov.m.x   m0, %[mask1], 0\n" \
                 "fsw.ps  " #f_1 ", 32(x30)\n" \
                 "add       x30, x30, %[stride]\n" \
                 "beq       x30, %[end_addr], 1f\n"

                 STORE_MATRIX_ROW(f0, f1)
                 STORE_MATRIX_ROW(f2, f3)
                 STORE_MATRIX_ROW(f4, f5)
                 STORE_MATRIX_ROW(f6, f7)
                 STORE_MATRIX_ROW(f8, f9)
                 STORE_MATRIX_ROW(f10, f11)
                 STORE_MATRIX_ROW(f12, f13)
                 STORE_MATRIX_ROW(f14, f15)
                 STORE_MATRIX_ROW(f16, f17)
                 STORE_MATRIX_ROW(f18, f19)
                 STORE_MATRIX_ROW(f20, f21)
                 STORE_MATRIX_ROW(f22, f23)
                 STORE_MATRIX_ROW(f24, f25)
                 STORE_MATRIX_ROW(f26, f27)
                 STORE_MATRIX_ROW(f28, f29)
                 STORE_MATRIX_ROW(f30, f31)

#undef STORE_MATRIX_ROW

                 "1:\n"
                 "mova.m.x  x31\n"
                 :
                 : [mask0] "r" (mask0), [mask1] "r" (mask1),
                   [addr] "r" (mem), [end_addr] "r" (uint64_t(mem) + stride * rows),
                   [stride] "r" (stride)
                 : "x30", "x31",
                   "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
                   "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31"
    );
}

#endif /* !INCLUDE_FOR_HOST */
#endif /* !__ASSEMBLER__ */

#endif /* SYS_INC_H */
