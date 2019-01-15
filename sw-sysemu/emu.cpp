/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>       // FIXME: Remove this, use "emu_gio.h" instead
#include <cassert>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <list>
#include <queue>
#include <unordered_map>

#include "emu.h"
#include "log.h"
#include "emu_casts.h"
#include "emu_gio.h"
#include "emu_memop.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"

#include <cfenv>       // FIXME: remove this when we purge std::fesetround() from the code!
#include <emmintrin.h> // FIXME: remove this when we fix the TXFMA code

// L1 Dcache configuration, used by the cache management operations
#define L1D_NUM_SETS  16
#define L1D_NUM_WAYS  4
#define L1D_LINE_SIZE 64

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

// Scratchpad defines
#define L1_ENTRIES        (L1D_NUM_SETS * L1D_NUM_WAYS)
#define L1_SCP_ENTRIES    48
#define L1_SCP_LINE_SIZE  (L1D_LINE_SIZE)
#define L1_SCP_BLOCKS     (L1_SCP_LINE_SIZE / (VL * 4))
#define L1_SCP_BLOCK_SIZE (VL * 4)

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
    bool     is_tbox;
    bool     is_rbox;
    uint8_t  data[(1 << PORT_LOG2_MAX_SIZE)];
    uint8_t  oob;
} msg_port_write_t;

typedef enum {
    // PS memory instructions
    FLW,
    FSW,
    FSWB,
    FSWH,
    FBC,
    FBCI,
    FGW,
    FGH,
    FGB,
    FSCW,
    FSCH,
    FSCB,
    FG32W,
    FG32H,
    FG32B,
    FSC32W,
    FSC32H,
    FSC32B,
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
    // PS computation instructions
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FMIN,
    FMAX,
    FMADD,
    FMSUB,
    FNMADD,
    FNMSUB,
    // PS 1-source
    FSQRT,
    FRSQ,
    FSIN,
    //FCOS,
    FEXP,
    FLOG,
    FRCP,
    FRCPFXP,
    FCVTPSPW,
    FCVTPSPWU,
    FFRC,
    FROUND,
    FSWIZZ,
    FCMOV, // PS conversion and move
    FCVTPWPS,
    FCVTPWUPS,
    FSGNJ,
    FSGNJN,
    FSGNJX,
    FMVZXPS,  // warning: unimplemented
    FMVSXPS,  // warning: unimplemented
    FEQ, // Floating point compare
    FLE,
    FLT,
    //FLTABS,
    CUBEFACE,
    CUBEFACEIDX,
    CUBESGNSC,
    CUBESGNTC,
    FCLASS,
    FCLASSPS,
    FCVTPSF16, // Graphics Upconvert to PS
    FCVTPSF11,
    FCVTPSF10,
    FCVTPSUN24,
    FCVTPSUN16,
    FCVTPSUN10,
    FCVTPSUN8,
    FCVTPSUN2,
    FCVTPSRAST,
    FCVTRASTPS,
    //FCVTPSSN24,
    FCVTPSSN16,
    //FCVTPSSN10,
    FCVTPSSN8,
    //FCVTPSSN2,
    FCVTF16PS, // Graphics DownConvert from PS
    FCVTF11PS,
    FCVTF10PS,
    FCVTUN24PS,
    FCVTUN16PS,
    FCVTUN10PS,
    FCVTUN8PS,
    FCVTUN2PS,
    //FCVTSN24PS,
    FCVTSN16PS,
    FCVTSN8PS,
    MAND, // Mask operations
    MOR,
    MXOR,
    MNOT,
    FSET,
    MOVAMX,
    MOVAXM,
    FADDPI, // Packed Integer extension
    FSUBPI,
    FMULPI,
    FMULHPI,
    FMULHUPI,
    //FMULHSUPI,
    FDIVPI,
    FDIVUPI,
    FREMPI,
    FREMUPI,
    FMINPI,
    FMAXPI,
    FMINUPI,
    FMAXUPI,
    FANDPI,
    FORPI,
    FXORPI,
    FNOTPI,
    FSAT8PI,
    FSATU8PI,
    FSLLPI,
    FSRLPI,
    FSRAPI,
    FLTPI,
    FLTUPI,
    FLEPI,
    FEQPI,
    FRCP_FIX_RAST,
    FADDIPI, // Packed Integer with Immediate
    FANDIPI,
    FORIPI,
    FXORIPI,
    FSLLIPI,
    FSRLIPI,
    FSRAIPI,
    FPACKREPBPI,
    FPACKREPHPI,
    FCVTWS, // integer opcodes that should not really be here :-)
    FCVTWUS,
    FCVTLS,
    FCVTLUS,
    FCVTSW,
    FCVTSWU,
    FCVTSL,
    FCVTSLU,
    SIMPLE_INT, // Integer ISA
    MUL_INT,
    DIV_INT,
    REM_INT,
    MASKOP,     // Mask ops
    LD,
    STORE_INT,
    // Please keep me last at all times!
    MAXOPCODE
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


// Neede by fence.i
extern void flush_insn_cache();

// State declaration
std::ostringstream uart_stream[EMU_NUM_THREADS];
int buflen = 0;
xdata xregs[EMU_NUM_THREADS][32];
fdata fregs[EMU_NUM_THREADS][32];
mdata mregs[EMU_NUM_THREADS][8];
uint64_t csrregs[EMU_NUM_THREADS][CSR_MAX];
bool mtvec_is_set[EMU_NUM_THREADS] = {};
bool stvec_is_set[EMU_NUM_THREADS] = {};
bool tensorload_setupb_topair[EMU_NUM_THREADS] = {false};
int tensorload_setupb_numlines[EMU_NUM_THREADS];
fdata scp[EMU_NUM_THREADS][L1_SCP_ENTRIES+TFMA_MAX_AROWS][L1_SCP_BLOCKS];
int scp_entry[EMU_NUM_THREADS];
int scp_size[EMU_NUM_THREADS];
bool scp_tm;
int tensorfma_size[EMU_NUM_THREADS];
int tensorfma_passes[EMU_NUM_THREADS];
fdata tensorfma_tenc[EMU_NUM_THREADS][32];
uint32_t tensorfma_data[EMU_NUM_THREADS][32][VL][TFMA_MAX_ACOLS];
bool tensorfma_mask_skip[TFMA_MAX_ACOLS][TFMA_MAX_AROWS];
bool tensorfma_zero_skip[TFMA_MAX_ACOLS][32][VL];
int tensorquant_size[EMU_NUM_THREADS];
int tensorquant_trans[EMU_NUM_THREADS];
bool tensorquant_is_pack[EMU_NUM_THREADS][TQUANT_MAX_TRANS];
uint32_t tensorquant_data[EMU_NUM_THREADS][32][VL][TQUANT_MAX_TRANS];
int reduce_entry[EMU_NUM_THREADS];
int reduce_size[EMU_NUM_THREADS];
uint32_t reduce_data[EMU_NUM_THREADS][32][VL];
msg_port_conf_t msg_ports[EMU_NUM_THREADS][NR_MSG_PORTS];
std::queue<uint8_t> msg_ports_oob[EMU_NUM_THREADS][NR_MSG_PORTS];
bool msg_port_delayed_write = false;
std::vector<msg_port_write_t> msg_port_pending_writes     [EMU_NUM_SHIRES];
std::vector<msg_port_write_t> msg_port_pending_writes_tbox[EMU_NUM_SHIRES];
std::vector<msg_port_write_t> msg_port_pending_writes_rbox[EMU_NUM_SHIRES];

uint64_t fcc_cnt;
uint16_t fcc[EMU_NUM_THREADS][2] ={{0}};

std::unordered_map<int, char const*> csr_names = {
   { csr_prv,               "prv"                },
   { csr_unknown,           "unknown"            },
   { csr_fflags,            "fflags"             },
   { csr_frm,               "frm"                },
   { csr_fcsr,              "fcsr"               },
   { csr_cycle,             "cycle"              },
   { csr_time,              "time"               },
   { csr_instret,           "instret"            },
   { csr_hpmcounter3,       "hpmcounter3"        },
   { csr_hpmcounter4,       "hpmcounter4"        },
   { csr_hpmcounter5,       "hpmcounter5"        },
   { csr_hpmcounter6,       "hpmcounter6"        },
   { csr_hpmcounter7,       "hpmcounter7"        },
   { csr_hpmcounter8,       "hpmcounter8"        },
   { csr_hpmcounter9,       "hpmcounter9"        },
   { csr_hpmcounter10,      "hpmcounter10"       },
   { csr_hpmcounter11,      "hpmcounter11"       },
   { csr_hpmcounter12,      "hpmcounter12"       },
   { csr_hpmcounter13,      "hpmcounter13"       },
   { csr_hpmcounter14,      "hpmcounter14"       },
   { csr_hpmcounter15,      "hpmcounter15"       },
   { csr_hpmcounter16,      "hpmcounter16"       },
   { csr_hpmcounter17,      "hpmcounter17"       },
   { csr_hpmcounter18,      "hpmcounter18"       },
   { csr_hpmcounter19,      "hpmcounter19"       },
   { csr_hpmcounter20,      "hpmcounter20"       },
   { csr_hpmcounter21,      "hpmcounter21"       },
   { csr_hpmcounter22,      "hpmcounter22"       },
   { csr_hpmcounter23,      "hpmcounter23"       },
   { csr_hpmcounter24,      "hpmcounter24"       },
   { csr_hpmcounter25,      "hpmcounter25"       },
   { csr_hpmcounter26,      "hpmcounter26"       },
   { csr_hpmcounter27,      "hpmcounter27"       },
   { csr_hpmcounter28,      "hpmcounter28"       },
   { csr_hpmcounter29,      "hpmcounter29"       },
   { csr_hpmcounter30,      "hpmcounter30"       },
   { csr_hpmcounter31,      "hpmcounter31"       },
   { csr_tensor_reduce,     "tensor_reduce"      },
   { csr_tensor_fma,        "tensor_fma"         },
   { csr_tensor_conv_size,  "tensor_conv_size"   },
   { csr_tensor_conv_ctrl,  "tensor_conv_ctrl"   },
   { csr_tensor_coop,       "tensor_coop"        },
   { csr_tensor_mask,       "tensor_mask"        },
   { csr_tensor_quant,      "tensor_quant"       },
   { csr_tex_send,          "tex_send"           },
   { csr_tensor_error,      "tensor_error"       },
   { csr_scratchpad_ctrl,   "scratchpad_ctrl"    },
   { csr_usr_cache_op,      "usr_cache_op"       },
   { csr_prefetch_va,       "prefetch_va"        },
   { csr_flb0,              "flb0"               },
   { csr_fcc,               "fcc"                },
   { csr_stall,             "stall"              },
   { csr_tensor_wait,       "tensor_wait"        },
   { csr_tensor_load,       "tensor_load"        },
   { csr_tensor_load_l2,    "tensor_load_l2"     },
   { csr_tensor_store,      "tensor_store"       },
   { csr_evict_va,          "evict_va"           },
   { csr_flush_va,          "flush_va"           },
   { csr_umsg_port0,        "umsg_port0"         },
   { csr_umsg_port1,        "umsg_port1"         },
   { csr_umsg_port2,        "umsg_port2"         },
   { csr_umsg_port3,        "umsg_port3"         },
   { csr_validation0,       "validation0"        },
   { csr_validation1,       "validation1"        },
   { csr_validation2,       "validation2"        },
   { csr_validation3,       "validation3"        },
   { csr_sleep_txfma_27,    "sleep_txfma_27"     },
   { csr_lock_va,           "lock_va"            },
   { csr_unlock_va,         "unlock_va"          },
   { csr_lock_sw,           "lock_sw"            },
   { csr_unlock_sw,         "unlock_sw"          },
   { csr_porthead0,         "porthead0"          },
   { csr_porthead1,         "porthead1"          },
   { csr_porthead2,         "porthead2"          },
   { csr_porthead3,         "porthead3"          },
   { csr_portheadnb0,       "portheadnb0"        },
   { csr_portheadnb1,       "portheadnb1"        },
   { csr_portheadnb2,       "portheadnb2"        },
   { csr_portheadnb3,       "portheadnb3"        },
   { csr_hartid,            "hartid"             },
   { csr_sstatus,           "sstatus"            },
   { csr_sie,               "sie"                },
   { csr_stvec,             "stvec"              },
   { csr_scounteren,        "scounteren"         },
   { csr_sscratch,          "sscratch"           },
   { csr_sepc,              "sepc"               },
   { csr_scause,            "scause"             },
   { csr_stval,             "stval"              },
   { csr_sip,               "sip"                },
   { csr_satp,              "satp"               },
   { csr_sys_cache_op,      "sys_cache_op"       },
   { csr_mcache_control,    "mcache_control"     },
   { csr_evict_sw,          "evict_sw"           },
   { csr_flush_sw,          "flush_sw"           },
   { csr_smsg_port0,        "smsg_port0"         },
   { csr_smsg_port1,        "smsg_port1"         },
   { csr_smsg_port2,        "smsg_port2"         },
   { csr_smsg_port3,        "smsg_port3"         },
   { csr_portctrl0,         "portctrl0"          },
   { csr_portctrl1,         "portctrl1"          },
   { csr_portctrl2,         "portctrl2"          },
   { csr_portctrl3,         "portctrl3"          },
   { csr_mvendorid,         "mvendorid"          },
   { csr_marchid,           "marchid"            },
   { csr_mimpid,            "mimpid"             },
   { csr_mhartid,           "mhartid"            },
   { csr_mstatus,           "mstatus"            },
   { csr_misa,              "misa"               },
   { csr_medeleg,           "medeleg"            },
   { csr_mideleg,           "mideleg"            },
   { csr_mie,               "mie"                },
   { csr_mtvec,             "mtvec"              },
   { csr_mcounteren,        "mcounteren"         },
   { csr_mscratch,          "mscratch"           },
   { csr_mepc,              "mepc"               },
   { csr_mcause,            "mcause"             },
   { csr_mtval,             "mtval"              },
   { csr_mip,               "mip"                },
   { csr_mcycle,            "mcycle"             },
   { csr_minstret,          "minstret"           },
   { csr_mhpmcounter3,      "mhpmcounter3"       },
   { csr_mhpmcounter4,      "mhpmcounter4"       },
   { csr_mhpmcounter5,      "mhpmcounter5"       },
   { csr_mhpmcounter6,      "mhpmcounter6"       },
   { csr_mhpmcounter7,      "mhpmcounter7"       },
   { csr_mhpmcounter8,      "mhpmcounter8"       },
   { csr_mhpmcounter9,      "mhpmcounter9"       },
   { csr_mhpmcounter10,     "mhpmcounter10"      },
   { csr_mhpmcounter11,     "mhpmcounter11"      },
   { csr_mhpmcounter12,     "mhpmcounter12"      },
   { csr_mhpmcounter13,     "mhpmcounter13"      },
   { csr_mhpmcounter14,     "mhpmcounter14"      },
   { csr_mhpmcounter15,     "mhpmcounter15"      },
   { csr_mhpmcounter16,     "mhpmcounter16"      },
   { csr_mhpmcounter17,     "mhpmcounter17"      },
   { csr_mhpmcounter18,     "mhpmcounter18"      },
   { csr_mhpmcounter19,     "mhpmcounter19"      },
   { csr_mhpmcounter20,     "mhpmcounter20"      },
   { csr_mhpmcounter21,     "mhpmcounter21"      },
   { csr_mhpmcounter22,     "mhpmcounter22"      },
   { csr_mhpmcounter23,     "mhpmcounter23"      },
   { csr_mhpmcounter24,     "mhpmcounter24"      },
   { csr_mhpmcounter25,     "mhpmcounter25"      },
   { csr_mhpmcounter26,     "mhpmcounter26"      },
   { csr_mhpmcounter27,     "mhpmcounter27"      },
   { csr_mhpmcounter28,     "mhpmcounter28"      },
   { csr_mhpmcounter29,     "mhpmcounter29"      },
   { csr_mhpmcounter30,     "mhpmcounter30"      },
   { csr_mhpmcounter31,     "mhpmcounter31"      },
   { csr_mhpmevent3,        "mhpmevent3"         },
   { csr_mhpmevent4,        "mhpmevent4"         },
   { csr_mhpmevent5,        "mhpmevent5"         },
   { csr_mhpmevent6,        "mhpmevent6"         },
   { csr_mhpmevent7,        "mhpmevent7"         },
   { csr_mhpmevent8,        "mhpmevent8"         },
   { csr_mhpmevent9,        "mhpmevent9"         },
   { csr_mhpmevent10,       "mhpmevent10"        },
   { csr_mhpmevent11,       "mhpmevent11"        },
   { csr_mhpmevent12,       "mhpmevent12"        },
   { csr_mhpmevent13,       "mhpmevent13"        },
   { csr_mhpmevent14,       "mhpmevent14"        },
   { csr_mhpmevent15,       "mhpmevent15"        },
   { csr_mhpmevent16,       "mhpmevent16"        },
   { csr_mhpmevent17,       "mhpmevent17"        },
   { csr_mhpmevent18,       "mhpmevent18"        },
   { csr_mhpmevent19,       "mhpmevent19"        },
   { csr_mhpmevent20,       "mhpmevent20"        },
   { csr_mhpmevent21,       "mhpmevent21"        },
   { csr_mhpmevent22,       "mhpmevent22"        },
   { csr_mhpmevent23,       "mhpmevent23"        },
   { csr_mhpmevent24,       "mhpmevent24"        },
   { csr_mhpmevent25,       "mhpmevent25"        },
   { csr_mhpmevent26,       "mhpmevent26"        },
   { csr_mhpmevent27,       "mhpmevent27"        },
   { csr_mhpmevent28,       "mhpmevent28"        },
   { csr_mhpmevent29,       "mhpmevent29"        },
   { csr_mhpmevent30,       "mhpmevent30"        },
   { csr_mhpmevent31,       "mhpmevent31"        },
   { csr_minstmask,         "minstmask"          },
   { csr_minstmatch,        "minstmatch"         },
   { csr_flush_icache,      "flush_icache"       },
   { csr_msleep_txfma_27,   "msleep_txfma_27"    },
   { csr_menable_shadows,   "menable_shadows"    },
   { csr_excl_mode,         "excl_mode"          },
   { csr_mtxfma_sleep_traps,"mtxfma_sleep_traps" }
};

// Used to access different threads transparently
#define SCP   scp[current_thread]

static et_core_t core_type = ET_MINION;
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

std::stringstream dump_xregs(uint32_t thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (int ii = 0; ii < 32; ++ii)
      {
         str << "XREG[" << std::dec << ii << "] = 0x" << std::hex << xregs[thread_id][ii].x << "\n";
      }
   }
   return str;
}

std::stringstream dump_fregs(uint32_t thread_id)
{
   std::stringstream str;
   if (thread_id < EMU_NUM_THREADS)
   {
      for (int ii = 0; ii < 32; ++ii)
      {
         for (int jj = 0; jj < VL; ++jj)
         {
            str << "FREG[" << std::dec << ii << "][" << jj <<  "] = 0x" << std::hex << fregs[thread_id][ii].u[jj] << "\t";
         }
         str << "\n";
      }
   }
   return str;
}

void init_emu(enum logLevel level)
{
   XREGS[x0].x  = 0;
   emu_log().setLogLevel(level);
   // FIXME: remove '#include <cfenv>' when we purge this function from the code
   std::fesetround(FE_TONEAREST); // set rne for host
}

void log_only_minion(int32_t m) {
    extern int32_t minion_only_log;
    minion_only_log = m;
}

// forward declarations
static uint64_t csrget(csr src1);
static void csrset(csr src1, uint64_t val);
static void tmask_conv();
static void tcoop(uint64_t value);
static void tensorload(uint64_t control);
static void tensorloadl2(uint64_t control);
static void tensorstore(uint64_t tstorereg);
static void tensorfma(uint64_t tfmareg);
static void tensorquant(uint64_t value);
static void tensorreduce(uint64_t value);
static uint64_t csr_cacheop_emu(uint64_t op_value);
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
    for (int _zeufb_index = start; _zeufb_index < VL; ++_zeufb_index) \
        FREGS[regid].u[_zeufb_index] = 0; \
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
    return fnames[(flags & 0x1f) | ((flags >> 2) & 0x20)];
}

static uint64_t sext32(uint32_t val)
{
    uint32_t s = val & 0x80000000;
    uint64_t r = s ? (0xffffffff00000000ull | val) : val;
    return r;
}

static uint64_t sext16(uint32_t val)
{
    uint32_t s = val & 0x00008000;
    uint64_t r = s ? (0xffffffffffff0000ull | val) : val;
    return r;
}

static uint64_t sext10(uint32_t val)
{
    uint32_t s = val & 0x0000200;
    uint64_t r = s ? (0xfffffffffffffc00ull | val) : val;
    return r;
}

static uint64_t sext8(uint32_t val)
{
    uint32_t s = val & 0x0000080;
    uint64_t r = s ? (0xffffffffffffff00ull | val) : val;
    return r;
}

static int32_t sext8_2(uint8_t val)
{
    uint32_t s = val & 0x80;
    int32_t r = s ? (0xffffff00 | val) : val;
    return r;
}

static void check_sp_out_of_range(uint64_t val)
{
    uint64_t lo = uint64_t(&shaderstack[current_thread][0]);
    uint64_t hi = uint64_t(&shaderstack[current_thread][MAXSTACK]);
    if (val < lo || val > hi)
        throw std::runtime_error("stack pointer is out-of-range");
}

void init(xreg dst, uint64_t val)
{
    if (dst != x0)
    {
       XREGS[dst].x = val;
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
    uint64_t val = XREGS[src1].x;
    return val;
}

void fpinit(freg dst, uint64_t val[2])
{
    FREGS[dst].x[0] = val[0];
    FREGS[dst].x[1] = val[1];
}

// internal accessor to prv; this is faster than doing csrget(csr_prv)
static inline int prvget()
{
    return csrregs[current_thread][csr_prv];
}

// internal accessor to frm; this is faster than doing csrget(csr_frm)
static inline int frm()
{
    return ((csrregs[current_thread][csr_fcsr] >> 5) & 0x7);
}

// internal accessor to tensor_error; this is faster than doing csrset()
static inline void update_tensor_error(uint64_t value)
{
    csrregs[current_thread][csr_tensor_error] |= value;
    LOG(DEBUG, "\tTensorError = 0x%016" PRIx64 " (0x%016" PRIx64 ")", csrregs[current_thread][csr_tensor_error], value);
}

// internal accessor to fflags; this is faster than doing
// csrset(csr_fflags, csrget(csr_fflags) | new_value)
static inline void update_fflags(uint_fast8_t flags)
{
    uint32_t newval = (flags & 0x1F) | (uint32_t(flags & 0x80) << 24);
    logfflagschange(newval);
    csrregs[current_thread][csr_fcsr] |= newval;
    LOG(DEBUG, "\tfpu flags = 0x%02x (%s)", flags, get_fp_flags(flags));
}

// internal accessor to mstatus.fs; this is faster than using csrset()
static inline void dirty_fp_state()
{
    csrregs[current_thread][csr_mstatus] |= 0x8000000000006000ULL;
}

static inline void require_fp_active()
{
    if ((csrregs[current_thread][csr_mstatus] & 0x0006000ULL) == 0)
        throw trap_illegal_instruction(current_inst);
}

static inline void handle_denormal(iufval32& a)
{
    if ((a.u & 0x7f800000) == 0)
        a.u &= 0x80000000; // preserve sign
}

static inline bool isNaN(uint32_t a)
{
    return (((~(a) & 0x7F800000) == 0) && ((a) & 0x007FFFFF));
}

static inline void handle_nan_default(iufval32& a)
{
    if (isNaN(a.u))
        a.u = 0x7FC00000;
}

static inline void clear_arithmetic_flags()
{
    fpu::flags(0);
}

static inline void set_rounding_mode(rounding_mode mode)
{
    uint_fast8_t round = (mode == rmdyn) ? frm() : mode;
    if (round > 4)
        throw trap_illegal_instruction(current_inst);
    fpu::rm(round);
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
    if (fpu::flags())
    {
        dirty_fp_state();
        update_fflags(fpu::flags());
        fpu::flags(0);
    }
}

static float gold_frsq(float a)
{
    iufval32 res, val;

    val.flt =a ;
    handle_denormal(val);
    res.flt = float(double(1.0) / sqrt(double(val.flt)));
    handle_nan_default(res);
    handle_denormal(res);
    return res.flt;
}

//TODO: Erase this once Grigoris does his refactor
static float gold_fsin(float a)
{
    double dummy;
    iufval32 res, val, tmp;

    val.flt = a;
    handle_denormal(val);

#define infinityF32UI      0x7F800000
#define minusInfinityF32UI 0xFF800000
    //Take care of special cases ruined by modf
    switch (val.u) {
      case minusInfinityF32UI:
          val.u = 0x7fc00000;
          return val.flt;
          break;
      case infinityF32UI     :
          val.u = 0x7fc00000;
          return val.flt;
          break;
    }

    tmp.flt = float(modf(double(val.flt), &dummy));
    tmp.flt =
          tmp.flt <= -0.75 ?  tmp.flt + 1.0 // -IV  Quartile
        : tmp.flt <= -0.5  ? -tmp.flt - 0.5 // -III Quartile
        : tmp.flt <= -0.25 ? -tmp.flt - 0.5 // -II  Quartile
        : tmp.flt <= -0.0  ?  tmp.flt       // -I   Quartile
        : tmp.flt >= 0.75  ?  tmp.flt - 1.0 // +IV  Quartile
        : tmp.flt >= 0.5   ? -tmp.flt + 0.5 // +III Quartile
        : tmp.flt >= 0.25  ? -tmp.flt + 0.5 // +II  Quartile
        : tmp.flt;                          // +I   Quartile

    if (tmp.flt == 0.25) res.flt = 1.0;
    else if (tmp.flt == -0.25) res.flt = -1.0;
    else if (tmp.flt == 0.0) res.flt = 0.0;
    else if (tmp.flt == -0.0) res.flt = -0.0;
    else res.flt = float(sin(2.0 * M_PI * double(tmp.flt)));

    handle_nan_default(res);
    handle_denormal(res);
    return res.flt;
}

static float gold_fexp(float a)
{
    iufval32 res, val;

    val.flt = a;
    handle_denormal(val);
    res.flt = exp2f(val.flt);
    handle_nan_default(res);
    handle_denormal(res);
    return res.flt;
}

static float gold_flog(float a)
{
    iufval32 res, val;

    val.flt = a;
    handle_denormal(val);
    res.flt = float(log2(double(val.flt)));
    handle_nan_default(res);
    handle_denormal(res);
    return res.flt;
}

static float gold_frcp(float a)
{
    iufval32 res, val;

    val.flt = a;
    handle_denormal(val);
    res.flt = float(double(1.0) / double(val.flt));
    handle_nan_default(res);
    handle_denormal(res);
    return res.flt;
}

static int32_t gold_frcp_fix_rast(int32_t a, int32_t b)
{
    // Input value is 2xtriArea with 15.16 precision
    double fval_a = double(a) / double(1 << 16);
    double fval_b = double(b) / double(1 << 14);
    // Result value is 17.14
    double fres = (2*fval_b - fval_b*fval_b*fval_a) * double(1 << 14);
    return int32_t(fres);
}

void initcsr(uint32_t thread)
{
    // Exit reset at M-mode
    csrregs[thread][csr_prv] = CSR_PRV_M;
    // Read-only registers
    csrregs[thread][csr_mvendorid] = (11<<7) | ( 0xe5 & 0x7f); // bank 11, code=0xE5 (0x65 without parity)
    csrregs[thread][csr_marchid] = 0x8000000000000001ULL;
    csrregs[thread][csr_mimpid] = 0x0;
    csrregs[thread][csr_mhartid] = thread;
    // misa is a 0-length register
    csrregs[thread][csr_misa] = CSR_ISA_MAX;
    // M-mode registers with reset
    csrregs[thread][csr_mstatus] = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    csrregs[thread][csr_mcause] = 0x0ULL;
    csrregs[thread][csr_mip] = 0x0ULL;
    csrregs[thread][csr_msleep_txfma_27] = 0x0ULL;
    csrregs[thread][csr_menable_shadows] = 0x0ULL;
    csrregs[thread][csr_excl_mode] = 0x0ULL;
    csrregs[thread][csr_mtxfma_sleep_traps] = 0x0ULL;
    csrregs[thread][csr_mcounteren] = 0x0ULL;
    csrregs[thread][csr_scounteren] = 0x0ULL;
    // Debug-mode registers with reset
    // TODO: csrregs[thread][csr_dcsr] <= xdebugver=1, prv=3;

    // Ports
    for (int i = 0; i < NR_MSG_PORTS; ++i)
    {
        memset(&msg_ports[thread][i], 0, sizeof(msg_port_conf_t));
        msg_ports[thread][i].offset = -1;
    }
    csrregs[thread][csr_portctrl0] = 0x0000000000008000ULL;
    csrregs[thread][csr_portctrl1] = 0x0000000000008000ULL;
    csrregs[thread][csr_portctrl2] = 0x0000000000008000ULL;
    csrregs[thread][csr_portctrl3] = 0x0000000000008000ULL;

    csrregs[thread][csr_minstmask] = 0x0ULL;
}

void minit(mreg dst, uint64_t val)
{
    for (int i = 0; i < VL; ++i)
    {
        MREGS[dst].b[i] = val & 0x1;
        val = val >> 1;
        LOG(DEBUG, "init m[%d].b[%d] <-- %d", int(dst), i, int(MREGS[dst].b[i]));
    }
}

static bool tmask_pass(int bit)
{
    // Returns the pass bit for a specific bit
    return ((csrget(csr_tensor_mask) >> bit) & 1);
}

static uint8_t security_ulp_check(uint32_t gold, uint32_t table)
{
    // Fast skip for zeros and infinity should be the same value in both in gold and table
    if (gold == table)
        return 0;

    // Detect NaNs
    bool gold_is_nan  = isNaN(gold);
    bool table_is_nan = isNaN(table);

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

static void trap_to_smode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = prvget();
    assert(curprv <= CSR_PRV_S);

    LOG(DEBUG, "\tTrapping to S-mode with cause %" PRIu64, cause);

    // Take sie
    uint64_t mstatus = csrget(csr_mstatus);
    uint64_t sie = (mstatus >> 1) & 0x1;
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set spie = sie, sie = 0, spp = prv
    csrset(csr_mstatus, mstatus_clean | (curprv << 8) | (sie << 5));
    // Set scause, stval and sepc
    csrset(csr_scause, cause);
    csrset(csr_stval, val);
    csrset(csr_sepc, current_pc);
    // Jump to stvec
    csrset(csr_prv, CSR_PRV_S);

    // Throw an error if no one ever set stvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (stvec_is_set[current_thread] == false)
        LOG(DEBUG, "WARNING Trap vector has never been set. Can't take exception properly");

    // compute address where to jump to:
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause[3:0] for interrupts, tvec for exceptions
    uint64_t tvec = csrget(csr_stvec);
    if ((tvec & 1) == 1 && (cause >> 31) == 1)
    {
        // vectored mode and interrupt => add +4 * cause
        tvec &=  ~0x1ULL;
        tvec += (cause & 0xF) << 2;
    }
    else
    {
        tvec &=  ~0x1ULL;
    }
    logpcchange(tvec);
}

static void trap_to_mmode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = prvget();

    // Check if we should deletegate the trap to S-mode
    if ((curprv < CSR_PRV_M) && (csrget(csr_medeleg) & (1ull << cause)))
    {
        trap_to_smode(cause, val);
        return;
    }

    LOG(DEBUG, "\tTrapping to M-mode with cause %" PRIu64 " and tval %" PRIx64, cause, val);

    // Take mie
    uint64_t mstatus = csrget(csr_mstatus);
    uint64_t mie = (mstatus >> 3) & 0x1;
    // Clean mie, mpie and mpp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFE777ULL;
    // Set mpie = mie, mie = 0, mpp = prv
    csrset(csr_mstatus, mstatus_clean | (curprv << 11) | (mie << 7));
    // Set mcause, mtval and mepc
    csrset(csr_mcause, cause);
    csrset(csr_mtval, val);
    csrset(csr_mepc, current_pc);
    // Jump to mtvec
    csrset(csr_prv, CSR_PRV_M);

    // Throw an error if no one ever set mtvec otherwise we'll enter an infinite loop of illegal
    // instruction exceptions
    if (mtvec_is_set[current_thread] == false)
        LOG(DEBUG, "WARNING Trap vector has never been set. Doesn't smell good...");

    // compute address where to jump to
    //  if tvec[0]==0 (direct mode) => jump to tvec
    //  if tvec[0]==1 (vectored mode) => jump to tvec + 4*cause[3:0] for interrupts, tvec for exceptions
    uint64_t tvec = csrget(csr_mtvec);
    if ((tvec & 1) == 1 && (cause >> 31) == 1)
    {
        // vectored mode and interrupt => add +4 * cause
        tvec &=  ~0x1ULL;
        tvec += (cause & 0xF) << 2;
    }
    else
    {
        tvec &=  ~0x1ULL;
    }
    logpcchange(tvec);
}

