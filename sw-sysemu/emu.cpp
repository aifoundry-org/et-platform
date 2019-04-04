/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>       // FIXME: Remove this, use "emu_gio.h" instead
#include <cassert>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <list>
#include <deque>
#include <unordered_map>

#include "decode.h"
#include "emu.h"
#include "emu_casts.h"
#include "emu_gio.h"
#include "emu_memop.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "gold.h"
#include "log.h"
#include "rbox.h"
#include "tbox_emu.h"
#include "txs.h"

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

#include <cfenv>       // FIXME: remove this when we purge std::fesetround() from the code!

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

// Scratchpad defines
#define L1_SCP_ENTRIES    48
#define L1_SCP_LINE_SIZE  (L1D_LINE_SIZE)
typedef Packed<L1D_LINE_SIZE*8> cache_line_t;

// MISA initial value
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

typedef enum {
    // PS memory instructions
    FGW,
    FGH,
    FGB,
    FSCW,
    FSCH,
    FSCB,
    FGWL,
    FGHL,
    FGBL,
    FGWG,
    FGHG,
    FGBG,
    FSCWL,
    FSCHL,
    FSCBL,
    FSCWG,
    FSCHG,
    FSCBG,
} opcode;

typedef enum {
   SWAP,
   AND,
   OR,
   XOR,
   ADD,
   MIN,
   MAX,
   MINU,
   MAXU,
   MINPS,
   MAXPS,
   // Keep last - do not remove
   MAXAMOOP
} amoop;

// UART
static std::ostringstream uart_stream[EMU_NUM_THREADS];

// Register files
static uint64_t xregs[EMU_NUM_THREADS][MAXXREG];
static freg_t   fregs[EMU_NUM_THREADS][MAXFREG];
static mreg_t   mregs[EMU_NUM_THREADS][MAXMREG];
static freg_t   tensorfma_tenc[EMU_NUM_THREADS][MAXFREG];

// RISCV CSR registers
static uint32_t csr_fcsr[EMU_NUM_THREADS];
static uint64_t csr_stvec[EMU_NUM_THREADS];
static uint32_t csr_scounteren[EMU_NUM_THREADS];
static uint64_t csr_sscratch[EMU_NUM_THREADS];
static uint64_t csr_sepc[EMU_NUM_THREADS];
static uint64_t csr_scause[EMU_NUM_THREADS];
static uint64_t csr_stval[EMU_NUM_THREADS];
static uint64_t csr_satp[EMU_NUM_THREADS];
static uint64_t csr_mstatus[EMU_NUM_THREADS];
static uint64_t csr_misa[EMU_NUM_THREADS]; // could be hardcoded
static uint32_t csr_medeleg[EMU_NUM_THREADS];
static uint32_t csr_mideleg[EMU_NUM_THREADS];
static uint32_t csr_mie[EMU_NUM_THREADS];
static uint64_t csr_mtvec[EMU_NUM_THREADS];
static uint32_t csr_mcounteren[EMU_NUM_THREADS];
static uint64_t csr_mscratch[EMU_NUM_THREADS];
static uint64_t csr_mepc[EMU_NUM_THREADS];
static uint64_t csr_mcause[EMU_NUM_THREADS];
static uint64_t csr_mtval[EMU_NUM_THREADS];
static uint32_t csr_mip[EMU_NUM_THREADS];
static uint64_t csr_tdata1[EMU_NUM_THREADS];
static uint64_t csr_tdata2[EMU_NUM_THREADS];
// dcsr, dpc, dscratch
static uint32_t csr_mvendorid[EMU_NUM_THREADS]; // could be hardcoded
static uint64_t csr_marchid[EMU_NUM_THREADS]; // could be hardcoded
static uint64_t csr_mimpid[EMU_NUM_THREADS]; // could be hardcoded
static uint16_t csr_mhartid[EMU_NUM_THREADS];

// Esperanto CSR registers
static uint64_t csr_minstmask[EMU_NUM_THREADS];
static uint32_t csr_minstmatch[EMU_NUM_THREADS];
static uint8_t  csr_msleep_txfma_27[EMU_NUM_THREADS]; // 1b
static uint8_t  csr_menable_shadows[EMU_NUM_THREADS]; // 2b
// TODO: static uint8_t csr_excl_mode[EMU_NUM_THREADS]; // 1b
static uint8_t  csr_mtxfma_sleep_traps[EMU_NUM_THREADS]; // 5b
static uint8_t  csr_mcache_control[EMU_NUM_THREADS]; // 2b
static uint64_t csr_tensor_conv_size[EMU_NUM_THREADS]; // can we remove?
static uint64_t csr_tensor_conv_ctrl[EMU_NUM_THREADS]; // can we remove?
static uint16_t csr_tensor_mask[EMU_NUM_THREADS];
static uint16_t csr_tensor_error[EMU_NUM_THREADS];
static uint16_t csr_ucache_control[EMU_NUM_THREADS];
static uint8_t  csr_gsc_progress[EMU_NUM_THREADS]; // log2(VL)
static uint64_t csr_validation0[EMU_NUM_THREADS];
static uint64_t csr_validation2[EMU_NUM_THREADS];
static uint64_t csr_validation3[EMU_NUM_THREADS];
static uint64_t csr_portctrl[4][EMU_NUM_THREADS];

// Other processor state
static uint8_t csr_prv[EMU_NUM_THREADS]; // FIXME: Drop 'csr_' prefix
static bool mtvec_is_set[EMU_NUM_THREADS] = {};
static bool stvec_is_set[EMU_NUM_THREADS] = {};
static bool break_on_load[EMU_NUM_THREADS] = {};
static bool break_on_store[EMU_NUM_THREADS] = {};
static bool break_on_fetch[EMU_NUM_THREADS] = {};
static bool debug_mode[EMU_NUM_THREADS] = {};
static bool tensorload_setupb_topair[EMU_NUM_THREADS] = {false};
static int tensorload_setupb_numlines[EMU_NUM_THREADS];

// Scratchpad
cache_line_t scp[EMU_NUM_THREADS][L1_SCP_ENTRIES+TFMA_MAX_AROWS];

// Used to access different threads transparently
#define XREGS xregs[current_thread]
#define FREGS fregs[current_thread]
#define MREGS mregs[current_thread]
#define TENC  tensorfma_tenc[current_thread]
#define SCP   scp[current_thread]

// Message ports
static msg_port_conf_t     msg_ports[EMU_NUM_THREADS][NR_MSG_PORTS];
static std::deque<uint8_t> msg_ports_oob[EMU_NUM_THREADS][NR_MSG_PORTS];
static bool                msg_port_delayed_write = false;
static std::vector<msg_port_write_t> msg_port_pending_writes     [EMU_NUM_SHIRES];
static std::vector<msg_port_write_t> msg_port_pending_writes_tbox[EMU_NUM_SHIRES];
static std::vector<msg_port_write_t> msg_port_pending_writes_rbox[EMU_NUM_SHIRES];

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

uint64_t fcc_cnt;
uint16_t fcc[EMU_NUM_THREADS][2] ={{0}};
bool fcc_wait[EMU_NUM_THREADS] = {false};

// Shire ESRs
bool esr_shire_coop_mode[EMU_NUM_SHIRES] = {};
#ifndef SYS_EMU
bool esr_icache_prefetch_active[EMU_NUM_SHIRES] = {};
#endif

// only for checker, list of minions to awake (e.g. waiting for FCC that has just been written)
std::queue<uint32_t> minions_to_awake;
std::queue<uint32_t> &get_minions_to_awake() {return minions_to_awake;}

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

uint64_t current_pc = 0;
uint32_t current_inst = 0;
uint32_t current_thread = 0;
uint32_t num_sets = 16;
uint32_t num_ways = 4;

#define MAXSTACK 2048
static uint32_t shaderstack[EMU_NUM_THREADS][MAXSTACK];
static bool check_stack = false;

uint8_t in_sysemu = 0;
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
      for (int ii = 0; ii < MAXFREG; ++ii)
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

void init_emu()
{
   XREGS[x0]  = 0;
   // FIXME: remove '#include <cfenv>' when we purge this function from the code
   std::fesetround(FE_TONEAREST); // set rne for host
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

#define ZERO_UNUSED_FREG_BITS(regid, start) do { \
    for (size_t _zeufb_index = start; _zeufb_index < VL; ++_zeufb_index) \
        FREGS[regid].u32[_zeufb_index] = 0; \
} while (0)

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

template<size_t N>
static inline uint64_t sext(uint64_t val)
{
    return (N >= 64) ? val : uint64_t((int64_t(val) << (64-N)) >> (64-N));
}

static int32_t sext8_2(uint8_t val)
{
    uint32_t s = val & 0x80;
    int32_t r = s ? (0xffffff00 | val) : val;
    return r;
}

static uint64_t sextVA(uint64_t addr)
{
    // if bits addr[63:VA-1] are not all the same then set bits addr[63:VA] to
    // ~addr[VA-1], else leave as is
    enum : int64_t {
        sha47 = 63 - VA_SIZE,
        sha48 = 64 - VA_SIZE,
        bit63 = 1ll << 63,
    };
    int64_t sign = int64_t(addr) >> (VA_SIZE-1);
    return (sign == 0 || ~sign == 0)
        ? addr
        : uint64_t(int64_t(((addr << sha47) & ~bit63) |
                           (~(addr << sha48) & bit63)) >> sha47);
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
static inline void update_tensor_error(uint16_t value)
{
    csr_tensor_error[current_thread] |= value;
    if (value)
        LOG(DEBUG, "\tTensorError = 0x%04" PRIx16 " (0x%04" PRIx16 ")", csr_tensor_error[current_thread], value);
}

// internal accessor to fflags
static inline void update_fflags(uint_fast8_t flags)
{
    uint32_t newval = (flags & 0x1F) | (uint32_t(flags & 0x20) << 26);
    log_fflags_write(newval);
    csr_fcsr[current_thread] |= newval;
    LOG(DEBUG, "\tfpu flags = 0x%02x (%s)", flags, get_fp_flags(flags));
}

// internal accessor to mstatus.fs
static inline void dirty_fp_state()
{
    csr_mstatus[current_thread] |= 0x8000000000006000ULL;
}

static inline void require_fp_active()
{
    if ((csr_mstatus[current_thread] & 0x0006000ULL) == 0)
        throw trap_illegal_instruction(current_inst);
}

static inline void set_rounding_mode(rounding_mode mode)
{
    uint_fast8_t round = (mode == rmdyn) ? frm() : mode;
    if (round > 4)
        throw trap_illegal_instruction(current_inst);
    softfloat_roundingMode = round;
}

static inline const char* get_rounding_mode(rounding_mode mode)
{
    static const char* rmnames[] = {
        "rne",      "rtz",       "rdn",       "rup",
        "rmm",      "res5",      "res6",      "dyn",
        "dyn(rne)", "dyn(rtz)",  "dyn(rdn)",  "dyn(rup)",
        "dyn(rmm)", "dyn(res5)", "dyn(res6)", "dyn(res7)",
    };

    return rmnames[(mode == rmdyn) ? (8 + frm()) : mode];
}

static inline void set_fp_exceptions()
{
    if (softfloat_exceptionFlags)
    {
        dirty_fp_state();
        update_fflags(softfloat_exceptionFlags);
        softfloat_exceptionFlags = 0;
    }
}

void initcsr(uint32_t thread)
{
    // Exit reset at M-mode
    csr_prv[thread] = CSR_PRV_M;
    debug_mode[thread] = false;
    // Read-only registers
    csr_mvendorid[thread] = (11<<7) | ( 0xe5 & 0x7f); // bank 11, code=0xE5 (0x65 without parity)
    csr_marchid[thread] = 0x8000000000000001ULL;
    csr_mimpid[thread] = 0x0;
    if (thread == ((EMU_IO_SHIRE_SP*EMU_MINIONS_PER_SHIRE) << 1))
    {
        LOG(INFO, "Repurposing Shire 33 for Service Process : Thread %u Original MHartID %" PRIu16 " New MHartID %u",thread,csr_mhartid[thread],(IO_SHIRE_ID*EMU_MINIONS_PER_SHIRE));
        csr_mhartid[thread] = IO_SHIRE_ID*EMU_MINIONS_PER_SHIRE;
    }
    else
    {
        csr_mhartid[thread] = thread;
    }

    // misa is a 0-length register
    csr_misa[thread] = CSR_ISA_MAX;
    // M-mode registers with reset
    csr_mstatus[thread] = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    csr_mcause[thread] = 0;
    csr_mip[thread] = 0;
    csr_msleep_txfma_27[thread] = 0;
    csr_menable_shadows[thread] = 0;
    // TODO: csr_excl_mode[thread] = 0;
    csr_mtxfma_sleep_traps[thread] = 0;
    csr_mcache_control[thread] = 0;
    csr_mcounteren[thread] = 0;
    csr_scounteren[thread] = 0;
    // Debug-mode registers with reset
    csr_tdata1[thread] = 0x20C0000000000000ULL;
    // TODO: csr_dcsr[thread] <= xdebugver=1, prv=3;

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

    csr_minstmask[thread] = 0;
    csr_gsc_progress[thread] = 0;
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

static uint8_t security_ulp_check(uint32_t gold, uint32_t table)
{
    // Fast skip for zeros and infinity should be the same value in both in gold and table
    if (gold == table)
        return 0;

    // Detect NaNs
    bool gold_is_nan  = gld::isNaN(gold);
    bool table_is_nan = gld::isNaN(table);

    assert((gold_is_nan == table_is_nan) && "Trans mismatch error. Please open a jira to jordi.sola@esperantotech.com.");

    bool gold_is_inf = ((gold == 0xff800000) || (gold == 0x7f800000));

    //LOG(DEBUG, "GOLD: %d TABLE: %d", gold_is_nan, table_is_nan);
    if (gold_is_inf)
    {
        assert((gold == table) && "Trans mismatch error. Please open a jira to jordi.sola@esperantotech.com.");
    }
    // Skip all other tests.
    if (gold_is_nan)
        return 0;

    uint32_t gold_clean = gold & 0x7f800000;     // clean mantissa and sign from gold

    // compute 1ulp from gold
    float err_1ulp = cast_uint32_to_float(gold_clean) / float(1 << 23); // put '1' in the unit of less precision

    // compute diff between gold and table approximation
    float goldf  = cast_uint32_to_float(gold);
    float tablef = cast_uint32_to_float(table);
    float diff   = fabsf(goldf - tablef);

    // fail if diff is bigger than 1ulp
    /*if (diff > err_1ulp)
    {
        LOG(DEBUG, "Gold IEEE: %.12e, Table TRANS: %.12e, Diff: %.12e, Max (1ulp): %.12e", goldf, tablef, diff, err_1ulp);
        LOG(DEBUG, "Hex Gold: %08X, Hex Table: %08X", gold, table);
    }*/
    return (diff > err_1ulp);
}

void check_pending_interrupts()
{
    // Are there any non-masked pending interrupts?
    uint64_t xip = csr_mip[current_thread] & csr_mie[current_thread];
    if (!xip) return;

    LOG(DEBUG, "Check Pending Interrupt mtvec:0x%016" PRIx64 " mip:0x%08" PRIx32 " mie:0x%08" PRIx32,
        csr_mtvec[current_thread], csr_mip[current_thread], csr_mie[current_thread]);

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

    LOG(DEBUG, "\tTrapping to S-mode with cause 0x%" PRIx64, cause);

    // if checking against RTL, clear the correspoding MIP bit
    // it will be set to 1 again if the pending bit was not really cleared
    // just before entering the interrupt again
    // TODO: you won't be able to read the MIP CSR from code or you'll get
    // checker errors (you would have errors if the interrupt is cleared by
    // a memory mapped store)
    if (interrupt && !in_sysemu) {
        csr_mip[current_thread] &= ~(1ULL<<code);
    }

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
    if (interrupt && !in_sysemu) {
        csr_mip[current_thread] &= ~(1ull<<code);
    }

    LOG(DEBUG, "\tTrapping to M-mode with cause 0x%" PRIx64 " and tval %" PRIx64, cause, val);

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

void set_thread(uint32_t thread)
{
    current_thread = thread;
}

uint32_t get_thread()
{
    return current_thread;
}

uint32_t get_mask(unsigned maskNr)
{
    return MREGS[maskNr].to_ulong();
}

extern inst_state_change * log_info;

////////////////////////////////////////////////////////////////////////////////
//
// Memory emulation
//
////////////////////////////////////////////////////////////////////////////////

static inline int effective_execution_mode(mem_access_type macc)
{
    // Read mstatus
    const uint64_t mstatus = csr_mstatus[current_thread];
    const int      mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const int      mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;
    return (macc == Mem_Access_Fetch || macc == Mem_Access_PTW)
        ? prvget()
        : (mprv ? mpp : prvget());
}

// Accessor functions for externally defined memory

static uint8_t host_memread8(uint64_t addr)
{ return * ((uint8_t *) addr); }

static uint16_t host_memread16(uint64_t addr)
{ return * ((uint16_t *) addr); }

static uint32_t host_memread32(uint64_t addr)
{ return * ((uint32_t *) addr); }

static uint64_t host_memread64(uint64_t addr)
{ return * ((uint64_t *) addr); }

static void host_memwrite8(uint64_t addr, uint8_t data)
{ * ((uint8_t *) addr) = data; }

static void host_memwrite16(uint64_t addr, uint16_t data)
{ * ((uint16_t *) addr) = data; }

static void host_memwrite32(uint64_t addr, uint32_t data)
{ * ((uint32_t *) addr) = data; }

static void host_memwrite64(uint64_t addr, uint64_t data)
{ * ((uint64_t *) addr) = data; }

static uint64_t virt_to_phys_host(uint64_t addr, mem_access_type macc __attribute__((unused)))
{ return addr; }

uint8_t  (*pmemread8)  (uint64_t addr) = host_memread8;
uint16_t (*pmemread16) (uint64_t addr) = host_memread16;
uint32_t (*pmemread32) (uint64_t addr) = host_memread32;
uint64_t (*pmemread64) (uint64_t addr) = host_memread64;

void (*pmemwrite8)  (uint64_t addr, uint8_t  data) = host_memwrite8;
void (*pmemwrite16) (uint64_t addr, uint16_t data) = host_memwrite16;
void (*pmemwrite32) (uint64_t addr, uint32_t data) = host_memwrite32;
void (*pmemwrite64) (uint64_t addr, uint64_t data) = host_memwrite64;

uint64_t (*vmemtranslate) (uint64_t addr, mem_access_type macc) = virt_to_phys_host;

// Breakpoints and watchpoints

static bool matches_breakpoint_address(uint64_t addr)
{
    uint64_t mcontrol = tdata1();
    uint64_t mvalue = tdata2();
    bool exact = (~mcontrol & 0x80);
    uint64_t mask = exact ? 0 : (((~mvalue & (mvalue + 1)) - 1) & 0x3f);
    return (mvalue == ((addr & VA_M) | mask));
}

bool halt_on_breakpoint()
{
    return (~tdata1() & 0x0800000000001000ull) == 0;
}

__attribute__((noreturn)) void throw_trap_breakpoint(uint64_t addr)
{
    if (halt_on_breakpoint())
        throw std::runtime_error("Debug mode not supported yet!");
    throw trap_breakpoint(addr);
}

static inline void check_load_breakpoint(uint64_t addr)
{
    if (break_on_load[current_thread] && matches_breakpoint_address(addr))
        throw_trap_breakpoint(addr);
}

static inline void check_store_breakpoint(uint64_t addr)
{
    if (break_on_store[current_thread] && matches_breakpoint_address(addr))
        throw_trap_breakpoint(addr);
}

bool matches_fetch_breakpoint(uint64_t addr)
{
    return break_on_fetch[current_thread] && matches_breakpoint_address(addr);
}

// PMA checks

// Minion Memory map
// +-------------------+---------------------------------+-------------+
// |   Address range   |      Address range (hex)        |             |
// | From    |   To    |      From      |      To        | Maps to     |
// +---------+---------+----------------+----------------+-------------+
// |    0G   |    1G   | 0x00_0000_0000 | 0x00_3fff_ffff | IO region   |
// |    1G   |    2G   | 0x00_4000_0000 | 0x00_7fff_ffff | SP region   |
// |    1G   | 1G+64K  | 0x00_4000_0000 | 0x00_4000_ffff | SP/ROM      |
// |  1G+1M  |  1G+2M  | 0x00_4040_0000 | 0x00_404f_ffff | SP/SRAM     |
// |    2G   |    4G   | 0x00_8000_0000 | 0x00_ffff_ffff | SCP region  |
// |    4G   |    8G   | 0x01_0000_0000 | 0x01_ffff_ffff | ESR region  |
// |    8G   |  256G   | 0x02_0000_0000 | 0x3f_ffff_ffff | Reserved    |
// |  256G   |  512G   | 0x40_0000_0000 | 0x7f_ffff_ffff | PCIe region |
// |  512G   | 512G+2M | 0x80_0000_0000 | 0x80_001f_ffff | DRAM/Mbox   |
// | 512G+2M |  516G   | 0x80_0020_0000 | 0x80_ffff_ffff | DRAM/OSbox  |
// |  516G   |  ...    | 0x81_0000_0000 | 0xff_ffff_ffff | DRAM/Other  |
// +---------+---------+----------------+----------------+-------------+

static inline bool paddr_is_io_space(uint64_t addr)
{
    return addr < UINT64_C(0x0040000000);
}

static inline bool paddr_is_sp_space(uint64_t addr)
{
    return (addr >= UINT64_C(0x0040000000)) && (addr < UINT64_C(0x0080000000));
}

static inline bool paddr_is_sp_rom(uint64_t addr)
{
    return (addr >= UINT64_C(0x0040000000)) && (addr < UINT64_C(0x0040010000));
}

static inline bool paddr_is_sp_sram(uint64_t addr)
{
    return (addr >= UINT64_C(0x0040400000)) && (addr < UINT64_C(0x0040500000));
}

static inline bool paddr_is_scratchpad(uint64_t addr)
{
    return (addr >= UINT64_C(0x0080000000)) && (addr < UINT64_C(0x0100000000));
}

static inline bool paddr_is_esr_space(uint64_t addr)
{
    return (addr >= UINT64_C(0x0100000000)) && (addr < UINT64_C(0x0200000000));
}

static inline bool paddr_is_reserved(uint64_t addr)
{
    return (addr >= UINT64_C(0x0200000000)) && (addr < UINT64_C(0x4000000000));
}

static inline bool paddr_is_pcie_space(uint64_t addr)
{
    return (addr >= UINT64_C(0x4000000000)) && (addr < UINT64_C(0x8000000000));
}

static inline bool paddr_is_dram_mbox(uint64_t addr)
{
    return (addr >= UINT64_C(0x8000000000)) && (addr < UINT64_C(0x8000200000));
}

static inline bool paddr_is_dram_osbox(uint64_t addr)
{
    return (addr >= UINT64_C(0x8000200000)) && (addr < UINT64_C(0x8100000000));
}

static inline bool paddr_is_dram_other(uint64_t addr)
{
    return addr >= UINT64_C(0x8100000000);
}

static inline bool paddr_is_dram(uint64_t addr)
{
    return addr >= UINT64_C(0x8000000000);
}

static inline bool access_is_size_aligned(uint64_t addr, size_t size)
{
    return !(addr % size);
}

static inline bool access_is_cacheable(uint64_t addr)
{
    return paddr_is_dram(addr)
        || paddr_is_scratchpad(addr)
        || paddr_is_sp_rom(addr)
        || paddr_is_sp_sram(addr);
}

static bool pma_check_data_access(uint64_t addr, size_t size, mem_access_type macc)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    bool amo      = (macc == Mem_Access_AtomicL || macc == Mem_Access_AtomicG);
    bool amo_g    = (macc == Mem_Access_AtomicG);
    bool ts_tl_co = (macc >= Mem_Access_TxLoad && macc <= Mem_Access_CacheOp);

    if (paddr_is_io_space(addr))
        return !amo
            && !ts_tl_co
            && (spio || /*!mprot.disable_io_access*/true);

    if (paddr_is_sp_space(addr))
        return spio && !amo && !ts_tl_co;

    if (paddr_is_scratchpad(addr))
        return ts_tl_co
            || amo_g
            || access_is_size_aligned(addr, size);

    if (paddr_is_esr_space(addr))
        return !amo
            && !ts_tl_co
            && (size == 8)
            && access_is_size_aligned(addr, size)
            && ( int((addr >> 30) & 0x3) <= effective_execution_mode(macc) )
            && ( int((addr >> 30) & 0x3) != 2 || spio );

    if (paddr_is_pcie_space(addr))
        return !amo
            && !ts_tl_co
            && (spio || /*!mprot.disable_pcie_access*/true);

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(macc) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}

static bool pma_check_ptw_access(uint64_t addr)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    if (paddr_is_sp_rom(addr))
        return spio;

    if (paddr_is_sp_sram(addr))
        return spio;

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(Mem_Access_PTW) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}

static bool pma_check_fetch_access(uint64_t addr)
{
    bool spio = (current_thread / EMU_THREADS_PER_SHIRE) == EMU_IO_SHIRE_SP;

    if (paddr_is_sp_rom(addr))
        return spio;

    if (paddr_is_sp_sram(addr))
        return spio;

    if (paddr_is_dram_mbox(addr))
        return effective_execution_mode(Mem_Access_Fetch) == CSR_PRV_M;

    if (paddr_is_dram_osbox(addr))
        return spio || /*!mprot.disable_osbox_access*/true;

    return paddr_is_dram(addr);
}

uint16_t pmemfetch16(uint64_t paddr)
{
    if (!pma_check_fetch_access(paddr))
    {
        throw trap_instruction_access_fault(paddr);
    }
    return pmemread16(paddr);
}

static uint8_t vmemread8(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 1, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    return pmemread8(paddr);
}

static uint16_t vmemread16(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 2, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 2))
    {
        throw trap_load_address_misaligned(addr);
    }
    return pmemread16(paddr);
}

