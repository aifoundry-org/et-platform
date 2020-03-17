/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

// LCOV_EXCL_START
#include <algorithm>
#include <array>
#include <cassert>
#include <cfenv>        // FIXME: remove this when we purge std::fesetround() from the code!
#include <cmath>        // FIXME: remove this, we should not do any math here
#include <cstdio>       // FIXME: Remove this, use "emu_gio.h" instead
#include <cstring>
#include <deque>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "cache.h"
#include "decode.h"
#include "emu.h"
#include "emu_gio.h"
#include "esrs.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "gold.h"
#include "log.h"
#include "memmap.h"
#include "memop.h"
#include "mmu.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#include "mem_directory.h"
#include "scp_directory.h"
#endif
#include "traps.h"
#include "txs.h"
#include "utility.h"

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

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

// Memory state
namespace bemu {
MainMemory memory{};
typename MemoryRegion::reset_value_type memory_reset_value = {0};
}

// Hart state
std::array<Processor,EMU_NUM_THREADS>  cpu;

// Other processor state
std::array<bool,    EMU_NUM_THREADS>    mtvec_is_set;
std::array<bool,    EMU_NUM_THREADS>    stvec_is_set;
std::array<bool,    EMU_NUM_THREADS>    debug_mode;
std::array<uint32_t,EMU_NUM_THREADS>    ext_seip;
static std::array<bool,EMU_NUM_THREADS-1> tensorload_setupb_topair;
static std::array<int, EMU_NUM_THREADS-1> tensorload_setupb_numlines;

// Scratchpad
// FIXME: The scratchpad is shared among all threads of a minion. We should
// really just have one scratchpad per minion, not one per thread. This all
// works for now because only thread0 of a minion can access the scratchpad.
std::array<std::array<cache_line_t,L1_SCP_ENTRIES+TFMA_MAX_AROWS>,EMU_NUM_THREADS> scp;

// Used to access different threads transparently
#define XREGS cpu[current_thread].xregs
#define FREGS cpu[current_thread].fregs
#define MREGS cpu[current_thread].mregs
#define TENC  cpu[current_thread].tenc
#define SCP   scp[current_thread]

// Message ports
static std::array<std::array<msg_port_conf_t,NR_MSG_PORTS>,EMU_NUM_THREADS>      msg_ports;
static std::array<std::array<std::deque<uint8_t>,NR_MSG_PORTS>,EMU_NUM_THREADS>  msg_ports_oob;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_tbox;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_rbox;
static bool msg_port_delayed_write = false;

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

// SCP checks
#ifdef SYS_EMU
    #define L1_SCP_CHECK_FILL(thread, idx, id) \
        { if(sys_emu::get_scp_check()) \
        { \
            sys_emu::get_scp_directory().l1_scp_fill(thread, idx, id); \
        } }
    #define L1_SCP_CHECK_READ(thread, idx) \
        { if(sys_emu::get_scp_check()) \
        { \
            sys_emu::get_scp_directory().l1_scp_read(thread, idx); \
        } }
#else
    #define L1_SCP_CHECK_FILL(thread, idx, id) \
        { }
    #define L1_SCP_CHECK_READ(thread, idx) \
        { }
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

#define MAXSTACK 2048
static std::array<std::array<uint32_t,MAXSTACK>,EMU_NUM_THREADS> shaderstack;
static bool check_stack = false;

bool m_emu_done = false;

bool emu_done()
{
   return m_emu_done;
}

std::string dump_xregs(unsigned thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (size_t ii = 0; ii < NXREGS; ++ii)
      {
         str << "XREG[" << std::dec << ii << "] = 0x" << std::hex << cpu[thread_id].xregs[ii] << "\n";
      }
   }
   return str.str();
}

