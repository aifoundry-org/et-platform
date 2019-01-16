#ifndef _EMU_DEFINES_H
#define _EMU_DEFINES_H

#if defined(__cplusplus) && (__cplusplus >= 201103L)
#include <cinttypes>
#else
#include <inttypes.h>
#endif

// Maximum number of threads
#define EMU_NUM_SHIRES          35 // at most 33 shires (Pending fix from JIRA: RTLMIN-1949)
#define EMU_NEIGH_PER_SHIRE     4
#define EMU_MINIONS_PER_NEIGH   8
#define EMU_TBOXES_PER_SHIRE    4
#define EMU_RBOXES_PER_SHIRE    1
#define EMU_MINIONS_PER_SHIRE   (EMU_NEIGH_PER_SHIRE*EMU_MINIONS_PER_NEIGH)
#define EMU_NUM_MINIONS         (EMU_NUM_SHIRES*EMU_MINIONS_PER_SHIRE)
#define EMU_THREADS_PER_MINION  2
#define EMU_THREADS_PER_NEIGH   (EMU_THREADS_PER_MINION*EMU_MINIONS_PER_NEIGH)
#define EMU_THREADS_PER_SHIRE   (EMU_THREADS_PER_NEIGH*EMU_NEIGH_PER_SHIRE)
#define EMU_NUM_THREADS         (EMU_THREADS_PER_SHIRE*EMU_NUM_SHIRES)
#define EMU_NUM_TBOXES          (EMU_NUM_SHIRES*EMU_TBOXES_PER_SHIRE)
#define EMU_NUM_RBOXES          (EMU_NUM_SHIRES*EMU_TBOXES_PER_SHIRE)

#define NR_MSG_PORTS            4

// Some TensorFMA defines
#define TFMA_MAX_AROWS    16
#define TFMA_MAX_ACOLS    16
#define TFMA_MAX_BCOLS    16
#define TFMA_REGS_PER_ROW (64 / (VL * 4))
// FastLocalBarrier
#define FAST_LOCAL_BARRIERS 32
// TensorQuant defines
#define TQUANT_MAX_TRANS 10
// FastCreditCounters
#define EMU_NUM_FCC_COUNTERS_PER_THREAD 2

// VA to PA translation
#define PA_SIZE        40
#define PA_M           ((((uint64_t)1) << PA_SIZE) - 1)
#define VA_SIZE        48
#define VA_M           ((((uint64_t)1) << VA_SIZE) - 1)
#define PG_OFFSET_SIZE 12
#define PG_OFFSET_M    ((((uint64_t)1) << PG_OFFSET_SIZE) - 1)
#define PPN_SIZE       (PA_SIZE - PG_OFFSET_SIZE)
#define PPN_M          ((((uint64_t)1) << PPN_SIZE) - 1)
#define PTE_V_OFFSET   0
#define PTE_R_OFFSET   1
#define PTE_W_OFFSET   2
#define PTE_X_OFFSET   3
#define PTE_U_OFFSET   4
#define PTE_G_OFFSET   5
#define PTE_A_OFFSET   6
#define PTE_D_OFFSET   7
#define PTE_PPN_OFFSET 10

// SATP mode field values
#define SATP_MODE_BARE  0
#define SATP_MODE_SV39  8
#define SATP_MODE_SV48  9

// MSTATUS field offsets
#define MSTATUS_MXR     19
#define MSTATUS_SUM     18
#define MSTATUS_MPRV    17
#define MSTATUS_XS      15
#define MSTATUS_FS      13
#define MSTATUS_MPP     11
#define MSTATUS_SPP     8

// ESR region
#define ESR_REGION             0x0100000000ULL  // ESR Region bit [32] == 1
#define ESR_REGION_MASK        0xFF00000000ULL  // Mask to determine if address is in the ESR region (bits [39:32]