static uint16_t aligned_vmemread16(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 2, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    if (!access_is_size_aligned(addr, 2))
    {
        throw trap_load_address_misaligned(addr);
    }
    return pmemread16(paddr);
}

static uint32_t vmemread32(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 4, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 4))
    {
        throw trap_load_address_misaligned(addr);
    }
    return pmemread32(paddr);
}

static uint32_t aligned_vmemread32(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 4, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    if (!access_is_size_aligned(addr, 4))
    {
        throw trap_load_address_misaligned(addr);
    }
    return pmemread32(paddr);
}

static uint64_t vmemread64(uint64_t addr)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Load);
    if (!pma_check_data_access(paddr, 8, Mem_Access_Load))
    {
        throw trap_load_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 8))
    {
        throw trap_load_address_misaligned(addr);
    }
    return pmemread64(paddr);
}

static void vmemwrite8(uint64_t addr, uint8_t data)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 1, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    pmemwrite8(paddr, data);
}

static void vmemwrite16(uint64_t addr, uint16_t data)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 2, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 2))
    {
        throw trap_store_address_misaligned(addr);
    }
    pmemwrite16(paddr, data);
}

static void aligned_vmemwrite16(uint64_t addr, uint16_t data)
{
    if (!access_is_size_aligned(addr, 2))
    {
        throw trap_store_address_misaligned(addr);
    }
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 2, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    pmemwrite16(paddr, data);
}

static void vmemwrite32(uint64_t addr, uint32_t data)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 4, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 4))
    {
        throw trap_store_address_misaligned(addr);
    }
    pmemwrite32(paddr, data);
}

static void aligned_vmemwrite32(uint64_t addr, uint32_t data)
{
    if (!access_is_size_aligned(addr, 4))
    {
        throw trap_store_address_misaligned(addr);
    }
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 4, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    pmemwrite32(paddr, data);
}

static void vmemwrite64(uint64_t addr, uint64_t data)
{
    uint64_t paddr = vmemtranslate(addr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 8, Mem_Access_Store))
    {
        throw trap_store_access_fault(addr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(addr, 8))
    {
        throw trap_store_address_misaligned(addr);
    }
    pmemwrite64(paddr, data);
}

// forward declaration
static uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc);

// Abstract memory accessors. By default we use the host memory directly,
// unless we are asked to use emulated memory.

void set_memory_funcs(uint8_t  (*func_memread8_ ) (uint64_t),
                      uint16_t (*func_memread16_) (uint64_t),
                      uint32_t (*func_memread32_) (uint64_t),
                      uint64_t (*func_memread64_) (uint64_t),
                      void (*func_memwrite8_ ) (uint64_t, uint8_t ),
                      void (*func_memwrite16_) (uint64_t, uint16_t),
                      void (*func_memwrite32_) (uint64_t, uint32_t),
                      void (*func_memwrite64_) (uint64_t, uint64_t))
{
    pmemread8   = func_memread8_;
    pmemread16  = func_memread16_;
    pmemread32  = func_memread32_;
    pmemread64  = func_memread64_;
    pmemwrite8  = func_memwrite8_;
    pmemwrite16 = func_memwrite16_;
    pmemwrite32 = func_memwrite32_;
    pmemwrite64 = func_memwrite64_;
    vmemtranslate = virt_to_phys_emu;
}

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
    LOG(DEBUG, "I: unknown @%016" PRIx64 "(0x%04x)", PC, current_inst);
    throw trap_illegal_instruction(current_inst);
}

void beq(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("beq");
    if (RS1 == RS2)
        WRITE_PC(PC + BIMM);
}

void bne(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("bne");
    if (RS1 != RS2)
        WRITE_PC(PC + BIMM);
}

void blt(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("blt");
    if (int64_t(RS1) < int64_t(RS2))
        WRITE_PC(PC + BIMM);
}

void bltu(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("bltu");
    if (RS1 < RS2)
        WRITE_PC(PC + BIMM);
}

void bge(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("bge");
    if (int64_t(RS1) >= int64_t(RS2))
        WRITE_PC(PC + BIMM);
}

void bgeu(xreg rs1, xreg rs2, int64_t b_imm, const char* comm __attribute__((unused)))
{
    DISASM_RS1_RS2_BIMM("bgeu");
    if (RS1 >= RS2)
        WRITE_PC(PC + BIMM);
}

void c_jalr(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("jalr");
    uint64_t tmp = (RS1 + IIMM) & ~1ull;
    WRITE_RD(C_NPC);
    WRITE_PC(tmp);
}

void c_jal(xreg rd, int64_t j_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_JIMM("jal");
    WRITE_RD(C_NPC);
    WRITE_PC(PC + JIMM);
}

void jalr(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("jalr");
    uint64_t tmp = (RS1 + IIMM) & ~1ull;
    WRITE_RD(NPC);
    WRITE_PC(tmp);
}

void jal(xreg rd, int64_t j_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_JIMM("jal");
    WRITE_RD(NPC);
    WRITE_PC(PC + JIMM);
}

void lui(xreg rd, int64_t u_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_UIMM("lui");
    WRITE_RD(UIMM);
}

void auipc(xreg rd, int64_t u_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_UIMM("auipc");
    WRITE_RD(PC + UIMM);
}

void addi(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("addi");
    WRITE_RD(RS1 + IIMM);
}

void slli(xreg rd, xreg rs1, unsigned shamt6, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT6("slli");
    WRITE_RD(RS1 << SHAMT6);
}

void slti(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("slti");
    WRITE_RD(int64_t(RS1) < IIMM);
}

void sltiu(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("sltiu");
    WRITE_RD(RS1 < uint64_t(IIMM));
}

void xori(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("xori");
    WRITE_RD(RS1 ^ IIMM);
}

void srli(xreg rd, xreg rs1, unsigned shamt6, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT6("srli");
    WRITE_RD(RS1 >> SHAMT6);
}

void srai(xreg rd, xreg rs1, unsigned shamt6, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT6("srai");
    WRITE_RD(int64_t(RS1) >> SHAMT6);
}

void ori(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("ori");
    WRITE_RD(RS1 | IIMM);
}

void andi(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("andi");
    WRITE_RD(RS1 & IIMM);
}

void add(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("add");
    WRITE_RD(RS1 + RS2);
}

void sub(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sub");
    WRITE_RD(RS1 - RS2);
}

void sll(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sll");
    WRITE_RD(RS1 << (RS2 % 64));
}

void slt(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("slt");
    WRITE_RD(int64_t(RS1) < int64_t(RS2));
}

void sltu(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sltu");
    WRITE_RD(RS1 < RS2);
}

void xor_(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("xor");
    WRITE_RD(RS1 ^ RS2);
}

void srl(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("srl");
    WRITE_RD(RS1 >> (RS2 % 64));
}

void sra(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sra");
    WRITE_RD(int64_t(RS1) >> (RS2 % 64));
}

void or_(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("or");
    WRITE_RD(RS1 | RS2);
}

void and_(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("and");
    WRITE_RD(RS1 & RS2);
}

void addiw(xreg rd, xreg rs1, int64_t i_imm, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_IIMM("addiw");
    WRITE_RD(sext<32>(RS1 + IIMM));
}

void slliw(xreg rd, xreg rs1, unsigned shamt5, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT5("slliw");
    WRITE_RD(sext<32>(RS1 << SHAMT5));
}

void srliw(xreg rd, xreg rs1, unsigned shamt5, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT5("srliw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> SHAMT5));
}

void sraiw(xreg rd, xreg rs1, unsigned shamt5, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_SHAMT5("sraiw");
    WRITE_RD(sext<32>(int32_t(RS1) >> SHAMT5));
}

void addw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("addw");
    WRITE_RD(sext<32>(RS1 + RS2));
}

void subw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("subw");
    WRITE_RD(sext<32>(RS1 - RS2));
}

void sllw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sllw");
    WRITE_RD(sext<32>(RS1 << (RS2 % 32)));
}

void srlw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("srlw");
    WRITE_RD(sext<32>(uint32_t(RS1) >> (RS2 % 32)));
}

void sraw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("sraw");
    WRITE_RD(sext<32>(int32_t(RS1) >> (RS2 % 32)));
}