std::string dump_fregs(unsigned thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (size_t ii = 0; ii < NFREGS; ++ii)
      {
         for (size_t jj = 0; jj < VL; ++jj)
         {
            str << "FREG[" << std::dec << ii << "][" << jj <<  "] = 0x" << std::hex << cpu[thread_id].fregs[ii].u32[jj] << "\t";
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
    unsigned neigh_count = (shire == EMU_IO_SHIRE_SP) ? 1 : EMU_NEIGH_PER_SHIRE;

    for (unsigned neigh = 0; neigh < neigh_count; ++neigh) {
        unsigned idx = EMU_NEIGH_PER_SHIRE*shire + neigh;
        bemu::neigh_esrs[idx].reset();
    }
    bemu::shire_cache_esrs[shire].reset();
    bemu::shire_other_esrs[shire].reset(shireid);
    bemu::broadcast_esrs[shire].reset();

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
static uint64_t csrset(uint16_t src1, uint64_t val);
static void tmask_conv();
static void tcoop(uint64_t value);
static void tensor_load_start(uint64_t control);
static void tensorloadl2(uint64_t control);
static void tensorstore(uint64_t tstorereg);
static void tensor_fma32_start(uint64_t tfmareg);
static void tensor_fma16a32_start(uint64_t tfmareg);
static void tensor_ima8a32_start(uint64_t tfmareg);
static void tensor_quant_start(uint64_t value);
static void tensor_reduce_start(uint64_t value);
static int64_t port_get(unsigned id, bool block);
static void configure_port(unsigned id, uint64_t wdata);
static uint64_t flbarrier(uint64_t value);
static uint64_t read_port_base_address(unsigned thread, unsigned id);

////////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
////////////////////////////////////////////////////////////////////////////////

// internal accessor to frm
static inline int frm()
{
    return (cpu[current_thread].fcsr >> 5) & 0x7;
}

static const char* get_rounding_mode(int mode)
{
    static const char* rmnames[] = {
        "rne",      "rtz",       "rdn",       "rup",
        "rmm",      "res5",      "res6",      "dyn",
        "dyn(rne)", "dyn(rtz)",  "dyn(rdn)",  "dyn(rup)",
        "dyn(rmm)", "dyn(res5)", "dyn(res6)", "dyn(res7)",
    };

    return rmnames[(mode == rmdyn) ? (8 + frm()) : (mode & 7)];
}

static const char* get_reduce_state(Processor::Reduce::State state)
{
    static const char* stnames[] = {
        "Idle", "Send", "Recv"
    };
    return stnames[static_cast<uint8_t>(state)];
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

uint64_t xget(xreg src1)
{
    return XREGS[src1];
}

uint64_t get_csr(unsigned thread, uint16_t cnum)
{
    unsigned oldthread = current_thread;
    current_thread = thread;
    uint64_t retval = csrget(cnum);
    current_thread = oldthread;
    return retval;
}

void set_csr(unsigned thread, uint16_t cnum, uint64_t data)
{
    unsigned oldthread = current_thread;
    current_thread = thread;
    csrset(cnum, data);
    current_thread = oldthread;
}

void fpinit(freg dst, uint64_t val[VL/2])
{
    for (size_t i = 0; i < VL/2; ++i)
        FREGS[dst].u64[i] = val[i];
}

static void activate_breakpoints(prv_t priv)
{
    uint64_t mcontrol = cpu[current_thread].tdata1;
    cpu[current_thread].break_on_load  = !(~mcontrol & ((8 << int(priv)) | 1));
    cpu[current_thread].break_on_store = !(~mcontrol & ((8 << int(priv)) | 2));
    cpu[current_thread].break_on_fetch = !(~mcontrol & ((8 << int(priv)) | 4));
}

// internal accessor to prv
static inline void set_prv(prv_t val)
{
    cpu[current_thread].prv = val;
    activate_breakpoints(val);
}

// internal accessor to tensor_error
static inline void update_tensor_error(unsigned thread, uint16_t value)
{
    cpu[thread].tensor_error |= value;
    if (value)
        LOG_OTHER(DEBUG, thread, "\ttensor_error = 0x%04" PRIx16 " (0x%04" PRIx16 ")", cpu[thread].tensor_error, value);
}

static inline void update_tensor_error(uint16_t value)
{
    update_tensor_error(current_thread, value);
}

void reset_hart(unsigned thread)
{
    // Register files
    cpu[thread].xregs[0] = 0;

    // RISCV control and status registers
    cpu[thread].scounteren = 0;
    cpu[thread].mstatus = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    cpu[thread].medeleg = 0;
    cpu[thread].mideleg = 0;
    cpu[thread].mie = 0;
    cpu[thread].mcounteren = 0;
    for (auto &elem : cpu[thread].mhpmevent) {
        elem = 0;
    }
    cpu[thread].mcause = 0;
    cpu[thread].mip = 0;
    cpu[thread].tdata1 = 0x20C0000000000000ULL;
    // TODO: cpu[thread].dcsr <= xdebugver=1, prv=3;
    cpu[thread].mhartid = (thread == (EMU_IO_SHIRE_SP*EMU_THREADS_PER_SHIRE))
                        ? (IO_SHIRE_ID*EMU_THREADS_PER_SHIRE)
                        : thread;

    // Esperanto control and status registers
    cpu[thread].matp = 0;
    cpu[thread].minstmask = 0;
    cpu[thread].minstmatch = 0;
    // TODO: cpu[thread].amofence_ctrl <= ...
    cpu[thread].menable_shadows = 0;
    cpu[thread].excl_mode = 0;
    cpu[thread].mcache_control = 0;
    cpu[thread].ucache_control = 0x200;
    cpu[thread].gsc_progress = 0;
    for (auto &elem : cpu[thread].portctrl) {
        elem = 0x8000;
    }

    // Other hart internal (microarchitectural or hidden) state
    cpu[thread].prv = PRV_M;

    // Pre-computed state to improve simulation speed
    cpu[thread].break_on_load = false;
    cpu[thread].break_on_store = false;
    cpu[thread].break_on_fetch = false;

    // Reset tensor operation state machines
    cpu[thread].reduce.count = 0;
    cpu[thread].reduce.state = Processor::Reduce::State::Idle;
    cpu[thread].wait.state = Processor::Wait::State::Idle;
    cpu[thread].txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].shadow_txquant = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].shadow_txfma = 0xFFFFFFFFFFFFFFFFULL;
    cpu[thread].txload.fill(0xFFFFFFFFFFFFFFFFULL);
    cpu[thread].shadow_txload.fill(0xFFFFFFFFFFFFFFFFULL);
    cpu[thread].tensor_op.fill(Processor::Tensor::None);

    // Other processor state outside of cpu[thread]
    for (int i = 0; i < NR_MSG_PORTS; ++i)
    {
        memset(&msg_ports[thread][i], 0, sizeof(msg_port_conf_t));
        msg_ports[thread][i].offset = -1;
    }
    debug_mode[thread] = false;
    ext_seip[thread] = 0;
    mtvec_is_set[thread] = false;
    stvec_is_set[thread] = false;
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
    return (cpu[current_thread].tensor_mask >> bit) & 1;
}

void check_pending_interrupts()
{
    // Are there any non-masked pending interrupts?
    // NB: If excl_mode != 0 this thread is either in exclusive mode or
    // blocked, but either way it cannot receive interrupts
    uint64_t xip = (cpu[current_thread].mip | ext_seip[current_thread]) & cpu[current_thread].mie;
    if (!xip || cpu[current_thread].excl_mode)
        return;

    LOG(DEBUG, "Check Pending Interrupt mtvec:0x%016" PRIx64 " mip:0x%08" PRIx32 " xseip:0x%08" PRIx32 " mie:0x%08" PRIx32,
        cpu[current_thread].mtvec, cpu[current_thread].mip, ext_seip[current_thread], cpu[current_thread].mie);

    // If there are any pending interrupts for the current privilege level
    // 'x', they are only taken if mstatus.xIE=1. If there are any pending
    // interrupts for a higher privilege level 'y>x' they must be taken
    // independently of the value in mstatus.yIE. Pending interrupts for a
    // lower privilege level 'w<x' are not taken.
    uint64_t mideleg = cpu[current_thread].mideleg;
    uint64_t mip = xip & ~mideleg;
    uint64_t sip = xip & mideleg;
    uint64_t mie = cpu[current_thread].mstatus & 8;
    uint64_t sie = cpu[current_thread].mstatus & 2;
    switch (PRV)
    {
        case PRV_M:
            if (!mip || !mie) return;
            xip = mip;
            break;
        case PRV_S:
            if (!mip && !sie) return;
            xip = mip | (sie ? sip : 0);
            break;
        default:
            /* nothing */
            break;
    }

    if (xip & 0x00800) throw trap_machine_external_interrupt();
    if (xip & 0x00008) throw trap_machine_software_interrupt();
    if (xip & 0x00080) throw trap_machine_timer_interrupt();
    if (xip & 0x00200) throw trap_supervisor_external_interrupt();
    if (xip & 0x00002) throw trap_supervisor_software_interrupt();
    if (xip & 0x00020) throw trap_supervisor_timer_interrupt();
#if 0
    if (xip & 0x00100) throw trap_user_external_interrupt();
    if (xip & 0x00001) throw trap_user_software_interrupt();
    if (xip & 0x00010) throw trap_user_timer_interrupt();
#endif
    if (xip & 0x10000) throw trap_bad_ipi_redirect_interrupt();
    if (xip & 0x80000) throw trap_icache_ecc_counter_overflow_interrupt();
    if (xip & 0x800000) throw trap_bus_error_interrupt();
}

static void trap_to_smode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = PRV;
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);
    assert(curprv <= PRV_S);

#ifdef SYS_EMU
    if (sys_emu::get_display_trap_info())  {
      LOG(INFO, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    } else
#endif
    {
      LOG(DEBUG, "\tTrapping to S-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    }

    // if checking against RTL, clear the correspoding MIP bit
    // it will be set to 1 again if the pending bit was not really cleared
    // just before entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by
    // a memory mapped store)
#ifndef SYS_EMU
    if (interrupt) {
        // Clear external supervisor interrupt
        if (code == 0x9 && !(cpu[current_thread].mip & 0x200)) {
            clear_external_supervisor_interrupt(current_thread);
            LOG(DEBUG, "%s", "\tClearing external supervisor interrupt");
        }
        cpu[current_thread].mip &= ~(1<<code);
    }
#endif

    // Take sie
    uint64_t mstatus = cpu[current_thread].mstatus;
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
    set_prv(PRV_S);

    // Throw an error if no one ever set stvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (stvec_is_set[current_thread] == false)
        LOG(WARN, "%s", "Trap vector has never been set. Can't take exception properly");

    // compute address where to jump to:
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause for interrupts, tvec for exceptions
    uint64_t tvec = cpu[current_thread].stvec;
    if ((tvec & 1) && interrupt)
    {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    log_trap(cpu[current_thread].mstatus, cpu[current_thread].scause, cpu[current_thread].stval, cpu[current_thread].sepc);
    log_pc_update(tvec);
}

static void trap_to_mmode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = PRV;
    bool interrupt = (cause & 0x8000000000000000ULL);
    int code = (cause & 63);

    // Check if we should deletegate the trap to S-mode
    if ((curprv < PRV_M) && ((interrupt ? cpu[current_thread].mideleg : cpu[current_thread].medeleg) & (1ull<<code)))
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
        cpu[current_thread].mip &= ~(1<<code);
        // Clear external supervisor interrupt
        if (cause == 9 && !(cpu[current_thread].mip & 0x200)) {
            clear_external_supervisor_interrupt(current_thread);
        }
    }
#endif

#ifdef SYS_EMU
    if (sys_emu::get_display_trap_info())  {
      LOG(INFO, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    } else
#endif
    {
      LOG(DEBUG, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval 0x%" PRIx64, cause, val);
    }

    // Take mie
    uint64_t mstatus = cpu[current_thread].mstatus;
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
    set_prv(PRV_M);

    // Throw an error if no one ever set mtvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (mtvec_is_set[current_thread] == false)
        LOG(WARN, "%s", "Trap vector has never been set. Doesn't smell good...");

    // compute address where to jump to
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause for interrupts, tvec for exceptions
    uint64_t tvec = cpu[current_thread].mtvec;
    if ((tvec & 1) && interrupt)
    {
        tvec += code * 4;
    }
    tvec &= ~0x1ULL;
    log_trap(cpu[current_thread].mstatus, cpu[current_thread].mcause, cpu[current_thread].mtval, cpu[current_thread].mepc);
    log_pc_update(tvec);
}

void take_trap(const trap_t& t)
{
    trap_to_mmode(t.cause(), t.tval());
}

void check_minst_match(uint32_t bits)
{
    uint64_t minstmask = cpu[current_thread].minstmask;
    if ((minstmask >> 32) != 0)
    {
        uint32_t mask = minstmask;
        if (((bits ^ cpu[current_thread].minstmatch) & mask) == 0)
            throw trap_mcode_instruction(bits);
    }
}

void set_pc(uint64_t pc)
{
    current_pc = sextVA(pc);
}

void set_thread(unsigned thread)
{
    current_thread = thread;
}

unsigned get_thread()
{
    return current_thread;
}

bool thread_is_blocked(unsigned thread)
{
    unsigned other_excl = 1 + ((~thread & 1) << 1);
    return cpu[thread].excl_mode == other_excl;
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
// SYSTEM emulation
//
////////////////////////////////////////////////////////////////////////////////

// forward declarations
static void dcache_change_mode(uint8_t, uint8_t);
static void dcache_evict_flush_set_way(bool, bool, int, int, int, int);
static void dcache_evict_flush_vaddr(bool, bool, int, uint64_t, int, int, uint64_t);
static void dcache_prefetch_vaddr(uint64_t);
static void dcache_lock_vaddr(bool, uint64_t, int, int, uint64_t);
static void dcache_unlock_vaddr(bool, uint64_t, int, int, uint64_t);
static void dcache_lock_paddr(int, uint64_t);
static void dcache_unlock_set_way(int, int);

static void check_counter_is_enabled(int cnt)
{
    uint64_t enabled = (cpu[current_thread].mcounteren & (1 << cnt));
    if ( ((PRV == PRV_U) && ((cpu[current_thread].scounteren & enabled) == 0))
         || ((PRV == PRV_S) && (enabled == 0)))
    {
        throw trap_illegal_instruction(current_inst);
    }
}
// LCOV_EXCL_STOP
static uint64_t csrget(uint16_t src1)
{
    uint64_t val;

    switch (src1)
    {
    case CSR_FFLAGS:
        require_fp_active();
        val = cpu[current_thread].fcsr & 0x8000001f;
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu[current_thread].fcsr >> 5) & 0x7;
        break;
    case CSR_FCSR:
        require_fp_active();
        val = cpu[current_thread].fcsr;
        break;
    case CSR_SSTATUS:
        // Hide sxl, tsr, tw, tvm, mprv, mpp, mpie, mie
        val = cpu[current_thread].mstatus & 0x80000003000DE133ULL;
        break;
    case CSR_SIE:
        val = cpu[current_thread].mie & cpu[current_thread].mideleg;
        break;
    case CSR_STVEC:
        val = cpu[current_thread].stvec;
        break;
    case CSR_SCOUNTEREN:
        val = cpu[current_thread].scounteren;
        break;
    case CSR_SSCRATCH:
        val = cpu[current_thread].sscratch;
        break;
    case CSR_SEPC:
        val = cpu[current_thread].sepc;
        break;
    case CSR_SCAUSE:
        val = cpu[current_thread].scause;
        break;
    case CSR_STVAL:
        val = cpu[current_thread].stval;
        break;
    case CSR_SIP:
        val = cpu[current_thread].mip & cpu[current_thread].mideleg;
        break;
    case CSR_SATP:
        val = cpu[current_thread].satp;
        break;
    case CSR_MSTATUS:
        val = cpu[current_thread].mstatus;
        break;
    case CSR_MISA:
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        val = cpu[current_thread].medeleg;
        break;
    case CSR_MIDELEG:
        val = cpu[current_thread].mideleg;
        break;
    case CSR_MIE:
        val = cpu[current_thread].mie;
        break;
    case CSR_MTVEC:
        val = cpu[current_thread].mtvec;
        break;
    case CSR_MCOUNTEREN:
        val = cpu[current_thread].mcounteren;
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val = cpu[current_thread].mhpmevent[src1 - CSR_MHPMEVENT3];
        break;
    case CSR_MHPMEVENT9:
    case CSR_MHPMEVENT10:
    case CSR_MHPMEVENT11:
    case CSR_MHPMEVENT12:
    case CSR_MHPMEVENT13:
    case CSR_MHPMEVENT14:
    case CSR_MHPMEVENT15:
    case CSR_MHPMEVENT16:
    case CSR_MHPMEVENT17:
    case CSR_MHPMEVENT18:
    case CSR_MHPMEVENT19:
    case CSR_MHPMEVENT20:
    case CSR_MHPMEVENT21:
    case CSR_MHPMEVENT22:
    case CSR_MHPMEVENT23:
    case CSR_MHPMEVENT24:
    case CSR_MHPMEVENT25:
    case CSR_MHPMEVENT26:
    case CSR_MHPMEVENT27:
    case CSR_MHPMEVENT28:
    case CSR_MHPMEVENT29:
    case CSR_MHPMEVENT30:
    case CSR_MHPMEVENT31:
        val = 0;
        break;
    case CSR_MSCRATCH:
        val = cpu[current_thread].mscratch;
        break;
    case CSR_MEPC:
        val = cpu[current_thread].mepc;
        break;
    case CSR_MCAUSE:
        val = cpu[current_thread].mcause;
        break;
    case CSR_MTVAL:
        val = cpu[current_thread].mtval;
        break;
    case CSR_MIP:
        val = cpu[current_thread].mip;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        val = cpu[current_thread].tdata1;
        break;
    case CSR_TDATA2:
        val = cpu[current_thread].tdata2;
        break;
    // unimplemented: TDATA3
    // TODO: DCSR
    // TODO: DPC
    // unimplemented: DSCRATCH0
    // unimplemented: DSCRATCH1
    case CSR_MCYCLE:
    case CSR_MINSTRET:
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
    case CSR_MHPMCOUNTER9:
    case CSR_MHPMCOUNTER10:
    case CSR_MHPMCOUNTER11:
    case CSR_MHPMCOUNTER12:
    case CSR_MHPMCOUNTER13:
    case CSR_MHPMCOUNTER14:
    case CSR_MHPMCOUNTER15:
    case CSR_MHPMCOUNTER16:
    case CSR_MHPMCOUNTER17:
    case CSR_MHPMCOUNTER18:
    case CSR_MHPMCOUNTER19:
    case CSR_MHPMCOUNTER20:
    case CSR_MHPMCOUNTER21:
    case CSR_MHPMCOUNTER22:
    case CSR_MHPMCOUNTER23:
    case CSR_MHPMCOUNTER24:
    case CSR_MHPMCOUNTER25:
    case CSR_MHPMCOUNTER26:
    case CSR_MHPMCOUNTER27:
    case CSR_MHPMCOUNTER28:
    case CSR_MHPMCOUNTER29:
    case CSR_MHPMCOUNTER30:
    case CSR_MHPMCOUNTER31:
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
        check_counter_is_enabled(src1 - CSR_CYCLE);
        val = 0;
        break;
    case CSR_HPMCOUNTER9:
    case CSR_HPMCOUNTER10:
    case CSR_HPMCOUNTER11:
    case CSR_HPMCOUNTER12:
    case CSR_HPMCOUNTER13:
    case CSR_HPMCOUNTER14:
    case CSR_HPMCOUNTER15:
    case CSR_HPMCOUNTER16:
    case CSR_HPMCOUNTER17:
    case CSR_HPMCOUNTER18:
    case CSR_HPMCOUNTER19:
    case CSR_HPMCOUNTER20:
    case CSR_HPMCOUNTER21:
    case CSR_HPMCOUNTER22:
    case CSR_HPMCOUNTER23:
    case CSR_HPMCOUNTER24:
    case CSR_HPMCOUNTER25:
    case CSR_HPMCOUNTER26:
    case CSR_HPMCOUNTER27:
    case CSR_HPMCOUNTER28:
    case CSR_HPMCOUNTER29:
    case CSR_HPMCOUNTER30:
    case CSR_HPMCOUNTER31:
        throw trap_illegal_instruction(current_inst);
        break;
    case CSR_MVENDORID:
        val = CSR_VENDOR_ID;
        break;
    case CSR_MARCHID:
        val = CSR_ARCH_ID;
        break;
    case CSR_MIMPID:
        val = CSR_IMP_ID;
        break;
    case CSR_MHARTID:
        val = cpu[current_thread].mhartid;
        break;
    // ----- Esperanto registers -------------------------------------
    case CSR_MATP:
        val = cpu[current_thread].matp;
        break;
    case CSR_MINSTMASK:
        val = cpu[current_thread].minstmask;
        break;
    case CSR_MINSTMATCH:
        val = cpu[current_thread].minstmatch;
        break;
    case CSR_CACHE_INVALIDATE:
        val = 0;
        break;
    case CSR_MENABLE_SHADOWS:
        val = cpu[current_thread].menable_shadows;
        break;
    case CSR_EXCL_MODE:
        val = cpu[current_thread].excl_mode & 1;
        break;
    case CSR_MBUSADDR:
        val = cpu[current_thread].mbusaddr;
        break;
    case CSR_MCACHE_CONTROL:
        val = cpu[current_thread].mcache_control;
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
        val = cpu[current_thread].tensor_mask;
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
        val = cpu[current_thread].tensor_error;
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        val = cpu[current_thread].ucache_control;
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_FLB:
    case CSR_FCC:
    case CSR_STALL:
        require_feature_ml();
        val = 0;
        break;
    case CSR_TENSOR_WAIT:
        val = 0;
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        val = 0;
        break;
    case CSR_GSC_PROGRESS:
        val = cpu[current_thread].gsc_progress;
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
// LCOV_EXCL_START
    case CSR_VALIDATION0:
        val = cpu[current_thread].validation0;
        break;
    case CSR_VALIDATION1:
#ifdef SYS_EMU
    switch (cpu[current_thread].validation1)
    {
    case ET_DIAG_CYCLE:
        val = sys_emu::get_emu_cycle();
        break;
    default:
        val = 0;
        break;
    }
#else
    val = 0;
#endif
        break;
    case CSR_VALIDATION2:
        val = cpu[current_thread].validation2;
        break;
    case CSR_VALIDATION3:
        val = cpu[current_thread].validation3;
        break;
// LCOV_EXCL_STOP
    case CSR_LOCK_VA:
    case CSR_UNLOCK_VA:
        require_feature_u_cacheops();
        val = 0;
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = cpu[current_thread].portctrl[src1 - CSR_PORTCTRL0];
        break;
    case CSR_FCCNB:
        require_feature_ml();
        val = (uint64_t(fcc[current_thread][1]) << 16) + uint64_t(fcc[current_thread][0]);
        break;
    case CSR_PORTHEAD0:
    case CSR_PORTHEAD1:
    case CSR_PORTHEAD2:
    case CSR_PORTHEAD3:
        if (((cpu[current_thread].portctrl[src1-CSR_PORTHEAD0] & 0x1) == 0)
            || ((PRV == PRV_U) && ((cpu[current_thread].portctrl[src1-CSR_PORTHEAD0] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(src1 - CSR_PORTHEAD0, true);
        break;
    case CSR_PORTHEADNB0:
    case CSR_PORTHEADNB1:
    case CSR_PORTHEADNB2:
    case CSR_PORTHEADNB3:
        if (((cpu[current_thread].portctrl[src1-CSR_PORTHEADNB0] & 0x1) == 0)
            || ((PRV == PRV_U) && ((cpu[current_thread].portctrl[src1-CSR_PORTHEADNB0] & 0x8) == 0)))
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = port_get(src1 - CSR_PORTHEADNB0, false);
        break;
    case CSR_HARTID:
        if (PRV != PRV_M && (cpu[current_thread].menable_shadows & 1) == 0)
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = cpu[current_thread].mhartid;
        break;
    case CSR_DCACHE_DEBUG:
        val = 0;
        break;
    // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }
    return val;
}

static uint64_t csrset(uint16_t src1, uint64_t val)
{
    uint64_t msk;

    switch (src1)
    {
    case CSR_FFLAGS:
        require_fp_active();
        val = (cpu[current_thread].fcsr & 0x000000E0) | (val & 0x8000001F);
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_FRM:
        require_fp_active();
        val = (cpu[current_thread].fcsr & 0x8000001F) | ((val & 0x7) << 5);
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_FCSR:
        require_fp_active();
        val &= 0x800000FF;
        cpu[current_thread].fcsr = val;
        dirty_fp_state();
        break;
    case CSR_SSTATUS:
        // Preserve sd, sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
        // Modify mxr, sum, fs, spp, spie, (upie=0), sie, (uie=0)
        val = (val & 0x00000000000C6122ULL) | (cpu[current_thread].mstatus & 0x0000000F00739888ULL);
        // Setting fs=1 or fs=2 will set fs=3
        if (val & 0x6000ULL)
        {
            val |= 0x6000ULL;
        }
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
        {
            val |= 0x8000000000000000ULL;
        }
        cpu[current_thread].mstatus = val;
        break;
    case CSR_SIE:
        // Only ssie, stie, and seie are writeable, and only if they are delegated
        // if mideleg[sei,sti,ssi]==1 then seie, stie, ssie is writeable, otherwise they are reserved
        msk = cpu[current_thread].mideleg & 0x0000000000000222ULL;
        val = (cpu[current_thread].mie & ~msk) | (val & msk);
        cpu[current_thread].mie = val;
        break;
    case CSR_STVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu[current_thread].stvec = val;
        stvec_is_set[current_thread] = true;
        break;
    case CSR_SCOUNTEREN:
        val &= 0x1FF;
        cpu[current_thread].scounteren = uint16_t(val);
        break;
    case CSR_SSCRATCH:
        cpu[current_thread].sscratch = val;
        break;
    case CSR_SEPC:
        // sepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu[current_thread].sepc = val;
        break;
    case CSR_SCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu[current_thread].scause = val;
        break;
    case CSR_STVAL:
        val = sextVA(val);
        cpu[current_thread].stval = val;
        break;
    case CSR_SIP:
        // Only ssip is writeable, and only if it is delegated
        msk = cpu[current_thread].mideleg & 0x0000000000000002ULL;
        val = (cpu[current_thread].mip & ~msk) | (val & msk);
        cpu[current_thread].mip = val;
        break;
    case CSR_SATP: // Shared register
        // MODE is 4 bits, ASID is 0bits, PPN is PPN_M bits
        val &= 0xF000000000000000ULL | PPN_M;
        switch (val >> 60)
        {
        case SATP_MODE_BARE:
        case SATP_MODE_SV39:
        case SATP_MODE_SV48:
            cpu[current_thread].satp = val;
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].satp = val;
            break;
        default: // reserved
            // do not write the register if attempting to set an unsupported mode
            break;
        }
        break;
    case CSR_MSTATUS:
        // Preserve sd, sxl, uxl, xs
        // Write all others (except upie=0, uie=0)
        val = (val & 0x00000000007E79AAULL) | (cpu[current_thread].mstatus & 0x0000000F00018000ULL);
        // Setting fs=1 or fs=2 will set fs=3
        if (val & 0x6000ULL)
        {
            val |= 0x6000ULL;
        }
        // Set sd if fs==3 or xs==3
        if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
        {
            val |= 0x8000000000000000ULL;
        }
        // Attempting to set mpp to 2 will set it to 0 instead
        if (((val >> 11) & 0x3) == 0x2)
            val &= ~(0x3ULL << 11);
        cpu[current_thread].mstatus = val;
        break;
    case CSR_MISA:
        // Writeable but hardwired
        val = CSR_ISA_MAX;
        break;
    case CSR_MEDELEG:
        // Not all exceptions can be delegated
        val &= 0x0000000000000B108ULL;
        cpu[current_thread].medeleg = val;
        break;
    case CSR_MIDELEG:
        // Not all interrupts can be delegated
        val &= 0x0000000000000222ULL;
        cpu[current_thread].mideleg = val;
        break;
    case CSR_MIE:
        // Hard-wire ueie, utie, usie
        val &= 0x0000000000890AAAULL;
        cpu[current_thread].mie = val;
        break;
    case CSR_MTVEC:
        val = sextVA(val & ~0xFFEULL);
        cpu[current_thread].mtvec = val;
        mtvec_is_set[current_thread] = true;
        break;
    case CSR_MCOUNTEREN:
        val &= 0x1FF;
        cpu[current_thread].mcounteren = uint16_t(val);
        break;
    case CSR_MHPMEVENT3:
    case CSR_MHPMEVENT4:
    case CSR_MHPMEVENT5:
    case CSR_MHPMEVENT6:
    case CSR_MHPMEVENT7:
    case CSR_MHPMEVENT8:
        val &= 0x1F;
        cpu[current_thread].mhpmevent[src1 - CSR_MHPMEVENT3] = val;
        break;
    case CSR_MHPMEVENT9:
    case CSR_MHPMEVENT10:
    case CSR_MHPMEVENT11:
    case CSR_MHPMEVENT12:
    case CSR_MHPMEVENT13:
    case CSR_MHPMEVENT14:
    case CSR_MHPMEVENT15:
    case CSR_MHPMEVENT16:
    case CSR_MHPMEVENT17:
    case CSR_MHPMEVENT18:
    case CSR_MHPMEVENT19:
    case CSR_MHPMEVENT20:
    case CSR_MHPMEVENT21:
    case CSR_MHPMEVENT22:
    case CSR_MHPMEVENT23:
    case CSR_MHPMEVENT24:
    case CSR_MHPMEVENT25:
    case CSR_MHPMEVENT26:
    case CSR_MHPMEVENT27:
    case CSR_MHPMEVENT28:
    case CSR_MHPMEVENT29:
    case CSR_MHPMEVENT30:
    case CSR_MHPMEVENT31:
        val = 0;
        break;
    case CSR_MSCRATCH:
        cpu[current_thread].mscratch = val;
        break;
    case CSR_MEPC:
        // mepc[0] = 0 always
        val = sextVA(val & ~1ULL);
        cpu[current_thread].mepc = val;
        break;
    case CSR_MCAUSE:
        // Maks all bits excepts the ones we implement
        val &= 0x800000000000001FULL;
        cpu[current_thread].mcause = val;
        break;
    case CSR_MTVAL:
        val = sextVA(val);
        cpu[current_thread].mtval = val;
        break;
    case CSR_MIP:
        // Only bus_error, mbad_red, seip, stip, ssip are writeable
        val &= 0x0000000000810222ULL;
        cpu[current_thread].mip = val;
        break;
    case CSR_TSELECT:
        val = 0;
        break;
    case CSR_TDATA1:
        if (debug_mode[current_thread])
        {
            // Preserve type, maskmax, timing; clearing dmode clears action too
            val = (val & 0x08000000000010DFULL) | (cpu[current_thread].tdata1 & 0xF7E0000000040000ULL);
            if (~val & 0x0800000000000000ULL)
            {
                val &= ~0x000000000000F000ULL;
            }
            cpu[current_thread].tdata1 = val;
            activate_breakpoints(PRV);
        }
        else if (~cpu[current_thread].tdata1 & 0x0800000000000000ULL)
        {
            // Preserve type, dmode, maskmax, timing, action
            val = (val & 0x00000000000000DFULL) | (cpu[current_thread].tdata1 & 0xFFE000000004F000ULL);
            cpu[current_thread].tdata1 = val;
            activate_breakpoints(PRV);
        }
        else
        {
            // Ignore writes to the register
            val = cpu[current_thread].tdata1;
        }
        break;
    case CSR_TDATA2:
        // keep only valid virtual or pysical addresses
        if ((~cpu[current_thread].tdata1 & 0x0800000000000000ULL) || debug_mode[current_thread])
        {
            val &= VA_M;
            cpu[current_thread].tdata2 = val;
        }
        else
        {
            val = cpu[current_thread].tdata2;
        }
        break;
    // unimplemented: TDATA3
    // TODO: DCSR
    // TODO: DPC
    // unimplemented: DSCRATCH0
    // unimplemented: DSCRATCH1
    case CSR_MCYCLE:
    case CSR_MINSTRET:
    case CSR_MHPMCOUNTER3:
    case CSR_MHPMCOUNTER4:
    case CSR_MHPMCOUNTER5:
    case CSR_MHPMCOUNTER6:
    case CSR_MHPMCOUNTER7:
    case CSR_MHPMCOUNTER8:
    case CSR_MHPMCOUNTER9:
    case CSR_MHPMCOUNTER10:
    case CSR_MHPMCOUNTER11:
    case CSR_MHPMCOUNTER12:
    case CSR_MHPMCOUNTER13:
    case CSR_MHPMCOUNTER14:
    case CSR_MHPMCOUNTER15:
    case CSR_MHPMCOUNTER16:
    case CSR_MHPMCOUNTER17:
    case CSR_MHPMCOUNTER18:
    case CSR_MHPMCOUNTER19:
    case CSR_MHPMCOUNTER20:
    case CSR_MHPMCOUNTER21:
    case CSR_MHPMCOUNTER22:
    case CSR_MHPMCOUNTER23:
    case CSR_MHPMCOUNTER24:
    case CSR_MHPMCOUNTER25:
    case CSR_MHPMCOUNTER26:
    case CSR_MHPMCOUNTER27:
    case CSR_MHPMCOUNTER28:
    case CSR_MHPMCOUNTER29:
    case CSR_MHPMCOUNTER30:
    case CSR_MHPMCOUNTER31:
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
    case CSR_HPMCOUNTER9:
    case CSR_HPMCOUNTER10:
    case CSR_HPMCOUNTER11:
    case CSR_HPMCOUNTER12:
    case CSR_HPMCOUNTER13:
    case CSR_HPMCOUNTER14:
    case CSR_HPMCOUNTER15:
    case CSR_HPMCOUNTER16:
    case CSR_HPMCOUNTER17:
    case CSR_HPMCOUNTER18:
    case CSR_HPMCOUNTER19:
    case CSR_HPMCOUNTER20:
    case CSR_HPMCOUNTER21:
    case CSR_HPMCOUNTER22:
    case CSR_HPMCOUNTER23:
    case CSR_HPMCOUNTER24:
    case CSR_HPMCOUNTER25:
    case CSR_HPMCOUNTER26:
    case CSR_HPMCOUNTER27:
    case CSR_HPMCOUNTER28:
    case CSR_HPMCOUNTER29:
    case CSR_HPMCOUNTER30:
    case CSR_HPMCOUNTER31:
    case CSR_MVENDORID:
    case CSR_MARCHID:
    case CSR_MIMPID:
    case CSR_MHARTID:
        throw trap_illegal_instruction(current_inst);
        break;
    // ----- Esperanto registers -------------------------------------
    case CSR_MATP: // Shared register
        // do not write the register if it is locked (L==1)
        if (~cpu[current_thread].matp & 0x800000000000000ULL)
        {
            // MODE is 4 bits, L is 1 bits, ASID is 0bits, PPN is PPN_M bits
            val &= 0xF800000000000000ULL | PPN_M;
            switch (val >> 60)
            {
            case MATP_MODE_BARE:
            case MATP_MODE_MV39:
            case MATP_MODE_MV48:
                cpu[current_thread].matp = val;
                if ((current_thread^1) < EMU_NUM_THREADS)
                    cpu[current_thread^1].matp = val;
                break;
            default: // reserved
                // do not write the register if attempting to set an unsupported mode
                break;
            }
        }
        break;
    case CSR_MINSTMASK:
        val &= 0x1ffffffffULL;
        cpu[current_thread].minstmask = val;
        break;
    case CSR_MINSTMATCH:
        val &= 0xffffffff;
        cpu[current_thread].minstmatch = val;
        break;
    // TODO: CSR_AMOFENCE_CTRL
    case CSR_CACHE_INVALIDATE:
        val &= 0x3;
        break;
    case CSR_MENABLE_SHADOWS:
        val &= 1;
        cpu[current_thread].menable_shadows = val;
        if ((current_thread^1) < EMU_NUM_THREADS)
            cpu[current_thread^1].menable_shadows = val;
        break;
    case CSR_EXCL_MODE:
        val &= 1;
        if (val)
        {
            cpu[current_thread].excl_mode = 1 + ((current_thread & 1) << 1);
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].excl_mode = 1 + ((current_thread & 1) << 1);
        }
        else
        {
            cpu[current_thread].excl_mode = 0;
            if ((current_thread^1) < EMU_NUM_THREADS)
                cpu[current_thread^1].excl_mode = 0;
        }
        break;
    case CSR_MBUSADDR:
        val = zextPA(val);
        cpu[current_thread].mbusaddr = val;
        break;
    case CSR_MCACHE_CONTROL:
        switch (cpu[current_thread].mcache_control)
        {
        case 0: msk = ((val & 3) == 1) ? 3 : 0; break;
        case 1: msk = ((val & 3) != 2) ? 3 : 0; break;
        case 3: msk = ((val & 3) != 2) ? 3 : 0; break;
        default: assert(0); break;
        }
        val = (val & msk) | (cpu[current_thread].ucache_control & ~msk);
        if (msk)
        {
            dcache_change_mode(cpu[current_thread].mcache_control, val);
            cpu[current_thread].ucache_control = val;
            cpu[current_thread].mcache_control = val & 3;
            if ((current_thread^1) < EMU_NUM_THREADS) {
                cpu[current_thread^1].ucache_control = val;
                cpu[current_thread^1].mcache_control = val & 3;
            }
            if ((~val & 2) && (current_thread != EMU_IO_SHIRE_SP_THREAD))
                tensorload_setupb_topair[current_thread] = false;
        }
        val &= 3;
#ifdef SYS_EMU
        if(sys_emu::get_coherency_check())
        {
            sys_emu::get_mem_directory().mcache_control_up((current_thread >> 1) / EMU_MINIONS_PER_SHIRE, (current_thread >> 1) % EMU_MINIONS_PER_SHIRE, cpu[current_thread].mcache_control);
        }
#endif
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
        tensor_reduce_start(val);
        break;
    case CSR_TENSOR_FMA:
        require_feature_ml_on_thread0();
        switch ((val >> 1) & 0x7)
        {
        case 0: tensor_fma32_start(val); break;
        case 1: tensor_fma16a32_start(val); break;
        case 3: tensor_ima8a32_start(val); break;
        default: throw trap_illegal_instruction(current_inst); break;
        }
        break;
    case CSR_TENSOR_CONV_SIZE:
        require_feature_ml();
        val &= 0xFF00FFFFFF00FFFFULL;
        cpu[current_thread].tensor_conv_size = val;
        tmask_conv();
        break;
    case CSR_TENSOR_CONV_CTRL:
        require_feature_ml();
        val &= 0x0000FFFF0000FFFFULL;
        cpu[current_thread].tensor_conv_ctrl = val;
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
        cpu[current_thread].tensor_mask = val;
        break;
    case CSR_TENSOR_QUANT:
        require_feature_ml_on_thread0();
        tensor_quant_start(val);
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
        val &= 0x3ff;
        cpu[current_thread].tensor_error = val;
        log_tensor_error_value(val);
        break;
    case CSR_UCACHE_CONTROL:
        require_feature_u_scratchpad();
        msk = (!(current_thread % EMU_THREADS_PER_MINION)
               && (cpu[current_thread].mcache_control & 1)) ? 1 : 3;
        val = (cpu[current_thread].mcache_control & msk) | (val & ~msk & 0x07df);
        assert((val & 3) != 2);
        dcache_change_mode(cpu[current_thread].mcache_control, val);
        cpu[current_thread].ucache_control = val;
        cpu[current_thread].mcache_control = val & 3;
        if ((current_thread^1) < EMU_NUM_THREADS) {
            cpu[current_thread^1].ucache_control = val;
            cpu[current_thread^1].mcache_control = val & 3;
        }
#ifdef SYS_EMU
        if(sys_emu::get_coherency_check())
        {
            sys_emu::get_mem_directory().mcache_control_up((current_thread >> 1) / EMU_MINIONS_PER_SHIRE, (current_thread >> 1) % EMU_MINIONS_PER_SHIRE, cpu[current_thread].mcache_control);
        }
#endif
        break;
    case CSR_PREFETCH_VA:
        require_feature_u_cacheops();
        dcache_prefetch_vaddr(val);
        break;
    // CSR_FLB is modelled outside this fuction!
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
        tensor_wait_start(val);
        break;
    case CSR_TENSOR_LOAD:
        require_feature_ml_on_thread0();
        tensor_load_start(val);
        break;
    case CSR_GSC_PROGRESS:
        val &= (VL-1);
        cpu[current_thread].gsc_progress = val;
        break;
    case CSR_TENSOR_LOAD_L2:
        require_feature_ml();
        tensorloadl2(val);
        break;
    case CSR_TENSOR_STORE:
        require_feature_ml_on_thread0();
        tensorstore(val);
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
            LOG_REG(":", 31);
            dcache_evict_flush_vaddr(src1 == CSR_EVICT_VA, tm, dest, vaddr, count, id, stride);
        }
        break;
    case CSR_VALIDATION0:
        cpu[current_thread].validation0 = val;
#ifdef SYS_EMU
        switch (val)
        {
        case 0x1FEED000:
            LOG_ALL_MINIONS(INFO, "%s", "Signal end test with PASS");
            sys_emu::deactivate_thread(current_thread);
            break;
        case 0x50BAD000:
            LOG_ALL_MINIONS(INFO, "%s", "Signal end test with FAIL");
            m_emu_done = true;
            break;
        }
#endif
        break;
    case CSR_VALIDATION1:
        switch ((val >> 56) & 0xFF)
        {
        case ET_DIAG_PUTCHAR:
            val = val & 0xFF;
            // EOT signals end of test
            if (val == 4)
            {
                LOG(INFO, "%s", "Validation1 CSR received End Of Transmission.");
                m_emu_done = true;
                break;
            }
            if (char(val) != '\n')
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
#ifdef SYS_EMU
        case ET_DIAG_IRQ_INJ:
            sys_emu::evl_dv_handle_irq_inj((val >> 55) & 1, (val >> 53) & 3, val & 0x3FFFFFFFFULL);
            break;
        case ET_DIAG_CYCLE:
            cpu[current_thread].validation1 = (val >> 56) & 0xFF;
            break;
#endif
        default:
            break;
        }
        break;
    case CSR_VALIDATION2:
        cpu[current_thread].validation2 = val;
        break;
    case CSR_VALIDATION3:
        cpu[current_thread].validation3 = val;
        break;
    case CSR_LOCK_VA:
        require_lock_unlock_enabled();
        {
            bool     tm     = (val >> 63) & 0x1;
            uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
            int      count  = (val & 0xF) + 1;
            uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
            int      id     = XREGS[31] & 0x0000000000000001ULL;
            LOG_REG(":", 31);
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
            LOG_REG(":", 31);
            dcache_unlock_vaddr(tm, vaddr, count, id, stride);
        }
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val &= 0x00000000030F0FF3ULL;
        val |= 0x0000000000008000ULL;
        cpu[current_thread].portctrl[src1 - CSR_PORTCTRL0] = val;
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
    // ----- All other registers -------------------------------------
    default:
        throw trap_illegal_instruction(current_inst);
    }

    return val;
}

// LCOV_EXCL_START
static void csr_insn(xreg dst, uint16_t src1, uint64_t oldval, uint64_t newval, bool read, bool write)
{
    // Check if current privilege mode has access to the register
    int curprv = PRV;
    int csrprv = (src1 >> 8) & 3;
    if (csrprv > curprv)
    {
        LOG(DEBUG, "Accessing a %c-mode CSR while in %c-mode", "USHM"[csrprv], "USHM"[curprv]);
        throw trap_illegal_instruction(current_inst);
    }
    if ((src1 == CSR_SATP) && ((cpu[current_thread].mstatus >> 20) & 1) && (curprv == PRV_S))
    {
        LOG(DEBUG, "Accessing SATP while in %c-mode and mstatus.tvm = %d (mstatus = 0x%016" PRIx64 ")",
            "USHM"[curprv], int((cpu[current_thread].mstatus >> 20) & 1), cpu[current_thread].mstatus);
        throw trap_illegal_instruction(current_inst);
    }
    if (read)
    {
        LOG_CSR(":", src1, oldval);
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
                newval = csrset(src1, newval);
                break;
        }
        LOG_CSR("=", src1, newval);
    }

    // the return value of mip.ssip should be set if external supervisor
    // interrupts are pending, but the RMO part of the csrrw/s/c instruction
    // should not take this into account
    switch (src1)
    {
        case CSR_SIP:
            oldval |= ext_seip[current_thread] & cpu[current_thread].mideleg;
            break;
        case CSR_MIP:
            oldval |= ext_seip[current_thread];
            break;
        default:
            break;
    }
    WRITE_REG(dst, oldval, write && (src1 == CSR_FLB));
}

void ecall(const char* comm __attribute__((unused)))
{
    DISASM_NOARG("ecall");
    switch (PRV)
    {
        case PRV_U: throw trap_user_ecall(); break;
        case PRV_S: throw trap_supervisor_ecall(); break;
        case PRV_M: throw trap_machine_ecall(); break;
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
    uint64_t curprv = PRV;
    uint64_t mstatus = cpu[current_thread].mstatus;
    if (curprv == PRV_U || (curprv == PRV_S && (((mstatus >> 22) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    DISASM_NOARG("sret");
    log_pc_update(cpu[current_thread].sepc);
    // Take spie and spp
    uint64_t spie = (mstatus >> 5) & 0x1;
    prv_t    spp = prv_t((mstatus >> 8) & 0x1);
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set sie = spie, spie = 1, spp = U (0), prv = spp
    csrset(CSR_MSTATUS, mstatus_clean | (spie << 1) | (1 << 5));
    set_prv(spp);
    LOG(DEBUG, "Now running in %c mode", "USHM"[spp]);
}

void mret(const char* comm __attribute__((unused)))
{
    if (PRV != PRV_M)
      throw trap_illegal_instruction(current_inst);

    DISASM_NOARG("mret");
    log_pc_update(cpu[current_thread].mepc);
    // Take mpie and mpp
    uint64_t mstatus = cpu[current_thread].mstatus;
    uint64_t mpie = (mstatus >> 7) & 0x1;
    prv_t    mpp = prv_t((mstatus >> 11) & 0x3);
    // Clean mie, mpie and mpp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFE777ULL;
    // Set mie = mpie, mpie = 1, mpp = U (0), prv = mpp
    csrset(CSR_MSTATUS, mstatus_clean | (mpie << 3) | (1 << 7));
    set_prv(mpp);
    LOG(DEBUG, "Now running in %c mode", "USHM"[mpp]);
}

void wfi(const char* comm __attribute__((unused)))
{
    uint64_t curprv = PRV;
    uint64_t mstatus = cpu[current_thread].mstatus;
    if (curprv == PRV_U || (curprv == PRV_S && (((mstatus >> 21) & 1) == 1)))
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
    LOG_REG(":", src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = XREGS[src2];
    csr_insn(dst, src1, oldval, newval, dst != x0, true);
}

void csrrs(xreg dst, uint16_t src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrs x%d, %s, x%d", PRVNAME, dst, csr_name(src1), src2);
    LOG_REG(":", src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | XREGS[src2];
    csr_insn(dst, src1, oldval, newval, true, src2 != x0);
}

void csrrc(xreg dst, uint16_t src1, xreg src2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrc x%d, %s, x%d", PRVNAME, dst, csr_name(src1), src2);
    LOG_REG(":", src2);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~XREGS[src2]);
    csr_insn(dst, src1, oldval, newval, true, src2 != x0);
}

void csrrwi(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrwi x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = imm;
    csr_insn(dst, src1, oldval, newval, dst != x0, true);
}

void csrrsi(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrsi x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | imm;
    csr_insn(dst, src1, oldval, newval, true, imm != 0);
}

void csrrci(xreg dst, uint16_t src1, uint64_t imm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I(%c): csrrci x%d, %s, 0x%016" PRIx64, PRVNAME, dst, csr_name(src1), imm);
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~imm);
    csr_insn(dst, src1, oldval, newval, true, imm != 0);
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

static void dcache_change_mode(uint8_t oldval, uint8_t newval)
{
    bool all_change = (oldval ^ newval) & 1;
    bool scp_change = (oldval ^ newval) & 2;
    bool scp_enabled = (newval & 2);

    if (!all_change && !scp_change)
        return;

    unsigned current_minion = current_thread / EMU_THREADS_PER_MINION;

    // clear locks
    if (all_change) {
        for (int i = 0; i < L1D_NUM_SETS; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                scp_locked[current_minion][i][j] = false;
                scp_trans[current_minion][i][j] = 0;
            }
        }
    }
    else if (scp_change) {
        for (int i = 0; i < L1D_NUM_SETS - 2; ++i) {
            for (int j = 0; j < L1D_NUM_WAYS; ++j) {
                scp_locked[current_minion][i][j] = false;
                scp_trans[current_minion][i][j] = 0;
            }
        }
    }

    // clear L1SCP
    if (scp_change && scp_enabled) {
        unsigned scratchpad_thread = current_minion * EMU_THREADS_PER_MINION;
        for (int i = 0; i < L1_SCP_ENTRIES; ++i) {
            scp[scratchpad_thread][i].u8.fill(0);
        }
    }
}

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
#ifdef SYS_EMU
            if(sys_emu::get_coherency_check())
            {
                unsigned shire = current_thread / EMU_THREADS_PER_SHIRE;
                unsigned minion = (current_thread / EMU_THREADS_PER_MINION) % EMU_MINIONS_PER_SHIRE;
                if(evict) sys_emu::get_mem_directory().l1_evict_sw(shire, minion, set, way);
                else      sys_emu::get_mem_directory().l1_flush_sw(shire, minion, set, way);
            }
#endif

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
            cacheop_type cop = CacheOp_None;
            if(evict)
            {
                if     (dest == 1) cop = CacheOp_EvictL2;
                else if(dest == 2) cop = CacheOp_EvictL3;
                else if(dest == 3) cop = CacheOp_EvictDDR;
            }
            else
            {
                if     (dest == 1) cop = CacheOp_FlushL2;
                else if(dest == 2) cop = CacheOp_FlushL3;
                else if(dest == 3) cop = CacheOp_FlushDDR;
            }
            paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), cop);
        }
        catch (const sync_trap_t& t)
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

static void dcache_prefetch_vaddr(uint64_t val)
{
    bool tm         = (val >> 63) & 0x1;
    int  dest       = (val >> 58) & 0x3;
    uint64_t vaddr  = val & 0x0000FFFFFFFFFFC0ULL;
    int  count      = (val & 0xF) + 1;
    uint64_t stride = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;
    //int      id   = XREGS[31] & 0x0000000000000001ULL;

    LOG_REG(":", 31);

    // Skip all if dest is MEM
    if (dest == 3)
        return;

    cacheop_type       cop = CacheOp_PrefetchL1;
    if     (dest == 1) cop = CacheOp_PrefetchL2;
    else if(dest == 2) cop = CacheOp_PrefetchL3;

    for (int i = 0; i < count; i++, vaddr += stride)
    {
        if (!tm || tmask_pass(i))
        {
            try {
                cache_line_t tmp;
                uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_Prefetch, mreg_t(-1), cop);
                bemu::pmemread512(paddr, tmp.u32.data());
                LOG_MEMREAD512(paddr, tmp.u32);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(1 << 7);
                return;
            }
            catch (const bemu::memory_error&) {
                raise_bus_error_interrupt(current_thread, 0);
            }
        }
    }
}

static void dcache_lock_paddr(int way, uint64_t paddr)
{
    if (!mmu_check_cacheop_access(paddr, CacheOp_Lock))
    {
        LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(1 << 7);
        return;
    }

    unsigned set = dcache_index(paddr, cpu[current_thread].mcache_control, current_thread, EMU_THREADS_PER_MINION);

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

    try {
        cache_line_t tmp;
        std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);
        bemu::pmemwrite512(paddr, tmp.u32.data());
        LOG_MEMWRITE512(paddr, tmp.u32);
    }
    catch (const sync_trap_t&) {
        LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d access fault", paddr, way);
        update_tensor_error(1 << 7);
        return;
    }
    catch (const bemu::memory_error&) {
        raise_bus_error_interrupt(current_thread, 0);
        return;
    }
    scp_locked[current_thread >> 1][set][way] = true;
    scp_trans[current_thread >> 1][set][way] = paddr;
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
    cache_line_t tmp;
    std::fill_n(tmp.u64.data(), tmp.u64.size(), 0);

    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        try {
            // LockVA is a hint, so no need to model soft-locking of the cache.
            // We just need to make sure we zero the cache line.
            uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), CacheOp_Lock);
            bemu::pmemwrite512(paddr, tmp.u32.data());
            LOG_MEMWRITE512(paddr, tmp.u32);
            LOG(DEBUG, "\tDoing LockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
        }
        catch (const sync_trap_t& t) {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tLockVA 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }
        catch (const bemu::memory_error&) {
            raise_bus_error_interrupt(current_thread, 0);
        }
    }
}

static void dcache_unlock_vaddr(bool tm, uint64_t vaddr, int numlines, int id __attribute__((unused)), uint64_t stride)
{
    for (int i = 0; i < numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i)) {
            LOG(DEBUG, "\tSkipping UnlockVA: 0x%016" PRIx64, vaddr);
            continue;
        }

        try {
            // Soft-locking of the cache is not modeled, so there is nothing more to do here.
            uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_CacheOp, mreg_t(-1), CacheOp_Unlock);
            LOG(DEBUG, "\tDoing UnlockVA: 0x%016" PRIx64 " (0x%016" PRIx64 ")", vaddr, paddr);
        }
        catch (const sync_trap_t& t) {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tUnlockVA: 0x%016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto messaging extension emulation
//
////////////////////////////////////////////////////////////////////////////////

bool get_msg_port_stall(unsigned thread, unsigned id)
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

static void write_msg_port_data_to_scp(unsigned thread, unsigned id, uint32_t *data, uint8_t oob)
{
    // Drop the write if port not configured
    if(!msg_ports[thread][id].enabled) return;

    if ( !scp_locked[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way] ) {
        LOG(DEBUG, "PORT_WRITE Port cache line (s%d w%d)  unlocked!", msg_ports[thread][id].scp_set, msg_ports[thread][id].scp_way);
    }
    uint64_t base_addr = scp_trans[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way];
    base_addr += msg_ports[thread][id].wr_ptr << msg_ports[thread][id].logsize;

    msg_ports[thread][id].stall = false;

    int wr_words = (1 << (msg_ports[thread][id].logsize))/4;

    LOG_ALL_MINIONS(DEBUG, "Writing MSG_PORT (m%u p%u) wr_words %d, logsize %u",  thread, id, wr_words, msg_ports[thread][id].logsize);
    for (int i = 0; i < wr_words; i++)
    {
        LOG_ALL_MINIONS(DEBUG, "Writing MSG_PORT (m%u p%u) data 0x%08" PRIx32 " to addr 0x%016" PRIx64,  thread, id, data[i], base_addr + 4 * i);
        bemu::pmemwrite<uint32_t>(base_addr + 4 * i, data[i]);
    }

    msg_ports[thread][id].size++;
    msg_ports[thread][id].wr_ptr = (msg_ports[thread][id].wr_ptr + 1) % (msg_ports[thread][id].max_msgs + 1);

    if (msg_ports[thread][id].enable_oob)
        msg_ports_oob[thread][id].push_back(oob);

    msg_to_thread(thread);
}

unsigned get_msg_port_write_width(unsigned thread, unsigned port)
{
    return 1 << msg_ports[thread][port].logsize;
}

void write_msg_port_data(unsigned thread, unsigned id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = current_thread;
        port_write.target_thread = thread;
        port_write.target_port   = id;
        port_write.is_tbox       = false;
        port_write.is_rbox       = false;

        for (unsigned b = 0; b < (1UL << msg_ports[thread][id].logsize)/4; b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_ALL_MINIONS(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from m%u", thread, id, current_thread);
        int wr_words = 1 << (msg_ports[thread][id].logsize)/4;
        for (int i = 0; i < wr_words; ++i)
            LOG_ALL_MINIONS(DEBUG, "                              data[%d] 0x%08" PRIx32, i, data[i]);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void write_msg_port_data_from_tbox(unsigned thread, unsigned id, unsigned tbox_id, uint32_t *data, uint8_t oob)
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

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from tbox%u", thread, id, tbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void write_msg_port_data_from_rbox(unsigned thread, unsigned id, unsigned rbox_id, uint32_t *data, uint8_t oob)
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

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from rbox%u", thread, id, rbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(thread, id, data, oob);
    }
}

void commit_msg_port_data(unsigned target_thread, unsigned port_id, unsigned source_thread)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG(DEBUG, "Commit write on MSG_PORT (h%u p%u) from h%u", target_thread, port_id, source_thread);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
    }
}

void commit_msg_port_data_from_tbox(unsigned target_thread, unsigned port_id, unsigned tbox_id)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG(DEBUG, "Commit write on MSG_PORT (m%u p%u) from tbox%u oob %d", target_thread, port_id, tbox_id, port_write.oob);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from tbox%u not found!!", target_thread, port_id, tbox_id);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from tbox%u not found!!", target_thread, port_id, tbox_id);
    }
}