void take_trap(const trap_t& t)
{
    trap_to_mmode(t.get_cause(), t.get_tval());
}

void check_minst_match(uint32_t bits)
{
    uint64_t minstmask = csrget(csr_minstmask);
    if ((minstmask >> 32) != 0)
    {
        uint32_t mask = minstmask;
        if (((bits ^ csrget(csr_minstmatch)) & mask) == 0)
            throw trap_mcode_instruction(bits);
    }
}

void set_core_type(et_core_t core)
{
   core_type = core;
}

et_core_t get_core_type()
{
   return core_type;
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
    return uint32_t((MREGS[maskNr].b[7] << 7) |
                    (MREGS[maskNr].b[6] << 6) |
                    (MREGS[maskNr].b[5] << 5) |
                    (MREGS[maskNr].b[4] << 4) |
                    (MREGS[maskNr].b[3] << 3) |
                    (MREGS[maskNr].b[2] << 2) |
                    (MREGS[maskNr].b[1] << 1) |
                    (MREGS[maskNr].b[0] << 0));
}

extern inst_state_change * log_info;

////////////////////////////////////////////////////////////////////////////////
//
// Memory emulation
//
////////////////////////////////////////////////////////////////////////////////

// Accessor functions for externally defined memory

typedef uint8_t  (*func_memread8_t) (uint64_t addr);
typedef uint16_t (*func_memread16_t)(uint64_t addr);
typedef uint32_t (*func_memread32_t)(uint64_t addr);
typedef uint64_t (*func_memread64_t)(uint64_t addr);

typedef void (*func_memwrite8_t)  (uint64_t addr, uint8_t data);
typedef void (*func_memwrite16_t) (uint64_t addr, uint16_t data);
typedef void (*func_memwrite32_t) (uint64_t addr, uint32_t data);
typedef void (*func_memwrite64_t) (uint64_t addr, uint64_t data);

static func_memread8_t   func_memread8   = NULL;
static func_memread16_t  func_memread16  = NULL;
static func_memread32_t  func_memread32  = NULL;
static func_memread64_t  func_memread64  = NULL;
static func_memwrite8_t  func_memwrite8  = NULL;
static func_memwrite16_t func_memwrite16 = NULL;
static func_memwrite32_t func_memwrite32 = NULL;
static func_memwrite64_t func_memwrite64 = NULL;

// forward declaration
static uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc);

static uint64_t virt_to_phys_host(uint64_t addr, mem_access_type macc)
{
    if (macc != Mem_Access_Fetch)
        throw std::runtime_error("virt_to_phys_host() for loads/stores is unimplemented");
    return addr;
}

static uint8_t emu_pmemread8(uint64_t paddr)
{
    log_info->mem_addr[0] = paddr;
    uint8_t data = func_memread8(paddr);
    LOG(DEBUG, "MEM8 %i, %02" PRIx16 " = [%016" PRIx64 "]", current_thread, data, paddr);
    return data;
}

static uint16_t emu_pmemread16(uint64_t paddr)
{
    log_info->mem_addr[0] = paddr;
    uint16_t data = func_memread16(paddr);
    LOG(DEBUG, "MEM16 %i, %04" PRIx16 " = [%016" PRIx64 "]", current_thread, data, paddr);
    return data;
}

static uint32_t emu_pmemread32(uint64_t paddr)
{
    log_info->mem_addr[0] = paddr;
    uint32_t data = func_memread32(paddr);
    LOG(DEBUG, "MEM32 %i, %08" PRIx32 " = [%016" PRIx64 "]", current_thread, data, paddr);
    return data;
}

static uint64_t emu_pmemread64(uint64_t paddr)
{
    log_info->mem_addr[0] = paddr;
    uint64_t data = func_memread64(paddr);
    LOG(DEBUG, "MEM64 %i, %016" PRIx64 " = [%016" PRIx64 "]", current_thread, data, paddr);
    return data;
}

static uint8_t emu_vmemread8(uint64_t addr)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    return emu_pmemread8(paddr);
}

static uint16_t emu_vmemread16(uint64_t addr)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    return emu_pmemread16(paddr);
}

static uint32_t emu_vmemread32(uint64_t addr)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    return emu_pmemread32(paddr);
}

static uint64_t emu_vmemread64(uint64_t addr)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    return emu_pmemread64(paddr);
}

static void emu_pmemwrite8(uint64_t paddr, uint8_t data)
{
    LOG(DEBUG, "MEM8 %i, [%016" PRIx64 "] = %02" PRIx8, current_thread, paddr, data);
    func_memwrite8(paddr, data);
}

static void emu_pmemwrite16(uint64_t paddr, uint16_t data)
{
    LOG(DEBUG, "MEM16 %i, [%016" PRIx64 "] = %04" PRIx16, current_thread, paddr, data);
    func_memwrite16(paddr, data);
}

static void emu_pmemwrite32(uint64_t paddr, uint32_t data)
{
    LOG(DEBUG, "MEM32 %i, [%016" PRIx64 "] = %08" PRIx32, current_thread, paddr, data);
    func_memwrite32(paddr, data);
}

static void emu_pmemwrite64(uint64_t paddr, uint64_t data)
{
    LOG(DEBUG, "MEM64 %i, [%016" PRIx64 "] = %016" PRIx64, current_thread, paddr, data);
    func_memwrite64(paddr, data);
}

static void emu_vmemwrite8(uint64_t addr, uint8_t data)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    emu_pmemwrite8(paddr, data);
}

static void emu_vmemwrite16(uint64_t addr, uint16_t data)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    emu_pmemwrite16(paddr, data);
}

static void emu_vmemwrite32(uint64_t addr, uint32_t data)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    emu_pmemwrite32(paddr, data);
}

static void emu_vmemwrite64(uint64_t addr, uint64_t data)
{
    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    emu_pmemwrite64(paddr, data);
}

static uint16_t emu_pmemfetch16(uint64_t paddr)
{
    return func_memread16(paddr);
}

static void emu_pmemfetch512(uint64_t paddr, uint16_t * data)
{
    for (int i = 0; i < 8; ++i)
    {
        uint64_t value = func_memread64(paddr + 8*i);
        ((uint64_t *) data)[i] = value;
        LOG(DEBUG, "MEM64 %i, %016" PRIx64 " = [%016" PRIx64 "]", current_thread, value, paddr + 8*i);
    }
}

static uint8_t host_vmemread8(uint64_t addr)
{
    return * ((uint8_t *) addr);
}

static uint16_t host_vmemread16(uint64_t addr)
{
    return * ((uint16_t *) addr);
}

static uint32_t host_vmemread32(uint64_t addr)
{
    return * ((uint32_t *) addr);
}

static uint64_t host_vmemread64(uint64_t addr)
{
    return * ((uint64_t *) addr);
}

static uint64_t host_pmemread64(uint64_t paddr)
{
    throw std::runtime_error("host_pmemread64() is unimplemented");
}

static void host_vmemwrite8(uint64_t addr, uint8_t data)
{
    * ((uint8_t *) addr) = data;
}

static void host_vmemwrite16(uint64_t addr, uint16_t data)
{
    * ((uint16_t *) addr) = data;
}

static void host_vmemwrite32(uint64_t addr, uint32_t data)
{
    * ((uint32_t *) addr) = data;
}

static void host_vmemwrite64(uint64_t addr, uint64_t data)
{
    * ((uint64_t *) addr) = data;
}

static void host_pmemwrite64(uint64_t paddr, uint64_t data)
{
    throw std::runtime_error("host_pmemwrite64() is unimplemented");
}

static uint16_t host_pmemfetch16(uint64_t addr)
{
    return * ((uint16_t *) addr);
}

static void host_pmemfetch512(uint64_t addr, uint16_t * data)
{
    memcpy(data, (void*) addr, 64);
}

// Abstract memory accessors. By default we use the host memory directly,
// unless we are asked to use emulated memory.

uint64_t (*vmemtranslate) (uint64_t addr, mem_access_type macc) = virt_to_phys_host;

uint8_t  (*vmemread8 ) (uint64_t addr) = host_vmemread8;
uint16_t (*vmemread16) (uint64_t addr) = host_vmemread16;
uint32_t (*vmemread32) (uint64_t addr) = host_vmemread32;
uint64_t (*vmemread64) (uint64_t addr) = host_vmemread64;

uint64_t (*pmemread64) (uint64_t paddr) = host_pmemread64;

void (*vmemwrite8 ) (uint64_t addr, uint8_t  data) = host_vmemwrite8;
void (*vmemwrite16) (uint64_t addr, uint16_t data) = host_vmemwrite16;
void (*vmemwrite32) (uint64_t addr, uint32_t data) = host_vmemwrite32;
void (*vmemwrite64) (uint64_t addr, uint64_t data) = host_vmemwrite64;

void (*pmemwrite64) (uint64_t paddr, uint64_t data) = host_pmemwrite64;

uint16_t (*pmemfetch16) (uint64_t paddr) = host_pmemfetch16;

void (*pmemfetch512) (uint64_t paddr, uint16_t * data) = host_pmemfetch512;

void set_memory_funcs(void * func_memread8_, void * func_memread16_,
                      void * func_memread32_, void * func_memread64_,
                      void * func_memwrite8_, void * func_memwrite16_,
                      void * func_memwrite32_, void * func_memwrite64_)
{
    func_memread8   = (func_memread8_t  ) func_memread8_;
    func_memread16  = (func_memread16_t ) func_memread16_;
    func_memread32  = (func_memread32_t ) func_memread32_;
    func_memread64  = (func_memread64_t ) func_memread64_;
    func_memwrite8  = (func_memwrite8_t ) func_memwrite8_;
    func_memwrite16 = (func_memwrite16_t) func_memwrite16_;
    func_memwrite32 = (func_memwrite32_t) func_memwrite32_;
    func_memwrite64 = (func_memwrite64_t) func_memwrite64_;

    vmemtranslate = virt_to_phys_emu;

    vmemread8  = emu_vmemread8;
    vmemread16 = emu_vmemread16;
    vmemread32 = emu_vmemread32;
    vmemread64 = emu_vmemread64;
    pmemread64 = emu_pmemread64;

    vmemwrite8  = emu_vmemwrite8;
    vmemwrite16 = emu_vmemwrite16;
    vmemwrite32 = emu_vmemwrite32;
    vmemwrite64 = emu_vmemwrite64;
    pmemwrite64 = emu_pmemwrite64;

    pmemfetch16 = emu_pmemfetch16;
    pmemfetch512 = emu_pmemfetch512;
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64I emulation
//
////////////////////////////////////////////////////////////////////////////////

// ILLEGAL INSTRUCTION
void unknown(const char* comm)
{
    LOG(DEBUG, "I: unknown @%016" PRIx64 "(0x%04x)%s%s", current_pc, current_inst, (comm?" # ":""), (comm?comm:""));
    throw trap_illegal_instruction(current_inst);
}

void beq(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: beq x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].x == XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bne(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: bne x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].x != XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void blt(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: blt x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].xs < XREGS[src2].xs)
        logpcchange(current_pc + imm);
}

void bltu(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: bltu x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].x < XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bge(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: bge x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].xs >= XREGS[src2].xs)
        logpcchange(current_pc + imm);
}

void bgeu(xreg src1, xreg src2, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: bgeu x%d, x%d, %" PRId64 "%s%s", src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    if (XREGS[src1].x >= XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void c_jalr(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    uint64_t val1 = XREGS[src1].x; // in case dst == src1
    LOG(DEBUG, "I: jalr x%d, x%d, %" PRId64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = current_pc + 2;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- ", XREGS[dst].x);
    }
    logxregchange(dst);
    logpcchange((val1 + imm) & 0xFFFFFFFFFFFFFFFE);
}

void c_jal(xreg dst, int64_t imm, const char* comm)
{
    // NB: spike-dasm already multiplies the immediate operand by 2
    LOG(DEBUG, "I: jal x%d, %" PRId64 "%s%s", dst, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = current_pc + 2;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- ", XREGS[dst].x);
    }
    logxregchange(dst);
    logpcchange(current_pc + imm);
}

void jalr(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    uint64_t val1 = XREGS[src1].x; // in case dst == src1
    LOG(DEBUG, "I: jalr x%d, x%d, %" PRId64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = current_pc + 4;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- ", XREGS[dst].x);
    }
    logxregchange(dst);
    logpcchange((val1 + imm) & 0xFFFFFFFFFFFFFFFE);
}

void jal(xreg dst, int64_t imm, const char* comm)
{
    // NB: spike-dasm already multiplies the immediate operand by 2
    LOG(DEBUG, "I: jal x%d, %" PRId64 "%s%s", dst, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = current_pc + 4;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- ", XREGS[dst].x);
    }
    logxregchange(dst);
    logpcchange(current_pc + imm);
}

void lui(xreg dst, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: lui x%d, 0x%016" PRIx64 "%s%s", dst, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = imm;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64, XREGS[dst].x, imm);
    }
    logxregchange(dst);
}

void auipc(xreg dst, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: auipc x%d, 0x%016" PRIx64 "%s%s", dst, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        XREGS[dst].x = current_pc + imm;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64, XREGS[dst].x, imm);
    }
    logxregchange(dst);
}

void addi(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: addi x%d, x%d, %" PRId64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x + imm;
        if (dst == x2 && check_stack)
        {
           check_sp_out_of_range(val);
        }
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " + 0x%016" PRIx64, val, XREGS[src1].x, imm);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void slli(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: slli x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = val1.x << imm;
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " << %u", XREGS[dst].x, val1.x, imm);
    }
    logxregchange(dst);
}

void slti(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: slti x%d, x%d, %" PRId64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = (XREGS[src1].xs < imm) ? 1 : 0;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " < 0x%016" PRIx64, val, XREGS[src1].x, imm);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sltiu(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: sltiu x%d, x%d, %" PRIu64 "%s%s", dst, src1, uint64_t(imm), (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = (XREGS[src1].x < uint64_t(imm)) ? 1 : 0;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " < 0x%016" PRIx64, val, XREGS[src1].x, imm);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void xori(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: xori x%d, x%d, 0x%016" PRIx64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x ^ imm;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " & 0x%016" PRIx64, val, XREGS[src1].x, imm);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void srli(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: srli x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = val1.x >> imm;
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " >> %u", XREGS[dst].x, val1.x, imm);
    }
    logxregchange(dst);
}

void srai(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: srai x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].xs = val1.xs >> imm;
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " >> %u", XREGS[dst].x, val1.x, imm);
    }
    logxregchange(dst);
}

void ori(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: ori x%d, x%d, 0x%016" PRIx64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = val1.x | imm;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " | 0x%016" PRIx64, XREGS[dst].x, val1.x, imm);
    }
    logxregchange(dst);
}

void andi(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: andi x%d, x%d, 0x%016" PRIx64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = val1.x & imm;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " & 0x%016" PRIx64, XREGS[dst].x, val1.x, imm);
    }
    logxregchange(dst);
}

void add(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: add x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x + XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " + 0x%016" PRIx64, val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sub(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sub x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x - XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " - 0x%016" PRIx64, val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sll(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sll x%d, x%d, %d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = val1.x << (val2.x & 0x3F);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " << %u", XREGS[dst].x, val1.x, unsigned(val2.x & 0x3f));
    }
    logxregchange(dst);
}

void slt(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: slt x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = (XREGS[src1].xs < XREGS[src2].xs) ? 1 : 0;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " < 0x%016" PRIx64, val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sltu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sltu x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = (XREGS[src1].x < XREGS[src2].x) ? 1 : 0;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " < 0x%016" PRIx64, val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void xor_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: xor x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x ^ XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " & 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void srl(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: srl x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = val1.x >> (val2.x & 0x3F);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " >> %u", XREGS[dst].x, val1.x, unsigned(val2.x & 0x3f));
    }
    logxregchange(dst);
}

void sra(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sra x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = val1.xs >> (val2.xs & 0x3F);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%016" PRIx64 " >> %u", XREGS[dst].x, val1.x, unsigned(val2.x & 0x3f));
    }
    logxregchange(dst);
}

void or_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: or x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x | XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " | 0x%016" PRIx64, val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void and_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: and x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x & XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " & 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void addiw(xreg dst, xreg src1, int64_t imm, const char* comm)
{
    LOG(DEBUG, "I: addiw x%d, x%d, %" PRId64 "%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = sext32(XREGS[src1].w[0] + imm);
        if (dst == x2 && check_stack)
        {
           check_sp_out_of_range(val);
        }
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x + 0x%08x", val, XREGS[src1].w[0], uint32_t(imm));
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void slliw(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: slliw x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(val1.w[0] << imm);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x << %u", XREGS[dst].x, val1.w[0], imm);
    }
    logxregchange(dst);
}

void srliw(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: srliw x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(val1.w[0] >> imm);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x >> %u", XREGS[dst].x, val1.w[0], imm);
    }
    logxregchange(dst);
}

void sraiw(xreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: sraiw x%d, x%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(val1.ws[0] >> imm);
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x >> %u", XREGS[dst].x, val1.w[0], imm);
    }
    logxregchange(dst);
}

void addw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: addw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = sext32(XREGS[src1].w[0] + XREGS[src2].w[0]);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x + 0x%08x", val, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void subw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: subw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = sext32(XREGS[src1].w[0] - XREGS[src2].w[0]);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x - 0x%08x", val, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sllw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sllw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(val1.w[0] << (val2.w[0] & 0x1F));
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x << %d", XREGS[dst].x, val1.w[0], val2.w[0] & 0x1f);
    }
    logxregchange(dst);
}

void srlw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: srlw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(val1.w[0] >> (val2.w[0] & 0x1F));
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x >> %d", XREGS[dst].x, val1.w[0], val2.w[0] & 0x1f);
    }
    logxregchange(dst);
}

void sraw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sraw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(val1.ws[0] >> (val2.ws[0] & 0x1F));
        LOG(DEBUG, "\t 0x%016" PRIx64 " <-- 0x%08x >> %d", XREGS[dst].x, val1.w[0], val2.w[0] & 0x1F);
    }
    logxregchange(dst);
}

void lb(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lb x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = sext8(vmemread8(XREGS[base].x + off));

    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void lh(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lh x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = sext16(vmemread16(XREGS[base].x + off));
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void lw(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lw x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = sext32(vmemread32(XREGS[base].x + off));
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void ld(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: ld x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = vmemread64(XREGS[base].x + off);
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x  = val;
    }
    logxregchange(dst);
}

void lbu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lbu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = vmemread8(XREGS[base].x + off);
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void lhu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lhu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = vmemread16(XREGS[base].x + off);
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void lwu(xreg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: lwu x%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t val = vmemread32(XREGS[base].x + off);
    if (dst != x0)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 "]", val, XREGS[base].x, off);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void sd(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sd x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x + off;
    uint64_t val  = XREGS[src1].x;
    vmemwrite64(addr, val);
    LOG(DEBUG, "\t%016" PRIx64 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 8, addr, val);
}

void sw(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sw x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x + off;
    uint32_t val  = XREGS[src1].w[0];
    vmemwrite32(addr, val);
    LOG(DEBUG, "\t0x%08" PRIx32 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 4, addr, val);
}

void sh(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sh x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x + off;
    uint16_t val  = XREGS[src1].h[0];
    vmemwrite16(addr, val);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 2, addr, val);
}

void sb(xreg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: sb x%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x + off;
    uint8_t val   = XREGS[src1].b[0];
    vmemwrite8(addr, val);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 1, addr, val);
}

void sbl(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: sbl x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x;
    uint8_t val   = XREGS[src1].b[0];
    vmemwrite8(addr, val);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 1, addr, val);
}

void sbg(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: sbg x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x;
    uint8_t val   = XREGS[src1].b[0];
    vmemwrite8(addr, val);
    LOG(DEBUG, "\t0x%02" PRIx8 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 1, addr, val);
}

void shl(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: shl x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x;
    uint16_t val  = XREGS[src1].h[0];
    vmemwrite16(addr, val);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 2, addr, val);
}

void shg(xreg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: shg x%d, (x%d)%s%s", src1, base, (comm?" # ":""), (comm?comm:""));
    uint64_t addr = XREGS[base].x;
    uint16_t val  = XREGS[src1].h[0];
    vmemwrite16(addr, val);
    LOG(DEBUG, "\t0x%04" PRIx16 " --> MEM[0x%016" PRIx64 "]", val, addr);
    logmemwchange(0, 2, addr, val);
}


void fence(const char* comm)
{
    LOG(DEBUG, "I: fence%s%s", (comm?" # ":""), (comm?comm:""));
}

void fence_i(const char* comm)
{
    LOG(DEBUG, "I: fence_i%s%s", (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    flush_insn_cache();
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64M emulation
//
////////////////////////////////////////////////////////////////////////////////

void mul(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: mul x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = XREGS[src1].x * XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " * 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void mulh(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: mulh x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        __int128_t val1 = XREGS[src1].xs;
        __int128_t val2 = XREGS[src2].xs;
        __int128_t val3 = val1 * val2;
        int64_t val = val3 >> 64;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " * 0x%016" PRIx64 "s", val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void mulhsu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: mulhsu x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        __int128_t  val1 = XREGS[src1].xs;
        __uint128_t val2 = XREGS[src2].x;
        __int128_t  val3 = val1 * val2;
        uint64_t val = val3 >> 64;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " * 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void mulhu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: mulhu x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        __uint128_t val1 = XREGS[src1].x;
        __uint128_t val2 = XREGS[src2].x;
        __uint128_t val3 = val1 * val2;
        uint64_t val = val3 >> 64;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " * 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void div_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: div x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int64_t val;
        if (XREGS[src2].x == 0)
        {
            val = -1;
        }
        else if ((XREGS[src2].xs == -1) && (XREGS[src1].x == 0x8000000000000000ULL))
        {
            val = XREGS[src1].xs; // Divide is out of range, return src1
        }
        else
        {
            val = XREGS[src1].xs / XREGS[src2].xs;
        }
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " / 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = uint64_t(val);
    }
    logxregchange(dst);
}

void divu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: divu x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val;
        if (XREGS[src2].x == 0)
            val = 0xFFFFFFFFFFFFFFFFULL;
        else
            val = XREGS[src1].x / XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " / 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void rem(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: rem x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int64_t val;
        if (XREGS[src2].x == 0)
            val = XREGS[src1].xs;
        else if ((XREGS[src2].xs == -1) && (XREGS[src1].x == 0x8000000000000000ULL))
            val = 0; // Divide is out of range in x86, return 0 straight
        else
            val = XREGS[src1].xs % XREGS[src2].xs;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " %% 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = uint64_t(val);
    }
    logxregchange(dst);
}

void remu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: remu x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val;
        if (XREGS[src2].x == 0)
            val = XREGS[src1].x;
        else
            val = XREGS[src1].x % XREGS[src2].x;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " %% 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void mulw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: mulw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = sext32(XREGS[src1].x * XREGS[src2].x);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x * 0x%08x", val, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void divw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: divw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int32_t val;
        if (XREGS[src2].ws[0] == 0)
            val = -1;
        else if ((XREGS[src2].ws[0] == -1) && (XREGS[src1].w[0] == 0x80000000))
            val = XREGS[src1].ws[0]; // Divide is out of range, return src1
        else
            val = XREGS[src1].ws[0] / XREGS[src2].ws[0];
        uint64_t val64 = sext32(val);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x / 0x%08x", val64, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
}

