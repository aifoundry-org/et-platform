/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>       // FIXME: Remove this, use "emu_gio.h" instead
#include <cstring>
#include <deque>
#include <list>
#include <stdexcept>
#include <unordered_map>
#include <math.h>

#include "decode.h"
#include "emu.h"
#include "emu_casts.h"
#include "emu_gio.h"
#include "esrs.h"
#include "memmap.h"
#include "mmu.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "gold.h"
#include "log.h"
#include "rbox.h"
#include "tbox_emu.h"
#include "traps.h"
#include "txs.h"
#include "utility.h"

#include <cfenv>       // FIXME: remove this when we purge std::fesetround() from the code!

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

// Scratchpad defines
#define L1_SCP_ENTRIES    48
#define L1_SCP_LINE_SIZE  (L1D_LINE_SIZE)
typedef Packed<L1D_LINE_SIZE*8> cache_line_t;

// vendor, arch, imp, ISA values
#define CSR_VENDOR_ID ((11<<7) |        /* bank 11 */ \
                       (0xe5 & 0x7f))   /* 0xE5 (0x65 without parity) */
#define CSR_ARCH_ID 0x8000000000000001ull
#define CSR_IMP_ID  0x0
#define CSR_ISA_MAX ((1ull << 2)  | /* "C" Compressed extension */                      \
                     (1ull << 5)  | /* "F" Single-precision floating-point extension */ \
                     (1ull << 8)  | /* "I" RV32I/64I/128I base ISA */                   \
                     (1ull << 12) | /* "M" Integer Multiply/Divide extension */         \
                     (1ull << 18) | /* "S" Supervisor mode implemented */               \
                     (1ull << 20) | /* "U" User mode implemented */                     \
                     (1ull << 23) | /* "X": Non-standard extensions present */          \
                     (2ull << 62))  /* XLEN = 64-bit */

typedef enum {
    MSG_ENABLE = 7,
    MSG_DISABLE = 3,
    MSG_PGET = 0,
    MSG_PGETNB = 1,
} msg_port_conf_action;

// message port value type
typedef struct {
    bool enabled;
    bool stall;
    bool umode;
    bool use_scp;
    bool enable_oob;
    uint8_t logsize;
    uint8_t max_msgs;
    uint8_t scp_set;
    uint8_t scp_way;
    uint8_t rd_ptr;
    uint8_t wr_ptr;
    uint8_t size;
    int32_t offset;
} msg_port_conf_t;

typedef struct
{
    uint32_t source_thread;
    uint32_t target_thread;
    uint32_t target_port;
    bool     is_remote;
    bool     is_tbox;
    bool     is_rbox;
    uint32_t  data[(1 << PORT_LOG2_MAX_SIZE)/4];
    uint8_t  oob;
} msg_port_write_t;

// UART
static std::ostringstream uart_stream[EMU_NUM_THREADS];

// Register files
uint64_t xregs[EMU_NUM_THREADS][NXREGS];
freg_t   fregs[EMU_NUM_THREADS][NFREGS];
mreg_t   mregs[EMU_NUM_THREADS][NMREGS];
freg_t   tensorfma_tenc[EMU_NUM_THREADS][NFREGS];

// RISCV CSR registers
std::array<uint32_t,EMU_NUM_THREADS>    csr_fcsr;
std::array<uint64_t,EMU_NUM_THREADS>    csr_stvec;
std::array<uint32_t,EMU_NUM_THREADS>    csr_scounteren;
std::array<uint64_t,EMU_NUM_THREADS>    csr_sscratch;
std::array<uint64_t,EMU_NUM_THREADS>    csr_sepc;
std::array<uint64_t,EMU_NUM_THREADS>    csr_scause;
std::array<uint64_t,EMU_NUM_THREADS>    csr_stval;
std::array<uint64_t,EMU_NUM_THREADS+1>  csr_satp;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mstatus;
std::array<uint64_t,EMU_NUM_THREADS>    csr_misa; // could be hardcoded
std::array<uint32_t,EMU_NUM_THREADS>    csr_medeleg;
std::array<uint32_t,EMU_NUM_THREADS>    csr_mideleg;
std::array<uint32_t,EMU_NUM_THREADS>    csr_mie;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mtvec;
std::array<uint32_t,EMU_NUM_THREADS>    csr_mcounteren;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mscratch;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mepc;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mcause;
std::array<uint64_t,EMU_NUM_THREADS>    csr_mtval;
std::array<uint32_t,EMU_NUM_THREADS>    csr_mip;
std::array<uint64_t,EMU_NUM_THREADS>    csr_tdata1;
std::array<uint64_t,EMU_NUM_THREADS>    csr_tdata2;
// dcsr, dpc, dscratch
std::array<uint32_t,EMU_NUM_THREADS>    csr_mvendorid; // could be hardcoded
std::array<uint64_t,EMU_NUM_THREADS>    csr_marchid; // could be hardcoded
std::array<uint64_t,EMU_NUM_THREADS>    csr_mimpid; // could be hardcoded
std::array<uint16_t,EMU_NUM_THREADS>    csr_mhartid;
// PMU
std::array<std::array<uint64_t,6>,EMU_NUM_THREADS> csr_mhpmevent;

// Esperanto CSR registers
std::array<uint64_t,EMU_NUM_THREADS>    csr_minstmask;
std::array<uint32_t,EMU_NUM_THREADS>    csr_minstmatch;
std::array<uint8_t, EMU_NUM_THREADS+1>  csr_menable_shadows; // 1b
std::array<uint8_t, EMU_NUM_THREADS+1>  csr_excl_mode; // 1b
std::array<uint8_t, EMU_NUM_THREADS+1>  csr_mcache_control; // 2b
std::array<uint64_t,EMU_NUM_THREADS>    csr_tensor_conv_size; // can we remove?
std::array<uint64_t,EMU_NUM_THREADS>    csr_tensor_conv_ctrl; // can we remove?
std::array<uint16_t,EMU_NUM_THREADS>    csr_tensor_mask;
std::array<uint16_t,EMU_NUM_THREADS>    csr_tensor_error;
std::array<uint16_t,EMU_NUM_THREADS+1>  csr_ucache_control;
std::array<uint8_t, EMU_NUM_THREADS>    csr_gsc_progress; // log2(VL)
std::array<uint64_t,EMU_NUM_THREADS>    csr_validation0;
std::array<uint64_t,EMU_NUM_THREADS>    csr_validation2;
std::array<uint64_t,EMU_NUM_THREADS>    csr_validation3;
std::array<std::array<uint64_t,EMU_NUM_THREADS>,4> csr_portctrl;

// Other processor state
std::array<uint8_t, EMU_NUM_THREADS>    csr_prv; // FIXME: Drop 'csr_' prefix
std::array<bool,    EMU_NUM_THREADS>    mtvec_is_set;
std::array<bool,    EMU_NUM_THREADS>    stvec_is_set;
std::array<bool,    EMU_NUM_THREADS>    break_on_load;
std::array<bool,    EMU_NUM_THREADS>    break_on_store;
std::array<bool,    EMU_NUM_THREADS>    break_on_fetch;
std::array<bool,    EMU_NUM_THREADS>    debug_mode;
std::array<uint32_t,EMU_NUM_THREADS>    ext_seip;
static std::array<bool,EMU_NUM_THREADS-1> tensorload_setupb_topair;
static std::array<int, EMU_NUM_THREADS-1> tensorload_setupb_numlines;

struct tensor_reduce_info_t {
    uint16_t minion_id;
    uint8_t  start_reg;
    uint8_t  num_reg;
    uint8_t  action;
};
static std::array<tensor_reduce_info_t,EMU_NUM_MINIONS-1> tensorreduce_info;

// Scratchpad
std::array<std::array<cache_line_t,L1_SCP_ENTRIES+TFMA_MAX_AROWS>,EMU_NUM_THREADS> scp;

// Used to access different threads transparently
#define XREGS xregs[current_thread]
#define FREGS fregs[current_thread]
#define MREGS mregs[current_thread]
#define TENC  tensorfma_tenc[current_thread]
#define SCP   scp[current_thread]

// Message ports
static std::array<std::array<msg_port_conf_t,NR_MSG_PORTS>,EMU_NUM_THREADS>      msg_ports;
static std::array<std::array<std::deque<uint8_t>,NR_MSG_PORTS>,EMU_NUM_THREADS>  msg_ports_oob;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_tbox;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_rbox;
static bool msg_port_delayed_write = false;

// Accelerators
#if (EMU_TBOXES_PER_SHIRE > 1)
TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES][EMU_TBOXES_PER_SHIRE];
#else
TBOX::TBOXEmu tbox[EMU_NUM_COMPUTE_SHIRES];
#endif

#if (EMU_RBOXES_PER_SHIRE > 1)
RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES][EMU_RBOXES_PER_SHIRE];
#else
RBOX::RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES];
#endif

// FCC: these are special ESRs that look like CSRs
std::array<std::array<uint16_t,2>,EMU_NUM_THREADS> fcc;
std::array<bool,EMU_NUM_THREADS> fcc_wait;

// Shire ESRs
// FIXME: move all these into shire_other_esrs_t
uint64_t fcc_cnt;
std::array<bool,EMU_NUM_SHIRES> esr_shire_coop_mode;
#ifndef SYS_EMU
std::array<bool,EMU_NUM_SHIRES> esr_icache_prefetch_active;
#endif

#ifndef SYS_EMU
// only for checker, list of minions to awake (e.g. waiting for FCC that has just been written)
std::queue<uint32_t> minions_to_awake;
std::queue<uint32_t> &get_minions_to_awake() {return minions_to_awake;}
#endif

const char* csr_name(uint16_t num)
{
    static thread_local char unknown_name[60];
    static thread_local int  unknown_name_start = 0;
    static const std::unordered_map<uint16_t, const char*> csr_names = {
#define CSRDEF(num, lower, upper)       { num, #lower },
#include "csrs.h"
#undef CSRDEF
    };
    auto it = csr_names.find(num);
    if (it == csr_names.cend()) {
        (void) snprintf(&unknown_name[unknown_name_start], 6, "0x%03" PRIx16, num & 0xfff);
        const char * ptr = &unknown_name[unknown_name_start];
        unknown_name_start = (unknown_name_start + 5) % 60;
        return ptr;
    }
    return it->second;
}


system_version_t sysver = system_version_t::UNKNOWN;

uint64_t current_pc = 0;
uint32_t current_inst = 0;
unsigned current_thread = 0;
uint32_t num_sets = 16;
uint32_t num_ways = 4;

#define MAXSTACK 2048
static std::array<std::array<uint32_t,MAXSTACK>,EMU_NUM_THREADS> shaderstack;
static bool check_stack = false;

bool m_emu_done = false;

bool emu_done()
{
   return m_emu_done;
}

std::string dump_xregs(uint32_t thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (int ii = 0; ii < 32; ++ii)
      {
         str << "XREG[" << std::dec << ii << "] = 0x" << std::hex << xregs[thread_id][ii] << "\n";
      }
   }
   return str.str();
}

std::string dump_fregs(uint32_t thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (size_t ii = 0; ii < NFREGS; ++ii)
      {
         for (size_t jj = 0; jj < VL; ++jj)
         {
            str << "FREG[" << std::dec << ii << "][" << jj <<  "] = 0x" << std::hex << fregs[thread_id][ii].u32[jj] << "\t";
         }
         str << "\n";
      }
   }
   return str.str();
}

void init_emu(system_version_t ver)
{
    sysver = ver;
   // FIXME: remove '#include <cfenv>' when we purge this function from the code
   std::fesetround(FE_TONEAREST); // set rne for host
}

void reset_esrs_for_shire(unsigned shireid)
{
    unsigned shire = (shireid == IO_SHIRE_ID) ? EMU_IO_SHIRE_SP : shireid;

    for (unsigned neigh = 0; neigh < EMU_NEIGH_PER_SHIRE; ++neigh) {
        unsigned idx = EMU_NEIGH_PER_SHIRE*shire + neigh;
        neigh_esrs[idx].reset();
    }
    shire_cache_esrs[shire].reset();
    shire_other_esrs[shire].reset(shireid);
    broadcast_esrs[shire].reset();

    esr_shire_coop_mode[shire] = false;
#ifndef SYS_EMU
    esr_icache_prefetch_active[shire] = 0;
#endif

    // reset FCC for all threads in shire
    unsigned thread0 = shire * EMU_THREADS_PER_SHIRE;
    unsigned threadN = thread0 + (shire == EMU_IO_SHIRE_SP ? 1 : EMU_THREADS_PER_SHIRE);
    for (unsigned thread = thread0; thread < threadN; ++thread) {
        fcc[thread][0] = 0;
        fcc[thread][1] = 0;
    }
}

// forward declarations
static uint64_t csrget(uint16_t src1);
static void csrset(uint16_t src1, uint64_t val);
static void tmask_conv();
static void tcoop(uint64_t value);
static void tensorload(uint64_t control);
static void tensorloadl2(uint64_t control);
static void tensorstore(uint64_t tstorereg);
static void tensor_fma32(uint64_t tfmareg);
static void tensor_fma16a32(uint64_t tfmareg);
static void tensor_ima8a32(uint64_t tfmareg);
static void tensorquant(uint64_t value);
static void tensorreduce(uint64_t value);
static int64_t port_get(uint32_t id, bool block);
static void configure_port(uint32_t id, uint64_t wdata);
static uint64_t flbarrier(uint64_t value);
static uint64_t read_port_base_address(unsigned thread, unsigned id);

////////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
////////////////////////////////////////////////////////////////////////////////

static inline const char* get_fp_flags(uint_fast8_t flags)
{
    static const char* fnames[] = {
        "",            "NX",             "UF",             "UF,NX",
        "OF",          "OF,NX",          "OF,UF",          "OF,UF,NX",
        "DZ",          "DZ,NX",          "DZ,UF",          "DZ,UF,NX",
        "DZ,OF",       "DZ,OF,NX",       "DZ,OF,UF",       "DZ,OF,UF,NX",
        "NV",          "NV,NX",          "NV,UF",          "NV,UF,NX",
        "NV,OF",       "NV,OF,NX",       "NV,OF,UF",       "NV,OF,UF,NX",
        "NV,DZ",       "NV,DZ,NX",       "NV,DZ,UF",       "NV,DZ,UF,NX",
        "NV,DZ,OF",    "NV,DZ,OF,NX",    "NV,DZ,OF,UF",    "NV,DZ,OF,UF,NX",
        "ID",          "ID,NX",          "ID,UF",          "ID,UF,NX",
        "ID,OF",       "ID,OF,NX",       "ID,OF,UF",       "ID,OF,UF,NX",
        "ID,DZ",       "ID,DZ,NX",       "ID,DZ,UF",       "ID,DZ,UF,NX",
        "ID,DZ,OF",    "ID,DZ,OF,NX",    "ID,DZ,OF,UF",    "ID,DZ,OF,UF,NX",
        "ID,NV",       "ID,NV,NX",       "ID,NV,UF",       "ID,NV,UF,NX",
        "ID,NV,OF",    "ID,NV,OF,NX",    "ID,NV,OF,UF",    "ID,NV,OF,UF,NX",
        "ID,NV,DZ",    "ID,NV,DZ,NX",    "ID,NV,DZ,UF",    "ID,NV,DZ,UF,NX",
        "ID,NV,DZ,OF", "ID,NV,DZ,OF,NX", "ID,NV,DZ,OF,UF", "ID,NV,DZ,OF,UF,NX"
    };
    return fnames[flags % 64];
}