#define ESR_REGION_PROT_MASK   0x00C0000000ULL  // ESR Region Protection is defined in bits [31:30]
#define ESR_REGION_PROT_SHIFT  30               // Bits to shift to get the ESR Region Protection defined in bits [31:30]

#define ESR_REGION_SHIRE_MASK  0x003FC00000ULL  // On the ESR Region the Shire is defined by bits [29:22]
#define ESR_REGION_LOCAL_SHIRE 0x003FC00000ULL  // On the ESR Region the Local Shire is defined by bits [29:22] == 8'hff
#define ESR_REGION_SHIRE_SHIFT 22               // Bits to shift to get Shire, Shire is defined by bits [19:22]

#define ESR_SREGION_MASK       0x0000300000ULL  // The ESR Region is defined by bits [21:20] and further limited by bits [19:12] depending on the region
#define ESR_SREGION_SHIFT      20               // Bits to shift to get Region, Region is defined in bits [21:20]
#define ESR_SREGION_EXT_MASK   0x00003E0000ULL  // The ESR Extended Region is defined by bits [21:17]
#define ESR_SREGION_EXT_SHIFT  17               // Bits to shift to get Extended Region, Extended Region is defined in bits [21:17]
#define ESR_REGION_OFFSET      0x0000400000ULL  // 
#define ESR_SHIRE_REGION       0x0100340000ULL  // Shire ESR Region is at region [21:20] == 2'b11 and [19:17] == 2'b010
#define ESR_NEIGH_REGION       0x0100100000ULL  // Neighborhood ESR Region is at region [21:20] == 2'b01

#define ESR_HART_MASK          0x00000FF000ULL  // On HART ESR Region the HART is defined in bits [19:12]
#define ESR_HART_SHIFT         12               // On HART ESR Region bits to shift to get the HART defined in bits [19:12]

#define ESR_NEIGH_MASK         0x00000F0000ULL  // On Neighborhood ESR Region Neighborhood is defined when bits [19:16]
#define ESR_NEIGH_SHIFT        16               // On Neighborhood ESR Region bits to shift to get the Neighborhood defined in bits [19:16]
#define ESR_NEIGH_OFFSET       0x0000010000ULL  // On Neighborhood ESR Region the Neighborhood is defined by bits [19:16]
#define ESR_NEIGH_BROADCAST    16               // On Neighborhood ESR Region Neighborhood Broadcast is defined when bits [19:16] == 4'b1111

#define ESR_BANK_MASK          0x000001E000ULL  // On Shire Cache ESR Region Bank is defined in bits [16:13]
#define ESR_BANK_SHIFT         13               // On Shire Cache ESR Region bits to shift to get Bank defined in bits [16:13]

#define ESR_HART_ESR_MASK      0x0000000FF8ULL  // On HART ESR Region the ESR is defined by bits [11:3]
#define ESR_NEIGH_ESR_MASK     0x000000FFF8ULL  // On Neighborhood ESR Region the ESR is defined in bits [15:3]
#define ESR_SC_ESR_MASK        0x0000001FF8ULL  // On Shire Cache ESR Region the ESR is defined in bits [12:3]
#define ESR_SHIRE_ESR_MASK     0x000001FFF8ULL  // On Shire ESR Region the ESR is defined in bits [16:3]
#define ESR_SHIRE_ESR_SHIFT    3

// ESR Offsets
#define ESR_SHIRE_FLB_OFFSET  0x100ULL
#define ESR_SHIRE_FCC0_OFFSET 0x0C0ULL
#define ESR_SHIRE_FCC1_OFFSET 0x0C8ULL
#define ESR_SHIRE_FCC2_OFFSET 0x0D0ULL
#define ESR_SHIRE_FCC3_OFFSET 0x0D8ULL

#define ESR_HART_PORT0_OFFSET 0x800ULL
#define ESR_HART_PORT1_OFFSET 0x810ULL
#define ESR_HART_PORT2_OFFSET 0x820ULL
#define ESR_HART_PORT3_OFFSET 0x830ULL