void divuw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: divuw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int32_t val;
        if (XREGS[src2].w[0] == 0)
            val = -1;
        else
            val = XREGS[src1].w[0] / XREGS[src2].w[0];
        uint64_t val64 = sext32(val);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x / 0x%08x", val64, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
}

void remw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: remw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int32_t val;
        if (XREGS[src2].ws[0] == 0)
            val = XREGS[src1].ws[0]; // Divide by 0
        else if ((XREGS[src2].ws[0] == -1) && (XREGS[src1].w[0] == 0x80000000))
            val = 0;                 // Divide is out of range in x86, return 0 straight
        else
            val = XREGS[src1].ws[0] % XREGS[src2].ws[0];
        uint64_t val64 = sext32(val);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x %% 0x%08x", val64, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
}

void remuw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: remuw x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        int32_t val;
        if (XREGS[src2].w[0] == 0)
            val = XREGS[src1].w[0];
        else
            val = XREGS[src1].w[0] % XREGS[src2].w[0];
        uint64_t val64 = sext32(val);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x %% 0x%08x", val64, XREGS[src1].w[0], XREGS[src2].w[0]);
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64A emulation
//
////////////////////////////////////////////////////////////////////////////////

#define AMO_EMU_W_FUNC(NAME, OPC) \
void NAME(xreg dst, xreg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " x%d, x%d, (x%d)%s%s", dst, src2, src1, comm ? " # " : "", comm ? comm : "");\
   amo_emu_w(OPC, dst, src1, src2);\
}

#define AMO_EMU_D_FUNC(NAME, OPC) \
void NAME(xreg dst, xreg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " x%d, x%d, (x%d)%s%s", dst, src2, src1, comm ? " # " : "", comm ? comm : "");\
   amo_emu_d(OPC, dst, src1, src2);\
}

static void amo_emu_w(amoop op, xreg dst, xreg src1, xreg src2)
{
    uint64_t addr;
    uint32_t res, val1, val2;

    addr = XREGS[src1].x;

    // Check misaligned access
    if ((addr & 0x3) != 0) throw trap_store_address_misaligned(addr);

    val1 = vmemread32(addr);
    val2 = XREGS[src2].w[0];

    // Save the loaded data
    if (dst != x0)
    {
        XREGS[dst].x = sext32(val1);
        LOG(DEBUG, "\t0x%08" PRIx64 " <-- MEM[0x%016" PRIx64 "]", XREGS[dst].x, addr);
    }
    logxregchange(dst);

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
    vmemwrite32(addr, res);
    LOG(DEBUG, "\t0x%08x --> MEM[0x%016" PRIx64 "]", res, addr);
    // note: for logging purposes, sending val2 instead of res => we want to check what the
    // dcache outputs to the shire caches, not the actual value written in memory
    logmemwchange(0, 4, addr, val2);
}