void lb(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lb x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = sext<8>(vmemread8(addr));
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM8[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void lh(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lh x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = sext<16>(vmemread16(addr));
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM16[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void lw(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lw x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = sext<32>(vmemread32(addr));
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM32[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void ld(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: ld x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = vmemread64(addr);
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM64[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst]  = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void lbu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lbu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = vmemread8(addr);
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM8[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void lhu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lhu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = vmemread16(addr);
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM16[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void lwu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lwu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    check_load_breakpoint(addr);
    uint64_t val = vmemread32(addr);
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM32[0x%016" PRIx64 "]", val, addr);
    if (dst != x0)
    {
        XREGS[dst] = val;
    }
    log_xreg_write(dst, XREGS[dst]);
}

void sd(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sd x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    uint64_t val  = XREGS[src1];
    LOG(DEBUG, "\t%016" PRIx64 " --> MEM64[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite64(addr, val);
    log_mem_write(0, 8, addr, val);
}

void sw(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sw x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    uint32_t val  = uint32_t(XREGS[src1]);
    LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite32(addr, val);
    log_mem_write(0, 4, addr, val);
}

void sh(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sh x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    uint16_t val  = uint16_t(XREGS[src1]);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite16(addr, val);
    log_mem_write(0, 2, addr, val);
}

void sb(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sb x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base] + off);
    uint8_t  val  = uint8_t(XREGS[src1]);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM8[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite8(addr, val);
    log_mem_write(0, 1, addr, val);
}

void sbl(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: sbl x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base]);
    uint8_t  val  = uint8_t(XREGS[src1]);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM8[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite8(addr, val);
    log_mem_write(0, 1, addr, val);
}

void sbg(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: sbg x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base]);
    uint8_t  val  = uint8_t(XREGS[src1]);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM8[0x%016" PRIx64 "]", val, addr);
    check_store_breakpoint(addr);
    vmemwrite8(addr, val);
    log_mem_write(0, 1, addr, val);
}

void shl(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: shl x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base]);
    uint16_t val  = uint16_t(XREGS[src1]);
    check_store_breakpoint(addr);
    vmemwrite16(addr, val);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", val, addr);
    log_mem_write(0, 2, addr, val);
}

void shg(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: shg x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = sextVA(XREGS[base]);
    uint16_t val  = uint16_t(XREGS[src1]);
    check_store_breakpoint(addr);
    vmemwrite16(addr, val);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", val, addr);
    log_mem_write(0, 2, addr, val);
}


void fence(const char* comm)
{
    LOG(DEBUG, "I: fence%s%s", (comm?" # ":""), (comm?comm:""));
}

void fence_i(const char* comm)
{
    LOG(DEBUG, "I: fence_i%s%s", (comm?" # ":""), (comm?comm:""));
    throw trap_mcode_instruction(current_inst);
    // NB: placeholder for flushing any cached decoding results we may have
    // to synchronize when fence_i() is executed
    // flush_insn_cache();
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64M emulation
//
////////////////////////////////////////////////////////////////////////////////

static inline int64_t mulh(int64_t a, int64_t b)
{
    return (__int128_t(a) * __int128_t(b)) >> 64;
}

static inline uint64_t mulhu(uint64_t a, uint64_t b)
{
    return (__uint128_t(a) * __uint128_t(b)) >> 64;
}

static inline int64_t mulhsu(int64_t a, uint64_t b)
{
    return (__int128_t(a) * __uint128_t(b)) >> 64;
}

static inline int64_t idiv(int64_t a, int64_t b)
{
    return (b == 0) ? UINT64_MAX : ((a == INT64_MIN && b == -1LL) ? a : (a / b));
}

static inline uint64_t udiv(uint64_t a, uint64_t b)
{
    return (b == 0) ? UINT64_MAX : (a / b);
}

static inline int64_t idivw(int64_t a, int64_t b)
{
    return (b == 0) ? UINT64_MAX : sext<32>(a / b);
}

static inline uint64_t udivw(uint64_t a, uint64_t b)
{
    return (b == 0) ? UINT64_MAX : sext<32>(a / b);
}

static inline int64_t irem(int64_t a, int64_t b)
{
    return (b == 0) ? a : ((a == INT64_MIN && b == -1LL) ? 0 : (a % b));
}

static inline uint64_t urem(uint64_t a, uint64_t b)
{
    return (b == 0) ? a : (a % b);
}

static inline int64_t iremw(int64_t a, int64_t b)
{
    return (b == 0) ? a : sext<32>(a % b);
}

static inline uint64_t uremw(uint64_t a, uint64_t b)
{
    return (b == 0) ? sext<32>(a) : sext<32>(a % b);
}

void mul(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("mul");
    WRITE_RD(RS1 * RS2);
}

void mulh(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("mulh");
    WRITE_RD(mulh(RS1, RS2));
}

void mulhsu(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("mulhsu");
    WRITE_RD(mulhsu(RS1, RS2));
}

void mulhu(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("mulhu");
    WRITE_RD(mulhu(RS1, RS2));
}

void div_(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("div");
    WRITE_RD(idiv(RS1, RS2));
}

void divu(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("divu");
    WRITE_RD(udiv(RS1, RS2));
}

void rem(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("rem");
    WRITE_RD(irem(RS1, RS2));
}

void remu(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("remu");
    WRITE_RD(urem(RS1, RS2));
}

void mulw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("mulw");
    WRITE_RD(sext<32>(RS1 * RS2));
}

void divw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("divw");
    WRITE_RD(idivw(sext<32>(RS1), sext<32>(RS2)));
}

void divuw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("divuw");
    WRITE_RD(udivw(uint32_t(RS1), uint32_t(RS2)));
}

void remw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("remw");
    WRITE_RD(iremw(sext<32>(RS1), sext<32>(RS2)));
}

void remuw(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("remuw");
    WRITE_RD(uremw(uint32_t(RS1), uint32_t(RS2)));
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64A emulation
//
////////////////////////////////////////////////////////////////////////////////

#define AMO_EMU_W_FUNC(NAME, LG, OPC) \
void NAME(xreg dst, xreg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " x%d, x%d, (x%d)%s%s", dst, src2, src1, comm ? " # " : "", comm ? comm : "");\
   amo_emu_w(OPC, dst, src1, src2, Mem_Access_Atomic ## LG);\
}

#define AMO_EMU_D_FUNC(NAME, LG, OPC) \
void NAME(xreg dst, xreg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " x%d, x%d, (x%d)%s%s", dst, src2, src1, comm ? " # " : "", comm ? comm : "");\
   amo_emu_d(OPC, dst, src1, src2, Mem_Access_Atomic ## LG);\
}

static void amo_emu_w(amoop op, xreg dst, xreg src1, xreg src2, mem_access_type macc)
{
    uint64_t addr = sextVA(XREGS[src1]);

    check_store_breakpoint(addr);
    if (!access_is_size_aligned(addr, 4))
    {
        throw trap_store_access_fault(addr);
    }
    uint64_t paddr = vmemtranslate(addr, macc);
    if (!pma_check_data_access(paddr, 4, macc))
    {
        throw trap_store_access_fault(addr);
    }
    uint32_t val1 = pmemread32(paddr);
    uint32_t val2 = uint32_t(XREGS[src2]);

    // Save the loaded data
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM32[0x%016" PRIx64 "]", sext<32>(val1), addr);
    if (dst != x0)
    {
        XREGS[dst] = sext<32>(val1);
    }
    log_xreg_write(dst, XREGS[dst]);

    uint32_t res;
    switch (op)
    {
       case SWAP:
          res = val2;
          LOG(DEBUG, "\t0x%08x <-- 0x%08x", res, val2);
          break;
       case AND:
          res = val1 & val2;
          LOG(DEBUG, "\t0x%08x <-- 0x%08x & 0x%08x", res, val1, val2);
          break;
       case OR:
          res = val1 | val2;
          LOG(DEBUG, "\t0x%08x <-- 0x%08x | 0x%08x", res, val1, val2);
          break;
       case XOR:
          res = val1 ^ val2;
          LOG(DEBUG, "\t0x%08x <-- 0x%08x ^ 0x%08x", res, val1, val2);
          break;
       case ADD:
          res = (int32_t)val1 + (int32_t)val2;
          LOG(DEBUG, "\t0x%08x <-- 0x%08x + 0x%08x", res, val1, val2);
          break;
       case MIN:
          res = ((int32_t)val1 < (int32_t)val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%08x <-- min(0x%08x, 0x%08x)", res, val1, val2);
          break;
       case MAX:
          res = ((int32_t)val1 > (int32_t)val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%08x <-- max(0x%08x, 0x%08x)", res, val1, val2);
          break;
       case MINU:
          res = (val1 < val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%08x <-- minu(0x%08x, 0x%08x)", res, val1, val2);
          break;
       case MAXU:
          res = (val1 > val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%08x <-- maxu(0x%08x, 0x%08x)", res, val1, val2);
          break;
       default:
          res = 0;
          LOG(DEBUG, "\tFATAL: Unknown atomic op %d", op);
    }

    // Stores the operated data
    LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", res, addr);
    pmemwrite32(paddr, res);
    // note: for logging purposes, sending val2 instead of res => we want to check what the
    // dcache outputs to the shire caches, not the actual value written in memory
    log_mem_write(0, 4, addr, val2);
}

static void amo_emu_d(amoop op, xreg dst, xreg src1, xreg src2, mem_access_type macc)
{
    uint64_t addr = sextVA(XREGS[src1]);

    check_store_breakpoint(addr);
    if (!access_is_size_aligned(addr, 8))
    {
        throw trap_store_access_fault(addr);
    }
    uint64_t paddr = vmemtranslate(addr, macc);
    if (!pma_check_data_access(paddr, 8, macc))
    {
        throw trap_store_access_fault(addr);
    }
    uint64_t val1 = pmemread64(paddr);
    uint64_t val2 = XREGS[src2];

    // Save the loaded data
    LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM64[0x%016" PRIx64 "]", val1, addr);
    if (dst != x0)
    {
        XREGS[dst] = val1;
    }
    log_xreg_write(dst, XREGS[dst]);

    uint64_t res;
    switch (op)
    {
       case SWAP:
          res = val2;
          break;
       case AND:
          res = val1 & val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " & 0x%016" PRIx64, res, val1, val2);
          break;
       case OR:
          res = val1 | val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " | 0x%016" PRIx64, res, val1, val2);
          break;
       case XOR:
          res = val1 ^ val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " ^ 0x%016" PRIx64, res, val1, val2);
          break;
       case ADD:
          res = (int64_t)val1 + (int64_t)val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " + 0x%016" PRIx64, res, val1, val2);
          break;
       case MIN:
          res = ((int64_t)val1 < (int64_t)val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- min(0x%016" PRIx64 ", 0x%016" PRIx64 ")", res, val1, val2);
          break;
       case MAX:
          res = ((int64_t)val1 > (int64_t)val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- max(0x%016" PRIx64 ", 0x%016" PRIx64 ")", res, val1, val2);
          break;
       case MINU:
          res = (val1 < val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- minu(0x%016" PRIx64 ", 0x%016" PRIx64 ")", res, val1, val2);
          break;
       case MAXU:
          res = (val1 > val2) ? val1 : val2;
          LOG(DEBUG, "\t0x%016" PRIx64 " <-- maxu(0x%016" PRIx64 ", 0x%016" PRIx64 ")", res, val1, val2);
          break;
       default:
          assert(0);
          res = 0;
          break;
    }

    // Store the operated data
    LOG(DEBUG, "\t0x%016" PRIx64 " --> MEM64[0x%016" PRIx64 "]", res, addr);
    pmemwrite64(paddr, res);
    // note: for logging purposes, sending val2 instead of res => we want to check what the
    // dcache outputs to the shire caches, not the actual value written in memory
    log_mem_write(0, 8, addr, val2);
}

#if 0
//
// Scalar 32 bits Atomics
//

AMO_EMU_W_FUNC(amoswap_w, SWAP)
AMO_EMU_W_FUNC(amoand_w,  AND)
AMO_EMU_W_FUNC(amoor_w,   OR)
AMO_EMU_W_FUNC(amoxor_w,  XOR)
AMO_EMU_W_FUNC(amoadd_w,  ADD)
AMO_EMU_W_FUNC(amomin_w,  MIN)
AMO_EMU_W_FUNC(amomax_w,  MAX)
AMO_EMU_W_FUNC(amominu_w, MINU)
AMO_EMU_W_FUNC(amomaxu_w, MAXU)

//
// Scalar 64 bits Atomics
//

AMO_EMU_D_FUNC(amoswap_d, SWAP)
AMO_EMU_D_FUNC(amoand_d,  AND)
AMO_EMU_D_FUNC(amoor_d,   OR)
AMO_EMU_D_FUNC(amoxor_d,  XOR)
AMO_EMU_D_FUNC(amoadd_d,  ADD)
AMO_EMU_D_FUNC(amomin_d,  MIN)
AMO_EMU_D_FUNC(amomax_d,  MAX)
AMO_EMU_D_FUNC(amominu_d, MINU)
AMO_EMU_D_FUNC(amomaxu_d, MAXU)
#endif

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
        val = csr_fcsr[current_thread] & 0x8000001f;
        break;
    case CSR_FRM:
        val = (csr_fcsr[current_thread] >> 5) & 0x7;
        break;
    case CSR_FCSR:
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
    case CSR_MSLEEP_TXFMA_27:
        val = csr_msleep_txfma_27[current_thread];
        break;
    case CSR_MENABLE_SHADOWS:
        val = csr_menable_shadows[current_thread];
        break;
    // TODO: CSR_EXCL_MODE
    case CSR_MTXFMA_SLEEP_TRAPS:
        val = csr_mtxfma_sleep_traps[current_thread];
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
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val = 0;
        break;
    case CSR_TENSOR_CONV_SIZE:
        val = 0;
        break;
    case CSR_TENSOR_CONV_CTRL:
        val = 0;
        break;
    case CSR_TENSOR_COOP:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val = 0;
        break;
    case CSR_TENSOR_MASK:
        val = csr_tensor_mask[current_thread];
        break;
    case CSR_TENSOR_QUANT:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val = 0;
        break;
    case CSR_TEX_SEND:
        val = 0;
        break;
    case CSR_TENSOR_ERROR:
        val = csr_tensor_error[current_thread];
        break;
    case CSR_UCACHE_CONTROL:
        val = csr_ucache_control[current_thread];
        break;
    case CSR_PREFETCH_VA:
    case CSR_FLB:
    case CSR_FCC:
    case CSR_STALL:
    case CSR_TENSOR_WAIT:
        val = 0;
        break;
    case CSR_TENSOR_LOAD:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val = 0;
        break;
    case CSR_GSC_PROGRESS:
        val = csr_gsc_progress[current_thread];
        break;
    case CSR_TENSOR_LOAD_L2:
        val = 0;
        break;
    case CSR_TENSOR_STORE:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val = 0;
        break;
    case CSR_EVICT_VA:
    case CSR_FLUSH_VA:
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
    case CSR_SLEEP_TXFMA_27:
        if (prvget() != CSR_PRV_M && (csr_menable_shadows[current_thread] & 2) == 0)
        {
            throw trap_illegal_instruction(current_inst);
        }
        val = csr_msleep_txfma_27[current_thread];
        break;
    case CSR_LOCK_VA:
    case CSR_UNLOCK_VA:
        val = 0;
        break;
    case CSR_PORTCTRL0:
    case CSR_PORTCTRL1:
    case CSR_PORTCTRL2:
    case CSR_PORTCTRL3:
        val = csr_portctrl[src1 - CSR_PORTCTRL0][current_thread];
        break;
    case CSR_FCCNB:
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
        val = (csr_fcsr[current_thread] & 0x000000E0) | (val & 0x8000001F);
        csr_fcsr[current_thread] = val;
        break;
    case CSR_FRM:
        val = (csr_fcsr[current_thread] & 0x8000001F) | ((val & 0x7) << 5);
        csr_fcsr[current_thread] = val;
        break;
    case CSR_FCSR:
        val &= 0x800000FF;
        csr_fcsr[current_thread] = val;
        break;
    case CSR_SSTATUS:
        // Preserve sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
        val = (val & 0x00000000000C6133ULL) | (csr_mstatus[current_thread] & 0x0000000F00739800ULL);
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
        val &= 0x0000000000000AAAULL;
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
    case CSR_MSLEEP_TXFMA_27:
        val &= 1;
        csr_msleep_txfma_27[current_thread] = val;
        csr_msleep_txfma_27[current_thread^1] = val;
        break;
    case CSR_MENABLE_SHADOWS:
        val &= 3;
        csr_menable_shadows[current_thread] = val;
        csr_menable_shadows[current_thread^1] = val;
        break;
    // TODO: CSR_EXCL_MODE:
    case CSR_MTXFMA_SLEEP_TRAPS:
        val &= 0x1f;
        csr_mtxfma_sleep_traps[current_thread] = val;
        csr_mtxfma_sleep_traps[current_thread^1] = val;
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
            uint64_t paddr = val & 0x0000FFFFFFFFFFC0ULL;
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
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        tensorreduce(val);
        break;
    case CSR_TENSOR_FMA:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        switch ((val >> 1) & 0x7)
        {
        case 0: tensor_fma32(val); break;
        case 1: tensor_fma16a32(val); break;
        case 3: tensor_ima8a32(val); break;
        default: /* nothing */ break;
        }
        break;
    case CSR_TENSOR_CONV_SIZE:
        val &= 0xFF00FFFFFF00FFFFULL;
        csr_tensor_conv_size[current_thread] = val;
        tmask_conv();
        break;
    case CSR_TENSOR_CONV_CTRL:
        val &= 0x0000FFFF0000FFFFULL;
        csr_tensor_conv_ctrl[current_thread] = val;
        tmask_conv();
        break;
    case CSR_TENSOR_COOP:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        val &= 0x0000000000FFFFFFULL;
        tcoop(val);
        break;
    case CSR_TENSOR_MASK:
        val &= 0xffff;
        csr_tensor_mask[current_thread] = val;
        break;
    case CSR_TENSOR_QUANT:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
        tensorquant(val);
        break;
    case CSR_TEX_SEND:
        //val &= 0xff;
        // Notify to TBOX that a Sample Request is ready
        // Thanks for making the code unreadable
        new_sample_request(current_thread,
                           val & 0xf,           // port_id
                           (val >> 4) & 0xf,    // num_packets
                           read_port_base_address(current_thread, val & 0xf /* port id */));
        break;
    case CSR_TENSOR_ERROR:
        val &= 0x1ff;
        csr_tensor_error[current_thread] = val;
        break;
    case CSR_UCACHE_CONTROL:
        msk = (csr_mcache_control[current_thread] & 1) ? 1 : 3;
        val = (csr_mcache_control[current_thread] & msk) | (val & ~msk & 0x07df);
        assert((val & 3) != 2);
        csr_ucache_control[current_thread] = val;
        csr_ucache_control[current_thread^1] = val;
        csr_mcache_control[current_thread] = val & 3;
        csr_mcache_control[current_thread^1] = val & 3;
        break;
    case CSR_PREFETCH_VA:
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
        fcc_cnt = val & 0x01;
        if (in_sysemu)
        {
            // If you are not going to block decrement it
            if (fcc[current_thread][fcc_cnt] != 0)
                fcc[current_thread][fcc_cnt]--;
        }
        else
        {
            // block if no credits
            if (fcc[current_thread][fcc_cnt] == 0 ) {
                fcc_wait[current_thread] = true;
                throw checker_wait_fcc();
            }
            else {
                // else, decrement
                fcc[current_thread][fcc_cnt]--;
            }
        }
        break;
    case CSR_STALL:
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_WAIT:
        // FIXME: Do something here?
        break;
    case CSR_TENSOR_LOAD:
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
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
        if (current_thread % EMU_THREADS_PER_MINION)
            throw trap_illegal_instruction(current_inst);
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
    case CSR_SLEEP_TXFMA_27:
        if (prvget() != CSR_PRV_M && (csr_menable_shadows[current_thread] & 2) == 0)
        {
            throw trap_illegal_instruction(current_inst);
        }
        val &= 1;
        csr_msleep_txfma_27[current_thread] = val;
        csr_msleep_txfma_27[current_thread^1] = val;
        break;
    case CSR_LOCK_VA:
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
        throw trap_illegal_instruction(current_inst);
    }
    if ((src1 == CSR_SATP) && ((csr_mstatus[current_thread] >> 20) & 1) && (prv == CSR_PRV_S))
    {
        throw trap_illegal_instruction(current_inst);
    }
    if (write)
    {
        switch (src1)
        {
            // Fast local barrier instructions encoded in the CSR space
            case CSR_FLB:
                oldval = flbarrier(newval);
                break;
            default:
                csrset(src1, newval);
                break;
        }
    }
    if (dst != x0)
    {
        XREGS[dst] = oldval;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- %s", oldval, csr_name(src1));
    }
    if (write)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " --> %s", newval, csr_name(src1));
    }
    log_xreg_write(dst, XREGS[dst]);
}