void commit_msg_port_data_from_rbox(unsigned target_thread, unsigned port_id, unsigned rbox_id)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG(DEBUG, "Commit write on MSG_PORT (m%u p%u) from rbox%u", target_thread, port_id, rbox_id);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from rbox%u not found!!", target_thread, port_id, rbox_id);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from rbox%u not found!!", target_thread, port_id, rbox_id);
    }
}

static int64_t port_get(unsigned id, bool block)
{
    if (((PRV == PRV_U) && !msg_ports[current_thread][id].umode) || !msg_ports[current_thread][id].enabled)
    {
        throw trap_illegal_instruction(current_inst);
    }

    if (msg_port_empty(current_thread,id))
    {
        LOG(DEBUG, "Blocking MSG_PORT%s (m%u p%u) wr_ptr=%d, rd_ptr=%d", block ? "" : "NB", current_thread, id,
            msg_ports[current_thread][id].wr_ptr, msg_ports[current_thread][id].rd_ptr);

        if (!block)
            return -1;

#ifdef SYS_EMU
        // if in sysemu stop thread if no data for port.. comparing rd_ptr and wr_ptr
        LOG(DEBUG, "Stalling MSG_PORT (m%u p%u)", current_thread, id);
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

static void configure_port(unsigned id, uint64_t wdata)
{
    int scp_set = (wdata >> 16) & 0xFF;
    int scp_way = (wdata >> 24) & 0xFF;
    int logsize = (wdata >> 5)  & 0x07;

    if (scp_set >= L1D_NUM_SETS)
        scp_set = scp_set % L1D_NUM_SETS;

    if (scp_way >= L1D_NUM_WAYS)
        scp_way = scp_way % L1D_NUM_WAYS;

    if (logsize < PORT_LOG2_MIN_SIZE)
        logsize = PORT_LOG2_MIN_SIZE;
    else if (logsize > PORT_LOG2_MAX_SIZE)
        logsize = PORT_LOG2_MAX_SIZE;

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
    uint64_t tconvsizereg = cpu[current_thread].tensor_conv_size;
    int srow =   int8_t((tconvsizereg >> 56) & 0xFF);
    int nrow = uint16_t((tconvsizereg >> 32) & 0xFFFF);
    int scol =   int8_t((tconvsizereg >> 24) & 0xFF);
    int ncol = uint16_t((tconvsizereg >>  0) & 0xFFFF);

    // Get the positions of the convolution
    uint64_t tconvctrlreg = cpu[current_thread].tensor_conv_ctrl;
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
    cpu[current_thread].tensor_mask = tmask_value;
    LOG_TENSOR_MASK("=");
}

static void tcoop(uint64_t value)
{
    uint32_t neigh_mask  = (value >> 16) & 0xF;
    uint32_t minion_mask = (value >>  8) & 0xFF;
    uint32_t coop_id     = (value >>  0) & 0x1F;
    cpu[current_thread].tensor_coop = neigh_mask << 16
                                    | minion_mask << 8
                                    | coop_id;
    // TODO: implement functionality checking the addresses and tcoop of every use of Tensor Load
    LOG(DEBUG, "\tSetting Tensor Cooperation: coopneighmask=%02X, coopminmask=%02X, coopid=%d", neigh_mask, minion_mask, coop_id);
}

// ----- TensorLoad emulation --------------------------------------------------

void tensor_load_start(uint64_t control)
{
    uint64_t stride  = XREGS[31] & 0xFFFFFFFFFFC0ULL;
    int      id      = XREGS[31] & 1;

    int      tm                 = (control >> 63) & 0x1;
    int      use_coop           = (control >> 62) & 0x1;
    int      trans              = (control >> 59) & 0x7;
    int      dst                = (control >> 53) & 0x3F;
    int      tenb               = (control >> 52) & 0x1;
    uint64_t addr               = sext<48>(control & 0xFFFFFFFFFFC0ULL);
    int      boffset            = (control >>  4) & 0x03;
    int      rows               = ((control      ) & 0xF) + 1;

    LOG_REG(":", 31);
    LOG(DEBUG, "\tStart TensorLoad with tm: %d, use_coop: %d, trans: %d, dst: %d, "
        "tenb: %d, addr: 0x%" PRIx64 ", boffset: %d, rows: %d, stride: 0x%" PRIx64 ", id: %d",
        tm, use_coop, trans, dst, tenb, addr, boffset, rows, stride, id);

#ifdef ZSIM
    bool txload_busy = (cpu[current_thread].txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL);
    if (txload_busy) {
        if (cpu[current_thread].shadow_txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_load_start() called while "
                                     "this thread's TensorLoad FSM is active");
    }
#else
    if (cpu[current_thread].txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_load_start() called while "
                                 "this thread's TensorLoad FSM is active");
    }
#endif

    // Cooperative tensor loads require the shire to be in cooperative mode
    if (use_coop) {
        uint64_t shire = current_thread / EMU_THREADS_PER_SHIRE;
        if (!esr_shire_coop_mode[shire])
            throw trap_illegal_instruction(current_inst);
    }

    // Check if SCP is enabled
    if (cpu[current_thread].mcache_control != 0x3) {
        LOG(WARN, "%s", "\tTensorLoad with SCP disabled!!");
        update_tensor_error(1 << 4);
        return;
    }

    if (tenb) {
        tensorload_setupb_topair[current_thread] = true;
        tensorload_setupb_numlines[current_thread] = rows;
        if ((current_thread^1) < EMU_NUM_THREADS) {
            tensorload_setupb_topair[current_thread^1] = true;
            tensorload_setupb_numlines[current_thread^1] = rows;
        }
    }
    else if ((trans == 0x3) || (trans == 0x4)) {
        // Invalid transformation
        LOG(WARN, "%s", "\tTensorLoad with illegal transform!!");
        update_tensor_error(1 << 1);
        return;
    }
    else if ((trans >= 0x5) && (trans <= 0x7)) {
        int size = 1 << ((trans & 0x3) - 1);
        if (size != 1 && size != 2 && size != 4) {
            LOG(WARN, "\tTensorLoad element size (%d) not valid!", size);
            update_tensor_error(1 << 1);
            return;
        }
    }

#ifdef ZSIM
    if (txload_busy) {
        cpu[current_thread].shadow_txload[int(tenb)] = control;
        cpu[current_thread].shadow_txstride[int(tenb)] = XREGS[31];
    } else {
        cpu[current_thread].txload[int(tenb)] = control;
        cpu[current_thread].txstride[int(tenb)] = XREGS[31];
    }
#else
    cpu[current_thread].txload[int(tenb)] = control;
    cpu[current_thread].txstride[int(tenb)] = XREGS[31];
    tensor_load_execute(tenb);
#endif
}

void tensor_load_execute(bool tenb)
{
    uint64_t txload = cpu[current_thread].txload[int(tenb)];
    uint64_t stride = cpu[current_thread].txstride[int(tenb)] & 0xFFFFFFFFFFC0ULL;
    int      id     = cpu[current_thread].txstride[int(tenb)] & 1;

    int      tm       = (txload >> 63) & 0x1;
    int      use_coop = (txload >> 62) & 0x1;
    int      trans    = (txload >> 59) & 0x7;
    int      dst      = (txload >> 53) & 0x3F;
    uint64_t addr     = sext<48>(txload & 0xFFFFFFFFFFC0ULL);
    int      boffset  = (txload >>  4) & 0x03;
    int      rows     = ((txload     ) & 0xF) + 1;

    assert(int(tenb) == int((txload >> 52) & 0x1));

    LOG(DEBUG, "\tExecute TensorLoad with tm: %d, use_coop: %d, trans: %d, dst: %d, "
        "tenb: %d, addr: 0x%" PRIx64 ", boffset: %d, rows: %d, stride: 0x%" PRIx64 ", id: %d",
        tm, use_coop, trans, dst, tenb, addr, boffset, rows, stride, id);

    if (txload == 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_load_execute() called while "
                                 "this thread's TensorLoad FSM is inactive");
    }

#ifdef SYS_EMU
    // Logs tensorload coop info
    if(use_coop)
    {
        sys_emu::coop_tload_add(current_thread, tenb, tenb ? 0 : id, cpu[current_thread].tensor_coop & 0xF, (cpu[current_thread].tensor_coop >> 8) & 0xFF, cpu[current_thread].tensor_coop >> 16);
    }
#endif

    int adj = 0;
    if (tenb)
    {
        // TenB is modelled as an extension to the SCP (these entries are not
        // accessible otherwise)
        dst = 0;
        adj = L1_SCP_ENTRIES;
        trans = 0x0;
        tm = 0;
    }

    log_tensor_load(trans, tenb, adj + (dst % L1_SCP_ENTRIES), tm ? cpu[current_thread].tensor_mask : 0xFFFF);

    //NO TRANS
    if (trans == 0x00)
    {
        LOG(DEBUG, "%s", "\tTensorLoad: No transformation");
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                try {
                    uint64_t vaddr = sextVA(addr + i*stride);
                    assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
                    uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_TxLoad, mreg_t(-1));
                    bemu::pmemread512(paddr, SCP[idx].u32.data());
                    LOG_MEMREAD512(paddr, SCP[idx].u32.data());
                    LOG_SCP_32x16("=", idx);
                    L1_SCP_CHECK_FILL(current_thread, idx, id);
                }
                catch (const sync_trap_t&) {
                    update_tensor_error(1 << 7);
                    goto tensor_load_execute_done;
                }
                catch (const bemu::memory_error&) {
                    raise_bus_error_interrupt(current_thread, 0);
                    continue;
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
            }
        }
    }
    //INTERLEAVE8
    else if (trans == 0x01)
    {
        LOG(DEBUG, "%s", "\tTensorLoad: Interleave8");
        boffset *= 16;
        LOG(DEBUG, "\t#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                bool dirty = false;
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(current_thread, idx, id);
                for (int r = 0; r < 4; ++r)
                {
                    try {
                        Packed<128> tmp;
                        uint64_t vaddr = sextVA(addr + boffset + (4*i+r)*stride);
                        assert(addr_is_size_aligned(vaddr, 16));
                        uint64_t paddr = vmemtranslate(vaddr, 16, Mem_Access_TxLoad, mreg_t(-1));
                        bemu::pmemread128(paddr, tmp.u32.data());
                        LOG_MEMREAD128(paddr, tmp.u32);
                        for (int c = 0; c < 16; ++c)
                            SCP[idx].u8[c*4 + r] = tmp.u8[c];
                    }
                    catch (const sync_trap_t&) {
                        update_tensor_error(1 << 7);
                        goto tensor_load_execute_done;
                    }
                    catch (const bemu::memory_error&) {
                        raise_bus_error_interrupt(current_thread, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty)
                {
                    log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
                    LOG_SCP_32x16("=", idx);
                }
            }
        }
    }
    //INTERLEAVE16
    else if (trans == 0x02)
    {
        LOG(DEBUG, "%s", "\tTensorLoad: Interleave16");
        boffset = (boffset & 0x2) * 16;
        LOG(DEBUG, "\t#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                bool dirty = false;
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(current_thread, idx, id);
                for (int r = 0; r < 2; ++r)
                {
                    try {
                        Packed<256> tmp;
                        uint64_t vaddr = sextVA(addr + boffset + (2*i+r)*stride);
                        assert(addr_is_size_aligned(vaddr, 32));
                        uint64_t paddr = vmemtranslate(vaddr, 32, Mem_Access_TxLoad, mreg_t(-1));
                        bemu::pmemread256(paddr, tmp.u32.data());
                        LOG_MEMREAD256(paddr, tmp.u32);
                        for (int c = 0; c < 16; ++c)
                            SCP[idx].u16[c*2 + r] = tmp.u16[c];
                    }
                    catch (const sync_trap_t&) {
                        update_tensor_error(1 << 7);
                        goto tensor_load_execute_done;
                    }
                    catch (const bemu::memory_error&) {
                        raise_bus_error_interrupt(current_thread, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty)
                {
                    log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
                    LOG_SCP_32x16("=", idx);
                }
            }
        }
    }
    //TRANSPOSE
    else if (trans == 0x05 || trans == 0x06 || trans==0x07)
    {
        cache_line_t tmp[64];
        uint64_t okay = 0;
        int size = (trans & 0x03);
        int offset = (trans == 0x7) ? 0 : ((trans == 0x5) ? (boffset*16) : ((boffset & 0x2) * 16));
        int elements = L1D_LINE_SIZE >> (size-1);
        size = 1 << (size-1);
        LOG(DEBUG, "\tTensorLoad: Transpose - elements:%d size:%d offset:%d", elements, size, offset);
        for (int j = 0; j < elements; ++j)
        {
            uint64_t vaddr = sextVA(addr + j*stride);
            assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
            try {
                uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_TxLoad, mreg_t(-1));
                bemu::pmemread512(paddr, tmp[j].u32.data());
                LOG_MEMREAD512(paddr, tmp[j].u32);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(1 << 7);
                goto tensor_load_execute_done;
            }
            catch (const bemu::memory_error&) {
                raise_bus_error_interrupt(current_thread, 0);
                continue;
            }
            okay |= 1ull << j;
        }
        for (int i = 0; i < rows; ++i)
        {
            if (((okay >> i) & 1) && (!tm || tmask_pass(i)))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(current_thread, idx, id);
                for (int j = 0; j < elements; ++j)
                {
                    switch (size) {
                    case 4: SCP[idx].u32[j] = tmp[j].u32[i+offset/4]; break;
                    case 2: SCP[idx].u16[j] = tmp[j].u16[i+offset/2]; break;
                    case 1: SCP[idx].u8[j] = tmp[j].u8[i+offset]; break;
                    }
                }
                log_tensor_load_scp_write(i, &SCP[idx].u64[0]);
                LOG_SCP_32x16("=", idx);
            }
        }
    }