void init(xreg dst, uint64_t val)
{
    if (dst != x0)
    {
       XREGS[dst] = val;
       LOG(DEBUG, "init x%d <-- 0x%016" PRIx64, dst, val);
    }
}

void init_stack()
{
    check_stack = true;
    init(x2, uint64_t(&shaderstack[current_thread][MAXSTACK-1]));
}

uint64_t xget(uint64_t src1)
{
    return XREGS[src1];
}

uint64_t get_csr(uint32_t thread, uint16_t cnum)
{
    uint32_t oldthread = current_thread;
    current_thread = thread;
    uint64_t retval = csrget(cnum);
    current_thread = oldthread;
    return retval;
}

void fpinit(freg dst, uint64_t val[VL/2])
{
    for (size_t i = 0; i < VL/2; ++i)
        FREGS[dst].u64[i] = val[i];
}

// internal accessor to frm
static inline int frm()
{
    return ((csr_fcsr[current_thread] >> 5) & 0x7);
}

// internal accessor to tdata1
static inline uint64_t tdata1()
{
    return csr_tdata1[current_thread];
}

// internal accessor to tdata2
static inline uint64_t tdata2()
{
    return csr_tdata2[current_thread];
}

static void activate_breakpoints(int prv)
{
    uint64_t mcontrol = tdata1();
    break_on_load[current_thread]  = !(~mcontrol & ((8 << prv) | 1));
    break_on_store[current_thread] = !(~mcontrol & ((8 << prv) | 2));
    break_on_fetch[current_thread] = !(~mcontrol & ((8 << prv) | 4));
}

// internal accessor to prv
static inline int prvget()
{
    return csr_prv[current_thread];
}

// internal accessor to prv
static inline void prvset(int val)
{
    csr_prv[current_thread] = val & 3;
    activate_breakpoints(val & 3);
}

// internal accessor to tensor_error
static inline void update_tensor_error(unsigned thread, uint16_t value)
{
    csr_tensor_error[thread] |= value;
    if (value)
        LOG_OTHER(DEBUG, thread, "\tTensorError = 0x%04" PRIx16 " (0x%04" PRIx16 ")", csr_tensor_error[thread], value);
}

static inline void update_tensor_error(uint16_t value)
{
    update_tensor_error(current_thread, value);
}

static inline const char* get_rounding_mode(int mode)
{
    static const char* rmnames[] = {
        "rne",      "rtz",       "rdn",       "rup",
        "rmm",      "res5",      "res6",      "dyn",
        "dyn(rne)", "dyn(rtz)",  "dyn(rdn)",  "dyn(rup)",
        "dyn(rmm)", "dyn(res5)", "dyn(res6)", "dyn(res7)",
    };

    return rmnames[(mode == rmdyn) ? (8 + frm()) : (mode & 7)];
}

void reset_hart(unsigned thread)
{
    // Exit reset at M-mode
    csr_prv[thread] = CSR_PRV_M;
    debug_mode[thread] = false;

    // Read-only registers
    xregs[thread][0] = 0;
    csr_mvendorid[thread] = CSR_VENDOR_ID;
    csr_marchid[thread] = CSR_ARCH_ID;
    csr_mimpid[thread] = CSR_IMP_ID;
    csr_mhartid[thread] = (thread == (EMU_IO_SHIRE_SP*EMU_THREADS_PER_SHIRE))
        ? (IO_SHIRE_ID*EMU_THREADS_PER_SHIRE)
        : thread;

    // misa is a 0-length register
    csr_misa[thread] = CSR_ISA_MAX;

    // CSR registers with reset
    csr_excl_mode[thread] = 0;
    csr_gsc_progress[thread] = 0;
    csr_mcache_control[thread] = 0;
    csr_mcause[thread] = 0;
    csr_mcounteren[thread] = 0;
    csr_menable_shadows[thread] = 0;
    csr_minstmask[thread] = 0;
    csr_mip[thread] = 0;
    csr_mstatus[thread] = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    csr_scounteren[thread] = 0;
    csr_ucache_control[thread] = 0x200;

    // Debug-mode registers with reset
    // TODO: csr_dcsr[thread] <= xdebugver=1, prv=3;
    csr_tdata1[thread] = 0x20C0000000000000ULL;

    // Cached information
    activate_breakpoints(CSR_PRV_M);

    // Ports
    for (int i = 0; i < NR_MSG_PORTS; ++i)
    {
        memset(&msg_ports[thread][i], 0, sizeof(msg_port_conf_t));
        msg_ports[thread][i].offset = -1;
    }
    csr_portctrl[0][thread] = 0x0000000000008000ULL;
    csr_portctrl[1][thread] = 0x0000000000008000ULL;
    csr_portctrl[2][thread] = 0x0000000000008000ULL;
    csr_portctrl[3][thread] = 0x0000000000008000ULL;

    // other processor state
    ext_seip[thread] = 0;
    mtvec_is_set[thread] = false;
    stvec_is_set[thread] = false;
    break_on_load[thread] = false;
    break_on_store[thread] = false;
    break_on_fetch[thread] = false;
    fcc_wait[thread] = false;
    if (thread != EMU_IO_SHIRE_SP*EMU_THREADS_PER_SHIRE)
    {
        tensorload_setupb_topair[thread] = false;
    }
}

void minit(mreg dst, uint64_t val)
{
    MREGS[dst] = std::bitset<VL>(val);
    LOG(DEBUG, "init m[%d] <-- 0x%02lx", int(dst), MREGS[dst].to_ulong());
}

static bool tmask_pass(int bit)
{
    // Returns the pass bit for a specific bit
    return (csr_tensor_mask[current_thread] >> bit) & 1;
}

void check_pending_interrupts()
{
    // Are there any non-masked pending interrupts?
    // NB: If csr_excl_mode != 0 this thread is either in exclusive mode or
    // blocked, but either way it cannot receive interrupts
    uint64_t xip = (csr_mip[current_thread] | ext_seip[current_thread]) & csr_mie[current_thread];
    if (!xip || csr_excl_mode[current_thread])
        return;

    LOG(DEBUG, "Check Pending Interrupt mtvec:0x%016" PRIx64 " mip:0x%08" PRIx32 " mie:0x%08" PRIx32,
        csr_mtvec[current_thread], csr_mip[current_thread] | ext_seip[current_thread], csr_mie[current_thread]);

    // If there are any pending interrupts for the current privilege level
    // 'x', they are only taken if mstatus.xIE=1. If there are any pending
    // interrupts for a higher privilege level 'y>x' they must be taken
    // independently of the value in mstatus.yIE. Pending interrupts for a
    // lower privilege level 'w<x' are not taken.
    uint64_t mideleg = csr_mideleg[current_thread];
    uint64_t mip = xip & ~mideleg;
    uint64_t sip = xip & mideleg;
    uint64_t mie = csr_mstatus[current_thread] & 8;
    uint64_t sie = csr_mstatus[current_thread] & 2;
    switch (prvget())
    {
        case CSR_PRV_M:
            if (!mip || !mie) return;
            xip = mip;
            break;
        case CSR_PRV_S:
            if (!mip && !sie) return;
            xip = mip | (sie ? sip : 0);
            break;
        default:
            /* nothing */
            break;
    }

    if (xip & 0x0800) throw trap_machine_external_interrupt();
    if (xip & 0x0008) throw trap_machine_software_interrupt();
    if (xip & 0x0080) throw trap_machine_timer_interrupt();
    if (xip & 0x0200) throw trap_supervisor_external_interrupt();
    if (xip & 0x0002) throw trap_supervisor_software_interrupt();
    if (xip & 0x0020) throw trap_supervisor_timer_interrupt();
#if 0
    if (xip & 0x0100) throw trap_user_external_interrupt();
    if (xip & 0x0001) throw trap_user_software_interrupt();
    if (xip & 0x0010) throw trap_user_timer_interrupt();
#endif
}

static void trap_to_smode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = prvget();
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);
    assert(curprv <= CSR_PRV_S);

    LOG(DEBUG, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);

    // if checking against RTL, clear the correspoding MIP bit
    // it will be set to 1 again if the pending bit was not really cleared
    // just before entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by
    // a memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        csr_mip[current_thread] &= ~(1<<code);
    }
#endif

    // Take sie
    uint64_t mstatus = csr_mstatus[current_thread];
    uint64_t sie = (mstatus >> 1) & 0x1;
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set spie = sie, sie = 0, spp = prv
    csrset(CSR_MSTATUS, mstatus_clean | (curprv << 8) | (sie << 5));
    // Set scause, stval and sepc
    csrset(CSR_SCAUSE, cause);
    csrset(CSR_STVAL, val);
    csrset(CSR_SEPC, current_pc);
    // Jump to stvec
    prvset(CSR_PRV_S);

    // Throw an error if no one ever set stvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (stvec_is_set[current_thread] == false)
        LOG(DEBUG, "%s", "WARNING Trap vector has never been set. Can't take exception properly");

    // compute address where to jump to:
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause for interrupts, tvec for exceptions
    uint64_t tvec = csr_stvec[current_thread];
    if ((tvec & 1) && interrupt)
    {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    log_trap(csr_mstatus[current_thread], csr_scause[current_thread], csr_stval[current_thread], csr_sepc[current_thread]);
    log_pc_update(tvec);
}

static void trap_to_mmode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = prvget();
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);

    // Check if we should deletegate the trap to S-mode
    if ((curprv < CSR_PRV_M) && ((interrupt ? csr_mideleg[current_thread] : csr_medeleg[current_thread]) & (1ull<<code)))
    {
        trap_to_smode(cause, val);
        return;
    }

    // if checking against RTL, clear the correspoding MIP bit
    // it will be set to 1 again if the pending bit was not really cleared
    // just before entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by
    // a memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        csr_mip[current_thread] &= ~(1<<code);
    }
#endif

    LOG(DEBUG, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);

    // Take mie
    uint64_t mstatus = csr_mstatus[current_thread];
    uint64_t mie = (mstatus >> 3) & 0x1;
    // Clean mie, mpie and mpp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFE777ULL;
    // Set mpie = mie, mie = 0, mpp = prv
    csrset(CSR_MSTATUS, mstatus_clean | (curprv << 11) | (mie << 7));
    // Set mcause, mtval and mepc
    csrset(CSR_MCAUSE, cause);
    csrset(CSR_MTVAL, val);
    csrset(CSR_MEPC, current_pc);
    // Jump to mtvec
    prvset(CSR_PRV_M);

    // Throw an error if no one ever set mtvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (mtvec_is_set[current_thread] == false)
        LOG(DEBUG, "%s", "WARNING Trap vector has never been set. Doesn't smell good...");

    // compute address where to jump to
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause for interrupts, tvec for exceptions
    uint64_t tvec = csr_mtvec[current_thread];
    if ((tvec & 1) && interrupt)
    {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    log_trap(csr_mstatus[current_thread], csr_mcause[current_thread], csr_mtval[current_thread], csr_mepc[current_thread]);
    log_pc_update(tvec);
}

void take_trap(const trap_t& t)
{
    trap_to_mmode(t.get_cause(), t.get_tval());
}

void check_minst_match(uint32_t bits)
{
    uint64_t minstmask = csr_minstmask[current_thread];
    if ((minstmask >> 32) != 0)
    {
        uint32_t mask = minstmask;
        if (((bits ^ csr_minstmatch[current_thread]) & mask) == 0)
            throw trap_mcode_instruction(bits);
    }
}

void set_pc(uint64_t pc)
{
    current_pc = pc;
}

void set_thread(unsigned thread)
{
    current_thread = thread;
}

uint32_t get_thread()
{
    return current_thread;
}

bool thread_is_blocked(unsigned thread)
{
    unsigned other_excl = 1 + ((~thread & 1) << 1);
    return csr_excl_mode[thread] == other_excl;
}

uint32_t get_mask(unsigned maskNr)
{
    return MREGS[maskNr].to_ulong();
}

extern inst_state_change * log_info;

////////////////////////////////////////////////////////////////////////////////
//
// Callback for messages
//
////////////////////////////////////////////////////////////////////////////////

void def_msg_to_thread(int thread_id __attribute__((unused)))
{
}

void (*msg_to_thread) (int) = def_msg_to_thread;

void set_msg_funcs(void (*func_msg_to_thread) (int))
{
    msg_to_thread = func_msg_to_thread;
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64I emulation
//
////////////////////////////////////////////////////////////////////////////////

// ILLEGAL INSTRUCTION
void unknown(const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): unknown @%016" PRIx64 "(0x%04x)", PRVNAME, PC, current_inst);
    throw trap_illegal_instruction(current_inst);
}

////////////////////////////////////////////////////////////////////////////////
//
// SYSTEM emulation
//
////////////////////////////////////////////////////////////////////////////////

// forward declarations
static void dcache_evict_flush_set_way(bool, bool, int, int, int, int);
static void dcache_evict_flush_vaddr(bool, bool, int, uint64_t, int, int, uint64_t);
static void dcache_prefetch_vaddr(bool, int, uint64_t, int, int, uint64_t);
static void dcache_lock_vaddr(bool, uint64_t, int, int, uint64_t);
static void dcache_unlock_vaddr(bool, uint64_t, int, int, uint64_t);
static void dcache_lock_paddr(int, uint64_t);
static void dcache_unlock_set_way(int, int);

static void check_counter_is_enabled(int cnt)
{
    uint64_t enabled = (csr_mcounteren[current_thread] & (1 << cnt));
    if ( ((prvget() == CSR_PRV_U) && ((csr_scounteren[current_thread] & enabled) == 0))
         || ((prvget() == CSR_PRV_S) && (enabled == 0)))
    {
        throw trap_illegal_instruction(current_inst);
    }
}