static void amo_emu_d(amoop op, xreg dst, xreg src1, xreg src2)
{
    uint64_t addr, val1, val2, res;

    addr = XREGS[src1].x;

    // Check misaligned access
    if ((addr & 0x7) != 0) throw trap_store_address_misaligned(addr);

    val1 = vmemread64(addr);
    val2 = XREGS[src2].x;

    // Save the loaded data
    if (dst != x0)
    {
        XREGS[dst].x = val1;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- MEM[0x%016" PRIx64 "]", val1, addr);
    }
    logxregchange(dst);

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
    vmemwrite64(addr, res);
    LOG(DEBUG, "\t0x%016" PRIx64 " --> MEM[0x%016" PRIx64 "]", res, addr);
    // note: for logging purposes, sending val2 instead of res => we want to check what the
    // dcache outputs to the shire caches, not the actual value written in memory
    logmemwchange(0, 8, addr, val2);
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
static int dcache_evict_flush_vaddr(bool, bool, int, uint64_t, int, int, uint64_t);
static int dcache_prefetch_vaddr(bool, int, uint64_t, int, int, uint64_t);
static int dcache_lock_vaddr(bool, int, uint64_t, int, int, uint64_t);
static int dcache_unlock_vaddr(bool, bool, uint64_t, int, int, uint64_t);
static int dcache_lock_paddr(int, uint64_t);
static int dcache_unlock_paddr(int, uint64_t);

static uint64_t csrget(csr src1)
{
    uint64_t val;

    switch (src1)
    {
        // ----- Illegal registers ---------------------------------------
        case csr_unknown:
            throw trap_illegal_instruction(current_inst);
        // ----- U-mode registers ----------------------------------------
        case csr_fflags:
            val = csrregs[current_thread][csr_fcsr] & 0x8000001f;
            break;
        case csr_frm:
            val = (csrregs[current_thread][csr_fcsr] >> 5) & 0x7;
            break;
        case csr_cycle:
        case csr_instret:
            val = 0;
            break;
        case csr_time:
        case csr_hpmcounter3:
        case csr_hpmcounter4:
        case csr_hpmcounter5:
        case csr_hpmcounter6:
        case csr_hpmcounter7:
        case csr_hpmcounter8:
        case csr_hpmcounter9:
        case csr_hpmcounter10:
        case csr_hpmcounter11:
        case csr_hpmcounter12:
        case csr_hpmcounter13:
        case csr_hpmcounter14:
        case csr_hpmcounter15:
        case csr_hpmcounter16:
        case csr_hpmcounter17:
        case csr_hpmcounter18:
        case csr_hpmcounter19:
        case csr_hpmcounter20:
        case csr_hpmcounter21:
        case csr_hpmcounter22:
        case csr_hpmcounter23:
        case csr_hpmcounter24:
        case csr_hpmcounter25:
        case csr_hpmcounter26:
        case csr_hpmcounter27:
        case csr_hpmcounter28:
        case csr_hpmcounter29:
        case csr_hpmcounter30:
        case csr_hpmcounter31:
            // Legal to read only if machine mode or {m,s}counteren correctly configured for next level of privilege
            if (   ((prvget() == CSR_PRV_U) && ((csrregs[current_thread][csr_scounteren] & (1 << (src1-csr_cycle))) == 0))
                || ((prvget() == CSR_PRV_S) && ((csrregs[current_thread][csr_mcounteren] & (1 << (src1-csr_cycle))) == 0)))
            {
               throw trap_illegal_instruction(current_inst);
            }
            val = csrregs[current_thread][src1];
            break;
        case csr_porthead0:
        case csr_porthead1:
        case csr_porthead2:
        case csr_porthead3:
            // Check that port is enabled and configured to be legally accessed by U-mode otherwise exception
            if (   (((csrregs[current_thread][csr_portctrl0 + src1 - csr_porthead0]) & 0x1) == 0)
                || ((prvget() == CSR_PRV_U) && ((csrregs[current_thread][csr_portctrl0 + src1 - csr_porthead0] & 0x0000000000000008ULL) == 0)))
            {
               throw trap_illegal_instruction(current_inst);
            }
            val = port_get(src1 - csr_porthead0, true);
            break;
        case csr_portheadnb0:
        case csr_portheadnb1:
        case csr_portheadnb2:
        case csr_portheadnb3:
            // Check that port is enabled and configured to be legally accessed by U-mode otherwise exception
            if (   (((csrregs[current_thread][csr_portctrl0 + src1 - csr_portheadnb0]) & 0x1) == 0)
                || ((prvget() == CSR_PRV_U) && ((csrregs[current_thread][csr_portctrl0 + src1 - csr_portheadnb0] & 0x0000000000000008ULL) == 0)))
            {
               throw trap_illegal_instruction(current_inst);
            }
            val = port_get(src1 - csr_portheadnb0, false);
            break;
        case csr_sleep_txfma_27:
          if (prvget() != CSR_PRV_M && (csrregs[current_thread][csr_menable_shadows] & 2) == 0)
          {
               throw trap_illegal_instruction(current_inst);
          }
          val = csrregs[current_thread][csr_msleep_txfma_27];
          break;
        case csr_hartid:
          //check shadow is allowed
          if (prvget() != CSR_PRV_M && (csrregs[current_thread][csr_menable_shadows] & 1) == 0)
          {
               throw trap_illegal_instruction(current_inst);
          }
          val = csrregs[current_thread][csr_mhartid];
          break;
        // ----- S-mode registers ----------------------------------------
        case csr_sstatus:
            // Hide sxl, tsr, tw, tvm, mprv, mpp, mpie, mie
            val = csrregs[current_thread][csr_mstatus] & 0xFFFFFFF3FF8DE7FFULL;
            break;
        case csr_sie:
            val = csrregs[current_thread][csr_mie] & csrregs[current_thread][csr_mideleg];
            break;
        case csr_sip:
            val = csrregs[current_thread][csr_mip] & csrregs[current_thread][csr_mideleg];
            break;
        // ----- Tensor, barrier, cacheop instructions -------------------
        case csr_tensor_load:
        case csr_tensor_coop:
        case csr_tensor_quant:
        case csr_tensor_fma:
        case csr_tensor_reduce:
        case csr_tensor_store:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
        case csr_tensor_load_l2:
        case csr_tensor_wait:
        case csr_flb0:
        case csr_fcc:
        case csr_stall:
        case csr_usr_cache_op:
        case csr_evict_va:
        case csr_flush_va:
        case csr_lock_va:
        case csr_unlock_va:
        case csr_lock_sw:
        case csr_unlock_sw:
        case csr_prefetch_va:
        case csr_mcache_control:
        case csr_sys_cache_op:
        case csr_evict_sw:
        case csr_flush_sw:
        case csr_flush_icache:
            val = 0;
            break;
        // ----- M-mode registers ----------------------------------------
        case csr_mcycle:
        case csr_minstret:
            val = 0;
            break;
        // ----- All other registers -------------------------------------
        default:
            val = csrregs[current_thread][src1];
            break;
    }
    return val;
}

uint64_t get_fcc_cnt(){
    return fcc_cnt;
}

/* TODO remove this nasty fix*/
uint64_t get_data_from_mem_64(uint64_t addr){
    return vmemread64(addr);
}

static void csrset(csr src1, uint64_t val)
{
    uint64_t msk;

    switch (src1)
    {
        // ----- Read-only and illegal registers -------------------------
        case csr_cycle:
        case csr_instret:
        case csr_mvendorid:
        case csr_marchid:
        case csr_mimpid:
        case csr_mhartid:
        case csr_hartid:
        case csr_porthead0:
        case csr_porthead1:
        case csr_porthead2:
        case csr_porthead3:
        case csr_portheadnb0:
        case csr_portheadnb1:
        case csr_portheadnb2:
        case csr_portheadnb3:
        case csr_unknown:
            throw trap_illegal_instruction(current_inst);
        // ----- Internal registers --------------------------------------
        case csr_prv:
            val &= 0x0000000000000003ULL;
            csrregs[current_thread][src1] = val;
            break;
        // ----- U-mode registers ----------------------------------------
        case csr_fflags:
            val = (csrregs[current_thread][csr_fcsr] & 0xE0) | (val & 0x1F);
            csrregs[current_thread][csr_fcsr] = val;
            break;
        case csr_frm:
            val = (csrregs[current_thread][csr_fcsr] & 0x1F) | ((val & 0x7) << 5);
            csrregs[current_thread][csr_fcsr] = val;
            break;
        case csr_fcsr:
            val &= 0x00000000000000FFULL;
            csrregs[current_thread][src1] = val;
            break;
        // ----- U-mode ET registers ---------------------------------------------
        case csr_tensor_load:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            tensorload(val);
            break;
        case csr_tensor_load_l2:
            tensorloadl2(val);
            break;
        case csr_tensor_mask:
            val &= 0x000000000000FFFFULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_tensor_conv_size:
            val &= 0xFF00FFFFFF00FFFFULL;
            csrregs[current_thread][src1] = val;
            tmask_conv();
            break;
        case csr_tensor_conv_ctrl:
            val &= 0x0000FFFF0000FFFFULL;
            csrregs[current_thread][src1] = val;
            tmask_conv();
            break;
        case csr_tensor_coop:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            val &= 0x0000000000FFFFFFULL;
            tcoop(val);
            break;
        case csr_tensor_quant:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            if (!txfma_off_allowed(src1, val))
                throw trap_txfma_off(current_inst);
            tensorquant(val);
            break;
        case csr_tensor_fma:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            if (!txfma_off_allowed(src1, val))
                throw trap_txfma_off(current_inst);
            tensorfma(val);
            break;
        case csr_tensor_reduce:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            if (!txfma_off_allowed(src1, val))
                throw trap_txfma_off(current_inst);
            tensorreduce(val);
            break;
        case csr_tensor_store:
            if (current_thread % EMU_THREADS_PER_MINION)
                throw trap_illegal_instruction(current_inst);
            tensorstore(val);
            break;
        case csr_fcc:
            fcc_cnt = val & 0x01;
            if (in_sysemu)
            {
                // If you are not going to block decrement it
                if (fcc[current_thread][fcc_cnt] != 0)
                    fcc[current_thread][fcc_cnt]--;
            }
            else
            {
                //fcc underflow
                if (fcc[current_thread][fcc_cnt] == 0)
                    LOG(DEBUG, "FCC underflow. You shouldn't be here...");
                fcc[current_thread][fcc_cnt]--;
            }
            break;
        case csr_stall:
            break;
        case csr_evict_va:
        case csr_flush_va:
            val &= 0x8C00FFFFFFFFFFCFULL;
            {
                bool     tm     = (val & 0x8000000000000000ULL);
                int      dest   = (val >> 58) & 0x03;
                int      count  = (val >>  0) & 0x0F;
                uint64_t vaddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                uint64_t stride = XREGS[31].x & 0x0000FFFFFFFFFFC0ULL;
                int      id     = XREGS[31].x & 0x0000000000000001ULL;
                dcache_evict_flush_vaddr(src1 == csr_evict_va, tm, dest, vaddr, count, id, stride);
            }
            break;
        case csr_lock_va:
            val &= 0xFF80FFFFFFFFFFCFULL;
            {
                bool     tm     = (val & 0x8000000000000000ULL);
                int      way    = (val >> 55) & 0xFF;
                int      count  = (val >>  0) & 0x0F;
                uint64_t vaddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                uint64_t stride = XREGS[31].x & 0x0000FFFFFFFFFFC0ULL;
                int      id     = XREGS[31].x & 0x0000000000000001ULL;
                if (dcache_lock_vaddr(tm, way, vaddr, count, id, stride))
                    update_tensor_error(1 << 5);
            }
            break;
        case csr_unlock_va:
            val &= 0xC000FFFFFFFFFFCFULL;
            {
                bool     tm     = (val & 0x8000000000000000ULL);
                bool     valid  = (val >> 55) & 0xFF;
                int      count  = (val >>  0) & 0x0F;
                uint64_t vaddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                uint64_t stride = XREGS[31].x & 0x0000FFFFFFFFFFC0ULL;
                int      id     = XREGS[31].x & 0x0000000000000001ULL;
                dcache_unlock_vaddr(tm, valid, vaddr, count, id, stride);
            }
            break;
        case csr_lock_sw:
            val &= 0xFF80FFFFFFFFFFCFULL;
            {
                int      way    = (val >> 55) & 0xFF;
                uint64_t paddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                int failed = dcache_lock_paddr(way, paddr);
           	    csrregs[current_thread][csr_tensor_error] |= (failed << 5); 
            }
            break;
        case csr_unlock_sw:
            val &= 0xC000FFFFFFFFFFCFULL;
            {
                int      way    = (val >> 55) & 0xFF;
                uint64_t paddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                int failed = dcache_unlock_paddr(way, paddr);
           	    csrregs[current_thread][csr_tensor_error] |= (failed << 5);
            }
            break;

        case csr_prefetch_va:
            val &= 0x8C00FFFFFFFFFFCFULL;
            {
                bool tm         = (val & 0x8000000000000000ULL);
                int  dest       = (val >> 58) & 0x03;
                int  count      = (val >>  0) & 0x0F;
                uint64_t vaddr  = val         & 0x0000FFFFFFFFFFC0ULL;
                uint64_t stride = XREGS[31].x & 0x0000FFFFFFFFFFC0ULL;
                int      id     = XREGS[31].x & 0x0000000000000001ULL;
                dcache_prefetch_vaddr(tm, dest, vaddr, count, id, stride);
            }
            break;
        case csr_scratchpad_ctrl: // Shared register
            val &= 0x0000000000000001ULL;
            csrregs[current_thread][src1] = val;
            csrregs[current_thread^1][src1] = val;
            num_sets = val ? 4 : 16;
            break;
        case csr_tex_send:
            val &= 0x00000000000000FFULL;
            csrregs[current_thread][src1] = val;
            // Notify to TBOX that a Sample Request is ready
            // unsigned port_id        = csrregs[current_thread][src1] & 0x0000000F;
            // unsigned number_packets = (csrregs[current_thread][src1] >> 4) & 0x0000000F;
            new_sample_request(csrregs[current_thread][src1] & 0x0000000F, (csrregs[current_thread][src1] >> 4) & 0x0000000F, read_port_base_address(current_thread, csrregs[current_thread][src1] & 0x0000000F));
            break;
        case csr_sleep_txfma_27:
            if (csrregs[current_thread][csr_prv] != CSR_PRV_M && (csrregs[current_thread][csr_menable_shadows] & 2) == 0)
            {
                throw trap_illegal_instruction(current_inst);
            }
            csrregs[current_thread][csr_msleep_txfma_27] = val;
            csrregs[current_thread ^ 1][csr_msleep_txfma_27] = val;
            break;
        // ----- S-mode registers ----------------------------------------
        case csr_sstatus:
            // Preserve sxl, uxl, tsr, tw, tvm, mprv, xs, mpp, mpie, mie
            val = (val & 0x00000000000C6133ULL) | (csrregs[current_thread][csr_mstatus] & 0x0000000F00739800ULL);
            // Set sd if fs==3 or xs==3
            if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
            {
                val |= 0x8000000000000000ULL;
            }
            csrregs[current_thread][csr_mstatus] = val;
            break;
        case csr_sie:
            // Preserve meie, mtie, msie
            // if mideleg[sei,sti,ssi]==1 then seie, stie, ssie is writeable, otherwise they are reserved
            msk = csrregs[current_thread][csr_mideleg];
            val = (csrregs[current_thread][csr_mie] & (~msk) & 0x0000000000000888ULL) | (val & msk & 0x0000000000000222ULL);
            csrregs[current_thread][csr_mie] = val;
            break;
        case csr_stvec:
            val &= 0xFFFFFFFFF001ULL;
            csrregs[current_thread][src1] = val;
            stvec_is_set[current_thread] = true;
            break;
        case csr_sepc:
            // sepc[0] = 0 always
            val &= 0xFFFFFFFFFFFFFFFEULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_sip:
            // Preserve meip, seip, mtip, stip, msip
            // if mideleg[ssi]==1 then ssip is writeable, otherwise it is reserved
            msk = csrregs[current_thread][csr_mideleg];
            val = (csrregs[current_thread][csr_mip] & (~msk) & 0x0000000000000BB8ULL) | (val & msk & 0x0000000000000002ULL);
            csrregs[current_thread][csr_mip] = val;
            break;
        case csr_satp: // Shared register
            // MODE is 4 bits, ASID is 0bits, PPN is PPN_M bits
            val &= 0xF000000000000000ULL | PPN_M;
            switch (val >> 60)
            {
                case SATP_MODE_BARE:
                case SATP_MODE_SV39:
                case SATP_MODE_SV48:
                    csrregs[current_thread][src1] = val;
                    csrregs[current_thread^1][src1] = val;
                    break;
                default: // reserved
                    // do not write the register if attempting to set an unsupported mode
                    break;
            }
            break;
        // ----- S-mode ET registers ---------------------------------------------
        case csr_evict_sw:
        case csr_flush_sw:
            val &= 0x8C0000000003C0CFULL;
            {
                bool tm    = (val & 0x8000000000000000ULL);
                int  dest  = (val >> 58) & 0x03;
                int  set   = (val >> 14) & 0x0F;
                int  way   = (val >>  6) & 0x03;
                int  count = (val >>  0) & 0x0F;
                dcache_evict_flush_set_way(src1 == csr_evict_sw, tm, dest, set, way, count);
            }
            break;
        case csr_portctrl0:
        case csr_portctrl1:
        case csr_portctrl2:
        case csr_portctrl3:
            val &= 0x00000000FFFF0FF3ULL;
            val |= 0x0000000000008000ULL;
            csrregs[current_thread][src1] = val;
            configure_port(src1 - csr_portctrl0, val);
            break;
        // ----- M-mode registers ----------------------------------------
        case csr_mstatus:
            // Preserve sxl, uxl, xs
            val = (val & 0x00000000007E78BBULL) | (csrregs[current_thread][src1] & 0x0000000F00018000ULL);
            // Set sd if fs==3 or xs==3
            if ((((val >> 13) & 0x3) == 0x3) || (((val >> 15) & 0x3) == 0x3))
            {
                val |= 0x8000000000000000ULL;
            }
            csrregs[current_thread][src1] = val;
            break;
        case csr_misa:
            // misa is a 0-length register, cannot be modified
            break;
        case csr_medeleg:
            // Not all exceptions can be delegated
            val &= 0x0000000000000B109ULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_mideleg:
            // Not all interrupts can be delegated
            val &= 0x0000000000000222ULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_mie:
            // Hard-wire ueie, utie, usie
            val &= 0x0000000000000AAAULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_mtvec:
            val &= 0xFFFFFFFFF001ULL;
            csrregs[current_thread][src1] = val;
            mtvec_is_set[current_thread] = true;
            break;
        case csr_mepc:
            // mepc[0] = 0 always
            val &= 0xFFFFFFFFFFFFFFFEULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_mip:
            // Hard-wire ueip, utip, usip
            // Write only seip, stip, ssip
            val &= 0x0000000000000222ULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_mcycle:
        case csr_minstret:
            // writes are ignored, always return 0
            break;
        // ----- Shared registers ----------------------------------------
        case csr_msleep_txfma_27:
        case csr_menable_shadows:
        case csr_mtxfma_sleep_traps:
          csrregs[current_thread][src1] = val;
          csrregs[current_thread^1][src1] = val;
          break;
        // ----- Verification registers ----------------------------------------
        case csr_validation1:
            // EOT signals end of test
            if ((char) val == 4)
            {
                LOG(INFO, "Validation1 CSR received End Of Transmission.");
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
        // ----- Not really ESRs -----------------------------------------
        case csr_usr_cache_op:
        case csr_sys_cache_op:
        case csr_umsg_port0:
        case csr_umsg_port1:
        case csr_umsg_port2:
        case csr_umsg_port3:
        case csr_smsg_port0:
        case csr_smsg_port1:
        case csr_smsg_port2:
        case csr_smsg_port3:
            // We shouldn't be here!
            assert(0);
            break;
        // ----- All other registers -------------------------------------
        default:
            csrregs[current_thread][src1] = val;
            break;
    }
}

static void csr_insn(xreg dst, csr src1, uint64_t oldval, uint64_t newval, bool write)
{
    // Check if current privilege mode has access to the register
    uint64_t prv = prvget();
    if (   ((prv == CSR_PRV_U) && (src1 >= CSR_MAX_UMODE))
        || ((prv == CSR_PRV_S) && (src1 >= CSR_MAX_SMODE)))
    {
        throw trap_illegal_instruction(current_inst);
    }
    if (write)
    {
        switch (src1)
        {
            // Fast local barrier instructions encoded in the CSR space
            case csr_flb0:
                oldval = flbarrier(newval);
                break;
            // TODO: remove old cacheop spec
            case csr_usr_cache_op:
            case csr_sys_cache_op:
                oldval = csr_cacheop_emu(newval);
                break;
            default:
                csrset(src1, newval);
                break;
        }
    }
    if (dst != x0)
    {
        XREGS[dst].x = oldval;
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- CSR[%s]", oldval, csr_names[src1]);
    }
    if (write)
    {
        LOG(DEBUG, "\t0x%016" PRIx64 " --> CSR[%s]", newval, csr_names[src1]);
    }
    logxregchange(dst);
}

void fcc_inc(uint64_t thread, uint64_t shire, uint64_t minion_mask, uint64_t fcc_id)
{
    for (int minion = 0; minion < EMU_MINIONS_PER_SHIRE; ++minion)
    {
        if (minion_mask & (1 << minion))
        {
            size_t fcc_addr = shire*EMU_THREADS_PER_SHIRE + EMU_THREADS_PER_MINION*minion + thread;
            fcc[fcc_addr][fcc_id] += 1;
            if (fcc[fcc_addr][fcc_id] == 0)
                update_tensor_error(1 << 3);
        }
    }
}

static void throw_page_fault(uint64_t addr, mem_access_type macc)
{
    switch (macc)
    {
        case Mem_Access_Load:
            throw trap_load_page_fault(addr);
            break;
        case Mem_Access_Store:
            throw trap_store_page_fault(addr);
            break;
        case Mem_Access_Fetch:
            throw trap_instruction_page_fault(addr);
            break;
    }
}

static uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc)
{

    // Read mstatus
    const uint64_t mstatus = csrget(csr_mstatus);
    const bool     mxr     = (mstatus >> MSTATUS_MXR ) & 0x1;
    const bool     sum     = (mstatus >> MSTATUS_SUM ) & 0x1;
    const bool     mprv    = (mstatus >> MSTATUS_MPRV) & 0x1;
    const bool     mpp     = (mstatus >> MSTATUS_MPP ) & 0x3;

    // Read satp
    const uint64_t satp      = csrget(csr_satp);
    const uint64_t satp_mode = (satp >> 60) & 0xF;
    const uint64_t satp_ppn  = satp & PPN_M;

    // Read prv
    const uint64_t prv = prvget();

    // Calculate effective privilege level
    const uint64_t prv_inst = prv;
    const uint64_t prv_data = mprv ? mpp : prv;

    // V2P mappings are enabled when all of the following are true:
    // - the effective execution mode is not 'M'
    // - satp.mode is not "Bare"
    bool vm_enabled = (((macc == Mem_Access_Fetch) ? prv_inst : prv_data) < CSR_PRV_M) && (satp_mode != SATP_MODE_BARE);

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
            sign = (int64_t(addr) >> 38);
            break;
        case SATP_MODE_SV48:
            Num_Levels = 4;
            PTE_top_Idx_Size = 17;
            // bits 63-48 of address must be equal to bit 47
            sign = (int64_t(addr) >> 47);
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
        level--;
        if (level < 0)
        {
            throw_page_fault(addr, macc);
        }

        // Take VPN[level]
        uint64_t vpn = (addr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
        // Read PTE
        pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
        // TODO: PMA / PMP checks
        pte = pmemread64(pte_addr);

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
            if (!(pte_r || (mxr && pte_x)) ||
                ((prv_data == CSR_PRV_U) && !pte_u) ||
                ((prv_data == CSR_PRV_S) && pte_u && !sum))
            {
                throw_page_fault(addr, macc);
            }
            break;
        case Mem_Access_Store:
            if (!pte_w ||
                ((prv_data == CSR_PRV_U) && !pte_u) ||
                ((prv_data == CSR_PRV_S) && pte_u && !sum))
            {
                throw_page_fault(addr, macc);
            }
            break;
        case Mem_Access_Fetch:
            if (!pte_x ||
                ((prv_inst == CSR_PRV_U) && !pte_u) ||
                ((prv_inst == CSR_PRV_S) && pte_u))
            {
                throw_page_fault(addr, macc);
            }
            break;
    }

    // Check if it is a misaligned superpage
    if ((level > 0) && ((ppn & ((1<<(PTE_Idx_Size*level))-1)) != 0))
    {
        throw_page_fault(addr, macc);
    }

    // Hardware A/D bit updates (FIXME: This should be done by SW)
    if (!pte_a || ((macc == Mem_Access_Store) && !pte_d))
    {
        // Set pte.a to 1 and, if the memory access is a store, also set pte.d to 1
        uint64_t pte_write = pte;
        pte_write |= uint64_t(1) << PTE_A_OFFSET;
        if (macc == Mem_Access_Store)
            pte_write |= uint64_t(1) << PTE_D_OFFSET;

        // Write PTE
        pmemwrite64(pte_addr, pte_write);
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

    LOG(DEBUG, "Physical address = 0x%016" PRIx64, paddr);

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
    uint64_t mstatus = csrget(csr_mstatus);
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 22) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    LOG(DEBUG, "I: sret%s%s", (comm?" # ":""), (comm?comm:""));
    logpcchange(csrget(csr_sepc));
    // Take spie and spp
    uint64_t spie = (mstatus >> 5) & 0x1;
    uint64_t spp = (mstatus >> 8) & 0x1;
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set sie = spie, spie = 1, spp = U (0), prv = spp
    csrset(csr_mstatus, mstatus_clean | (spie << 1) | (1 << 8));
    csrset(csr_prv, spp);
    LOG(DEBUG, "Now running in %s mode", (spp == CSR_PRV_M) ? "M" : (spp == CSR_PRV_S) ? "S" : "U");
}

void mret(const char* comm)
{
    LOG(DEBUG, "I: mret%s%s", (comm?" # ":""), (comm?comm:""));
    logpcchange(csrget(csr_mepc));
    // Take mpie and mpp
    uint64_t mstatus = csrget(csr_mstatus);
    uint64_t mpie = (mstatus >> 7) & 0x1;
    uint64_t mpp = (mstatus >> 11) & 0x3;
    // Clean mie, mpie and mpp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFE777ULL;
    // Set mie = mpie, mpie = 1, mpp = U (0), prv = mpp
    csrset(csr_mstatus, mstatus_clean | (mpie << 3) | (1 << 7));
    csrset(csr_prv, mpp);
    LOG(DEBUG, "Now running in %s mode", (mpp == CSR_PRV_M) ? "M" : (mpp == CSR_PRV_S) ? "S" : "U");
}

void wfi(const char* comm)
{
    uint64_t curprv = prvget();
    uint64_t mstatus = csrget(csr_mstatus);
    if (curprv == CSR_PRV_U || (curprv == CSR_PRV_S && (((mstatus >> 21) & 1) == 1)))
      throw trap_illegal_instruction(current_inst);

    LOG(DEBUG, "I: wfi%s%s", (comm?" # ":""), (comm?comm:""));
}

// TODO
void sfence_vma(xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: sfence_vma x%d, x%d%s%s", src1, src2, (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);
}

void csrrw(xreg dst, csr src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrw x%d, csrreg[%s], x%d%s%s", dst, csr_names[src1], src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = XREGS[src2].x;
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrs(xreg dst, csr src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrs x%d, csrreg[%s], x%d%s%s", dst, csr_names[src1], src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | XREGS[src2].x;
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrc(xreg dst, csr src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: csrrc x%d, csrreg[%s], x%d%s%s", dst, csr_names[src1], src2, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~XREGS[src2].x);
    csr_insn(dst, src1, oldval, newval, src2 != x0);
}

void csrrwi(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrwi x%d, csrreg[%s], 0x%016" PRIx64 "%s%s", dst, csr_names[src1], imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = imm;
    csr_insn(dst, src1, oldval, newval, true);
}

void csrrsi(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrsi x%d, csrreg[%s], 0x%016" PRIx64 "%s%s", dst, csr_names[src1], imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval | imm;
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

void csrrci(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    LOG(DEBUG, "I: csrrci x%d, csrreg[%s], 0x%016" PRIx64 "%s%s", dst, csr_names[src1], imm, (comm?" # ":""), (comm?comm:""));
    uint64_t oldval = csrget(src1);
    uint64_t newval = oldval & (~imm);
    csr_insn(dst, src1, oldval, newval, imm != 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64F emulation
//
////////////////////////////////////////////////////////////////////////////////

static void femuld(int count, freg dst, int64_t off, xreg base, int use_mask)
{
    assert(count <= VL);

    uint64_t base_addr = XREGS[base].x + off;
    for (int i = 0; i < count; i++)
    {
        if (use_mask && MREGS[0].b[i] == 0) continue;
        uint64_t addr = base_addr + i * 4;
        uint32_t val = vmemread32(addr);
        FREGS[dst].u[i] = val;
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- MEM[0x%016" PRIx64 "]", i, val, cast_uint32_to_float(val), addr);
    }
    ZERO_UNUSED_FREG_BITS(dst, count);
    dirty_fp_state();
    logfregchange(dst);
}

static void femust(int count, freg src1, int64_t off, xreg base, int use_mask)
{
    assert(count <= VL);

    uint64_t base_addr = XREGS[base].x + off;
    for (int i = 0; i < count; i++)
    {
        if (use_mask && MREGS[0].b[i] == 0) continue;
        uint64_t addr = base_addr + i * 4;
        uint32_t val = FREGS[src1].u[i];
        LOG(DEBUG, "\t[%d] 0x%08x (%g) --> MEM[0x%016" PRIx64 "]", i, val, cast_uint32_to_float(val), addr);
        vmemwrite32(addr, val);
        logmemwchange(i, 4, addr, val);
    }
}

static void femucvtf2x(opcode opc, xreg dst, freg src1, rounding_mode rm)
{
    iufval32 val;
    u32_i32_u64_i64 res;

    set_rounding_mode(rm);
    clear_arithmetic_flags();
    val.u = FREGS[src1].u[0];
    switch (opc)
    {
        case FCVTWS:
            res.i = fpu::f32_to_i32(val.f);
            LOG(DEBUG, "\t0x%08x (%d) <-- 0x%08x (%g)", res.u, res.i, val.u, val.flt);
            break;
        case FCVTWUS:
            res.u = fpu::f32_to_ui32(val.f);
            LOG(DEBUG, "\t0x%08x (%u) <-- 0x%08x (%g)", res.u, res.u, val.u, val.flt);
            break;
        case FCVTLS:
            res.l = fpu::f32_to_i64(val.f);
            LOG(DEBUG, "\t0x%08x (%d) <-- 0x%08x (%g)", res.u, res.i, val.u, val.flt);
            break;
        case FCVTLUS:
            res.lu = fpu::f32_to_ui64(val.f);
            LOG(DEBUG, "\t0x%08x (%u) <-- 0x%08x (%g)", res.u, res.u, val.u, val.flt);
            break;
        default:
            assert(0);
            break;
    }
    set_fp_exceptions();
    if (dst != x0)
        XREGS[dst].x = sext32(res.u);
    logxregchange(dst);
}

static void femucvtx2f(opcode opc, freg dst, xreg src1, rounding_mode rm)
{
    iufval32 res;
    u32_i32_u64_i64 val;

    set_rounding_mode(rm);
    clear_arithmetic_flags();

    switch (opc)
    {
        case FCVTSW:
            val.u = XREGS[src1].w[0];
            res.f = fpu::i32_to_f32(val.i);
            LOG(DEBUG, "\t0x%08x (%g) <-- 0x%08x (%d)", res.u, res.flt, val.u, val.i);
            break;
        case FCVTSWU:
            val.u = XREGS[src1].w[0];
            res.f = fpu::ui32_to_f32(val.u);
            LOG(DEBUG, "\t0x%08x (%g) <-- 0x%08x (%u)", res.u, res.flt, val.u, val.u);
            break;
        case FCVTSL:
            val.lu = XREGS[src1].x;
            res.f = fpu::i64_to_f32(val.l);
            LOG(DEBUG, "\t0x%08x (%g) <-- 0x%016" PRIx64 " (%" PRId64 ")", res.u, res.flt, val.lu, val.l);
            break;
        case FCVTSLU:
            val.lu = XREGS[src1].x;
            res.f = fpu::ui64_to_f32(val.lu);
            LOG(DEBUG, "\t0x%08x (%g) <-- 0x%016" PRIx64 " (%" PRIu64 ")", res.u, res.flt, val.lu, val.lu);
            break;
        default:
            assert(0);
            break;
    }
    FREGS[dst].u[0] = res.u;
    ZERO_UNUSED_FREG_BITS(dst, 1);
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

static void femu1src(opcode opc, int count, freg dst, freg src1, rounding_mode rm)
{
    assert(count <= VL);

    set_rounding_mode(rm);
    clear_arithmetic_flags();

    for (int i = 0; i < count; ++i)
    {
        if (count == VL && MREGS[0].b[i] == 0) continue;

        iufval32 val, res;
        val.u = FREGS[src1].u[i];
        switch (opc)
        {
            case FSQRT:
                {
                    res.f = fpu::f32_sqrt(val.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FRSQ:
                {
                    iufval32 res_gold;
                    res_gold.flt = gold_frsq(val.flt);
                    res.f = fpu::f32_rsqrt(val.f);
                    // security ulp check
                    if (security_ulp_check(res_gold.u,res.u))
                    {
                        LOG(DEBUG, "RSQ TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                        LOG(DEBUG, "WARNING. Don't panic. Trans mismatch error for operation RSQ with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u);
                    }
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FSIN:
                {
                    iufval32 res_gold;
                    res_gold.flt = gold_fsin(val.flt);
                    res.f = fpu::f32_sin2pi(val.f);
                    // security ulp check

                    LOG(DEBUG, "SIN TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                    if (security_ulp_check(res_gold.u,res.u))
                    {
                        LOG(DEBUG, "SIN TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                        LOG(DEBUG, "WARNING. Don't panic. Trans mismatch error for operation FSIN with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u);
                    }
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FEXP:
                {
                    iufval32 res_gold;
                    res_gold.flt = gold_fexp(val.flt);
                    res.f = fpu::f32_exp2(val.f);
                    // security ulp check
                    if (security_ulp_check(res_gold.u,res.u))
                    {
                        LOG(DEBUG, "EXP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                        LOG(DEBUG, "WARNING. Don't panic. Trans mismatch error for operation FEXP with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u);
                    }
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FLOG:
                {
                    iufval32 res_gold;
                    res_gold.flt = gold_flog(val.flt);
                    res.f = fpu::f32_log2(val.f);
                    // security ulp check
                    if (security_ulp_check(res_gold.u,res.u))
                    {
                        LOG(DEBUG, "LOG TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                        LOG(DEBUG, "WARNING. Don't panic. Trans mismatch error for operation FLOG with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u);
                    }
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FRCP:
                {
                    iufval32 res_gold;
                    res_gold.flt = gold_frcp(val.flt);
                    res.f = fpu::f32_rcp(val.f);
                    // security ulp check
                    if (security_ulp_check(res_gold.u,res.u))
                    {
                        LOG(DEBUG, "RCP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x", val.u, res.u, res_gold.u);
                        LOG(DEBUG, "WARNING. Don't panic. Trans mismatch error for operation FRCP with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u);
                    }
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            case FRCPFXP:
                // FIXME: THIS INSTRUCTION IS OBSOLETE
                {
                    // Input value is 2xtriArea with 15.16 precision
                    double tmp = double(val.i) / double(1 << 16);

                    // Result value is 17.14
                    double tmp_rcp = (1.0 / tmp) * double(1 << 14);

                    res.i = int32_t(tmp_rcp);
                    LOG(DEBUG, "\t[%d] 0x%08x (%d) <-- 1 / 0x%08x (%d)", i, res.u, res.i, val.u, val.i);
                }
                break;
            case FCVTPSPW:
                {
                    res.f = fpu::i32_to_f32(val.i);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%d)", i, res.u, res.flt, val.u, val.i);
                }
                break;
            case FCVTPSRAST:
                {
                    res.f = fpu::fxp1516_to_f32(val.i);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%d)", i, res.u, res.flt, val.u, val.i);
                }
                break;
            case FCVTRASTPS:
                {
                    res.i = fpu::f32_to_fxp1714(val.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%d) <-- 0x%08x (%g)", i, res.u, res.i, val.u, val.flt);
                }
                break;
            case FCVTPSPWU:
                {
                    res.f = fpu::ui32_to_f32(val.u);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%u)", i, res.u, res.flt, val.u, val.u);
                }
                break;
            case FCVTPWPS:
                {
                    res.i = fpu::f32_to_i32(val.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%d) <-- 0x%08x (%g)", i, res.u, res.i, val.u, val.flt);
                }
                break;
            case FCVTPWUPS:
                {
                    res.u = fpu::f32_to_ui32(val.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%u) <-- 0x%08x (%g)", i, res.u, res.u, val.u, val.flt);
                }
                break;
            case FFRC:
                {
                    res.f = fpu::f32_frac(val.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g)", i, res.u, res.flt, val.u, val.flt);
                }
                break;
            default:
                assert(0);
                break;
        }
        FREGS[dst].u[i] = res.u;
    }
    ZERO_UNUSED_FREG_BITS(dst, count);
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

static void femu2src(opcode opc, int count, freg dst, freg src1, freg src2, rounding_mode rm)
{
    assert(count <= VL);

    set_rounding_mode(rm);
    clear_arithmetic_flags();

    for (int i = 0; i < count; i++)
    {
        if (count == VL && MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = (src2 != fnone) ? FREGS[src2].u[i] : 0;
        switch (opc)
        {
            case FADD:
                {
                    res.f = fpu::f32_add(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) + 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FSUB:
                {
                    res.f = fpu::f32_sub(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) - 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FMUL:
                {
                    res.f = fpu::f32_mul(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FDIV:
                {
                    res.f = fpu::f32_div(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) / 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FMIN:
                {
                    res.f = fpu::f32_minNum(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- min(0x%08x (%g), 0x%08x (%g))", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FMAX:
                {
                    res.f = fpu::f32_maxNum(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- max(0x%08x (%g), 0x%08x (%g))", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FLT:
                {
                    res.u = fpu::f32_lt(val1.f, val2.f) ? 0xffffffff : 0;
                    LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (%g) < 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FLE:
                {
                    res.u = fpu::f32_le(val1.f, val2.f) ? 0xffffffff : 0;
                    LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (%g) <= 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FEQ:
                {
                    res.u = fpu::f32_eq(val1.f, val2.f) ? 0xffffffff : 0;
                    LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (%g) == 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FSGNJ:
                {
                    res.f = fpu::f32_copySign(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g), 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FSGNJN:
                {
                    res.f = fpu::f32_copySignNot(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g), 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FSGNJX:
                {
                    res.f = fpu::f32_copySignXor(val1.f, val2.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g), 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
                }
                break;
            case FRCP_FIX_RAST:
                {
                    res.i = fpu::fxp1714_rcpStep(val1.i, val2.i);

                    LOG(DEBUG, "\t[%d] 0x%08x (%d) <-- 0x%08x (%d), 0x%08x (%d)", i, res.u, res.i, val1.u, val1.i, val2.u, val2.i);

                    //Check 1ulp
                    iufval32 res_gold;
                    res_gold.i = gold_frcp_fix_rast(val1.i, val2.i);
                    if (abs(res.i - res_gold.i) > 1)
                    {
                        LOG(DEBUG, "\t\tEXPECTED: 0x%08x (%g) RESULT: 0x%08x (%g)", res_gold.u, res_gold.flt, res.u, res.flt);
                        assert(0 && "Trans mismatch error. Please open jira to jordi.sola@esperantotech.com.");
                    }
                }
                break;
            default:
                assert(0);
                break;
        }
        FREGS[dst].u[i] = res.u;
    }
    ZERO_UNUSED_FREG_BITS(dst, count);
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

static void femu3src(opcode opc, int count, freg dst, freg src1, freg src2, freg src3, rounding_mode rm)
{
    assert(count <= VL);

    set_rounding_mode(rm);
    clear_arithmetic_flags();

    for (int i = 0; i < count; i++)
    {
        if (count == VL && MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, val3, res;

        val1.u = FREGS[src1].u[i];
        val2.u = FREGS[src2].u[i];
        val3.u = FREGS[src3].u[i];
        switch (opc)
        {
            case FMADD:
                {
                    res.f = fpu::f32_mulAdd(val1.f, val2.f, val3.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g) + 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt, val3.u, val3.flt);
                }
                break;
            case FNMADD:
                {
                    res.f = fpu::f32_negMulAdd(val1.f, val2.f, val3.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- -(0x%08x (%g) * 0x%08x (%g) + 0x%08x (%g))", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt, val3.u, val3.flt);
                }
                break;
            case FMSUB:
                {
                    res.f = fpu::f32_mulSub(val1.f, val2.f, val3.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g) - 0x%08x (%g)", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt, val3.u, val3.flt);
                }
                break;
            case FNMSUB:
                {
                    res.f = fpu::f32_negMulSub(val1.f, val2.f, val3.f);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- -(0x%08x (%g) * 0x%08x (%g) - 0x%08x (%g))", i, res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt, val3.u, val3.flt);
                }
                break;
            case FCMOV:
                {
                    res.u = (val1.u ? val2.u : val3.u);
                    LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- %u ? 0x%08x (%g) : 0x%08x (%g)", i, res.u, res.flt, val1.u, val2.u, val2.flt, val3.u, val3.flt);
                }
                break;
            default:
                assert(0);
                break;
        }
        FREGS[dst].u[i] = res.u;
    }
    ZERO_UNUSED_FREG_BITS(dst, count);
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

static void femucmp(opcode opc, xreg dst, freg src1, freg src2)
{
    iufval32 val1, val2, res;

    clear_arithmetic_flags();
    val1.u = FREGS[src1].u[0];
    val2.u = FREGS[src2].u[0];
    switch (opc)
    {
        case FLT:
            res.u = fpu::f32_lt(val1.f, val2.f) ? 1 : 0;
            LOG(DEBUG, "\t0x%08x <-- 0x%08x (%g) < 0x%08x (%g)?", res.u, val1.u, val1.flt, val2.u, val2.flt);
            break;
        case FLE:
            res.u = fpu::f32_le(val1.f, val2.f) ? 1 : 0;
            LOG(DEBUG, "\t0x%08x <-- 0x%08x (%g) <= 0x%08x (%g)?", res.u, val1.u, val1.flt, val2.u, val2.flt);
            break;
        case FEQ:
            res.u = fpu::f32_eq(val1.f, val2.f) ? 1 : 0;
            LOG(DEBUG, "\t0x%08x <-- 0x%08x (%g) == 0x%08x (%g)?", res.u, val1.u, val1.flt, val2.u, val2.flt);
            break;
        default:
            assert(0);
            break;
    }
    set_fp_exceptions();
    if (dst != x0)
        XREGS[dst].x = sext32(res.u);
    logxregchange(dst);
}

void fadd_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fadd.s f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FADD, 1, dst, src1, src2, rm);
}

void fsub_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fsub.s f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FSUB, 1, dst, src1, src2, rm);
}

void fmul_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmul.s f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FMUL, 1, dst, src1, src2, rm);
}

void fdiv_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fdiv.s f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femu2src(FDIV, 1, dst, src1, src2, rm);
}

void fsgnj_s(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnj.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FSGNJ, 1, dst, src1, src2, rmdyn);
}

void fsgnjn_s(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnjn.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FSGNJN, 1, dst, src1, src2, rmdyn);
}

void fsgnjx_s(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnjx.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FSGNJX, 1, dst, src1, src2, rmdyn);
}

void fmin_s(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmin.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FMIN, 1, dst, src1, src2, rmdyn);
}

void fmax_s(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmax.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu2src(FMAX, 1, dst, src1, src2, rmdyn);
}

void fsqrt_s(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fsqrt.s f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femu1src(FSQRT, 1, dst, src1, rm);
}

void feq_s(xreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: feq.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucmp(FEQ, dst, src1, src2);
}

void fle_s(xreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fle.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucmp(FLE, dst, src1, src2);
}

void flt_s(xreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: flt.s f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucmp(FLT, dst, src1, src2);
}

void fcvt_w_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.w.s x%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucvtf2x(FCVTWS, dst, src1, rm);
}

void fcvt_wu_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.wu.s x%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucvtf2x(FCVTWUS, dst, src1, rm);
}

void fcvt_l_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.l.s x%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femucvtf2x(FCVTLS, dst, src1, rm);
}

void fcvt_lu_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.lu.s x%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femucvtf2x(FCVTLUS, dst, src1, rm);
}

void fmv_x_w(xreg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fmv.x.w x%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    if (dst != x0)
    {
        XREGS[dst].x = sext32(FREGS[src1].u[0]);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%08x (%g)", XREGS[dst].x, FREGS[src1].u[0], cast_uint32_to_float(FREGS[src1].u[0]));
    }
    logxregchange(dst);
}

void fclass_s(xreg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fclass.s x%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    iufval32 val, res;
    val.u = FREGS[src1].u[0];
    res.u = fpu::f32_classify(val.f);
    if (dst != x0)
    {
        XREGS[dst].x = sext32(res.u);
        LOG(DEBUG, "\t0x%08x <-- 0x%08x (%g)", res.u, val.u, val.flt);
    }
    logxregchange(dst);
}

void fcvt_s_w(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.s.w f%d, x%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucvtx2f(FCVTSW, dst, src1, rm);
}

void fcvt_s_wu(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.s.wu f%d, x%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femucvtx2f(FCVTSWU, dst, src1, rm);
}

void fcvt_s_l(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.s.l f%d, x%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femucvtx2f(FCVTSL, dst, src1, rm);
}

void fcvt_s_lu(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.s.lu f%d, x%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    femucvtx2f(FCVTSLU, dst, src1, rm);
}

void fmv_w_x(freg dst, xreg src1, const char* comm)
{
    LOG(DEBUG, "I: fmv.w.x f%d, x%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    iufval32 val, res;
    val.u = XREGS[src1].w[0];
    res.u = val.u;
    FREGS[dst].u[0] = res.u;
    LOG(DEBUG, "\t0x%08x (%g) <-- 0x%08x", res.u, res.flt, val.u);
    ZERO_UNUSED_FREG_BITS(dst, 1);
    dirty_fp_state();
    logfregchange(dst);
}

void flw(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: flw f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femuld(1, dst, off,  base, 0);
}

void fsw(freg src1, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsw f%d, %" PRId64 "(x%d)%s%s", src1, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femust(1, src1, off, base, 0);
}

void fmadd_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmadd.s f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu3src(FMADD, 1, dst, src1, src2, src3, rm);
}

void fmsub_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmsub.s f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu3src(FMSUB, 1, dst, src1, src2, src3, rm);
}

void fnmsub_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fnmsub.s f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu3src(FNMSUB, 1, dst, src1, src2, src3, rm);
}

void fnmadd_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fnmadd.s f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    femu3src(FNMADD, 1, dst, src1, src2, src3, rm);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto mask extension emulation
//
////////////////////////////////////////////////////////////////////////////////

static void maskop(opcode opc, mreg dst, mreg src1, mreg src2)
{
    for (int i = 0; i < VL; i++)
    {
        uint8_t val1  = MREGS[src1].b[i];
        uint8_t val2  = (src2 == mnone) ? 0 : MREGS[src2].b[i];

        uint8_t res;
        switch (opc)
        {
            case MAND:   res = (val1 & val2) & 0x1;
                         LOG(DEBUG, "\t[%d] %d <-- %d & %d", i, res, val1, val2);
                         break;
            case MOR:    res = (val1 | val2) & 0x1;
                         LOG(DEBUG, "\t[%d] %d <-- %d | %d", i, res, val1, val2);
                         break;
            case MXOR:   res = (val1 ^ val2) & 0x1;
                         LOG(DEBUG, "\t[%d] %d <-- %d ^ %d", i, res, val1, val2);
                         break;
            case MNOT:   res = (~val1) & 0x1;
                         LOG(DEBUG, "\t[%d] %d <-- ~%d", i, res, val1);
                         break;
            default:     assert(0);
                         break;

        }
        MREGS[dst].b[i] = res;
    }

    logmregchange(dst);
}

void maskand(mreg dst, mreg src1, mreg src2, const char* comm)
{
    LOG(DEBUG, "I: maskand m%d, m%d, m%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    DEBUG_MASK(MREGS[0]);
    maskop(MAND, dst, src1, src2);
}

void maskor(mreg dst, mreg src1, mreg src2, const char* comm)
{
    LOG(DEBUG, "I: maskor m%d, m%d, m%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    DEBUG_MASK(MREGS[0]);
    maskop(MOR, dst, src1, src2);
}

void maskxor(mreg dst, mreg src1, mreg src2, const char* comm)
{
    LOG(DEBUG, "I: maskxor m%d, m%d, m%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    DEBUG_MASK(MREGS[0]);
    maskop(MXOR, dst, src1, src2);
}

void masknot(mreg dst, mreg src1, const char* comm)
{
    LOG(DEBUG, "I: masknot m%d, m%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    DEBUG_MASK(MREGS[0]);
    maskop(MNOT, dst, src1, mnone);
}

void mova_x_m(xreg dst, const char* comm)
{
    LOG(DEBUG, "I: mova.x.m x%d, allmasks%s%s", dst, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = 0;
        for (int m = 7; m >= 0; m--)
        {
            val <<= VL;
            for (int i = 0; i < VL; i++)
            {
                val |= (MREGS[m].b[i] & 0x1) << i;
            }
            LOG(DEBUG, "\taccumulating into 0x%016" PRIx64 " reg m%d = 0x%016" PRIx64, val, m, (val&((1u<<VL)-1)));
        }
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void mova_m_x(xreg src1, const char* comm)
{
    LOG(DEBUG, "I: mova.m.x allmasks, x%d%s%s", src1, (comm?" # ":""), (comm?comm:""));

    uint64_t val = XREGS[src1].x;

    LOG(DEBUG, "\tallmasks <-- 0x%016" PRIx64, val);
    for (int m = 0; m < 8; m++)
    {
        for (int i = 0; i < VL; i++)
        {
            MREGS[m].b[i] = (val >> i) & 0x1;
            LOG(DEBUG, "\tm%d.b[%d] = 0x%x", m, i, MREGS[m].b[i]);
        }
        val >>= VL;
        logmregchange(m);
    }
}

void mov_m_x(mreg dst, xreg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: mov.m.x m%d, x%d, 0x%02x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    DEBUG_MASK(MREGS[0]);

    uint32_t val = XREGS[src1].w[0] | (imm & ((1u<<VL) - 1u));
    for (int i = 0; i < VL; i++)
    {
        MREGS[dst].b[i] = (val >> i) & 0x1;
        LOG(DEBUG, "\tm%d.b[%d] = 0x%x  (from val=0x%08x)", dst, i, MREGS[dst].b[i], val);
    }
    logmregchange(dst);
}

void maskpopc(xreg dst, mreg src1, const char* comm)
{
    LOG(DEBUG, "I: maskpopc x%d, m%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    uint64_t count = 0;
    for (int i = 0; i < VL; i++)
    {
        count += (MREGS[src1].b[i] ? 1 : 0);
        LOG(DEBUG, "\tcount = %ld from m%d.b[%d] = %d", count, src1, i, MREGS[src1].b[i]);
    }
    if (dst != x0)
        XREGS[dst].x = count;
    logxregchange(dst);
}

void maskpopcz(xreg dst, mreg src1, const char* comm)
{
    LOG(DEBUG, "I: maskpopcz x%d, m%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    uint64_t count = 0;
    for (int i = 0; i < VL; i++)
    {
        count += (MREGS[src1].b[i] ? 0 : 1);
        LOG(DEBUG, "\tcount = %ld from m%d.b[%d] = %d ", count, src1, i, MREGS[src1].b[i]);
    }
    if (dst != x0)
        XREGS[dst].x = count;
    logxregchange(dst);
}

void maskpopc_rast(xreg dst, mreg src1, mreg src2, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: maskpopc.rast x%d, m%d, m%d, %d%s%s", dst, src1, src2, imm, (comm?" # ":""), (comm?comm:""));
    uint64_t count = 0;

    uint32_t mask;
    switch(imm)
    {
        case 0  : mask = 0x0f0f; break;
        case 1  : mask = 0x3c3c; break;
        case 2  : mask = 0xf0f0; break;
        default : mask = 0xffff; break;
    }

    for (int i = 0; i < VL; i++)
    {
        count += ((MREGS[src1].b[i] & (mask & 0x1)) ? 1 : 0);
        LOG(DEBUG, "\tcount = %ld from m%d.b[%d] = %d m = %d ", count, src1, i, MREGS[src1].b[i], mask & 0x1);
        mask = mask >> 1;
    }

    for (int i = 0; i < VL; i++)
    {
        count += ((MREGS[src2].b[i] & (mask & 0x1)) ? 1 : 0);
        LOG(DEBUG, "\tcount = %ld from m%d.b[%d] = %d m = %d ", count, src2, i, MREGS[src2].b[i], mask & 0x1);
        mask = mask >> 1;
    }

    if (dst != x0)
        XREGS[dst].x = count;
    logxregchange(dst);
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
    uint64_t addr = XREGS[base].x + off;
    if ((addr % 4) != 0) throw trap_load_address_misaligned(addr);
    femuld(VL, dst, off,  base, 0);
}

void flw_ps(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: flw.ps f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuld(VL, dst, off,  base, 1);
}

void flwl_ps(freg dst, xreg base, const char* comm)
{
    LOG(DEBUG, "I: flwl.ps f%d, (x%d)%s%s", dst, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuld(VL, dst, 0,  base, 1);
}

void flwg_ps(freg dst, xreg base, const char* comm)
{
    LOG(DEBUG, "I: flwg.ps f%d, (x%d)%s%s", dst, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuld(VL, dst, 0,  base, 1);
}

void fsq2(freg src, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsq2 f%d, %" PRId64 "(x%d)%s%s", src, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    uint64_t addr = XREGS[base].x + off;
    if ((addr % 4) != 0) throw trap_store_address_misaligned(addr);
    femust(VL, src, off, base, 0);
}

void fsw_ps(freg src, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fsw.ps f%d, %" PRId64 "(x%d)%s%s", src, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femust(VL, src, off, base, 1);
}

void fswl_ps(freg src, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fswl.ps f%d, (x%d)%s%s", src, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femust(VL, src, 0, base, 1);
}

void fswg_ps(freg src, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fswg.ps f%d, (x%d)%s%s", src, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femust(VL, src, 0, base, 1);
}

// ----- Broadcast -----------------------------------------

void fbc_ps(freg dst, xreg base, int64_t off, const char* comm)
{
    LOG(DEBUG, "I: fbc_ps f%d, %" PRId64 "(x%d)%s%s", dst, off, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    iufval32 val;
    uint64_t addr = XREGS[base].x + off;
    val.u = 0;
    uint8_t  b = 0;
    for (int i = 0; i < VL; i++)
    {
        b |= MREGS[0].b[i];
    }
    if (b != 0)
    {
        val.u = vmemread32(addr);
    }
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i])
        {
            FREGS[dst].u[i] = val.u;
            LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- MEM[0x%016" PRIx64 "]", i, val.u, val.flt, addr);
        }
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fbci_ps(freg dst, uint32_t imm, const char* comm)
{
    LOG(DEBUG, "I: fbci_ps f%d, 0x%08x%s%s", dst, (imm&0xfffff), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    iufval32 val;
    val.u = imm;
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i])
        {
            FREGS[dst].u[i] = val.u;
            LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x", i, val.u, val.flt, imm);
        }
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fbcx_ps(freg dst, xreg src, const char* comm)
{
    LOG(DEBUG, "I: fbcx_ps f%d, x%d%s%s", dst, src, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    iufval32 val;
    val.u = XREGS[src].w[0];
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i])
        {
            FREGS[dst].u[i] = val.u;
            LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x", i, val.u, val.flt, val.u);
        }
    }
    dirty_fp_state();
    logfregchange(dst);
}

// ----- Gather and scatter --------------------------------

static void gatheremu(opcode opc, int size, freg dst, freg src1, xreg base)
{
    uint64_t baddr = XREGS[base].x;
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val;
        int32_t off   = FREGS[src1].i[i];
        uint64_t addr = baddr + off;
        switch (opc)
        {
            case FGW: val.u = vmemread32(addr); break;
            case FGH: val.u = sext16(vmemread16(addr)); break;
            case FGBL:
            case FGBG:
            case FGB: val.u = sext8(vmemread8(addr)); break;
            case FGWL:
            case FGWG:
                if ((addr % size) != 0)
                    throw trap_load_address_misaligned(addr);
                val.u = vmemread32(addr);
                break;
            case FGHL:
            case FGHG:
                if ((addr % size) != 0)
                    throw trap_load_address_misaligned(addr);
                val.u = sext16(vmemread16(addr));
                break;
            default : assert(0); break;
        }
        FREGS[dst].u[i] = val.u;
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 " = 0x%016" PRIx64 "]", i, val.u, val.flt, baddr, int64_t(off), addr);
    }
    dirty_fp_state();
    logfregchange(dst);
}

static void gatheremu32(opcode opc, int size, freg dst, xreg src1, xreg src2)
{
    uint64_t baddr = XREGS[src2].x;
    uint64_t index = XREGS[src1].x;
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        uint64_t off;
        uint64_t addr;
        switch(size)
        {
            case 1 : off =  (index >> (i * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
            case 2 : off = ((index >> (i * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
            case 4 : off = ((index >> (i * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
            default: assert(0); break;
        }

        iufval32 val;
        switch (size)
        {
            case 1 : val.u = sext8(vmemread8(addr));  break;
            case 2 : val.u = sext16(vmemread16(addr)); break;
            case 4 : val.u = vmemread32(addr); break;
            default: assert(0); break;
        }
        FREGS[dst].u[i] = val.u;
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- MEM[0x%016" PRIx64 " + 0x%016" PRIx64 " = 0x%016" PRIx64 "]", i, val.u, val.flt, baddr, int64_t(off), addr);
    }
    dirty_fp_state();
    logfregchange(dst);
}

static void femuscat(opcode opc, freg src1, freg src2, xreg base)
{
    uint64_t baddr = XREGS[base].x;
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        int32_t  off  = FREGS[src2].i[i];
        uint64_t addr = baddr + off;
        iufval32 val;
        val.u = FREGS[src1].u[i];

        switch (opc)
        {
            case FSCW : vmemwrite32(addr, val.u); logmemwchange(i, 4, addr, val.u); break;
            case FSCH : vmemwrite16(addr, uint16_t(val.u)); logmemwchange(i, 2, addr, val.u); break;
            case FSCBL :
            case FSCBG :
            case FSCB : vmemwrite8(addr, uint8_t(val.u));  logmemwchange(i, 1, addr, val.u); break;
            case FSCWL :
            case FSCWG :
                if ((addr % 4) != 0)
                    throw trap_load_address_misaligned(addr);
                vmemwrite32(addr, val.u);
                logmemwchange(i, 4, addr, val.u);
                break;
            case FSCHL :
            case FSCHG :
                if ((addr % 2) != 0)
                    throw trap_load_address_misaligned(addr);
                vmemwrite16(addr, uint16_t(val.u));
                logmemwchange(i, 2, addr, val.u);
                break;
            default   : assert(0); break;
        }

        // Scatter writes are not logged!!!!
        LOG(DEBUG, "\t[%d] 0x%08x (%g) --> MEM[0x%016" PRIx64 " + 0x%08" PRIx32 " = 0x%016" PRIx64 "]", i, val.u, val.flt, baddr, off, addr);
    }
}

static void femuscat32(opcode opc, int size, freg src3, xreg src1, xreg src2)
{
    uint64_t baddr = XREGS[src2].x;
    uint64_t index = XREGS[src1].x;
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        uint64_t off;
        uint64_t addr;
        switch(size)
        {
            case 1 : off =  (index >> (i * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
            case 2 : off = ((index >> (i * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
            case 4 : off = ((index >> (i * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
            default: assert(0); break;
        }

        iufval32 val;
        val.u = FREGS[src3].u[i];
        switch (size)
        {
            case 1 : vmemwrite8(addr, uint8_t(val.u)); break;
            case 2 : vmemwrite16(addr, uint16_t(val.u)); break;
            case 4 : vmemwrite32(addr, val.u); break;
            default: assert(0); break;
        }

        LOG(DEBUG, "\t[%d] 0x%08x (%g) --> MEM[0x%016" PRIx64 " + 0x%016" PRIx64 " = 0x%016" PRIx64 "]", i, val.u, val.flt, baddr, off, addr);

        // Do not track store swizzles?  Same with scatters.
        logmemwchange(i, size, addr, val.u);
    }
}

void fgb_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgb.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGB, 1, dst, src1, base);
}

void fgh_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgh.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGH, 2, dst, src1, base);
}

void fgw_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgw.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGW, 4, dst, src1, base);
}

void fgwl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgwl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGWL, 4, dst, src1, base);
}

void fghl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fghl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGHL, 2, dst, src1, base);
}

void fgbl_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgbl.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGBL, 1, dst, src1, base);
}

void fgwg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgwg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGWG, 4, dst, src1, base);
}

void fghg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fghg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGHG, 2, dst, src1, base);
}

void fgbg_ps(freg dst, freg src1, xreg base, const char* comm)
{
    LOG(DEBUG, "I: fgbg.ps f%d, f%d(x%d)%s%s", dst, src1, base, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    throw std::runtime_error("Unimplemented instruction (fgwg.ps)");
    gatheremu(FGBG, 1, dst, src1, base);
}

void fg32b_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32b.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32B, 1, dst, src1, src2);
}

void fg32h_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32h.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32H, 2, dst, src1, src2);
}

void fg32w_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fg32w.ps f%d, x%d(x%d)%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32W, 4, dst, src1, src2);
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
    femuscat32(FSC32B, 1, src, src1, src2);
}

void fsc32h_ps(freg src, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fsc32h.ps f%d, x%d(x%d)%s%s", src, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat32(FSC32H, 2, src, src1, src2);
}

void fsc32w_ps(freg src, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: fsc32w.ps f%d, x%d(x%d)%s%s", src, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femuscat32(FSC32W, 4, src, src1, src2);
}

// ----- Computational (follows RV64F) ---------------------

static void fmask(opcode opc, mreg dst, freg src1, freg src2)
{
    clear_arithmetic_flags();
    for (int i = 0; i < VL; i++)
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;

        val1.u = FREGS[src1].u[i];
        val2.u = (src2 != fnone) ? FREGS[src2].u[i] : 0;
        switch (opc)
        {
            case FLT:
                res.u = fpu::f32_lt(val1.f, val2.f) ? 1 : 0;
                LOG(DEBUG, "\t[%d] %d <-- 0x%08x (%g) < 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                break;
            case FLE:
                res.u = fpu::f32_le(val1.f, val2.f) ? 1 : 0;
                LOG(DEBUG, "\t[%d] %d <-- 0x%08x (%g) <= 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                break;
            case FEQ:
                res.u = fpu::f32_eq(val1.f, val2.f) ? 1 : 0;
                LOG(DEBUG, "\t[%d] %d <-- 0x%08x (%g) == 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                break;
            case FSET:
                // NB: should this be a !feq() comparison?
                // softfloat: res.u = !f32_eq(val1.f, {0});
                // hardfloat: res.u = (val1.f == 0.0) ? 0 : 1;
                res.u = (val1.u) ? 1 : 0;
                LOG(DEBUG, "\t[%d] %d <-- 0x%08x ? 1 : 0", i, res.u, val1.u);
                break;
            case FLTPI:
                res.u = (val1.i < val2.i) ? 1 : 0;
                LOG(DEBUG, "\t[%d] %d <-- 0x%08x (%g) < 0x%08x (%g)?", i, res.u, val1.u, val1.flt, val2.u, val2.flt);
                break;
            default:
                assert(0);
                break;
        }
        MREGS[dst].b[i] = res.u;
    }
    set_fp_exceptions();
    logmregchange(dst);
}

static void fswizz(freg dst, freg src1, uint8_t imm)
{
    fdata val = FREGS[src1];
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        int sel = (i & ~0x3) | ((imm >> ((2*i) % 8)) & 0x03);
        FREGS[dst].u[i] = val.u[sel];
        LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (chan %d)", i, FREGS[dst].u[i], val.u[sel], sel);
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fadd_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fadd.ps f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FADD, VL, dst, src1, src2, rm);
}

void fsub_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fsub.ps f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FSUB, VL, dst, src1, src2, rm);
}

void fmul_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmul.ps f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FMUL, VL, dst, src1, src2, rm);
}

void fdiv_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fdiv.ps f%d, f%d, f%d, %s%s%s", dst, src1, src2, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FDIV, VL, dst, src1, src2, rm);
}

void fsgnj_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnj.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJ, VL, dst, src1, src2, rmdyn);
}

void fsgnjn_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnjn.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJN, VL, dst, src1, src2, rmdyn);
}

void fsgnjx_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsgnjx.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJX, VL, dst, src1, src2, rmdyn);
}

void fmin_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmin.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FMIN, VL, dst, src1, src2, rmdyn);
}

void fmax_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmax.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FMAX, VL, dst, src1, src2, rmdyn);
}

void fsqrt_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fsqrt.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FSQRT, VL, dst, src1, rm);
}

void feq_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: feq.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FEQ, VL, dst, src1, src2, rmdyn);
}