tensor_load_execute_done:
    cpu[current_thread].txload[int(tenb)] = 0xFFFFFFFFFFFFFFFFULL;
#ifdef ZSIM
    if (cpu[current_thread].shadow_txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu[current_thread].txload[int(tenb)], cpu[current_thread].shadow_txload[int(tenb)]);
        std::swap(cpu[current_thread].txstride[int(tenb)], cpu[current_thread].shadow_txstride[int(tenb)]);
    }
#endif
}

// ----- TensorLoadL2Scp emulation --------------------------------------------------

void tensorloadl2(uint64_t control)//TranstensorloadL2
{
    uint64_t stride  = XREGS[31] & 0xFFFFFFFFFFC0ULL;
    uint32_t id      = XREGS[31] & 1ULL;

    int      tm      = (control >> 63) & 0x1;
    int      dst     = ((control >> 46) & 0x1FFFC)  + ((control >> 4)  & 0x3);
    uint64_t base    = control & 0xFFFFFFFFFFC0ULL;
    int      rows    = ((control     ) & 0xF) + 1;
    uint64_t addr    = sext<48>(base);

    LOG_REG(":", 31);
    LOG(DEBUG, "TensorLoadL2SCP: rows:%d - tm:%d - dst:%d - addr:0x%" PRIx64 " - stride: 0x%" PRIx64 " - id: %d",
        rows, tm, dst, addr, stride, id);

    uint64_t shire = current_thread / EMU_THREADS_PER_SHIRE;
    for (int i = 0; i < rows; ++i)
    {
        if (!tm || tmask_pass(i))
        {
            uint64_t l2scp_addr = L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * L1D_LINE_SIZE);
            uint64_t vaddr = sextVA(addr + i*stride);
            assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
            try {
                cache_line_t tmp;
                uint64_t paddr = vmemtranslate(vaddr, L1D_LINE_SIZE, Mem_Access_TxLoadL2Scp, mreg_t(-1));
                bemu::pmemread512(paddr, tmp.u32.data());
                LOG_MEMREAD512(paddr, tmp.u32);
                bemu::pmemwrite512(l2scp_addr, tmp.u32.data());
                LOG_MEMWRITE512(l2scp_addr, tmp.u32);
#ifdef SYS_EMU
                if(sys_emu::get_scp_check())
                {
                    sys_emu::get_scp_directory().l2_scp_fill(current_thread, dst + i, id, paddr);
                }
#endif
            }
            catch (const sync_trap_t&) {
                update_tensor_error(1 << 7);
                return;
            }
            catch (const bemu::memory_error&) {
                raise_bus_error_interrupt(current_thread, 0);
            }
        }
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

static void tensor_quant_start(uint64_t value)
{
    unsigned fstart = (value >> 57) & 0x1F;
    unsigned ncols  = (value >> 55) & 0x3;
    unsigned nrows  = (value >> 51) & 0xF;
    unsigned line   = (value >> 45) & 0x3F;

    ncols = (ncols + 1) * 4;
    nrows = nrows + 1;
    line = line % L1_SCP_ENTRIES;

    LOG(DEBUG, "\tStart TensorQuant with line: %u, rows: %u, cols: %u, regstart: %u, frm: %s",
        line, nrows, ncols, fstart, get_rounding_mode(frm()));

#ifdef ZSIM
    bool txquant_busy = (cpu[current_thread].txquant != 0xFFFFFFFFFFFFFFFFULL);
    if (txquant_busy) {
        if (cpu[current_thread].shadow_txquant != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_quant_start() called while "
                                     "this thread's TensorQuant FSM is active");
    }
#else
    if (cpu[current_thread].txquant != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_quant_start() called while "
                                 "this thread's TensorQuant FSM is active");
    }
#endif

    // TensorQuant raises illegal instruction exception when rounding mode is
    // invalid even if the transforms do not use FRM.
    set_rounding_mode(frm());

    // If a transformation needs the scratchpad, and the scratchpad is
    // disabled, then we set tensor_error and do nothing.
    for (int trans = 0; trans < TQUANT_MAX_TRANS; trans++) {
        int funct = (value >> (trans*4)) & 0xF;
        if (!funct) {
            if (trans == 0) {
                // Nothing to do, don't activate the state machine
                return;
            }
            break;
        }
        if ((funct >= 4) && (funct <= 7) &&
            (cpu[current_thread].mcache_control != 0x3)) {
            LOG(DEBUG, "\tTransformation %d is %s but scratchpad is disabled",
                trans, get_quant_transform(funct));
            update_tensor_error(1 << 4);
            // Error, don't activate the state machine
            return;
        }
    }

    // Activate the state machine
#ifdef ZSIM
    if (txquant_busy) {
        cpu[current_thread].shadow_txquant = value;
    } else {
        cpu[current_thread].txquant = value;
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::Quant));
    }