static uint64_t csrget(uint16_t src1)
{
    uint64_t val;

    switch (src1)
    {
    case CSR_FFLAGS:
        require_fp_active();
        val = csr_fcsr[current_thread] & 0x8000001f;
        break;
    case CSR_FRM:
        require_fp_active();
        val = (csr_fcsr[current_thread] >> 5) & 0x7;
        break;
    case CSR_FCSR:
        require_fp_active();
        val = csr_fcsr[current_thread];
        break;
    case CSR_SSTATUS:
        // Hide sxl, tsr, tw, tvm, mprv, mpp, mpie, mie
        val = csr_mstatus[current_thread] & 0x80000003000DE133ULL;
        break;
    case CSR_SIE:
        val = csr_mie[current_thread] & csr_mideleg[current_thread];
        break;
    case CSR_STVEC:
        val = csr_stvec[current_thread];
        break;
    case CSR_SCOUNTEREN:
        val = csr_scounteren[current_thread];
        break;
    case CSR_SSCRATCH:
        val = csr_sscratch[current_thread];
        break;
    case CSR_SEPC:
        val = csr_sepc[current_thread];
        break;
    case CSR_SCAUSE:
        val = csr_scause[current_thread];
        break;
    case CSR_STVAL:
        val = csr_stval[current_thread];
        break;
    case CSR_SIP:
        val = csr_mip[current_thread] & csr_mideleg[current_thread];
        break;
    case CSR_SATP:
        val = csr_satp[current_thread];
        break;
    case CSR_MSTATUS:
        val = csr_mstatus[current_thread];
        break;
    case CSR_MISA:
        val = csr_misa[current_thread];
        break;
    case CSR_MEDELEG:
        val = csr_medeleg[current_thread];
        break;
    case CSR_MIDELEG:
        val = csr_mideleg[current_thread];
        break;
    case CSR_MIE:
        val = csr_mie[current_thread];
        break;
    case CSR_MTVEC:
        val = csr_mtvec[current_thread];
        break;
    case CSR_MCOUNTEREN:
        val = csr_mcounteren[current_thread];
        break;
    // MHPMEVENT3...MHPMEVENT31
    case CSR_MSCRATCH:
        val = csr_mscratch[current_thread];
        break;
    case CSR_MEPC:
        val = csr_mepc[current_thread];
        break;
    case CSR_MCAUSE:
        val = csr_mcause[current_thread];
        break;
    case CSR_MTVAL:
        val = csr_mtval[current_thread];
        break;
    case CSR_MIP:
        val = csr_mip[current_thread];
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        val = csr_tdata1[current_thread];
        break;
    case CSR_TDATA2:
        val = csr_tdata2[current_thread];
        break;
    case CSR_TDATA3:
        val = 0;
        break;
    // TODO: DCSR
    // TODO: DPC
    // TODO: DSCRATCH
    case CSR_MCYCLE:
    case CSR_MINSTRET:
        val = 0;
        break;
    case CSR_CYCLE:
    case CSR_INSTRET:
    case CSR_HPMCOUNTER3:
    case CSR_HPMCOUNTER4:
    case CSR_HPMCOUNTER5:
    case CSR_HPMCOUNTER6:
    case CSR_HPMCOUNTER7:
    case CSR_HPMCOUNTER8:
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
        check_counter_is_enabled(src1 - CSR_CYCLE);
        val = 0;
        break;
    case CSR_MVENDORID:
        val = csr_mvendorid[current_thread];
        break;
    case CSR_MARCHID:
        val = csr_marchid[current_thread];
        break;
    case CSR_MIMPID:
        val = csr_mimpid[current_thread];
        break;
    case CSR_MHARTID:
        val = csr_mhartid[current_thread];
        break;
    // ----- Esperanto registers -------------------------------------
    // TODO: CSR_MATP
    case CSR_MINSTMASK:
        val = csr_minstmask[current_thread];
        break;
    case CSR_MINSTMATCH:
        val = csr_minstmatch[current_thread];
        break;
    // TODO: CSR_AMOFENCE_CTRL
    case CSR_CACHE_INVALIDATE:
        val = 0;
        break;
    case CSR_MENABLE_SHADOWS:
        val = csr_menable_shadows[current_thread];
        break;
    case CSR_EXCL_MODE:
        val = csr_excl_mode[current_thread] & 1;
        break;
    case CSR_MCACHE_CONTROL:
        val = csr_mcache_control[current_thread];
        break;
    case CSR_EVICT_SW:
    case CSR_FLUSH_SW:
    case CSR_LOCK_SW:
    case CSR_UNLOCK_SW:
        val = 0;
        break;
    case CSR_TENSOR_REDUCE:
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TENSOR_CONV_SIZE:
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_COOP:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TENSOR_MASK:
        require_feature_ml();
        val = csr_tensor_mask[current_thread];
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_TEX_SEND:
        require_feature_gfx();
        val = 0;
        break;
    case CSR_TENSOR_ERROR:
        require_feature_ml();
        val = csr_tensor_error[current_thread];
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        val = csr_ucache_control[current_thread];
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_FLB:
    case CSR_FCC:
    case CSR_STALL:
    case CSR_TENSOR_WAIT:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_GSC_PROGRESS:
        val = csr_gsc_progress[current_thread];
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_EVICT_VA:
    case CSR_FLUSH_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_VALIDATION0:
        val = csr_validation0[current_thread];
        break;
    case CSR_VALIDATION1:
        val = 0;
        break;
    case CSR_VALIDATION2:
        val = csr_validation2[current_thread];
        break;
    case CSR_VALIDATION3:
        val = csr_validation3[current_thread];
        break;
    case CSR_LOCK_VA:
    case CSR_UNLOCK_VA:
        require_lock_unlock_enabled();
        val = 0;
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = csr_portctrl[src1 - CSR_PORTCTRL0][current_thread];
        break;
    case CSR_FCCNB:
        require_feature_ml();
        val = (uint64_t(fcc[current_thread][1]) << 16) + uint64_t(fcc[current_thread][0]);
        break;
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
        if (((csr_portctrl[src1-CSR_PORTHEAD0][current_thread] & 0x1) == 0)
            || ((prvget() == CSR_PRV_U) && ((csr_portctrl[src1-CSR_PORTHEAD0][current_thread] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(src1 - CSR_PORTHEAD0, true);
        break;
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
        if (((csr_portctrl[src1-CSR_PORTHEADNB0][current_thread] & 0x1) == 0)
            || ((prvget() == CSR_PRV_U) && ((csr_portctrl[src1-CSR_PORTHEADNB0][current_thread] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(src1 - CSR_PORTHEADNB0, false);
        break;
    case CSR_HARTID:
        if (prvget() != CSR_PRV_M && (csr_menable_shadows[current_thread] & 1) == 0)
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = csr_mhartid[current_thread];
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val = csr_mhpmevent[src1 - CSR_MHPMEVENT3][current_thread];
        break;
    // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }
    return val;
}

static void csrset(uint16_t src1, uint64_t val)
{
    uint64_t msk;

    switch (src1)
    {
    case CSR_FFLAGS:
        require_fp_active();
        val = (csr_fcsr[current_thread] & 0x000000E0) | (val & 0x8000001F);
        csr_fcsr[current_thread] = val;
        LOG(DEBUG, "Updating FFLAGS, new CSR is %08lx", val);
        break;
    case CSR_FRM:
        require_fp_active();
        val = (csr_fcsr[current_thread] & 0x8000001F) | ((val & 0x7) << 5);
        csr_fcsr[current_thread] = val;
        LOG(DEBUG, "Updating FRM, new CSR is %08lx", val);
        break;
    case CSR_FCSR:
        require_fp_active();
        val &= 0x800000FF;
        csr_fcsr[current_thread] = val;
        LOG(DEBUG, "Updating FCSR, new CSR is %08lx", val);
        break;
    case CSR_SSTATUS:
        // Preserve sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
        val = (val & 0x00000000000C6133ULL) | (csr_mstatus[current_thread] & 0x0000000F00739888ULL);
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
        {
            val |= 0x8000000000000000ULL;
        }
        csr_mstatus[current_thread] = val;
        break;
    case CSR_SIE:
        // Only ssie, stie, and seie are writeable, and only if they are delegated
        // if mideleg[sei,sti,ssi]==1 then seie, stie, ssie is writeable, otherwise they are reserved
        msk = csr_mideleg[current_thread] & 0x0000000000000222ULL;
        val = (csr_mie[current_thread] & ~msk) | (val & msk);
        csr_mie[current_thread] = val;
        break;
    case CSR_STVEC:
        val = sextVA(val & ~0xFFEULL);
        csr_stvec[current_thread] = val;
        stvec_is_set[current_thread] = true;
        break;
    case CSR_SCOUNTEREN:
        val &= 0xFFFFFFFFULL;
        csr_scounteren[current_thread] = val;
        break;
    case CSR_SSCRATCH:
        csr_sscratch[current_thread] = val;
        break;
    case CSR_SEPC:
        // sepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        csr_sepc[current_thread] = val;
        break;
    case CSR_SCAUSE:
        // Maks all bits excepts the ones we care about
        val &= 0x800000000000001FULL;
        csr_scause[current_thread] = val;
        break;
    case CSR_STVAL:
        val = sextVA(val);
        csr_stval[current_thread] = val;
        break;
    case CSR_SIP:
        // Only ssip is writeable, and only if it is delegated
        msk = csr_mideleg[current_thread] & 0x0000000000000002ULL;
        val = (csr_mip[current_thread] & ~msk) | (val & msk);
        csr_mip[current_thread] = val;
        break;
    case CSR_SATP: // Shared register
        // MODE is 4 bits, ASID is 0bits, PPN is PPN_M bits
        val &= 0xF000000000000000ULL | PPN_M;
        switch (val >> 60)
        {
        case SATP_MODE_BARE:
        case SATP_MODE_SV39:
        case SATP_MODE_SV48:
            csr_satp[current_thread] = val;
            csr_satp[current_thread^1] = val;
            break;
        default: // reserved
            // do not write the register if attempting to set an unsupported mode
            break;
        }
        break;
    case CSR_MSTATUS:
        // Preserve sd, sxl, uxl, xs
        val = (val & 0x00000000007E79BBULL) | (csr_mstatus[current_thread] & 0x8000000F00018000ULL);
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
        {
            val |= 0x8000000000000000ULL;
        }
        csr_mstatus[current_thread] = val;
        break;
    case CSR_MISA:
        // misa is a 0-length register, cannot be modified
        break;
    case CSR_MEDELEG:
        // Not all exceptions can be delegated
        val &= 0x0000000000000B109ULL;
        csr_medeleg[current_thread] = val;
        break;
    case CSR_MIDELEG:
        // Not all interrupts can be delegated
        val &= 0x0000000000000222ULL;
        csr_mideleg[current_thread] = val;
        break;
    case CSR_MIE:
        // Hard-wire ueie, utie, usie
        val &= 0x0000000000088AAAULL;
        csr_mie[current_thread] = val;
        break;
    case CSR_MTVEC:
        val = sextVA(val & ~0xFFEULL);
        csr_mtvec[current_thread] = val;
        mtvec_is_set[current_thread] = true;
        break;
    case CSR_MCOUNTEREN:
        val &= 0xffffffff;
        csr_mcounteren[current_thread] = val;
        break;
    case CSR_MSCRATCH:
        csr_mscratch[current_thread] = val;
        break;
    case CSR_MEPC:
        // mepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        csr_mepc[current_thread] = val;
        break;
    case CSR_MCAUSE:
        // Maks all bits excepts the ones we care about
        val &= 0x800000000000001FULL;
        csr_mcause[current_thread] = val;
        break;
    case CSR_MTVAL:
        val = sextVA(val);
        csr_mtval[current_thread] = val;
        break;
    case CSR_MIP:
        // Only seip, stip, ssip are writeable
        val &= 0x0000000000000222ULL;
        csr_mip[current_thread] = val;
        break;
    case CSR_TSELECT:
        break;
    case CSR_TDATA1:
        if ((~val & 0x0800000000000000ull) || debug_mode[current_thread])
        {
            // Preserve type, maskmax, timing
            val = (val & 0x08000000000010DFULL) | (csr_tdata1[current_thread] & 0xF7E0000000040000ULL);
            csr_tdata1[current_thread] = val;
            activate_breakpoints(prvget());
        }
        break;
    case CSR_TDATA2:
        // keep only valid virtual or pysical addresses
        val &= VA_M;
        csr_tdata2[current_thread] = val;
        break;
    case CSR_TDATA3:
        break;
    // DCSR
    // DPC
    // DSCRATCH
    case CSR_MCYCLE:
    case CSR_MINSTRET:
        break;
    case CSR_CYCLE:
    case CSR_INSTRET:
    case CSR_MVENDORID:
    case CSR_MARCHID:
    case CSR_MIMPID:
    case CSR_MHARTID:
        throw trap_illegal_instruction(current_inst);
    // ----- Esperanto registers -------------------------------------
    // TODO: MATP
    case CSR_MINSTMASK:
        val &= 0x1ffffffffULL;
        csr_minstmask[current_thread] = val;
        break;
    case CSR_MINSTMATCH:
        val &= 0xffffffff;
        csr_minstmatch[current_thread] = val;
        break;
    // TODO: CSR_AMOFENCE_CTRL
    case CSR_CACHE_INVALIDATE:
        break;
    case CSR_MENABLE_SHADOWS:
        val &= 1;
        csr_menable_shadows[current_thread] = val;
        csr_menable_shadows[current_thread^1] = val;
        break;
    case CSR_EXCL_MODE:
        val &= 1;
        if (val)
        {
            csr_excl_mode[current_thread] = 1 + ((current_thread & 1) << 1);
            csr_excl_mode[current_thread^1] = 1 + ((current_thread & 1) << 1);
        }
        else
        {
            csr_excl_mode[current_thread] = 0;
            csr_excl_mode[current_thread^1] = 0;
        }
        break;
    case CSR_MCACHE_CONTROL:
        msk = (csr_mcache_control[current_thread] & 1) ? 3 : 1;
        val = (val & msk) | (csr_ucache_control[current_thread] & ~msk);
        if ((val & 3) != 2)
        {
            csr_ucache_control[current_thread] = val;
            csr_ucache_control[current_thread^1] = val;
            csr_mcache_control[current_thread] = val & 3;
            csr_mcache_control[current_thread^1] = val & 3;
            num_sets = (val & 0x1) ? 4 : 16;
        }
        val &= 3;
        break;
    case CSR_EVICT_SW:
    case CSR_FLUSH_SW:
        {
            bool tm    = (val >> 63) & 0x1;
            int  dest  = (val >> 58) & 0x3;
            int  set   = (val >> 14) & 0xF;
            int  way   = (val >>  6) & 0x3;
            int  count = (val & 0xF) + 1;
            dcache_evict_flush_set_way(src1 == CSR_EVICT_SW, tm, dest, set, way, count);
        }
        break;
    case CSR_LOCK_SW:
        {
            int      way   = (val >> 55) & 0x3;
            uint64_t paddr = val & 0x000000FFFFFFFFC0ULL;
            dcache_lock_paddr(way, paddr);
        }
        break;
    case CSR_UNLOCK_SW:
        {
            int way = (val >> 55) & 0x3;
            int set = (val >>  6) & 0xF;
            dcache_unlock_set_way(set, way);
        }
        break;
    case CSR_TENSOR_REDUCE:
        require_feature_ml_on_thread0();
        tensorreduce(val);
        break;
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        switch ((val >> 1) & 0x7)
        {
        case 0: tensor_fma32(val); break;
        case 1: tensor_fma16a32(val); break;
        case 3: tensor_ima8a32(val); break;
        default: throw trap_illegal_instruction(current_inst); break;
        }
        break;
    case CSR_TENSOR_CONV_SIZE:
        require_feature_ml();
        val &= 0xFF00FFFFFF00FFFFULL;
        csr_tensor_conv_size[current_thread] = val;
        tmask_conv();
        break;
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val &= 0x0000FFFF0000FFFFULL;
        csr_tensor_conv_ctrl[current_thread] = val;
        tmask_conv();
        break;
    case CSR_TENSOR_COOP:
        require_feature_ml_on_thread0();
        val &= 0x0000000000FFFF0FULL;
        tcoop(val);
        break;
    case CSR_TENSOR_MASK:
        require_feature_ml();
        val &= 0xffff;
        csr_tensor_mask[current_thread] = val;
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        tensorquant(val);
        break;
    case CSR_TEX_SEND:
        require_feature_gfx();
        //val &= 0xff;
        // Notify to TBOX that a Sample Request is ready
        // Thanks for making the code unreadable
        new_sample_request(current_thread,
                           val & 0xf,           // port_id
                           (val >> 4) & 0xf,    // num_packets
                           read_port_base_address(current_thread, val & 0xf /* port id */));
        break;
    case CSR_TENSOR_ERROR:
        require_feature_ml();
        val &= 0x1ff;
        csr_tensor_error[current_thread] = val;
        log_tensor_error_value(val);
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        msk = (!(current_thread % EMU_THREADS_PER_MINION)
               && (csr_mcache_control[current_thread] & 1)) ? 1 : 3;
        val = (csr_mcache_control[current_thread] & msk) | (val & ~msk & 0x07df);
        assert((val & 3) != 2);
        csr_ucache_control[current_thread] = val;
        csr_ucache_control[current_thread^1] = val;
        csr_mcache_control[current_thread] = val & 3;
        csr_mcache_control[current_thread^1] = val & 3;
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        {
            bool tm         = (val >> 63) & 0x1;
            int  dest       = (val >> 58) & 0x3;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int  count      = (val & 0xF) + 1;
            uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
            int      id     = XREGS[31] & 0x0000000000000001ULL;
            dcache_prefetch_vaddr(tm, dest, vaddr, count, id, stride);
        }
        break;
    // FLB0
    case CSR_FCC:
        require_feature_ml();
        fcc_cnt = val & 0x01;
#ifdef SYS_EMU
        // If you are not going to block decrement it
        if (fcc[current_thread][fcc_cnt] != 0)
            fcc[current_thread][fcc_cnt]--;
#else
        // block if no credits, else decrement
        if (fcc[current_thread][fcc_cnt] == 0 ) {
            fcc_wait[current_thread] = true;
            throw std::domain_error("FCC write with no credits");
        } else {
            fcc[current_thread][fcc_cnt]--;
        }
#endif
        break;
    case CSR_STALL:
        require_feature_ml();
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_WAIT:
        require_feature_ml();
        log_tensor_error_value(csr_tensor_error[current_thread]);
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        try
        {
            tensorload(val);
        }
        catch (const trap_illegal_instruction&)
        {
            throw;
        }
        catch (const trap_t&)
        {
            update_tensor_error(1 << 7);
        }
        break;
    case CSR_GSC_PROGRESS:
        val &= (VL-1);
        csr_gsc_progress[current_thread] = val;
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        try
        {
            tensorloadl2(val);
        }
        catch (const trap_t&)
        {
            update_tensor_error(1 << 7);
        }
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        try
        {
            tensorstore(val);
        }
        catch (const trap_illegal_instruction&)
        {
            throw;
        }
        catch (const trap_t&)
        {
            update_tensor_error(1 << 7);
        }
        break;
    case CSR_EVICT_VA:
    case CSR_FLUSH_VA:
        require_feature_u_cacheops();
        {
            bool     tm     = (val >> 63) & 0x1;
            int      dest   = (val >> 58) & 0x3;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0x0F) + 1;
            uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
            int      id     = XREGS[31] & 0x0000000000000001ULL;
            dcache_evict_flush_vaddr(src1 == CSR_EVICT_VA, tm, dest, vaddr, count, id, stride);
        }
        break;
    case CSR_VALIDATION0:
        csr_validation0[current_thread] = val;
        break;
    case CSR_VALIDATION1:
        // EOT signals end of test
        if ((char) val == 4)
        {
            LOG(INFO, "%s", "Validation1 CSR received End Of Transmission.");
            m_emu_done = true;
            break;
        }
        if ((char) val != '\n')
        {
            uart_stream[current_thread] << (char) val;
        }
        else
        {
            // If line feed, flush to stdout
            std::cout << uart_stream[current_thread].str() << std::endl;
            uart_stream[current_thread].str("");
            uart_stream[current_thread].clear();
        }
        break;
    case CSR_VALIDATION2:
        csr_validation2[current_thread] = val;
        break;
    case CSR_VALIDATION3:
        csr_validation3[current_thread] = val;
        break;
    case CSR_LOCK_VA:
        require_lock_unlock_enabled();
        {
            bool     tm     = (val >> 63) & 0x1;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0xF) + 1;
            uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
            int      id     = XREGS[31] & 0x0000000000000001ULL;
            dcache_lock_vaddr(tm, vaddr, count, id, stride);
        }
        break;
    case CSR_UNLOCK_VA:
        require_lock_unlock_enabled();
        val &= 0xC000FFFFFFFFFFCFULL;
        {
            bool     tm     = (val >> 63) & 0x1;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0xF) + 1;
            uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
            int      id     = XREGS[31] & 0x0000000000000001ULL;
            dcache_unlock_vaddr(tm, vaddr, count, id, stride);
        }
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val &= 0x00000000030F0FF3ULL;
        val |= 0x0000000000008000ULL;
        csr_portctrl[src1 - CSR_PORTCTRL0][current_thread] = val;
        configure_port(src1 - CSR_PORTCTRL0, val);
        break;
    case CSR_FCCNB:
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
    case CSR_HARTID:
        throw trap_illegal_instruction(current_inst);
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        csr_mhpmevent[src1 - CSR_MHPMEVENT3][current_thread] = val;
        break;
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
        // Waived registers. The value is taken from the RTL
        break;
    // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }
}

static void csr_insn(xreg dst, uint16_t src1, uint64_t oldval, uint64_t newval, bool write)
{
    // Check if current privilege mode has access to the register
    int prv = prvget();
    int csrprv = (src1 >> 8) & 3;
    if (csrprv > prv)
    {
        LOG(DEBUG, "Accessing a %c-mode CSR while in %c-mode", "USHM"[csrprv], "USHM"[prv]);
        throw trap_illegal_instruction(current_inst);
    }
    if ((src1 == CSR_SATP) && ((csr_mstatus[current_thread] >> 20) & 1) && (prv == CSR_PRV_S))
    {
        LOG(DEBUG, "Accessing SATP while in %c-mode and mstatus.tvm = %d (mstatus = 0x%016" PRIx64 ")",
            "USHM"[prv], int((csr_mstatus[current_thread] >> 20) & 1), csr_mstatus[current_thread]);
        throw trap_illegal_instruction(current_inst);
    }
    if (write)
    {
        switch (src1)
        {
            // Fast local barrier instructions encoded in the CSR space
            case CSR_FLB:
                require_feature_ml();
                oldval = flbarrier(newval);
                break;
            default:
                csrset(src1, newval);
                break;
        }
        LOG(DEBUG, "\t%s = 0x%" PRIx64, csr_name(src1), newval);
    }

    // the return value of mip.ssip should be set if external supervisor
    // interrupts are pending, but the RMO part of the csrrw/s/c instruction
    // should not take this into account
    switch (src1)
    {
        case CSR_SIP:
            oldval |= ext_seip[current_thread] & csr_mideleg[current_thread];
            break;
        case CSR_MIP:
            oldval |= ext_seip[current_thread];
            break;
        default:
            break;
    }
    WRITE_REG(dst, oldval);
}

void ecall(const char* comm __attribute__((unused)))
{
    DISASM_NOARG("ecall");
    switch (prvget())
    {
        case CSR_PRV_U: throw trap_user_ecall(); break;
        case CSR_PRV_S: throw trap_supervisor_ecall(); break;
        case CSR_PRV_M: throw trap_machine_ecall(); break;
        default       : assert(0); break;
    }
}

void ebreak(const char* comm __attribute__((unused)))
{
    DISASM_NOARG("ebreak");
    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(current_pc);
}

void sret(const char* comm __attribute__((unused)))
{
    uint64_t curprv = prvget();
    uint64_t mstatus = csr_mstatus[current_thread];
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 22) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    DISASM_NOARG("sret");
    log_pc_update(csr_sepc[current_thread]);
    // Take spie and spp
    uint64_t spie = (mstatus >> 5) & 0x1;
    uint64_t spp = (mstatus >> 8) & 0x1;
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set sie = spie, spie = 1, spp = U (0), prv = spp
    csrset(CSR_MSTATUS, mstatus_clean | (spie << 1) | (1 << 5));
    prvset(spp);
    LOG(DEBUG, "Now running in %s mode", (spp == CSR_PRV_M) ? "M" : (spp == CSR_PRV_S) ? "S" : "U");
}

void mret(const char* comm __attribute__((unused)))
{
    if (prvget() != CSR_PRV_M)
      throw trap_illegal_instruction(current_inst);

    DISASM_NOARG("mret");
    log_pc_update(csr_mepc[current_thread]);
    // Take mpie and mpp
    uint64_t mstatus = csr_mstatus[current_thread];
    uint64_t mpie = (mstatus >> 7) & 0x1;
    uint64_t mpp = (mstatus >> 11) & 0x3;
    // Clean mie, mpie and mpp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFE777ULL;
    // Set mie = mpie, mpie = 1, mpp = U (0), prv = mpp
    csrset(CSR_MSTATUS, mstatus_clean | (mpie << 3) | (1 << 7));
    prvset(mpp);
    LOG(DEBUG, "Now running in %s mode", (mpp == CSR_PRV_M) ? "M" : (mpp == CSR_PRV_S) ? "S" : "U");
}

void wfi(const char* comm __attribute__((unused)))
{
    uint64_t curprv = prvget();
    uint64_t mstatus = csr_mstatus[current_thread];
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 21) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    DISASM_NOARG("wfi");
}

void sfence_vma(xreg src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): sfence.vma x%d, x%d", PRVNAME, src1, src2);
    throw trap_mcode_instruction(current_inst);
}

void csrrw(xreg dst, uint16_t src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrw x%d, %s, x%d", PRVNAME, dst, csr_name(src1), src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = XREGS[src2];
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrs(xreg dst, uint16_t src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrs x%d, %s, x%d", PRVNAME, dst, csr_name(src1), src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | XREGS[src2];
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrc(xreg dst, uint16_t src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrc x%d, %s, x%d", PRVNAME, dst, csr_name(src1), src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~XREGS[src2]);
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrwi(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrwi x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = imm;
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrsi(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrsi x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | imm;
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

void csrrci(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrci x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~imm);
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto cache control extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Scratchpad emulation --------------------------------------------------

// True if a cacheline is locked
static bool scp_locked[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];

// Which PA a locked cacheline is mapped to
static uint64_t scp_trans[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];

// ----- CacheOp emulation -----------------------------------------------------

static void dcache_evict_flush_set_way(bool evict, bool tm, int dest, int set, int way, int numlines)
{
    // Skip all if dest is L1, or if set is outside the cache limits
    if ((dest == 0) || (set >= L1D_NUM_SETS))
        return;

    for (int i = 0; i < numlines; i++)
    {
        // skip if masked or evicting and hard-locked
        if ((!tm || tmask_pass(i)) && !(evict && scp_locked[current_thread>>1][set][way]))
        {
            // NB: Hardware sets TensorError[7] if the PA in the set/way
            // corresponds to L2SCP and dest > L2, but we do not keep track of
            // unlocked cache lines.
            if ((dest >= 2) && scp_locked[current_thread>>1][set][way] && paddr_is_scratchpad(scp_trans[current_thread>>1][set][way]))
            {
                LOG(DEBUG, "\tFlushSW: Set: %d, Way: %d, DestLevel: %d cannot flush L2 scratchpad address 0x%016" PRIx64,
                    set, way, dest, scp_trans[current_thread>>1][set][way]);
                update_tensor_error(1 << 7);
                return;
            }
            LOG(DEBUG, "\tDoing %s: Set: %d, Way: %d, DestLevel: %d",
                evict ? "EvictSW" : "FlushSW", set, way, dest);
        }
        // Increment set and way with wrap-around
        if (++set >= L1D_NUM_SETS)
        {
            if (++way >= L1D_NUM_WAYS)
            {
                way = 0;
            }
            set = 0;
        }
    }
}

static void dcache_evict_flush_vaddr(bool evict, bool tm, int dest, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    (void)(id);

    // Skip all if dest is L1
    if (dest == 0)
        return;

    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp);
        }
        catch (const trap_t& t)
        {
            LOG(DEBUG, "\t%s: %016" PRIx64 ", DestLevel: %01x generated exception (suppressed)",
                evict ? "EvictVA" : "FlushVA", vaddr, dest);
            update_tensor_error(1 << 7);
            return;
        }
        LOG(DEBUG, "\tDoing %s: %016" PRIx64 " (%016" PRIx64 "), DestLevel: %01x",
            evict ? "EvictVA" : "FlushVA", vaddr, paddr, dest);
    }
}

static void dcache_prefetch_vaddr(bool tm, int dest, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    // Skip all if dest is MEM
    if (dest == 3)
        return;

    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_Prefetch);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tPrefetchVA: %016" PRIx64 ", DestLevel: %01x generated exception (suppressed)", vaddr, dest);
            update_tensor_error(1 << 7);
            return;
        }
        LOG(DEBUG, "\tDoing PrefetchVA: %016" PRIx64 " (%016" PRIx64 "), DestLevel: %01x", vaddr, paddr, dest);
    }
}

static void dcache_lock_paddr(int way, uint64_t paddr)
{
    if (!mmu_check_cacheop_access(paddr))
    {
        LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(1 << 7);
        return;
    }

    // FIXME: This should take mcache_control into account!!!
    int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;

    // Check if paddr already locked in the cache
    int nlocked = 0;
    for (int w = 0; w < L1D_NUM_WAYS; ++w)
    {
        if (scp_locked[current_thread >> 1][set][w])
        {
            ++nlocked;
            if ((w == way) || (scp_trans[current_thread >> 1][set][w] == paddr))
            {
                // Requested PA already locked in a different way, or requested
                // way already locked with a different PA; stop the operation.
                // NB: Hardware sets TensorError[5] also when the PA is
                // in the L1 cache on a different set/way but we do not keep
                // track of unlocked cache lines.
                LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d double-locking on way %d (addr: 0x%016" PRIx64 ")",
                    paddr, way, w, scp_trans[current_thread >> 1][set][w]);
                update_tensor_error(1 << 5);
                return;
            }
        }
    }

    // Cannot lock any more lines in this set; stop the operation
    if (nlocked >= (L1D_NUM_WAYS-1))
    {
        update_tensor_error(1 << 5);
        return;
    }

    scp_locked[current_thread >> 1][set][way] = true;
    scp_trans[current_thread >> 1][set][way] = paddr;
    for (uint64_t addr = paddr; addr < paddr + L1D_LINE_SIZE; addr += 8)
    {
        pmemwrite64(addr, 0);
        uint64_t value = 0;
        LOG_MEMWRITE(64, addr, value);
    }
    LOG(DEBUG, "\tDoing LockSW: (%016" PRIx64 "), Way: %d, Set: %d", paddr, way, set);
}

static void dcache_unlock_set_way(int set, int way)
{
    if ((set < L1D_NUM_SETS) && (way < L1D_NUM_WAYS))
    {
        scp_locked[current_thread >> 1][set][way] = false;
    }
}

static void dcache_lock_vaddr(bool tm, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_Store);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tLockVA 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }

        // LockVA is a hint, so no need to model soft-locking of the cache.
        // We just need to make sure we zero the cache line.
        for (uint64_t addr = paddr; addr < paddr + L1D_LINE_SIZE; addr += 8)
        {
            pmemwrite64(addr, 0);
            uint64_t value = 0;
            LOG_MEMWRITE(64, addr, value);
        }
        LOG(DEBUG, "\tDoing LockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
    }
}