#define ESR_SHIRE_BROADCAST0_OFFSET  0x1FFF0L
#define ESR_SHIRE_BROADCAST1_OFFSET  0x1FFF8L

#define ESR_BROADCAST_PROT_MASK      0xC00000000000000ULL // Region protection is defined in bits [60:59] in esr broadcast data write.
#define ESR_BROADCAST_PROT_SHIFT     59

#define ESR_BROADCAST_ESR_ADDR_MASK  0x7FFFF0000000000ULL // ESR broadcastable address is defined in bits[58:40] in esr broadcast data write.
#define ESR_BROADCAST_ESR_ADDR_SHIFT 40
#define ESR_BROADCAST_ESR_MAX_SHIRES ESR_BROADCAST_ESR_ADDR_SHIFT
#define ESR_BROADCAST_ESR_SHIRE_MASK 0xFFFFFFFFFFULL      // Shire to spread the broadcast address in bits [39:0] in esr broadcast data write.


// L2 scratchpad
#define L2_SCP_BASE   0x80000000ULL
#define L2_SCP_OFFSET 0x00800000ULL
#define L2_SCP_SIZE   0x00400000ULL

//
// CSRs -  DO NOT REORDER
//
typedef enum {
    csr_prv = 0, // this is internal to HW
    /* Unimplemented register */
    csr_unknown,
    /* RISCV user registers */
    // csr_ustatus,
    // csr_uie,
    // csr_utvec,
    // csr_uscratch,
    // csr_uepc,
    // csr_ucause,
    // csr_utval,
    // csr_uip,
    csr_fflags,
    csr_frm,
    csr_fcsr,
    csr_cycle,
    csr_time,
    csr_instret,
    csr_hpmcounter3,
    csr_hpmcounter4,
    csr_hpmcounter5,
    csr_hpmcounter6,
    csr_hpmcounter7,
    csr_hpmcounter8,
    csr_hpmcounter9,
    csr_hpmcounter10,
    csr_hpmcounter11,
    csr_hpmcounter12,
    csr_hpmcounter13,
    csr_hpmcounter14,
    csr_hpmcounter15,
    csr_hpmcounter16,
    csr_hpmcounter17,
    csr_hpmcounter18,
    csr_hpmcounter19,
    csr_hpmcounter20,
    csr_hpmcounter21,
    csr_hpmcounter22,
    csr_hpmcounter23,
    csr_hpmcounter24,
    csr_hpmcounter25,
    csr_hpmcounter26,
    csr_hpmcounter27,
    csr_hpmcounter28,
    csr_hpmcounter29,
    csr_hpmcounter30,
    csr_hpmcounter31,
    /* Esperanto user registers */
    csr_tensor_reduce,
    csr_tensor_fma,
    csr_tensor_conv_size,
    csr_tensor_conv_ctrl,
    csr_tensor_coop,
    csr_tensor_mask,
    csr_tensor_quant,
    csr_tex_send,
    csr_tensor_error,
    csr_scratchpad_ctrl,
    csr_usr_cache_op, //TODO remove once everything is up to spec
    csr_prefetch_va,
    csr_flb0,
    csr_fcc,
    csr_stall,
    csr_tensor_wait,
    csr_tensor_load,
    //csr_gsc_progress,
    csr_tensor_load_l2,
    csr_tensor_store,
    csr_evict_va,
    csr_flush_va,
    csr_umsg_port0, // TODO remove once everything is up to spec
    csr_umsg_port1, // TODO remove once everything is up to spec
    csr_umsg_port2, // TODO remove once everything is up to spec
    csr_umsg_port3, // TODO remove once everything is up to spec
    csr_validation0,
    csr_validation1,
    csr_validation2,
    csr_validation3,
    csr_sleep_txfma_27,
    csr_lock_va,
    csr_unlock_va,
    csr_porthead0,
    csr_porthead1,
    csr_porthead2,
    csr_porthead3,
    csr_portheadnb0,
    csr_portheadnb1,
    csr_portheadnb2,
    csr_portheadnb3,
    csr_hartid,
    /* RISCV supervisor registers */
    csr_sstatus,
    // csr_sedeleg,
    // csr_sideleg,
    csr_sie,
    csr_stvec,
    csr_scounteren,
    csr_sscratch,
    csr_sepc,
    csr_scause,
    csr_stval,
    csr_sip,
    csr_satp,
    /* Esperanto supervisor registers */
    csr_sys_cache_op,
    csr_evict_sw,
    csr_flush_sw,
    csr_smsg_port0, // TODO remove once everything is up to spec
    csr_smsg_port1, // TODO remove once everything is up to spec
    csr_smsg_port2, // TODO remove once everything is up to spec
    csr_smsg_port3, // TODO remove once everything is up to spec
    csr_portctrl0,
    csr_portctrl1,
    csr_portctrl2,
    csr_portctrl3,
    /* RISCV machine registers */
    csr_mvendorid,
    csr_marchid,
    csr_mimpid,
    csr_mhartid,
    csr_mstatus,
    csr_misa,
    csr_medeleg,
    csr_mideleg,
    csr_mie,
    csr_mtvec,
    csr_mcounteren,
    csr_mscratch,
    csr_mepc,
    csr_mcause,
    csr_mtval,
    csr_mip,
    // csr_pmcfg0,
    // csr_pmcfg1,
    // csr_pmcfg2,
    // csr_pmcfg3,
    // csr_pmpaddr0,
    // csr_pmpaddr1,
    // ...
    // csr_pmpaddr15,
    csr_mcycle,
    csr_minstret,
    csr_mhpmcounter3,
    csr_mhpmcounter4,
    csr_mhpmcounter5,
    csr_mhpmcounter6,
    csr_mhpmcounter7,
    csr_mhpmcounter8,
    csr_mhpmcounter9,
    csr_mhpmcounter10,
    csr_mhpmcounter11,
    csr_mhpmcounter12,
    csr_mhpmcounter13,
    csr_mhpmcounter14,
    csr_mhpmcounter15,
    csr_mhpmcounter16,
    csr_mhpmcounter17,
    csr_mhpmcounter18,
    csr_mhpmcounter19,
    csr_mhpmcounter20,
    csr_mhpmcounter21,
    csr_mhpmcounter22,
    csr_mhpmcounter23,
    csr_mhpmcounter24,
    csr_mhpmcounter25,
    csr_mhpmcounter26,
    csr_mhpmcounter27,
    csr_mhpmcounter28,
    csr_mhpmcounter29,
    csr_mhpmcounter30,
    csr_mhpmcounter31,
    csr_mhpmevent3,
    csr_mhpmevent4,
    csr_mhpmevent5,
    csr_mhpmevent6,
    csr_mhpmevent7,
    csr_mhpmevent8,
    csr_mhpmevent9,
    csr_mhpmevent10,
    csr_mhpmevent11,
    csr_mhpmevent12,
    csr_mhpmevent13,
    csr_mhpmevent14,
    csr_mhpmevent15,
    csr_mhpmevent16,
    csr_mhpmevent17,
    csr_mhpmevent18,
    csr_mhpmevent19,
    csr_mhpmevent20,
    csr_mhpmevent21,
    csr_mhpmevent22,
    csr_mhpmevent23,
    csr_mhpmevent24,
    csr_mhpmevent25,
    csr_mhpmevent26,
    csr_mhpmevent27,
    csr_mhpmevent28,
    csr_mhpmevent29,
    csr_mhpmevent30,
    csr_mhpmevent31,
    /* RISCV debug registers */
    // csr_tselect,
    // csr_tdata1,
    // csr_tdata2,
    // csr_tdata3,
    // csr_tinfo,
    // csr_tcontrol,
    // csr_dcsr,
    // csr_dpc,
    // csr_dscratch,
    /* Esperanto machine registers */
    csr_minstmask,
    csr_minstmatch,
    //csr_amofence_ctrl,
    csr_flush_icache,
    csr_msleep_txfma_27,
    csr_menable_shadows,
    csr_excl_mode,
    csr_mtxfma_sleep_traps,
    CSR_MAX
} csr;