static void throw_page_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
        case Mem_Access_Load:
        case Mem_Access_TxLoad:
        case Mem_Access_Prefetch:
            throw trap_load_page_fault(addr);
            break;
        case Mem_Access_Store:
        case Mem_Access_TxStore:
        case Mem_Access_AtomicL:
        case Mem_Access_AtomicG:
        case Mem_Access_CacheOp:
            throw trap_store_page_fault(addr);
            break;
        case Mem_Access_Fetch:
            throw trap_instruction_page_fault(addr);
            break;
        case Mem_Access_PTW:
            assert(0);
            break;
    }
}

static void throw_access_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
        case Mem_Access_Load:
        case Mem_Access_TxLoad:
        case Mem_Access_Prefetch:
            throw trap_load_access_fault(addr);
            break;
        case Mem_Access_Store:
        case Mem_Access_TxStore:
        case Mem_Access_AtomicL:
        case Mem_Access_AtomicG:
        case Mem_Access_CacheOp:
            throw trap_store_access_fault(addr);
            break;
        case Mem_Access_Fetch:
            throw trap_instruction_access_fault(addr);
            break;
        case Mem_Access_PTW:
            assert(0);
            break;
    }
}

static uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc)
{
    // Read mstatus
    const uint64_t mstatus = csr_mstatus[current_thread];
    const int      mxr     = (mstatus >> MSTATUS_MXR ) & 0x1;
    const int      sum     = (mstatus >> MSTATUS_SUM ) & 0x1;
    const int      mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const int      mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;

    // Read satp
    const uint64_t satp      = csr_satp[current_thread];
    const uint64_t satp_mode = (satp >> 60) & 0xF;
    const uint64_t satp_ppn  = satp & PPN_M;

    // Calculate effective privilege level
    const int prv = (macc == Mem_Access_Fetch) ? prvget() : (mprv ? mpp : prvget());

    // V2P mappings are enabled when all of the following are true:
    // - the effective execution mode is not 'M'
    // - satp.mode is not "Bare"
    bool vm_enabled = (prv < CSR_PRV_M) && (satp_mode != SATP_MODE_BARE);

    if (!vm_enabled)
    {
        // Direct mapping
        return addr & PA_M;
    }

    int64_t sign;
    int Num_Levels;
    int PTE_top_Idx_Size;
    const int PTE_Size     = 8;
    const int PTE_Idx_Size = 9;
    switch (satp_mode)
    {
        case SATP_MODE_SV39:
            Num_Levels = 3;
            PTE_top_Idx_Size = 26;
            // bits 63-39 of address must be equal to bit 38
            sign = int64_t(addr) >> 38;
            break;
        case SATP_MODE_SV48:
            Num_Levels = 4;
            PTE_top_Idx_Size = 17;
            // bits 63-48 of address must be equal to bit 47
            sign = int64_t(addr) >> 47;
            break;
        default:
            assert(0); // we should never get here!
            break;
    }

    if (sign != int64_t(0) && sign != ~int64_t(0))
    {
        throw_page_fault(addr, macc);
    }

    const uint64_t pte_idx_mask     = (uint64_t(1) << PTE_Idx_Size) - 1;
    const uint64_t pte_top_idx_mask = (uint64_t(1) << PTE_top_Idx_Size) - 1;

    LOG(DEBUG, "Virtual memory enabled. Performing page walk on addr 0x%016" PRIx64 "...", addr);

    // Perform page walk. Anything that goes wrong raises a page fault error
    // for the access type of the original access, setting tval to the
    // original virtual address.
    uint64_t pte_addr, pte;
    bool pte_v, pte_r, pte_w, pte_x, pte_u, pte_a, pte_d;
    int level    = Num_Levels;
    uint64_t ppn = satp_ppn;
    do
    {
        if (--level < 0)
        {
            throw_page_fault(addr, macc);
        }

        // Take VPN[level]
        uint64_t vpn = (addr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
        // Read PTE
        pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
        if (!pma_check_ptw_access(pte_addr))
        {
            throw_access_fault(addr, macc);
        }
        pte = pmemread64(pte_addr);
        LOG(DEBUG, "\tPTW: %016" PRIx64 " <-- PMEM64[%016" PRIx64 "]", pte, pte_addr);

        // Read PTE fields
        pte_v = (pte >> PTE_V_OFFSET) & 0x1;
        pte_r = (pte >> PTE_R_OFFSET) & 0x1;
        pte_w = (pte >> PTE_W_OFFSET) & 0x1;
        pte_x = (pte >> PTE_X_OFFSET) & 0x1;
        pte_u = (pte >> PTE_U_OFFSET) & 0x1;
        pte_a = (pte >> PTE_A_OFFSET) & 0x1;
        pte_d = (pte >> PTE_D_OFFSET) & 0x1;
        // Read PPN
        ppn = (pte >> PTE_PPN_OFFSET) & PPN_M;

        // Check invalid entry
        if (!pte_v || (!pte_r && pte_w))
        {
            throw_page_fault(addr, macc);
        }

        // Check if PTE is a pointer to next table level
    }
    while (!pte_r && !pte_x);

    // A leaf PTE has been found

    // Check permissions. This is different for each access type.
    // Load accesses are permitted iff all the following are true:
    // - the page has read permissions or the page has execute permissions and mstatus.mxr is set
    // - if the effective execution mode is user, then the page permits user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits system-mode access (U=0 or SUM=1)
    // Store accesses are permitted iff all the following are true:
    // - the page has write permissions
    // - if the effective execution mode is user, then the page permits user-mode access (U=1)
    // - if the effective execution mode is system, then the page permits system-mode access (U=0 or SUM=1)
    // Instruction fetches are permitted iff all the following are true:
    // - the page has execute permissions
    // - if the execution mode is user, then the page permits user-mode access (U=1)
    // - if the execution mode is system, then the page does not permit user-mode access (U=0)
    switch (macc)
    {
        case Mem_Access_Load:
        case Mem_Access_TxLoad:
        case Mem_Access_Prefetch:
            if (!(pte_r || (mxr && pte_x)) ||
                ((prv == CSR_PRV_U) && !pte_u) ||
                ((prv == CSR_PRV_S) && pte_u && !sum))
            {
                throw_page_fault(addr, macc);
            }
            break;
        case Mem_Access_Store:
        case Mem_Access_TxStore:
        case Mem_Access_AtomicL:
        case Mem_Access_AtomicG:
        case Mem_Access_CacheOp:
            if (!pte_w ||
                ((prv == CSR_PRV_U) && !pte_u) ||
                ((prv == CSR_PRV_S) && pte_u && !sum))
            {
                throw_page_fault(addr, macc);
            }
            break;
        case Mem_Access_Fetch:
            if (!pte_x ||
                ((prv == CSR_PRV_U) && !pte_u) ||
                ((prv == CSR_PRV_S) && pte_u))
            {
                throw_page_fault(addr, macc);
            }
            break;
        case Mem_Access_PTW:
            assert(0);
            break;
    }

    // Check if it is a misaligned superpage
    if ((level > 0) && ((ppn & ((1<<(PTE_Idx_Size*level))-1)) != 0))
    {
        throw_page_fault(addr, macc);
    }

    // Check if A/D bit should be updated
    if (!pte_a || ((macc == Mem_Access_Store) && !pte_d))
    {
        throw_page_fault(addr, macc);
    }

    // Obtain physical address
    uint64_t paddr;

    // Copy page offset
    paddr = addr & PG_OFFSET_M;

    for (int i = 0; i < Num_Levels; i++)
    {
        // If level > 0, this is a superpage translation so VPN[level-1:0] are part of the page offset
        if (i < level)
        {
            paddr |= addr & (pte_idx_mask << (PG_OFFSET_SIZE + PTE_Idx_Size*i));
        }
        else if (i == Num_Levels-1)
        {
            paddr |= (ppn & (pte_top_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
        }
        else
        {
            paddr |= (ppn & (pte_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
        }
    }

    // Final physical address only uses 40 bits
    paddr &= PA_M;
    LOG(DEBUG, "\tPTW: Paddr = 0x%016" PRIx64, paddr);
    return paddr;
}

void ecall(const char* comm)
{
    LOG(DEBUG, "I: ecall%s%s", (comm?" # ":""), (comm?comm:""));
    switch (prvget())
    {
        case CSR_PRV_U: throw trap_user_ecall(); break;
        case CSR_PRV_S: throw trap_supervisor_ecall(); break;
        case CSR_PRV_M: throw trap_machine_ecall(); break;
        default       : assert(0); break;
    }
}

void ebreak(const char* comm)
{
    LOG(DEBUG, "I: ebreak%s%s", (comm?" # ":""), (comm?comm:""));
    // The spec says that hardware breakpoint sets mtval/stval to the current
    // PC but ebreak is a software breakpoint; should it also set mtval/stval
    // to the current PC or set it to 0?
    throw trap_breakpoint(current_pc);
}

void sret(const char* comm)
{
    uint64_t curprv = prvget();
    uint64_t mstatus = csr_mstatus[current_thread];
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 22) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    LOG(DEBUG, "I: sret%s%s", (comm?" # ":""), (comm?comm:""));
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

void mret(const char* comm)
{
    if (prvget() != CSR_PRV_M)
      throw trap_illegal_instruction(current_inst);

    LOG(DEBUG, "I: mret%s%s", (comm?" # ":""), (comm?comm:""));
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

void wfi(const char* comm)
{
    uint64_t curprv = prvget();
    uint64_t mstatus = csr_mstatus[current_thread];
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 21) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    LOG(DEBUG, "I: wfi%s%s", (comm?" # ":""), (comm?comm:""));
}

void sfence_vma(xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sfence_vma x%d, x%d%s%s", src1, src2, (comm?" # ":""), (comm?comm:""));
    throw trap_mcode_instruction(current_inst);
}

void csrrw(xreg dst, uint16_t src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrw x%d, %s, x%d%s%s", dst, csr_name(src1), src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = XREGS[src2];
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrs(xreg dst, uint16_t src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrs x%d, %s, x%d%s%s", dst, csr_name(src1), src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | XREGS[src2];
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrc(xreg dst, uint16_t src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrc x%d, %s, x%d%s%s", dst, csr_name(src1), src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~XREGS[src2]);
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrwi(xreg dst, uint16_t src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrwi x%d, %s, 0x%016" PRIx64 "%s%s", dst, csr_name(src1), imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = imm;
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrsi(xreg dst, uint16_t src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrsi x%d, %s, 0x%016" PRIx64 "%s%s", dst, csr_name(src1), imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | imm;
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

void csrrci(xreg dst, uint16_t src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrci x%d, %s, 0x%016" PRIx64 "%s%s", dst, csr_name(src1), imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~imm);
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64F emulation
//
////////////////////////////////////////////////////////////////////////////////

template<size_t Nelems, bool Mask>
static void femuld(freg dst, uint64_t vaddr)
{
    static_assert(Nelems <= VL, "Number of elements must be not be greater than VL");

    if (!Mask || MREGS[0].any())
    {
        check_load_breakpoint(vaddr);
        uint64_t paddr = vmemtranslate(vaddr, Mem_Access_Load);
        if (!pma_check_data_access(paddr, 4*Nelems, Mem_Access_Load))
        {
            throw trap_load_access_fault(vaddr);
        }
        if (!access_is_cacheable(paddr) && !access_is_size_aligned(vaddr, 4*Nelems))
        {
            throw trap_load_address_misaligned(vaddr);
        }
        uint64_t next_line = paddr & (L1D_LINE_SIZE - (paddr % L1D_LINE_SIZE));
        for (unsigned i = 0; i < Nelems; i++)
        {
            if (Mask && !MREGS[0][i]) continue;
            if (paddr >= next_line)
            {
                check_load_breakpoint(vaddr + i*4);
                paddr = vmemtranslate(vaddr + i*4, Mem_Access_Load);
                next_line += L1D_LINE_SIZE;
            }
            uint32_t val = pmemread32(paddr);
            FREGS[dst].u32[i] = val;
            LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]", i, val, vaddr + i*4);
            paddr += 4;
        }
    }
    ZERO_UNUSED_FREG_BITS(dst, Nelems);
    dirty_fp_state();
    log_freg_write(dst, FREGS[dst]);
}

template<size_t Nelems, bool Mask>
static void femust(freg src1, uint64_t vaddr)
{
    static_assert(Nelems <= VL, "Number of elements must be not be greater than VL");

    if (Mask && MREGS[0].none())
        return;

    check_store_breakpoint(vaddr);
    uint64_t paddr = vmemtranslate(vaddr, Mem_Access_Store);
    if (!pma_check_data_access(paddr, 4*Nelems, Mem_Access_Store))
    {
        throw trap_store_access_fault(vaddr);
    }
    if (!access_is_cacheable(paddr) && !access_is_size_aligned(vaddr, 4*Nelems))
    {
        throw trap_store_address_misaligned(vaddr);
    }
    uint64_t next_line = paddr & (L1D_LINE_SIZE - (paddr % L1D_LINE_SIZE));
    for (unsigned i = 0; i < Nelems; i++)
    {
        if (Mask && !MREGS[0][i]) continue;
        if (paddr >= next_line)
        {
            check_store_breakpoint(vaddr + i*4);
            paddr = vmemtranslate(vaddr + i*4, Mem_Access_Store);
            next_line += L1D_LINE_SIZE;
        }
        uint32_t val = FREGS[src1].u32[i];
        LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", i, val, vaddr + i*4);
        pmemwrite32(paddr, val);
        log_mem_write(i, 4, vaddr + i*4, val);
        paddr += 4;
    }
}

void fadd_s(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fadd.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_add(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}

void fsub_s(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fsub.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_sub(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}

void fmul_s(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fmul.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_mul(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}

void fdiv_s(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fdiv.s f%d,f%d,f%d,%s", fd, fs1, fs2, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fsgnj_s(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnj.s");
    WRITE_FD( fpu::f32_copySign(FS1.f32[0], FS2.f32[0]) );
}

void fsgnjn_s(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjn.s");
    WRITE_FD( fpu::f32_copySignNot(FS1.f32[0], FS2.f32[0]) );
}

void fsgnjx_s(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjx.s");
    WRITE_FD( fpu::f32_copySignXor(FS1.f32[0], FS2.f32[0]) );
}

void fmin_s(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.s");
    WRITE_FD( fpu::f32_minimumNumber(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}

void fmax_s(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.s");
    WRITE_FD( fpu::f32_maximumNumber(FS1.f32[0], FS2.f32[0]) );
    set_fp_exceptions();
}

void fsqrt_s(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fsqrt.s f%d,f%d,%s", fd, fs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void feq_s(xreg rd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_FS2("feq.s");
    bool tmp = fpu::f32_eq(FS1.f32[0], FS2.f32[0]);
    WRITE_RD(tmp);
    set_fp_exceptions();
}

void fle_s(xreg rd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_FS2("fle.s");
    bool tmp = fpu::f32_le(FS1.f32[0], FS2.f32[0]);
    WRITE_RD(tmp);
    set_fp_exceptions();
}

void flt_s(xreg rd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_FS2("flt.s");
    bool tmp = fpu::f32_lt(FS1.f32[0], FS2.f32[0]);
    WRITE_RD(tmp);
    set_fp_exceptions();
}

void fcvt_w_s(xreg rd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_RM("fcvt.w.s");
    set_rounding_mode(rm);
    int32_t tmp = fpu::f32_to_i32(FS1.f32[0]);
    WRITE_RD(sext<32>(tmp));
    set_fp_exceptions();
}

void fcvt_wu_s(xreg rd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_RM("fcvt.wu.s");
    set_rounding_mode(rm);
    uint32_t tmp = fpu::f32_to_ui32(FS1.f32[0]);
    WRITE_RD(sext<32>(tmp));
    set_fp_exceptions();
}

void fcvt_l_s(xreg rd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fcvt.l.s x%d,f%d,%s", rd, fs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fcvt_lu_s(xreg rd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fcvt.lu.s x%d,f%d,%s", rd, fs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fmv_x_w(xreg rd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1("fmv.x.w");
    WRITE_RD(sext<32>(FS1.u32[0]));
}

void fclass_s(xreg rd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1("fclass.s");
    WRITE_RD(sext<32>(fpu::f32_classify(FS1.f32[0])));
}

void fcvt_s_w(freg fd, xreg rs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_RS1_RM("fcvt.s.w");
    set_rounding_mode(rm);
    WRITE_FD( fpu::i32_to_f32(RS1) );
    set_fp_exceptions();
}

void fcvt_s_wu(freg fd, xreg rs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_RS1_RM("fcvt.s.wu");
    set_rounding_mode(rm);
    WRITE_FD( fpu::ui32_to_f32(RS1) );
    set_fp_exceptions();
}

void fcvt_s_l(freg fd, xreg rs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fcvt.s.l f%d,x%d,%s", fd, rs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fcvt_s_lu(freg fd, xreg rs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fcvt.s.lu f%d,x%d,%s", fd, rs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fmv_w_x(freg fd, xreg rs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_RS1("fmv.w.x");
    WRITE_FD( uint32_t(RS1) );
}

void flw(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: flw f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    uint64_t addr = sextVA(XREGS[base] + off);
    femuld<1,false>(dst, addr);
}

void fsw(freg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsw f%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    uint64_t addr = sextVA(XREGS[base] + off);
    femust<1,false>(src1, addr);
}

void fmadd_s(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmadd.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_mulAdd(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}

void fmsub_s(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmsub.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_mulSub(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}

void fnmsub_s(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmsub.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_subMulSub(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}

void fnmadd_s(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmadd.s");
    set_rounding_mode(rm);
    WRITE_FD( fpu::f32_subMulAdd(FS1.f32[0], FS2.f32[0], FS3.f32[0]) );
    set_fp_exceptions();
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto mask extension emulation
//
////////////////////////////////////////////////////////////////////////////////

static inline size_t popcount1(const mreg_t& m)
{
    return m.count();
}

static inline size_t popcount0(const mreg_t& m)
{
    return MLEN - m.count();
}

void maskand(mreg md, mreg ms1, mreg ms2, const char* comm __attribute__((unused)))
{
    DISASM_MD_MS1_MS2("maskand");
    WRITE_MD(MS1 & MS2);
}

void maskor(mreg md, mreg ms1, mreg ms2, const char* comm __attribute__((unused)))
{
    DISASM_MD_MS1_MS2("maskor");
    WRITE_MD(MS1 | MS2);
}

void maskxor(mreg md, mreg ms1, mreg ms2, const char* comm __attribute__((unused)))
{
    DISASM_MD_MS1_MS2("maskxor");
    WRITE_MD(MS1 ^ MS2);
}

void masknot(mreg md, mreg ms1, const char* comm __attribute__((unused)))
{
    DISASM_MD_MS1("masknot");
    WRITE_MD(~MS1);
}

void mova_x_m(xreg rd, const char* comm __attribute__((unused)))
{
    DISASM_RD_ALLMASK("mova.x.m");
    WRITE_RD((MREGS[0].to_ullong() << (0*MLEN)) +
             (MREGS[1].to_ullong() << (1*MLEN)) +
             (MREGS[2].to_ullong() << (2*MLEN)) +
             (MREGS[3].to_ullong() << (3*MLEN)) +
             (MREGS[4].to_ullong() << (4*MLEN)) +
             (MREGS[5].to_ullong() << (5*MLEN)) +
             (MREGS[6].to_ullong() << (6*MLEN)) +
             (MREGS[7].to_ullong() << (7*MLEN)));
}

void mova_m_x(xreg rs1, const char* comm __attribute__((unused)))
{
    DISASM_RS1("mova.m.x");
    uint64_t tmp = RS1;
    WRITE_MREG(0, tmp >> (0*MLEN));
    WRITE_MREG(1, tmp >> (1*MLEN));
    WRITE_MREG(2, tmp >> (2*MLEN));
    WRITE_MREG(3, tmp >> (3*MLEN));
    WRITE_MREG(4, tmp >> (4*MLEN));
    WRITE_MREG(5, tmp >> (5*MLEN));
    WRITE_MREG(6, tmp >> (6*MLEN));
    WRITE_MREG(7, tmp >> (7*MLEN));
}

void mov_m_x(mreg md, xreg rs1, unsigned uimm8, const char* comm __attribute__((unused)))
{
    DISASM_MD_RS1_UIMM8("mov.m.x");
    WRITE_MD(uint32_t(RS1) | UIMM8);
}

void maskpopc(xreg rd, mreg ms1, const char* comm __attribute__((unused)))
{
    DISASM_RD_MS1("maskpopc");
    WRITE_RD(popcount1(MS1));
}

void maskpopcz(xreg rd, mreg ms1, const char* comm __attribute__((unused)))
{
    DISASM_RD_MS1("maskpopcz");
    WRITE_RD(popcount0(MS1));
}

void maskpopc_rast(xreg rd, mreg ms1, mreg ms2, unsigned umsk4, const char* comm __attribute__((unused)))
{
    DISASM_RD_MS1_MS2_UMSK4("maskpopc.rast");
    mreg_t m1, m2;
    switch (UMSK4) {
    case 0  : m2 = 0x0f; m1 = 0x0f; break;
    case 1  : m2 = 0x3c; m1 = 0x3c; break;
    case 2  : m2 = 0xf0; m1 = 0xf0; break;
    default : m2 = 0xff; m1 = 0xff; break;
    }
    WRITE_RD(popcount1(MS1 & m1) + popcount1(MS2 & m2));
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto packed-single extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Load and store ------------------------------------

void flq2(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: flq2 f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    uint64_t addr = sextVA(XREGS[base] + off);
    femuld<VL,false>(dst, addr);
}

void flw_ps(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: flw.ps f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base] + off);
    femuld<VL,true>(dst, addr);
}

void flwl_ps(freg dst, xreg base, const char* comm)
{
    LOG(DEBUG, "I: flwl.ps f%d, (x%d)%s%s", dst, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base]);
    if (MREGS[0].any() && !access_is_size_aligned(addr, VL*4))
    {
        throw trap_load_address_misaligned(addr);
    }
    femuld<VL,true>(dst, addr);
}

void flwg_ps(freg dst, xreg base, const char* comm)
{
    LOG(DEBUG, "I: flwg.ps f%d, (x%d)%s%s", dst, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base]);
    if (MREGS[0].any() && !access_is_size_aligned(addr, VL*4))
    {
        throw trap_load_address_misaligned(addr);
    }
    femuld<VL,true>(dst, addr);
}

void fsq2(freg src, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsq2 f%d, %" PRId64 "(x%d)%s%s", src, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    uint64_t addr = sextVA(XREGS[base] + off);
    femust<VL,false>(src, addr);
}

void fsw_ps(freg src, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsw.ps f%d, %" PRId64 "(x%d)%s%s", src, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base] + off);
    femust<VL,true>(src, addr);
}

void fswl_ps(freg src, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fswl.ps f%d, (x%d)%s%s", src, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base]);
    if (MREGS[0].any() && !access_is_size_aligned(addr, VL*4))
    {
        throw trap_load_address_misaligned(addr);
    }
    femust<VL,true>(src, addr);
}

void fswg_ps(freg src, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fswg.ps f%d, (x%d)%s%s", src, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    uint64_t addr = sextVA(XREGS[base]);
    if (MREGS[0].any() && !access_is_size_aligned(addr, VL*4))
    {
        throw trap_load_address_misaligned(addr);
    }
    femust<VL,true>(src, addr);
}

// ----- Broadcast -----------------------------------------

void fbc_ps(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fbc_ps f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    iufval32 val;
    uint64_t addr = sextVA(XREGS[base] + off);
    val.u = 0;
    if (MREGS[0].any())
    {
        check_load_breakpoint(addr);
        val.u = vmemread32(addr);
    }
    for (unsigned i = 0; i < VL; i++)
    {
        if (MREGS[0][i])
        {
            FREGS[dst].u32[i] = val.u;
            LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]", i, val.u, addr);
        }
    }
    dirty_fp_state();
    log_freg_write(dst, FREGS[dst]);
}

void fbci_ps(freg fd, uint32_t f32imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_F32IMM("fbci.ps");
    WRITE_VD( F32IMM );
}

void fbcx_ps(freg fd, xreg rs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_RS1("fbcx.ps");
    WRITE_VD( uint32_t(RS1) );
}

// ----- Gather and scatter --------------------------------

static void gatheremu(opcode opc, freg dst, freg src1, xreg base)
{
    uint64_t baddr = XREGS[base];
    unsigned elem = csr_gsc_progress[current_thread];
    bool dirty = false;
    try
    {
        while (elem < VL)
        {
            if (MREGS[0][elem])
            {
                iufval32 val;
                int32_t off   = FREGS[src1].i32[elem];
                uint64_t addr = sextVA(baddr + off);
                check_load_breakpoint(addr);
                switch (opc)
                {
                case FGW:
                    val.u = vmemread32(addr);
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM8[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case FGH:
                    val.u = sext<16>(vmemread16(addr));
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM16[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case FGBL:
                case FGBG:
                case FGB:
                    val.u = sext<8>(vmemread8(addr));
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM8[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case FGWL:
                case FGWG:
                    val.u = aligned_vmemread32(addr);
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case FGHL:
                case FGHG:
                    val.u = sext<16>(aligned_vmemread16(addr));
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM16[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                default:
                    assert(0);
                    break;
                }
                FREGS[dst].u32[elem] = val.u;
                dirty = true;
            }
            ++elem;
        }
    }
    catch (const trap_t&)
    {
        csr_gsc_progress[current_thread] = elem;
        log_gsc_progress(elem, false);
        if (dirty)
        {
            dirty_fp_state();
            log_freg_write(dst, FREGS[dst]);
        }
        throw;
    }
    csr_gsc_progress[current_thread] = 0;
    log_gsc_progress(0, true);
    if (dirty)
        dirty_fp_state();
    log_freg_write(dst, FREGS[dst]);
}

static void gatheremu32(int size, freg dst, xreg src1, xreg src2)
{
    uint64_t baddr = sextVA(XREGS[src2]);
    uint64_t index = XREGS[src1];
    unsigned elem = csr_gsc_progress[current_thread];
    bool dirty = false;
    try
    {
        while (elem < VL)
        {
            if (MREGS[0][elem])
            {
                check_store_breakpoint(baddr);
                uint64_t off;
                uint64_t addr;
                switch(size)
                {
                case 1 : off =  (index >> (elem * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
                case 2 : off = ((index >> (elem * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
                case 4 : off = ((index >> (elem * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
                default: assert(0); break;
                }
                iufval32 val;
                switch (size)
                {
                case 1:
                    val.u = sext<8>(vmemread8(addr));
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM8[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case 2:
                    val.u = sext<16>(vmemread16(addr));
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM16[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                case 4:
                    val.u = vmemread32(addr);
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]", elem, val.u, addr);
                    break;
                default:
                    assert(0);
                    break;
                }
                FREGS[dst].u32[elem] = val.u;
                dirty = true;
            }
            ++elem;
        }
    }
    catch (const trap_t&)
    {
        csr_gsc_progress[current_thread] = elem;
        log_gsc_progress(elem, false);
        if (dirty)
        {
            dirty_fp_state();
            log_freg_write(dst, FREGS[dst]);
        }
        throw;
    }
    csr_gsc_progress[current_thread] = 0;
    log_gsc_progress(0, true);
    if (dirty)
        dirty_fp_state();
    log_freg_write(dst, FREGS[dst]);
}

static void femuscat(opcode opc, freg src1, freg src2, xreg base)
{
    uint64_t baddr = XREGS[base];
    unsigned elem = csr_gsc_progress[current_thread];
    try
    {
        while (elem < VL)
        {
            if (MREGS[0][elem])
            {
                int32_t  off  = FREGS[src2].i32[elem];
                uint64_t addr = sextVA(baddr + off);
                iufval32 val;
                val.u = FREGS[src1].u32[elem];

                check_store_breakpoint(addr);
                switch (opc)
                {
                case FSCW:
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", elem, val.u, addr);
                    vmemwrite32(addr, val.u);
                    log_mem_write(elem, 4, addr, val.u);
                    break;
                case FSCH:
                    LOG(DEBUG, "\t[%u] 0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", elem, uint16_t(val.u), addr);
                    vmemwrite16(addr, uint16_t(val.u));
                    log_mem_write(elem, 2, addr, val.u);
                    break;
                case FSCBL:
                case FSCBG:
                case FSCB:
                    LOG(DEBUG, "\t[%u] 0x%02" PRIx8 " --> MEM8[0x%016" PRIx64 "]", elem, uint8_t(val.u), addr);
                    vmemwrite8(addr, uint8_t(val.u));
                    log_mem_write(elem, 1, addr, val.u);
                    break;
                case FSCWL:
                case FSCWG:
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", elem, val.u, addr);
                    aligned_vmemwrite32(addr, val.u);
                    log_mem_write(elem, 4, addr, val.u);
                    break;
                case FSCHL:
                case FSCHG:
                    LOG(DEBUG, "\t[%u] 0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", elem, uint16_t(val.u), addr);
                    aligned_vmemwrite16(addr, uint16_t(val.u));
                    log_mem_write(elem, 2, addr, val.u);
                    break;
                default:
                    assert(0);
                    break;
                }
            }
            ++elem;
        }
    }
    catch (const trap_t&)
    {
        csr_gsc_progress[current_thread] = elem;
        log_gsc_progress(elem, false);
        throw;
    }
    csr_gsc_progress[current_thread] = 0;
    log_gsc_progress(0, true);
}

static void femuscat32(int size, freg src3, xreg src1, xreg src2)
{
    uint64_t baddr = sextVA(XREGS[src2]);
    uint64_t index = XREGS[src1];
    unsigned elem = csr_gsc_progress[current_thread];
    try
    {
        while (elem < VL)
        {
            if (MREGS[0][elem])
            {
                check_store_breakpoint(baddr);
                uint64_t off;
                uint64_t addr;
                switch(size)
                {
                case 1 : off =  (index >> (elem * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
                case 2 : off = ((index >> (elem * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
                case 4 : off = ((index >> (elem * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
                default: assert(0); break;
                }

                iufval32 val;
                val.u = FREGS[src3].u32[elem];
                switch (size)
                {
                case 1:
                    LOG(DEBUG, "\t[%u] 0x%02" PRIx8 " --> MEM8[0x%08" PRIx64 "]", elem, uint8_t(val.u), addr);
                    vmemwrite8(addr, uint8_t(val.u));
                    log_mem_write(elem, 1, addr, val.u);
                    break;
                case 2:
                    LOG(DEBUG, "\t[%u] 0x%04" PRIx16 " --> MEM16[0x%016" PRIx64 "]", elem, uint16_t(val.u), addr);
                    vmemwrite16(addr, uint16_t(val.u));
                    log_mem_write(elem, 2, addr, val.u);
                    break;
                case 4:
                    LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", elem, val.u, addr);
                    vmemwrite32(addr, val.u);
                    log_mem_write(elem, 4, addr, val.u);
                    break;
                default:
                    assert(0);
                    break;
                }
            }
            ++elem;
        }
    }
    catch (const trap_t&)
    {
        csr_gsc_progress[current_thread] = elem;
        log_gsc_progress(elem, false);
        throw;
    }
    csr_gsc_progress[current_thread] = 0;
    log_gsc_progress(0, true);
}

void fgb_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgb.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGB, dst, src1, base);
}

void fgh_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgh.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGH, dst, src1, base);
}

void fgw_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgw.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGW, dst, src1, base);
}

void fgwl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgwl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGWL, dst, src1, base);
}

void fghl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fghl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGHL, dst, src1, base);
}

void fgbl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgbl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGBL, dst, src1, base);
}

void fgwg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgwg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGWG, dst, src1, base);
}

void fghg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fghg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGHG, dst, src1, base);
}

void fgbg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgbg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGBG, dst, src1, base);
}

void fg32b_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32b.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(1, dst, src1, src2);
}

void fg32h_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32h.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(2, dst, src1, src2);
}

void fg32w_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32w.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(4, dst, src1, src2);
}

void fscb_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscb.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCB, src, src1, base);
}

void fsch_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fsch.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCH, src, src1, base);
}

void fscw_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscw.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCW, src, src1, base);
}

void fscwl_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscwl.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCWL, src, src1, base);
}

void fschl_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fschl.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCHL, src, src1, base);
}

void fscbl_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscbl.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCBL, src, src1, base);
}

void fscwg_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscwg.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCWG, src, src1, base);
}

void fschg_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fschg.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCHG, src, src1, base);
}