static void dcache_unlock_vaddr(bool tm, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_Store);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tUnlockVA: 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }
        // Soft-locking of the cache is not modeled, so there is nothing more
        // to do here.
        LOG(DEBUG, "\tDoing UnlockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto messaging extension emulation
//
////////////////////////////////////////////////////////////////////////////////

bool get_msg_port_stall(uint32_t thread, uint32_t id)
{
    return msg_ports[thread][id].stall;
}

bool msg_port_empty(uint32_t thread, uint32_t id)
{
    return msg_ports[thread][id].size == 0;
}

bool msg_port_full(uint32_t thread, uint32_t id)
{
    return msg_ports[thread][id].size == (msg_ports[thread][id].max_msgs + 1);
}

uint64_t read_port_base_address(unsigned thread, unsigned id)
{
    return scp_trans[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way];
}

void set_delayed_msg_port_write(bool f)
{
    msg_port_delayed_write = f;
}

static void write_msg_port_data_to_scp(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob)
{
    // Drop the write if port not configured
    if(!msg_ports[thread][id].enabled) return;

    if ( !scp_locked[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way] ) {
        LOG(DEBUG, "PORT_WRITE Port cache line (s%d w%d)  unlocked!", msg_ports[thread][id].scp_set, msg_ports[thread][id].scp_way);
    }
    uint64_t base_addr = scp_trans[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way];
    base_addr += msg_ports[thread][id].wr_ptr << msg_ports[thread][id].logsize;

    msg_ports[thread][id].stall  = false;

    int wr_words = (1 << (msg_ports[thread][id].logsize))/4;

    LOG_ALL_MINIONS(DEBUG, "Writing MSG_PORT (m%d p%d) wr_words %d, logsize %d",  thread, id, wr_words, msg_ports[thread][id].logsize);
    for (int i = 0; i < wr_words; i++)
    {
        LOG_ALL_MINIONS(DEBUG, "Writing MSG_PORT (m%d p%d) data 0x%08" PRIx32 " to addr 0x%016" PRIx64,  thread, id, data[i], base_addr + 4 * i);
        pmemwrite32(base_addr + 4 * i, data[i]);
    }

    msg_ports[thread][id].size++;
    msg_ports[thread][id].wr_ptr = (msg_ports[thread][id].wr_ptr + 1) % (msg_ports[thread][id].max_msgs + 1);

    if (msg_ports[thread][id].enable_oob)
        msg_ports_oob[thread][id].push_back(oob);

    msg_to_thread(thread);
}

void write_msg_port_data(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = current_thread;
        port_write.target_thread = thread;
        port_write.target_port   = id;
        port_write.is_tbox       = false;
        port_write.is_rbox       = false;

        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize)/4; b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_ALL_MINIONS(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from m%d", thread, id, current_thread);
        int wr_words = 1 << (msg_ports[thread][id].logsize)/4;
        for(int i = 0 ; i < wr_words ; ++i)
            LOG_ALL_MINIONS(DEBUG, "                              data[%d] 0x%08" PRIx32, i, data[i]);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void write_msg_port_data_from_tbox(uint32_t thread, uint32_t id, uint32_t tbox_id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = tbox_id;
        port_write.target_thread = thread;
        port_write.target_port   = id;
        port_write.is_tbox       = true;
        port_write.is_rbox       = false;
        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize)/4; b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from tbox%d", thread, id, tbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void write_msg_port_data_from_rbox(uint32_t thread, uint32_t id, uint32_t rbox_id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = rbox_id;
        port_write.target_thread = thread;
        port_write.target_port   = id;
        port_write.is_tbox       = false;
        port_write.is_rbox       = true;
        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize)/4; b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from rbox%d", thread, id, rbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void commit_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t source_thread)
{
    uint32_t shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %d is %ld", shire, msg_port_pending_writes[shire].size());

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); it++)
        {
            port_write = *it;
            if ((port_write.target_thread == target_thread) &&
                (port_write.target_port   == port_id)       &&
                (port_write.source_thread == source_thread) &&
                !(port_write.is_tbox || port_write.is_rbox))
            {
                found = true;
                msg_port_pending_writes[shire].erase(it);
                break;
            }
        }

        if (found)
        {
            LOG(DEBUG, "Commit write on MSG_PORT (h%d p%d) from h%d", target_thread, port_id, source_thread);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (h%d p%d) from h%d not found!!", target_thread, port_id, source_thread);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (h%d p%d) from h%d not found!!", target_thread, port_id, source_thread);
    }
}