#else
    cpu[current_thread].txquant = value;
    tensor_quant_execute();
#endif
}

void tensor_quant_execute()
{
    uint64_t quant = cpu[current_thread].txquant;
    unsigned fstart = (quant >> 57) & 0x1F;
    unsigned ncols  = (quant >> 55) & 0x3;
    unsigned nrows  = (quant >> 51) & 0xF;
    unsigned line   = (quant >> 45) & 0x3F;

#ifdef ZSIM
    if (cpu[current_thread].active_tensor_op() != Processor::Tensor::Quant) {
        throw std::runtime_error("tensor_quant_execute() called while this "
                                 "thread's TensorQuant FSM is inactive");
    }
#else
    if (cpu[current_thread].txquant == 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_quant_execute() called while this "
                                 "thread's TensorQuant FSM is inactive");
    }
#endif

    ncols = (ncols + 1) * 4;
    nrows = nrows + 1;
    line = line % L1_SCP_ENTRIES;

    LOG(DEBUG, "\tExecute TensorQuant with line: %u, rows: %u, cols: %u, regstart: %u, frm: %s",
        line, nrows, ncols, fstart, get_rounding_mode(frm()));

    set_rounding_mode(frm());

    for (unsigned trans = 0; trans < TQUANT_MAX_TRANS; ++trans) {
        unsigned funct = (quant >> (trans*4)) & 0xF;
        LOG(DEBUG, "\tTransformation %d: %s", trans, get_quant_transform(funct));
        if (!funct) {
            break;
        }
        // PACK_128B RTL operates on even registers first, and then on odd
        // registers, so it generates two writes to the destination register
        // when a row spans a vector.
        log_tensor_quant_new_transform((funct == 10) && (ncols > VL));

        for (unsigned row = 0; row < nrows; ++row) {
            for (unsigned col = 0; col < ncols; col += VL) {
                unsigned nelem = std::min(ncols - col, VL);
                unsigned fs1 = (fstart + row*2 + col/VL) % NFREGS;
                unsigned fd = (funct == 10) ? ((fstart + row*2) % NFREGS) : fs1;
                switch (funct) {
                case 1: // INT32_TO_FP32
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::i32_to_f32(FREGS[fd].i32[e]);
                    }
                    LOG_FREG("=", fd);
                    set_fp_exceptions();
                    dirty_fp_state();
                    break;
                case 2: // FP32_TO_INT32
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = fpu::f32_to_i32(FREGS[fd].f32[e]);
                    }
                    LOG_FREG("=", fd);
                    set_fp_exceptions();
                    dirty_fp_state();
                    break;
                case 3: // INT32_RELU
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = std::max(int32_t(0), FREGS[fd].i32[e]);
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 4: // INT32_ADD_ROW
                    LOG_SCP(":", line, col);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = FREGS[fd].i32[e] + SCP[line].i32[col+e];
                    }
                    L1_SCP_CHECK_READ(current_thread, line);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 5: // INT32_ADD_COL
                    LOG_SCP_32x1(":", line, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = FREGS[fd].i32[e] + SCP[line].i32[row];
                    }
                    L1_SCP_CHECK_READ(current_thread, line);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 6: // FP32_MUL_ROW
                    LOG_SCP(":", line, col);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e], SCP[line].f32[col+e]);
                    }
                    L1_SCP_CHECK_READ(current_thread, line);
                    LOG_FREG("=", fd);
                    set_fp_exceptions();
                    dirty_fp_state();
                    break;
                case 7: // FP32_MUL_COL
                    LOG_SCP_32x1(":", line, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e], SCP[line].f32[row]);
                    }
                    L1_SCP_CHECK_READ(current_thread, line);
                    LOG_FREG("=", fd);
                    set_fp_exceptions();
                    dirty_fp_state();
                    break;
                case 8: // SATINT8
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = std::min(int32_t(127), std::max(int32_t(-128), FREGS[fd].i32[e])) & 0xFF;
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 9: // SATUINT8
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = std::min(int32_t(255), std::max(int32_t(0), FREGS[fd].i32[e])) & 0xFF;
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 10: // PACK_128B
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].u8[col+e] = uint8_t(FREGS[fs1].u32[e]);
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                default:
                    throw std::runtime_error("Illegal TensorQuant transform!");
                    break;
                }

                // Notify the checker
                if (funct == 10) {
                    log_tensor_quant_write(trans, fd, mkmask(nelem/4) << (col/4), FREGS[fd]);
                } else {
                    log_tensor_quant_write(trans, fd, mkmask(nelem), FREGS[fd]);
                }
            }
        }

        if ((funct >= 4) && (funct <= 7)) {
            line = (line + 1) % L1_SCP_ENTRIES;
        }
    }
    cpu[current_thread].txquant = 0xFFFFFFFFFFFFFFFFULL;