void fle_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fle.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FLE, VL, dst, src1, src2, rmdyn);
}

void flt_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: flt.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FLT, VL, dst, src1, src2, rmdyn);
}

void feqm_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: feqm.ps m%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fmask(FEQ, dst, src1, src2);
}

void flem_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: flem.ps m%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fmask(FLE, dst, src1, src2);
}

void fltm_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fltm.ps m%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fmask(FLT, dst, src1, src2);
}

void fsetm_pi(mreg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fsetm.pi m%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fmask(FSET, dst, src1, fnone);
}

void fcmov_ps(freg dst, freg src1, freg src2, freg src3, const char* comm)
{
    LOG(DEBUG, "I: fcmov.ps f%d, f%d, f%d, f%d%s%s", dst, src1, src2, src3, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu3src(FCMOV, VL, dst, src1, src2, src3, rmdyn);
}

void fcmovm_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fcmovm.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();

    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        iufval32 val1, val2, res;
        val1.u  = FREGS[src1].u[i];
        val2.u  = FREGS[src2].u[i];
        int sel = MREGS[0].b[i];
        res.u   = sel ? val1.u : val2.u;
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- %d ? 0x%08x (%g) : 0x%08x (%g)", i, res.u, res.flt, sel, val1.u, val1.flt, val2.u, val2.flt);
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fmvz_x_ps(xreg dst, freg src1, unsigned index, const char* comm)
{
    LOG(DEBUG, "I: fmvz.x.ps x%d, f%d, %d%s%s", dst, src1, index, (comm?" # ":""), (comm?comm:""));
    require_fp_active();

    index = index % VL;
    if (dst != x0)
        XREGS[dst].x = FREGS[src1].u[index];

    LOG(DEBUG, "\t 0x%016" PRIx64 " <-- {0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x}", XREGS[dst].x,
        FREGS[src1].u[0], FREGS[src1].u[1], FREGS[src1].u[2], FREGS[src1].u[3],
        FREGS[src1].u[4], FREGS[src1].u[5], FREGS[src1].u[6], FREGS[src1].u[7]);

    logxregchange(dst);
}

void fmvs_x_ps(xreg dst, freg src1, unsigned index, const char* comm)
{
    LOG(DEBUG, "I: fmvs.x.ps x%d, f%d, %d %s%s", dst, src1, index, (comm?" # ":""), (comm?comm:""));
    require_fp_active();

    index = index % VL;
    if (dst != x0)
        XREGS[dst].x = sext32(FREGS[src1].u[index]);

    LOG(DEBUG, "\t 0x%016" PRIx64 " <-- {0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x}", XREGS[dst].x,
        FREGS[src1].u[0], FREGS[src1].u[1], FREGS[src1].u[2], FREGS[src1].u[3],
        FREGS[src1].u[4], FREGS[src1].u[5], FREGS[src1].u[6], FREGS[src1].u[7]);

    logxregchange(dst);
}

void fswizz_ps(freg dst, freg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: fswizz.ps f%d, f%d, %u%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fswizz(dst, src1, imm);
}

void fcvt_pw_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.pw.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPWPS, VL, dst, src1, rm);
}

void fcvt_pwu_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.pwu.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPWUPS, VL, dst, src1, rm);
}

void fclass_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fclass.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    for (int i = 0; i < VL; ++i)
    {
        if (MREGS[0].b[i] == 0) continue;
        iufval32 val, res;
        val.u = FREGS[src1].u[i];
        res.u = fpu::f32_classify(val.f);
        LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (%g)", i, res.u, val.u, val.flt);
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fcvt_ps_pw(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.pw f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSPW, VL, dst, src1, rm);
}

void fcvt_ps_pwu(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.pwu f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSPWU, VL, dst, src1, rm);
}

void fmadd_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmadd.ps f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu3src(FMADD, VL, dst, src1, src2, src3, rm);
}

void fmsub_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fmsub.ps f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu3src(FMSUB, VL, dst, src1, src2, src3, rm);
}

void fnmsub_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fnmsub.ps f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu3src(FNMSUB, VL, dst, src1, src2, src3, rm);
}

void fnmadd_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fnmadd.ps f%d, f%d, f%d, f%d, %s%s%s", dst, src1, src2, src3, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu3src(FNMADD, VL, dst, src1, src2, src3, rm);
}

// ----- Graphics upconvert --------------------------------

static void ucvtemu(opcode opc, freg dst, freg src1)
{
    set_rounding_mode(rmdyn);
    clear_arithmetic_flags();
    for (int i = 0; i < VL; i++)
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        uint32_t val = FREGS[src1].u[i];
        iufval32 res;
        switch (opc)
        {
            case FCVTPSF16:  res.f = fpu::f16_to_f32(cast_uint16_to_float16(val));  break;
            case FCVTPSF11:  res.f = fpu::f11_to_f32(cast_uint16_to_float11(val));  break;
            case FCVTPSF10:  res.f = fpu::f10_to_f32(cast_uint16_to_float10(val));  break;
            case FCVTPSUN24: res.f = fpu::un24_to_f32(val); break;
            case FCVTPSUN16: res.f = fpu::un16_to_f32(val); break;
            case FCVTPSUN10: res.f = fpu::un10_to_f32(val); break;
            case FCVTPSUN8:  res.f = fpu::un8_to_f32(val);  break;
            case FCVTPSUN2:  res.f = fpu::un2_to_f32(val);  break;
            case FCVTPSSN16: res.f = fpu::sn16_to_f32(val); break;
            case FCVTPSSN8:  res.f = fpu::sn8_to_f32(val);  break;
            default: assert(0); break;
        }
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%d)", i, res.u, res.flt, val, val);
        FREGS[dst].u[i] = res.u;
    }
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

void fcvt_ps_f16(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.f16 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF16, dst, src1);
}

void fcvt_ps_f11(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.f11 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF11, dst, src1);
}

void fcvt_ps_f10(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.f10 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF10, dst, src1);
}

void fcvt_ps_un24(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.un24 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN24, dst, src1);
}

void fcvt_ps_un16(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.un16 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN16, dst, src1);
}

void fcvt_ps_un10(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.un10 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN10, dst, src1);
}

void fcvt_ps_un8(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.un8 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN8, dst, src1);
}

void fcvt_ps_un2(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.un2 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN2, dst, src1);
}

void fcvt_ps_sn16(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.sn16 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSSN16, dst, src1);
}

void fcvt_ps_sn8(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.sn8 f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSSN8, dst, src1);
}

// ----- Graphics downconvert ------------------------------

static void dcvtemu(opcode opc, freg dst, freg src1)
{
    set_rounding_mode(rmdyn);
    clear_arithmetic_flags();
    for (int i = 0; i < VL; i++)
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val, res;
        val.u = FREGS[src1].u[i];
        switch (opc)
        {
            case FCVTF10PS:  res.u = cast_float10_to_uint16(fpu::f32_to_f10(val.f));  break;
            case FCVTF11PS:  res.u = cast_float11_to_uint16(fpu::f32_to_f11(val.f));  break;
            case FCVTF16PS:  res.u = cast_float16_to_uint16(fpu::f32_to_f16(val.f));  break;
            case FCVTUN24PS: res.u = fpu::f32_to_un24(val.f); break;
            case FCVTUN16PS: res.u = fpu::f32_to_un16(val.f); break;
            case FCVTUN10PS: res.u = fpu::f32_to_un10(val.f); break;
            case FCVTUN8PS:  res.u = fpu::f32_to_un8(val.f);  break;
            case FCVTUN2PS:  res.u = fpu::f32_to_un2(val.f);  break;
            case FCVTSN16PS: res.u = fpu::f32_to_sn16(val.f); break;
            case FCVTSN8PS:  res.u = fpu::f32_to_sn8(val.f);  break;
            default: assert(0); break;
        }
        LOG(DEBUG, "\t[%d] 0x%08x (%d) <-- down- 0x%08x (%g)", i, res.u, res.i, val.u, val.flt);
        FREGS[dst].u[i] = res.u;
    }
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

void fcvt_f16_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.f16.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF16PS, dst, src1);
}

void fcvt_f11_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.f11.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF11PS, dst, src1);
}

void fcvt_f10_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.f10.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF10PS, dst, src1);
}

void fcvt_un24_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.un24.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN24PS, dst, src1);
}

void fcvt_un16_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.un16.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN16PS, dst, src1);
}

void fcvt_un10_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.un10.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN10PS, dst, src1);
}

void fcvt_un8_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.un8.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN8PS, dst, src1);
}

void fcvt_un2_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.un2.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN2PS, dst, src1);
}

void fcvt_sn16_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.sn16.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTSN16PS, dst, src1);
}

void fcvt_sn8_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.sn8.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rmdyn), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTSN8PS, dst, src1);
}

// ----- Graphics additional -------------------------------

void fsin_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fsin.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FSIN, VL, dst, src1, rmdyn);
}

void fexp_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fexp.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FEXP, VL, dst, src1, rmdyn);
}

void flog_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: flog.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FLOG, VL, dst, src1, rmdyn);
}

void ffrc_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: ffrc.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FFRC, VL, dst, src1, rm);
}

void fround_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fround.ps f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    set_rounding_mode(rm);
    clear_arithmetic_flags();
    DEBUG_MASK(MREGS[0]);
    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val, res;
        val.u = FREGS[src1].u[i];
        res.f = fpu::f32_roundToInt(val.f);
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%08x (%g) ", i, res.u, res.flt, val.u, val.flt);
        FREGS[dst].u[i] = res.u;
    }
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

void frcp_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: frcp.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FRCP, VL, dst, src1, rmdyn);
}

void frsq_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: frsq.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FRSQ, VL, dst, src1, rmdyn);
}

// FIXME: THIS INSTRUCTION IS OBSOLETE
void frcpfxp_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: frcpfxp.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FRCPFXP, VL, dst, src1, rmdyn);
}

void cubeface_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: cubeface.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        // check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        uint8_t rz_lt_ry = (FREGS[ dst].u[i] & 0x1);
        uint8_t rz_lt_rx = (FREGS[src1].u[i] & 0x1);
        uint8_t ry_lt_rx = (FREGS[src2].u[i] & 0x1);
        uint32_t res = rz_lt_ry ? (ry_lt_rx ? 0 : 1) : (rz_lt_rx ? 0 : 2);

        LOG(DEBUG, "\t[%d] %d <-- %d %d %d", i, res, rz_lt_ry, rz_lt_rx, ry_lt_rx);

        FREGS[dst].u[i] = res;
    }

    dirty_fp_state();
    logfregchange(dst);
}

void cubefaceidx_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: cubefaceidx.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    clear_arithmetic_flags();
    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        // check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = FREGS[src2].u[i];
        res.f  = fpu::f32_cubeFaceIdx(val1.u, val2.f);
        LOG(DEBUG, "\t[%d] %g <-- %u %g", i, res.flt, val1.u, val2.flt);
        FREGS[dst].u[i] = res.u;
    }
    set_fp_exceptions();
    dirty_fp_state();
    logfregchange(dst);
}

void cubesgnsc_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: cubesgnsc.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        // check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = FREGS[src2].u[i];
        res.f  = fpu::f32_cubeFaceSignS(val1.u, val2.f);
        LOG(DEBUG, "\t[%d] 0x08%x (%g) <-- 0x%x 0x%08x (%g)", i, res.u, res.flt, val1.u, val2.u, val2.flt);
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

void cubesgntc_ps(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: cubesgntc.ps f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        // check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = FREGS[src2].u[i];
        res.f  = fpu::f32_cubeFaceSignT(val1.u, val2.f);
        LOG(DEBUG, "\t[%d] 0x%08x (%g) <-- 0x%x 0x%08x (%g)", i, res.u, res.flt, val1.u, val2.u, val2.flt);
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

void fcvt_ps_rast(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    LOG(DEBUG, "I: fcvt.ps.rast f%d, f%d, %s%s%s", dst, src1, get_rounding_mode(rm), (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSRAST, VL, dst, src1, rm);
}

void fcvt_rast_ps(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fcvt.rast.ps f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTRASTPS, VL, dst, src1, rmdyn);
}

void frcp_fix_rast(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: frcp.fix.rast f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    femu2src(FRCP_FIX_RAST, VL, dst, src1, src2, rmdyn);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto packed-integer extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Broadcast -----------------------------------------

void fbci_pi(freg dst, int32_t imm, const char* comm)
{
    LOG(DEBUG, "I: fbci.pi f%d, 0x%08x%s%s", dst, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);

    for (int i = 0; i < VL; i++)
    {
        if (MREGS[0].b[i])
        {
            FREGS[dst].i[i] = imm;
            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x", i, FREGS[dst].i[i], imm);
        }
    }
    dirty_fp_state();
    logfregchange(dst);
}

// ----- Computational (follows RV64I/F/M) -----------------

static void iemu2src(opcode opc, freg dst, freg src1, freg src2)
{
    for (int i = 0; i < VL; i++)
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = (src2 != fnone) ? FREGS[src2].u[i] : 0;

        switch (opc)
        {
            case FADDPI :   res.u = val1.u + val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x + 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FSUBPI :   res.u = val1.u - val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x - 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FMULPI :   res.u = val1.u * val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x * 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FMULHPI :  res.i = ((int64_t(val1.i) * int64_t(val2.i)) >> 32) & 0xFFFFFFFF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x * 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FMULHUPI : res.u = ((uint64_t(val1.u) * uint64_t(val2.u)) >> 32) & 0xFFFFFFFF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x * 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FDIVPI :   res.i = val1.i / val2.i;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x / 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FDIVUPI :  res.u = val2.u ? (val1.u / val2.u) : 0xFFFFFFFF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x /u 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FREMPI  :  res.i = val2.i ? (val1.i % val2.i) : 0xFFFFFFFF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x %% 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FREMUPI :  res.u = val2.u ? (val1.u % val2.u) : 0xFFFFFFFF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x %%u 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FMAXPI :   res.i = (val1.i >= val2.i) ? val1.i : val2.i;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- max(0x%08x, 0x%08x )", i, res.u, val1.u, val2.u);
                            break;
            case FMINPI :   res.i = (val1.i < val2.i) ? val1.i : val2.i;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- min(0x%08x, 0x%08x )", i, res.u, val1.u, val2.u);
                            break;
            case FMAXUPI :  res.u = (val1.u >= val2.u) ? val1.u : val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- maxu(0x%08x, 0x%08x )", i, res.u, val1.u, val2.u);
                            break;
            case FMINUPI :  res.u = (val1.u < val2.u) ? val1.u : val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- minu(0x%08x, 0x%08x )", i, res.u, val1.u, val2.u);
                            break;
            case FANDPI :   res.u = val1.u & val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x & 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FORPI :    res.u = val1.u | val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x | 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FXORPI :   res.u = val1.u ^ val2.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x ^ 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FNOTPI :   res.u = ~val1.u;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- ~ 0x%08x", i, res.u, val1.u);
                            break;
            case FSAT8PI :  res.i = ((val1.i > 127) ? 127 :(val1.i < -128 ? -128 : val1.i)) & 0x0FF;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- sat8(0x%08x)", i, res.u, val1.u);
                            break;
            case FSATU8PI : res.u = ((val1.i > 255) ? 255u :(val1.i < 0 ? 0u : val1.u)) & 0x0FFu;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- sat8u(0x%08x)", i, res.u, val1.u);
                            break;
            case FSLLPI :   res.u = (val2.u >= 32) ? 0 : (val1.u << val2.u);
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x << %d", i, res.u, val1.u, val2.u);
                            break;
            case FSRLPI :   res.u = (val2.u >= 32) ? 0 : (val1.u >> val2.u);
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x >> %u", i, res.u, val1.u, val2.u);
                            break;
            case FSRAPI :   res.u = (val2.u >= 32) ? (val1.i >> 31) : (val1.i >> val2.i);
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x >>a %d", i, res.u, val1.u, val2.u);
                            break;
            case FLTPI :    res.u = (val1.i < val2.i) ? 0xFFFFFFFF : 0;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x < 0x%08x ", i, res.u, val1.u, val2.u);
                            break;
            case FLTUPI :   res.u = (val1.u < val2.u) ? 0xFFFFFFFF : 0;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x < 0x%08x ", i, res.u, val1.u, val2.u);
                            break;
            case FLEPI :    res.u = (val1.i <= val2.i) ? 0xFFFFFFFF : 0;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x <= 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            case FEQPI :    res.u = (val1.u == val2.u) ? 0xFFFFFFFF : 0;
                            LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x == 0x%08x", i, res.u, val1.u, val2.u);
                            break;
            default:        assert(0);
                            break;
        }
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

static void iemu2srcimm(opcode opc, freg dst, freg src1, uint32_t imm)
{
    for (int i = 0; i < VL; i++)
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i] == 0) continue;

        iufval32 val1, val2, res;
        val1.u = FREGS[src1].u[i];
        val2.u = sext10(imm); // sign extend the 10-low order bits of imm

        switch (opc)
        {
            case FADDIPI: res.u = val1.u + val2.u;
                          LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x + 0x%08x", i, res.u, val1.u, val2.u);
                          break;
            case FANDIPI: res.u = val1.u & val2.u;
                          LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x & 0x%08x", i, res.u, val1.u, val2.u);
                          break;
            case FSLLIPI: res.u = val1.u << val2.u;
                          LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x << %u", i, res.u, val1.u, val2.u);
                          break;
            case FSRLIPI: res.u = val1.u >> val2.u;
                          LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x >> %u", i, res.u, val1.u, val2.u);
                          break;
            case FSRAIPI: res.i = val1.i >> val2.i;
                          LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x >>a %u", i, res.u, val1.u, val2.u);
                          break;
            default:      assert(0);
                          break;
        }
        FREGS[dst].u[i] = res.u;
    }
    dirty_fp_state();
    logfregchange(dst);
}