void commit_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id)
{
    uint32_t shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %d is %ld", shire, msg_port_pending_writes[shire].size());

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); it++)
        {
            port_write = *it;
            if ((port_write.target_thread == target_thread) &&
                (port_write.target_port   == port_id)       &&
                (port_write.source_thread == tbox_id)       &&
                 port_write.is_tbox && !port_write.is_rbox)
            {
                found = true;
                msg_port_pending_writes[shire].erase(it);
                break;
            }
        }

        if (found)
        {
            LOG(DEBUG, "Commit write on MSG_PORT (m%d p%d) from tbox%d oob %d", target_thread, port_id, tbox_id, port_write.oob);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from tbox%d not found!!", target_thread, port_id, tbox_id);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from tbox%d not found!!", target_thread, port_id, tbox_id);
    }
}

void commit_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id)
{
    uint32_t shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %d is %ld", shire, msg_port_pending_writes[shire].size());

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); it++)
        {
            port_write = *it;
            if ((port_write.target_thread == target_thread) &&
                (port_write.target_port   == port_id)       &&
                (port_write.source_thread == rbox_id)       &&
                !port_write.is_tbox && port_write.is_rbox)
            {
                found = true;
                msg_port_pending_writes[shire].erase(it);
                break;
            }
        }

        if (found)
        {
            LOG(DEBUG, "Commit write on MSG_PORT (m%d p%d) from rbox%d", target_thread, port_id, rbox_id);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from rbox%d not found!!", target_thread, port_id, rbox_id);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from rbox%d not found!!", target_thread, port_id, rbox_id);
    }
}

static int64_t port_get(uint32_t id, bool block)
{
    if (((prvget() == CSR_PRV_U) && !msg_ports[current_thread][id].umode) || !msg_ports[current_thread][id].enabled)
    {
        throw trap_illegal_instruction(current_inst);
    }

    if (msg_port_empty(current_thread,id))
    {
        LOG(DEBUG, "Blocking MSG_PORT%s (m%d p%d) wr_ptr=%d, rd_ptr=%d", block ? "" : "NB", current_thread, id,
            msg_ports[current_thread][id].wr_ptr, msg_ports[current_thread][id].rd_ptr);

        if (!block)
            return -1;

#ifdef SYS_EMU
        // if in sysemu stop thread if no data for port.. comparing rd_ptr and wr_ptr
        LOG(DEBUG, "Stalling MSG_PORT (m%d p%d)", current_thread, id);
        msg_ports[current_thread][id].stall = true;
        return 0;
#endif
    }

    int32_t offset = msg_ports[current_thread][id].rd_ptr << msg_ports[current_thread][id].logsize;

    if (msg_ports[current_thread][id].enable_oob)
    {
        uint8_t oob = msg_ports_oob[current_thread][id].front();
        msg_ports_oob[current_thread][id].pop_front();
        offset|=oob;
    }

    if (++msg_ports[current_thread][id].rd_ptr > msg_ports[current_thread][id].max_msgs)
    {
        msg_ports[current_thread][id].rd_ptr = 0;
    }
    msg_ports[current_thread][id].size--;

    return offset;
}

static void configure_port(uint32_t id, uint64_t wdata)
{
    int scp_set = (wdata >> 16) & 0xFF;
    int scp_way = (wdata >> 24) & 0xFF;
    int logsize = (wdata >> 5)  & 0x07;

    if ((scp_set >= L1D_NUM_SETS) || (scp_way >= L1D_NUM_WAYS) ||
        (logsize < PORT_LOG2_MIN_SIZE) || (logsize > PORT_LOG2_MAX_SIZE))
    {
        throw trap_illegal_instruction(current_inst);
    }

    msg_ports[current_thread][id].enabled    = wdata & 0x1;
    msg_ports[current_thread][id].stall      = false;
    msg_ports[current_thread][id].umode      = (wdata >> 4)  & 0x1;
    msg_ports[current_thread][id].use_scp    = true;
    msg_ports[current_thread][id].enable_oob = (wdata >> 1)  & 0x1;
    msg_ports[current_thread][id].logsize    = logsize;
    msg_ports[current_thread][id].max_msgs   = (wdata >> 8)  & 0xF;
    msg_ports[current_thread][id].scp_set    = scp_set;
    msg_ports[current_thread][id].scp_way    = scp_way;
    msg_ports[current_thread][id].rd_ptr     = 0;
    msg_ports[current_thread][id].wr_ptr     = 0;
    msg_ports[current_thread][id].offset     = -1;

    //reset the monitor queue so we don't get incorrect oob if the user doesn't pull all msgs
    msg_ports_oob[current_thread][id].clear();
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto tensor extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- TensorConvolution emulation -------------------------------------------

// Update to the tensor Mask due a convolution CSR write
static void tmask_conv()
{
    uint16_t tmask_value = 0;

    // Get the sizes of the convolution
    uint64_t tconvsizereg = csr_tensor_conv_size[current_thread];
    int srow =   int8_t((tconvsizereg >> 56) & 0xFF);
    int nrow = uint16_t((tconvsizereg >> 32) & 0xFFFF);
    int scol =   int8_t((tconvsizereg >> 24) & 0xFF);
    int ncol = uint16_t((tconvsizereg >>  0) & 0xFFFF);

    // Get the positions of the convolution
    uint64_t tconvctrlreg = csr_tensor_conv_ctrl[current_thread];
    int rowstart = int16_t((tconvctrlreg >> 32) & 0xFFFF);
    int colstart = int16_t((tconvctrlreg >>  0) & 0xFFFF);

    for (int i = 0; i < 16; ++i, rowstart += srow, colstart += scol)
    {
        if ((rowstart >= 0) && (rowstart < nrow) && (colstart >= 0) && (colstart < ncol))
        {
            LOG(DEBUG, "TensorMask[%d] pass for row: %d, col: %d, nrow: %d, ncol %d",
                i, rowstart, colstart, nrow, ncol);
            tmask_value |= (1u << i);
        }
        else
        {
            LOG(DEBUG, "TensorMask[%d] skip for row: %d, col: %d, nrow: %d, ncol %d",
                i, rowstart, colstart, nrow, ncol);
        }
    }
    csr_tensor_mask[current_thread] = tmask_value;
}

static void tcoop(uint64_t value)
{
    uint8_t neigh_mask  = (value >> 16) & 0xF;
    uint8_t minion_mask = (value >>  8) & 0xFF;
    int     coop_id     = (value >>  0) & 0xFF;
    // TODO: implement functionality checking the addresses and tcoop of every use of Tensor Load
    LOG(DEBUG, "\tSetting Tensor Cooperation: coopneighmask=%02X, coopminmask=%02X, coopid=%d", neigh_mask, minion_mask, coop_id);
}

// ----- TensorLoad emulation --------------------------------------------------

void tensorload(uint64_t control)
{
    uint64_t stride  = XREGS[31] & 0xFFFFFFFFFFC0ULL;

    int      tm                 = (control >> 63) & 0x1;
    int      use_coop           = (control >> 62) & 0x1;
    int      trans              = (control >> 59) & 0x7;
    int      dst                = (control >> 53) & 0x3F;
    int      tenb               = (control >> 52) & 0x1;
    //uint64_t virtual_addr_l2_sc = (control >>  6) & 0x3FFFFFFFFFF;
    uint64_t base               = control & 0xFFFFFFFFFFC0ULL;
    int      boffset            = (control >>  4) & 0x03;
    int      rows               = ((control      ) & 0xF) + 1;
    int      adj                = 0;

    uint64_t addr             = sext<48>(base);
    LOG(DEBUG, "Tensor Load: Trans:%d - rows:%d - tm:%d - use_coop:%d - dst:%d - tenb:%d - boffset:%d - addr:0x%016" PRIx64, trans, rows, tm, use_coop, dst, tenb, boffset, addr);

    // Cooperative tensor loads require the shire to be in cooperative mode
    if (use_coop)
    {
        uint64_t shire = current_thread / EMU_THREADS_PER_SHIRE;
        if (!esr_shire_coop_mode[shire])
            throw trap_illegal_instruction(current_inst);
    }

    // Check if SCP is enabled
    if (csr_mcache_control[current_thread] != 0x3)
    {
        LOG(DEBUG, "%s", "Tensor_Error TensorLoad with SCP disabled!!");
        update_tensor_error(1 << 4);
        return;
    }

    // Check if transform is valid
    if (trans == 0x3 || trans == 0x4)
    {
        LOG(DEBUG, "%s", "Tensor_Error TensorLoad with illegal transform!!");
        update_tensor_error(1 << 1);
        return;
    }

    // In case of loading data straight to tenb, we fake it by writing at position 64 and forth (not accessible otherwise)
    if (tenb)
    {
        dst = 0;
        adj = L1_SCP_ENTRIES;
        tensorload_setupb_topair[current_thread] = true;
        tensorload_setupb_topair[current_thread^1] = true;
        tensorload_setupb_numlines[current_thread] = rows;
        tensorload_setupb_numlines[current_thread^1] = rows;
    }

    log_tensor_load(trans, tenb, adj + (dst % L1_SCP_ENTRIES) , rows, tm ? csr_tensor_mask[current_thread] : 0xFFFF);

    //NO TRANS
    if (trans == 0x00)
    {
        LOG(DEBUG, "%s", "TensorLoad: No transformation");
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                uint64_t paddr = vmemtranslate(addr, L1D_LINE_SIZE, Mem_Access_TxLoad);
                for (int j = 0; j < L1D_LINE_SIZE/4; j++)
                {
                    SCP[idx].u32[j] = pmemread32(paddr + j*4);
                    LOG(DEBUG, "\tSCP[%d].u32[%d] = 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]" PRIx32, idx, j, SCP[idx].u32[j], addr+j*4);
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
            }
            LOG(DEBUG, "\t\tAddress = 0x%016" PRIx64 " - Stride = 0x%016" PRIx64, addr, stride);
            addr = sextVA(addr + stride);
        }
    }
    //INTERLEAVE8
    else if (trans == 0x01)
    {
        LOG(DEBUG, "%s", "TensorLoad: Interleave");
        boffset *= 16;
        LOG(DEBUG, "#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                for (int r = 0; r < 4; ++r)
                {
                    uint64_t vaddr = sextVA(addr + boffset + (4*i+r)*stride);
                    assert(addr_is_size_aligned(vaddr, 16));
                    uint64_t paddr = vmemtranslate(vaddr, 16, Mem_Access_TxLoad);
                    for (int c = 0; c < 16; ++c)
                    {
                        SCP[idx].u8[c*4 + r] = pmemread8(paddr + c);
                        LOG(DEBUG, "SCP[%d].u8[%d] = 0x%02" PRIx8 " <-- MEM8[0x%016" PRIx64 "]", idx, c*4+r, SCP[idx].u8[c*4+r], vaddr + c);
                    }
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
            }
        }
    }
    //INTERLEAVE16
    else if (trans == 0x02)
    {
        LOG(DEBUG, "%s", "TensorLoad: Interleave");
        boffset = (boffset & 0x2) * 16;
        LOG(DEBUG, "#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                for (int r = 0; r < 2; ++r)
                {
                    uint64_t vaddr = sextVA(addr + boffset + (2*i+r)*stride);
                    assert(addr_is_size_aligned(vaddr, 32));
                    uint64_t paddr = vmemtranslate(vaddr, 32, Mem_Access_TxLoad);
                    for (int c = 0; c < 16; ++c)
                    {
                        SCP[idx].u16[c*2 + r] = pmemread16(paddr + c*2);
                        LOG(DEBUG, "SCP[%d].u16[%d] = 0x%04" PRIx16 " <-- MEM16[0x%016" PRIx64 "]",
                            idx, c*2+r, SCP[idx].u16[c*4+r], vaddr + c*2);
                    }
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
            }
        }
    }
    //TRANSPOSE
    else if (trans == 0x05 || trans == 0x06 || trans==0x07)
    {
        bool exist_conv = 0;
        for (int i=0; (i<rows) & (!exist_conv);++i)
        {
            exist_conv = tmask_pass(i);
        }
        if (tm && !exist_conv)
        {
            LOG(DEBUG, "%s", "Exit Condition Broken");
            return;
        }
        uint8_t tmp_buffer[64][L1D_LINE_SIZE];
        int size = (trans & 0x03);
        int offset = (trans == 0x7) ? 0 : ((trans == 0x5) ? (boffset*16) : ((boffset & 0x2) * 16));
        int elements = L1D_LINE_SIZE >> (size-1);
        size = 1 << (size-1);
        LOG(DEBUG, "TensorLoad: Transpose - elements:%d size:%d offset:%d", elements, size, offset);
        for (int elem = 0; elem < elements; ++elem)
        {
            //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
            assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, L1D_LINE_SIZE, Mem_Access_TxLoad);
            for (int j = 0; j < L1D_LINE_SIZE; j++)
            {
                uint8_t val = pmemread8(paddr + j);
                tmp_buffer[elem][j] = val;
                LOG(DEBUG, "\tLoading into tmp_buffer - MEM8[0x%016" PRIx64 "]: Row%d-Elem%d <= 0x%02" PRIx8, addr+j, elem, j, val);
            }
            addr = sextVA(addr + stride);
        }
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                for (int j = 0; j < elements; ++j)
                {
                    if (size == 4)
                    {
                        SCP[idx].u8[j*4  ] = tmp_buffer[j][i*4+offset  ];
                        SCP[idx].u8[j*4+1] = tmp_buffer[j][i*4+offset+1];
                        SCP[idx].u8[j*4+2] = tmp_buffer[j][i*4+offset+2];
                        SCP[idx].u8[j*4+3] = tmp_buffer[j][i*4+offset+3];
                        LOG(DEBUG, "\tI'm size 4 - b[0]=0x%02" PRIx8 " b[1]=0x%02" PRIx8 " b[2]=0x%02" PRIx8 " b[3]=0x%02" PRIx8,
                            tmp_buffer[j][i*4+offset], tmp_buffer[j][i*4+offset+1], tmp_buffer[j][i*4+offset+2], tmp_buffer[j][i*4+offset+3]);
                    }
                    else if (size == 2)
                    {
                        SCP[idx].u8[j*2  ] = tmp_buffer[j][i*2+offset  ];
                        SCP[idx].u8[j*2+1] = tmp_buffer[j][i*2+offset+1];
                        LOG(DEBUG, "\tI'm size 2 - b[0]=0x%02" PRIx8 " b[1]=0x%02" PRIx8,
                            tmp_buffer[j][i*2+offset], tmp_buffer[j][i*2+offset+1]);
                    }
                    else if (size == 1)
                    {
                        SCP[idx].u8[j] = tmp_buffer[j][i+offset];
                        LOG(DEBUG, "\tI'm size 1 - b[0]=0x%02" PRIx8, tmp_buffer[j][i+offset]);
                    }
                    else
                    {
                        LOG(DEBUG, "%s", "ERROR Tensor Load element size not valid!!");
                        update_tensor_error(1 << 1);
                        return;
                    }
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
                for (int x = 0; x < L1D_LINE_SIZE/4; ++x)
                    LOG(DEBUG, "SCP[%d].u32[%d] = 0x%08" PRIx32, idx, x, SCP[idx].u32[x]);
            }
        }
    }
}