#ifdef ZSIM
    assert(cpu[current_thread].dequeue_tensor_op() == Processor::Tensor::Quant);
    if (cpu[current_thread].shadow_txquant != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu[current_thread].txquant, cpu[current_thread].shadow_txquant);
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::Quant));
    }
#endif
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
        int      src      = scpstart % L1_SCP_ENTRIES;

        uint64_t stride   = XREGS[31] & 0x0000FFFFFFFFFFC0ULL;

        LOG_REG(":", 31);
        LOG(DEBUG, "\tStart TensorStoreFromScp with addr: %016" PRIx64 ", stride: %016" PRIx64 ", rows: %d, scpstart: %d, srcinc: %d", addr, stride, rows, src, srcinc);

        log_tensor_store(true, rows, 4, 1);

        // Check if L1 SCP is enabled
        if (cpu[current_thread].mcache_control != 0x3)
        {
            update_tensor_error(1 << 4);
            log_tensor_store_error(1 << 4);
            return;
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
            LOG_SCP_32x16(":", src);
            try {
                uint64_t paddr = vmemtranslate(addr, L1D_LINE_SIZE, Mem_Access_TxStore, mreg_t(-1));
                bemu::pmemwrite512(paddr, SCP[src].u32.data());
                LOG_MEMWRITE512(paddr, SCP[src].u32);
                for (int col=0; col < 16; col++) {
                    log_tensor_store_write(paddr + col*4, SCP[src].u32[col]);
                }
                L1_SCP_CHECK_READ(current_thread, src);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(1 << 7);
                log_tensor_store_error(1 << 7);
                return;
            }
            catch (const bemu::memory_error&) {
                raise_bus_error_interrupt(current_thread, 0);
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

        LOG_REG(":", 31);
        LOG(DEBUG, "\tStart TensorStore with addr: %016" PRIx64 ", stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, srcinc: %d, coop: %d",
            addr, stride, regstart, rows, cols, srcinc, coop);

        // Check legal coop combination
        // xs[50:49]/xs[56:55]
        static const bool coop_comb[4*4] = {
            true,  true,  false, true,
            true,  true,  false, false,
            false, false, false, false,
            true,  false, false, false
        };

        log_tensor_store(false, rows, cols, coop);

        if (!coop_comb[4*(coop-1)+(cols-1)])
        {
            update_tensor_error(1 << 8);
            log_tensor_store_error(1 << 8);
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
        int src = regstart;
        uint64_t mask = ~(16ull*cols - 1ull);
        for (int row = 0; row < rows; row++)
        {
            // For all the blocks of 128b
            for (int col = 0; col < cols; col++)
            {
                try {
                    uint64_t vaddr = sextVA((addr + row * stride) & mask);
                    uint64_t paddr = vmemtranslate(vaddr + col*16, 16, Mem_Access_TxStore, mreg_t(-1));
                    if (!(col & 1)) LOG_FREG(":", src);
                    const uint32_t* ptr = &FREGS[src].u32[(col & 1) * 4];
                    bemu::pmemwrite128(paddr, ptr);
                    LOG_MEMWRITE128(paddr, ptr);
                    for (int w=0; w < 4; w++) {
                        log_tensor_store_write(paddr + w*4, *(ptr+w));
                    }
                }
                catch (const sync_trap_t&) {
                    update_tensor_error(1 << 7);
                    log_tensor_store_error(1 << 7);
                    return;
                }
                catch (const bemu::memory_error&) {
                    raise_bus_error_interrupt(current_thread, 0);
                }
                // For 128b stores, move to next desired register immediately.
                // For 256b and 512b stores, move to next desired register
                // when 256b are written
                if ((cols == 1) || (col & 1)) src = (src + srcinc) % NFREGS;
            }
        }
    }
}

// ----- TensorFMA emulation ---------------------------------------------------

static void tensor_fma32_execute()
{
    bool usemsk     = (cpu[current_thread].txfma >> 63) & 0x1;
    int  bcols      = (cpu[current_thread].txfma >> 55) & 0x3;
    int  arows      = (cpu[current_thread].txfma >> 51) & 0xF;
    int  acols      = (cpu[current_thread].txfma >> 47) & 0xF;
    int  aoffset    = (cpu[current_thread].txfma >> 43) & 0xF;
    bool tenb       = (cpu[current_thread].txfma >> 20) & 0x1;
    int  bstart     = (cpu[current_thread].txfma >> 12) & 0x3F;
    int  astart     = (cpu[current_thread].txfma >>  4) & 0x3F;
    bool first_pass = (cpu[current_thread].txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        bool resolved = sys_emu::coop_tload_check(current_thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
            cpu[current_thread].wait.value = 0; // Marks FMA32 for replay
            LOG(DEBUG, "TensorFMA32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu[current_thread].wait.state = Processor::Wait::State::Idle;
        }
    }
    else
    {
        cpu[current_thread].wait.state = Processor::Wait::State::Idle;
    }
#endif

    LOG(DEBUG, "\tExecute TensorFMA32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(frm()));
    LOG_TENSOR_MASK(":");

    set_rounding_mode(frm());
    for (int k = 0; k < acols; ++k)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES)];
        if(!tenb) L1_SCP_CHECK_READ(current_thread, ((bstart+k)%L1_SCP_ENTRIES));

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
                        LOG(DEBUG, "\tTensorFMA32(0) f%u[%u] = 0x0", i*TFMA_REGS_PER_ROW+j/VL, j%VL);
                        log_tensor_fma_write(0, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, 0);
                    }
                }
                continue;
            }

            uint32_t a_scp_entry = (astart+i) % L1_SCP_ENTRIES;
            float32_t a = SCP[a_scp_entry].f32[(aoffset+k) % (L1D_LINE_SIZE/4)];
            L1_SCP_CHECK_READ(current_thread, a_scp_entry);

            // If first_pass is 1 and this is the first iteration we do FMUL
            // instead of FMA
            if (first_pass && !k)
            {
                for (int j = 0; j < bcols; ++j)
                {
                    float32_t b = tmpb.f32[j];
                    float32_t c = fpu::f32_mul(a, b);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = fpu::UI32(c);
                    LOG(DEBUG, "\tTensorFMA32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(a), fpu::UI32(b));
                    log_tensor_fma_write(k, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
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
                    LOG(DEBUG, "\tTensorFMA32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(c0), fpu::UI32(a), fpu::UI32(b));
                    log_tensor_fma_write(k, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%u[%u] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}

static void tensor_fma32_start(uint64_t tfmareg)
{
    bool usemsk     = (tfmareg >> 63) & 0x1;
    int  bcols      = (tfmareg >> 55) & 0x3;
    int  arows      = (tfmareg >> 51) & 0xF;
    int  acols      = (tfmareg >> 47) & 0xF;
    int  aoffset    = (tfmareg >> 43) & 0xF;
    bool tenb       = (tfmareg >> 20) & 0x1;
    int  bstart     = (tfmareg >> 12) & 0x3F;
    int  astart     = (tfmareg >>  4) & 0x3F;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

    LOG(DEBUG, "\tStart TensorFMA32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(frm()));

#ifdef ZSIM
    bool txfma_busy = (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu[current_thread].shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_fma32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Illegal instruction exception has higher priority than other errors
    set_rounding_mode(frm());

    // Unpair the last TensorLoadSetupB
    bool load_tenb = tensorload_setupb_topair[current_thread];
    int  brows_tenb = tensorload_setupb_numlines[current_thread];
    tensorload_setupb_topair[current_thread] = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        tensorload_setupb_topair[current_thread^1] = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // rows/columns size, or not tenb and orphaned TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu[current_thread].mcache_control != 3)
            update_tensor_error((1 << 4) | (1 << 6));
        else
            update_tensor_error(1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu[current_thread].mcache_control != 3) {
        update_tensor_error(1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu[current_thread].shadow_txfma = tfmareg;
    } else {
        cpu[current_thread].txfma = tfmareg;
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::FMA));
    }
#elif SYS_EMU
    cpu[current_thread].txfma = tfmareg;
    cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
#else
    cpu[current_thread].txfma = tfmareg;
    tensor_fma32_execute();
    cpu[current_thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}

static void tensor_fma16a32_execute()
{
    bool usemsk     = (cpu[current_thread].txfma >> 63) & 0x1;
    int  bcols      = (cpu[current_thread].txfma >> 55) & 0x3;
    int  arows      = (cpu[current_thread].txfma >> 51) & 0xF;
    int  acols      = (cpu[current_thread].txfma >> 47) & 0xF;
    int  aoffset    = (cpu[current_thread].txfma >> 43) & 0xF;
    bool tenb       = (cpu[current_thread].txfma >> 20) & 0x1;
    int  bstart     = (cpu[current_thread].txfma >> 12) & 0x3F;
    int  astart     = (cpu[current_thread].txfma >>  4) & 0x3F;
    bool first_pass = (cpu[current_thread].txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 2;
    aoffset = aoffset * 2;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        bool resolved = sys_emu::coop_tload_check(current_thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
            cpu[current_thread].wait.value = 1; // Marks FMA16A32 for replay
            LOG(DEBUG, "TensorFMA16A32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu[current_thread].wait.state = Processor::Wait::State::Idle;
        }
    }
    else
    {
        cpu[current_thread].wait.state = Processor::Wait::State::Idle;
    }
#endif

    LOG(DEBUG, "\tExecute TensorFMA16A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(rtz));

    LOG_TENSOR_MASK(":");

    set_rounding_mode(rtz);
    for (int k = 0; k < acols; k += 2)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES)];
        if(!tenb) L1_SCP_CHECK_READ(current_thread, ((bstart+k/2)%L1_SCP_ENTRIES));

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
                        LOG(DEBUG, "\tTensorFMA16A32(0) f%u[%u] = 0x0", i*TFMA_REGS_PER_ROW+j/VL, j%VL);
                        log_tensor_fma_write(0, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, 0);
                    }
                }
                continue;
            }

            uint32_t a_scp_entry = (astart+i) % L1_SCP_ENTRIES;
            float16_t a1 = SCP[a_scp_entry].f16[(aoffset+k+0) % (L1D_LINE_SIZE/2)];
            float16_t a2 = SCP[a_scp_entry].f16[(aoffset+k+1) % (L1D_LINE_SIZE/2)];
            L1_SCP_CHECK_READ(current_thread, a_scp_entry);

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
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%u[%u]: 0x%08" PRIx32 " = (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                    log_tensor_fma_write(k/2, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
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
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VL, j%VL, fpu::UI32(c), fpu::UI32(c0), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                    log_tensor_fma_write(k/2, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%u[%u] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}

static void tensor_fma16a32_start(uint64_t tfmareg)
{
    bool usemsk     = (tfmareg >> 63) & 0x1;
    int  bcols      = (tfmareg >> 55) & 0x3;
    int  arows      = (tfmareg >> 51) & 0xF;
    int  acols      = (tfmareg >> 47) & 0xF;
    int  aoffset    = (tfmareg >> 43) & 0xF;
    bool tenb       = (tfmareg >> 20) & 0x1;
    int  bstart     = (tfmareg >> 12) & 0x3F;
    int  astart     = (tfmareg >>  4) & 0x3F;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 2;
    aoffset = aoffset * 2;

    LOG(DEBUG, "\tStart TensorFMA16A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(rtz));

#ifdef ZSIM
    bool txfma_busy = (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu[current_thread].shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_fma16a32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma16a32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Unpair the last TensorLoadSetupB
    bool load_tenb = tensorload_setupb_topair[current_thread];
    int  brows_tenb = 2 * tensorload_setupb_numlines[current_thread];
    tensorload_setupb_topair[current_thread] = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        tensorload_setupb_topair[current_thread^1] = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // combination of rows and columns length, or not tenb and orphaned
    // TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu[current_thread].mcache_control != 3)
            update_tensor_error((1 << 4) | (1 << 6));
        else
            update_tensor_error(1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu[current_thread].mcache_control != 3) {
        update_tensor_error(1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu[current_thread].shadow_txfma = tfmareg;
    } else {
        cpu[current_thread].txfma = tfmareg;
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::FMA));
    }
#elif SYS_EMU
    cpu[current_thread].txfma = tfmareg;
    cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
#else
    cpu[current_thread].txfma = tfmareg;
    tensor_fma16a32_execute();
    cpu[current_thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}

static void tensor_ima8a32_execute()
{
    bool usemsk     = (cpu[current_thread].txfma >> 63) & 0x1;
    int  bcols      = (cpu[current_thread].txfma >> 55) & 0x3;
    int  arows      = (cpu[current_thread].txfma >> 51) & 0xF;
    int  acols      = (cpu[current_thread].txfma >> 47) & 0xF;
    int  aoffset    = (cpu[current_thread].txfma >> 43) & 0xF;
    bool tenc2rf    = (cpu[current_thread].txfma >> 23) & 0x1;
    bool ub         = (cpu[current_thread].txfma >> 22) & 0x1;
    bool ua         = (cpu[current_thread].txfma >> 21) & 0x1;
    bool tenb       = (cpu[current_thread].txfma >> 20) & 0x1;
    int  bstart     = (cpu[current_thread].txfma >> 12) & 0x3F;
    int  astart     = (cpu[current_thread].txfma >>  4) & 0x3F;
    bool first_pass = (cpu[current_thread].txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 4;
    aoffset = aoffset * 4;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        bool resolved = sys_emu::coop_tload_check(current_thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
            cpu[current_thread].wait.value = 3; // Marks IMA8A32 for replay
            LOG(DEBUG, "TensorIMA8A32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu[current_thread].wait.state = Processor::Wait::State::Idle;
        }
    }
    else
    {
        cpu[current_thread].wait.state = Processor::Wait::State::Idle;
    }
#endif

#ifdef ZSIM
    LOG(DEBUG, "\tExecute TensorIMA8A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc2rf: %d, tenb: %d, bstart: %d, astart: %d",
        usemsk, aoffset, first_pass, bcols, acols, arows, ub, ua, tenc2rf, tenb, bstart, astart);
#endif

    LOG_TENSOR_MASK(":");

    for (int k = 0; k < acols; k += 4)
    {
        log_tensor_fma_new_pass();

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES)];
        if(!tenb) L1_SCP_CHECK_READ(current_thread, ((bstart+k/4)%L1_SCP_ENTRIES));

        bool write_freg = (tenc2rf && (k+4 == acols));
        freg_t* dst = write_freg ? FREGS : TENC;
        const char* dname = write_freg ? "f" : "TenC";

        for (int i = 0; i < arows; ++i)
        {
            // We should skip computation for this row, but:
            // * if first_pass is set and this is the first iteration then we still set TenC to 0
            // * if tenc2rf is set and we are in the last pass then we must copy TenC to FREGS even for this row.
            if (usemsk && !tmask_pass(i))
            {
                if (write_freg)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL] = (first_pass && !k) ? 0 : TENC[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL];
                        LOG(DEBUG, "\tTensorIMA8A32(%d) f%u[%u] = 0x%08" PRIx32, k/4, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                        log_tensor_fma_write(k/4, true, i*TFMA_REGS_PER_ROW+j/VL, j%VL, FREGS[i*TFMA_REGS_PER_ROW + j/VL].u32[j%VL]);
                    }
                }
                else if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        TENC[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL] = 0;
                        log_tensor_fma_write(0, false, i*TFMA_REGS_PER_ROW+j/VL, j%VL, TENC[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
                    }
                }
                continue;
            }

            // If first_pass is 1 and this is the first iteration we do
            // a1*b1+a2*b2+a3*b3+a4*b4 instead of c0+a1*b1+a2*b2+a3*b3+a4*b4
            if (first_pass && !k)
            {
#define ASRC(x) SCP[(astart+i) % L1_SCP_ENTRIES].u8[(aoffset+k+(x)) % L1D_LINE_SIZE]
                int32_t a1 = ua ? ASRC(0) : sext8_2(ASRC(0));
                int32_t a2 = ua ? ASRC(1) : sext8_2(ASRC(1));
                int32_t a3 = ua ? ASRC(2) : sext8_2(ASRC(2));
                int32_t a4 = ua ? ASRC(3) : sext8_2(ASRC(3));
#undef ASRC
                L1_SCP_CHECK_READ(current_thread, (astart+i) % L1_SCP_ENTRIES);
                for (int j = 0; j < bcols; ++j)
                {
#define BSRC(x) tmpb.u8[j*4+(x)]
                    int32_t b1 = ub ? BSRC(0) : sext8_2(BSRC(0));
                    int32_t b2 = ub ? BSRC(1) : sext8_2(BSRC(1));
                    int32_t b3 = ub ? BSRC(2) : sext8_2(BSRC(2));
                    int32_t b4 = ub ? BSRC(3) : sext8_2(BSRC(3));
#undef BSRC
                    int32_t c = (a1 * b1) + (a2 * b2) + (a3 * b3) + (a4 * b4);
                    dst[i*TFMA_REGS_PER_ROW+j/VL].i32[j%VL] = c;
                    LOG(DEBUG, "\tTensorIMA8A32(%d) %s%u[%u]: 0x%08" PRIx32 " = (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ")",
                        k/4, dname, i*TFMA_REGS_PER_ROW+j/VL, j%VL, c, uint8_t(a1), uint8_t(b1), uint8_t(a2), uint8_t(b2), uint8_t(a3), uint8_t(b3), uint8_t(a4), uint8_t(b4));
                    log_tensor_fma_write(k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VL, j%VL, uint32_t(c));
                }
            }
            // If all products are 0, we can skip the operation, except if TenC must
            // be copied to FREGS and this is the last iteration. NB: The detection
            // is done at 32-bit granularity, not at element (8-bit) granularity.
            else if (write_freg || SCP[(astart+i) % L1_SCP_ENTRIES].u32[((aoffset+k)/4) % (L1D_LINE_SIZE/4)])
            {
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
                    // If all products are 0 for both column @j and column @j+8 or @j-8, we can skip the
                    // operation, except if TenC must be copied to FREGS and this is the last iteration.
                    // NB: The detection is done at 32-bit granularity, not at element (8-bit) granularity
                    if (j >= 8)
                    {
                        if (!write_freg && (tmpb.u32[j] == 0) && (tmpb.u32[j-8] == 0))
                            continue;
                    }
                    else
                    {
                        if (!write_freg && (tmpb.u32[j] == 0) && ((j+8 >= bcols) || (tmpb.u32[j+8] == 0)))
                            continue;
                    }
                    int32_t c0 = TENC[i*TFMA_REGS_PER_ROW+j/VL].i32[j%VL];
                    int32_t c = c0 + (a1 * b1) + (a2 * b2) + (a3 * b3) + (a4 * b4);
                    dst[i*TFMA_REGS_PER_ROW+j/VL].i32[j%VL] = c;
                    LOG(DEBUG, "\tTensorIMA8A32(%d) %s%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ")",
                        k/4, dname, i*TFMA_REGS_PER_ROW+j/VL, j%VL, c, c0, uint8_t(a1), uint8_t(b1), uint8_t(a2), uint8_t(b2), uint8_t(a3), uint8_t(b3), uint8_t(a4), uint8_t(b4));
                    log_tensor_fma_write(k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VL, j%VL, uint32_t(c));
                }
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
            LOG(DEBUG, "\tC[%d][%d]: %s%u[%u] = 0x%08" PRIx32, i, j, dname,
                i*TFMA_REGS_PER_ROW+j/VL, j%VL, dst[i*TFMA_REGS_PER_ROW+j/VL].u32[j%VL]);
    }
}

static void tensor_ima8a32_start(uint64_t tfmareg)
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
    int  bstart     = (tfmareg >> 12) & 0x3F;
    int  astart     = (tfmareg >>  4) & 0x3F;
    bool first_pass = (tfmareg >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 4;
    aoffset = aoffset * 4;

    LOG(DEBUG, "\tStart TensorIMA8A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc2rf: %d, tenb: %d, bstart: %d, astart: %d",
        usemsk, aoffset, first_pass, bcols, acols, arows, ub, ua, tenc2rf, tenb, bstart, astart);

#ifdef ZSIM
    bool txfma_busy = (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu[current_thread].shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_ima8a32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu[current_thread].txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_ima8a32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Unpair the last TensorLoadSetupB
    bool load_tenb = tensorload_setupb_topair[current_thread];
    int  brows_tenb = 4 * tensorload_setupb_numlines[current_thread];
    tensorload_setupb_topair[current_thread] = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        tensorload_setupb_topair[current_thread^1] = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // combination of rows and columns length, or not tenb and orphaned
    // TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu[current_thread].mcache_control != 3)
            update_tensor_error((1 << 4) | (1 << 6));
        else
            update_tensor_error(1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu[current_thread].mcache_control != 3) {
        update_tensor_error(1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu[current_thread].shadow_txfma = tfmareg;
    } else {
        cpu[current_thread].txfma = tfmareg;
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::FMA));
    }
#elif SYS_EMU
    cpu[current_thread].txfma = tfmareg;
    cpu[current_thread].wait.state = Processor::Wait::State::TxFMA;
#else
    cpu[current_thread].txfma = tfmareg;
    tensor_ima8a32_execute();
    cpu[current_thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}


void tensor_fma_execute()
{
#ifdef ZSIM
    if (cpu[current_thread].active_tensor_op() != Processor::Tensor::FMA) {
        throw std::runtime_error("tensor_fma_execute() called while "
                                 "TensorFMA FSM is inactive");
    }
#else
    if (cpu[current_thread].txfma == 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma_execute() called while "
                                 "this thread's TensorFMA FSM is inactive");
    }
#endif
    switch ((cpu[current_thread].txfma >> 1) & 0x7)
    {
    case 0: tensor_fma32_execute(); break;
    case 1: tensor_fma16a32_execute(); break;
    case 3: tensor_ima8a32_execute(); break;
    default: throw std::runtime_error("Illegal tensor_fma configuration");
    }
    if(cpu[current_thread].wait.state != Processor::Wait::State::TxFMA) {
        cpu[current_thread].txfma = 0xFFFFFFFFFFFFFFFFULL;
    }
#ifdef ZSIM
    assert(cpu[current_thread].dequeue_tensor_op() == Processor::Tensor::FMA);
    if (cpu[current_thread].shadow_txfma != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu[current_thread].txfma, cpu[current_thread].shadow_txfma);
        assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::FMA));
    }
#endif
}

// ----- TensorReduce emulation ------------------------------------------------

static void tensor_reduce_start(uint64_t value)
{
    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    unsigned type      = value & 3;
    unsigned level     = (value >> 3) & 0xF;
    unsigned distance  = 1 << level;
    unsigned minmask   = (1 << (level + 1)) - 1;
    unsigned operation = (value >> 24) & 0xF;

    if ((cpu[current_thread].reduce.state != Processor::Reduce::State::Idle) &&
        (cpu[current_thread].reduce.state != Processor::Reduce::State::Skip))
        throw std::runtime_error("tensor_reduce_start() called while "
                                 "this thread's TensorReduce FSM is active");

    if ((type != 0) && (operation == 0)) {
        // TensorRecv, TensorBroadcast and TensorReduce should raise illegal
        // instruction if the encoded operation is FADD and FRM does not hold
        // a valid rounding mode
        set_rounding_mode(frm());
    }

    cpu[current_thread].reduce.regid  = (value >> 57) & 0x1F;
    cpu[current_thread].reduce.count  = (value >> 16) & 0x7F;
    cpu[current_thread].reduce.optype = operation | (type << 4);

    if (type == 0) {
        cpu[current_thread].reduce.state = Processor::Reduce::State::Send;
        cpu[current_thread].reduce.thread = (value >> 2) & 0x3FFE;
    } else if (type == 1) {
        cpu[current_thread].reduce.state = Processor::Reduce::State::Recv;
        cpu[current_thread].reduce.thread = (value >> 2) & 0x3FFE;
    } else if (type == 2) {
        // Broadcast: compute sender/receiver using recursive halving
        unsigned minion = current_thread / EMU_THREADS_PER_MINION;
        if ((minion & minmask) == distance) {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Recv;
            cpu[current_thread].reduce.thread = EMU_THREADS_PER_MINION * (minion - distance);
        } else if ((minion & minmask) == 0) {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Send;
            cpu[current_thread].reduce.thread = EMU_THREADS_PER_MINION * (minion + distance);
        } else {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Skip;
        }
    } else {
        // Reduce: compute sender/receiver using recursive halving
        unsigned minion = current_thread / EMU_THREADS_PER_MINION;
        if ((minion & minmask) == distance) {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Send;
            cpu[current_thread].reduce.thread = EMU_THREADS_PER_MINION * (minion - distance);
        } else if ((minion & minmask) == 0) {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Recv;
            cpu[current_thread].reduce.thread = EMU_THREADS_PER_MINION * (minion + distance);
        } else {
            cpu[current_thread].reduce.state = Processor::Reduce::State::Skip;
        }
    }

    if (cpu[current_thread].reduce.state == Processor::Reduce::State::Skip) {
        LOG(DEBUG, "\t%s(skip) with level: %u, distance: %u, minmask: 0x%08u",
            reducecmd[type], level, distance, minmask);
        return;
    }

    // Sending and receiving from the same minion should fail immediately
    if (cpu[current_thread].reduce.thread == current_thread) {
        cpu[current_thread].reduce.state = Processor::Reduce::State::Skip;
        LOG(DEBUG, "\t%s(error) other_thread: %u, start_reg: %u, num_reg: %u", reducecmd[type],
            current_thread, cpu[current_thread].reduce.regid, cpu[current_thread].reduce.count);
        update_tensor_error(1 << 9);
        return;
    }

    // Illegal operation on a receiving minion should fail immediately
    if ((cpu[current_thread].reduce.state == Processor::Reduce::State::Recv) &&
        (operation == 1 || operation == 5 || operation > 8))
    {
        cpu[current_thread].reduce.state = Processor::Reduce::State::Skip;
        LOG(DEBUG, "\t%s(error) illegal operation: %u", reducecmd[type], operation);
        update_tensor_error(1 << 9);
        return;
    }

    // Sending or receiving 0 registers means do nothing
    // NB: This check has lower priority than other errors because
    // tensor_error[9] should be set even when "count" == 0".
    if (cpu[current_thread].reduce.count == 0) {
        cpu[current_thread].reduce.state = Processor::Reduce::State::Skip;
        LOG(DEBUG, "\t%s(skip) num_reg: 0", reducecmd[type]);
        return;
    }

    assert(cpu[current_thread].enqueue_tensor_op(Processor::Tensor::Reduce));

    log_tensor_reduce(cpu[current_thread].reduce.state == Processor::Reduce::State::Recv,
                      cpu[current_thread].reduce.regid, cpu[current_thread].reduce.count);

#if !defined(SYS_EMU) && !defined(ZSIM)
    tensor_reduce_execute();
#endif
}

void tensor_reduce_step(unsigned thread)
{
    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    Processor::Reduce& send = cpu[thread].reduce;
    Processor::Reduce& recv = cpu[current_thread].reduce;

    unsigned type = (recv.optype >> 4);

    if (send.count-- == 0) {
        throw std::runtime_error("Tensor reduce sender register count is 0");
    }
    if (!send.count) {
        assert(cpu[thread].dequeue_tensor_op() == Processor::Tensor::Reduce);
        cpu[thread].reduce.state = Processor::Reduce::State::Idle;
    }
    if (recv.count-- == 0) {
        LOG(WARN, "%s", "Mismatched tensor reduce register count");
        send.regid = (send.regid + 1) % NFREGS;
        recv.count = 0;
        return;
    }
    if (!recv.count) {
        assert(cpu[current_thread].dequeue_tensor_op() == Processor::Tensor::Reduce);
        cpu[current_thread].reduce.state = Processor::Reduce::State::Idle;
    }

    switch (recv.optype & 0xF) {
    case 0: // fadd
        set_rounding_mode(frm());
        LOG(DEBUG, "\t%s(recv) op=fadd sender=H%u rounding_mode=%s", reducecmd[type], thread, get_rounding_mode(frm()));
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_add(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 2: // fmax
        LOG(DEBUG, "\t%s(recv) op=fmax sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_maximumNumber(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 3: // fmin
        LOG(DEBUG, "\t%s(recv) op=fmin sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_minimumNumber(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 4: // iadd
        LOG(DEBUG, "\t%s(recv) op=iadd sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].u32[j] = cpu[thread].fregs[send.regid].u32[j] + FREGS[recv.regid].u32[j];
        }
        LOG_FREG("(this) =", recv.regid);
        break;
    case 6: // imax
        LOG(DEBUG, "\t%s(recv) op=imax sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].i32[j] = std::max(cpu[thread].fregs[send.regid].i32[j], FREGS[recv.regid].i32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        break;
    case 7: // imin
        LOG(DEBUG, "\t%s(recv) op=imin sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VL; j++) {
            FREGS[recv.regid].i32[j] = std::min(cpu[thread].fregs[send.regid].i32[j], FREGS[recv.regid].i32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        break;
    case 8: // fget
        LOG(DEBUG, "\t%s(recv) op=fget sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        FREGS[recv.regid] = cpu[thread].fregs[send.regid];
        LOG_FREG("(this) =", recv.regid);
        break;
    default:
        throw std::runtime_error("TensorReduce with illegal operation code!");
    }
    dirty_fp_state();
    log_tensor_reduce_write(recv.regid, FREGS[recv.regid]);

    send.regid = (send.regid + 1) % NFREGS;
    recv.regid = (recv.regid + 1) % NFREGS;
}

void tensor_reduce_execute()
{
    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    // Get information from receiver
    unsigned other_thread   = cpu[current_thread].reduce.thread;
    unsigned this_start_reg = cpu[current_thread].reduce.regid;
    unsigned this_num_reg   = cpu[current_thread].reduce.count;
    unsigned type           = cpu[current_thread].reduce.optype >> 4;

    if (cpu[current_thread].reduce.state == Processor::Reduce::State::Skip) {
        return;
    }
    if (cpu[current_thread].active_tensor_op() != Processor::Tensor::Reduce) {
        throw std::runtime_error("tensor_reduce_execute() called while "
                                 "this thread's TensorReduce FSM is inactive");
    }
#ifdef SYS_EMU
    if (cpu[other_thread].active_tensor_op() != Processor::Tensor::Reduce) {
        throw std::runtime_error("tensor_reduce_execute() called while "
                                 "the other thread's TensorReduce FSM is inactive");
    }
#endif

    if (cpu[current_thread].reduce.state != Processor::Reduce::State::Recv) {
#ifndef SYS_EMU
        if (cpu[current_thread].reduce.state == Processor::Reduce::State::Send) {
            LOG(DEBUG, "\t%s(send) receiver=H%u, start_reg=%u, num_reg=%u", reducecmd[type], other_thread, this_start_reg, this_num_reg);
            for (unsigned i = 0; i < this_num_reg; ++i) {
                unsigned this_op_reg = (i + this_start_reg) % NFREGS;
                LOG_FREG("(this) :", this_op_reg);
            }
        }
#endif
        if (cpu[current_thread].reduce.state == Processor::Reduce::State::Skip)
            cpu[current_thread].reduce.state = Processor::Reduce::State::Idle;
        return;
    }

    // Get information from sender
    unsigned this_thread     = cpu[other_thread].reduce.thread;
    unsigned other_start_reg = cpu[other_thread].reduce.regid;
    unsigned other_num_reg   = cpu[other_thread].reduce.count;

    if (this_thread != current_thread) {
        LOG(WARN, "\t%s(recv) sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, get_reduce_state(cpu[other_thread].reduce.state));
        throw std::runtime_error("Mismatched tensor reduce sender target minion");
    }
    if (cpu[other_thread].reduce.state != Processor::Reduce::State::Send) {
        LOG(WARN, "\t%s(recv) with sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, get_reduce_state(cpu[other_thread].reduce.state));
        throw std::runtime_error("Mismatched tensor reduce sender state");
    }
    if (other_num_reg != this_num_reg) {
        LOG(WARN, "\t%s(recv) with sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u receiver_start_reg=%u receiver_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, this_start_reg, this_num_reg, get_reduce_state(cpu[other_thread].reduce.state));
    }

    unsigned count = std::min(this_num_reg, other_num_reg);
    while (count--)
        tensor_reduce_step(other_thread);
}

// ----- TensorWait emulation ------------------------------------------------

// Starts a tensor wait, checks for stall conditions
void tensor_wait_start(uint64_t value)
{
    value = value & 0xF;
    cpu[current_thread].wait.id = value;
    cpu[current_thread].wait.value = value;
    cpu[current_thread].wait.state = Processor::Wait::State::Idle;
#ifdef SYS_EMU
    uint64_t id = value & 0x1;
    // TensorLoad
    if(value < 2)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        bool resolved = sys_emu::coop_tload_check(current_thread, false, id, requested_mask, present_mask);
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu[current_thread].wait.state = Processor::Wait::State::Wait;
            LOG(DEBUG, "TensorWait with id %i not ready => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", (int) id, requested_mask, present_mask);
        }
    }
    // TensorLoad L2
    else if(value < 4)
    {
    }
    // Prefetch
    else if(value < 6)
    {
    }
    // CacheOp
    else if(value == 6)
    {
    }
    // TensorFMA
    else if(value == 7)
    {
    }
    // TensorStore
    else if(value == 8)
    {
    }
    // TensorReduce
    else if(value == 9)
    {
    }
    // TensorQuant
    else if(value == 10)
    {
    }
#endif

    // Execute tensorwait right away for non sys_emu envs
#ifndef SYS_EMU
    tensor_wait_execute();
#endif
}

// Actual execution of tensor wait
void tensor_wait_execute()
{
#ifdef SYS_EMU
    if(sys_emu::get_scp_check())
    {
        // TensorWait TLoad Id0
        if     ((cpu[current_thread].wait.id) == 0) { sys_emu::get_scp_directory().l1_scp_wait(current_thread, 0); }
        else if((cpu[current_thread].wait.id) == 1) { sys_emu::get_scp_directory().l1_scp_wait(current_thread, 1); }
        else if((cpu[current_thread].wait.id) == 2) { sys_emu::get_scp_directory().l2_scp_wait(current_thread, 0); }
        else if((cpu[current_thread].wait.id) == 3) { sys_emu::get_scp_directory().l2_scp_wait(current_thread, 1); }
    }
#endif
    cpu[current_thread].wait.state = Processor::Wait::State::Idle;
    log_tensor_error_value(cpu[current_thread].tensor_error);
}

// ----- Shire cooperative mode ------------------------------------------------

void write_shire_coop_mode(unsigned shire, uint64_t val)
{
    assert(shire < EMU_NUM_SHIRES);
    esr_shire_coop_mode[shire] = !!(val & 1);
#ifndef SYS_EMU
    if (!esr_shire_coop_mode[shire])
        esr_icache_prefetch_active[shire] = false;
#endif
}

uint64_t read_shire_coop_mode(unsigned shire)
{
    assert(shire < EMU_NUM_SHIRES);
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
    unsigned barrier = value & 0x1F;
    unsigned limit   = (value >> 5) & 0xFF;

    unsigned shire = current_thread / EMU_THREADS_PER_SHIRE;
    unsigned oldval = bemu::shire_other_esrs[shire].fast_local_barrier[barrier];

    LOG(DEBUG, "FastLocalBarrier: doing barrier %u with value %u, limit %u",
        barrier, oldval, limit);

    if (oldval == limit)
    {
        // Last thread, zero barrier and return 1
        LOG(DEBUG, "%s", "FastLocalBarrier: last hart, set barrier to 0");
        bemu::shire_other_esrs[shire].fast_local_barrier[barrier] = 0;
        return 1;
    }

    // Not last thread, increment barrier and return 0
    LOG(DEBUG, "FastLocalBarrier: not last hart, increment barrier to %u",
        oldval + 1);
    bemu::shire_other_esrs[shire].fast_local_barrier[barrier] = oldval + 1;
    return 0;
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
            uint64_t fcc_addr = shire*EMU_THREADS_PER_SHIRE + EMU_THREADS_PER_MINION*minion + thread;
            LOG(DEBUG, "Incrementing FCC%" PRIu64 "[H%" PRIu64 "]=%" PRIu16, thread*2 + fcc_id, fcc_addr, uint16_t(fcc[fcc_addr][fcc_id] + 1));
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

#define set_mip_bit(thread, cause) do {  \
    if (~cpu[thread].mip & 1<<(cause)) \
        LOG(DEBUG, "Raising interrupt number %d", cause); \
    cpu[thread].mip |= 1<<(cause); \
} while (0)

#define clear_mip_bit(thread, cause) do {  \
    if (cpu[thread].mip & 1<<(cause)) \
        LOG(DEBUG, "Clearing interrupt number %d", cause); \
    cpu[thread].mip &= ~(1<<(cause)); \
} while (0)

#define set_ext_seip(thread) do { \
    if (ext_seip[thread] & (1<<SUPERVISOR_EXTERNAL_INTERRUPT)) \
        LOG(DEBUG, "Raising external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT); \
    ext_seip[thread] |= (1<<SUPERVISOR_EXTERNAL_INTERRUPT); \
} while (0)

#define clear_ext_seip(thread) do { \
    if (~ext_seip[thread] & (1<<SUPERVISOR_EXTERNAL_INTERRUPT)) \
        LOG(DEBUG, "Clearing external interrupt number %d", SUPERVISOR_EXTERNAL_INTERRUPT); \
    ext_seip[thread] &= ~(1<<SUPERVISOR_EXTERNAL_INTERRUPT); \
} while (0)

void raise_interrupt(int thread, int cause, uint64_t mip, uint64_t mbusaddr)
{
    if (cause == SUPERVISOR_EXTERNAL_INTERRUPT && !(mip & (1<<SUPERVISOR_EXTERNAL_INTERRUPT)))
    {
        set_ext_seip(thread);
    }
    else
    {
        set_mip_bit(thread, cause);
        if (cause == BUS_ERROR_INTERRUPT)
            cpu[thread].mbusaddr = zextPA(mbusaddr);
    }
}

void raise_software_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_SOFTWARE_INTERRUPT);
}

void clear_software_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_SOFTWARE_INTERRUPT);
}

void raise_timer_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_TIMER_INTERRUPT);
}

void clear_timer_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_TIMER_INTERRUPT);
}

void raise_external_machine_interrupt(int thread)
{
    set_mip_bit(thread, MACHINE_EXTERNAL_INTERRUPT);
}

void clear_external_machine_interrupt(int thread)
{
    clear_mip_bit(thread, MACHINE_EXTERNAL_INTERRUPT);
}

void raise_external_supervisor_interrupt(int thread)
{
    set_ext_seip(thread);
}

void clear_external_supervisor_interrupt(int thread)
{
    clear_ext_seip(thread);
}

void raise_bus_error_interrupt(int thread, uint64_t busaddr)
{
    set_mip_bit(thread, BUS_ERROR_INTERRUPT);
    cpu[thread].mbusaddr = zextPA(busaddr);
}

void clear_bus_error_interrupt(int thread)
{
    clear_mip_bit(thread, BUS_ERROR_INTERRUPT);
}

void pu_plic_interrupt_pending_set(uint32_t source_id)
{
    bemu::memory.pu_io_space.pu_plic.interrupt_pending_set(source_id);
}

void pu_plic_interrupt_pending_clear(uint32_t source_id)
{
    bemu::memory.pu_io_space.pu_plic.interrupt_pending_clear(source_id);
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
    return 1;
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
// LCOV_EXCL_STOP