#define CSR_MAX_UMODE   csr_sstatus
#define CSR_MAX_SMODE   csr_mvendorid
#define CSR_MAX_MMODE   CSR_MAX

// Memory access type
typedef enum {
    Mem_Access_Load,
    Mem_Access_Store,
    Mem_Access_Fetch
} mem_access_type;

typedef enum {
    m0 = 0,
    m1 = 1,
    m2 = 2,
    m3 = 3,
    m4 = 4,
    m5 = 5,
    m6 = 6,
    m7 = 7,
    MAXMREG = 8,
    mnone = -1
} mreg;

typedef enum {
    x0 = 0,
    x1 = 1,
    x2 = 2,
    x3 = 3,
    x4 = 4,
    x5 = 5,
    x6 = 6,
    x7 = 7,
    x8 = 8,
    x9 = 9,
    x10 = 10,
    x11 = 11,
    x12 = 12,
    x13 = 13,
    x14 = 14,
    x15 = 15,
    x16 = 16,
    x17 = 17,
    x18 = 18,
    x19 = 19,
    x20 = 20,
    x21 = 21,
    x22 = 22,
    x23 = 23,
    x24 = 24,
    x25 = 25,
    x26 = 26,
    x27 = 27,
    x28 = 28,
    x29 = 29,
    x30 = 30,
    x31 = 31,
    MAXXREG = 32,
    xnone = -1
} xreg;