// ----- TensorLoadL2Scp emulation --------------------------------------------------

void tensorloadl2(uint64_t control)//TranstensorloadL2
{
    uint64_t stride  = XREGS[31] & 0xFFFFFFFFFFC0ULL;

    int      tm      = (control >> 63) & 0x1;
    int      dst     = ((control >> 46) & 0x1FFFC)  + ((control >> 4)  & 0x3);
    uint64_t base    = control & 0xFFFFFFFFFFC0ULL;
    int      rows    = ((control     ) & 0xF) + 1;
    uint64_t addr    = sext<48>(base);

    LOG(DEBUG, "TensorLoadL2SCP: rows:%d - tm:%d - dst:%d - addr:0x%16" PRIx64, rows, tm,  dst,  addr);

    uint64_t shire = current_thread / EMU_THREADS_PER_SHIRE;
    for (int i = 0; i < rows; ++i)
    {
        uint64_t l2scp_addr = L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * L1D_LINE_SIZE);
        if (!tm || tmask_pass(i))
        {
            assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, L1D_LINE_SIZE, Mem_Access_TxLoad);
            for (int j = 0; j < L1D_LINE_SIZE/4; j++)
            {
                uint32_t val = pmemread32(paddr + j*4);
                pmemwrite32(l2scp_addr + j*4, val);
                LOG(DEBUG, "\tTensorLoadL2SCP MEM32[0x%016" PRIx64 "] to PMEM32[0x%016" PRIx64 "] line %d, base 0x%016" PRIx64 " offset 0x%x <= 0x%08" PRIx32,
                    addr+j*4, l2scp_addr+j*4, dst+i, l2scp_addr, j*4, val);
            }
        }
        LOG(DEBUG, "\t\tVaddr = 0x%016" PRIx64 " Paddr = 0x%016" PRIx64 " - Stride = 0x%016" PRIx64 "- line %d", addr, l2scp_addr, stride, dst+i);
        addr = sextVA(addr + stride);
    }
}

// ----- TensorQuant emulation -------------------------------------------------

static const char* get_quant_transform(int op)
{
    static const char* trans_int_to_str[16] = {
        "LAST",
        "INT32_TO_FP32",
        "FP32_TO_INT32",
        "INT32_RELU",
        "INT32_ADD_ROW",
        "INT32_ADD_COL",
        "FP32_MUL_ROW",
        "FP32_MUL_COL",
        "SATINT8",
        "SATUINT8",
        "PACK_128B",
        "Reserved(11)",
        "Reserved(12)",
        "Reserved(13)",
        "Reserved(14)",
        "Reserved(15)"
    };
    return trans_int_to_str[op&15];
}

static void tensorquant(uint64_t value)
{
    unsigned fstart = (value >> 57) & 0x1F;
    unsigned cols   = (value >> 55) & 0x3;
    unsigned rows   = (value >> 51) & 0xF;
    unsigned line   = (value >> 45) & 0x3F;

    cols = (cols + 1) * 4;
    rows = rows + 1;
    line = line % L1_SCP_ENTRIES;

    set_rounding_mode(frm());

    LOG(DEBUG, "\tStart Tensor Quant with scratchpad: %u, rows: %u, cols: %u, regstart: %u", line, rows, cols, fstart);
    for (int k = 0; k < TQUANT_MAX_TRANS; k++)
    {
        int trans = (value >> (k*4)) & 0xF;
        LOG(DEBUG, "\t\tTransformation %d: %s", k, get_quant_transform(trans));

        switch (trans)
        {
            case 0: // NONE
                if (k)
                {
                    set_fp_exceptions();
                    dirty_fp_state();
                }
                return;
            case 1: // INT32_TO_FP32
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.f = fpu::i32_to_f32(val.i);
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                break;
            case 2: // FP32_TO_INT32
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = fpu::f32_to_i32(val.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                break;
            case 3: // INT32_RELU
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i < 0) ? 0 : val.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                break;
            case 4: // INT32_ADD_ROW
                if (csr_mcache_control[current_thread] != 0x3)
                {
                    update_tensor_error(1 << 4);
                    return;
                }
                log_tensor_quant_new_transform();
                for (unsigned j = 0; j < cols; j += VL)
                    LOG_SCP(":", line, j);
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, val2, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        val2.u = SCP[line].u32[j];
                        res.i = val1.i + val2.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                line = (line + 1) % L1_SCP_ENTRIES;
                break;
            case 5: // INT32_ADD_COL
                if (csr_mcache_control[current_thread] != 0x3)
                {
                    update_tensor_error(1 << 4);
                    return;
                }
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; i += VL)
                    LOG_SCP(":", line, i);
                for (unsigned i = 0; i < rows; ++i)
                {
                    iufval32 val2;
                    val2.u = SCP[line].u32[i];
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        res.i = val1.i + val2.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                line = (line + 1) % L1_SCP_ENTRIES;
                break;
            case 6: // FP32_MUL_ROW
                if (csr_mcache_control[current_thread] != 0x3)
                {
                    update_tensor_error(1 << 4);
                    return;
                }
                log_tensor_quant_new_transform();
                for (unsigned j = 0; j < cols; j += VL)
                    LOG_SCP(":", line, j);
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, val2, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        val2.u = SCP[line].u32[j];
                        res.f = fpu::f32_mul(val1.f, val2.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                line = (line + 1) % L1_SCP_ENTRIES;
                break;
            case 7: // FP32_MUL_COL
                if (csr_mcache_control[current_thread] != 0x3)
                {
                    update_tensor_error(1 << 4);
                    return;
                }
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; i += VL)
                    LOG_SCP(":", line, i);
                for (unsigned i = 0; i < rows; ++i)
                {
                    iufval32 val2;
                    val2.u = SCP[line].u32[i];
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        res.f = fpu::f32_mul(val1.f, val2.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                line = (line + 1) % L1_SCP_ENTRIES;
                break;
            case 8: // SATINT8
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i > 127 ? 127 : (val.i < -128 ? -128 : val.i)) & 0xFF;
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                break;
            case 9: // SATUINT8
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i > 255 ? 255 : (val.i < 0 ? 0 : val.i)) & 0xFF;
                        FREGS[freg].u32[j%VL] = res.u;
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG("=", (fstart + i*2 + j) % 32);
                }
                break;
            case 10: // PACK_128B
                // RTL operates on even registers first, and then on odd
                // registers, so it generates two writes to the destination
                // register when a row spans a vector.
                log_tensor_quant_new_transform(cols > VL);
                for (unsigned i = 0; i < rows; ++i)
                {
                    unsigned fdst = (fstart + i*2) % 32;
                    for (unsigned j = 0; j < cols/VL; j++)
                        LOG_FREG(":", (fstart + i*2 + j) % 32);
                    for (unsigned j = 0; j < cols; j += 4)
                    {
                        unsigned fsrc = (fstart + i*2 + j/VL) % 32;
                        uint32_t val0 = FREGS[fsrc].u32[(j+0)%VL];
                        uint32_t val1 = FREGS[fsrc].u32[(j+1)%VL];
                        uint32_t val2 = FREGS[fsrc].u32[(j+2)%VL];
                        uint32_t val3 = FREGS[fsrc].u32[(j+3)%VL];
                        FREGS[fdst].u8[j+0] = uint8_t(val0 & 0xFF);
                        FREGS[fdst].u8[j+1] = uint8_t(val1 & 0xFF);
                        FREGS[fdst].u8[j+2] = uint8_t(val2 & 0xFF);
                        FREGS[fdst].u8[j+3] = uint8_t(val3 & 0xFF);
                        log_tensor_quant_write(k, fdst, j/4, FREGS[fdst].u32[j/4]);
                    }
                    LOG_FREG("=", fdst);
                }
                break;
            default:
                throw std::runtime_error("Illegal TensorQuant transform!");
                break;
        }
    }

    // Executed TQUANT_MAX_TRANS transforms without early exit
    set_fp_exceptions();
    dirty_fp_state();
}

// ----- TensorStore emulation -------------------------------------------------

static void tensorstore(uint64_t tstorereg)
{
    uint64_t tstore_scp = (tstorereg >> 48) & 0x1;

    if (tstore_scp)
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to scratchpad source
        int      scpstart =  (tstorereg & 0x3F00000000000000ULL) >> 56;      // Start scratchpad entry to store
        int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
        uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFC0ULL);     // Address where to store the results

        uint64_t stride   = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;

        int src = scpstart % L1_SCP_ENTRIES;
        LOG(DEBUG, "\tStart Tensor Store Scp with addr: %016" PRIx64 ", stride: %016" PRIx64 ", rows: %d, scpstart: %d, srcinc: %d", addr, stride, rows, src, srcinc);

        // Check if L1 SCP is enabled
        if (csr_mcache_control[current_thread] != 0x3)
        {
            update_tensor_error(1 << 4);
            return;
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, L1D_LINE_SIZE, Mem_Access_TxStore);
            // For all the elements of the lane
            for (int i = 0; i < L1D_LINE_SIZE/4; i++)
            {
                uint32_t val = SCP[src].u32[i];
                LOG(DEBUG, "\tSCP[%d].u32[%d] = 0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", src, i, val, addr + i*4);
                pmemwrite32(paddr + i*4, val);
                //log_mem_write(0, 4, addr + i*4, val); => Don't log mem changes!
            }
            src = (src + srcinc) % L1_SCP_ENTRIES;
            addr = sextVA(addr + stride);
        }
    }
    else
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to register source
        int      regstart =  (tstorereg & 0x3E00000000000000ULL) >> 57;      // Start register to store
        int      cols     = ((tstorereg & 0x0180000000000000ULL) >> 55) + 1; // Number of register per col
        int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
        int      coop     = ((tstorereg & 0x0006000000000000ULL) >> 49) + 1; // Number of cooperative minions
        uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFF0ULL);     // Address where to store the results

        uint64_t stride   = XREGS[31] & 0x0000FFFFFFFFFFF0ULL;

        LOG(DEBUG, "\tStart Tensor Store with addr: %016" PRIx64 ", stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, srcinc: %d, coop: %d",
            addr, stride, regstart, rows, cols, srcinc, coop);

        int src = regstart;

        // Check legal coop combination
        // xs[50:49]/xs[56:55]
        static const bool coop_comb[4*4] = {
            true,  true,  false, true,
            true,  true,  false, false,
            false, false, false, false,
            true,  false, false, false
        };

        if (!coop_comb[4*(coop-1)+(cols-1)])
        {
            update_tensor_error(1 << 8);
            return;
        }

        // Cooperative tensor stores require the shire to be in cooperative mode
        if (coop > 1)
        {
            uint64_t shire = current_thread / EMU_THREADS_PER_SHIRE;
            if (!esr_shire_coop_mode[shire])
                throw trap_illegal_instruction(current_inst);
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            // For all the blocks of 128b
            for (int col = 0; col < cols; col++)
            {
                assert(addr_is_size_aligned(addr, 16));
                uint64_t paddr = vmemtranslate(addr + col*16, 16, Mem_Access_TxStore);
                // For all the 32 elements of the 128b block
                for (uint64_t i = 0; i < 4; i++)
                {
                    uint32_t idx = (col & 1) * 4 + i;
                    uint32_t val = FREGS[src].u32[idx];
                    LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", val, addr + col*16 + i*4);
                    pmemwrite32(paddr + i*4, val);
                    //log_mem_write(0, 4, addr + col*16 + i*4, val); => Don't log mem changes!
                }
                // For 128b stores, move to next desired register immediately.
                // For 256b and 512b stores, move to next desired register
                // when 256b are written
                if ((cols == 1) || (col & 1)) src = (src + srcinc) % NFREGS;
            }
            addr = sextVA(addr + stride);
        }
    }
}