static void packrep(opcode opc, freg dst, freg src1)
{
    fdata val = FREGS[src1];
    switch (opc)
    {
        case FPACKREPHPI :
            for (int i = 0; i < VL; i++)
            {
                if (MREGS[0].b[i] == 0) continue;

                int j = (4 * i) % (VL*2);
                FREGS[dst].u[i] = uint32_t(val.h[j]) | (uint32_t(val.h[j+2]) << 16);
                //LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (chan %d, %d)", i, FREGS[dst].u[i], FREGS[dst].u[i], j, j+2);
            }
            break;
        case FPACKREPBPI:
            for (int i = 0; i < VL; i++)
            {
                if (MREGS[0].b[i] == 0) continue;

                int j = (16 * i) % (VL*4);
                FREGS[dst].u[i] = uint32_t(val.b[j]) | (uint32_t(val.b[j+4]) << 8) | (uint32_t(val.b[j+8]) << 16) | (uint32_t(val.b[j+12]) << 24);
                //LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x (chan %d, %d, %d, %d)", i, FREGS[dst].u[i], FREGS[dst].u[i], j, j+4, j+8, j+12);
            }
            break;
        default:
            assert(0);
            break;
    }

    for (int i = 0; i < VL; i++)
        LOG(DEBUG, "\t[%d] 0x%08x <-- 0x%08x", i, FREGS[dst].u[i], val.u[i]);

    dirty_fp_state();
    logfregchange(dst);
}

void feq_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: feq.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FEQPI, dst, src1, src2);
}

void fle_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fle.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLEPI, dst, src1, src2);
}

void flt_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: flt.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLTPI, dst, src1, src2);
}

void fltu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fltu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLTUPI, dst, src1, src2);
}

void fltm_pi(mreg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fltm.pi m%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    fmask(FLTPI, dst, src1, src2);
}

void faddi_pi(freg dst, freg src1, int32_t imm, const char* comm)
{
    LOG(DEBUG, "I: faddi.pi f%d, f%d, 0x%08x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FADDIPI, dst, src1, imm);
}

void fslli_pi(freg dst, freg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: fslli.pi f%d, f%d, 0x%08x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSLLIPI, dst, src1, imm);
}

void fsrli_pi(freg dst, freg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: fsrli.pi f%d, f%d, 0x%08x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSRLIPI, dst, src1, imm);
}

void fsrai_pi(freg dst, freg src1, unsigned imm, const char* comm)
{
    LOG(DEBUG, "I: fsrai.pi f%d, f%d, 0x%08x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSRAIPI, dst, src1, imm);
}

void fandi_pi(freg dst, freg src1, int32_t imm, const char* comm)
{
    LOG(DEBUG, "I: fandi.pi f%d, f%d, 0x%08x%s%s", dst, src1, imm, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FANDIPI, dst, src1, imm);
}

void fadd_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fadd.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FADDPI, dst, src1, src2);
}

void fsub_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsub.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSUBPI, dst, src1, src2);
}

void fsll_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsll.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSLLPI, dst, src1, src2);
}

void fxor_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fxor.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FXORPI, dst, src1, src2);
}

void fsrl_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsrl.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSRLPI, dst, src1, src2);
}

void fsra_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fsra.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSRAPI, dst, src1, src2);
}

void for_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: for.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FORPI, dst, src1, src2);
}

void fand_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fand.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FANDPI, dst, src1, src2);
}

void fnot_pi(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fnot.pi f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FNOTPI, dst, src1, fnone);
}

void fsat8_pi(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fsat8.pi f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSAT8PI, dst, src1, fnone);
}

void fsatu8_pi(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fsatu8.pi f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSATU8PI, dst, src1, fnone);
}

void fpackreph_pi(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fpackreph.pi f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    packrep(FPACKREPHPI, dst, src1);
}

void fpackrepb_pi(freg dst, freg src1, const char* comm)
{
    LOG(DEBUG, "I: fpackrepb.pi f%d, f%d%s%s", dst, src1, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    packrep(FPACKREPBPI, dst, src1);
}

void fmul_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmul.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULPI, dst, src1, src2);
}

void fmulh_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmulh.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULHPI, dst, src1, src2);
}

void fmulhu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmulhu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULHUPI, dst, src1, src2);
}

void fdiv_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fdiv.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FDIVPI, dst, src1, src2);
}

void fdivu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fdivu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FDIVUPI, dst, src1, src2);
}

void frem_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: frem.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FREMPI, dst, src1, src2);
}

void fremu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fremu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (core_type == ET_MINION)
        throw trap_mcode_instruction(current_inst);

    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FREMUPI, dst, src1, src2);
}

void fmin_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmin.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMINPI, dst, src1, src2);
}

void fmax_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmax.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMAXPI, dst, src1, src2);
}

void fminu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fminu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMINUPI, dst, src1, src2);
}

void fmaxu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    LOG(DEBUG, "I: fmaxu.pi f%d, f%d, f%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    require_fp_active();
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMAXUPI, dst, src1, src2);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto scalar extension for graphics emulation
//
////////////////////////////////////////////////////////////////////////////////

void packb(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: packb x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = (XREGS[src1].x & 0x0FF) | ((XREGS[src2].x << 8) & 0x0FF00);
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " + 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
}

void bitmixb(xreg dst, xreg src1, xreg src2, const char* comm)
{
    LOG(DEBUG, "I: bitmixb x%d, x%d, x%d%s%s", dst, src1, src2, (comm?" # ":""), (comm?comm:""));
    if (dst != x0)
    {
        uint64_t val = 0;
        uint64_t mask = XREGS[src1].x;
        uint64_t in0 = XREGS[src2].x;
        uint64_t in1 = XREGS[src2].x >> 8;
        for (uint32_t b = 0; b < 16; b++)
        {
            if (mask & 0x01)
            {
                val = val | ((in1 & 0x01) << b);
                in1 = in1 >> 1;
            }
            else
            {
                val = val | ((in0 & 0x01) << b);
                in0 = in0 >> 1;
            }
            mask = mask >> 1;
        }
        LOG(DEBUG, "\t0x%016" PRIx64 " <-- 0x%016" PRIx64 " + 0x%016" PRIx64 , val, XREGS[src1].x, XREGS[src2].x);
        XREGS[dst].x = val;
    }
    logxregchange(dst);
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

#define AMO_EMU_F_FUNC(NAME, OPC) \
void NAME(freg dst, freg src1, xreg src2, const char* comm)\
{\
   LOG(DEBUG, "I: " #NAME " f%d, f%d(x%d)%s%s", dst, src1, src2, comm ? " # " : "", comm ? comm : "");\
   amo_emu_f(OPC, dst, src1, src2);\
}

void amo_emu_f(amoop op, freg dst, freg src1, xreg src2)
{
    uint64_t addr;
    iufval32 res, val1, val2;

    for (int el = 0; el < VL; el++)
    {
        if (MREGS[0].b[el] == 0) continue;

        addr = XREGS[src2].x + FREGS[src1].i[el];

        // Check misaligned access
        if ((addr & 0x3) != 0) throw trap_store_address_misaligned(addr);

        val1.u = vmemread32(addr);
        val2.u = FREGS[dst].u[el];

        // Save the loaded data
        FREGS[dst].u[el] = val1.u;
        LOG(DEBUG, "\t0x%08" PRIx32 " <-- MEM[0x%016" PRIx64 "]", val1.u, addr);

        switch (op)
        {
           case SWAP:
              res.u = val2.u;
              break;
           case AND:
              res.u = val1.u & val2.u;
              LOG(DEBUG, "\t0x%08x <-- 0x%08x & 0x%08x", res.u, val1.u, val2.u);
              break;
           case OR:
              res.u = val1.u | val2.u;
              LOG(DEBUG, "\t0x%08x <-- 0x%08x | 0x%08x", res.u, val1.u, val2.u);
              break;
           case XOR:
              res.u = val1.u ^ val2.u;
              LOG(DEBUG, "\t0x%08x <-- 0x%08x ^ 0x%08x", res.u, val1.u, val2.u);
              break;
           case ADD:
              res.u = val1.i + val2.i;
              LOG(DEBUG, "\t0x%08x <-- 0x%08x + 0x%08x", res.u, val1.u, val2.u);
              break;
           case MIN:
              res.u = (val1.i < val2.i) ? val1.u : val2.u;
              LOG(DEBUG, "\t0x%08x <-- min(0x%08x, 0x%08x)", res.u, val1.u, val2.u);
              break;
           case MAX:
              res.u = (val1.i > val2.i) ? val1.u : val2.u;
              LOG(DEBUG, "\t0x%08x <-- max(0x%08x, 0x%08x)", res.u, val1.u, val2.u);
              break;
           case MINU:
              res.u = (val1.u < val2.u) ? val1.u : val2.u;
              LOG(DEBUG, "\t0x%08x <-- minu(0x%08x, 0x%08x)", res.u, val1.u, val2.u);
              break;
           case MAXU:
              res.u = (val1.u > val2.u) ? val1.u : val2.u;
              LOG(DEBUG, "\t0x%08x <-- maxu(0x%08x, 0x%08x)", res.u, val1.u, val2.u);
              break;
           case MINPS:
              res.f = fpu::f32_minNum(val1.f, val2.f);
              LOG(DEBUG, "\t0x%08x (%g) <-- min(0x%08x (%g), 0x%08x (%g))", res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
              break;
           case MAXPS:
              res.f = fpu::f32_maxNum(val1.f, val2.f);
              LOG(DEBUG, "\t0x%08x (%g) <-- max(0x%08x (%g), 0x%08x (%g))", res.u, res.flt, val1.u, val1.flt, val2.u, val2.flt);
              break;
           default:
              res.u = 0;
              LOG(DEBUG, "\tFATAL: Unknown atomic op %d", op);
        }

        // Stores the operated data
        vmemwrite32(addr, res.u);
        LOG(DEBUG, "\t0x%08x --> MEM[0x%016" PRIx64 "]", res.u, addr);

        // note: for logging purposes, sending val2.u instead of res.u => we want to check what the
        // dcache outputs to the shire caches, not the actual value written in memory
        logmemwchange(el, 4, addr, val2.u);
    }
    dirty_fp_state();
    logfregchange(dst);
}


//
// Local Scalar 32 bits Atomics
//

AMO_EMU_W_FUNC(amoswapl_w, SWAP)
AMO_EMU_W_FUNC(amoandl_w,  AND)
AMO_EMU_W_FUNC(amoorl_w,   OR)
AMO_EMU_W_FUNC(amoxorl_w,  XOR)
AMO_EMU_W_FUNC(amoaddl_w,  ADD)
AMO_EMU_W_FUNC(amominl_w,  MIN)
AMO_EMU_W_FUNC(amomaxl_w,  MAX)
AMO_EMU_W_FUNC(amominul_w, MINU)
AMO_EMU_W_FUNC(amomaxul_w, MAXU)

//
// Global Scalar 32 bits Atomics
//

AMO_EMU_W_FUNC(amoswapg_w, SWAP)
AMO_EMU_W_FUNC(amoandg_w,  AND)
AMO_EMU_W_FUNC(amoorg_w,   OR)
AMO_EMU_W_FUNC(amoxorg_w,  XOR)
AMO_EMU_W_FUNC(amoaddg_w,  ADD)
AMO_EMU_W_FUNC(amoming_w,  MIN)
AMO_EMU_W_FUNC(amomaxg_w,  MAX)
AMO_EMU_W_FUNC(amominug_w, MINU)
AMO_EMU_W_FUNC(amomaxug_w, MAXU)

//
// Local Scalar 64 bits Atomics
//

AMO_EMU_D_FUNC(amoswapl_d, SWAP)
AMO_EMU_D_FUNC(amoandl_d,  AND)
AMO_EMU_D_FUNC(amoorl_d,   OR)
AMO_EMU_D_FUNC(amoxorl_d,  XOR)
AMO_EMU_D_FUNC(amoaddl_d,  ADD)
AMO_EMU_D_FUNC(amominl_d,  MIN)
AMO_EMU_D_FUNC(amomaxl_d,  MAX)
AMO_EMU_D_FUNC(amominul_d, MINU)
AMO_EMU_D_FUNC(amomaxul_d, MAXU)

//
// Global Scalar 64 bits Atomics
//

AMO_EMU_D_FUNC(amoswapg_d, SWAP)
AMO_EMU_D_FUNC(amoandg_d,  AND)
AMO_EMU_D_FUNC(amoorg_d,   OR)
AMO_EMU_D_FUNC(amoxorg_d,  XOR)
AMO_EMU_D_FUNC(amoaddg_d,  ADD)
AMO_EMU_D_FUNC(amoming_d,  MIN)
AMO_EMU_D_FUNC(amomaxg_d,  MAX)
AMO_EMU_D_FUNC(amominug_d, MINU)
AMO_EMU_D_FUNC(amomaxug_d, MAXU)

//
// Local Packed 32 bits Atomics
//

AMO_EMU_F_FUNC(famoswapl_pi, SWAP)
AMO_EMU_F_FUNC(famoandl_pi,  AND)
AMO_EMU_F_FUNC(famoorl_pi,   OR)
AMO_EMU_F_FUNC(famoxorl_pi,  XOR)
AMO_EMU_F_FUNC(famoaddl_pi,  ADD)
AMO_EMU_F_FUNC(famominl_pi,  MIN)
AMO_EMU_F_FUNC(famomaxl_pi,  MAX)
AMO_EMU_F_FUNC(famominul_pi, MINU)
AMO_EMU_F_FUNC(famomaxul_pi, MAXU)
AMO_EMU_F_FUNC(famominl_ps,  MINPS)
AMO_EMU_F_FUNC(famomaxl_ps,  MAXPS)

//
// Global Packed 32 bits Atomics
//

AMO_EMU_F_FUNC(famoswapg_pi, SWAP)
AMO_EMU_F_FUNC(famoandg_pi,  AND)
AMO_EMU_F_FUNC(famoorg_pi,   OR)
AMO_EMU_F_FUNC(famoxorg_pi,  XOR)
AMO_EMU_F_FUNC(famoaddg_pi,  ADD)
AMO_EMU_F_FUNC(famoming_pi,  MIN)
AMO_EMU_F_FUNC(famomaxg_pi,  MAX)
AMO_EMU_F_FUNC(famominug_pi, MINU)
AMO_EMU_F_FUNC(famomaxug_pi, MAXU)
AMO_EMU_F_FUNC(famoming_ps,  MINPS)
AMO_EMU_F_FUNC(famomaxg_ps,  MAXPS)


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

uint64_t get_scratchpad_value(int entry, int block, int * last_entry, int * size)
{
    * last_entry = scp_entry[current_thread];
    * size = scp_size[current_thread];
    return SCP[entry][block >> 2].x[block & 3];
}

void get_scratchpad_conv_list(std::list<bool> * list)
{
    for (int i = 0; i < 16; i++)
        list->push_back(scp_tm && !tmask_pass(i));
}

// This is a temporal fix until all the software is migrated to the new SCP.
// When migration is complete, BEMU tensor operations should only operate with
// SCP ranges
int get_scratchpad_next_entry(int entry)
{
    const static int row_to_scp[48] = {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
        48, 49, 50, 41, 52, 53, 54, 55, 56, 57, 58, 59
    };
    if (csrregs[current_thread][csr_scratchpad_ctrl] & 0x1)
    {
        if (entry > 47 || entry < 0)
        {
            LOG(DEBUG,"SCP entry out of range : %i\n", entry);
            return entry;
        }
        return row_to_scp[entry];
    }
    return entry;
}

// ----- CacheOp emulation -----------------------------------------------------

static void dcache_evict_flush_set_way(bool evict, bool tm, int dest, int set, int way, int numlines)
{
    // Skip all if dest is L1, or if set/way is outside the cache limits
    if ((dest == 0) || (set >= L1D_NUM_SETS) || (way >= L1D_NUM_WAYS))
        return;

    for (int i = 0; i <= numlines; i++)
    {
        // If cacheline is locked or not passing tensor mask condition, skip operation
        if (!scp_locked[current_thread >> 1][set][way] && (!tm || tmask_pass(i)))
        {
          LOG(DEBUG, "\tDoing %s (%d.%d) to Set: %d, Way: %d, DestLevel: %d",
              evict ? "EvictSW" : "FlushSW", current_thread >> 1, current_thread & 1,
              set, way, dest);
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

static int dcache_evict_flush_vaddr(bool evict, bool tm, int dest, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    (void)(id);

    // Skip all if dest is L1
    if (dest == 0)
        return 0;

    for (int i = 0; i <= numlines; i++, vaddr += stride)
    {
        // skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, Mem_Access_Load);
        }
        catch (const trap_t& t)
        {
            LOG(DEBUG, "\t%s: %016" PRIx64 ", DestLevel: %01x generated exception (suppressed)",
                evict ? "EvictVA" : "FlushVA", vaddr, dest);
            update_tensor_error(1 << 7);
            return 1;
        }
        int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;
        bool skip = false;
        for (int j = 0; j < L1D_NUM_WAYS; ++j)
        {
            skip = skip || (scp_locked[current_thread >> 1][set][j] && (scp_trans[current_thread >> 1][set][j] == paddr));
        }
        if (skip)
            continue;
        LOG(DEBUG, "\tDoing %s: %016" PRIx64 " (%016" PRIx64 "), DestLevel: %01x",
            evict ? "EvictVA" : "FlushVA", vaddr, paddr, dest);
    }
    return 0;
}

static int dcache_prefetch_vaddr(bool tm, int dest, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    (void)(id);

    // Skip all if dest is MEM
    if (dest == 3)
        return 0;

    for (int i = 0; i <= numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, Mem_Access_Load);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tPrefetchVA: %016" PRIx64 ", DestLevel: %01x generated exception (suppressed)", vaddr, dest);
            update_tensor_error(1 << 7);
            return 1;
        }
        // If target level is L1 check if the line is locked
        bool skip = false;
        if (dest == 0)
        {
            int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;
            for (int j = 0; j < L1D_NUM_WAYS; ++j)
            {
                skip = skip || (scp_locked[current_thread >> 1][set][j] && (scp_trans[current_thread >> 1][set][j] == paddr));
            }
        }
        if (skip)
            continue;
        LOG(DEBUG, "\tDoing PrefetchVA: %016" PRIx64 " (%016" PRIx64 "), DestLevel: %01x", vaddr, paddr, dest);
    }
    return 0;
}

static int dcache_lock_paddr(int way, uint64_t paddr)
{
    // Skip all if way is outside the cache limits
    if ((way >= L1D_NUM_WAYS) && (way != 255))
        return 0;

    int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;

    if (way == 255)
    {
        // Lock the first available way
        // FIXME: or if the line exists unlocked in the cache use the way of the existing line.
        bool way_found = false;
        for (int w = 0; w < L1D_NUM_WAYS; ++w)
        {
            if (!scp_locked[current_thread >> 1][set][w])
            {
                way = w;
                way_found = true;
                break;
            }
        }
        // No free way found to lock
        if(!way_found) {
            csrregs[current_thread][csr_tensor_error] |= (0x1 << 5); 
            return 1;
        }
    }
    if (way == 255)
    {
        // All ways are locked; stop the operation
        LOG(DEBUG, "\tLockSW: %016" PRIx64 ", Way: %d no unlocked ways", paddr, way);
        return 1;
    }

    // Check if paddr already locked in the cache
    for (int w = 0; w < L1D_NUM_WAYS; ++w)
    {
        if (scp_locked[current_thread >> 1][set][w] && (scp_trans[current_thread >> 1][set][w] == paddr))
        {
            // Line already locked; stop the operation
            LOG(DEBUG, "\tLockSW: %016" PRIx64 ", Way: %d double-locking on way %d", paddr, way, w);
            csrregs[current_thread][csr_tensor_error] |= (0x1 << 5); 
            return 1;
        }	    
    }
    // FIXME: We should check if PA exists, unlocked, in another set in the cache

    // check if the way is locked
    if (scp_locked[current_thread >> 1][set][way]) {
        csrregs[current_thread][csr_tensor_error] |= (0x1 << 5);
        return 1;
    }

    scp_locked[current_thread >> 1][set][way] = true;
    scp_trans[current_thread >> 1][set][way] = paddr;
    LOG(DEBUG, "\tDoing LockSW: (%016" PRIx64 "), Way: %d, Set: %d", paddr, way, set);
    return 0;
}

static int dcache_unlock_paddr(int way, uint64_t paddr)
{
    int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;

    // Check if paddr is locked in the cache
    for (int w = 0; w < L1D_NUM_WAYS; ++w)
    {
        if (scp_locked[current_thread >> 1][set][w] && (scp_trans[current_thread >> 1][set][w] == paddr))
        {
            LOG(DEBUG, "\tDoing UnlockSW: (%016" PRIx64 "), Way: %d, Set: %d",
                     paddr, w, set);
            scp_locked[current_thread >> 1][set][w] = false;
        }
    }
    return 0;
}

static int dcache_lock_vaddr(bool tm, int way, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    (void)(id);

    // Skip all if way is outside the cache limits
    if ((way >= L1D_NUM_WAYS) && (way != 255))
        return 0;

    for (int i = 0; i <= numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, Mem_Access_Store);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tLockVA %016" PRIx64 ", Way: %d generated exception (suppressed)", vaddr, way);
            update_tensor_error(1 << 7);
            return 1;
        }
        int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;

        if (way == 255)
        {
            // Lock the first available way
            // FIXME: or if the line exists unlocked in the cache use the way of the existing line.
            bool way_found = false;
            for (int w = 0; w < L1D_NUM_WAYS; ++w)
            {
                if (!scp_locked[current_thread >> 1][set][w])
                {
                    way = w;
                    way_found = true;
                    break;
                }
            }
            // No free way found to lock
            if (!way_found)
            {
                update_tensor_error(1 << 5);
                return 1;
            }
        }
        if (way == 255)
        {
            // All ways are locked; stop the operation
            LOG(DEBUG, "\tLockVA: %016" PRIx64 ", Way: %d no unlocked ways", vaddr, way);
            return 1;
        }

        // Check if paddr already locked in the cache
        for (int w = 0; w < L1D_NUM_WAYS; ++w)
        {
            if (scp_locked[current_thread >> 1][set][w] && (scp_trans[current_thread >> 1][set][w] == paddr))
            {
                // Line already locked; stop the operation
                LOG(DEBUG, "\tLockVA: %016" PRIx64 ", Way: %d double-locking on way %d", vaddr, way, w);
                update_tensor_error(1 << 5);
                return 1;
            }
        }
        // FIXME: We should check if PA exists, unlocked, in another set in the cache

        // check if the way is locked
        if (scp_locked[current_thread >> 1][set][way])
        {
            update_tensor_error(1 << 5);
            return 1;
        }

        scp_locked[current_thread >> 1][set][way] = true;
        scp_trans[current_thread >> 1][set][way] = paddr;
        LOG(DEBUG, "\tDoing LockVA: %016" PRIx64 " (%016" PRIx64 "), Way: %d, Set: %d", vaddr, paddr, way, set);
    }
    return 0;
}

static int dcache_unlock_vaddr(bool tm, bool keep_valid, uint64_t vaddr, int numlines, int id, uint64_t stride)
{
    for (int i = 0; i <= numlines; i++, vaddr += stride)
    {
        // Skip if masked
        if (tm && !tmask_pass(i))
            continue;

        uint64_t paddr;
        try
        {
            paddr = vmemtranslate(vaddr, Mem_Access_Store);
        }
        catch (const trap_t& t)
        {
            // Stop the operation if there is an exception
            LOG(DEBUG, "\tUnlockVA: %016" PRIx64 " generated exception (suppressed)", vaddr);
            update_tensor_error(1 << 7);
            return 1;
        }
        int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;

        // Check if paddr is locked in the cache
        for (int w = 0; w < L1D_NUM_WAYS; ++w)
        {
            if (scp_locked[current_thread >> 1][set][w] && (scp_trans[current_thread >> 1][set][w] == paddr))
            {
              LOG(DEBUG, "\tDoing UnlockVA: %016" PRIx64 " (%016" PRIx64 "), Way: %d, Set: %d, FinalState: %s",
                         vaddr, paddr, w, set, keep_valid ? "valid" : "invalid");
              scp_locked[current_thread >> 1][set][w] = false;
            }
        }
    }
    return 0;
}

// static bool dcache_vaddr_is_locked(bool tm, uint64_t vaddr, int numlines, uint64_t stride)
// {
//     bool locked = true;
//     for (int i = 0; i <= numlines; i++, vaddr += stride)
//     {
//         // Skip if masked
//         if (tm && !tmask_pass(i))
//             continue;
//
//         uint64_t paddr = 0;
//         try
//         {
//             paddr = vmemtranslate(vaddr, Mem_Access_Store);
//         }
//         catch (const trap_t& t)
//         {
//             // Stop the operation if there is an exception
//             //LOG(DEBUG, "\tUnlockVA: %016" PRIx64 " generated exception (suppressed)", vaddr);
//             //return false;
//         }
//         int set = (paddr / L1D_LINE_SIZE) % L1D_NUM_SETS;
//
//         // Check if paddr is locked in the cache
//         for (int w = 0; w < L1D_NUM_WAYS; ++w)
//         {
//             locked &= scp_locked[current_thread >> 1][set][w];
//         }
//     }
//     return locked;
// }

static uint64_t csr_cacheop_emu(uint64_t op_value)
{
    bool tm         = ((op_value >> 63) & 1);
    bool keep_valid = ((op_value >> 59) & 1);

    int  op       = (op_value >> 60) & 0x07;
    int  dest     = (op_value >> 58) & 0x03;
    int  way      = (op_value >> 48) & 0xFF;
    int  set      = (op_value >>  6) & 0x0F;
    int  numlines = (op_value >>  0) & 0x0F;

    uint64_t addr   = op_value    & 0x0000FFFFFFFFFFC0ULL;
    uint64_t stride = XREGS[31].x & 0x0000FFFFFFFFFFC0ULL;
    int      id     = XREGS[31].x & 0x0000000000000001ULL;

    LOG(DEBUG, "\tDoing CacheOp with value %016" PRIx64, op_value);

    switch (op)
    {
        case 0: // LockVA
            return dcache_lock_vaddr(tm, way, addr, numlines, id, stride);
        case 1: // UnlockVA
            return dcache_unlock_vaddr(tm, keep_valid, addr, numlines, id, stride);
        case 2: // FlushSW
            if (prvget() != CSR_PRV_M)
                throw trap_illegal_instruction(current_inst);
            dcache_evict_flush_set_way(false, tm, dest, set, way, numlines);
            break;
        case 3: // EvictSW
            if (prvget() != CSR_PRV_M)
                throw trap_illegal_instruction(current_inst);
            dcache_evict_flush_set_way(true, tm, dest, set, way, numlines);
            break;
        case 4: // PrefetchVA
            return dcache_prefetch_vaddr(tm, dest, addr, numlines, id, stride);
        case 6: // FlushVA
            return dcache_evict_flush_vaddr(false, tm, dest, addr, numlines, id, stride);
        case 7: // EvictVA
            return dcache_evict_flush_vaddr(true, tm, dest, addr, numlines, id, stride);
        default:
           LOG(DEBUG, "\tUnknown CacheOp Opcode (%d)!", op);
           throw trap_illegal_instruction(current_inst);
    }
    return 0;
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

void write_msg_port_data_to_scp(uint32_t thread, uint32_t id, uint32_t *data, uint8_t oob)
{
    uint64_t base_addr = scp_trans[thread >> 1][msg_ports[thread][id].scp_set][msg_ports[thread][id].scp_way];
    base_addr += msg_ports[thread][id].rd_ptr << msg_ports[thread][id].logsize;

    msg_ports[thread][id].size++;

    int wr_words = 1 << (msg_ports[thread][id].logsize - 2);

    LOG(DEBUG, "Writing MSG_PORT (m%d p%d) wr_words %d, logsize %d",  thread, id, wr_words, msg_ports[thread][id].logsize);
    for (int i = 0; i < wr_words; i++)
    {
        LOG(DEBUG, "Writing MSG_PORT (m%d p%d) data 0x%08x to addr 0x%016" PRIx64,  thread, id, data[i], base_addr + 4 * i);
        vmemwrite32(base_addr + 4 * i, data[i]);
    }

    if (msg_ports[thread][id].enable_oob)
        msg_ports_oob[thread][id].push(oob);
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
        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize); b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION)].push_back(port_write);

        LOG(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from m%d", thread, id, current_thread);
    }
    else
        write_msg_port_data_to_scp(thread, id, data, oob);
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
        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize); b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION)].push_back(port_write);

        LOG(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from tbox%d", thread, id, tbox_id);
    }
    else
        write_msg_port_data_to_scp(thread, id, data, oob);
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
        for (uint32_t b = 0; b < (1UL << msg_ports[thread][id].logsize); b++)
            port_write.data[b] = data[b];
        port_write.oob = oob;
        msg_port_pending_writes[thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION)].push_back(port_write);

        LOG(DEBUG, "Delayed write on MSG_PORT (m%d p%d) from rbox%d", thread, id, rbox_id);
    }
    else
        write_msg_port_data_to_scp(thread, id, data, oob);
}