typedef enum {
    f0 = 0,
    f1 = 1,
    f2 = 2,
    f3 = 3,
    f4 = 4,
    f5 = 5,
    f6 = 6,
    f7 = 7,
    f8 = 8,
    f9 = 9,
    f10 = 10,
    f11 = 11,
    f12 = 12,
    f13 = 13,
    f14 = 14,
    f15 = 15,
    f16 = 16,
    f17 = 17,
    f18 = 18,
    f19 = 19,
    f20 = 20,
    f21 = 21,
    f22 = 22,
    f23 = 23,
    f24 = 24,
    f25 = 25,
    f26 = 26,
    f27 = 27,
    f28 = 28,
    f29 = 29,
    f30 = 30,
    f31 = 31,
    MAXFREG = 32,
    fnone = -1
} freg;

typedef enum {
    rne = 0,
    rtz = 1,
    rdn = 2,
    rup = 3,
    rmm = 4,
    rmdyn = 7 //dynamic rounding mode, read from rm register
} rounding_mode;

typedef enum {
   ET_MINION = 0,
   ET_MAXION
} et_core_t;

// Number of 32-bit lanes in a vector register
#ifndef VL
#define VL  8
#endif

#if (VL != 4) && (VL != 8)
#error "Only 128-bit and 256-bit vectors supported"
#endif

/* Obsolete texsnd/texrcv instuctions use the low 128b of the fregs for data transfers */
#define VL_TBOX 4

// vector register value type
typedef union {
    uint8_t   b[VL*4];
    uint16_t  h[VL*2];
    uint32_t  u[VL];
    int32_t   i[VL];
    uint64_t  x[VL/2];
    int64_t   q[VL/2];
} fdata;

// general purpose register value type
typedef union {
    uint8_t   b[8];
    uint16_t  h[4];
    uint32_t  w[2];
    int32_t   ws[2];
    uint64_t  x;
    int64_t   xs;
} xdata;