// ----- TensorFMA emulation ---------------------------------------------------

static void tensor_fma32(uint64_t tfmareg)
{
    bool usemsk     = (tfmareg >> 63) & 0x1;
    int  bcols      = (tfmareg >> 55) & 0x3;
    int  arows      = (tfmareg >> 51) & 0xF;
    int  acols      = (tfmareg >> 47) & 0xF;
    int  aoffset    = (tfmareg >> 43) & 0xF;
    bool tenb       = (tfmareg >> 20) & 0x1;
    int  bstart     = (tfmareg >> 12) & 0xFF;
    int  astart     = (tfmareg >>  4) & 0xFF;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

    // Check if L1 SCP is enabled
    if (csr_mcache_control[current_thread] != 3)
    {
        update_tensor_error(1 << 4);
        return;
    }

    LOG(DEBUG, "\tStart TensorFMA32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(frm()));

    if (tenb && (!tensorload_setupb_topair[current_thread] ||
                 (tensorload_setupb_numlines[current_thread] != acols)))
    {
        // No TensorLoad to pair or incompatible combination of rows and columns length
        update_tensor_error(1 << 6);
        return;
    }

    // Unpair a paired TensorLoad
    tensorload_setupb_topair[current_thread] = false;
    tensorload_setupb_topair[current_thread^1] = false;

    set_rounding_mode(frm());

    for (int k = 0; k < acols; ++k)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES)];

        for (int i = 0; i < arows; ++i)
        {
            // Skip computation for this row
            if (usemsk && !tmask_pass(i))
            {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL] = 0;
                        log_tensor_fma_write(0, i*TFMA_REGS_PER_ROW+j/VL, j%VL, 0);
                    }
                }
                continue;
            }

            float32_t a = SCP[(astart+i) % L1_SCP_ENTRIES].f32[(aoffset+k) % (L1D_LINE_SIZE/4)];

            // If first_pass is 1 and this is the first iteration we do FMUL
            // instead of FMA
            if (first_pass && !k)
            {
                for (int j = 0; j < bcols; ++j)
                {
                    float32_t b = tmpb.f32[j];
                    float32_t c = fpu::f32_mul(a, b);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = fpu::UI32(c);
                    log_tensor_fma_write(k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    LOG(DEBUG, "\tTensorFMA32(%d) f%zu[%zu]: 0x%08" PRIx32 " = 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(a), fpu::UI32(b));
                }
            }
            else
            {
                // If the product will be 0, we can skip the operation
                if (fpu::UI32(a) == 0)
                    continue;

                for (int j = 0; j < bcols; ++j)
                {
                    float32_t b = tmpb.f32[j];
                    // If the product will be 0, we can skip the operation
                    if (fpu::UI32(b)==0)
                        continue;
                    float32_t c0 = FREGS[i*TFMA_REGS_PER_ROW+j/VL].f32[j%VL];
                    float32_t c = fpu::f32_mulAdd(a, b, c0);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = fpu::UI32(c);
                    log_tensor_fma_write(k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    LOG(DEBUG, "\tTensorFMA32(%d) f%zu[%zu]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(c0), fpu::UI32(a), fpu::UI32(b));
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%zu[%zu] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}

static void tensor_fma16a32(uint64_t tfmareg)
{
    bool usemsk     = (tfmareg >> 63) & 0x1;
    int  bcols      = (tfmareg >> 55) & 0x3;
    int  arows      = (tfmareg >> 51) & 0xF;
    int  acols      = (tfmareg >> 47) & 0xF;
    int  aoffset    = (tfmareg >> 43) & 0xF;
    bool tenb       = (tfmareg >> 20) & 0x1;
    int  bstart     = (tfmareg >> 12) & 0xFF;
    int  astart     = (tfmareg >>  4) & 0xFF;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 2;
    aoffset = aoffset * 2;

    // Check if L1 SCP is enabled
    if (csr_mcache_control[current_thread] != 3)
    {
        update_tensor_error(1 << 4);
        return;
    }

    LOG(DEBUG, "\tStart TensorFMA16A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(rtz));

    if (tenb && (!tensorload_setupb_topair[current_thread] ||
                 (tensorload_setupb_numlines[current_thread] != acols/2)))
    {
        // No TensorLoad to pair or incompatible combination of rows and columns length
        update_tensor_error(1 << 6);
        return;
    }

    // Unpair a paired TensorLoad
    tensorload_setupb_topair[current_thread] = false;
    tensorload_setupb_topair[current_thread^1] = false;

    set_rounding_mode(rtz);

    for (int k = 0; k < acols; k += 2)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES)];

        for (int i = 0; i < arows; ++i)
        {
            // Skip computation for this row
            if (usemsk && !tmask_pass(i))
            {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL] = 0;
                        log_tensor_fma_write(0, i*TFMA_REGS_PER_ROW+j/VL, j%VL, 0);
                    }
                }
                continue;
            }

            float16_t a1 = SCP[(astart+i) % L1_SCP_ENTRIES].f16[(aoffset+k+0) % (L1D_LINE_SIZE/2)];
            float16_t a2 = SCP[(astart+i) % L1_SCP_ENTRIES].f16[(aoffset+k+1) % (L1D_LINE_SIZE/2)];

            // If first_pass is 1 and this is the first iteration we do
            // a1*b1+a2*b2 instead of a1*b1+a2*b2+c0
            if (first_pass && !k)
            {
                for (int j = 0; j < bcols; ++j)
                {
                    float16_t b1 = tmpb.f16[2*j+0];
                    float16_t b2 = tmpb.f16[2*j+1];
                    float32_t c = fpu::f1632_mulAdd2(a1, b1, a2, b2);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = fpu::UI32(c);
                    log_tensor_fma_write(k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%zu[%zu]: 0x%08" PRIx32 " = (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                }
            }
            // If all products will be 0, we can skip the operation. NB: The detection
            // is done at 32-bit granularity, not at element (16-bit) granularity.
            else if ((fpu::UI16(a1) != 0) || (fpu::UI16(a2) != 0))
            {
                for (int j = 0; j < bcols; ++j)
                {
                    float16_t b1 = tmpb.f16[2*j+0];
                    float16_t b2 = tmpb.f16[2*j+1];
                    // If all products will be 0, we can skip the operation.
                    // NB: The detection is done at 32-bit granularity, not at
                    // element (16-bit) granularity.
                    if ((fpu::UI16(b1)==0) && (fpu::UI16(b2)==0))
                        continue;
                    float32_t c0 = FREGS[i*TFMA_REGS_PER_ROW+j/VL].f32[j%VL];
                    float32_t c = fpu::f1632_mulAdd3(a1, b1, a2, b2, c0);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = fpu::UI32(c);
                    log_tensor_fma_write(k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%zu[%zu]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(c0), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%zu[%zu] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}

static void tensor_ima8a32(uint64_t tfmareg)
{
    bool usemsk     = (tfmareg >> 63) & 0x1;
    int  bcols      = (tfmareg >> 55) & 0x3;
    int  arows      = (tfmareg >> 51) & 0xF;
    int  acols      = (tfmareg >> 47) & 0xF;
    int  aoffset    = (tfmareg >> 43) & 0xF;
    bool tenc2rf    = (tfmareg >> 23) & 0x1;
    bool ub         = (tfmareg >> 22) & 0x1;
    bool ua         = (tfmareg >> 21) & 0x1;
    bool tenb       = (tfmareg >> 20) & 0x1;
    int  bstart     = (tfmareg >> 12) & 0xFF;
    int  astart     = (tfmareg >>  4) & 0xFF;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 4;
    aoffset = aoffset * 4;

    // Check if L1 SCP is enabled
    if (csr_mcache_control[current_thread] != 3)
    {
        update_tensor_error(1 << 4);
        return;
    }

    LOG(DEBUG, "\tStart TensorIMA8A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc2rf: %d, tenb: %d, bstart: %d, astart: %d",
        usemsk, aoffset, first_pass, bcols, acols, arows, ub, ua, tenc2rf, tenb, bstart, astart);

    if (tenb && (!tensorload_setupb_topair[current_thread] ||
                 (tensorload_setupb_numlines[current_thread] != acols/4)))
    {
        // No TensorLoad to pair or incompatible combination of rows and columns length
        update_tensor_error(1 << 6);
        return;
    }

    // Unpair a paired TensorLoad
    tensorload_setupb_topair[current_thread] = false;
    tensorload_setupb_topair[current_thread^1] = false;

    if (first_pass)
    {
        for (int i = 0; i < arows; ++i)
        {
            for (int j = 0; j < bcols; ++j)
            {
                TENC[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = 0;
            }
        }
    }

    for (int k = 0; k < acols; k += 4)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES)];

        for (int i = 0; i < arows; ++i)
        {
            // We should skip computation for this row, but if tenc2rf is set,
            // and we are in the last pass then we must copy TenC to FREGS even
            // for this row.
            if (usemsk && !tmask_pass(i))
            {
                if (tenc2rf && (k+4 == acols))
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL] = TENC[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL];
                        LOG(DEBUG, "\tTensorIMA8A32(%d) f%zu[%zu] = 0x%08" PRIx32, k/4, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                        log_tensor_fma_write(k/4, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL]);
                    }
                }
                else if (first_pass && (k == 0))
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        log_tensor_fma_write(0, i*TFMA_REGS_PER_ROW+j/VL, j%VL, TENC[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    }
                }
                continue;
            }

            freg_t* dst = (tenc2rf && (k+4 == acols)) ? FREGS : TENC;
            const char* dname = (tenc2rf && (k+4 == acols)) ? "f" : "TenC";

            // If all products are 0, we can skip the operation, except if first_pass is set and this
            // is the first iteration, or TenC must be copied to FREGS and this is the last iteration.
            // NB: The detection is done at 32-bit granularity, not at element (8-bit) granularity.
            if (!(first_pass && (k == 0)) && !(tenc2rf && (k+4 == acols)) &&
                (SCP[(astart+i) % L1_SCP_ENTRIES].u32[((aoffset+k)/4) % (L1D_LINE_SIZE/4)] == 0))
                continue;

#define ASRC(x) SCP[(astart+i) % L1_SCP_ENTRIES].u8[(aoffset+k+(x)) % L1D_LINE_SIZE]
            int32_t a1 = ua ? ASRC(0) : sext8_2(ASRC(0));
            int32_t a2 = ua ? ASRC(1) : sext8_2(ASRC(1));
            int32_t a3 = ua ? ASRC(2) : sext8_2(ASRC(2));
            int32_t a4 = ua ? ASRC(3) : sext8_2(ASRC(3));
#undef ASRC

            for (int j = 0; j < bcols; ++j)
            {
#define BSRC(x) tmpb.u8[j*4+(x)]
                int32_t b1 = ub ? BSRC(0) : sext8_2(BSRC(0));
                int32_t b2 = ub ? BSRC(1) : sext8_2(BSRC(1));
                int32_t b3 = ub ? BSRC(2) : sext8_2(BSRC(2));
                int32_t b4 = ub ? BSRC(3) : sext8_2(BSRC(3));
#undef BSRC
                // If all products are 0, we can skip the operation, except if first_pass is set and this
                // is the first iteration, or TenC must be copied to FREGS and this is the last iteration
                // NB: The detection is done at 32-bit granularity, not at element (8-bit) granularity
                if (!(first_pass && (k == 0)) && !(tenc2rf && (k+4 == acols)) && (tmpb.u32[j] == 0))
                    continue;

                int32_t c0 = TENC[i*TFMA_REGS_PER_ROW+j/VL].i32[j%VL];
                int32_t c = c0 + (a1 * b1) + (a2 * b2) + (a3 * b3) + (a4 * b4);
                dst[i*TFMA_REGS_PER_ROW+j/VL].i32[j%VL] = c;
                log_tensor_fma_write(k/4, i*TFMA_REGS_PER_ROW+j/VL, j%VL, uint32_t(c));
                LOG(DEBUG, "\tTensorIMA8A32(%d) %s%zu[%zu]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ")",
                    k/4, dname, i*TFMA_REGS_PER_ROW+j/VL, j%VL, c, c0, uint8_t(a1), uint8_t(b1), uint8_t(a2), uint8_t(b2), uint8_t(a3), uint8_t(b3), uint8_t(a4), uint8_t(b4));
            }
        }
    }
    if (tenc2rf)
        dirty_fp_state();

    // logging
    for (int i = 0; i < arows; ++i)
    {
        const freg_t* dst = tenc2rf ? FREGS : TENC;
        const char* dname = tenc2rf ? "f" : "TenC";
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: %s%zu[%zu] = 0x%08" PRIx32, i, j, dname,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, dst[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }
}

// ----- TensorReduce emulation ------------------------------------------------

static void tensorreduce(uint64_t value)
{
    unsigned other_min, action;

    tensor_reduce_decode(current_thread>>1, value, &other_min, &action);

    // Do nothing
    if (action == 2)
    {
#ifndef SYS_EMU
        unsigned level = (value >> 3) & 0xF;
        unsigned type = value & 3;
        uint64_t distance = 1ull << level;
        uint64_t minmask = (1ull << (level + 1)) - 1ull;
        LOG(DEBUG, "%s with level: %u, distance: %" PRId64 ", minmask: 0x%016" PRIx64,
            (type == 2) ? "TensorBroadcast" : "TensorReduceAuto", level, distance, minmask);
#endif
        return;
    }

    //op = rs[35:32]
    int      this_start_reg = (value >> 57) & 0x1F;
    uint8_t  this_operation = (value >> 24) & 0xF;
    int      this_num_reg   = (value >> 16) & 0x7F;

    // Send or receive to/from the same minion
    if (action == 3)
    {
#ifndef SYS_EMU
        static const char* reducecmd[4] = {
            "TensorReduceSend", "TensorReduceRecv",
            "TensorBroadcast", "TensorReduceAuto"
        };
        LOG(DEBUG, "\t%s other_minion: %u, start_reg: %d, num_reg: %d",
            reducecmd[value & 3], other_min, this_start_reg, this_num_reg);
#endif
        update_tensor_error(1 << 9);
        return;
    }

    // Send
    if (action == 0)
    {
#ifndef SYS_EMU
        LOG(DEBUG, "\t%s other_minion: %u, start_reg: %d, num_reg: %d",
            ((value & 3) == 2) ? "TensorBroadcast(send)" : (((value & 3) == 3) ? "TensorReduceAuto(send)" : "TensorReduceSend"),
            other_min, this_start_reg, this_num_reg);
        for (int i = 0; i < this_num_reg; ++i)
        {
            int this_op_reg = (i + this_start_reg) % NFREGS;
            LOG_FREG("(this) :", this_op_reg);
        }
#endif
        return;
    }

    // Receive

    // Get information from sender
    unsigned this_min   = tensorreduce_info[other_min].minion_id;
    int other_start_reg = tensorreduce_info[other_min].start_reg;
    int other_num_reg   = tensorreduce_info[other_min].num_reg;
    int other_action    = tensorreduce_info[other_min].action;
    if (this_min != (current_thread>>1))
    {
        LOG_ALL_MINIONS(DEBUG, "\t%s with sender=%u sender_other_min=%u sender_start_reg=%d sender_num_reg=%d sender_action=%u",
                        ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                        other_min, this_min, other_start_reg, other_num_reg, other_action);
        throw std::runtime_error("Mismatched tensor reduce sender target minion");
    }
    if (other_action != 0)
    {
        LOG_ALL_MINIONS(DEBUG, "\t%s with sender=%u sender_other_min=%u sender_start_reg=%d sender_num_reg=%d sender_action=%u",
                        ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                        other_min, this_min, other_start_reg, other_num_reg, other_action);
        throw std::runtime_error("Mismatched tensor reduce sender action");
    }
    if (other_num_reg != this_num_reg)
    {
        LOG_ALL_MINIONS(DEBUG, "\t%s with sender=%u sender_other_min=%u sender_start_reg=%d sender_num_reg=%d sender_action=%u",
                        ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                        other_min, this_min, other_start_reg, other_num_reg, other_action);
    }

    // Info for checker
    log_tensor_reduce(this_start_reg, this_num_reg);

    switch (this_operation)
    {
        case 0x0: // fadd
            set_rounding_mode(frm());
            LOG(DEBUG, "\t%s op: fadd, other_minion: %u, rounding_mode: %s",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min, get_rounding_mode(frm()));
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].f32[j] = fpu::f32_add(fregs[other_min<<1][other_op_reg].f32[j], FREGS[this_op_reg].f32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            set_fp_exceptions();
            break;
        case 0x1: // fsub
            set_rounding_mode(frm());
            LOG(DEBUG, "\t%s op: fsub, other_minion: %u, rounding_mode: %s",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min, get_rounding_mode(frm()));
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].f32[j] = fpu::f32_sub(fregs[other_min<<1][other_op_reg].f32[j], FREGS[this_op_reg].f32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            set_fp_exceptions();
            break;
        case 0x2: // fmax
            LOG(DEBUG, "\t%s op: fmax, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].f32[j] = fpu::f32_maximumNumber(fregs[other_min<<1][other_op_reg].f32[j], FREGS[this_op_reg].f32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            set_fp_exceptions();
            break;
        case 0x3: // fmin
            LOG(DEBUG, "\t%s op: fmax, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].f32[j] = fpu::f32_minimumNumber(fregs[other_min<<1][other_op_reg].f32[j], FREGS[this_op_reg].f32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            set_fp_exceptions();
            break;
        case 0x4: // iadd
            LOG(DEBUG, "\t%s op: iadd, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].u32[j] = fregs[other_min<<1][other_op_reg].u32[j] + FREGS[this_op_reg].u32[j];
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            break;
        case 0x5: // isub
            LOG(DEBUG, "\t%s op: isub, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].u32[j] = fregs[other_min<<1][other_op_reg].u32[j] - FREGS[this_op_reg].u32[j];
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            break;
        case 0x6: // imax
            LOG(DEBUG, "\t%s op: imax, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].i32[j] = std::max(fregs[other_min<<1][other_op_reg].i32[j], FREGS[this_op_reg].i32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            break;
        case 0x7: // imin
            LOG(DEBUG, "\t%s op: imin, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                LOG_FREG("(this) :", this_op_reg);
                for (unsigned j = 0; j < VL; j++)
                {
                    FREGS[this_op_reg].i32[j] = std::min(fregs[other_min<<1][other_op_reg].i32[j], FREGS[this_op_reg].i32[j]);
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                }
                LOG_FREG("(this) =", this_op_reg);
            }
            break;
        case 0x8: // fget
            LOG(DEBUG, "\t%s op: fget, other_minion: %u",
                ((value & 3) == 2) ? "TensorBroadcast(recv)" : (((value & 3) == 3) ? "TensorReduceAuto(recv)" : "TensorReduceRecv"),
                other_min);
            for (int i = 0; i < this_num_reg; i++)
            {
                int this_op_reg = (i + this_start_reg) % NFREGS;
                int other_op_reg = (i + other_start_reg) % NFREGS;
                LOG_FREG_OTHER(other_min<<1, "(othr) :", other_op_reg);
                FREGS[this_op_reg] = fregs[other_min<<1][other_op_reg];
                for (unsigned j = 0; j < VL; j++)
                    log_tensor_reduce_write(this_op_reg, j, FREGS[this_op_reg].u32[j]);
                LOG_FREG("(this) =", this_op_reg);
            }
            break;
        default:
            throw std::runtime_error("TensorReduce with illegal operation code!");
            break;
    }
    if (this_num_reg)
        dirty_fp_state();
}

// Helper function that given the written value to the CSR, returns:
//   - what is the ID of the other minion of the reduce
//   - what is the action taken by the minion (send, receive, do nothing)
void tensor_reduce_decode(uint64_t minion_id, uint64_t value, unsigned* other_min, unsigned* action)
{
    uint64_t level = (value >> 3) & 0xF;
    uint64_t type  = value & 3;

    // SENDER
    if (type == 0)
    {
        *action = 0;
        *other_min = (value >> 3) & 0x1FFF;
        //LOG_ALL_MINIONS(DEBUG, "TensorSend[0x%" PRIx64 "]: other_min=%d", value, *other_min);
    }
    // RECEIVER
    else if (type == 1)
    {
        *action = 1;
        *other_min = (value >> 3) & 0x1FFF;
        //LOG_ALL_MINIONS(DEBUG, "TensorRecv[0x%" PRIx64 "]: other_min=%d", value, *other_min);
    }
    // BROADCAST: Compute sender/receiver assuming recursive halving
    else if (type == 2)
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if ((minion_id & minion_mask) == distance)
        {
            *action = 1; // receiver
            *other_min = minion_id - distance;
        }
        else if ((minion_id & minion_mask) == 0)
        {
            *action = 0; // sender
            *other_min = minion_id + distance;
        }
        else
        {
            *action = 2; // do nothing
        }
        //LOG_ALL_MINIONS(DEBUG, "TensorBroadcast[0x%" PRIx64 "]: action=%s, other_min=%d", value,
        //                (*action == 0 ? "SEND" : (*action == 1 ? "RECV" : "NONE")), *other_min);
    }
    // REDUCE: Compute sender/receiver assuming recursive halving
    else
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if ((minion_id & minion_mask) == distance)
        {
            *action = 0; // sender
            *other_min = minion_id - distance;
        }
        else if ((minion_id & minion_mask) == 0)
        {
            *action = 1; // receiver
            *other_min = minion_id + distance;
        }
        else
        {
            *action = 2; // do nothing
        }
        //LOG_ALL_MINIONS(DEBUG, "TensorReduce[0x%" PRIx64 "]: action=%s, other_min=%d", value,
        //                (*action == 0 ? "SEND" : (*action == 1 ? "RECV" : "NONE")), *other_min);
    }

    if (*action == 2)
        return;

    // Sending and receiving from the same minion should fail immediately
    if (*other_min == (current_thread>>1))
    {
        *action = 3;
        return;
    }

    // Update sender information so it can be used by the receiver
    if (*action == 0)
    {
        tensorreduce_info[minion_id].minion_id = *other_min;
        tensorreduce_info[minion_id].start_reg = (value >> 57) & 0x1F;
        tensorreduce_info[minion_id].num_reg   = (value >> 16) & 0x7F;
        tensorreduce_info[minion_id].action    = *action;
    }
}

// ----- Shire cooperative mode ------------------------------------------------

void write_shire_coop_mode(unsigned shire, uint64_t val)
{
    assert(shire < EMU_NUM_MINION_SHIRES);
    esr_shire_coop_mode[shire] = !!(val & 1);
#ifndef SYS_EMU
    if (!esr_shire_coop_mode[shire])
        esr_icache_prefetch_active[shire] = false;
#endif
}

uint64_t read_shire_coop_mode(unsigned shire)
{
    assert(shire < EMU_NUM_MINION_SHIRES);
    return esr_shire_coop_mode[shire] ? 1ull : 0ull;
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto fast local barrier extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// Fast local barriers can be accessed through UC to do stores and loads,
// and also through the CSR that implement the fast local barrier function.
static uint64_t flbarrier(uint64_t value)
{
    uint64_t barrier = value % FAST_LOCAL_BARRIERS;
    uint64_t limit   = (value / FAST_LOCAL_BARRIERS) & 0x7F;
    uint64_t shire   = current_thread / EMU_THREADS_PER_SHIRE;
    if (shire == EMU_IO_SHIRE_SP)
        shire = IO_SHIRE_ID;

    // Gets what is the address that the fast local barrier is mapped to
    uint64_t addr = ESR_SHIRE(shire, FAST_LOCAL_BARRIER0) + (barrier * 8); // Access is private per cache

    // NB: No PMA checks here... we know it will pass ;-)

    uint64_t orig_value = pmemread64(addr);
    uint64_t result = -1;

    LOG(DEBUG,"FastLocalBarrier: Shire %i: Minion %i Thread %i doing barrier %" PRIu64 " value  %" PRIu64 ", limit %" PRIu64 " ",
         (int) shire, current_thread / EMU_THREADS_PER_MINION, current_thread % EMU_THREADS_PER_MINION, barrier, orig_value, limit);
    // Last guy, return 1 and zero barrier
    if (orig_value == limit)
    {
        LOG(DEBUG,"FastLocalBarrier: last minion Shire %i!!", (int) shire);

        pmemwrite64(addr, 0);
        result = 1;
    }
    // Not the last guy, return 0 and increment barrier
    else
    {
        LOG(DEBUG, "FastLocalBarrier: Limit %" PRIu64", Incrementing to %" PRIu64 "!!", limit, orig_value + 1);
        pmemwrite64(addr, orig_value + 1);
        result = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto fast credit counter extension
//
////////////////////////////////////////////////////////////////////////////////

uint64_t get_fcc_cnt()
{
    return fcc_cnt;
}

void fcc_inc(uint64_t thread, uint64_t shire, uint64_t minion_mask, uint64_t fcc_id)
{
    LOG(DEBUG,"fcc_inc(%" PRIu64 ", %" PRIu64 ", 0x%" PRIx64 ", %" PRIu64 ")",
        thread, shire, minion_mask, fcc_id);

    int minions_in_shire = EMU_MINIONS_PER_SHIRE;
    if (shire == EMU_IO_SHIRE_SP) {
        minions_in_shire = 1;
        if (thread)
            throw std::runtime_error("fcc_inc to SP for thread1");
    }

    for (int minion = 0; minion < minions_in_shire; ++minion)
    {
        if (minion_mask & (1ull << minion))
        {
            size_t fcc_addr = shire*EMU_THREADS_PER_SHIRE + EMU_THREADS_PER_MINION*minion + thread;
            LOG(DEBUG, "Incrementing FCC%" PRIu64 "[H%" PRIu64 "]=%" PRId32, thread*2 + fcc_id, fcc_addr, fcc[fcc_addr][fcc_id] + 1);
            fcc[fcc_addr][fcc_id]++;

#ifndef SYS_EMU
            // wake up waiting threads (only for checker, not sysemu)
            if (fcc_wait[fcc_addr]){
                fcc_wait[fcc_addr] = false;
                minions_to_awake.push(fcc_addr>>1);
            }
#endif

            //check for overflow
            if (fcc[fcc_addr][fcc_id] == 0x000) {
                update_tensor_error(fcc_addr, 1 << 3);
                fcc[fcc_addr][fcc_id] = 0;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto IPI extension
//
////////////////////////////////////////////////////////////////////////////////

void raise_interrupt(int thread, int cause)
{
    if (cause == 9)
    {
        ext_seip[thread] |= 1<<cause;
    }
    else
    {
        csr_mip[thread] |= 1<<cause;
    }
}

void raise_software_interrupt(int thread)
{
    csr_mip[thread] |= 0x8;
}

void clear_software_interrupt(int thread)
{
    csr_mip[thread] &= ~0x8;
}

void raise_timer_interrupt(int thread)
{
    csr_mip[thread] |= 0x80;
}

void clear_timer_interrupt(int thread)
{
    csr_mip[thread] &= ~0x80;
}

void raise_external_machine_interrupt(int thread)
{
    csr_mip[thread] |= 0x800;
}

void clear_external_machine_interrupt(int thread)
{
    csr_mip[thread] &= ~0x800;
}

void raise_external_supervisor_interrupt(int thread)
{
    ext_seip[thread] |= 0x200;
}

void clear_external_supervisor_interrupt(int thread)
{
    ext_seip[thread] &= ~0x200;
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto code prefetching extension emulation
//
////////////////////////////////////////////////////////////////////////////////

void write_icache_prefetch(int privilege, unsigned shire, uint64_t val)
{
    assert(shire <= EMU_MASTER_SHIRE);
    (void)(privilege);
#ifdef SYS_EMU
    (void)(shire);
    (void)(val);
#else
    if (!esr_icache_prefetch_active[shire])
    {
        bool active = ((val >> 48) & 0xF) && esr_shire_coop_mode[shire];
        esr_icache_prefetch_active[shire] = active;
    }
#endif
}

uint64_t read_icache_prefetch(int privilege __attribute__((unused)), unsigned shire)
{
    assert(shire <= EMU_MASTER_SHIRE);
#ifdef SYS_EMU
    // NB: Prefetches finish instantaneously in sys_emu
    return esr_shire_coop_mode[shire];
#else
    return esr_icache_prefetch_active[shire] ? 0ull : 1ull;
#endif
}

void finish_icache_prefetch(unsigned shire)
{
    assert(shire <= EMU_MASTER_SHIRE);
#ifndef SYS_EMU
    esr_icache_prefetch_active[shire] = false;
#endif
}