void fscbg_ps(freg src, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fscbg.ps f%d, f%d(x%d)%s%s", src, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuscat(FSCBG, src, src1, base);
}

void fsc32b_ps(freg src, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fsc32b.ps f%d, x%d(x%d)%s%s", src, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat32(1, src, src1, src2);
}

void fsc32h_ps(freg src, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fsc32h.ps f%d, x%d(x%d)%s%s", src, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat32(2, src, src1, src2);
}

void fsc32w_ps(freg src, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fsc32w.ps f%d, x%d(x%d)%s%s", src, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat32(4, src, src1, src2);
}

// ----- Computational (follows RV64F) ---------------------

void fadd_ps(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fadd.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_add(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fsub_ps(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fsub.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_sub(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fmul_ps(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_RM("fmul.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_mul(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fdiv_ps(freg fd, freg fs1, freg fs2, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fdiv.ps f%d,f%d,f%d,%s", fd, fs1, fs2, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void fsgnj_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnj.ps");
    WRITE_VD( fpu::f32_copySign(FS1.f32[e], FS2.f32[e]) );
}

void fsgnjn_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjn.ps");
    WRITE_VD( fpu::f32_copySignNot(FS1.f32[e], FS2.f32[e]) );
}

void fsgnjx_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsgnjx.ps");
    WRITE_VD( fpu::f32_copySignXor(FS1.f32[e], FS2.f32[e]) );
}

void fmin_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.ps");
    WRITE_VD( fpu::f32_minimumNumber(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fmax_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.ps");
    WRITE_VD( fpu::f32_maximumNumber(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fsqrt_ps(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fsqrt.ps f%d,f%d,%s", fd, fs1, get_rounding_mode(rm));
    throw trap_mcode_instruction(current_inst);
}

void feq_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("feq.ps");
    WRITE_VD( fpu::f32_eq(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions();
}

void fle_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fle.ps");
    WRITE_VD( fpu::f32_le(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions();
}

void flt_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("flt.ps");
    WRITE_VD( fpu::f32_lt(FS1.f32[e], FS2.f32[e]) ? UINT32_MAX : 0 );
    set_fp_exceptions();
}

void feqm_ps(mreg md, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_MD_FS1_FS2("feqm.ps");
    WRITE_VMD( fpu::f32_eq(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void flem_ps(mreg md, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_MD_FS1_FS2("flem.ps");
    WRITE_VMD( fpu::f32_le(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fltm_ps(mreg md, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_MD_FS1_FS2("fltm.ps");
    WRITE_VMD( fpu::f32_lt(FS1.f32[e], FS2.f32[e]) );
    set_fp_exceptions();
}

void fsetm_pi(mreg md, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_MD_FS1("fsetm.pi");
    WRITE_VMD( !!FS1.u32[e] );
    set_fp_exceptions();
}

void fcmov_ps(freg fd, freg fs1, freg fs2, freg fs3, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3("fcmov.ps");
    WRITE_VD( FS1.u32[e] ? FS2.u32[e] : FS3.u32[e] );
}

void fcmovm_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fcmovm.ps");
    LOG_MREG0;
    WRITE_VD_NOMASK( M0[e] ? FS1.u32[e] : FS2.u32[e] );
}

void fmvz_x_ps(xreg rd, freg fs1, unsigned uimm3, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_UIMM3("fmvz.x.ps");
    WRITE_RD(FS1.u32[UIMM3]);
}

void fmvs_x_ps(xreg rd, freg fs1, unsigned uimm3, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_RD_FS1_UIMM3("fmvs.x.ps");
    WRITE_RD(sext<32>(FS1.u32[UIMM3]));
}

void fswizz_ps(freg fd, freg fs1, unsigned uimm8, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_UIMM8("fswizz.ps");
    freg_t tmp = FS1;
    WRITE_VD( tmp.u32[(e & ~3) | ((UIMM8 >> ((2*e) % 8)) & 3)] );
}

void fcvt_pw_ps(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.pw.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_to_i32(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_pwu_ps(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.pwu.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_to_ui32(FS1.f32[e]) );
    set_fp_exceptions();
}

void fclass_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fclass.ps");
    WRITE_VD( fpu::f32_classify(FS1.f32[e]) );
}

void fcvt_ps_pw(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.pw");
    set_rounding_mode(rm);
    WRITE_VD( fpu::i32_to_f32(FS1.i32[e]) );
    set_fp_exceptions();
}

void fcvt_ps_pwu(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.pwu");
    set_rounding_mode(rm);
    WRITE_VD( fpu::ui32_to_f32(FS1.u32[e]) );
    set_fp_exceptions();
}

void fmadd_ps(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmadd.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_mulAdd(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions();
}

void fmsub_ps(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fmsub.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_mulSub(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions();
}

void fnmsub_ps(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmsub.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_subMulSub(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions();
}

void fnmadd_ps(freg fd, freg fs1, freg fs2, freg fs3, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2_FS3_RM("fnmadd.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_subMulAdd(FS1.f32[e], FS2.f32[e], FS3.f32[e]) );
    set_fp_exceptions();
}

// ----- Graphics upconvert --------------------------------

void fcvt_ps_f16(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f16");
    WRITE_VD( fpu::f16_to_f32(FS1.f16[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_f11(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f11");
    WRITE_VD( fpu::f11_to_f32(FS1.f11[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_f10(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.f10");
    WRITE_VD( fpu::f10_to_f32(FS1.f10[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_un24(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.un24");
    WRITE_VD( fpu::un24_to_f32(FS1.u32[e]) );
    set_fp_exceptions();
}

void fcvt_ps_un16(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.un16");
    WRITE_VD( fpu::un16_to_f32(FS1.u16[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_un10(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.un10");
    WRITE_VD( fpu::un10_to_f32(FS1.u16[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_un8(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.un8");
    WRITE_VD( fpu::un8_to_f32(FS1.u8[4*e]) );
    set_fp_exceptions();
}

void fcvt_ps_un2(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.un2");
    WRITE_VD( fpu::un2_to_f32(FS1.u8[4*e]) );
    set_fp_exceptions();
}

void fcvt_ps_sn16(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.sn16");
    WRITE_VD( fpu::sn16_to_f32(FS1.i16[2*e]) );
    set_fp_exceptions();
}

void fcvt_ps_sn8(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.ps.sn8");
    WRITE_VD( fpu::sn8_to_f32(FS1.i8[4*e]) );
    set_fp_exceptions();
}

// ----- Graphics downconvert ------------------------------

void fcvt_f16_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.f16.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_f16(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_f11_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.f11.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_f11(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_f10_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.f10.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_f10(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_un24_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.un24.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_un24(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_un16_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.un16.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_un16(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_un10_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.un10.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_un10(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_un8_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.un8.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_un8(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_un2_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.un2.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_un2(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_sn16_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.sn16.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_sn16(FS1.f32[e]) );
    set_fp_exceptions();
}

void fcvt_sn8_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.sn8.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_sn8(FS1.f32[e]) );
    set_fp_exceptions();
}

// ----- Graphics additional -------------------------------

static inline float32_t fsin_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_sin2pi(x);
    float32_t gldval = gld::f32_sin2pi(x);
    if (security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(DEBUG, "SIN TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x.v, fpuval.v, gldval.v);
        LOG(WARN, "Trans mismatch error for operation FSIN with input: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x.v);
    }
    return fpuval;
}

static inline float32_t fexp_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_exp2(x);
    float32_t gldval = gld::f32_exp2(x);
    if (security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(DEBUG, "EXP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x.v, fpuval.v, gldval.v);
        LOG(WARN, "Trans mismatch error for operation FEXP with input: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x.v);
    }
    return fpuval;
}

static inline float32_t flog_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_log2(x);
    float32_t gldval = gld::f32_log2(x);
    if (security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(DEBUG, "LOG TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x.v, fpuval.v, gldval.v);
        LOG(WARN, "Trans mismatch error for operation FLOG with input: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x.v);
    }
    return fpuval;
}

static inline float32_t frcp_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_rcp(x);
    float32_t gldval = gld::f32_rcp(x);
    if (security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(DEBUG, "RCP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x.v, fpuval.v, gldval.v);
        LOG(WARN, "Trans mismatch error for operation FRCP with input: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x.v);
    }
    return fpuval;
}

static inline float32_t frsq_vs_gold(float32_t x)
{
    float32_t fpuval = fpu::f32_rsqrt(x);
    float32_t gldval = gld::f32_rsqrt(x);
    if (security_ulp_check(gldval.v, fpuval.v))
    {
        LOG(DEBUG, "RSQ TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x.v, fpuval.v, gldval.v);
        LOG(WARN, "Trans mismatch error for operation FRSQ with input: 0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x.v);
    }
    return fpuval;
}

static inline int32_t frcp_fix_rast_vs_gold(int32_t x, int32_t y)
{
    int32_t fpuval = fpu::fxp1714_rcpStep(x, y);
    int32_t gldval = gld::fxp1714_rcpStep(x, y);
    if (abs(fpuval - gldval) > 1)
    {
        LOG(DEBUG, "FRCP_FIX_RAST TRANS\tIN: 0x%08x,0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", x, y, fpuval, gldval);
        LOG(WARN, "Trans mismatch error for operation FRCP_FIX_RAST with input: 0x%08x,0x%08x. This might happen, report to jordi.sola@esperantotech.com if needed.", x, y);
    }
    return fpuval;
}

void fsin_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fsin.ps");
    WRITE_VD( fsin_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}

void fexp_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fexp.ps");
    WRITE_VD( fexp_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}

void flog_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("flog.ps");
    WRITE_VD( flog_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}

void ffrc_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("ffrc.ps");
    WRITE_VD( fpu::f32_frac(FS1.f32[e]) );
    set_fp_exceptions();
}

void fround_ps(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fround.ps");
    set_rounding_mode(rm);
    WRITE_VD( fpu::f32_roundToInt(FS1.f32[e]) );
    set_fp_exceptions();
}

void frcp_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("frcp.ps");
    WRITE_VD( frcp_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}

void frsq_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("frsq.ps");
    WRITE_VD( frsq_vs_gold(FS1.f32[e]) );
    set_fp_exceptions();
}

void cubeface_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FDS0_FS1_FS2("cubeface.ps");
    WRITE_VD( (FD.u32[e] & 1)
              ? ((FS2.u32[e] & 1) ? 0 : 1)
              : ((FS1.u32[e] & 1) ? 0 : 2) );
}

void cubefaceidx_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("cubefaceidx.ps");
    WRITE_VD( fpu::f32_cubeFaceIdx(uint8_t(FS1.u32[e]), FS2.f32[e]) );
    set_fp_exceptions();
}

void cubesgnsc_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("cubesgnsc.ps");
    WRITE_VD( fpu::f32_cubeFaceSignS(uint8_t(FS1.u32[e]), FS2.f32[e]) );
}

void cubesgntc_ps(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("cubesgntc.ps");
    WRITE_VD( fpu::f32_cubeFaceSignT(uint8_t(FS1.u32[e]), FS2.f32[e]) );
}

void fcvt_ps_rast(freg fd, freg fs1, rounding_mode rm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_RM("fcvt.ps.rast");
    set_rounding_mode(rm);
    WRITE_VD( fpu::fxp1516_to_f32(FS1.i32[e]) );
    set_fp_exceptions();
}

void fcvt_rast_ps(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fcvt.rast.ps");
    set_rounding_mode(rmdyn);
    WRITE_VD( fpu::f32_to_fxp1714(FS1.f32[e]) );
    set_fp_exceptions();
}

void frcp_fix_rast(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("frcp.fix.rast");
    set_rounding_mode(rmdyn);
    WRITE_VD( frcp_fix_rast_vs_gold(FS1.i32[e], FS2.i32[e]) );
    set_fp_exceptions();
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto packed-integer extension emulation
//
////////////////////////////////////////////////////////////////////////////////

static inline uint8_t sat8(int32_t x)
{
    return std::min(INT8_MAX, std::max(INT8_MIN, x));
}

static inline uint8_t satu8(int32_t x)
{
    return std::min(UINT8_MAX, std::max(0, x));
}

void fbci_pi(freg fd, int32_t i32imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_I32IMM("fbci.pi");
    WRITE_VD( I32IMM );
}

void feq_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("feq.pi");
    WRITE_VD( (FS1.u32[e] == FS2.u32[e]) ? UINT32_MAX : 0 );
}

void fle_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fle.pi");
    WRITE_VD( (FS1.i32[e] <= FS2.i32[e]) ? UINT32_MAX : 0 );
}

void flt_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("flt.pi");
    WRITE_VD( (FS1.i32[e] < FS2.i32[e]) ? UINT32_MAX : 0 );
}

void fltu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fltu.pi");
    WRITE_VD( (FS1.u32[e] < FS2.u32[e]) ? UINT32_MAX : 0 );
}

void fltm_pi(mreg md, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_MD_FS1_FS2("fltm.pi");
    WRITE_VMD( FS1.i32[e] < FS2.i32[e] );
}

void faddi_pi(freg fd, freg fs1, int32_t v_imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_VIMM("faddi.pi");
    WRITE_VD( FS1.u32[e] + VIMM );
}

void fslli_pi(freg fd, freg fs1, int32_t v_imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_UVIMM("fslli.pi");
    WRITE_VD( FS1.u32[e] << uint32_t(VIMM) );
}

void fsrli_pi(freg fd, freg fs1, int32_t v_imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_UVIMM("fsrli.pi");
    WRITE_VD( FS1.u32[e] >> uint32_t(VIMM) );
}

void fsrai_pi(freg fd, freg fs1, int32_t v_imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_UVIMM("fsrai.pi");
    WRITE_VD( FS1.i32[e] >> uint32_t(VIMM) );
}

void fandi_pi(freg fd, freg fs1, int32_t v_imm, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_UVIMM("fandi.pi");
    WRITE_VD( FS1.u32[e] & uint32_t(VIMM) );
}

void fadd_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fadd.pi");
    WRITE_VD( FS1.u32[e] + FS2.u32[e] );
}

void fsub_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsub.pi");
    WRITE_VD( FS1.u32[e] - FS2.u32[e] );
}

void fsll_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsll.pi");
    WRITE_VD( (FS2.u32[e] >= 32) ? 0 : (FS1.u32[e] << FS2.u32[e]) );
}

void fxor_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fxor.pi");
    WRITE_VD( FS1.u32[e] ^ FS2.u32[e] );
}

void fsrl_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsrl.pi");
    WRITE_VD( (FS2.u32[e] >= 32) ? 0 : (FS1.u32[e] >> FS2.u32[e]) );
}

void fsra_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fsra.pi");
    WRITE_VD( FS1.i32[e] >> std::min(FS2.u32[e], 31u) );
}

void for_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("for.pi");
    WRITE_VD( FS1.u32[e] | FS2.u32[e] );
}

void fand_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fand.pi");
    WRITE_VD( FS1.u32[e] & FS2.u32[e] );
}

void fnot_pi(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fnot.pi");
    WRITE_VD( ~FS1.u32[e] );
}

void fsat8_pi(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fsat8.pi");
    WRITE_VD( sat8(FS1.i32[e]) );
}

void fsatu8_pi(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fsatu8.pi");
    WRITE_VD( satu8(FS1.i32[e]) );
}

void fpackreph_pi(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fpackreph.pi");
    freg_t tmp = FS1;
    WRITE_VD( uint32_t(tmp.u16[(4*e+0)%(VLEN/16)])
              | (uint32_t(tmp.u16[(4*e+2)%(VLEN/16)]) << 16) );
}

void fpackrepb_pi(freg fd, freg fs1, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1("fpackrepb.pi");
    freg_t tmp = FS1;
    WRITE_VD( uint32_t(tmp.u8[(16*e+0)%(VLEN/8)])
              | (uint32_t(tmp.u8[(16*e+4)%(VLEN/8)]) << 8)
              | (uint32_t(tmp.u8[(16*e+8)%(VLEN/8)]) << 16)
              | (uint32_t(tmp.u8[(16*e+12)%(VLEN/8)]) << 24) );
}

void fmul_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmul.pi");
    WRITE_VD( FS1.u32[e] * FS2.u32[e] );
}

void fmulh_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmulh.pi");
    WRITE_VD( (int64_t(FS1.i32[e]) * int64_t(FS2.i32[e]) >> 32) );
}

void fmulhu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmulhu.pi");
    WRITE_VD( (uint64_t(FS1.u32[e]) * uint64_t(FS2.u32[e]) >> 32) );
}

void fdiv_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fdiv.pi f%d,f%d,f%d", fd, fs1, fs2);
    throw trap_mcode_instruction(current_inst);
}

void fdivu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fdivu.pi f%d,f%d,f%d", fd, fs1, fs2);
    throw trap_mcode_instruction(current_inst);
}

void frem_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: frem.pi f%d,f%d,f%d", fd, fs1, fs2);
    throw trap_mcode_instruction(current_inst);
}

void fremu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    LOG(DEBUG, "I: fremu.pi f%d,f%d,f%d", fd, fs1, fs2);
    throw trap_mcode_instruction(current_inst);
}

void fmin_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmin.pi");
    WRITE_VD( std::min(FS1.i32[e], FS2.i32[e]) );
}

void fmax_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmax.pi");
    WRITE_VD( std::max(FS1.i32[e], FS2.i32[e]) );
}

void fminu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fminu.pi");
    WRITE_VD( std::min(FS1.u32[e], FS2.u32[e]) );
}

void fmaxu_pi(freg fd, freg fs1, freg fs2, const char* comm __attribute__((unused)))
{
    require_fp_active();
    DISASM_FD_FS1_FS2("fmaxu.pi");
    WRITE_VD( std::max(FS1.u32[e], FS2.u32[e]) );
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto scalar extension for graphics emulation
//
////////////////////////////////////////////////////////////////////////////////

static inline uint_fast16_t bitmixb(uint_fast16_t sel, uint_fast16_t val0, uint_fast16_t val1)
{
    uint_fast16_t val = 0;
    for (unsigned pos = 0; pos < 16; ++pos) {
        if (sel & 1) {
            val |= ((val1 & 1) << pos);
            val1 >>= 1;
        } else {
            val |= ((val0 & 1) << pos);
            val0 >>= 1;
        }
        sel >>= 1;
    }
    return val;
}

void packb(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("packb");
    WRITE_RD( (RS1 & 0xff) | ((RS2 & 0xff) << 8) );
}

void bitmixb(xreg rd, xreg rs1, xreg rs2, const char* comm __attribute__((unused)))
{
    DISASM_RD_RS1_RS2("bitmixb");
    WRITE_RD( bitmixb(uint16_t(RS1), uint16_t(RS2), uint16_t(RS2 >> 8)) );
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto Atomics extension emulation
//
////////////////////////////////////////////////////////////////////////////////

//
// NB: BEMU does not differentiate between local and global atomic ops because
// memory is flat
//

#define AMO_EMU_F_FUNC(NAME, LG, OPC) \
void NAME(freg dst, freg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " f%d, f%d(x%d)%s%s", dst, src1, src2, comm ? " # " : "", comm ? comm : "");\
   amo_emu_f(OPC, dst, src1, src2, Mem_Access_Atomic ## LG);\
}

static float32_t amo_max_f32(float32_t a, float32_t b)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool signA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signB;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if (isNaNF32UI(uiA) || isNaNF32UI(uiB)) {
        if (softfloat_isSigNaNF32UI(uiA) || softfloat_isSigNaNF32UI(uiB)) {
            uiZ = defaultNaNF32UI;
            goto uiZ;
        }
        uiZ = isNaNF32UI(uiA) ? (isNaNF32UI(uiB) ? defaultNaNF32UI : uiB) : uiA;
        goto uiZ;
    }
    signA = signF32UI(uiA);
    signB = signF32UI(uiB);
    uiZ = (signA != signB)
        ? (signB ? uiA : uiB)
        : (((uiA != uiB) && (signB ^ (uiB < uiA))) ? uiA : uiB);
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

static float32_t amo_min_f32(float32_t a, float32_t b)
{
    union ui32_f32 uA;
    uint_fast32_t uiA;
    bool signA;
    union ui32_f32 uB;
    uint_fast32_t uiB;
    bool signB;
    union ui32_f32 uZ;
    uint_fast32_t uiZ;

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
    if (isNaNF32UI(uiA) || isNaNF32UI(uiB)) {
        if (softfloat_isSigNaNF32UI(uiA) || softfloat_isSigNaNF32UI(uiB)) {
            uiZ = defaultNaNF32UI;
            goto uiZ;
        }
        uiZ = isNaNF32UI(uiA) ? (isNaNF32UI(uiB) ? defaultNaNF32UI : uiB) : uiA;
        goto uiZ;
    }
    signA = signF32UI( uiA );
    signB = signF32UI( uiB );
    uiZ = (signA != signB)
        ? (signA ? uiA : uiB)
        : (((uiA != uiB) && (signA ^ (uiA < uiB))) ? uiA : uiB );
 uiZ:
    uZ.ui = uiZ;
    return uZ.f;
}

void amo_emu_f(amoop op, freg dst, freg src1, xreg src2, mem_access_type macc)
{
    unsigned el = csr_gsc_progress[current_thread];
    bool dirty = false;
    try
    {
        while (el < VL)
        {
            iufval32 res, val1, val2;

            if (!MREGS[0][el]) continue;

            uint64_t addr = sextVA(XREGS[src2] + FREGS[src1].i32[el]);

            check_store_breakpoint(addr);
            if (!access_is_size_aligned(addr, 4))
            {
                throw trap_store_access_fault(addr);
            }
            uint64_t paddr = vmemtranslate(addr, macc);
            if (!pma_check_data_access(paddr, 4, macc))
            {
                throw trap_store_access_fault(addr);
            }
            val1.u = pmemread32(paddr);
            val2.u = FREGS[dst].u32[el];

            // Save the loaded data
            FREGS[dst].u32[el] = val1.u;
            LOG(DEBUG, "\t[%u] 0x%08" PRIx32 " <-- MEM32[0x%016" PRIx64 "]", el, val1.u, addr);
            dirty = true;

            switch (op)
            {
            case SWAP:
                res.u = val2.u;
                break;
            case AND:
                res.u = val1.u & val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- 0x%08" PRIx32 " & 0x%08" PRIx32 "", res.u, val1.u, val2.u);
                break;
            case OR:
                res.u = val1.u | val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- 0x%08" PRIx32 " | 0x%08" PRIx32 "", res.u, val1.u, val2.u);
                break;
            case XOR:
                res.u = val1.u ^ val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- 0x%08" PRIx32 " ^ 0x%08" PRIx32 "", res.u, val1.u, val2.u);
                break;
            case ADD:
                res.u = val1.i + val2.i;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- 0x%08" PRIx32 " + 0x%08" PRIx32 "", res.u, val1.u, val2.u);
                break;
            case MIN:
                res.u = (val1.i < val2.i) ? val1.u : val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- min(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            case MAX:
                res.u = (val1.i > val2.i) ? val1.u : val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- max(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            case MINU:
                res.u = (val1.u < val2.u) ? val1.u : val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- minu(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            case MAXU:
                res.u = (val1.u > val2.u) ? val1.u : val2.u;
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- maxu(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            case MINPS:
                res.f = amo_min_f32(val1.f, val2.f);
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- fmin(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            case MAXPS:
                res.f = amo_max_f32(val1.f, val2.f);
                LOG(DEBUG, "\t0x%08" PRIx32 " <-- fmax(0x%08" PRIx32 ", 0x%08" PRIx32 ")", res.u, val1.u, val2.u);
                break;
            default:
                res.u = 0;
                LOG(FTL, "Unknown atomic op %d", op);
            }

            // Stores the operated data
            LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", res.u, addr);
            pmemwrite32(paddr, res.u);

            // note: for logging purposes, sending val2.u instead of res.u => we want to check what the
            // dcache outputs to the shire caches, not the actual value written in memory
            log_mem_write(el, 4, addr, val2.u);

            ++el;
        }
    }
    catch (const trap_t&)
    {
        csr_gsc_progress[current_thread] = el;
        log_gsc_progress(el, false);
        if (dirty)
        {
            dirty_fp_state();
            log_freg_write(dst, FREGS[dst]);
        }
        throw;
    }
    csr_gsc_progress[current_thread] = 0;
    log_gsc_progress(0, true);
    if (dirty)
        dirty_fp_state();
    log_freg_write(dst, FREGS[dst]);
}

//
// Local Scalar 32 bits Atomics
//

AMO_EMU_W_FUNC(amoswapl_w, L, SWAP)
AMO_EMU_W_FUNC(amoandl_w,  L, AND)
AMO_EMU_W_FUNC(amoorl_w,   L, OR)
AMO_EMU_W_FUNC(amoxorl_w,  L, XOR)
AMO_EMU_W_FUNC(amoaddl_w,  L, ADD)
AMO_EMU_W_FUNC(amominl_w,  L, MIN)
AMO_EMU_W_FUNC(amomaxl_w,  L, MAX)
AMO_EMU_W_FUNC(amominul_w, L, MINU)
AMO_EMU_W_FUNC(amomaxul_w, L, MAXU)

//
// Global Scalar 32 bits Atomics
//

AMO_EMU_W_FUNC(amoswapg_w, G, SWAP)
AMO_EMU_W_FUNC(amoandg_w,  G, AND)
AMO_EMU_W_FUNC(amoorg_w,   G, OR)
AMO_EMU_W_FUNC(amoxorg_w,  G, XOR)
AMO_EMU_W_FUNC(amoaddg_w,  G, ADD)
AMO_EMU_W_FUNC(amoming_w,  G, MIN)
AMO_EMU_W_FUNC(amomaxg_w,  G, MAX)
AMO_EMU_W_FUNC(amominug_w, G, MINU)
AMO_EMU_W_FUNC(amomaxug_w, G, MAXU)

//
// Local Scalar 64 bits Atomics
//

AMO_EMU_D_FUNC(amoswapl_d, L, SWAP)
AMO_EMU_D_FUNC(amoandl_d,  L, AND)
AMO_EMU_D_FUNC(amoorl_d,   L, OR)
AMO_EMU_D_FUNC(amoxorl_d,  L, XOR)
AMO_EMU_D_FUNC(amoaddl_d,  L, ADD)
AMO_EMU_D_FUNC(amominl_d,  L, MIN)
AMO_EMU_D_FUNC(amomaxl_d,  L, MAX)
AMO_EMU_D_FUNC(amominul_d, L, MINU)
AMO_EMU_D_FUNC(amomaxul_d, L, MAXU)

//
// Global Scalar 64 bits Atomics
//

AMO_EMU_D_FUNC(amoswapg_d, G, SWAP)
AMO_EMU_D_FUNC(amoandg_d,  G, AND)
AMO_EMU_D_FUNC(amoorg_d,   G, OR)
AMO_EMU_D_FUNC(amoxorg_d,  G, XOR)
AMO_EMU_D_FUNC(amoaddg_d,  G, ADD)
AMO_EMU_D_FUNC(amoming_d,  G, MIN)
AMO_EMU_D_FUNC(amomaxg_d,  G, MAX)
AMO_EMU_D_FUNC(amominug_d, G, MINU)
AMO_EMU_D_FUNC(amomaxug_d, G, MAXU)

//
// Local Packed 32 bits Atomics
//

AMO_EMU_F_FUNC(famoswapl_pi, L, SWAP)
AMO_EMU_F_FUNC(famoandl_pi,  L, AND)
AMO_EMU_F_FUNC(famoorl_pi,   L, OR)
AMO_EMU_F_FUNC(famoxorl_pi,  L, XOR)
AMO_EMU_F_FUNC(famoaddl_pi,  L, ADD)
AMO_EMU_F_FUNC(famominl_pi,  L, MIN)
AMO_EMU_F_FUNC(famomaxl_pi,  L, MAX)
AMO_EMU_F_FUNC(famominul_pi, L, MINU)
AMO_EMU_F_FUNC(famomaxul_pi, L, MAXU)
AMO_EMU_F_FUNC(famominl_ps,  L, MINPS)
AMO_EMU_F_FUNC(famomaxl_ps,  L, MAXPS)

//
// Global Packed 32 bits Atomics
//

AMO_EMU_F_FUNC(famoswapg_pi, G, SWAP)
AMO_EMU_F_FUNC(famoandg_pi,  G, AND)
AMO_EMU_F_FUNC(famoorg_pi,   G, OR)
AMO_EMU_F_FUNC(famoxorg_pi,  G, XOR)
AMO_EMU_F_FUNC(famoaddg_pi,  G, ADD)
AMO_EMU_F_FUNC(famoming_pi,  G, MIN)
AMO_EMU_F_FUNC(famomaxg_pi,  G, MAX)
AMO_EMU_F_FUNC(famominug_pi, G, MINU)
AMO_EMU_F_FUNC(famomaxug_pi, G, MAXU)
AMO_EMU_F_FUNC(famoming_ps,  G, MINPS)
AMO_EMU_F_FUNC(famomaxg_ps,  G, MAXPS)


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
        // skip if masked
        if (tm && !tmask_pass(i))
            continue;

        LOG(DEBUG, "\tDoing %s: Set: %d, Way: %d, DestLevel: %d",
            evict ? "EvictSW" : "FlushSW", set, way, dest);

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
            paddr = vmemtranslate(vaddr, Mem_Access_CacheOp);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_CacheOp))
            {
                throw_access_fault(vaddr, Mem_Access_CacheOp);
            }
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
            paddr = vmemtranslate(vaddr, Mem_Access_Prefetch);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_Prefetch))
            {
                throw_access_fault(vaddr, Mem_Access_Prefetch);
            }
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
                // Requested line or requested way already locked; stop the operation
                LOG(DEBUG, "\tLockSW: 0x%016" PRIx64 ", Way: %d double-locking on way %d (addr: 0x%016" PRIx64 ")",
                    paddr, way, w, scp_trans[current_thread >> 1][set][w]);
                update_tensor_error(1 << 5);
                return;
            }
        }
    }
    // FIXME: We should check if PA exists, unlocked, in another set in the cache

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
    }
    LOG(DEBUG, "\tDoing LockSW: (%016" PRIx64 "), Way: %d, Set: %d", paddr, way, set);
}

static void dcache_unlock_set_way(int set, int way)
{
    if (set < L1D_NUM_SETS)
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
            paddr = vmemtranslate(vaddr, Mem_Access_Store);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_CacheOp))
            {
                throw_access_fault(vaddr, Mem_Access_CacheOp);
            }
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
            paddr = vmemtranslate(vaddr, Mem_Access_Store);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_CacheOp))
            {
                throw_access_fault(vaddr, Mem_Access_CacheOp);
            }
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
        LOG(DEBUG, "PORT_WRITE Port cache line (s%d w%d)  unlocked!\n", msg_ports[thread][id].scp_set, msg_ports[thread][id].scp_way);
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

        if (in_sysemu)
        {
            // if in sysemu stop thread if no data for port.. comparing rd_ptr and wr_ptr
            LOG(DEBUG, "Stalling MSG_PORT (m%d p%d)", current_thread, id);
            msg_ports[current_thread][id].stall = true;
            return 0;
        }
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

static bool txfma_off_allowed(uint16_t src1, uint64_t val)
{
    // if txfma is not sleep, allow
    if (csr_msleep_txfma_27[current_thread] == 0)
        return true;

    // and for each CSR, allow if corresponding bit in txfma_sleep_traps is 0
    // and do not allow if using the txfma
    uint32_t trap_conf = csr_mtxfma_sleep_traps[current_thread];
    switch (src1)
    {
        case CSR_TENSOR_FMA:
            if (((trap_conf >> 4) & 1) == 0) return true;
            return ((val & 0xE) == 6); // only allow for int8

        case CSR_TENSOR_QUANT:
            if ((( trap_conf >> 3) & 1) == 0) return true; //trap disabled
            return false;

        case CSR_TENSOR_REDUCE:
            if (((trap_conf >> 2) & 1) == 0) return true;
            // allow for int, do not allow for fp
            switch ((val >> 24) & 0xF)
            {
                case 0:  // fadd
                case 1:  // fsub
                case 2:  // fmax
                case 3:  // fmin
                case 8:  // fget
                    return false;
                default:
                    return true;
            }

        default:
            return true;
    }
}

// ----- TensorConvolution emulation -------------------------------------------

// Returns if there something that needs to be processed or not based on current position and configuration
static bool conv_skip_pass(int conv_row_pos, int conv_col_pos, int conv_row_size, int conv_col_size)
{
    LOG(DEBUG, "%s", "Doing Conv skip pass check for:");
    LOG(DEBUG, "\tRow Pos:  %d", conv_row_pos);
    LOG(DEBUG, "\tCol Pos:  %d", conv_col_pos);
    LOG(DEBUG, "\tRow Size: %d", conv_row_size);
    LOG(DEBUG, "\tCol Size: %d", conv_col_size);
    // Negative position
    bool skip = 0;
    if (conv_col_pos < 0) skip = 1;
    if (conv_row_pos < 0) skip = 1;
    // Outside position
    if (conv_col_pos >= int64_t(conv_col_size)) skip = 1;
    if (conv_row_pos >= int64_t(conv_row_size)) skip = 1;

    if (skip)
    {
        LOG(DEBUG, "\tSkip conv_row_pos %d conv_col_pos %d conv_row_size %d conv_col_size %d",
            conv_row_pos, conv_col_pos, conv_row_size, conv_col_size);
    }
    return skip;
}

// Update to the tensor Mask due a convolution CSR write
static void tmask_conv()
{
    uint16_t tmask_value = 0;

    // Gets the sizes of the convolution
    uint64_t tconvsizereg         = csr_tensor_conv_size[current_thread];
    int      conv_row_step_offset = (tconvsizereg & 0xFF00000000000000ULL) >> 56;
    int      conv_row_size        = (tconvsizereg & 0x0000FFFF00000000ULL) >> 32; // Convolution size in rows
    int      conv_col_step_offset = (tconvsizereg & 0x00000000FF000000ULL) >> 24;
    int      conv_col_size        = (tconvsizereg & 0x000000000000FFFFULL);       // Convolution size in cols

    // Gets the positions of the convolution
    uint64_t tconvctrlreg = csr_tensor_conv_ctrl[current_thread];
    int      conv_row_pos = (tconvctrlreg & 0x0000FFFF00000000ULL) >> 32; // Convolution pos in rows
    int      conv_col_pos = (tconvctrlreg & 0x000000000000FFFFULL);       // Convolution pos in cols

    // Sign extend
    if (conv_row_pos & 0x8000) conv_row_pos = conv_row_pos | 0xFFFFFFFFFFFF0000ULL;
    if (conv_col_pos & 0x8000) conv_col_pos = conv_col_pos | 0xFFFFFFFFFFFF0000ULL;

    // Goes through the 16 elements of the tensormap
    for (int i = 0; i < 16; i++)
    {
        // Sets a 1 if convolution passes
        if (!conv_skip_pass(conv_row_pos, conv_col_pos, conv_row_size, conv_col_size))
        {
            tmask_value |= (1 << i);
        }
        // Move the position of the convolution sampling based on the configuration register
        conv_row_pos += conv_row_step_offset;
        conv_col_pos += conv_col_step_offset;
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
        assert(shire != EMU_IO_SHIRE_SP);
        if (!esr_shire_coop_mode[shire])
            throw trap_illegal_instruction(current_inst);
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

    // Check if SCP is enabled
    if (csr_mcache_control[current_thread] != 0x3)
    {
        LOG(DEBUG, "%s", "Tensor_Error TensorLoad with SCP disabled!!");
        update_tensor_error(1 << 4);
        return;
    }

    log_tensor_load(trans, dst + adj, rows, tm ? csr_tensor_mask[current_thread] : 0);

    //NO TRANS
    if (trans == 0x00)
    {
        LOG(DEBUG, "%s", "TensorLoad: No transformation");
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                assert(access_is_size_aligned(addr, L1D_LINE_SIZE));
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                uint64_t paddr = vmemtranslate(addr, Mem_Access_TxLoad);
                if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_TxLoad))
                {
                    throw_access_fault(addr, Mem_Access_TxLoad);
                }
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
                    assert(access_is_size_aligned(vaddr, 16));
                    uint64_t paddr = vmemtranslate(vaddr, Mem_Access_TxLoad);
                    if (!pma_check_data_access(paddr, 16, Mem_Access_TxLoad))
                    {
                        throw_access_fault(vaddr, Mem_Access_TxLoad);
                    }
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
        boffset *= 32;
        LOG(DEBUG, "#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/32);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                for (int r = 0; r < 2; ++r)
                {
                    uint64_t vaddr = sextVA(addr + boffset + (2*i+r)*stride);
                    assert(access_is_size_aligned(vaddr, 32));
                    uint64_t paddr = vmemtranslate(vaddr, Mem_Access_TxLoad);
                    if (!pma_check_data_access(paddr, 32, Mem_Access_TxLoad))
                    {
                        throw_access_fault(vaddr, Mem_Access_TxLoad);
                    }
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
        int offset = (size==1) ? (control & 0x30) : (control & 0x20) ;
        int elements = L1D_LINE_SIZE >> (size-1);
        size = 1 << (size-1);
        LOG(DEBUG, "TensorLoad: Transpose - elements:%d size:%d offset:%d", elements, size, offset);
        for (int elem = 0; elem < elements; ++elem)
        {
            //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
            assert(access_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, Mem_Access_TxLoad);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_TxLoad))
            {
                throw_access_fault(addr, Mem_Access_TxLoad);
            }
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
    assert(shire != EMU_IO_SHIRE_SP);

    for (int i = 0; i < rows; ++i)
    {
        uint64_t l2scp_addr = L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * L1D_LINE_SIZE);
        if (!tm || tmask_pass(i))
        {
            assert(access_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, Mem_Access_TxLoad);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_TxLoad))
            {
                throw_access_fault(addr, Mem_Access_TxLoad);
            }
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
    if (!txfma_off_allowed(CSR_TENSOR_QUANT, value))
        throw trap_txfma_off(current_inst);

    unsigned fstart = (value >> 57) & 0x1F;
    unsigned cols   = (value >> 55) & 0x3;
    unsigned rows   = (value >> 51) & 0xF;
    unsigned line   = (value >> 45) & 0x3F;

    cols = (cols + 1) * 4;
    rows = rows + 1;
    line = line % L1_SCP_ENTRIES;

    set_rounding_mode(rmdyn);

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
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.f = fpu::i32_to_f32(val.i);
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x (%g) <-- 0x%08x (%d)", freg, j%VL, res.u, res.flt, val.u, val.i);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                break;
            case 2: // FP32_TO_INT32
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = fpu::f32_to_i32(val.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x (%d) <-- 0x%08x (%g)", freg, j%VL, res.u, res.i, val.u, val.flt);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                break;
            case 3: // INT32_RELU
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i < 0) ? 0 : val.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x <-- MAX_INT32(0x%08x, 0x0)", freg, j%VL, res.i, val.i);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                break;
            case 4: // INT32_ADD_ROW
                if (csr_mcache_control[current_thread] != 0x3)
                {
                    update_tensor_error(1 << 4);
                    return;
                }
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, val2, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        val2.u = SCP[line].u32[j];
                        res.i = val1.i + val2.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x <-- 0x%08x + 0x%08x", freg, j%VL, res.u, val1.u, val2.u);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
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
                for (unsigned i = 0; i < rows; ++i)
                {
                    iufval32 val2;
                    val2.u = SCP[line].u32[i];
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        res.i = val1.i + val2.i;
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x <-- 0x%08x + 0x%08x", freg, j%VL, res.u, val1.u, val2.u);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
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
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, val2, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        val2.u = SCP[line].u32[j];
                        res.f = fpu::f32_mul(val1.f, val2.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g)", freg, j%VL, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
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
                for (unsigned i = 0; i < rows; ++i)
                {
                    iufval32 val2;
                    val2.u = SCP[line].u32[i];
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val1, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val1.u = FREGS[freg].u32[j%VL];
                        res.f = fpu::f32_mul(val1.f, val2.f);
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g)", freg, j%VL, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                line = (line + 1) % L1_SCP_ENTRIES;
                break;
            case 8: // SATINT8
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i > 127 ? 127 : (val.i < -128 ? -128 : val.i)) & 0xFF;
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x <-- 0x%08x", freg, j%VL, res.u, val.u);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                break;
            case 9: // SATUINT8
                log_tensor_quant_new_transform();
                for (unsigned i = 0; i < rows; ++i)
                {
                    for (unsigned j = 0; j < cols; ++j)
                    {
                        iufval32 val, res;
                        unsigned freg = (fstart + i*2 + j/VL) % 32;
                        val.u = FREGS[freg].u32[j%VL];
                        res.i = (val.i > 255 ? 255 : (val.i < 0 ? 0 : val.i)) & 0xFF;
                        FREGS[freg].u32[j%VL] = res.u;
                        LOG(DEBUG, "\tf%u[%zu] 0x%08x <-- 0x%08x", freg, j%VL, res.u, val.u);
                        log_tensor_quant_write(k, freg, j%VL, res.u);
                    }
                }
                break;
            case 10: // PACK_128B
                // RTL operates on even registers first, and then on odd
                // registers, so it generates two writes to the destination
                // register when a row spans a vector.
                log_tensor_quant_new_transform(cols >= VL);
                for (unsigned i = 0; i < rows; ++i)
                {
                    unsigned fdst = (fstart + i*2) % 32;
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
                        LOG(DEBUG, "\tf%u[%u] 0x%08x <-- {0x%08x, 0x%08x, 0x%08x, 0x%08x}", fdst, j/4, FREGS[fdst].u32[j/4], val0, val1, val2, val3);
                        log_tensor_quant_write(k, fdst, j/4, FREGS[fdst].u32[j/4]);
                    }
                }
                break;
            default:
                assert(0);
                break;
        }
    }
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
        uint64_t addr     = sext<48>(tstorereg & 0x00FFFFFFFFFFC0ULL);       // Address where to store the results

        uint64_t stride   = XREGS[31] & 0xFFFFFFFFFFFFULL;

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
            assert(access_is_size_aligned(addr, L1D_LINE_SIZE));
            uint64_t paddr = vmemtranslate(addr, Mem_Access_TxStore);
            if (!pma_check_data_access(paddr, L1D_LINE_SIZE, Mem_Access_TxStore))
            {
                throw_access_fault(addr, Mem_Access_TxStore);
            }
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

        uint64_t stride   = XREGS[31] & 0xFFFFFFFFFFF0ULL;

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
            assert(shire != EMU_IO_SHIRE_SP);
            if (!esr_shire_coop_mode[shire])
                throw trap_illegal_instruction(current_inst);
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            // For all the blocks of 128b
            for (int col = 0; col < cols; col++)
            {
                assert(access_is_size_aligned(addr, 16));
                uint64_t paddr = vmemtranslate(addr + col*16, Mem_Access_TxStore);
                if (!pma_check_data_access(paddr, 16, Mem_Access_TxStore))
                {
                    throw_access_fault(addr, Mem_Access_TxStore);
                }
                // For all the 32 elements of the 128b block
                for (uint64_t i = 0; i < 4; i++)
                {
                    uint32_t idx = (col & 1) * 4 + i;
                    uint32_t val = FREGS[src].u32[idx];
                    LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM32[0x%016" PRIx64 "]", val, addr + col*16 + i*4);
                    pmemwrite32(paddr + i*4, val);
                    //log_mem_write(0, 4, addr + col*16 + i*4, val); => Don't log mem changes!
                }
                if (cols == 1)    src += srcinc; // For 128b stores, move to next desired register
                else if (col & 1) src += srcinc; // For 256b and 512b stores, move to next desired register when 256b are written
            }
            addr = sextVA(addr + stride);
        }
    }
}

// ----- TensorFMA emulation ---------------------------------------------------

static void tensor_fma32(uint64_t tfmareg)
{
    if (!txfma_off_allowed(CSR_TENSOR_FMA, tfmareg))
        throw trap_txfma_off(current_inst);

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
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(rmdyn));

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

    set_rounding_mode(rmdyn);

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
    if (!txfma_off_allowed(CSR_TENSOR_FMA, tfmareg))
        throw trap_txfma_off(current_inst);

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
    if (!txfma_off_allowed(CSR_TENSOR_FMA, tfmareg))
        throw trap_txfma_off(current_inst);

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
                (SCP[(astart+i) % L1_SCP_ENTRIES].u32[(aoffset+(k/4)) % (L1D_LINE_SIZE/4)] == 0))
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
    uint64_t other_min;
    uint64_t action;

    if (!txfma_off_allowed(CSR_TENSOR_REDUCE, value))
        throw trap_txfma_off(current_inst);

    tensor_reduce_decode(value, &other_min, &action);

    // Do nothing
    if (action == 2) return;
    // Send
    if (action == 0) return;
    // Receive

    //op = rs[35:32]
    int      start_reg = (value >> 57) & 0x1F;
    uint32_t operation = (value >> 24) & 0xF;
    int      num_reg   = (value >> 16) & 0xFF;

    // Sending and receiving from the same minion
    if (other_min == (current_thread>>1))
    {
        update_tensor_error(1 << 9);
        return;
    }

    // Info for checker
    log_tensor_reduce(start_reg, num_reg);

    if (operation == 0) // FADD
    {
        set_rounding_mode(rmdyn);
        LOG(DEBUG, "\tReduce (fadd) with rounding mode: %s", get_rounding_mode(rmdyn));
    }
    for (int i = 0; i < num_reg; i++)
    {
        for (unsigned j = 0; j < VL; j++)
        {
            int op_reg = (i + start_reg) % 32;
            if (operation == 0) // FADD
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u32[j];
                src2.u = fregs[other_min<<1][op_reg].u32[j];
                rslt.f = fpu::f32_add(src1.f, src2.f);
                FREGS[op_reg].u32[j] = rslt.u;
                LOG(DEBUG, "\tReduce (fadd) f%d[%u]: 0x%08x = 0x%08x + 0x%08x (m%" PRId64 ")",op_reg,j,rslt.u,src1.u,src2.u,other_min);
            }
            else if (operation == 2) // FMAX
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u32[j];
                src2.u = fregs[other_min<<1][op_reg].u32[j];
                rslt.f = fpu::f32_maximumNumber(src1.f,src2.f);
                FREGS[op_reg].u32[j] = rslt.u;
                LOG(DEBUG, "\tReduce (fmax) f%d[%u]: 0x%08x = 0x%08x > 0x%08x (m%" PRId64 ")",op_reg,j,rslt.u,src1.u,src2.u,other_min);
            }
            else if (operation == 4) // IADD
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u32[j];
                src2.u = fregs[other_min<<1][op_reg].u32[j];
                rslt.u = src1.u + src2.u;
                FREGS[op_reg].u32[j] = rslt.u;
                LOG(DEBUG, "\tReduce (iadd) f%d[%u]: 0x%08x = 0x%08x + 0x%08x (m%" PRId64 ")",op_reg,j,rslt.u,src1.u,src2.u,other_min);
            }
            else if (operation == 8) // FGET
            {
                iufval32 tmp;
                tmp.u = fregs[other_min<<1][op_reg].u32[j];
                FREGS[op_reg].u32[j] = tmp.u;
                LOG(DEBUG, "\tReduce (get) f%d[%u]: <= 0x%08x (m%" PRId64 ")",op_reg,j,tmp.u,other_min);
            }
            else
            {
                LOG(ERR, "Reduce/broadcast operation = %d not yet supported in emu", operation);
            }
            log_tensor_reduce_write(op_reg, j, FREGS[op_reg].u32[j]);
        }
    }
    set_fp_exceptions();
    dirty_fp_state();
}

// Helper function that given the written value to the CSR, returns:
//   - what is the ID of the other minion of the reduce
//   - what is the action taken by the minion (send, receive, do nothing)
void tensor_reduce_decode(uint64_t value, uint64_t* other_min, uint64_t* action)
{
    uint64_t level = (value >> 3) & 0xF;
    uint64_t type  = value & 3;
    uint64_t minion_id = current_thread >> 1;

    // SENDER
    if (type == 0)
    {
        * action = 1;
        * other_min = (value >> 3) & 0x1FFF;
    }
    // RECEIVER
    else if (type == 1)
    {
        * action = 0;
        * other_min = (value >> 3) & 0x1FFF;
    }
    // BROADCAST: Compute sender/receiver assuming recursive halving
    else if (type == 2)
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if ((minion_id & minion_mask) == distance)
        {
            * action    = 1; // sender
            * other_min = minion_id - distance;
        }
        else if ((minion_id & minion_mask) == 0)
        {
            * action    = 0; // receiver
            * other_min = minion_id + distance;
        }
        else
        {
            * action    = 2; // do nothing
        }
    }
    // REDUCE: Compute sender/receiver assuming recursive halving
    else
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if ((minion_id & minion_mask) == distance)
        {
            * action    = 0; // sender
            * other_min = minion_id - distance;
        }
        else if ((minion_id & minion_mask) == 0)
        {
            * action    = 1; // receiver
            * other_min = minion_id + distance;
        }
        else
        {
            * action    = 2; // do nothing
        }
    }
}

// ----- Shire cooperative mode ------------------------------------------------

void write_shire_coop_mode(unsigned shire, uint64_t val)
{
    assert(shire <= EMU_MASTER_SHIRE);
    esr_shire_coop_mode[shire] = !!(val & 1);
#ifndef SYS_EMU
    if (!esr_shire_coop_mode[shire])
        esr_icache_prefetch_active[shire] = false;
#endif
}

uint64_t read_shire_coop_mode(unsigned shire)
{
    assert(shire <= EMU_MASTER_SHIRE);
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
    uint64_t addr = ESR_SHIRE(CSR_PRV_U, shire, FLB0) + (barrier * 8); // Access is private per cache

    // NB: No PMA checks here... we know it will pass ;-)

    uint64_t orig_value = pmemread64(addr);
    uint64_t result = -1;

    // LOG_ALL_MINIONS(DEBUG,"FastLocalBarrier: Shire %i: Minion %i Thread %i doing barrier %" PRIu64 " value  %" PRIu64 ", limit %" PRIu64 " ",
    //     (int) shire, current_thread / EMU_THREADS_PER_MINION, current_thread % EMU_THREADS_PER_MINION, barrier, orig_value, limit);
    LOG(DEBUG,"FastLocalBarrier: Shire %i: Minion %i Thread %i doing barrier %" PRIu64 " value  %" PRIu64 ", limit %" PRIu64 " ",
         (int) shire, current_thread / EMU_THREADS_PER_MINION, current_thread % EMU_THREADS_PER_MINION, barrier, orig_value, limit);
    // Last guy, return 1 and zero barrier
    if (orig_value == limit)
    {
        //LOG_ALL_MINIONS(DEBUG,"FastLocalBarrier: last minion Shire %i!!", (int) shire);
        LOG(DEBUG,"FastLocalBarrier: last minion Shire %i!!", (int) shire);

        pmemwrite64(addr, 0);
        result = 1;
    }
    // Not the last guy, return 0 and increment barrier
    else
    {
        //LOG_ALL_MINIONS(DEBUG, "FastLocalBarrier: Limit %" PRIu64", Incrementing to %" PRIu64 "!!", limit, orig_value + 1);
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
    LOG(DEBUG,"fcc_inc(%" PRIu64 ", %" PRIu64 ", %" PRIx64 ", %" PRIu64 ")",
        thread, shire, minion_mask, fcc_id);

    for (int minion = 0; minion < EMU_MINIONS_PER_SHIRE; ++minion)
    {
        if (minion_mask & (1 << minion))
        {
            size_t fcc_addr = shire*EMU_THREADS_PER_SHIRE + EMU_THREADS_PER_MINION*minion + thread;
            LOG(DEBUG, "Incrementing FCC[ %" PRIu64 "][%" PRIu64 "]=%" PRId32, fcc_addr, fcc_id, fcc[fcc_addr][fcc_id] + 1);
            fcc[fcc_addr][fcc_id] ++;

            // wake up waiting threads (only for checker, not sysemu)
            if (!in_sysemu && fcc_wait[fcc_addr]){
                fcc_wait[fcc_addr] = false;
                minions_to_awake.push(fcc_addr>>1);
            }

            //check for overflow
            if (fcc[fcc_addr][fcc_id] == 0x000) {
                update_tensor_error(1 << 3);
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
    csr_mip[thread] |= 1<<cause;
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

void raise_external_interrupt(int thread)
{
    csr_mip[thread] |= 0x800;
}

void clear_external_interrupt(int thread)
{
    csr_mip[thread] &= ~0x800;
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