// mask register value type
typedef struct {
    uint8_t   b[VL];
#if (VL==4)
    // FIXME: for compatibility with the RTL that has 8b masks for 128b vectors
    uint8_t   _xb[VL];
#endif
} mdata;

// Privilege levels
#define CSR_PRV_U  0
#define CSR_PRV_S  1
#define CSR_PRV_H  2
#define CSR_PRV_M  3

// Traps
#define CAUSE_MISALIGNED_FETCH      0x00
#define CAUSE_FETCH_ACCESS          0x01
#define CAUSE_ILLEGAL_INSTRUCTION   0x02
#define CAUSE_BREAKPOINT            0x03
#define CAUSE_MISALIGNED_LOAD       0x04
#define CAUSE_LOAD_ACCESS           0x05
#define CAUSE_MISALIGNED_STORE      0x06
#define CAUSE_STORE_ACCESS          0x07
#define CAUSE_USER_ECALL            0x08
#define CAUSE_SUPERVISOR_ECALL      0x09
#define CAUSE_HYPERVISOR_ECALL      0x0a
#define CAUSE_MACHINE_ECALL         0x0b
#define CAUSE_FETCH_PAGE_FAULT      0x0c
#define CAUSE_LOAD_PAGE_FAULT       0x0d
#define CAUSE_STORE_PAGE_FAULT      0x0f
#define CAUSE_MCODE_INSTRUCTION     0x1e
#define CAUSE_TXFMA_OFF             0x1f

// base class for all traps
class trap_t {
  public:
    trap_t(uint64_t n) : cause(n) {}
    uint64_t get_cause() const { return cause; }

    virtual bool has_tval() const = 0;
    virtual uint64_t get_tval() const = 0;
    virtual const char* what() = 0;

  private:
    const uint64_t cause;
};

// define a trap type without tval
#define DECLARE_TRAP_TVAL_N(n, x) \
    class x : public trap_t { \
      public: \
        x() : trap_t(n) {} \
        virtual bool has_tval() const override { return false; } \
        virtual uint64_t get_tval() const override { return 0; } \
        virtual const char* what() override { return #x; } \
    };

// define a trap type with tval
#define DECLARE_TRAP_TVAL_Y(n, x) \
    class x : public trap_t { \
      public: \
        x(uint64_t v) : trap_t(n), tval(v) {} \
        virtual bool has_tval() const override { return true; } \
        virtual uint64_t get_tval() const override { return tval; } \
        virtual const char* what() override { return #x; } \
      private: \
        const uint64_t tval; \
    };

DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_FETCH,     trap_instruction_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_ACCESS,         trap_instruction_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_ILLEGAL_INSTRUCTION,  trap_illegal_instruction)
DECLARE_TRAP_TVAL_Y(CAUSE_BREAKPOINT,           trap_breakpoint)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_LOAD,      trap_load_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_MISALIGNED_STORE,     trap_store_address_misaligned)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_ACCESS,          trap_load_access_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_ACCESS,         trap_store_access_fault)
DECLARE_TRAP_TVAL_N(CAUSE_USER_ECALL,           trap_user_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_SUPERVISOR_ECALL,     trap_supervisor_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_HYPERVISOR_ECALL,     trap_hypervisor_ecall)
DECLARE_TRAP_TVAL_N(CAUSE_MACHINE_ECALL,        trap_machine_ecall)
DECLARE_TRAP_TVAL_Y(CAUSE_FETCH_PAGE_FAULT,     trap_instruction_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_LOAD_PAGE_FAULT,      trap_load_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_STORE_PAGE_FAULT,     trap_store_page_fault)
DECLARE_TRAP_TVAL_Y(CAUSE_MCODE_INSTRUCTION,    trap_mcode_instruction)
DECLARE_TRAP_TVAL_Y(CAUSE_TXFMA_OFF,            trap_txfma_off)

#endif // _EMU_DEFINES_H