void commit_msg_port_data(uint32_t target_thread, uint32_t port_id, uint32_t source_thread)
{
    uint32_t shire = target_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); /**/)
        {
            port_write = *it;
            if ((port_write.target_thread == target_thread) &&
                (port_write.target_thread == port_id)       &&
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
            LOG(DEBUG, "Commit write on MSG_PORT (m%d p%d) from m%d", target_thread, port_id, source_thread);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from m%d not found!!", target_thread, port_id, source_thread);

    }
    else
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from m%d not found!!", target_thread, port_id, source_thread);
}

void commit_msg_port_data_from_tbox(uint32_t target_thread, uint32_t port_id, uint32_t tbox_id)
{
    uint32_t shire = target_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); /**/)
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
            LOG(DEBUG, "Commit write on MSG_PORT (m%d p%d) from tbox%d", target_thread, port_id, tbox_id);
            write_msg_port_data_to_scp(target_thread, port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from tbox%d not found!!", target_thread, port_id, tbox_id);
    }
    else
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from tbox%d not found!!", target_thread, port_id, tbox_id);
}

void commit_msg_port_data_from_rbox(uint32_t target_thread, uint32_t port_id, uint32_t rbox_id)
{
    uint32_t shire = target_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        for (auto it = msg_port_pending_writes[shire].begin(); it != msg_port_pending_writes[shire].end(); /**/)
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
            LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from rbox%d not found!!", target_thread, port_id, rbox_id);
    }
    else
        LOG(DEBUG, "ERROR Commit write on MSG_PORT (m%d p%d) from rbox%d not found!!", target_thread, port_id, rbox_id);
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
        msg_ports_oob[current_thread][id].pop();
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
    std::queue<uint8_t> to_delete;
    std::swap(msg_ports_oob[current_thread][id], to_delete);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto tensor extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- TensorConvolution emulation -------------------------------------------

// Returns if there something that needs to be processed or not based on current position and configuration
static bool conv_skip_pass(int conv_row_pos, int conv_col_pos, int conv_row_size, int conv_col_size)
{
    LOG(DEBUG, "Doing Conv skip pass check for:");
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
    uint64_t tmask_value = 0;

    // Gets the sizes of the convolution
    uint64_t tconvsizereg         = csrget(csr_tensor_conv_size);
    int      conv_row_step_offset = (tconvsizereg & 0xFF00000000000000ULL) >> 56;
    int      conv_row_size        = (tconvsizereg & 0x0000FFFF00000000ULL) >> 32; // Convolution size in rows
    int      conv_col_step_offset = (tconvsizereg & 0x00000000FF000000ULL) >> 24;
    int      conv_col_size        = (tconvsizereg & 0x000000000000FFFFULL);       // Convolution size in cols

    // Gets the positions of the convolution
    uint64_t tconvctrlreg = csrget(csr_tensor_conv_ctrl);
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
            tmask_value |= 1 << i;
        }
        // Move the position of the convolution sampling based on the configuration register
        conv_row_pos += conv_row_step_offset;
        conv_col_pos += conv_col_step_offset;
    }

    csrset(csr_tensor_mask, tmask_value);
}

static void tcoop(uint64_t value)
{
    int     timeout   = (value >> 16) & 0x1FF;
    uint8_t coop_mask = (value >>  8) & 0xFF;
    int     coop_id   = (value >>  0) & 0xFF;
    // TODO: implement functionality checking the addresses and tcoop of every use of Tensor Load
    LOG(DEBUG, "\tSetting Tensor Cooperation:  Timeout %d. Coop Mask %02X. Coop ID: %d", timeout, coop_mask, coop_id);
}

// ----- TensorLoad emulation --------------------------------------------------

void tensorload(uint64_t control)
{
    uint64_t stride  = XREGS[31].x & 0xFFFFFFFFFFC0ULL;

    int      tm                 = (control >> 63) & 0x1;
    int      use_coop           = (control >> 62) & 0x1;
    int      trans              = (control >> 59) & 0x7;
    int      dst                = (control >> 53) & 0x3F;
    int      tenb               = (control >> 52) & 0x1;
    //uint64_t virtual_addr_l2_sc = (control >>  6) & 0x3FFFFFFFFFF;
    uint64_t base               = control & 0xFFFFFFFFFFC0ULL;
    int      boffset            = (control >>  4) & 0x03;
    int      rows               = ((control      ) & 0xF) + 1;

    uint64_t addr             = base;
    scp_tm                    = tm;

    LOG(DEBUG, "Tensor Load: Trans:%d - rows:%d - tm:%d - use_coop:%d - dst:%d - tenb:%d - boffset:%d - addr:0x%16" PRIx64, trans, rows, tm, use_coop, dst, tenb, boffset, addr);

    // In case of loading data straight to tenb, we fake it by writing at position 64 and forth (not accessible otherwise)
    if (tenb)
    {
        dst = L1_SCP_ENTRIES;
        tensorload_setupb_topair[current_thread] = true;
        tensorload_setupb_topair[current_thread^1] = true;
        tensorload_setupb_numlines[current_thread] = rows;
        tensorload_setupb_numlines[current_thread^1] = rows;
    }

    scp_entry[current_thread] = dst;
    scp_size[current_thread]  = rows;

    // // Check if SCP is enabled
    // // Disabled until the SCPControl register spec is updated
    // if (!(csrregs[current_thread][csr_scratchpad_ctrl] & 0x1) && false)
    // {
    //     LOG(DEBUG, "ERROR TensorLoad with SCP disabled!!");
    //     update_tensor_error(1 << 4);
    //     return;
    // }

    // // Check if line is locked
    // // Disabled until the spec is updated
    // if (!dcache_vaddr_is_locked(tm, base, rows, stride))
    // {
    //     LOG(DEBUG, "ERROR Tensor Load writing to unlocked scp lines!!");
    //     update_tensor_error(1 << 0);
    //     return;
    // }

    //NO TRANS
    if (trans == 0x00)
    {
        LOG(DEBUG, "TensorLoad: No transformation");
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || tmask_pass(i))
            {
                if (addr & 0x3F)
                {
                    LOG(DEBUG, "ERROR Tensor Load not aligned to cache line!!");

                }
                for (int j = 0; j < L1_SCP_BLOCKS; j++)
                {
                    for (int k = 0; k < VL; k++)
                    {
                        uint64_t addr_final = addr+j*VL*4+k*4;
                        uint32_t val = 0;
                        try
                        {
                            val = vmemread32(addr_final);
                        }
                        catch (const trap_t& t)
                        {
                            // Memory exception
                            update_tensor_error(1 << 7);
                            return;
                        }
                        SCP[dst + i][j].u[k] = val;
                        LOG(DEBUG, "\tScratchpad tensor load MEM[%016" PRIx64 "]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)", addr_final, dst+i, j, k, SCP[dst+i][j].u[k], SCP[dst+i][j].u[k]);
                    }
                }
            }
            LOG(DEBUG, "\t\tAddress = 0x%016" PRIx64 " - Stride = 0x%016" PRIx64, addr, stride);
            addr += stride;
        }
    }
    //INTERLEAVE
    else if (trans == 0x01 || trans == 0x02)
    {
       LOG(DEBUG, "TensorLoad: Interleave");
       uint8_t tmp_buffer[4][64];
       int size = trans & 0x03;
       int start;
       start=size==1 ?  boffset << 4 : (boffset & 0x02) << 5;
       int elements = 4 / size;

       LOG(DEBUG, "#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, size, start, elements, boffset);
       for (int i = 0; i < rows; ++i)
       {
            if (!tm || tmask_pass(i))
            {
                if (addr & 0x3F)
                {
                    LOG(DEBUG, "ERROR Tensor Load not aligned to cache line!!");
                }
                for (int elem = 0; elem < elements; ++elem)
                {
                    //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
                    for (int j = 0; j < 8; j++)
                    {
                        for (int k = 0; k < 8; k++)
                        {
                            uint64_t addr_final = addr+j*8+k;
                            uint8_t val = 0;
                            try
                            {
                                val = vmemread8(addr_final);
                            }
                            catch (const trap_t& t)
                            {
                                // Memory exception
                                update_tensor_error(1 << 7);
                                return;
                            }
                            tmp_buffer[elem][j*8+k] = val;
                            LOG(DEBUG, "\tLoading into tmp_buffer - MEM[%016" PRIx64 "]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)", addr_final, elem, j, k, tmp_buffer[elem][j*8+k], tmp_buffer[elem][j*8+k]);
                        }
                    }

                    LOG(DEBUG, "\t\tAddres = 0x%016" PRIx64 " - Stride = 0x%016" PRIx64, addr, stride);
                    addr += stride;
                }
                for (int line = 0; line < L1_SCP_BLOCKS; ++ line)
                {
                    for (int byte = 0; byte < L1_SCP_BLOCK_SIZE; byte+=4)
                    {
                        // We interleve 32 bits each pass
                        if (elements == 2)
                        {
                            SCP[dst+i][line].b[byte]   = tmp_buffer[0][start+line*16+byte/elements];
                            SCP[dst+i][line].b[byte+1] = tmp_buffer[0][start+line*16+byte/elements+1];
                            SCP[dst+i][line].b[byte+2] = tmp_buffer[1][start+line*16+byte/elements];
                            SCP[dst+i][line].b[byte+3] = tmp_buffer[1][start+line*16+byte/elements+1];
                        }
                        if (elements == 4)
                        {
                            SCP[dst+i][line].b[byte]   = tmp_buffer[0][start+line*8+byte/elements];
                            SCP[dst+i][line].b[byte+1] = tmp_buffer[1][start+line*8+byte/elements];
                            SCP[dst+i][line].b[byte+2] = tmp_buffer[2][start+line*8+byte/elements];
                            SCP[dst+i][line].b[byte+3] = tmp_buffer[3][start+line*8+byte/elements];
                        }

                        LOG(DEBUG, "SCP[%d][%d].u[%d] = 0x%08x", dst+i, line, byte/4, SCP[dst+i][line].u[byte/4]);
                    }

                }
            }
        }
       //printSCP(addr,rows,stride,dst);
    }
    //TRANSPOSE
    else if (trans == 0x05 || trans == 0x06 || trans==0x07)
    {

        bool exist_conv = 0;
        for (int i=0; (i<rows) & (!exist_conv);++i)
            exist_conv = tmask_pass(i);
        if (tm && !exist_conv)
        {
            LOG(DEBUG, "Exit Condition Broken");
            return;
        }
        int offset;
        uint8_t tmp_buffer[64][64];
        int size = (trans & 0x03);

        offset = (size==1) ?  (control & 0x30) : (control & 0x20) ;
        int elements = 64 >> (size-1);
        size = 1 << (size-1);
        LOG(DEBUG, "TensorLoad: Transpose - elements:%d size:%d offset:%d", elements, size, offset);
        for (int elem = 0; elem < elements; ++elem)
        {
            //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
            for (int j = 0; j < 8; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    uint64_t addr_final = addr+j*8+k;
                    uint8_t val = 0;
                    try
                    {
                        val = vmemread8(addr_final);
                    }
                    catch (const trap_t& t)
                    {
                        // Memory exception
                        update_tensor_error(1 << 7);
                        return;
                    }
                    tmp_buffer[elem][j*8+k]=val;
                    LOG(DEBUG, "\tLoading into tmp_buffer - MEM[%016" PRIx64 "]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)", addr_final, elem, j, k, tmp_buffer[elem][j*8+k], tmp_buffer[elem][j*8+k]);
                }
            }
            addr += stride;
        }
        for (int  i =0 ;i < rows; ++i)
        {
             if (!tm || tmask_pass(i))
             {
                if (addr & 0x3F)
                {
                    LOG(DEBUG, "ERROR Tensor Load not aligned to cache line!!");
                }
                for (int j = 0; j < elements; ++j)
                {
                    if (size == 4)
                    {
                        SCP[dst+i][j*4/L1_SCP_BLOCK_SIZE].b[(j*size)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset];
                        SCP[dst+i][j*4/L1_SCP_BLOCK_SIZE].b[(j*size+1)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset+1];
                        SCP[dst+i][j*4/L1_SCP_BLOCK_SIZE].b[(j*size+2)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset+2];
                        SCP[dst+i][j*4/L1_SCP_BLOCK_SIZE].b[(j*size+3)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset+3];
                        LOG(DEBUG, "\tI'm size 4 - b[0]=0x%02x b[1]=0x%02x", tmp_buffer[j][(i)*size+offset], tmp_buffer[j][(i)*size+offset+1]);
                    }
                    else if (size == 2)
                    {
                        SCP[dst+i][j*2/L1_SCP_BLOCK_SIZE].b[(j*size)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset];
                        SCP[dst+i][j*2/L1_SCP_BLOCK_SIZE].b[(j*size+1)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset+1];
                        LOG(DEBUG, "\tI'm size 2 - b[0]=0x%02x b[1]=0x%02x", tmp_buffer[j][(i)*size+offset], tmp_buffer[j][(i)*size+offset+1]);
                    }
                    else if (size == 1)
                    {
                        SCP[dst+i][j/L1_SCP_BLOCK_SIZE].b[(j*size)%L1_SCP_BLOCK_SIZE] = tmp_buffer[j][(i)*size+offset];
                        LOG(DEBUG, "\tI'm size 1 - b[0]=0x%02x b[1]=0x%02x", tmp_buffer[j][dst+(i)*size+offset], tmp_buffer[j][dst+(i)*size+offset+1]);
                    }
                    else
                    {
                        LOG(DEBUG, "ERROR Tensor Load element size not valid!!");
                        update_tensor_error(1 << 1);
                        return;
                    }

                }
                for (int x = 0; x < L1_SCP_BLOCKS; ++x)
                {
                    for (int y = 0; y < VL; ++y)
                    {
                        LOG(DEBUG, "SCP[%d][%d].u[%d] = 0x%08x", dst+i, x, y, SCP[dst+i][x].u[y]);
                    }
                }
            }

        }
    }
    int op = 0;
    if (trans == 0x05 || trans == 0x06 || trans==0x07)
	op = 1;
    else if (trans == 0x01 || trans == 0x02)
	op = 2;
    logtensorchange(op);
}

// ----- TensorLoadL2Scp emulation --------------------------------------------------

void tensorloadl2(uint64_t control)//TranstensorloadL2
{
    uint64_t stride  = XREGS[31].x & 0xFFFFFFFFFFC0ULL;

    int      tm      = (control >> 63) & 0x1;
    int      dst     = ((control >> 46) & 0x1FFFC)  + ((control >> 4)  & 0x3);
    uint64_t base    = control & 0xFFFFFFFFFFC0ULL;
    int      rows    = ((control     ) & 0xF) + 1;
    uint64_t addr    = base;

    LOG(DEBUG, "Tensor Load L2 SCP:  rows:%d - tm:%d - dst:%d -  addr:0x%16" PRIx64, rows, tm,  dst,  addr);

    uint64_t shire   = current_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);

    for (int i  = 0; i < rows; ++i)
    {
        uint64_t paddr_line_base =  L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * 64);
        if (!tm || tmask_pass(i))
        {
            if (addr & 0x3F)
            {
                LOG(DEBUG, "ERROR Tensor Load not aligned to cache line!!");
            }
            for (int j = 0; j < L1_SCP_LINE_SIZE/4; j++)
            {
                uint64_t addr_offset  = j*4;
                uint64_t addr_final = addr+addr_offset;
                uint32_t val = vmemread32(addr_final);
                uint64_t paddr = paddr_line_base + addr_offset ;
                emu_pmemwrite32(paddr, val);
                LOG(DEBUG, "\t tensor load L2 SCP VA_MEM[%016" PRIx64 "] to  P_MEM[%016" PRIx64 "] line %d, base 0x%016" PRIx64 "  offset 0x%016" PRIx64 " <= 0x%08x (%d)", addr_final,  paddr , dst+i, paddr_line_base , addr_offset , val, val);
            }
        }
        LOG(DEBUG, "\t\tVA Address = 0x%016" PRIx64 " P Address = 0x%016" PRIx64 "  - Stride = 0x%016" PRIx64 "- line %d", addr, paddr_line_base , stride,  dst+i);
        addr += stride;
    }
}

// ----- TensorQuant emulation -------------------------------------------------

static void tensorquant(uint64_t value)
{
    int regstart =  (value & 0x3E00000000000000) >> 57;      // Start register to operate
    int cols     = ((value & 0x0180000000000000) >> 55) + 1; // Number of register per col
    int rows     = ((value & 0x0078000000000000) >> 51) + 1; // Number of rows to store
    int scpsrc   =  (value & 0x0007E00000000000) >> 45;      // Scratchpad source where data is

    // Array that converts an integer to string to print transformation information
    char trans_int_to_str[32][128] = { "LAST",
                                       "INT32 to FP32",
                                       "FP32 to INT32",
                                       "INT32 ReLU",
                                       "INT32 add row-wise",
                                       "INT32 add col-wise",
                                       "FP32 mul row-wise",
                                       "FP32 mul col-wise",
                                       "INT8 saturate",
                                       "UINT8 saturate",
                                       "Pack to 128b" };

    tensorquant_size[current_thread] = (cols / 2) * rows;
    tensorquant_trans[current_thread] = 0;

    // Gets all the transformations done by the tensorquant operation
    uint64_t transformations[TQUANT_MAX_TRANS];
    for (int i = 0; i < TQUANT_MAX_TRANS; i++)
        transformations[i] = (value >> (i * 4)) & 0xF;

    LOG(DEBUG, "\tStart Tensor Quant with scratchpad: %d, rows: %d, cols: %d, regstart: %d", scpsrc, rows, cols, regstart);
    for (int trans = 0; trans < TQUANT_MAX_TRANS; trans++)
    {
        tensorquant_is_pack[current_thread][trans] = (transformations[trans] >= 10);
        LOG(DEBUG, "\t\tTransformation %d: %s", trans, trans_int_to_str[transformations[trans]]);
        if (transformations[trans] == 0) break;
        tensorquant_trans[current_thread]++;
        bool scp_inc = false;

        if (transformations[trans] == 4 || transformations[trans] == 5 ||
            transformations[trans] == 6 || transformations[trans] == 7)
        {
            // Check if SCP is enabled
            // Dissabled until software update
            // if (!(csrregs[current_thread][csr_scratchpad_ctrl] & 0x1))
            // {
            //     update_tensor_error(1 << 4);
            //     return;
            // }
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            // For all the blocks of 128b
            for (int col = 0; col < cols; col++)
            {
                // For all the 32 elements of the 128b block
                for (int elem = 0; elem < 4; elem++)
                {
                    int src = regstart + row * 2 + (col / 2);
                    int idx = (col & 1) * 4 + elem;
                    iufval32 val, val2, res;
                    val.u = FREGS[src].u[idx];
                    res.u = val.u;

                    // INT32 to FP32
                    if (transformations[trans] == 1)
                    {
                        res.f = fpu::i32_to_f32(val.i);
                        LOG(DEBUG, "\tf%d[%d] 0x%08x (%g) <-- 0x%08x (%d)", src, idx, res.u, res.flt, val.u, val.i);
                    }
                    // FP32 to INT32
                    else if (transformations[trans] == 2)
                    {
                        res.i = fpu::f32_to_i32(val.f);
                        LOG(DEBUG, "\tf%d[%d] 0x%08x (%d) <-- 0x%08x (%g)", src, idx, res.u, res.i, val.u, val.flt);
                    }
                    // INT32 ReLU
                    else if (transformations[trans] == 3)
                    {
                        res.i = (val.i < 0) ? 0 : val.i;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x <-- MAX_INT32(0x%08x, 0x00000000)",src,idx,res.i,val.i);
                    }
                    // INT32 add row-wise
                    else if (transformations[trans] == 4)
                    {
                        int col_pos = col * 4 + elem;
                        val2.i = SCP[scpsrc][col_pos / VL].i[col_pos % VL];
                        res.i = val.i + val2.i;
                        scp_inc = true;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x <-- 0x%08x + 0x%08x", src, idx, res.u, val.u, val2.u);
                    }
                    // INT32 add col-wise
                    else if (transformations[trans] == 5)
                    {
                        int row_pos = row;
                        val2.i = SCP[scpsrc][row_pos / VL].i[row_pos % VL];
                        res.i = val.i + val2.i;
                        scp_inc = true;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x <-- 0x%08x + 0x%08x", src, idx, res.u, val.u, val2.u);
                    }
                    // FP32 mul row-wise
                    else if (transformations[trans] == 6)
                    {
                        int col_pos = col * 4 + elem;
                        val2.u = SCP[scpsrc][col_pos / VL].u[col_pos % VL];
                        res.f = fpu::f32_mul(val.f, val2.f);
                        scp_inc = true;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g)", src, idx, res.u, res.flt, val.u, val.flt, val2.u, val2.flt);
                    }
                    // FP32 mul col-wise
                    else if (transformations[trans] == 7)
                    {
                        int row_pos = row;
                        val2.u = SCP[scpsrc][row_pos / VL].u[row_pos % VL];
                        res.f = fpu::f32_mul(val.f, val2.f);
                        scp_inc = true;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x (%g) <-- 0x%08x (%g) * 0x%08x (%g)", src, idx, res.u, res.flt, val.u, val.flt, val2.u, val2.flt);
                    }
                    // INT8 saturate
                    else if (transformations[trans] == 8)
                    {
                        res.i = ((val.i > 127) ? 127 :(val.i < -128 ? -128 : val.i)) & 0x0FF;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x <-- 0x%08x", src, idx, res.u, val.u);
                    }
                    // UINT8 saturate
                    else if (transformations[trans] == 9)
                    {
                        res.u = ((val.i > 255) ? 255u :(val.i < 0 ? 0u : val.u)) & 0x0FFu;
                        LOG(DEBUG, "\tf%d[%d] 0x%08x <-- 0x%08x", src, idx, res.u, val.u);
                    }
                    // Pack to 128b
                    else
                    {
                        // Only first col gets updated
                        if ((col == 0) && ((cols >= 2) || (elem < 2)))
                        {
                            for (int elem_pack = 0; elem_pack < 4; elem_pack++)
                            {
                                int src_pack = regstart + row * 2 + (elem * 4 + elem_pack) / VL;
                                int idx_pack = (elem * 4 + elem_pack) % VL;
                                int byte_offset = elem_pack;
                                iufval32 val_pack;
                                val_pack.u = FREGS[src_pack].u[idx_pack];

                                // Clears the byte offset
                                res.i = res.i & ~(0xFF << (byte_offset * 8));
                                // Puts the byte value
                                res.i = res.i | ((val_pack.u & 0xFF) << (byte_offset * 8));
                            }
                            LOG(DEBUG, "\tf%d[%d] 0x%08x <-- 0x%08x", src, idx, res.u, val.u);
                        }
                    }

                    // Updates RF
                    FREGS[src].u[idx] = res.u;

                    // Checker
                    tensorquant_data[current_thread][src][idx][trans] = res.u;
                }
            }
        }
        if (scp_inc) scpsrc = (scpsrc + 1) % L1_SCP_ENTRIES;
    }
}

uint32_t get_tensorquant_value(int entry, int transform, int lane, int * size, int * transforms, bool * is_pack)
{
    * size       = tensorquant_size[current_thread];
    * transforms = tensorquant_trans[current_thread];
    * is_pack    = tensorquant_is_pack[current_thread][transform];
    return tensorquant_data[current_thread][entry][lane][transform];
}

// ----- TensorStore emulation -------------------------------------------------

static void tensorstore(uint64_t tstorereg)
{
    uint64_t tstore_scp = (tstorereg >> 48) & 0x1;

    if (tstore_scp)
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000C) >> 62) + 1; // Increment done to scratchpad source
        int      scpstart =  (tstorereg & 0x3F00000000000000) >> 56;      // Start scratchpad entry to store
        int      rows     = ((tstorereg & 0x0078000000000000) >> 51) + 1; // Number of rows to store
        uint64_t addr     =  (tstorereg & 0x00FFFFFFFFFFC0);              // Address where to store the results

        uint64_t stride   = XREGS[31].x & 0xFFFFFFFFFFFFUL;

        int src = scpstart % L1_SCP_ENTRIES;
        LOG(DEBUG, "\tStart Tensor Store Scp with addr: %016" PRIx64 ", stride: %016" PRIx64 ", rows: %d, scpstart: %d, srcinc: %d", addr, stride, rows, src, srcinc);

        // // check if L1 SCP is enabled
        // // Disabled until software update
        // if (!(csrregs[current_thread][csr_scratchpad_ctrl] & 0x1))
        // {
        //     update_tensor_error(1 << 4);
        //     return;
        // }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            // For all the elements of the lane
            for (int j = 0; j < L1_SCP_BLOCKS; j++)
            {
                for (int i = 0; i < VL; i++)
                {
                    uint32_t val = SCP[src][j].u[i];
                    uint64_t waddr = addr + j * VL * 4 + i * 4;
                    try
                    {
                        vmemwrite32(waddr, val);
                    }
                    catch (const trap_t& t)
                    {
                        // Memory exception
                        update_tensor_error(1 << 7);
                        return;
                    }
                    LOG(DEBUG, "\t0x%08x --> MEM[0x%016" PRIx64 "]", val, waddr);
                    LOG(DEBUG, "\t\tSCP[%d][%d].u[%d]", src, j, i);
                    //logmemwchange(0, 4, waddr, val); => Don't log mem changes!
                }
            }
            src += srcinc;
            src = src % L1_SCP_ENTRIES;
            addr += stride;
        }
    }
    else
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000C) >> 62) + 1; // Increment done to register source
        int      regstart =  (tstorereg & 0x3E00000000000000) >> 57;      // Start register to store
        int      cols     = ((tstorereg & 0x0180000000000000) >> 55) + 1; // Number of register per col
        int      rows     = ((tstorereg & 0x0078000000000000) >> 51) + 1; // Number of rows to store
        int      coop     = ((tstorereg & 0x0006000000000000) >> 49) + 1; // Number of cooperative minions
        uint64_t addr     =  (tstorereg & 0x0000FFFFFFFFFFF0);            // Address where to store the results

        uint64_t stride   = XREGS[31].x & 0xFFFFFFFFFFF0UL;

        LOG(DEBUG, "\tStart Tensor Store with addr: %016" PRIx64 ", stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, srcinc: %d, coop: %d",
            addr, stride, regstart, rows, cols, srcinc, coop);

        int src = regstart;

        // Check legal coop combination
        // xs[50:49]/xs[56:55]
        const char coop_comb[4*4] = {
            1, 1, 0, 1,
            1, 1, 0, 0,
            0, 0, 0, 0,
            1, 0, 0, 0
        };

        if (!coop_comb[4*(coop-1)+(cols-1)])
        {
            update_tensor_error(1 << 8);
            return;
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            // For all the blocks of 128b
            for (int col = 0; col < cols; col++)
            {
                // For all the 32 elements of the 128b block
                for (uint64_t i = 0; i < 4; i++)
                {
                    uint32_t idx = (col & 1) * 4 + i;
                    uint32_t val = FREGS[src].u[idx];
                    try
                    {
                        vmemwrite32(addr + col * 16 + i * 4, val);
                    }
                    catch (const trap_t& t)
                    {
                        // Memory exception
                        update_tensor_error(1 << 7);
                        return;
                    }
                    LOG(DEBUG, "\t0x%08x --> MEM[0x%016" PRIx64 "]", val, addr + col * 16 + i * 4);
                    //logmemwchange(0, 4, addr + col * 16 + i * 4, val); => Don't log mem changes!
                }
                if (cols == 1)    src += srcinc; // For 128b stores, move to next desired register
                else if (col & 1) src += srcinc; // For 256b and 512b stores, move to next desired register when 256b are written
            }
            addr += stride;
        }
    }
}

// ----- TensorFMA emulation ---------------------------------------------------

static void tensorfma(uint64_t tfmareg)
{
    int tm         = (tfmareg & 0x8000000000000000) >> 63; // Is a Conv2D operation (use tensor conv register)
    int bcols      = (tfmareg & 0x0180000000000000) >> 55; // Number of B cols to be processed
    int arows      = (tfmareg & 0x0078000000000000) >> 51; // Number of A rows to be processed
    int acols      = (tfmareg & 0x0007800000000000) >> 47; // Number of A cols to be processed
    int aoffset    = (tfmareg & 0x0000780000000000) >> 43; // A matrix 32b offset
    int tenc_to_rf = (tfmareg & 0x0000000000800000) >> 23; // Store TIMA results in VPU RF (IMA only)
    int ub         = (tfmareg & 0x0000000000400000) >> 22; // Matrix B is unsigned (IMA only)
    int ua         = (tfmareg & 0x0000000000200000) >> 21; // Matrix A is unsigned (IMA only)
    int tenb       = (tfmareg & 0x0000000000100000) >> 20; // B is stored in TENB and not in SCP
    int bstart     = (tfmareg & 0x00000000000FF000) >> 12; // SCP entry where B is stored
    int astart     = (tfmareg & 0x0000000000000FF0) >>  4; // SCP entry where A is stored
    int type       = (tfmareg & 0x000000000000000E) >>  1; // Mode: 00 => FP32 | 01 => *FP16+FP32 | 10 => FP16 | 11 => *INT8+INT32
    int first_pass = (tfmareg & 0x0000000000000001);       // Doing a first pass op (do MUL instead of FMA)

    // Decodes fields
    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

    set_rounding_mode(rmdyn);
    clear_arithmetic_flags();

    LOG(DEBUG, "\tStart Tensor FMA with tm: %d, aoffset: %d, Type: %d, First pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc_to_rf: %d, tenb: %d, bstart: %d, astart: %d, rm: %s", tm, aoffset, type, first_pass, bcols, acols, arows, ub, ua, tenc_to_rf, tenb, bstart, astart, get_rounding_mode(rmdyn));

    // //  check if L1 SCP is enabled
    // // Disabled until software update - JIRA RTLMIN-2096
    // if (!(csrregs[current_thread][csr_scratchpad_ctrl] & 0x1))
    // {
    //     update_tensor_error(1 << 4);
    //     return;
    // }

    // In case of loading data straight to tenb, we fake it by writing at position 64 and forth (not accessible otherwise)
    if (tenb)
    {
        bstart = L1_SCP_ENTRIES;
        if (!tensorload_setupb_topair[current_thread] || (tensorload_setupb_numlines[current_thread] != acols))
        {
            // No TensorLoad to pair or incompatible combination of rows and columns length
            update_tensor_error(1 << 6);
            return;
        }
    }
    // Unpair a paired TensorLoad
    tensorload_setupb_topair[current_thread] = false;
    tensorload_setupb_topair[current_thread^1] = false;

    tensorfma_size[current_thread] = arows * bcols / VL;
    tensorfma_passes[current_thread] = acols;

    // No mask skip by default
    for (int i = 0; i < TFMA_MAX_ACOLS; i++)
    {
        for (int j = 0; j < TFMA_MAX_AROWS; j++)
        {
            tensorfma_mask_skip[i][j] = 0;
        }
    }

    // No zero skip by default
    for (int i = 0; i < TFMA_MAX_ACOLS; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            for (int k = 0; k < VL; k++)
            {
                tensorfma_zero_skip[i][j][k] = 0;
            }
        }
    }

    // FP32 flow
    if (type == 0)
    {
        if (first_pass)
        {
            for (int ar = 0; ar < arows; ar++)
            {
                for (int bc = 0; bc < bcols; bc++)
                {
                    int bf = bc / VL;
                    int bm = bc % VL;

                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = 0;
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][0] = 0;
                }
            }
        }

        for (int ar = 0; ar < arows; ar++)              // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if (tm)
            {
                if (!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for (int i = 0; i < TFMA_MAX_ACOLS; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if (first_pass)
                    {
                        tensorfma_mask_skip[0][ar] = 0;
                    }
                    continue;
                }
            }

            for (int bc = 0; bc < bcols; bc++)          // B: process bcols cols
            {
                int bf = bc / VL;
                int bm = bc % VL;

                for (int ac = 0; ac < acols; ac++)      // A: traverse acols cols
                {
                    iufval32 accum, mul_a, mul_b, res;

                    int af = (aoffset + ac) / VL;
                    int am = (aoffset + ac) % VL;
                    int br = bstart + ac;               // B: traverse acols rows

                    accum.u = FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];
                    mul_a.u = SCP[astart+ar][af].u[am];
                    mul_b.u = SCP[br][bf].u[bm];
                    res.f = fpu::f32_mulAdd(mul_a.f, mul_b.f, accum.f);
                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res.u;
                    LOG(DEBUG, "\tTensor FMA f%d[%d]: %g = %g + %g * %g", TFMA_MAX_BCOLS/VL*ar+bf, bm, res.flt, accum.flt, mul_a.flt, mul_b.flt);
                    LOG(DEBUG, "\t           f%d[%d]: 0x%08x = 0x%08x + 0x%08x * 0x%08x", TFMA_MAX_BCOLS/VL*ar+bf, bm, res.u, accum.u, mul_a.u, mul_b.u);
                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][ac] = res.u;

                    if ((first_pass == 0) || (ac != 0))
                    {
                        // If As are zeroes, we skip operation
                        if (mul_a.u == 0)
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                        // If Bs are zeroes, we skip operation
                        if (mul_b.u == 0)
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                    }
                }
            }
            LOG(DEBUG, "\tC row %d: f%d[%d] = 0x%08x (%g)", ar, TFMA_MAX_BCOLS/VL*ar+0, 0, FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0], cast_uint32_to_float(FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0]));
            for (int reg = 0; reg < 2; reg++)
                for (int lane = 0; lane < VL; lane++)
                    if ((reg != 0) || (lane != 0))
                        LOG(DEBUG, "\t         f%d[%d] = 0x%08x (%g)",    TFMA_MAX_BCOLS/VL*ar+reg, lane, FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane], cast_uint32_to_float(FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane]));
        }
    }
    // *FP16+FP32
    else if (type == 1)
    {
        if (first_pass)
        {
            for (int ar = 0; ar < arows; ar++)          // A: traverse arows rows
            {
                for (int bc = 0; bc < bcols; bc++)      // B: process bcols cols
                {
                    int bf = bc / VL;
                    int bm = bc % VL;

                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = 0;
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][0] = 0;
                }
            }
        }

        for (int ar = 0; ar < arows; ar++)              // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if (tm)
            {
                if (!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for (int i = 0; i < TFMA_MAX_ACOLS; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if (first_pass)
                        tensorfma_mask_skip[0][ar] = 0;
                    continue;
                }
            }

            for (int bc = 0; bc < bcols; bc++)          // B: process bcols cols
            {
                int bf = bc / VL;
                int bm = bc % VL;

                for (int ac = 0; ac < acols; ac++)      // A: accumulate acols values
                {
                    int af = (aoffset + ac) / VL;
                    int am = (aoffset + ac) % VL;
                    int br = bstart + ac;               // B: traverse rows


                    // Doing two FMAs per lane and accumulating to previous results
                    iufval32 accum, res;
                    accum.u = FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];

                    uint16_t a1 = SCP[astart+ar][af].h[am * 2];       // get first operand
                    uint16_t a2 = SCP[astart+ar][af].h[am * 2 + 1];   // get third operand
                    uint16_t b1 = SCP[br][bf].h[bm * 2];              // get second operand
                    uint16_t b2 = SCP[br][bf].h[bm * 2 + 1];          // get fourth operand

                    res.f = fpu::f32_tensorMulAddF16(accum.f, fpu::F16(a1), fpu::F16(b1), fpu::F16(a2), fpu::F16(b2));
                    FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res.u;

                    float fa1 = fpu::FLT( fpu::f16_to_f32(fpu::F16(a1)) );
                    float fb1 = fpu::FLT( fpu::f16_to_f32(fpu::F16(b1)) );
                    float fa2 = fpu::FLT( fpu::f16_to_f32(fpu::F16(a2)) );
                    float fb2 = fpu::FLT( fpu::f16_to_f32(fpu::F16(b2)) );
                    LOG(DEBUG, "\tTensor FMA f%d[%d]: %g = %g + (%g * %g) + (%g * %g)", TFMA_MAX_BCOLS/VL*ar+bf,bm,res.flt,accum.flt,fa1,fb1,fa2,fb2);
                    LOG(DEBUG, "\t           f%d[%d]: 0x%08x = 0x%08x + (0x%04x * 0x%04x) + (0x%04x * 0x%04x)", TFMA_MAX_BCOLS/VL*ar+bf,bm,res.u,accum.u,a1,b1,a2,b2);

                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][ac] = FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];

                    if ((first_pass == 0) || (ac != 0))
                    {
                        // If both As are zeroes, we skip operation
                        if ((a1 == 0) && (a2 == 0))
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                        // If both Bs are zeroes, we skip operation
                        if ((b1 == 0) && (b2 == 0))
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                    }
                }
            }
            LOG(DEBUG, "\tC row %d: f%d[%d] = 0x%08x (%g)", ar, TFMA_MAX_BCOLS/VL*ar+0, 0, FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0], cast_uint32_to_float(FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0]));
            for (int reg = 0; reg < 2; reg++)
                for (int lane = 0; lane < VL; lane++)
                    if ((reg != 0) || (lane != 0))
                        LOG(DEBUG, "\t         f%d[%d] = 0x%08x (%g)",    TFMA_MAX_BCOLS/VL*ar+reg, lane, FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane], cast_uint32_to_float(FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane]));
        }
    }
    else if (type == 3) //INT8-INT32
    {
        if (first_pass)
        {
            for (int ar = 0; ar < arows; ar++)          // A: traverse arows rows
            {
                for (int bc = 0; bc < bcols; bc++)      // B: process bcols cols
                {
                    int bf = bc / VL;
                    int bm = bc % VL;

                    tensorfma_tenc[current_thread][TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = 0;
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][0] = 0;
                }
            }
        }

        for (int ar = 0; ar < arows; ar++)              // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if (tm)
            {
                if (!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for (int i = 0; i < TFMA_MAX_ACOLS; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if (first_pass)
                        tensorfma_mask_skip[0][ar] = 0;

                    // If writing to VPU RF, need to write data
                    if (tenc_to_rf)
                    {
                        for (int bc = 0; bc < bcols; bc++) // For all the Cols
                        {
                            int bf = bc / VL;
                            int bm = bc % VL;

                            if (first_pass)
                            {
                                // Write 0s
                                FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = 0;
                            }
                            else
                            {
                                // Write TENC to VPU RF
                                int32_t accum = tensorfma_tenc[current_thread][TFMA_MAX_BCOLS/VL*ar+bf].u[bm];
                                FREGS[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = accum;
                            }
                        }
                        LOG(DEBUG, "\tC row %d: f%d[%d] = 0x%08x (%d)", ar, TFMA_MAX_BCOLS/VL*ar  ,0, FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0], FREGS[TFMA_MAX_BCOLS/VL*ar+0].u[0]);
                        for (int reg = 0; reg < 2; reg++)
                            for (int lane = 0; lane < VL; lane++)
                                if ((reg != 0) || (lane != 0))
                                    LOG(DEBUG, "\t         f%d[%d] = 0x%08x (%d)",    TFMA_MAX_BCOLS/VL*ar+reg, lane, FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane], FREGS[TFMA_MAX_BCOLS/VL*ar+reg].u[lane]);
                    }
                    continue;
                }
            }

            fdata * tensor_dest = (fdata *) &FREGS;
            char str[256] = "";
            for (int bc = 0; bc < bcols; bc++)          // B: process bcols cols
            {
                int bf = bc / VL;
                int bm = bc % VL;

                for (int ac = 0; ac < acols; ac++)      // A: accumulate acols values
                {
                    int af = (aoffset + ac) / VL;
                    int am = (aoffset + ac) % VL;
                    int br = bstart + ac;               // B: traverse rows

                    // If in last pass and dumping results to VPU RF, then the destination is the VPU RF
                    if ((ac == (acols - 1)) && tenc_to_rf)
                    {
                        tensor_dest = (fdata *) &FREGS;
                        strcpy(str, "");
                    }
                    else
                    {
                        tensor_dest = (fdata *) &tensorfma_tenc[current_thread];
                        strcpy(str, "TENC_");
                    }
                    // Doing four IMAs per lane and accumulating to previous results
                    int32_t accum     = tensorfma_tenc[current_thread][TFMA_MAX_BCOLS/VL*ar+bf].u[bm];

                    // 1st IMA
                    int32_t  mul_a    = ua ? SCP[astart+ar][af].b[am * 4] : sext8_2 (SCP[astart+ar][af].b[am * 4]);
                    int32_t  mul_b    = ub ? SCP[br][bf].b[bm * 4]        : sext8_2 (SCP[br][bf].b[bm * 4]);
                    int32_t  res_mul  = mul_a * mul_b;
                    int32_t  res      = res_mul + accum;

                    tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res;

                    LOG(DEBUG, "\tTensor IMA %sf%d[%d]: %d = %d + %d * %d", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, accum, mul_a, mul_b);
                    LOG(DEBUG, "\t           %sf%d[%d]: 0x%08x = 0x%08x + 0x%02x * 0x%02x", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, * ((int *) &accum), mul_a, mul_b);

                    // 2nd IMA
                    mul_a    = ua ? SCP[astart+ar][af].b[am * 4 + 1] : sext8_2 (SCP[astart+ar][af].b[am * 4 + 1]);
                    mul_b    = ub ? SCP[br][bf].b[bm * 4 + 1]        : sext8_2 (SCP[br][bf].b[bm * 4 + 1]);
                    accum    = tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;

                    tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res;

                    LOG(DEBUG, "\tTensor IMA %sf%d[%d]: %d = %d + %d * %d", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, accum, mul_a, mul_b);
                    LOG(DEBUG, "\t           %sf%d[%d]: 0x%08x = 0x%08x + 0x%02x * 0x%02x", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, * ((int *) &accum), mul_a, mul_b);

                    // 3rd IMA
                    mul_a    = ua ? SCP[astart+ar][af].b[am * 4 + 2] : sext8_2 (SCP[astart+ar][af].b[am * 4 + 2]);
                    mul_b    = ub ? SCP[br][bf].b[bm * 4 + 2]        : sext8_2 (SCP[br][bf].b[bm * 4 + 2]);
                    accum    = tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;

                    tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res;

                    LOG(DEBUG, "\tTensor IMA %sf%d[%d]: %d = %d + %d * %d", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, accum, mul_a, mul_b);
                    LOG(DEBUG, "\t           %sf%d[%d]: 0x%08x = 0x%08x + 0x%02x * 0x%02x", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, * ((int *) &accum), mul_a, mul_b);

                    // 4th IMA
                    mul_a    = ua ? SCP[astart+ar][af].b[am * 4 + 3] : sext8_2 (SCP[astart+ar][af].b[am * 4 + 3]);
                    mul_b    = ub ? SCP[br][bf].b[bm * 4 + 3]        : sext8_2 (SCP[br][bf].b[bm * 4 + 3]);
                    accum    = tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;

                    tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm] = res;

                    LOG(DEBUG, "\tTensor IMA %sf%d[%d]: %d = %d + %d * %d", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, accum, mul_a, mul_b);
                    LOG(DEBUG, "\t           %sf%d[%d]: 0x%08x = 0x%08x + 0x%02x * 0x%02x", str, TFMA_MAX_BCOLS/VL * ar + bf, bm, res, * ((int *) &accum), mul_a, mul_b);

                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][TFMA_MAX_BCOLS/VL*ar+bf][bm][ac] = tensor_dest[TFMA_MAX_BCOLS/VL*ar+bf].u[bm];

                    bool do_not_zero_skip = ((first_pass == 1) && (ac == 0))            // Can't skip for first pass and first acol
                                         || ((tenc_to_rf == 1) && (ac == (acols - 1))); // Can't skip for TENC write to RF and last acol
                    if (!do_not_zero_skip)
                    {
                        // If As are zeroes, we skip operation
                        if ((SCP[astart+ar][af].b[am * 4] == 0) && (SCP[astart+ar][af].b[am * 4 + 1] == 0) && (SCP[astart+ar][af].b[am * 4 + 2] == 0) && (SCP[astart+ar][af].b[am * 4 + 3] == 0))
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                        // If Bs are zeroes, we skip operation
                        if ((SCP[br][bf].b[bm * 4] == 0) && (SCP[br][bf].b[bm * 4 + 1] == 0) && (SCP[br][bf].b[bm * 4 + 2] == 0) && (SCP[br][bf].b[bm * 4 + 3] == 0))
                            tensorfma_zero_skip[ac][TFMA_MAX_BCOLS/VL*ar+bc/VL][bc%VL] = 1;
                    }
                }
            }

            LOG(DEBUG, "\tC row %d: %sf%d[%d] = 0x%08x (%d)", ar, str, TFMA_MAX_BCOLS/VL*ar  ,0, tensor_dest[TFMA_MAX_BCOLS/VL*ar+0].u[0], tensor_dest[TFMA_MAX_BCOLS/VL*ar+0].u[0]);
            for (int reg = 0; reg < 2; reg++)
                for (int lane = 0; lane < VL; lane++)
                    if ((reg != 0) || (lane != 0))
                        LOG(DEBUG, "\t         %sf%d[%d] = 0x%08x (%d)",    str, TFMA_MAX_BCOLS/VL*ar+reg, lane, tensor_dest[TFMA_MAX_BCOLS/VL*ar+reg].u[lane], tensor_dest[TFMA_MAX_BCOLS/VL*ar+reg].u[lane]);
        }
    }
    else
    {
        LOG(DEBUG, "ERROR Unimplemented tensor FMA Type!!");
    }
    if (type != 3)
    {
        set_fp_exceptions();
        dirty_fp_state();
    }
}

uint32_t get_tensorfma_value(int entry, int pass, int lane, int * size, int * passes, bool * mask_skip)
{
    * size      = tensorfma_size[current_thread];
    * passes    = tensorfma_passes[current_thread];
    * mask_skip = tensorfma_mask_skip[pass][entry / TFMA_REGS_PER_ROW] || tensorfma_zero_skip[pass][entry][lane];
    return tensorfma_data[current_thread][entry][lane][pass];
}

// ----- TensorReduce emulation ------------------------------------------------

static void tensorreduce(uint64_t value)
{
    uint64_t other_min;
    uint64_t action;

    get_reduce_info(value, &other_min, &action);

    reduce_size[current_thread] = 0;

    // Do nothing
    if (action == 2) return;
    // Send
    if (action == 0) return;
    // Receive

    //op = rs[35:32]
    int      start_reg = (value >> 57) & 0x1F;
    uint32_t operation = (value >> 24) & 0xF;
    int      num_reg   = (value >> 16) & 0xFF;

    // Info for checker
    reduce_size[current_thread]  = num_reg;
    reduce_entry[current_thread] = start_reg;

    // Sending and receiving from the same minion
    if (other_min == (current_thread>>1))
    {
        update_tensor_error(1 << 9);
        return;
    }

    clear_arithmetic_flags();
    if (operation == 0) // FADD
    {
        set_rounding_mode(rmdyn);
        LOG(DEBUG, "\tReduce (fadd) with rounding mode: %s", get_rounding_mode(rmdyn));
    }
    for (int i = 0; i < num_reg; i++)
    {
        for (int j = 0; j < VL; j++)
        {
            int op_reg = (i + start_reg) % 32;
            if (operation == 0) // FADD
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u[j];
                src2.u = fregs[other_min<<1][op_reg].u[j];
                rslt.f = fpu::f32_add(src1.f, src2.f);
                FREGS[op_reg].u[j] = rslt.u;
                LOG(DEBUG, "\tReduce (fadd) f%d[%d]: %g = %g(m%d) + %g(m%" PRId64 ")",op_reg,j,rslt.flt,src1.flt,current_thread>>1,src2.flt,other_min);
                LOG(DEBUG, "\t              f%d[%d]: 0x%08x = 0x%08x + 0x%08x",op_reg,j,rslt.u,src1.u,src2.u);
            }
            else if (operation == 2) // FMAX
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u[j];
                src2.u = fregs[other_min<<1][op_reg].u[j];
                rslt.f = fpu::f32_maxNum(src1.f,src2.f);//src1.u > src2.u ? src1.u : src2.u;
                FREGS[op_reg].u[j] = rslt.u;
                LOG(DEBUG, "\tReduce (fmax) f%d[%d]: %g = %g(m%d) > %g(m%" PRId64 ")",op_reg,j,rslt.flt,src1.flt,current_thread>>1,src2.flt,other_min);
                LOG(DEBUG, "\t              f%d[%d]: 0x%08x = 0x%08x > 0x%08x",op_reg,j,rslt.u,src1.u,src2.u);
            }
            else if (operation == 4) // IADD
            {
                iufval32 src1, src2, rslt;
                src1.u = FREGS[op_reg].u[j];
                src2.u = fregs[other_min<<1][op_reg].u[j];
                rslt.u = src1.u + src2.u;
                FREGS[op_reg].u[j] = rslt.u;
                LOG(DEBUG, "\tReduce (iadd) f%d[%d]: %d = %d(m%d) + %d(m%" PRId64 ")",op_reg,j,rslt.u,src1.u,current_thread>>1,src2.u,other_min);
                LOG(DEBUG, "\t              f%d[%d]: 0x%08x = 0x%08x + 0x%08x",op_reg,j,rslt.u,src1.u,src2.u);
            }
            else if (operation == 8) // FGET
            {
                iufval32 tmp;
                tmp.u = fregs[other_min<<1][op_reg].u[j];
                FREGS[op_reg].u[j] = tmp.u;
                LOG(DEBUG, "\tReduce (get) f%d[%d]: <= %g(m%" PRId64 ")",op_reg,j,tmp.flt,other_min);
                LOG(DEBUG, "\t             f%d[%d]: <= 0x%08x",op_reg,j,tmp.u);
            }
            else
            {
                LOG(DEBUG, "ERROR reduce/broadcast operation = %d not yet supported in emu", operation);
            }

            // Checker
            reduce_data[current_thread][op_reg][j] = FREGS[op_reg].u[j];
        }
    }
    set_fp_exceptions();
    dirty_fp_state();
}

// Helper function that given the written value to the CSR, returns:
//   - what is the ID of the other minion of the reduce
//   - what is the action taken by the minion (send, receive, do nothing)
void get_reduce_info(uint64_t value, uint64_t * other_min, uint64_t * action)
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

uint64_t get_reduce_value(int entry, int block, int * size, int * start_entry)
{
    * size = reduce_size[current_thread];
    * start_entry = reduce_entry[current_thread];
    return reduce_data[current_thread][entry][block];
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
    uint64_t shire   = current_thread / (EMU_MINIONS_PER_SHIRE * EMU_THREADS_PER_MINION);

    // Gets what is the address that the fast local barrier is mapped to
    uint64_t addr    = ESR_SHIRE_REGION + ESR_SHIRE_FLB_OFFSET + (barrier * 8) + shire * ESR_REGION_OFFSET; // Access is private per cache

    uint64_t orig_value = vmemread64(addr);
    uint64_t result = -1;

    LOG_ALL_MINIONS(DEBUG,"FastLocalBarrier: Shire %i: Minion %i Thread %i doing barrier %" PRIu64 " value  %" PRIu64 ", limit %" PRIu64 " ",
        (int) shire, current_thread / EMU_THREADS_PER_MINION, current_thread % EMU_THREADS_PER_MINION, barrier, orig_value, limit);
    // Last guy, return 1 and zero barrier
    if (orig_value == limit)
    {
        LOG_ALL_MINIONS(DEBUG,"FastLocalBarrier: last minion Shire %i!!", (int) shire);
        vmemwrite64(addr, 0);
        result = 1;
    }
    // Not the last guy, return 0 and increment barrier
    else
    {
        LOG_ALL_MINIONS(DEBUG, "FastLocalBarrier: Limit %" PRIu64", Incrementing to %" PRIu64 "!!", limit, orig_value + 1);
        vmemwrite64(addr, orig_value + 1);
        result = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// determine if instruction needs to trap if txfma is off
//
////////////////////////////////////////////////////////////////////////////////

bool txfma_off_allowed(csr src1, uint64_t val) {

    // if txfma is not sleep, allow
    if (csrregs[current_thread][csr_msleep_txfma_27] == 0)
        return true;

    // and for each csr, allow if  corresponding bit in txfma_sleep_traps is 0
    // and do not allow if using the txfma
    uint32_t trap_conf = csrregs[current_thread][csr_mtxfma_sleep_traps];
    switch (src1)
    {
        case csr_tensor_fma:
            if (((trap_conf >> 4) & 1) == 0) return true;
            else return ((val & 0xE) == 6); // only allow for int8

        case csr_tensor_quant:
            if ((( trap_conf >> 3) & 1) == 0) return true; //trap disabled
            else return false;

        case csr_tensor_reduce:
            if (((trap_conf >> 2) & 1) == 0) return true;
            // allow for int, not allow for fp32
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

    return true;
}
