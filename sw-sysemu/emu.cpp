#include <cstdio>
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <strings.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <list>

#include "emu.h"
#include "cvt.h"
#include "log.h"
#include "ipc.h"
#include "ttrans.h"
#include "emu_casts.h"
#include "emu_gio.h"

using std::fpclassify;
using std::signbit;

using emu::gprintf;
using emu::gsprintf;
using emu::gfprintf;

#ifdef GFX_ONLY
 #define GFX(x) x
#else
 #define GFX(x)
#endif

// State declaration
xdata xregs[EMU_NUM_THREADS][32];
fdata fregs[EMU_NUM_THREADS][32];
mdata mregs[EMU_NUM_THREADS][8];
uint64_t csrregs[EMU_NUM_THREADS][CSR_MAX];
fdata scp[EMU_NUM_THREADS][64][4];
int scp_entry[EMU_NUM_THREADS];
int scp_size[EMU_NUM_THREADS];
bool scp_tm;
int tensorfma_size[EMU_NUM_THREADS];
int tensorfma_passes[EMU_NUM_THREADS];
uint32_t tensorfma_data[EMU_NUM_THREADS][32][4][16];
bool tensorfma_mask_skip[16][8];
bool tensorfma_zero_skip[16][32][4];
int reduce_entry[EMU_NUM_THREADS];
int reduce_size[EMU_NUM_THREADS];
uint32_t reduce_data[EMU_NUM_THREADS][32][4];
msg_port_conf msg_ports[EMU_NUM_THREADS][NR_MSG_PORTS];
int32_t msg_ports_pending_offset[EMU_NUM_THREADS][NR_MSG_PORTS];

static uint64_t current_pc;
uint32_t current_thread = 0;

#define MAXSTACK 2048
static uint32_t shaderstack[EMU_NUM_THREADS][MAXSTACK];

bool check_stack = false;
char dis[1024];

int fake_sampler = 0;
#ifdef USE_FAKE_TXFMA
uint8_t emu_use_fake_txfma = 1;
#else
uint8_t emu_use_fake_txfma = 0;
#endif
uint8_t in_sysemu = 0;

void init_emu(int debug, int fakesam)
{
    print_debug  = debug;
    fake_sampler = fakesam;
}

// forward declarations
static uint64_t csrget(csr src1);
static void csrset(csr src1, uint64_t val);
static void tmask_conv();
static void tcoop();
static void tensorload(uint64_t control);
static void tensorstore(uint64_t tstorereg);
static void tensorfma(uint64_t tfmareg);
static void tensorreduce(uint64_t value);
static uint64_t csr_cacheop_emu(uint64_t op_value);
static uint64_t msg_port_csr(uint32_t id, uint64_t wdata, bool umode);
static uint64_t flbarrier(uint64_t value);

////////////////////////////////////////////////////////////////////////////////
//
// Helper routines
//
////////////////////////////////////////////////////////////////////////////////

void print_comment(const char *comm)
{
    DEBUG_EMU(gprintf("// %s\n",comm);)
}

static uint64_t sext32(uint32_t val)
{
    uint32_t s = val & 0x80000000;
    uint64_t r = s ? (0xffffffff00000000ull | val ) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

static uint64_t sext16(uint32_t val)
{
    uint32_t s = val & 0x00008000;
    uint64_t r = s ? (0xffffffffffff0000ull | val ) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

static uint64_t sext12(uint32_t val)
{
    uint32_t s = val & 0x0000800;
    uint64_t r = s ? (0xfffffffffffff000ull | val) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

static uint64_t sext10(uint32_t val)
{
    uint32_t s = val & 0x0000200;
    uint64_t r = s ? (0xfffffffffffffc00ull | val) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

static uint64_t sext8(uint32_t val)
{
    uint32_t s = val & 0x0000080;
    uint64_t r = s ? (0xffffffffffffff00ull | val) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

static int32_t sext8_2(uint8_t val)
{
    uint32_t s = val & 0x80;
    int32_t r = s ? (0xffffff00 | val) : val;
    //DEBUG_EMU(gprintf("\tsext(%d) = %llu | sext(0x%08x) = 0x%016llx\n",val,r,val,r);)
    return r;
}

void init_stack()
{
    gsprintf(dis,"Sorry disassembly disabled\n");
    check_stack = true;
    XREGS[x2].x = (uint64_t)&(shaderstack[current_thread][MAXSTACK-1]);
    DEBUG_EMU(gprintf("init x2.x = 0x%016llx\n",XREGS[x2].x);)
    ipc_init_xreg(x2);
}

void init(xreg dst, uint64_t val)
{
    XREGS[dst].x = val;
    DEBUG_EMU(gprintf("init x%d <- 0x%016llx\n",dst,val);)
    ipc_init_xreg(dst);
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

static float roundnef (float f){
    if ((fabs(f)-0.5 == trunc(fabs(f))) && !((int)fabs(f) % 2))
        if (f>0)
            return roundf(f)-1;
        else
            return roundf(f) +1;
    else
        return roundf(f);
}

static float roundf ( float val, rounding_mode rm) {
    switch (rm) {
        case rne   : return roundnef(val);
        case rtz   : return trunc(val);
        case rdn   : return floor(val);
        case rup   : return ceil(val);
        case rmm   : return roundf(val);
        case rmdyn : return roundf(val, (rounding_mode) csrget(csr_frm) );
    }
    gprintf("invalid rounding mode: %d... using rm register instead\n", rm);
    return roundf(val, (rounding_mode) csrget(csr_frm) );
}

void initcsr(uint32_t thread)
{
    // Exit reset at M-mode
    csrregs[thread][csr_prv] = CSR_PRV_M;
    // Read-only registers
    csrregs[thread][csr_mvendorid] = 0xdeadbeef;
    csrregs[thread][csr_marchid] = 0xdeadbeef;
    csrregs[thread][csr_mimpid] = 0xdeadbeef;
    csrregs[thread][csr_mhartid] = thread;
    // misa is a 0-length register
    csrregs[thread][csr_misa] = 0x800000000014112dULL;
    // M-mode registers with reset
    csrregs[thread][csr_mstatus] = 0x0000000A00001800ULL; // mpp=11, sxl=uxl=10
    csrregs[thread][csr_mcause] = 0x0ULL;
    csrregs[thread][csr_mip] = 0x0ULL;
    csrregs[thread][csr_icache_ctrl] = 0x0ULL;
    csrregs[thread][csr_write_ctrl] = 0x0ULL;
    // Debug-mode registers with reset
    // TODO: csrregs[thread][csr_dcsr] <= xdebugver=1, prv=3;

    // Ports
    if (thread == 0) {
        assert(sizeof(msg_ports) >= EMU_NUM_THREADS*NR_MSG_PORTS*sizeof(msg_port_conf));
        assert(sizeof(msg_ports_pending_offset) >= EMU_NUM_THREADS*NR_MSG_PORTS*sizeof(int32_t));
        bzero(msg_ports, sizeof(msg_ports));
        memset(msg_ports_pending_offset, 0xFF, sizeof(msg_ports_pending_offset));
    }
}

void minit(mreg dst, uint64_t val)
{
    for(int i = 0; i<4; i++)
    {
        MREGS[dst].b[i*2  ] = val & 0x1;
        MREGS[dst].b[i*2+1] = val & 0x1;
        val = val >> 1;
        DEBUG_EMU( gprintf("init m[%d].b[%d] <- %d\n",dst,2*i  ,MREGS[dst].b[2*i]);
                   gprintf("init m[%d].b[%d] <- %d\n",dst,2*i+1,MREGS[dst].b[2*i+1]); )
    }
    ipc_init_mreg(dst);
}

static bool tmask_pass(int bit)
{
    // Returns the pass bit for a specific bit
    return ((csrget(csr_tmask) >> bit) & 1);
}

static uint8_t security_ulp_check(uint32_t gold, uint32_t table)
{
    // Fast skip for zeros and infinity should be the same value in both in gold and table
    if (gold == table)
        return 0;

    // Detect NaNs
    bool gold_is_nan  = ((gold  & 0x7F800000) == 0x7F800000) && ((gold  & 0x007FFFFF) != 0);
    bool table_is_nan = ((table & 0x7F800000) == 0x7F800000) && ((table & 0x007FFFFF) != 0);

    assert((gold_is_nan == table_is_nan) && "Trans mismatch error. Please open a jira to jordi.sola@esperantotech.com.");

    bool gold_is_inf = ((gold == 0xff800000) || (gold == 0x7f800000));

    //printf("GOLD: %d TABLE: %d\n", gold_is_nan, table_is_nan);
    if(gold_is_inf){
        assert((gold == table) && "Trans mismatch error. Please open a jira to jordi.sola@esperantotech.com.");
    }
    // Skip all other tests.
    if (gold_is_nan)
        return 0;

    uint32_t gold_clean = gold & 0x7F800000;     // clean mantissa and sign from gold

    // compute 1ulp from gold
    float err_1ulp = cast_uint32_to_float32(gold_clean);
    err_1ulp = err_1ulp / float (1 << 23); // put '1' in the unit of less precision

    // compute diff between gold and table approximation
    float goldf  = cast_uint32_to_float32(gold);
    float tablef = cast_uint32_to_float32(table);
    float diff = fabsf(goldf - tablef);

    // fail if diff is bigger than 1ulp
    if (diff > err_1ulp) {
        printf("Gold IEEE: %.12e, Table TRANS: %.12e, Diff: %.12e, Max (1ulp): %.12e\n", goldf, tablef, diff, err_1ulp);
        printf("Hex Gold: %08X, Hex Table: %08X\n", gold, table);
    }
    return (diff > err_1ulp);
}

static void trap_to_smode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = csrget(csr_prv);
    assert(curprv <= CSR_PRV_S);

    DEBUG_EMU(gprintf("\tTrapping to S-mode with cause %llu\n",cause);)

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
    logtrap();
    logpcchange(csrget(csr_stvec));
}

static void trap_to_mmode(uint64_t cause, uint64_t val)
{
    // Get current privilege mode
    uint64_t curprv = csrget(csr_prv);

    // Check if we should deletegate the trap to S-mode
    if ((curprv < CSR_PRV_M) && (csrget(csr_medeleg) & (1ull << cause)))
    {
        trap_to_smode(cause, val);
        return;
    }

    DEBUG_EMU(gprintf("\tTrapping to M-mode with cause %llu\n",cause);)

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
    logtrap();
    logpcchange(csrget(csr_mtvec));
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

uint32_t get_mask ( unsigned maskNr )
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

#ifdef CHECKER

extern inst_state_change * log_info;

// Defines the functions to access to the main memory during checker mode
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

uint8_t memread8(uint64_t addr, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    // Used to detect special load accesses like ticketer
    log_info->mem_addr[0] = paddr;
    return func_memread8(paddr);
}

uint16_t memread16(uint64_t addr, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    // Used to detect special load accesses like ticketer
    log_info->mem_addr[0] = paddr;
    return func_memread16(paddr);
}

uint32_t memread32(uint64_t addr, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    // Used to detect special load accesses like ticketer
    log_info->mem_addr[0] = paddr;
    return func_memread32(paddr);
}

uint64_t memread64(uint64_t addr, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Load);
    // Used to detect special load accesses like ticketer
    log_info->mem_addr[0] = paddr;
    return func_memread64(paddr);
}

void memwrite8(uint64_t addr, uint8_t data, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    printf("MEM8 %i, %016" PRIx64 ", %02" PRIx8 ", (%016" PRIx64 ")\n", current_thread, paddr, data, addr);
    func_memwrite8(paddr, data);
}

void memwrite16(uint64_t addr, uint16_t data, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    printf("MEM16 %i, %016" PRIx64 ", %04" PRIx16 ", (%016" PRIx64 ")\n", current_thread, paddr, data, addr);
    func_memwrite16(paddr, data);
}

void memwrite32(uint64_t addr, uint32_t data, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    printf("MEM32 %i, %016" PRIx64 ", %08" PRIx32 ", (%016" PRIx64 ")\n", current_thread, paddr, data, addr);
    func_memwrite32(paddr, data);
}

void memwrite64(uint64_t addr, uint64_t data, bool trans)
{
    uint64_t paddr = addr;
    if(trans) paddr = virt_to_phys_emu(addr, Mem_Access_Store);
    printf("MEM32 %i, %016" PRIx64 ", %016" PRIx64 ", (%016" PRIx64 ")\n", current_thread, paddr, data, addr);
    func_memwrite64(paddr, data);
}

#else

uint8_t memread8(uint64_t addr, bool trans)
{
    return * ((uint8_t *) addr);
}

uint16_t memread16(uint64_t addr, bool trans)
{
    return * ((uint16_t *) addr);
}

uint32_t memread32(uint64_t addr, bool trans)
{
    return * ((uint32_t *) addr);
}

uint64_t memread64(uint64_t addr, bool trans)
{
    return * ((uint64_t *) addr);
}

void memwrite8(uint64_t addr, uint8_t data, bool trans)
{
    * ((uint8_t *) addr) = data;
}

void memwrite16(uint64_t addr, uint16_t data, bool trans)
{
    * ((uint16_t *) addr) = data;
}

void memwrite32(uint64_t addr, uint32_t data, bool trans)
{
    * ((uint32_t *) addr) = data;
}

void memwrite64(uint64_t addr, uint64_t data, bool trans)
{
    * ((uint64_t *) addr) = data;
}

#endif

void set_memory_funcs(void * func_memread8_, void * func_memread16_,
                      void * func_memread32_, void * func_memread64_,
                      void * func_memwrite8_, void * func_memwrite16_,
                      void * func_memwrite32_, void * func_memwrite64_)
{
#ifdef CHECKER
    func_memread8   = (func_memread8_t  ) func_memread8_;
    func_memread16  = (func_memread16_t ) func_memread16_;
    func_memread32  = (func_memread32_t ) func_memread32_;
    func_memread64  = (func_memread64_t ) func_memread64_;
    func_memwrite8  = (func_memwrite8_t ) func_memwrite8_;
    func_memwrite16 = (func_memwrite16_t) func_memwrite16_;
    func_memwrite32 = (func_memwrite32_t) func_memwrite32_;
    func_memwrite64 = (func_memwrite64_t) func_memwrite64_;
#endif
}

// ILLEGAL INSTRUCTION
void unknown(const char* comm)
{
    DISASM(gsprintf(dis,"I: trap_illegal_instruction (%016llx)%s%s",current_pc,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    // TODO: We may want to set mtval/stval to the instructions bits
    trap_to_mmode(CSR_MCAUSE_ILLEGAL_INSTRUCTION, 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64I emulation
//
////////////////////////////////////////////////////////////////////////////////

void beq(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: beq x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(XREGS[src1].x == XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bne(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: bne x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(XREGS[src1].x != XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void blt(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: blt x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if((int64_t) XREGS[src1].x < (int64_t) XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bltu(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: bltu x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if((uint64_t) XREGS[src1].x < (uint64_t) XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bge(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: bge x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if((int64_t) XREGS[src1].x >= (int64_t) XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void bgeu(xreg src1, xreg src2, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: bgeu x%d, x%d, %d%s%s",src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if((uint64_t) XREGS[src1].x >= (uint64_t) XREGS[src2].x)
        logpcchange(current_pc + imm);
}

void c_jalr(xreg dst, xreg src1, int imm, const char* comm)
{
    uint64_t src1_old = XREGS[src1].x; // in case dst == src1
    DISASM(gsprintf(dis,"I: jalr x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = current_pc + 2;
        DEBUG_EMU(gprintf("\t0x%016llx <- \n",XREGS[dst].x);)
    }
    logxregchange(dst);
    logpcchange((src1_old + imm) & 0xFFFFFFFFFFFFFFFE);
}

void c_jal(xreg dst, int imm, const char* comm)
{
    // NB: spike-dasm already multiplies the immediate operand by 2
    DISASM(gsprintf(dis,"I: jal x%d, %d%s%s",dst,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = current_pc + 2;
        DEBUG_EMU(gprintf("\t0x%016llx <- \n",XREGS[dst].x);)
    }
    logxregchange(dst);
    logpcchange(current_pc + imm);
}

void jalr(xreg dst, xreg src1, int imm, const char* comm)
{
    uint64_t src1_old = XREGS[src1].x; // in case dst == src1
    DISASM(gsprintf(dis,"I: jalr x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = current_pc + 4;
        DEBUG_EMU(gprintf("\t0x%016llx <- \n",XREGS[dst].x);)
    }
    logxregchange(dst);
    logpcchange((src1_old + imm) & 0xFFFFFFFFFFFFFFFE);
}

void jal(xreg dst, int imm, const char* comm)
{
    // NB: spike-dasm already multiplies the immediate operand by 2
    DISASM(gsprintf(dis,"I: jal x%d, %d%s%s",dst,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = current_pc + 4;
        DEBUG_EMU(gprintf("\t0x%016llx <- \n",XREGS[dst].x);)
    }
    logxregchange(dst);
    logpcchange(current_pc + imm);
}

void lui(xreg dst, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: lui x%d, %d%s%s",dst,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = sext32(imm << 12);
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx\n",XREGS[dst].x,(uint64_t) imm);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,xnone,xnone,dis);)
}

void auipc(xreg dst, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: auipc x%d, %d%s%s",dst,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        XREGS[dst].x = current_pc + sext32(imm << 12);
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx\n",XREGS[dst].x,(uint64_t) imm);)
    }
    logxregchange(dst);
}

void addi(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: addi x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x + sext12(imm);
    if ( check_stack && dst == x2 && (val < (uint64_t)(&shaderstack[current_thread][0]) || val > (uint64_t)(&shaderstack[current_thread][MAXSTACK])) )
    {
        gprintf("x2 out of bounds: 0x%016llx 0x%016llx 0x%016llx\n",val,(uint64_t)&shaderstack[current_thread][0],(uint64_t)&shaderstack[current_thread][MAXSTACK]);
        exit(-1);
    }
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx + 0x%08x\n",val,XREGS[src1].x,imm);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void slli(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: slli x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = XREGS[src1].x << (imm & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%016llx << %d\n", XREGS[dst].x, val1.x, imm & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void slti(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: slti x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if((int64_t) XREGS[src1].x < (int64_t) sext12(imm))
        val = 1;
    else
        val = 0;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx < 0x%016llx\n",val,XREGS[src1].x,sext12(imm));)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void sltiu(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: sltiu x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if(XREGS[src1].x < sext12(imm))
        val = 1;
    else
        val = 0;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx < 0x%016llx\n",val,XREGS[src1].x,sext12(imm));)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void xori(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: xori x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x ^ sext12(imm);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx & 0x%016llx\n",val,XREGS[src1].x,(uint64_t)imm);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void srli(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: srli x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = XREGS[src1].x >> (imm & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%016llx >> %d\n", XREGS[dst].x, val1.x, imm & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void srai(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: srai x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = XREGS[src1].xs >> (int64_t) (imm & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%016llx >> %d\n", XREGS[dst].x, val1.x, imm & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void ori(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: ori x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = XREGS[src1].x | sext12(imm);
        DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx | 0x%016llx\n", XREGS[dst].x, val1.x, sext12(imm));)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void andi(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: andi x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = XREGS[src1].x & sext12(imm);
        DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx & 0x%016llx\n", XREGS[dst].x, val1.x, sext12(imm));)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void add(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: add x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x + XREGS[src2].x;
    if(dst != x0)
    {

        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx + 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sub(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sub x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x - XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx - 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sll(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sll x%d, x%d, %d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = XREGS[src1].x << (XREGS[src2].x & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%016llx << %d\n", XREGS[dst].x, val1.x, val2.x & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void slt(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: slt x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if((int64_t) XREGS[src1].x < (int64_t) XREGS[src2].x)
        val = 1;
    else
        val = 0;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx < 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sltu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sltu x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if(XREGS[src1].x < XREGS[src2].x)
        val = 1;
    else
        val = 0;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx < 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void xor_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: xor x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x ^ XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx & 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void srl(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: srl x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = XREGS[src1].x >> (XREGS[src2].x & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%016llx >> %d\n", XREGS[dst].x, val1.x, val2.xs & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sra(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sra x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = XREGS[src1].xs >> (XREGS[src2].xs & 0x3F);
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x >> %d\n", XREGS[dst].x, val1.x, val2.x & 0x3f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void or_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: or x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x | XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx | 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void and_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: and x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x & XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx & 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void addiw(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: addiw x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext32(XREGS[src1].x + imm);
    if ( check_stack && dst == x2 && (val < (uint64_t)(&shaderstack[current_thread][0]) || val > (uint64_t)(&shaderstack[current_thread][MAXSTACK])) )
    {
        gprintf("x2 out of bounds: 0x%016llx 0x%016llx 0x%016llx\n",val,(uint64_t)&shaderstack[current_thread][0],(uint64_t)&shaderstack[current_thread][MAXSTACK]);
        exit(-1);
    }
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx + 0x%08x\n",val,XREGS[src1].x,imm);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void slliw(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: slliw x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(XREGS[src1].x << (imm & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x << %d\n", XREGS[dst].x, val1.w[0], imm & 0x1f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void srliw(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: srliw x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(XREGS[src1].w[0] >> (imm & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x >> %d\n", XREGS[dst].x, val1.w[0], imm & 0x1f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void sraiw(xreg dst, xreg src1, int imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: sraiw x%d, x%d, %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        XREGS[dst].x = sext32(XREGS[src1].ws[0] >> (int32_t) (imm & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%08x <- 0x%08x >> %d\n", XREGS[dst].xs, val1.ws[0], imm & 0x1f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,xnone,dis);)
}

void addw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: addw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext32(XREGS[src1].x + XREGS[src2].x);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x + 0x%08x\n",val,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void subw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: subw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext32(XREGS[src1].x - XREGS[src2].x);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x - 0x%08x\n",val,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sllw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sllw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(XREGS[src1].x << (XREGS[src2].x & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x << %d\n", XREGS[dst].x, val1.w, val2.x & 0x1f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void srlw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: srlw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(XREGS[src1].w[0] >> (XREGS[src2].w[0] & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x >> %d\n", XREGS[dst].x, val1.w[0], val2.w[0] & 0x1f);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void sraw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: sraw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    if(dst != x0)
    {
        xdata val1 = XREGS[src1];
        xdata val2 = XREGS[src2];
        XREGS[dst].x = sext32(XREGS[src1].ws[0] >> (XREGS[src2].ws[0] & 0x1F));
        DEBUG_EMU(gprintf("\t 0x%016llx <- 0x%08x >> %d\n", XREGS[dst].x, val1.w[0], val2.w[0] & 0x1F);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void lb(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lb x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext8(memread8(XREGS[base].x + sext12(off)));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void lh(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lh x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext16(memread16(XREGS[base].x + sext12(off)));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void lw(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lw x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext32(memread32(XREGS[base].x + sext12(off)));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void ld(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: ld x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = memread64(XREGS[base].x + sext12(off));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x  = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void lbu(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lbu x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = memread8(XREGS[base].x + sext12(off));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void lhu(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lhu x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = memread16(XREGS[base].x + sext12(off));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void lwu(xreg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: lwu x%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = memread32(XREGS[base].x + sext12(off));

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%08x + 0x%016llx]\n",val,off,XREGS[base].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_ld(LD,dst,base,XREGS[base].x+sext12(off),dis);)
}

void sd(xreg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: sd x%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t addr = XREGS[base].x + off;
    uint64_t val  = XREGS[src1].x;
    memwrite64(addr, val);
    DEBUG_EMU(gprintf("\t%016llx --> MEM[0x%016llx]\n",val,addr);)
    logmemwchange(0, 8, addr, val);
    IPC(ipc_st(STORE_INT, 1, 8, src1,base,XREGS[base].x+off,dis);)
}

void sw(xreg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: sw x%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t addr = XREGS[base].x + off;
    uint32_t val  = (uint32_t) XREGS[src1].x;
    memwrite32(addr, val);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",val,addr);)
    logmemwchange(0, 4, addr, val);
    IPC(ipc_st(STORE_INT, 1, 4, src1,base,XREGS[base].x+off,dis);)
}

void sh(xreg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: sh x%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t addr = XREGS[base].x + off;
    uint32_t val  = (uint32_t) XREGS[src1].x;
    memwrite16(addr, val);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",val,addr);)
    logmemwchange(0, 2, addr, val);
    IPC(ipc_st(STORE_INT, 1, 2, src1,base,XREGS[base].x+off,dis);)
}

void sb(xreg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: sb x%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t addr = XREGS[base].x + off;
    uint32_t val  = (uint32_t) XREGS[src1].x;
    memwrite8(addr, val);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",val,addr);)
    logmemwchange(0, 1, addr, val);
    IPC(ipc_st(STORE_INT, 1, 1, src1,base,XREGS[base].x+off,dis);)
}

void fence(const char* comm)
{
    DISASM(gsprintf(dis,"I: fence%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
}

void fence_i(const char* comm)
{
    DISASM(gsprintf(dis,"I: fence_i%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
}

// TODO
void sfence_vma(const char* comm)
{
    DISASM(gsprintf(dis,"I: sfence_vma%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64M emulation
//
////////////////////////////////////////////////////////////////////////////////

void mul(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: mul x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = XREGS[src1].x * XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx * 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(MUL_INT,dst,src1,src2,dis);)
}

void mulh(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: mulh x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    __int128_t val1 = XREGS[src1].xs;
    __int128_t val2 = XREGS[src2].xs;
    __int128_t val3 = val1 * val2;
    int64_t val = val3 >> 64;

    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx * 0x%016llxs\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(MUL_INT,dst,src1,src2,dis);)
}

void mulhu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: mulhu x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    __uint128_t val1 = XREGS[src1].x;
    __uint128_t val2 = XREGS[src2].x;
    __uint128_t val3 = val1 * val2;
    uint64_t val = val3 >> 64;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx * 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(MUL_INT,dst,src1,src2,dis);)
}

void div_(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: div x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val;
    if(XREGS[src2].x == 0)                                                      val = -1;
    else if((XREGS[src2].xs == -1) && (XREGS[src1].x == 0x8000000000000000ULL)) val = XREGS[src1].xs; // Divide is out of range, return src1
    else                                                                        val = (int64_t) XREGS[src1].x / (int64_t) XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx * 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = (uint64_t) val;
    }
    logxregchange(dst);
    IPC(ipc_int(DIV_INT,dst,src1,src2,dis);)
}

void divu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: divu x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if(XREGS[src2].x == 0) val = (uint64_t) -1;
    else                   val = XREGS[src1].x / XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx / 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(DIV_INT,dst,src1,src2,dis);)
}

void rem(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: rem x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val;
    if(XREGS[src2].x == 0)                                                      val = (int64_t) XREGS[src1].x;
    else if((XREGS[src2].xs == -1) && (XREGS[src1].x == 0x8000000000000000ULL)) val = 0; // Divide is out of range in x86, return 0 straight
    else                                                                        val = (int64_t) XREGS[src1].x % (int64_t) XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx %% 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = (uint64_t) val;
    }
    logxregchange(dst);
    IPC(ipc_int(REM_INT,dst,src1,src2,dis);)
}

void remu(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: remu x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val;
    if(XREGS[src2].x == 0) val = XREGS[src1].x;
    else                   val = XREGS[src1].x % XREGS[src2].x;
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx %% 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(REM_INT,dst,src1,src2,dis);)
}

void mulw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: mulw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = sext32(XREGS[src1].x * XREGS[src2].x);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x * 0x%08lx\n",val,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val;
    }
    logxregchange(dst);
    IPC(ipc_int(MUL_INT,dst,src1,src2,dis);)
}

void divw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: divw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int32_t val;
    if(XREGS[src2].ws[0] == 0)                                             val = -1;
    else if((XREGS[src2].ws[0] == -1) && (XREGS[src1].w[0] == 0x80000000)) val = XREGS[src1].ws[0]; // Divide is out of range, return src1
    else                                                                   val = XREGS[src1].ws[0] / XREGS[src2].ws[0];
    uint64_t val64 = sext32(val);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x / 0x%08x\n",val64,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
    IPC(ipc_int(DIV_INT,dst,src1,src2,dis);)
}

void divuw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: divuw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int32_t val;
    if(XREGS[src2].w[0] == 0) val = -1;
    else                      val = XREGS[src1].w[0] / XREGS[src2].w[0];
    uint64_t val64 = sext32(val);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x / 0x%08x\n",val64,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
    IPC(ipc_int(DIV_INT,dst,src1,src2,dis);)
}

void remw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: remw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int32_t val;
    gprintf("Doing %d %d\n", XREGS[src1].ws[0] , XREGS[src2].ws[0]);
    if(XREGS[src2].ws[0] == 0)                                              val = XREGS[src1].ws[0]; // Divide by 0
    else if((XREGS[src2].ws[0] == -1) && (XREGS[src1].w[0] == 0x80000000))  val = 0;                 // Divide is out of range in x86, return 0 straight
    else                                                                    val = XREGS[src1].ws[0] % XREGS[src2].ws[0];
    uint64_t val64 = sext32(val);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x %% 0x%08x\n",val64,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
    IPC(ipc_int(REM_INT,dst,src1,src2,dis);)
}

void remuw(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: remuw x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int32_t val;
    if(XREGS[src2].w[0] == 0) val = XREGS[src1].w[0];
    else                      val = XREGS[src1].w[0] % XREGS[src2].w[0];
    uint64_t val64 = sext32(val);
    if(dst != x0)
    {
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%08x %% 0x%08x\n",val64,XREGS[src1].w[0],XREGS[src2].w[0]);)
        XREGS[dst].x = val64;
    }
    logxregchange(dst);
    IPC(ipc_int(REM_INT,dst,src1,src2,dis);)
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64A emulation
//
////////////////////////////////////////////////////////////////////////////////

void amoadd_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoadd_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = val + XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- 0x%08x + 0x%08x\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amoxor_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoxor_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = sext32(memread32(addr));
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = val ^ XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- 0x%08x ^ 0x%08x\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amoor_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoor_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = val | XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- 0x%08x | 0x%08x\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amoand_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoand_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = val & XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- 0x%08x & 0x%08x\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amomin_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomin_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = ((int32_t) val < (int32_t) XREGS[src1].w[0]) ? (int32_t) val : (int32_t) XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- min(0x%08x, 0x%08x)\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amomax_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomax_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int32_t res = ((int32_t) val < (int32_t) XREGS[src1].w[0]) ? (int32_t) XREGS[src1].w[0] : (int32_t) val;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- max(0x%08x, 0x%08x)\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amominu_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amominu_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    uint32_t res = (val < XREGS[src1].w[0])? val : XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- minu(0x%08x, 0x%08x)\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amomaxu_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomaxu_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint32_t val = memread32(addr);
    uint64_t val64 = sext32(val);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    uint32_t res = (val > XREGS[src1].w[0]) ? val : XREGS[src1].w[0];

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val64;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val64,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%08x <- maxu(0x%08x, 0x%08x)\n",res,val,XREGS[src1].w[0]);)

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amoswap_w(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoswap_w x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = sext32(memread32(addr));
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = sext32(XREGS[src1].x);

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    // Stores the operated data
    memwrite32(addr, res);
    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 4, addr, res);
}

void amoadd_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoadd_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = val + XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx + 0x%016llx\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amoxor_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoxor_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = val ^ XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx ^ 0x%016llx\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amoor_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoor_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = val | XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx | 0x%016llx\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amoand_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoand_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = val & XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- 0x%016llx & 0x%016llx\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amomin_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomin_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = ((int64_t) val < (int64_t) XREGS[src1].x) ? (int64_t) val : (int64_t) XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- min(0x%016llx,0x%016llx)\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llxx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amomax_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomax_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = ((int64_t) val > (int64_t) XREGS[src1].x) ? (int64_t) val : (int64_t) XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- max(0x%016llx,0x%016llx)\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amominu_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amominu_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = ((uint64_t) val < (uint64_t) XREGS[src1].x) ? (uint64_t) val : (uint64_t) XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- minu(0x%016llx,0x%016llx)\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amomaxu_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amomaxu_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = ((uint64_t) val > (uint64_t) XREGS[src1].x) ? (uint64_t) val : (uint64_t) XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    DEBUG_EMU(gprintf("\t0x%016llx <- maxu(0x%016llx,0x%016llx)\n",res,val,XREGS[src1].x);)

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

void amoswap_d(xreg dst, xreg src1, xreg src2, const char* comm)
{
    uint64_t addr = XREGS[src2].x;
    DISASM(gsprintf(dis,"I: amoswap_d x%d, x%d, (x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    int64_t val = memread64(addr);
    IPC(ipc_ld(LD,dst,src2,addr,dis);)
    int64_t res = XREGS[src1].x;

    // Saves the loaded data
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- MEM[0x%016llx]\n",val,addr);)
    }
    logxregchange(dst);

    // Stores the operated data
    memwrite64(addr, res);
    DEBUG_EMU(gprintf("\t0x%016llx --> MEM[0x%016llx]\n",res,addr);)
    logmemwchange(0, 8, addr, res);
}

////////////////////////////////////////////////////////////////////////////////
//
// SYSTEM emulation
//
////////////////////////////////////////////////////////////////////////////////

static uint64_t csrget(csr src1)
{
    uint64_t val;
    switch (src1) {
        // ----- U-mode registers ----------------------------------------
        case csr_fflags:
            val = csrregs[current_thread][csr_fcsr] & 0x1F;
            break;
        case csr_frm:
            val = (csrregs[current_thread][csr_fcsr] >> 5) & 0x7;
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
        // ----- Tensor instructions -------------------------------------
        case csr_treduce:
        case csr_tfmastart:
        case csr_flbarrier:
        case csr_ucacheop:
        case csr_tloadctrl:
        case csr_tstore:
        case csr_scacheop:
            val = 0;
            break;
        // ----- Shared registers ----------------------------------------

        // ----- All other registers -------------------------------------
        default:
            val = csrregs[current_thread][src1];
            break;
    }
    //DEBUG_EMU(gprintf("csrget 0x%016llx <- csrreg[%d]\n",val,src1);)
    return val;
}

static void csrset(csr src1, uint64_t val)
{
    uint64_t msk;

    switch (src1) {
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
        case csr_tmask:
            val &= 0x000000000000FFFFULL;
            csrregs[current_thread][src1] = val;
            break;
        case csr_tconvsize:
            val &= 0xFF00FFFFFF00FFFFULL;
            csrregs[current_thread][src1] = val;
            tmask_conv();
            break;
        case csr_tconvctrl:
            val &= 0x0000FFFF0000FFFFULL;
            csrregs[current_thread][src1] = val;
            tmask_conv();
            break;
        case csr_tcoop:
            csrregs[current_thread][src1] = val;
            tcoop();
            break;
        // ----- S-mode registers ----------------------------------------
        case csr_sstatus:
            // Preserve sd, sxl, uxl, tsr, tw, tvm, mprv, mpp, mpie, mie
            val = (val & 0x00000000000DE133ULL) | (csrregs[current_thread][csr_mstatus] & 0x8000000F00721800ULL);
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
        case csr_satp:
            // ASID is 4bits, PPN is 28bits
            val &= 0xF000F0000FFFFFFFULL;
            switch (val >> 60) {
                case 0: // Bare
                case 9: // Sv48
                case 8: // Sv39
                    csrregs[current_thread][src1] = val;
                    break;
                default: // reserved
                    // do not write the register if attempting to set an unsupported mode
                    break;
            }
            break;
        // ----- M-mode registers ----------------------------------------
        case csr_mstatus:
            // Preserve sd, sxl, uxl
            val = (val & 0x00000000007FF8BBULL) | (csrregs[current_thread][src1] & 0x8000000F00000000ULL);
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
        // ----- Tensor instructions -------------------------------------
        case csr_treduce:
        case csr_tfmastart:
        case csr_flbarrier:
        case csr_ucacheop:
        case csr_tloadctrl:
        case csr_tstore:
        case csr_scacheop:
            break;
        // ----- Shared registers ----------------------------------------

        // ----- All other registers -------------------------------------
        default:
            csrregs[current_thread][src1] = val;
            break;
    }
    //DEBUG_EMU(gprintf("csrset csrreg[%d] <- 0x%016llx\n",src1,val);)
}

static void csr_insn(xreg dst, csr src1, uint64_t val, bool write)
{
    // Check if current privilege mode has access to the register
    uint64_t prv = csrget(csr_prv);
    if (   ((prv == CSR_PRV_U) && (src1 > CSR_MAX_UMODE))
        || ((prv == CSR_PRV_S) && (src1 > CSR_MAX_SMODE)))
    {
        unknown();
        return;
    }

    uint64_t x = csrget(src1);

    if (write)
    {
        DEBUG_EMU(gprintf("\t0x%016llx --> CSR[%08x]\n", val, src1);)
        switch (src1) {
            // Check if attempting to write a read-only register
            case csr_mvendorid:
            case csr_marchid:
            case csr_mimpid:
            case csr_mhartid:
                unknown();
                return;
#ifdef CHECKER
            case csr_treduce:
                tensorreduce(val);
                break;
            case csr_tfmastart:
                tensorfma(val);
                break;
            case csr_flbarrier:
                x = flbarrier(val);
                break;
            case csr_ucacheop:
            case csr_scacheop:
                x = csr_cacheop_emu(val);
                break;
            case csr_tloadctrl:
                tensorload(val);
                break;
            case csr_tstore:
                tensorstore(val);
                break;
            case csr_umsg_port0:
            case csr_umsg_port1:
            case csr_umsg_port2:
            case csr_umsg_port3:
                x = msg_port_csr(src1 - csr_umsg_port0, val, true);
                break;
            case csr_smsg_port0:
            case csr_smsg_port1:
            case csr_smsg_port2:
            case csr_smsg_port3:
                x = msg_port_csr(src1 - csr_smsg_port0, val, false);
                break;
#endif
            default:
                csrset(src1, val);
                break;
        }
    }

    if (dst != x0)
    {
        XREGS[dst].x = x;
        DEBUG_EMU(gprintf("\t0x%016llx  <- CSR[%08x]\n", x, src1);)
    }
    logxregchange(dst);
}

uint64_t virt_to_phys_emu(uint64_t addr, mem_access_type macc)
{
    // Read SATP, PRV and MSTATUS
    uint64_t satp      = csrget(csr_satp);;
    uint64_t prv       = csrget(csr_prv);
    uint64_t mstatus   = csrget(csr_mstatus);
    uint64_t satp_mode = (satp >> 60) & 0xF;
    uint64_t satp_ppn  = satp & PPN_M;
    bool     sum       = (mstatus >> 18) & 0x1;
    bool     mxr       = (mstatus >> 19) & 0x1;

    bool vm_enabled = (satp_mode != 0) && (prv <= CSR_PRV_S);

    // Default values for Sv48
    int Num_Levels       = 4;
    int PTE_Size         = 8;
    int PTE_Idx_Size     = 9;
    int PTE_top_Idx_Size = 17;
    // if Sv39
    if (satp_mode == 8) {
       Num_Levels = 3;
       PTE_top_Idx_Size = 26;
    }

    uint64_t pte_idx_mask     = (uint64_t(1) << PTE_Idx_Size) - 1;
    uint64_t pte_top_idx_mask = (uint64_t(1) << PTE_top_Idx_Size) - 1;
    if (vm_enabled)
    {
        DEBUG_EMU(gprintf("Virtual memory enabled. Performing page walk on addr 0x%016llx...\n", addr);)

        // Perform page walk
        int level;
        uint64_t ppn, pte_addr, pte;
        bool pte_v, pte_r, pte_w, pte_x, pte_u, pte_a, pte_d;

        level = Num_Levels;
        ppn = satp_ppn;
        do {
          level--;
          if (level < 0)
          {
            //log << LOG_ERR << "Last level PTE is a pointer to a page table: PTE = 0x" << std::hex << pte << endm;
            //DEBUG_EMU(gprintf("ERROR: Last level PTE is a pointer to a page table: PTE = 0x%x\n", pte);)
            return -1;
          }

          // Take VPN[level]
          uint64_t vpn = (addr >> (PG_OFFSET_SIZE + PTE_Idx_Size*level)) & pte_idx_mask;
          // Read PTE
          pte_addr = (ppn << PG_OFFSET_SIZE) + vpn*PTE_Size;
          // TODO: PMA / PMP checks
          pte = memread64(pte_addr, false);
          //log << LOG_DEBUG << "* Level " << std::dec << level << ": PTE = 0x" << std::hex << pte << endm;
          //DEBUG_EMU(gprintf("* Level %d: PTE = 0x%x\n", level, pte);)

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
            //log << LOG_ERR << "Invalid entry: PTE = 0x" << std::hex << pte << endm;
            //DEBUG_EMU(gprintf("ERROR: Invalid entry: PTE = 0x%x\n", pte);)
            return -1;
          }

          // Check if PTE is a pointer to next table level
        } while (!pte_r && !pte_x);

        // A leaf PTE has been found
        //log << LOG_DEBUG << "Valid leaf PTE found at level " << std::dec << level << endm;
        //DEBUG_EMU(gprintf("Valid leaf PTE found at level %d\n", level);)

        // Check permissions
        bool perm_ok = (macc == Mem_Access_Load)  ? pte_r || (mxr && pte_x) :
                       (macc == Mem_Access_Store) ? pte_w :
                                                    pte_x; // Mem_Access_Fetch
        if (!perm_ok)
        {
          //log << LOG_ERR << "Page permissions do not allow this type of access (";
          //switch (macc)
          //{
          //  case Mem_Access_Load:  log << "Load)" << endm; break;
          //  case Mem_Access_Store: log << "Store)" << endm; break;
          //  case Mem_Access_Fetch: log << "Fetch)" << endm; break;
          //  default:               log << "*Invalid*: " << std::dec << (int)macc << ")" << endm;
          //}

          return -1;
        }

        // Check privilege mode
        // If page is accessible to user mode, supervisor mode SW may also access it if sum bit from the sstatus is set
        // Otherwise, check that privilege mode is higher than user
        bool priv_ok = pte_u ? (prv == CSR_PRV_U) || sum : prv != CSR_PRV_U;
        if (!priv_ok)
        {
          //log << LOG_ERR << "Page not accessible for current privilege mode (";
          //switch (prv)
          //{
          //  case CSR_PRV_U: log << "User)" << endm;
          //  case CSR_PRV_S: log << "Supervisor)" << endm;
          //  default:        log << "*Invalid*: " << std::dec << prv << ")" << endm;
          //}

          return -1;
        }

        // Check if it is a misaligned superpage
        if ((level > 0) && ((ppn & ((1<<(PTE_Idx_Size*level))-1)) != 0))
        {
          //log << LOG_ERR << "Misaligned superpage" << endm;
          //for (int i = 0; i < level; i++)
          //  log << LOG_DEBUG << "* ppn[" << std::dec << i << "] = 0x" << std::hex << ((ppn>>(PTE_Idx_Size*i)) & ((1<<(PTE_Idx_Size))-1)) << endm;
          return -1;
        }

        if (!pte_a || ((macc == Mem_Access_Store) && !pte_d))
        {
          //log << LOG_DEBUG << "* Setting A/D bits in PTE" << endm;

          // Set pte.a to 1 and, if the memory access is a store, also set pte.d to 1
          uint64_t pte_write = pte;
          pte_write |= 1 << PTE_A_OFFSET;
          if (macc == Mem_Access_Store)
            pte_write |= 1 << PTE_D_OFFSET;

          // Write PTE
          memwrite64(pte_addr, pte_write, false);
        }

        // Obtain physical address
        uint64_t paddr;

        // Copy page offset
        paddr = addr & PG_OFFSET_M;

        for (int i = 0; i < Num_Levels; i++)
        {
          // If level > 0, this is a superpage translation so VPN[level-1:0] are part of the page offset
          if (i < level) {
            paddr |= addr & (pte_idx_mask << (PG_OFFSET_SIZE + PTE_Idx_Size*i));
          } else if (i == Num_Levels-1) {
            paddr |= (ppn & (pte_top_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
          } else {
            paddr |= (ppn & (pte_idx_mask << (PTE_Idx_Size*i))) << PG_OFFSET_SIZE;
          }
        }

        // Final physical address only uses 40 bits
        paddr &= PA_M;

        DEBUG_EMU(gprintf("Physical address = 0x%x\n",paddr);)

        return paddr;
    }
    else
    {
        // Direct mapping, physical address is 40 bits
        return addr & PA_M;
    }
}

void ecall(const char* comm)
{
    DISASM(gsprintf(dis,"I: ecall%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    trap_to_mmode(CSR_MCAUSE_ECALL_FROM_UMODE + csrget(csr_prv), 0);
}

void ebreak(const char* comm)
{
    DISASM(gsprintf(dis,"I: ebreak%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    // TODO: The spec says that hardware breakpoint sets mtval/stval to the
    // current PC but ebreak is a software breakpoint; should it also set
    // mtval/stval to the current PC or set it to 0?
    trap_to_mmode(CSR_MCAUSE_BREAKPOINT, 0);
}

void sret(const char* comm)
{
    DISASM(gsprintf(dis,"I: sret%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    logpcchange(csrget(csr_sepc));
    // Take spie and spp
    uint64_t mstatus = csrget(csr_mstatus);
    uint64_t spie = (mstatus >> 5) & 0x1;
    uint64_t spp = (mstatus >> 8) & 0x1;
    // Clean sie, spie and spp
    uint64_t mstatus_clean = mstatus & 0xFFFFFFFFFFFFFEDDULL;
    // Set sie = spie, spie = 1, spp = U (0), prv = spp
    csrset(csr_mstatus, mstatus_clean | (spie << 1) | (1 << 8));
    csrset(csr_prv, spp);
    DEBUG_EMU(gprintf("Now running in %s mode\n", (spp == CSR_PRV_M) ? "M" : (spp == CSR_PRV_S) ? "S" : "U");)
}

void mret(const char* comm)
{
    DISASM(gsprintf(dis,"I: mret%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
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
    DEBUG_EMU(gprintf("Now running in %s mode\n", (mpp == CSR_PRV_M) ? "M" : (mpp == CSR_PRV_S) ? "S" : "U");)
}

void wfi(const char* comm)
{
    DISASM(gsprintf(dis,"I: wfi%s%s",(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
}

void csrrw(xreg dst, csr src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrw x%d, csrreg[%d], x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, XREGS[src2].x, true);
}

void csrrs(xreg dst, csr src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrs x%d, csrreg[%d], x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, csrget(src1) | XREGS[src2].x, src2 != x0);
}

void csrrc(xreg dst, csr src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrc x%d, csrreg[%d], x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, csrget(src1) & (~XREGS[src2].x), src2 != x0);
}

void csrrwi(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrwi x%d, csrreg[%d], %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, imm, true);
}

void csrrsi(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrsi x%d, csrreg[%d], %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, csrget(src1) | imm, imm != 0);
}

void csrrci(xreg dst, csr src1, uint64_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: csrrci x%d, csrreg[%d], %d%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    csr_insn(dst, src1, csrget(src1) & (~imm), imm != 0);
}

////////////////////////////////////////////////////////////////////////////////
//
// RV64F emulation
//
////////////////////////////////////////////////////////////////////////////////

static void femuld(opcode opc, int count, int size, freg dst, int off, xreg base,  int use_mask)
{
    for ( int i = 0; i < count; i++ )
    {
        uint64_t addr = XREGS[base].x + off;
        addr = addr + i * size;

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        // except if using SP register as base

        bool genResult = ! ( use_mask && MREGS[0].b[i*size/2] == 0 );

        uint32_t  val32;
        float32_t fval32;

        val32 = FREGS[dst].u[i]; // default result when element is masked

        switch ( opc )
        {
            case FLW:
                if ( genResult )
                {
                    val32 = memread32(addr);
                    fval32  = cast_uint32_to_float32(val32);
                    FREGS[dst].u[i] = val32;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <- MEM[0x%016llx]\n",i,val32,fval32,addr););
                }
                break;
            default:
                assert(0);
                break;
        }
    }

#ifdef ZERO_EXTEND_UNUSED_FREG_BITS
    for (int i = count; i < 4; ++i)
        FREGS[dst].u[i] = 0;
#endif
    logfregchange(dst);
    IPC(ipc_ld(opc,count,size,dst,base,(XREGS[base].x+off),dis););
}

static void femust(opcode opc, int count, int size, freg src1, int off, xreg base, int use_mask)
{
    for ( int i = 0; i < count; i++ )
    {
        if ( use_mask && MREGS[0].b[i*size/2] == 0 ) continue;

        uint64_t addr = XREGS[base].x  + off;
        addr = addr + i * size;

        float32_t fval32;
        uint32_t  val32;
        switch ( opc )
        {
            case FSW:
                fval32 = FREGS[src1].f[i];
                val32  = FREGS[src1].u[i];
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) --> MEM[0x%016llx]\n",i,val32,fval32,addr););
                memwrite32(addr, val32);
                logmemwchange(i, 4, addr, val32);
                break;
            default:
                assert(0);
                break;
        }
    }
    IPC(ipc_st(opc,count,size,src1,base,(XREGS[base].x+off),dis);)
}

static const char* rmnames[] = {
    "rne", "rtz", "rdn", "rup",
    "rmm", "res", "res", "dyn"
};

static void femucvtf2x(opcode opc, int count, xreg dst, freg src1, rounding_mode rm)
{
    iufval val;
    iufval rescvt;

    val.f = FREGS[src1].f[0];
    switch ( opc )
    {
        case FCVTWS:
            rescvt.x = XREGS[dst].x;
            if      ( isnan(val.f) ) rescvt.i = 0x7fffffff;
            else if ( isinf(val.f) )
            {
                if ( val.f > 0 )     rescvt.i = 0x7fffffff;
                else                 rescvt.i = 0x80000000;
            }
            else if ( (val.f > 0) && (val.u > 0x4effffff) ) // Float not representable in int32
            {
                                     rescvt.i = 0x7fffffff;
            }
            else if ( (val.f < 0) && (val.u > 0xcf000000) ) // Float not representable in int32
            {
                                     rescvt.i = 0x80000000;
            }
            else                     rescvt.i = roundf(val.f, rm);
            DEBUG_EMU(gprintf("\t0x%08x (%d) <-- 0x%08x (%f)\n",rescvt.u,rescvt.i,val.u,val.f););
            break;
        case FCVTWUS:
            rescvt.x = XREGS[dst].x;
            if      ( isnan(val.f) ) rescvt.u = 0xffffffff;
            else if ( isinf(val.f) )
            {
                if ( val.f > 0 )     rescvt.u = 0xffffffff;
                else                 rescvt.u = 0x00000000;
            }
            else if ( (val.f > 0) && (val.u > 0x4f7fffff) ) // Float not representable in uint32
            {
                rescvt.u = 0xffffffff;
            }
            else if ( val.f < 0 ) // Float not representable in uint32
            {
                rescvt.u = 0x00000000;
            }
            else                     rescvt.u = roundf(val.f, rm);
            DEBUG_EMU(gprintf("\t0x%08x (%d) <-- 0x%08x (%f)\n",rescvt.u,rescvt.u,val.u,val.f););
            break;
        default:
            assert(0);
            break;
    }
    if (dst != x0)
        XREGS[dst].x = sext32(rescvt.x);
    logxregchange(dst);
    IPC(ipc_ps(opc,1,dst,src1,fnone,fnone,dis);)
}

static void femucvtx2f(opcode opc, freg dst, xreg src1, rounding_mode rm)
{
    iufval res;
    iufval valcvt;

    valcvt.x = XREGS[src1].x;
    switch ( opc )
    {
        case FCVTSW:
            res.f  = valcvt.i;
            DEBUG_EMU(gprintf("\t0x%08x (%f) <-- 0x%08x (%d)\n",res.u,res.f,valcvt.u,valcvt.i););
            break;
        case FCVTSWU:
            res.f  = valcvt.u;
            DEBUG_EMU(gprintf("\t0x%08x (%f) <-- 0x%08x (%u)\n",res.u,res.f,valcvt.u,valcvt.u););
            break;
        default:
            assert(0);
            break;
    }
    FREGS[dst].f[0] = res.f;
#ifdef ZERO_EXTEND_UNUSED_FREG_BITS
    for(int i = 1; i < 4; i++)
        FREGS[dst].u[i] = 0;
#endif
    logfregchange(dst);
    IPC(ipc_ps(opc,1,dst,src1,fnone,fnone,dis);)
}

static void femu1src(opcode opc, int count, freg dst, freg src1, rounding_mode rm)
{
    iufval val;
    double intpart;

    for (int i = 0; i < count; ++i)
    {
        val.f = FREGS[src1].f[i];

        bool genResult = !(count == 4 && MREGS[0].b[i*2] == 0);

        iufval res;
        res.f = FREGS[dst].f[i]; // result when element is masked (existing reg value)
        switch ( opc )
        {
            case FSQRT:
                if ( genResult )
                {
                    res.f = sqrtf(val.f);
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                }
                break;
            case FRSQ:
                if ( genResult )
                {
                    if (emu_use_fake_txfma)
                    {
                        res.f = (float) ((double) 1.0 / sqrt((double) val.f));
                    }
                    else
                    {
                        res.f = ttrans_frsq(val.u);
                        // security ulp check
                        iufval res_gold;
                        res_gold.f = (float) ((double) 1.0 / sqrt((double) val.f));
                        DEBUG_EMU(gprintf("RSQ TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x\n", val.u, res.u, res_gold.u););
                        if(security_ulp_check(res_gold.u,res.u)){
                            DEBUG_EMU(gprintf("WARNING. Don't panic. Trans mismatch error for operation RSQ with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u););
                        }
                    }
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                break;
            case FSIN:
                if ( genResult )
                {
                    if (emu_use_fake_txfma)
                    {
                        res.f = sin(2*M_PI*val.f);
                    }
                    else
                    {
                        res.f = ttrans_fsin(val.u);
                        // security ulp check
                        iufval res_gold, sin_tmp;
                        double f;
                        sin_tmp.f = (float) modf(val.f, &f);

                        sin_tmp.f = sin_tmp.f > 0.5 ? sin_tmp.f - 1.0
                                  : sin_tmp.f < -0.5 ? sin_tmp.f + 1.0
                                  : sin_tmp.f;

                        res_gold.f = (float) sin(2 * M_PI * (double) sin_tmp.f);
                        DEBUG_EMU(gprintf("SIN TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x\n", val.u, res.u, res_gold.u););
                        if(security_ulp_check(res_gold.u,res.u)){
                            DEBUG_EMU(gprintf("WARNING. Don't panic. Trans mismatch error for operation FSIN with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u););
                        }
                    }
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                break;
            case FEXP:
                if ( genResult )
                {
                    if (emu_use_fake_txfma)
                    {
                        res.f = exp2f(val.f);
                    }
                    else
                    {
                        res.f = ttrans_fexp2(val.u);
                        // security ulp check
                        iufval res_gold;
                        res_gold.f = exp2f(val.f);

                        // Remove denormals
                        if ((res.u & 0x7f800000) == 0) res.u = res.u & 0xff800000;
                        if ((res_gold.u & 0x7f800000) == 0) res_gold.u = res_gold.u & 0xff800000;

                        DEBUG_EMU(gprintf("EXP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x\n", val.u, res.u, res_gold.u););
                        if(security_ulp_check(res_gold.u,res.u)){
                            DEBUG_EMU(gprintf("WARNING. Don't panic. Trans mismatch error for operation FEXP with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u););
                        }
                    }
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                }
                printf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f);
                break;
            case FLOG:
                if ( genResult )
                {
                    if (emu_use_fake_txfma)
                    {
                        res.f = log2f(val.f);
                    }
                    else
                    {
                        res.f = ttrans_flog2(val.u);
                        // security ulp check
                        iufval res_gold;
                        res_gold.f = (float)log2((double)val.f);
                        //DEBUG_EMU(gprintf("LOG TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x\n", val.u, res.u, res_gold.u););
                        DEBUG_EMU(gprintf("LOG TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x (%.20f)\n", val.u, res.u, res_gold.u, res_gold.f););
                        if(security_ulp_check(res_gold.u,res.u)){
                            DEBUG_EMU(gprintf("WARNING. Don't panic. Trans mismatch error for operation FLOG with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u););
                        }
                    }
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                break;
            case FRCP:
                if ( genResult )
                {
                    if (emu_use_fake_txfma)
                    {
                        res.f = (float) (1.0 / (double) val.f);
                    }
                    else
                    {
                        res.f = ttrans_frcp(val.u);
                        // security ulp check
                        iufval res_gold;
                        res_gold.f = (float) (1.0 / (double) val.f);
                        DEBUG_EMU(gprintf("RCP TRANS\tIN: 0x%08x\tOUT: 0x%08x\tEXPECTED: 0x%08x\n", val.u, res.u, res_gold.u););
                        //assert(res.u == res_gold.u);
                        if(security_ulp_check(res_gold.u,res.u)){
                            DEBUG_EMU(gprintf("WARNING. Don't panic. Trans mismatch error for operation FRCP with input: 0x%08X. This might happen, report to jordi.sola@esperantotech.com if needed.", val.u););
                        }
                    }
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                break;
            case FRCPFXP:
                if ( genResult )
                {
                    // Input value is 2xtriArea with 15.16 precision
                    float64_t tmp = float64_t(val.i) / float64_t(1 << 16);

                    // Result value is 17.14
                    float64_t tmp_rcp = (1.0f / tmp) * float64_t(1 << 14);

                    res.i = int32_t(tmp_rcp);
                    DEBUG_EMU( printf("\t[%d] 0x%08x (%d) <-- 1 / 0x%08x (%d)\n", i, res.u, res.i, val.u, val.i); )
                }
                break;
            case FCVTPSPW:
                if ( genResult )
                {
                    res.f  = val.i;
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%d)\n",i,res.u,res.f,val.u,val.i););
                }
                break;
            case FCVTPSRAST:
                if( genResult)
                {
                    res.f = ((float)val.i) / (1 << 16);
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%d)\n",i,res.u,res.f,val.u,val.i););
                }
                break;
            case FCVTRASTPS:
                if( genResult)
                {
                    res.i = (int32_t)(val.f*(1 << 14) + 0.5);
                    // convert to canonical NaN
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%f)\n",i,res.u,res.i,val.u,val.f););
                }
                break;
            case FCVTPSPWU:
                if ( genResult )
                {
                    res.f  = val.u;
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%u)\n",i,res.u,res.f,val.u,val.u););
                }
                break;
            case FCVTPWPS:
                if ( genResult )
                {
                    if      ( isnan(val.f) ) res.i = 0x7fffffff;
                    else if ( isinf(val.f) )
                    {
                        if ( val.f > 0 )     res.i = 0x7fffffff;
                        else                 res.i = 0x80000000;
                    }
                    else if ( (val.f > 0) && (val.u > 0x4effffff) ) // Float not representable in int32
                    {
                                             res.i = 0x7fffffff;
                    }
                    else if ( (val.f < 0) && (val.u > 0xcf000000) ) // Float not representable in int32
                    {
                                             res.i = 0x80000000;
                    }
                    else                     res.i = roundf(val.f, rm);
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08lx (%f)\n",i,res.u,res.i,val.u,val.f););
                }
                break;
            case FCVTPWUPS:
                if ( genResult )
                {
                    if      ( isnan(val.f) ) res.u = 0xffffffff;
                    else if ( isinf(val.f) )
                    {
                        if ( val.f > 0 )     res.u = 0xffffffff;
                        else                 res.u = 0x00000000;
                    }
                    else if ( (val.f > 0) && (val.u > 0x4f7fffff) ) // Float not representable in uint32
                    {
                                             res.u = 0xffffffff;
                    }
                    else if ( val.f < 0 ) // Float not representable in uint32
                    {
                                             res.u = 0x00000000;
                    }
                    else                     res.u = roundf(val.f, rm);
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%u) <-- 0x%08x (%f)\n",i,res.u,res.u,val.u,val.f););
                }
                break;
            case FFRC:
                if ( genResult )
                {
                    res.f  = modf(val.f,&intpart);
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f)\n",i,res.u,res.f,val.u,val.f););
                }
                break;
            case FROUND:
                if ( genResult )
                {
                    res.f  = roundf(val.f);
                    // convert to canonical NaN
                    if ( isnan(res.f) ) res.f = nanf("");
                    DEBUG_EMU(printf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) (warning! ignoring rounding mode!!!! fixme!)\n",i,res.u,res.f,val.u,val.f););
                }
                break;
            case FCLASSPS:
                if (genResult)
                {
                    switch (fpclassify(val.f)) {
                        case FP_INFINITE:  if (signbit(val.f)) res.u = 1<<0;
                                           else res.u = 1<<7;
                                           break;
                        case FP_NAN:       if (val.u & 0x00400000) res.u = 1<<9; // quiet NaN
                                           else res.u = 1 << 8;                  // signaling NaN
                                           break;
                        case FP_ZERO:      if (signbit(val.f)) res.u = 1<<3;
                                           else res.u = 1<<4;
                                           break;
                        case FP_SUBNORMAL: if (signbit(val.f)) res.u = 1<<2;
                                           else res.u = 1<<5;
                                           break;
                        case FP_NORMAL:    if (signbit(val.f)) res.u = 1<<1;
                                           else res.u = 1<<6;
                                           break;
                        default:           assert(0); // error!
                                           break;
                    }
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f)\n",i,res.u,val.u,val.f););
                }
                break;
            default:
                assert(0);
                break;
        }
        FREGS[dst].f[i] = res.f;
    }
#ifdef ZERO_EXTEND_UNUSED_FREG_BITS
    for (int i = count; i < 4; ++i)
        FREGS[dst].u[i] = 0;
#endif
    logfregchange(dst);
    IPC(ipc_ps(opc,count,dst,src1,fnone,fnone,dis);)
}

static void femu2src(opcode opc, int count, freg dst, freg src1, freg src2, rounding_mode rm)
{
    iufval val1, val2;

    for ( int i = 0; i < count; i++ )
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        //if ( count == 4 && MREGS[0].b[i*2] == 0 ) continue;

        val1.f  = FREGS[src1].f[i];
        val2.f  = src2 != fnone ? FREGS[src2].f[i] : 0;

        bool genResult = !(count == 4 && MREGS[0].b[i*2] == 0);
        iufval res;
        res.u = FREGS[dst].u[i];
        switch ( opc )
        {
            case FADD:
                if (genResult)
                {
                    res.f  = val1.f + val2.f;
                    if (isnan(res.f)) res.u = 0x7fc00000;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) + 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FSUB:
                if (genResult)
                {
                    res.f  = val1.f - val2.f;
                    if (isnan(res.f)) res.u = 0x7fc00000;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) - 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FMUL:
                if (genResult)
                {
                    res.f  = val1.f * val2.f;
                    if (isnan(res.f)) res.u = 0x7fc00000;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) * 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FDIV:
                if (genResult)
                {
                    res.f  = val1.f / val2.f;
                    if (isnan(res.f)) res.u = 0x7fc00000;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) / 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FMIN:
                if (genResult)
                {
                    res.f  = (val1.f <= val2.f) ? val1.f : val2.f;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- min(0x%08x (%f), 0x%08x (%f))\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FMAX:
                if (genResult)
                {
                    res.f  = (val1.f >= val2.f) ? val1.f : val2.f;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- max(0x%08x (%f), 0x%08x (%f))\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FLT:
                if (genResult)
                {
                    res.u  = (val1.f < val2.f) ? 0xFFFFFFFF : 0;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) < 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f););
                }
                break;
//            case FLTABS:
//                if (genResult)
//                {
//                    res.u  = (fabs(val1.f) < fabs(val2.f)) ? 0xFFFFFFFF : 0;
//                    DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) < 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f););
//                }
//                break;
            case FLE:
                if (genResult)
                {
                    res.u  = (val1.f <= val2.f) ? 0xFFFFFFFF : 0;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) <= 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FEQ:
                if (genResult)
                {
                    res.u  = (val1.u == val2.u) ? 0xFFFFFFFF : 0;
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) == 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FSGNJ:
                if (genResult)
                {
                    res.u  = val1.u & 0x7fffffff;
                    res.u  = res.u | (val2.u & 0x80000000 );
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f), 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FSGNJN:
                if (genResult)
                {
                    res.u  = val1.u & 0x7fffffff;
                    res.u  = res.u | ((~val2.u) & 0x80000000);
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f), 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FSGNJX:
                if (genResult)
                {
                    res.u  = val1.u & 0x7fffffff;
                    res.u  = res.u | ((val1.u ^ val2.u) & 0x80000000);
                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f), 0x%08x (%f)\n",i,res.u,res.f,val1.u,val1.f,val2.u,val2.f););
                }
                break;
            case FRCP_FIX_RAST:
                if (genResult)
                {
                    // Input value is 2xtriArea with 15.16 precision
                    float64_t tmp = float64_t(val1.i) / float64_t(1 << 16);

                    // Result value is 17.14
                    float64_t tmp_rcp = (1.0f / tmp) * float64_t(1 << 14);

                    iufval res_gold;
                    res_gold.i = int32_t(tmp_rcp);

                    float64_t yn = float64_t(val2.i)/float64_t(1 << 14);
                    double a = yn * tmp;
                    uint32_t partial = (uint32_t)(a * (((uint64_t)1) << 31));
                    //printf("Partial: 0x%08x\n", partial);
                    float64_t unpartial = float64_t(partial)/float64_t(((uint64_t)1) << 31);
                    float64_t result = yn*(2.0-unpartial);
                    res.i = (int32_t)(result*(1 << 14));

                    //printf("FRCPFXP NR EXPECTED: 0x%08x RESULT: 0x%08x\n", res_gold.u, res.u); 

                    //Check 1ulp
                    assert((abs(res.i - res_gold.i) <=1) && "Trans mismatch error. Please open jira to jordi.sola@esperantotech.com.");

                    DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%f), 0x%08x (%d)\n",i,res.u,res.i,val1.u,tmp,val2.u,val2.i););
                }
                break;
            default:
                assert(0);
                break;
        }
        FREGS[dst].f[i] = res.f;
    }
#ifdef ZERO_EXTEND_UNUSED_FREG_BITS
    for (int i = count; i < 4; ++i)
        FREGS[dst].u[i] = 0;
#endif
    logfregchange(dst);
    IPC(ipc_ps(opc,count,dst,src1,src2,fnone,dis);)
}

static void femu3src(opcode opc, int count, freg dst, freg src1, freg src2, freg src3, rounding_mode rm)
{
    for ( int i = 0; i < count; i++ )
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        //if ( count == 4 && MREGS[0].b[i*2] == 0 ) continue;

        float32_t val1 = FREGS[src1].f[i];
        float32_t val2 = FREGS[src2].f[i];
        float32_t val3 = FREGS[src3].f[i];

        uint32_t val1u = cast_float32_to_uint32(val1);
        uint32_t val2u = cast_float32_to_uint32(val2);
        uint32_t val3u = cast_float32_to_uint32(val3);

        bool genResult = ! ( count == 4 && MREGS[0].b[i*2] == 0 );

        float32_t res = FREGS[dst].f[i];
        uint32_t resu = FREGS[dst].u[i];

        switch ( opc )
        {
            case FMADD:
                if ( genResult )
                {
                    res  = fmaf(val1,val2,val3);
                    if (isnan(res)) res = nanf("");
                    resu = cast_float32_to_uint32(res);
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) * 0x%08x (%f) + 0x%08x (%f)\n",i,resu,res,val1u,val1,val2u,val2,val3u,val3););
                break;
            case FNMADD:
                if ( genResult )
                {
                    res  = - fmaf(val1,val2,val3);
                    if (isnan(res)) res = nanf("");
                    resu = cast_float32_to_uint32(res);
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- -(0x%08x (%f) * 0x%08x (%f) + 0x%08x (%f))\n",i,resu,res,val1u,val1,val2u,val2,val3u,val3););
                break;
            case FMSUB:
                if ( genResult )
                {
                    res  = fmaf(val1,val2,-val3);
                    if (isnan(res)) res = nanf("");
                    resu = cast_float32_to_uint32(res);
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) * 0x%08x (%f) - 0x%08x (%f)\n",i,resu,res,val1u,val1,val2u,val2,val3u,val3););
                break;
            case FNMSUB:
                if ( genResult )
                {
                    res  = -fmaf(val1,val2,-val3);
                    if (isnan(res)) res = nanf("");
                    resu = cast_float32_to_uint32(res);
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- -(0x%08x (%f) * 0x%08x (%f) - 0x%08x (%f))\n",i,resu,res,val1u,val1,val2u,val2,val3u,val3););
                break;
            case FCMOV:
                if ( genResult )
                {
                    res  = (FREGS[src1].u[i] != 0) ? val2 : val3;
                    resu = cast_float32_to_uint32(res);
                }
                DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- %d ? 0x%08x (%f) : 0x%08x (%f)\n",i,resu,res,FREGS[src1].u[i],val2u,val2,val3u,val3););
                break;
            default:
                assert(0);
                break;
        }

        FREGS[dst].f[i] = res;
    }
#ifdef ZERO_EXTEND_UNUSED_FREG_BITS
    for (int i = count; i < 4; ++i)
        FREGS[dst].u[i] = 0;
#endif
    logfregchange(dst);
    IPC(ipc_ps(opc,count,dst,src1,src2,src3,dis);)
}

static void femucmp(opcode opc, int count, int size, xreg dst, freg src1, freg src2)
{
    iufval val1, val2;

    for ( int i = 0; i < count; i++ )
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if ( count == 4 && MREGS[0].b[i*2] == 0 ) continue;

        val1.f  = FREGS[src1].f[i];
        val2.f  = FREGS[src2].f[i];

        iufval res;
        switch ( opc )
        {
            case FLT:    res.u  = (val1.f < val2.f) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) < 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            case FLE:    res.u  = (val1.f <= val2.f) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) <= 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            case FEQ:    res.u  = (val1.u == val2.u) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x (%f) == 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            default:     assert(0);
                         break;
        }
        if(dst != x0)
            XREGS[dst].x = sext32(res.u);
    }
    logxregchange(dst);
    IPC(ipc_f2x(opc,dst,src1,src2,dis);)
}


void fadd_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fadd.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FADD, 1, dst, src1, src2, rm); 
}

void fsub_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsub.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FSUB, 1, dst, src1, src2, rm); 
}

void fmul_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmul.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FMUL, 1, dst, src1, src2, rm); 
}

void fdiv_s(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fdiv.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FDIV, 1, dst, src1, src2, rm); 
}

void fsgnj_s(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnj.s f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FSGNJ, 1, dst, src1, src2, rmdyn); 
}

void fsgnjn_s(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnjn.s f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FSGNJN, 1, dst, src1, src2, rmdyn); 
}

void fsgnjx_s(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnjx.s f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FSGNJX, 1, dst, src1, src2, rmdyn); 
}

void fmin_s(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmin.s f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FMIN, 1, dst, src1, src2, rmdyn); 
}

void fmax_s(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmax.s f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu2src(FMAX, 1, dst, src1, src2, rmdyn); 
}

void fsqrt_s(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsqrt.s f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu1src(FSQRT, 1, dst, src1, rm); 
}

void feq_s(xreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: feq.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucmp(FEQ, 1, 4, dst, src1, src2); 
}

void fle_s(xreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fle.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucmp(FLE, 1, 4, dst, src1, src2); 
}

void flt_s(xreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: flt.s f%d, f%d, f%d, %s%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucmp(FLT, 1, 4, dst, src1, src2); 
}

void fcvt_w_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.w.s x%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucvtf2x(FCVTWS, 1, dst, src1, rm);
}

void fcvt_wu_s(xreg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.wu.s x%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucvtf2x(FCVTWUS, 1, dst, src1, rm);
}

void fmv_x_w(xreg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmv.x.w x%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););

    if(dst != x0)
    {
        XREGS[dst].x = sext32(FREGS[src1].u[0]);
        DEBUG_EMU(gprintf("\t0x%016llx <- 0x%08x (%f)\n", XREGS[dst].x, FREGS[src1].u[0], FREGS[src1].f[0]););
    }
    logxregchange(dst);
}

void fclass_s(xreg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fclass.s x%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););

    iufval val;
    iufval res;
    val.f = FREGS[src1].f[0];
    switch (fpclassify(val.f)) {
        case FP_INFINITE:  if (signbit(val.f)) res.u = 1<<0;
                           else res.u = 1<<7;
                           break;
        case FP_NAN:       if (val.u & 0x00400000) res.u = 1<<9; // quiet NaN
                           else res.u = 1 << 8;                  // signaling NaN
                           break;
        case FP_ZERO:      if (signbit(val.f)) res.u = 1<<3;
                           else res.u = 1<<4;
                           break;
        case FP_SUBNORMAL: if (signbit(val.f)) res.u = 1<<2;
                           else res.u = 1<<5;
                           break;
        case FP_NORMAL:    if (signbit(val.f)) res.u = 1<<1;
                           else res.u = 1<<6;
                           break;
        default:           assert(0); // error!
                           break;
    }
    if (dst != x0)
    {
        XREGS[dst].x = res.x;
        DEBUG_EMU(gprintf("\t0x%08x <-- 0x%08x (%f)\n",res.u,val.u,val.f););
    }
    logxregchange(dst);
    IPC(ipc_ps(FCLASS,1,dst,src1,fnone,fnone,dis);)
}

void fcvt_s_w(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.s.w f%d, x%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucvtx2f(FCVTSW, dst, src1, rm);
}

void fcvt_s_wu(freg dst, xreg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.s.wu f%d, x%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femucvtx2f(FCVTSWU, dst, src1, rm);
}

void fmv_w_x(freg dst, xreg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmv.w.x f%d, x%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););

    FREGS[dst].u[0] = XREGS[src1].w[0];
    DEBUG_EMU(gprintf("\t0x%08x (%f) <- 0x%08x\n", FREGS[dst].u[0], FREGS[dst].f[0], XREGS[src1].w[0]););
    for(int i = 1; i < 4; i++)
        FREGS[dst].u[i] = 0;
    logfregchange(dst);
}

void flw(freg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: flw f%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femuld(FLW, 1, 4, dst, off,  base, 0);
}

void fsw(freg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsw f%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femust(FSW, 1, 4, src1, off, base, 0);
}

void fmadd_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmadd.s f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu3src(FMADD, 1, dst, src1, src2, src3, rm);
}

void fmsub_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmsub.s f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu3src(FMSUB, 1, dst, src1, src2, src3, rm);
}

void fnmsub_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fnmsub.s f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu3src(FNMSUB, 1, dst, src1, src2, src3, rm);
}

void fnmadd_s(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fnmadd.s f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femu3src(FNMADD, 1, dst, src1, src2, src3, rm);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto mask extension emulation
//
////////////////////////////////////////////////////////////////////////////////

static void maskop(opcode opc, mreg dst, mreg src1, mreg src2)
{
    uint8_t val1, val2;

    for ( int i = 0; i < 8; i++ )
    {
        val1  = MREGS[src1].b[i];
        val2  = (src2 == mnone) ? 0 : MREGS[src2].b[i];

        bool res;
        switch ( opc )
        {
            case MAND:   res = (val1 & val2) & 0x1;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- %d & %d\n",i,res,val1,val2);)
                         break;
            case MOR:    res = (val1 | val2) & 0x1;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- %d | %d\n",i,res,val1,val2);)
                         break;
            case MXOR:   res = (val1 ^ val2) & 0x1;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- %d ^ %d\n",i,res,val1,val2);)
                         break;
            case MNOT:   res = (~val1) & 0x1;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- ~%d\n",i,res,val1);)
                         break;
            default:     assert(0);
                         break;

        }
        MREGS[dst].b[i] = res;
    }
    logmregchange(dst);
    IPC(ipc_msk(opc,dst,src1,src2,dis);)
}

void maskand(mreg dst, mreg src1, mreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskand m%d, m%d, m%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    maskop(MAND, dst, src1, src2);
}

void maskor(mreg dst, mreg src1, mreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskand m%d, m%d, m%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    maskop(MOR, dst, src1, src2);
}

void maskxor(mreg dst, mreg src1, mreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskand m%d, m%d, m%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    maskop(MXOR, dst, src1, src2);
}

void masknot(mreg dst, mreg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskand m%d, m%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    maskop(MNOT, dst, src1, mnone);
}

void mova_x_m(xreg dst, const char* comm)
{
    DISASM(gsprintf(dis,"I: mova.x.m x%d, allmasks%s%s",dst,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);

    uint64_t val = 0;
    for ( int m = 0; m < 8; m++ )
    {
        uint32_t start = m * 8;
        uint64_t msk   = 0;
        for ( int i = 0; i < 8; i++ )
        {
            msk  |= (MREGS[m].b[i] & 0x1) << i;
        }
        val |= msk << start;
        DEBUG_EMU(gprintf("\taccumulating into 0x%016llx reg m%d = 0x%08x \n",val,m,msk););
    }
    if(dst != x0)
        XREGS[dst].x = val;
    logxregchange(dst);
}

void mova_m_x(xreg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: mova.m.x allmasks, x%d%s%s",src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);

    uint64_t val = XREGS[src1].x;

    for ( int m = 0; m < 8; m++ )
    {
        uint32_t start = m * 8;
        uint64_t msk   = (val >> start) & 0xff;
        for ( int i = 0; i < 8; i++ )
        {
            MREGS[m].b[i] = (msk >> i) & 0x1;
            DEBUG_EMU(gprintf("\tm%d.b[%d] = 0x%08x \n",m,i,MREGS[m].b[i]););
        }
        logmregchange(m);
    }
}

void mov_m_x(mreg dst, xreg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: mov.m.x m%d, x%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);

    unsigned char val = XREGS[src1].b[0] | (imm & 0xFF);
    DEBUG_EMU(gprintf("\t0x%08x <- \n", val);)
    for ( int i = 0; i < 8; i++ )
    {
        MREGS[dst].b[i] = ( val >> i ) & 0x1;
        //DEBUG_EMU(gprintf("\tm%d.b[%d] = 0x%08x  (from val=0x%08x)\n",dst,i,MREGS[dst].b[i],val););
    }
    logmregchange(dst);
}

void maskpopc(xreg dst, mreg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskpopc x%d, m%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    uint64_t count = 0;
    for(int i = 0; i < 8; i++ )
    {
        count += (MREGS[src1].b[i] ? 1 : 0);
        DEBUG_EMU(gprintf("\tcount = %ld from m%d.b[%d] = %d\n",count,src1,i,MREGS[src1].b[i]););
    }
    if ( dst != x0 ) XREGS[dst].x = count;
    logxregchange(dst);
}

void maskpopcz(xreg dst, mreg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskpopcz x%d, m%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    uint64_t count = 0;
    for(int i = 0; i < 8; i++ )
    {
        count += (MREGS[src1].b[i] ? 0 : 1);
        DEBUG_EMU(gprintf("\tcount = %ld from m%d.b[%d] = %d \n",count,src1,i,MREGS[src1].b[i]););
    }
    if ( dst != x0 ) XREGS[dst].x = count;
    logxregchange(dst);
}

void maskpopc_rast(xreg dst, mreg src1, mreg src2, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: maskpopc.rast x%d, m%d, m%d, %d%s%s",dst,src1,src2,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    uint64_t count = 0;

    uint32_t mask;
    switch(imm)
    {
        case 0  : mask = 0x0f0f; break;
        case 1  : mask = 0x3c3c; break;
        case 2  : mask = 0xf0f0; break;
        default : mask = 0xffff; break;
    }

    for(int i = 0; i < 8; i++ )
    {
        count += ((MREGS[src1].b[i] & (mask & 0x1)) ? 1 : 0);
        DEBUG_EMU(gprintf("\tcount = %ld from m%d.b[%d] = %d m = %d \n",count,src1,i,MREGS[src1].b[i], mask & 0x1););
        mask = mask >> 1;
    }

    for(int i = 0; i < 8; i++ )
    {
        count += ((MREGS[src2].b[i] & (mask & 0x1)) ? 1 : 0);
        DEBUG_EMU(gprintf("\tcount = %ld from m%d.b[%d] = %d m = %d \n",count,src2,i,MREGS[src2].b[i], mask & 0x1););
        mask = mask >> 1;
    }

    if ( dst != x0 ) XREGS[dst].x = count;
    logxregchange(dst);
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto packed-single extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Load and store ------------------------------------

void flw_ps(freg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: flw.ps f%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femuld(FLW, 4, 4, dst, off,  base, 1);
}

void flq(freg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: flq f%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femuld(FLW, 4, 4, dst, off,  base, 0);
}

void fsw_ps(freg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsw.ps f%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femust(FSW, 4, 4, src1, off, base, 1);
}

void fsq(freg src1, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsq f%d, %d(x%d)%s%s",src1,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    femust(FSW, 4, 4, src1, off, base, 0);
}

// ----- Broadcast -----------------------------------------

void fbc_ps(freg dst, int off, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fbc_ps f%d, %d(x%d)%s%s",dst,off,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);

    uint64_t addr  = (XREGS[base].x  + off);
    uint8_t  b     = 0;
    uint32_t val   = 0;
    for ( int i = 0; i < 4; i++ )
    {
        b |= MREGS[0].b[i*2];
    }
    if ( b != 0 )
    {
        val   = memread32(addr);
    }
    for ( int i = 0; i < 4; i++ )
    {
        if ( MREGS[0].b[i*2] )
        {
            FREGS[dst].u[i] = val;
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <- MEM[0x%08x + 0x%016llx = 0x%016llx]\n",i,FREGS[dst].u[i],FREGS[dst].f[i],off,XREGS[base].x,addr););
        }
    }
    logfregchange(dst);
    IPC(ipc_ld(FBC,1,4,dst,base,(XREGS[base].x+off),dis);)
}

void fbci_ps(freg dst, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fbci_ps f%d, 0x%08x%s%s",dst,(imm&0xfffff),(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);

    uint32_t val = (imm & 0xfffff) << 12;  // make sure we only use 20b immediate and put into position

    // the low 4 bits of the immediate are replicated to fill the bottom 12 bits of the Fp number
    // Replication is as follows
    // let low be bits [3..0] of the immediate.
    // THen
    //  bits  [3..0] of the fp value are 'low' if low < 8 or low+1 otherwise
    //  bits  [7..4] of the fp value are 'low'
    //  bits [11..8] of the fp value are 'low'

    // take the low 4 bits of the immediate
    uint32_t low = (imm & 0xf);

    // do the replication
    low = low < 8 ? ((low<<8) | (low<<4) | low) :
                    ((low<<8) | (low<<4) | (low+1));

    // now merge low with the upper part of the immediate
    val = val | low;

    for ( int i = 0; i < 4; i++ )
    {
        if ( MREGS[0].b[i*2] )
        {
            FREGS[dst].u[i] = val;
            DEBUG_EMU( gprintf("\t[%d] 0x%08x (%f) <- 0x%08x\n", i, FREGS[dst].u[i], FREGS[dst].f[i], imm); );
        }
    }
    logfregchange(dst);
    IPC(ipc_ps(FBCI,4,dst,fnone,fnone,fnone,dis);)
}

void fbcx_ps(freg dst, xreg src, const char* comm)
{
    DISASM(gsprintf(dis,"I: fbcx_ps f%d, x%d%s%s",dst,src,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);

    for ( int i = 0; i < 4; i++ )
    {
        if ( MREGS[0].b[i*2] )
        {
            FREGS[dst].u[i] = XREGS[src].w[0];
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <- 0x%08x\n",i,FREGS[dst].u[i],FREGS[dst].f[i],XREGS[src].w[0]););
        }
    }
    logfregchange(dst);
    IPC(ipc_ps(FBCI,4,dst,fnone,fnone,fnone,dis);)
}

// ----- Gather and scatter --------------------------------

static void gatheremu(opcode opc, int size, freg dst, freg src1, xreg base)
{
    uint64_t baddr = XREGS[base].x;
    for ( int i = 0; i < 4; i++ )
    {
        int32_t off    = FREGS[src1].i[i];
        uint64_t addr  = baddr + off;

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i*2])
        {
            // notice here the use of 'int32_t' to force sign extension of the value
            iufval val;
            switch ( opc )
            {
                case FGW:  val.i   = memread32(addr); break;
                case FGH:  val.i   = (int32_t) ((int16_t) memread16(addr)); break;
                case FGB:  val.i   = (int32_t) ((int8_t) memread8(addr)); break;
                default :  assert(0); break;
            }
            FREGS[dst].i[i] = val.i;
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <- MEM[0x%08x + 0x%016llx = 0x%016llx]\n",i,FREGS[dst].i[i],FREGS[dst].f[i],off,baddr,addr););
            IPC(ipc_gt(opc,4,size,dst,src1,base,addr,dis, idx++);)
        }
    }
    logfregchange(dst);
}

static void gatheremu32(opcode opc, int size, freg dst, xreg src1, xreg src2)
{
    uint64_t baddr = XREGS[src2].x;
    uint64_t index = XREGS[src1].x;
    for ( int i = 0; i < 4; i++ )
    {
        uint64_t off;
        uint64_t addr;
        switch(size)
        {
            case 1 : off =  (index >> (i * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
            case 2 : off = ((index >> (i * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
            case 4 : off = ((index >> (i * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
            default: assert(0); break;
        }

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i*2])
        {
            // notice here the use of 'int32_t' to force sign extension of the value
            switch (size)
            {
                case 1 :  FREGS[dst].i[i] = (int32_t) ((int8_t)  memread8(addr));  break;
                case 2 :  FREGS[dst].i[i] = (int32_t) ((int16_t) memread16(addr)); break;
                case 4 :  FREGS[dst].i[i] =                  memread32(addr);  break;
                default: assert(0); break;
            }
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <- MEM[0x%08x + 0x%016llx = 0x%016llx]\n", i, FREGS[dst].i[i], FREGS[dst].f[i], off, baddr, addr););
        }
    }
    logfregchange(dst);
    IPC(ipc_ld(opc,4,size,dst,src1,src2,baddr,dis);)
}

static void femuscat(opcode opc, freg src1, freg src2, xreg base)
{
    uint64_t baddr = XREGS[base].x;
    for ( int i = 0; i < 4; i++ )
    {
        uint32_t val   = FREGS[src1].u[i];
        int32_t off    = FREGS[src2].i[i];
        uint64_t addr  = baddr + off;
        //
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        // notice here the use of 'int32_t' to force sign extension of the value
        switch (opc)
        {
            case FSCW : memwrite32(addr, (uint32_t)val); logmemwchange(i, 4, addr, val); break;
            case FSCH : memwrite16(addr, (uint16_t)val); logmemwchange(i, 2, addr, val); break;
            case FSCB : memwrite8(addr, (uint8_t)val);   logmemwchange(i, 1, addr, val); break;
            default   : assert(0); break;
        }

        // Scatter writes are not logged!!!!

        DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) --> MEM[0x%08x + 0x%016llx = 0x%016llx = %llu]\n",
            i, FREGS[src1].u[i], FREGS[src1].f[i], off, baddr, addr, addr););
    }
}

static void femuscat32(opcode opc, int size, freg src3, xreg src1, xreg src2)
{
    uint64_t baddr = XREGS[src2].x;
    uint64_t index = XREGS[src1].x;
    for ( int i = 0; i < 4; i++ )
    {
        uint64_t off;
        uint64_t addr;
        switch(size)
        {
            case 1 : off =  (index >> (i * 5)) & 0x01f      ; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01f); break;
            case 2 : off = ((index >> (i * 4)) & 0x00f) << 1; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01e); break;
            case 4 : off = ((index >> (i * 3)) & 0x007) << 2; addr = (baddr & ~0x01f) | ((baddr + off) & 0x01c); break;
            default: assert(0); break;
        }

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i*2])
        {
            switch (size)
            {
                case 1 : memwrite8( addr, (uint8_t)  FREGS[src3].u[i]); break;
                case 2 : memwrite16(addr, (uint16_t) FREGS[src3].u[i]); break;
                case 4 : memwrite32(addr, (uint32_t) FREGS[src3].u[i]); break;
                default: assert(0); break;
            }

            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) --> MEM[0x%08x + 0x%016llx = 0x%016llx = %llu]\n",
                i, FREGS[src3].u[i], FREGS[src3].f[i], off, baddr, addr, addr););

            // Do not track store swizzles?  Same with scatters.
            logmemwchange(i, size, addr, FREGS[src3].u[i]);
        }
    }
    IPC(ipc_st(opc, 4, size, src3, base, baddr, dis);)
}

void fgb_ps(freg dst, freg src1, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fgb.ps f%d, (f%d, x%d)%s%s",dst,src1,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGB, 1, dst, src1, base); 
}

void fgh_ps(freg dst, freg src1, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fgh.ps f%d, (f%d, x%d)%s%s",dst,src1,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGH, 2, dst, src1, base); 
}

void fgw_ps(freg dst, freg src1, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fgw.ps f%d, (f%d, x%d)%s%s",dst,src1,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu(FGW, 4, dst, src1, base); 
}

void fg32b_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fg32b.ps f%d, (x%d, x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32B, 1, dst, src1, src2); 
}

void fg32h_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fg32h.ps f%d, (x%d, x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32H, 2, dst, src1, src2); 
}

void fg32w_ps(freg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fg32w.ps f%d, (x%d, x%d)%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    gatheremu32(FG32W, 4, dst, src1, src2); 
}

void fscb_ps(freg src1, freg src2, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fscb.ps f%d, f%d, x%d%s%s",src1,src2,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCB, src1, src2, base); 
}

void fsch_ps(freg src1, freg src2, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsch.ps f%d, f%d, x%d%s%s",src1,src2,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCH, src1, src2, base); 
}

void fscw_ps(freg src1, freg src2, xreg base, const char* comm)
{
    DISASM(gsprintf(dis,"I: fscw.ps f%d, f%d, x%d%s%s",src1,src2,base,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat(FSCW, src1, src2, base); 
}

void fsc32b_ps(freg src3, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsc32b.ps f%d, (x%d, x%d)%s%s",src3,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat32(FSC32B, 1, src3, src1, src2); 
}

void fsc32h_ps(freg src3, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsc32h.ps f%d, (x%d, x%d)%s%s",src3,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat32(FSC32H, 2, src3, src1, src2); 
}

void fsc32w_ps(freg src3, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsc32w.ps f%d, (x%d, x%d)%s%s",src3,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    femuscat32(FSC32W, 4, src3, src1, src2); 
}

// ----- Computational (follows RV64F) ---------------------

static void fmask(opcode opc, mreg dst, freg src1, freg src2)
{
    iufval val1, val2;

    for ( int i = 0; i < 4; i++ )
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        val1.f  = FREGS[src1].f[i];

        // For FSET, don't read the second sourc
        if ( src2 != fnone ) { val2.f  = FREGS[src2].f[i]; } else { val2.u = 0; }

        iufval res;
        switch ( opc )
        {
            case FLT:    res.u  = (val1.f < val2.f) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- 0x%08x (%f) < 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            case FLE:    res.u  = (val1.f <= val2.f) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- 0x%08x (%f) <= 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            case FEQ:    res.u  = (val1.u == val2.u) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- 0x%08x (%f) == 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            case FSET:   res.u  = (val1.u) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- 0x%08x ? 1 : 0\n",i,res.u,val1.u);)
                         break;
            case FLTPI:  res.u  = (val1.i < val2.i) ? 1 : 0;
                         DEBUG_EMU(gprintf("\t[%d] %d <-- 0x%08x (%f) < 0x%08x (%f)?\n",i,res.u,val1.u,val1.f,val2.u,val2.f);)
                         break;
            default:     assert(0);
                         break;
        }
        MREGS[dst].b[i*2] = res.u;
        MREGS[dst].b[i*2+1] = res.u;
    }
    logmregchange(dst);
    IPC(ipc_msk(opc,dst,src1,src2,dis);)
}

static void fswizz(opcode opc, freg dst, freg src1, uint8_t imm)
{
    fdata val = FREGS[src1];

    if ( MREGS[0].b[0] )
    {
        FREGS[dst].u[0] = val.u[(imm)     & 0x3];
        DEBUG_EMU(gprintf("\t[0] 0x%08x <-- 0x%08x (chan %d)\n", FREGS[dst].u[0], val.u[ imm       & 0x3],  imm       & 0x3););
    }

    if ( MREGS[0].b[2] )
    {
        FREGS[dst].u[1] = val.u[(imm>>2)  & 0x3];
        DEBUG_EMU(gprintf("\t[1] 0x%08x <-- 0x%08x (chan %d)\n", FREGS[dst].u[1], val.u[(imm >> 2) & 0x3], (imm >> 2) & 0x3););
    }

    if ( MREGS[0].b[4] )
    {
        FREGS[dst].u[2] = val.u[(imm>>4)  & 0x3];
        DEBUG_EMU(gprintf("\t[2] 0x%08x <-- 0x%08x (chan %d)\n", FREGS[dst].u[2], val.u[(imm >> 4) & 0x3], (imm >> 4) & 0x3););
    }

    if ( MREGS[0].b[6] )
    {
        FREGS[dst].u[3] = val.u[(imm>>6)  & 0x3];
        DEBUG_EMU(gprintf("\t[3] 0x%08x <-- 0x%08x (chan %d)\n", FREGS[dst].u[3], val.u[(imm >> 6) & 0x3], (imm >> 6) & 0x3););
    }

    logfregchange(dst);
    IPC(ipc_ps(opc,4,dst,src1,fnone,fnone,dis);)
}

void fadd_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fadd.ps f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FADD, 4, dst, src1, src2, rm); 
}

void fsub_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsub.ps f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FSUB, 4, dst, src1, src2, rm); 
}

void fmul_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmul.ps f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FMUL, 4, dst, src1, src2, rm); 
}

void fdiv_ps(freg dst, freg src1, freg src2, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fdiv.ps f%d, f%d, f%d, %s%s%s",dst,src1,src2,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FDIV, 4, dst, src1, src2, rm); 
}

void fsgnj_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnj.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJ, 4, dst, src1, src2, rmdyn); 
}

void fsgnjn_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnjn.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJN, 4, dst, src1, src2, rmdyn); 
}

void fsgnjx_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsgnjx.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FSGNJX, 4, dst, src1, src2, rmdyn); 
}

void fmin_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmin.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FMIN, 4, dst, src1, src2, rmdyn); 
}

void fmax_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmax.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FMAX, 4, dst, src1, src2, rmdyn); 
}

void fsqrt_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsqrt.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FSQRT, 4, dst, src1, rm); 
}

void feq_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: feq.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FEQ, 4, dst, src1, src2, rmdyn); 
}

void fle_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fle.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FLE, 4, dst, src1, src2, rmdyn); 
}

void flt_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: flt.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FLT, 4, dst, src1, src2, rmdyn); 
}

void feqm_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: feqm.ps m%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fmask(FEQ, dst, src1, src2); 
}

void flem_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: flem.ps m%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fmask(FLE, dst, src1, src2); 
}

void fltm_ps(mreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fltm.ps m%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fmask(FLT, dst, src1, src2); 
}

void fsetm_ps(mreg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsetm.ps m%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fmask(FSET, dst, src1, fnone); 
}

void fcmov_ps(freg dst, freg src1, freg src2, freg src3, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcmov.ps f%d, f%d, f%d, f%d%s%s",dst,src1,src2,src3,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu3src(FCMOV, 4, dst, src1, src2, src3, rmdyn); 
}

void fcmovm_ps(freg dst, freg src1, freg src2, const char* comm)
{
    iufval val1, val2, res;

    DISASM(gsprintf(dis,"I: fcmovm.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);

    for ( int i = 0; i < 4; i++ )
    {
        val1.u  = FREGS[src1].u[i];
        val2.u  = FREGS[src2].u[i];
        int sel = MREGS[0].b[i*2];
        res.u   = sel ? val1.u : val2.u;
        DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- %d ? 0x%08x (%f) : 0x%08x (%f)\n",i,res.u,res.f,sel,val1.u,val1.f,val2.u,val2.f););
        FREGS[dst].u[i] = res.u;
    }
    logfregchange(dst);
    IPC(ipc_ps(FCMOV, 4,dst,src1,src2,fnone,dis);)
}

void fmvz_x_ps(xreg dst, freg src1, uint8_t index, const char* comm)
{
    DISASM( gsprintf(dis,"I: fmvz.x.ps x%d, f%d, %d%s%s",dst,src1,index,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    IPC(ipc_f2x(FMVZXPS,dst,src1,dis););

    index = index & 0x03;
    if(dst != x0)
        XREGS[dst].x = FREGS[src1].u[index];
    DEBUG_EMU(gprintf("\t 0x%08x (%d)\n", XREGS[dst].x, XREGS[dst].x););
    logxregchange(dst);
}

void fmvs_x_ps(xreg dst, freg src1, uint8_t index, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmvs.x.ps x%d, f%d, %d %s%s",dst,src1,index,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););

    index = index & 0x03;
    if(dst != x0)
        XREGS[dst].x = sext32(FREGS[src1].u[index]);
    DEBUG_EMU(gprintf("\t 0x%08x (%d) <- {0x%08x (%d), 0x%08x (%d), 0x%08x (%d), 0x%08x (%d)}[%d]\n",
                      XREGS[dst].x, XREGS[dst].x,
                      FREGS[src1].u[0], FREGS[src1].u[0],
                      FREGS[src1].u[1], FREGS[src1].u[1],
                      FREGS[src1].u[2], FREGS[src1].u[2],
                      FREGS[src1].u[3], FREGS[src1].u[3],
                      index);)
    logxregchange(dst);
}

void fswizz_ps(freg dst, freg src1, uint8_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fswizz.ps f%d, f%d, %u%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fswizz(FSWIZZ, dst, src1, imm);
}

void fcvt_pw_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.pw.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPWPS, 4, dst, src1, rm); 
}

void fcvt_pwu_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.pwu.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPWUPS, 4, dst, src1, rm); 
}

void fclass_ps(freg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fclass.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCLASSPS, 4, dst, src1, rmdyn); 
}

void fcvt_ps_pw(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.pw f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSPW, 4, dst, src1, rm); 
}

void fcvt_ps_pwu(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.pwu f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSPWU, 4, dst, src1, rm); 
}

void fmadd_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmadd.ps f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu3src(FMADD, 4, dst, src1, src2, src3, rm); 
}

void fmsub_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmsub.ps f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu3src(FMSUB, 4, dst, src1, src2, src3, rm); 
}

void fnmsub_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fnmsub.ps f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu3src(FNMSUB, 4, dst, src1, src2, src3, rm); 
}

void fnmadd_ps(freg dst, freg src1, freg src2, freg src3, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fnmadd.ps f%d, f%d, f%d, f%d, %s%s%s",dst,src1,src2,src3,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu3src(FNMADD, 4, dst, src1, src2, src3, rm); 
}

// ----- Graphics upconvert --------------------------------

static void ucvtemu(opcode opc, freg dst, freg src1, rounding_mode rm)
{
    for ( int i = 0; i < 4; i++ )
    {
        uint32_t val = FREGS[src1].u[i];

        // Forcing to 0 in case of denormal input
        if ((opc == FCVTPSF16) && ((val & 0x7c00) == 0)) {
          val = val & 0x8000;
        }

        if ((opc == FCVTPSF11) && ((val & 0x7c0) == 0)) {
          val = 0;
        }

        if ((opc == FCVTPSF10) && ((val & 0x3e0) == 0)) {
          val = 0;
        }

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        bool genResult = ( MREGS[0].b[i*2] != 0 );

        iufval res;

        if ( genResult )
        {
            switch ( opc )
            {
#ifdef NEW_UPCONVERT
                case FCVTPSF16:  res.f = float16tofloat32(val >> 16); break;
                case FCVTPSF11:  res.f = float11tofloat32(val >> 21); break;
                case FCVTPSF10:  res.f = float10tofloat32(val >> 22); break;
                case FCVTPSUN24: res.f = unorm24tofloat32(val >>  8); break;
                case FCVTPSUN16: res.f = unorm16tofloat32(val >> 16); break;
                case FCVTPSUN10: res.f = unorm10tofloat32(val >> 22); break;
                case FCVTPSUN8:  res.f =  unorm8tofloat32(val >> 24);  break;
                case FCVTPSUN2:  res.f =  unorm2tofloat32(val >> 30);  break;
                case FCVTPSSN16: res.f = snorm16tofloat32(val >> 16); break;
                case FCVTPSSN8:  res.f =  snorm8tofloat32(val >> 24);  break;
#else
                case FCVTPSF16:  res.f = float16tofloat32(val); break;
                case FCVTPSF11:  res.f = float11tofloat32(val); break;
                case FCVTPSF10:  res.f = float10tofloat32(val); break;
                case FCVTPSUN24: res.f = unorm24tofloat32(val); break;
                case FCVTPSUN16: res.f = unorm16tofloat32(val); break;
                case FCVTPSUN10: res.f = unorm10tofloat32(val); break;
                case FCVTPSUN8:  res.f = unorm8tofloat32(val);  break;
                case FCVTPSUN2:  res.f = unorm2tofloat32(val);  break;
                //case FCVTPSSN24: res.f = snorm24tofloat32(val); break;
                case FCVTPSSN16: res.f = snorm16tofloat32(val); break;
                //case FCVTPSSN10: res.f = snorm10tofloat32(val); break;
                case FCVTPSSN8:  res.f = snorm8tofloat32(val);  break;
                //case FCVTPSSN2:  res.f = snorm2tofloat32(val);  break;
#endif
                default: assert(0); break;
            }
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%d)\n",i,res.u,res.f,val,val);)
            FREGS[dst].f[i] = res.f;
        }
    }
    logfregchange(dst);
    IPC(ipc_ps(opc,4,dst,src1,fnone,fnone,dis);)
}

void fcvt_ps_f16(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.f16 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF16, dst, src1, rm); 
}

void fcvt_ps_f11(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.f11 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF11, dst, src1, rm); 
}

void fcvt_ps_f10(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.f10 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSF10, dst, src1, rm); 
}

void fcvt_ps_un24(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.un24 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN24, dst, src1, rm); 
}

void fcvt_ps_un16(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.un16 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN16, dst, src1, rm); 
}

void fcvt_ps_un10(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.un10 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN10, dst, src1, rm); 
}

void fcvt_ps_un8(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.un8 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN8, dst, src1, rm); 
}

void fcvt_ps_un2(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.un2 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSUN2, dst, src1, rm); 
}

void fcvt_ps_sn16(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.sn16 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSSN16, dst, src1, rm); 
}

void fcvt_ps_sn8(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.sn8 f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    ucvtemu(FCVTPSSN8, dst, src1, rm); 
}

// ----- Graphics downconvert ------------------------------

static void dcvtemu(opcode opc, freg dst, freg src1, rounding_mode rm)
{
    for ( int i = 0; i < 4; i++ )
    {
        float32_t val  = FREGS[src1].f[i];

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        bool genResult = ( MREGS[0].b[i*2] != 0 );

        uint32_t res;

        if ( genResult )
        {
            switch ( opc )
            {
                case FCVTF10PS:  res  = float32tofloat10(val) ; break;
                case FCVTF11PS:  res  = float32tofloat11(val) ; break;
                case FCVTF16PS:  res  = float32tofloat16(val) ; break;
                case FCVTUN24PS: res  = float32tounorm24(val) ; break;
                case FCVTUN16PS: res  = float32tounorm16(val) ; break;
                case FCVTUN10PS: res  = float32tounorm10(val) ; break;
                case FCVTUN8PS:  res  = float32tounorm8(val)  ; break;
                case FCVTUN2PS:  res  = float32tounorm2(val)  ; break;
                //case FCVTSN24PS: res  = float32tosnorm24(val) ; break;
                case FCVTSN16PS: res  = float32tosnorm16(val) ; break;
                case FCVTSN8PS:  res  = float32tosnorm8(val)  ; break;
                default:         assert(0)                    ; break;
            }
            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- down- 0x%08x (%f)\n",i,res,res,cast_float32_to_uint32(val),val);)
            FREGS[dst].u[i] = res;
        }
    }
    logfregchange(dst);
    IPC(ipc_ps(opc,4,dst,src1,fnone,fnone,dis);)
}

void fcvt_f16_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.f16.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF16PS, dst, src1, rm); 
}

void fcvt_f11_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.f11.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF11PS, dst, src1, rm); 
}

void fcvt_f10_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.f10.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTF10PS, dst, src1, rm); 
}

void fcvt_un24_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.un24.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN24PS, dst, src1, rm); 
}

void fcvt_un16_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.un16.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN16PS, dst, src1, rm); 
}

void fcvt_un10_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.un10.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN10PS, dst, src1, rm); 
}

void fcvt_un8_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.un8.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN8PS, dst, src1, rm); 
}

void fcvt_un2_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.un2.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTUN2PS, dst, src1, rm); 
}

void fcvt_sn16_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.sn16.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTSN16PS, dst, src1, rm); 
}

void fcvt_sn8_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.sn8.ps f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);
    dcvtemu(FCVTSN8PS, dst, src1, rm); 
}

// ----- Graphics additional -------------------------------

void fsin_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsin.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FSIN, 4, dst, src1, rm); 
}

void fexp_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fexp.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FEXP, 4, dst, src1, rm); 
}

void flog_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: flog.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FLOG, 4, dst, src1, rm); 
}

void ffrc_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: ffrc.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FFRC, 4, dst, src1, rm); 
}

void fround_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    iufval val;
    DISASM(gsprintf(dis,"I: fround.ps f%d, f%d using rounding mode %d%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    for ( int i = 0; i < 4; i++ )
    {
        val.f = FREGS[src1].f[i];

        if ( MREGS[0].b[i*2] == 0 ) continue;

        iufval res;
        if (isnan(val.f))
            res.f = nanf("");
        else  // use c++ functions
            res.f = roundf(val.f,  rm);
        DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) <-- 0x%08x (%f) \n",i,res.x,res.f,val.x,val.f););
        FREGS[dst].f[i] = res.f;
    }
    logfregchange(dst);
    IPC(ipc_ps(opc,4,dst,src1,fnone,fnone,dis););
}

void frcp_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: frcp.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FRCP, 4, dst, src1, rm); 
}

void frsq_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: frsq.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FRSQ, 4, dst, src1, rm); 
}

void frcpfxp_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: frcpfxp.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FRCPFXP, 4, dst, src1, rm); 
}

void cubeface_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: cubeface.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n", dis););
    DEBUG_MASK(MREGS[0]);

    for(int i = 0; i < 4; i++)
    {
        // check the corresponding mask bit. If not set, skip this lane
        if (MREGS[0].b[i*2] == 0) continue;

        uint32_t rz_lt_ry =  (FREGS[dst].u[i]) & 0x1;
        uint32_t rz_lt_rx = (FREGS[src1].u[i]) & 0x1;
        uint32_t ry_lt_rx = (FREGS[src2].u[i]) & 0x1;

        uint32_t res = rz_lt_ry ? (ry_lt_rx ? 0 : 1) : (rz_lt_rx ? 0 : 2);

        DEBUG_EMU(gprintf("\t[%d] %d <-- %d %d %d\n", i, res, rz_lt_ry, rz_lt_rx, ry_lt_rx););

        FREGS[dst].u[i] = res;
    }

    logfregchange(dst);
    IPC(ipc_ps(CUBEFACE, 4, dst, dst, src1, src2, dis););
}

void cubefaceidx_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: cubefaceidx.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n", dis););
    DEBUG_MASK(MREGS[0]);

    for ( int i = 0; i < 4; i++ )
    {
        // check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        uint32_t face = (FREGS[src1].u[i])&0x3;
        float32_t rc = FREGS[src2].f[i];

        float32_t res = (face==0x3) ? nanf("") : (rc < 0) ? float32_t(face * 2 + 1) : float32_t(face * 2);

        DEBUG_EMU(gprintf("\t[%d] %d <-- %d %f\n", i, res, face, rc););
        FREGS[dst].f[i] = res;
    }

    logfregchange(dst);
    IPC(ipc_ps(CUBEFACEIDX, 4, dst, dst, src1, fnone, dis););
}

void cubesgnsc_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: cubesgnsc.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n", dis););
    DEBUG_MASK(MREGS[0]);

    for ( int i = 0; i < 4; i++ )
    {
        // check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        uint32_t face = (FREGS[src1].u[i])&0x7;
        float32_t sc = FREGS[src2].f[i];

        float32_t res = ((face == 0) || (face == 5)) ? -fabs(sc) : fabs(sc);

        DEBUG_EMU(gprintf("\t[%d] 0x08%x [%f] <-- %d %f\n", i, res, res, face, sc););
        FREGS[dst].f[i] = res;
    }

    logfregchange(dst);
    IPC(ipc_ps(CUBESGNSC, 4, dst, dst, src1, fnone, dis););
}

void cubesgntc_ps(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: cubesgntc.ps f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n", dis););
    DEBUG_MASK(MREGS[0]);

    for ( int i = 0; i < 4; i++ )
    {
        // check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        uint32_t face = (FREGS[src1].u[i])&0x7;
        float32_t tc = FREGS[src2].f[i];

        float32_t res = (face == 2) ? fabs(tc) : -fabs(tc);

        DEBUG_EMU(gprintf("\t[%d] 0x%08x [%f] <-- %d %f\n", i, res, res, face, tc););
        FREGS[dst].f[i] = res;
    }

    logfregchange(dst);
    IPC(ipc_ps(CUBESGNTC, 4, dst, dst, src1, fnone, dis););
}

void fcvt_ps_rast(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.ps.rast f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTPSRAST, 4, dst, src1, rm); 
}

void fcvt_rast_ps(freg dst, freg src1, rounding_mode rm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fcvt.rast.ps f%d, f%d, %s%s%s",dst,src1,rmnames[rm],(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu1src(FCVTRASTPS, 4, dst, src1, rm); 
}

void frcp_fix_rast(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: frcp.fix.rast f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    femu2src(FRCP_FIX_RAST, 4, dst, src1, src2, rmdyn); 
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto packed-integer extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Broadcast -----------------------------------------

void fbci_pi(freg dst, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fbci.pi f%d, 0x%08x%s%s",dst,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis);)
    DEBUG_MASK(MREGS[0]);

    uint32_t sgn = imm & 0x80000;
    uint32_t val = imm & 0xfffff;  // Make sure the imm is really only 20b long
    val = sgn ?  (0xfff00000 | val) : val;

    for ( int i = 0; i < 4; i++ )
    {
        if ( MREGS[0].b[i*2] )
        {
            FREGS[dst].i[i] = val;
            DEBUG_EMU(gprintf("\t[%d] 0x%08x <- 0x%08x\n",i,FREGS[dst].i[i],val););
        }
    }
    IPC(ipc_pi(FBCI,4,dst,fnone,fnone,fnone,dis);)
    logfregchange(dst);
}

// ----- Computational (follows RV64I/F/M) -----------------

static void iemu2src(opcode opc, freg dst, freg src1, freg src2)
{
    for ( int i = 0; i < 4; i++ )
    {
        int32_t   val1 = FREGS[src1].i[i];
        int32_t   val2 = src2 != fnone? FREGS[src2].i[i] : 0;
        uint32_t uval1 = FREGS[src1].u[i];
        uint32_t uval2 = src2 != fnone ? FREGS[src2].u[i] : 0;
        uint32_t isu = 0;

        // for packed single, check the corresponding mask bit. If not set, skip this lane
        if ( MREGS[0].b[i*2] == 0 ) continue;

        int32_t res;
        uint32_t ures;
        switch ( opc )
        {
            case FADDPI :   res  = val1 + val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x + 0x%08x\n",i,res,val1,val2);)
                            break;
            case FSUBPI :   res  = val1 - val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x - 0x%08x\n",i,res,val1,val2);)
                            break;
            case FMULPI :   res  = val1 * val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x * 0x%08x\n",i,res,val1,val2);)
                            break;
            case FMULHPI :
                            {
                                int64_t res_full;
                                res_full = int64_t(val1) * int64_t(val2);
                                res  = int32_t((res_full >> 32) & 0xFFFFFFFF);
                                DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x * 0x%08x\n",i,res,val1,val2);)
                            }
                            break;
            case FMULHUPI :
                            {
                                uint64_t res_full;
                                res_full = uint64_t(uval1) * uint64_t(uval2);
                                ures  = uint32_t((res_full >> 32) & 0xFFFFFFFF);
                                isu = 1;
                                DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x * 0x%08x\n",i,ures,uval1,uval2);)
                            }
                            break;
//                        case FMULHSUPI :
//                                        {
//                                            uint64_t res_full;
//                                            res_full = int64_t(val1) * uint64_t(uval2);
//                                            ures  = uint32_t((res_full >> 32) & 0xFFFFFFFF);
//                                            isu = 1;
//                                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x * 0x%08x\n",i,ures,uval1,uval2);)
//                                        }
//                                        break;
            case FDIVPI :   res  = val1 / val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x / 0x%08x\n",i,res,val1,val2);)
                            break;
            case FDIVUPI :  ures  = uval2 ? uval1 / uval2 : 0xFFFFFFFF;
                            isu = 1;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%u) <-- 0x%08x (%u) /u 0x%08x (%u)\n",i,ures,ures,uval1,uval1,uval2,uval2);)
                            break;
            case FREMPI  :  res  = val2 ? val1 % val2 : 0xFFFFFFFF;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%d) %%u 0x%08x (%d)\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FREMUPI :  ures  = uval2 ? uval1 % uval2 : 0xFFFFFFFF;
                            isu = 1;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%u) <-- 0x%08x (%u) %%u 0x%08x (%u)\n",i,ures,ures,uval1,uval1,uval2,uval2);)
                            break;
            case FMAXPI :   res  = val1 >= val2 ? val1 : val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- min(0x%08x (%d), 0x%08x (%d) )\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FMINPI :   res  = val1 < val2 ? val1 : val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- min(0x%08x (%d), 0x%08x (%d) )\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FMAXUPI :  res  = uval1 >= uval2 ? uval1 : uval2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- max(0x%08x (%d), 0x%08x (%d) )\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FMINUPI :  res  = uval1 < uval2 ? uval1 : uval2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- min(0x%08x (%d), 0x%08x (%d) )\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FANDPI :   res  = val1 & val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x & 0x%08x\n",i,res,val1,val2);)
                            break;
            case FORPI :    res  = val1 | val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x | 0x%08x\n",i,res,val1,val2);)
                            break;
            case FXORPI :   res  = val1 ^ val2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x ^ 0x%08x\n",i,res,val1,val2);)
                            break;
            case FNOTPI :   res  = ~val1;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- ~ 0x%08x\n",i,res,val1);)
                            break;
            case FSAT8PI :  res = ((val1 > 127) ? 127 :(val1 < -128 ? -128 : val1)) & 0x0FF;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- ~ 0x%08x\n",i,res,val1);)
                            break;
            case FSLLPI :   if (uval2 >= 32)
                                res  = 0;
                            else
                                res  = uval1 << uval2;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x << %d\n",i,res,val1,val2);)
                            break;
            case FSRLPI :   if (uval2 >= 32)
                                res = 0;
                            else
                            res  = int32_t(uint32_t(val1) >> uint32_t(uval2));
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x >> %d\n",i,res,val1,val2);)
                            break;
            case FSRAPI :   if (uval2 >= 32)
                                res = 0;
                            else
                                res  = val1 >> uint32_t(uval2);
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x >> %d\n",i,res,val1,val2);)
                            break;
            case FLTPI :    res  = (val1 < val2) ? 0xFFFFFFFF : 0;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%d) < 0x%08x (%d) \n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FLTUPI :   ures  = (uval1 < uval2) ? 0xFFFFFFFF : 0;
                            isu = 1;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%u) < 0x%08x (%u) \n",i,ures,ures,uval1,uval1,uval2,uval2);)
                            break;
            case FLEPI :    res  = (val1 <= val2) ? 0xFFFFFFFF : 0;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%d) <= 0x%08x (%d)\n",i,res,res,val1,val1,val2,val2);)
                            break;
            case FEQPI :    res  = (val1 == val2) ? 0xFFFFFFFF : 0;
                            DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%d) == 0x%08x (%d)\n",i,res,res,val1,val1,val2,val2);)
                            break;
            default:        assert(0);
                            break;
        }
        if ( isu )
            FREGS[dst].u[i] = ures;
        else
            FREGS[dst].i[i] = res;
    }
    logfregchange(dst);
    IPC(ipc_pi(opc,4,dst,src1,src2,fnone,dis);)
}

static void iemu2srcimm(opcode opc, freg dst, freg src1, uint32_t imm)
{
    for ( int i = 0; i < 4; i++ )
    {
        // for packed single, check the corresponding mask bit. If not set, skip this lane
        bool genResult = !( MREGS[0].b[i*2] == 0 );

        int32_t val1 = FREGS[src1].i[i];
        int32_t val2 = sext10(imm); // sign extend the 10-low order bits of imm

        int32_t res;

        if ( genResult )
        {
            switch ( opc )
            {
                case FADDIPI: res  = val1 + val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x (%d) <-- 0x%08x (%d) + 0x%08x (%d)\n",i,res,res,val1,val1,val2,val2););
                              break;
                case FANDIPI: res  = val1 & val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x & 0x%08x\n",i,res,val1,val2););
                              break;
                case FORIPI:  res  = val1 | val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x | 0x%08x\n",i,res,val1,val2););
                              break;
                case FXORIPI: res  = val1 ^ val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x ^ 0x%08x\n",i,res,val1,val2););
                              break;
                case FSLLIPI: res  = val1 << val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x << %d\n",i,res,val1,val2););
                              break;
                case FSRLIPI: res  = int32_t(uint32_t(val1) >> uint32_t(val2));
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x >> %d\n",i,res,val1,val2););
                              break;
                case FSRAIPI: res  = val1 >> val2;
                              DEBUG_EMU(gprintf("\t[%d] 0x%08x <-- 0x%08x >> %d\n",i,res,val1,val2););
                              break;
                default:      assert(0);
                              break;
            }
            FREGS[dst].i[i] = res;
        }

    }
    logfregchange(dst);
    IPC(ipc_pi(opc,4,dst,src1,fnone,fnone,dis);)
}

static void packrep(opcode opc, freg dst, freg src1)
{
    fdata val = FREGS[src1];
    switch (opc)
    {
        case FPACKREPHPI :
            if ( MREGS[0].b[0] ) FREGS[dst].h[0] = val.h[0];
            if ( MREGS[0].b[1] ) FREGS[dst].h[1] = val.h[2];
            if ( MREGS[0].b[2] ) FREGS[dst].h[2] = val.h[4];
            if ( MREGS[0].b[3] ) FREGS[dst].h[3] = val.h[6];
            if ( MREGS[0].b[4] ) FREGS[dst].h[4] = val.h[0];
            if ( MREGS[0].b[5] ) FREGS[dst].h[5] = val.h[2];
            if ( MREGS[0].b[6] ) FREGS[dst].h[6] = val.h[4];
            if ( MREGS[0].b[7] ) FREGS[dst].h[7] = val.h[6];
            break;
        case FPACKREPBPI:
            if ( MREGS[0].b[0] ) FREGS[dst].u[0] = val.b[0] | (val.b[4] << 8) | (val.b[8] << 16) | (val.b[12] << 24);
            if ( MREGS[0].b[2] ) FREGS[dst].u[1] = val.b[0] | (val.b[4] << 8) | (val.b[8] << 16) | (val.b[12] << 24);
            if ( MREGS[0].b[4] ) FREGS[dst].u[2] = val.b[0] | (val.b[4] << 8) | (val.b[8] << 16) | (val.b[12] << 24);
            if ( MREGS[0].b[6] ) FREGS[dst].u[3] = val.b[0] | (val.b[4] << 8) | (val.b[8] << 16) | (val.b[12] << 24);
            break;
        default:
            assert(0);
            break;
    }

    DEBUG_EMU(gprintf("\t[0] 0x%08x <-- 0x%08x\n", FREGS[dst].u[0], val.u[0]););
    DEBUG_EMU(gprintf("\t[1] 0x%08x <-- 0x%08x\n", FREGS[dst].u[1], val.u[1]););
    DEBUG_EMU(gprintf("\t[2] 0x%08x <-- 0x%08x\n", FREGS[dst].u[2], val.u[2]););
    DEBUG_EMU(gprintf("\t[3] 0x%08x <-- 0x%08x\n", FREGS[dst].u[3], val.u[3]););

    logfregchange(dst);
    IPC(ipc_ps(opc,4,dst,src1,fnone,fnone,dis);)
}

void feq_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: feq.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FEQPI, dst, src1, src2); 
}

void fle_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fle.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLEPI, dst, src1, src2); 
}

void flt_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: flt.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLTPI, dst, src1, src2); 
}

void fltu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fltu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FLTUPI, dst, src1, src2); 
}

void fltm_pi(mreg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fltm.pi m%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    fmask(FLTPI, dst, src1, src2);
}

void faddi_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: faddi.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FADDIPI, dst, src1, imm); 
}

void fslli_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fslli.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSLLIPI, dst, src1, imm); 
}

void fxori_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fxori.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FXORIPI, dst, src1, imm); 
}

void fsrli_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsrli.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSRLIPI, dst, src1, imm); 
}

void fsrai_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsrai.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FSRAIPI, dst, src1, imm); 
}

void fori_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fori.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FORIPI, dst, src1, imm); 
}

void fandi_pi(freg dst, freg src1, uint32_t imm, const char* comm)
{
    DISASM(gsprintf(dis,"I: fandi.pi f%d, f%d, 0x%08x%s%s",dst,src1,imm,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2srcimm(FANDIPI, dst, src1, imm); 
}

void fadd_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fadd.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FADDPI, dst, src1, src2); 
}

void fsub_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsub.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSUBPI, dst, src1, src2); 
}

void fsll_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsll.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSLLPI, dst, src1, src2); 
}

void fxor_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fxor.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FXORPI, dst, src1, src2); 
}

void fsrl_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsrl.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSRLPI, dst, src1, src2); 
}

void fsra_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsra.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSRAPI, dst, src1, src2); 
}

void for_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: for.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FORPI, dst, src1, src2); 
}

void fand_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fand.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FANDPI, dst, src1, src2); 
}

void fnot_pi(freg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fnot.pi f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FNOTPI, dst, src1, fnone); 
}

void fsat8_pi(freg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fsat8.pi f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FSAT8PI, dst, src1, fnone); 
}

void fpackreph_pi(freg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fpackreph.pi f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    packrep(FPACKREPHPI, dst, src1); 
}

void fpackrepb_pi(freg dst, freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: fpackrepb.pi f%d, f%d%s%s",dst,src1,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    packrep(FPACKREPBPI, dst, src1); 
}

void fmul_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmul.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULPI, dst, src1, src2); 
}

void fmulh_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmulh.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULHPI, dst, src1, src2); 
}

void fmulhu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmulhu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMULHUPI, dst, src1, src2); 
}

void fdiv_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fdiv.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FDIVPI, dst, src1, src2); 
}

void fdivu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fdivu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FDIVUPI, dst, src1, src2); 
}

void frem_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: frem.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FREMPI, dst, src1, src2); 
}

void fremu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fremu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FREMUPI, dst, src1, src2); 
}

void fmin_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmin.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMINPI, dst, src1, src2); 
}

void fmax_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmax.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMAXPI, dst, src1, src2); 
}

void fminu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fminu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
    DEBUG_MASK(MREGS[0]);
    iemu2src(FMINUPI, dst, src1, src2); 
}

void fmaxu_pi(freg dst, freg src1, freg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: fmaxu.pi f%d, f%d, f%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:"")););
    DEBUG_EMU(gprintf("%s\n",dis););
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
    DISASM(gsprintf(dis,"I: packb x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
    uint64_t val = (XREGS[src1].x & 0x0FF) | ((XREGS[src2].x << 8) & 0x0FF00);
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx + 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

void bitmixb(xreg dst, xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: bitmixb x%d, x%d, x%d%s%s",dst,src1,src2,(comm?" # ":""),(comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis);)
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
    if(dst != x0)
    {
        XREGS[dst].x = val;
        DEBUG_EMU(gprintf("\t0x%016llx  <- 0x%016llx + 0x%016llx\n",val,XREGS[src1].x,XREGS[src2].x);)
    }
    logxregchange(dst);
    IPC(ipc_int(SIMPLE_INT,dst,src1,src2,dis);)
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto atomic extension emulation
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto cache control extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- Scratchpad emulation --------------------------------------------------

static bool     scp_locked[EMU_NUM_MINIONS][64]; // A cacheline is locked
static uint64_t scp_trans[EMU_NUM_MINIONS][64];  // Which PA the cacheline is mapped to

uint64_t get_scratchpad_value(int entry, int block, int * last_entry, int * size)
{
    * last_entry = scp_entry[current_thread];
    * size = scp_size[current_thread];
    return SCP[entry][block >> 1].x[block & 1];
}

void get_scratchpad_conv_list(std::list<bool> * list)
{
    for(int i = 0; i < 16; i++)
        list->push_back(scp_tm && !tmask_pass(i));
}

// ----- CacheOp emulation -----------------------------------------------------

static uint64_t csr_cacheop_emu(uint64_t op_value)
{
    uint64_t tm     = op_value >> 63;
    uint64_t op     = (op_value >> 60) & 0x7;
    uint64_t dest   = (op_value >> 58) & 0x3;
    uint64_t start  = (op_value >> 56) & 0x3;
    uint64_t addr   = op_value & 0xFFFFFFFFFFC0UL;
    int      repeat = (op_value & 0xF) + 1;
    
    uint64_t stride = XREGS[31].x & 0xFFFFFFFFFFC0UL;

    uint64_t set  = (addr >> 6) & 0xFFFFFF;
    uint64_t way  = (op_value >> 48) & 0xFF;
    // uint64_t cl   = (set << 2) + way % 4; // FIXME: Only valid for 4 ways
    uint64_t cl   = (way << 4) + set % 16; // FIXME: Only valid for 4 ways

    DEBUG_EMU(gprintf("\tDoing CacheOp with value %016llX\n", op_value);)

    switch (op) {
        case 3: // EvictSW
            if(dest == 0) break;     // If dest is L1, done
            if(dest <= start) break; // If start is bigger or equal than dest, done
            if(set >= 16) break;     // Skip sets outside cache
            if(way >= 4) break;      // Skip ways outside cache

            for(int i = 0; i < repeat; i++)
            {
                // If cacheline is locked or not passing tensor mask condition, skip operation
                if(!scp_locked[current_thread >> 1][cl] && (!tm || tmask_pass(i)))
                {
                    DEBUG_EMU(gprintf("\tDoing EvictSW (%d.%d) to Set: %X, Way: %X, CL: %02X, StartLevel: %01X, DestLevel: %01X\n", 
                              current_thread >> 1, current_thread & 1, set, way, cl, start, dest);)
                }

                set = (++cl) & 0xF;
                way = (cl >> 4) & 0x3;
            }
            break;
        case 2: // FlushSW
            if(dest == 0) break;     // If dest is L1, done
            if(dest <= start) break; // If start is bigger or equal than dest, done
            if(set >= 16) break;     // Skip sets outside cache
            if(way >= 4) break;      // Skip ways outside cache

            for(int i = 0; i < repeat; i++) {
                // If cacheline is locked or not passing tensor mask condition, skip operation
                if(!scp_locked[current_thread >> 1][cl] && (!tm || tmask_pass(i)))
                {
                    DEBUG_EMU(gprintf("\tDoing FlushSW (%d.%d) to Set: %X, Way: %X, CL: %02X, StartLevel: %01X, DestLevel: %01X\n", 
                              current_thread >> 1, current_thread & 1, set, way, cl, start, dest);)
                }

                set = (++cl) & 0xF;
                way = (cl >> 4) & 0x3;
            }
            break;
        case 7: // EvictVA
            if(dest == 0) break;     // If dest is L1, done
            if(dest <= start) break; // If start is bigger or equal than dest, done

            for(int i = 0; i < repeat; i++) {
                // If not masked
                if(!tm || tmask_pass(i))
                {
                    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);

                    // Looks if the address is locked (gets set and checks the 4 ways) and start level is L1
                    bool scp_en = false;
                    set = (paddr >> 6) & 0xF;
                    cl  = (set << 2);
                    for(int j = 0; j < 4; j++)
                    {
                        if((start == 0) && scp_locked[cl + j] && (scp_trans[current_thread >> 1][cl + j] == paddr)) scp_en = true;
                    }
                    // Address is not locked
                    if(!scp_en)
                        DEBUG_EMU(gprintf("\tDoing EvictVA: %016X, StartLevel: %01X, DestLevel: %01X\n", addr, start, dest);)
                        
                }
                addr += stride;
            }
            break;
        case 6: // FlushVA
            if(dest == 0) break;     // If dest is L1, done
            if(dest <= start) break; // If start is bigger or equal than dest, done

            for(int i = 0; i < repeat; i++) {
                // If not masked
                if(!tm || tmask_pass(i))
                {
                    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);

                    // Looks if the address is locked (gets set and checks the 4 ways) and start level is L1
                    bool scp_en = false;
                    set = (paddr >> 6) & 0xF;
                    cl  = (set << 2);
                    for(int j = 0; j < 4; j++)
                    {
                        if((start == 0) && scp_locked[cl + j] && (scp_trans[current_thread >> 1][cl + j] == paddr)) scp_en = true;
                    }
                    // Address is not locked
                    if(!scp_en)
                        DEBUG_EMU(gprintf("\tDoing FlushVA: %016X, StartLevel: %01X, DestLevel: %01X\n", addr, start, dest);)
                        
                }
                addr += stride;
            }
            break;
        case 4: // PrefetchVA
            for(int i = 0; i < repeat; i++)
            {
                // If not masked
                if(!tm || tmask_pass(i))
                {
                    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);

                    // Looks if the address is locked (gets set and checks the 4 ways) and start level is L1
                    bool scp_en = false;
                    set = (paddr >> 6) & 0xF;
                    cl  = (set << 2);
                    for(int w = 0; w < 4; w++)
                    {
                        if((start == 0) && scp_locked[cl + w] && (scp_trans[current_thread >> 1][cl + w] == paddr)) scp_en = true;
                    }
                    // Address is not locked
                    if(!scp_en)
                        DEBUG_EMU(gprintf("\tDoing PrefetchVA: %016X, Dest: %X\n", addr, dest);)
                }
                addr += stride;
            }
            break;
            // TODO
            break;
        case 0: // LockVA
            if((way >= 4) && (way != 255)) break; // Skip ways outside cache
            set = set % 16;

            for(int i = 0; i < repeat; i++)
            {
                // If not masked
                if(!tm || tmask_pass(i))
                {
                    DEBUG_EMU(gprintf("\tDoing LockVA: %016X, Way: %X\n", addr, way);)
                    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);

                    // Looks for 1st way available
                    if(way == 255)
                    {
                        cl = set << 2;
                        // For all the ways
                        for(int w = 0; w < 4; w++)
                        {
                            // Check if not locked and same PADDR
                            if(!scp_locked[current_thread >> 1][cl + w])
                            {
                                way = w;
                                break;
                            }
                        }
                        // No free way found, return 1
                        if(way == 255) return 1;
                    }
                    else
                    {
                        // For all the ways
                        for(int w = 0; w <= 4; w++)
                        {
                            // Check if not locked and same PADDR
                            if(!scp_locked[current_thread >> 1][cl + w])
                            {
                                break;
                            }
                            // No free way found, return 1
                            if(w == 4) return 1;
                        }
                    }

                    cl = (set << 2) + way;
                    scp_locked[current_thread >> 1][cl] = true;
                    scp_trans[current_thread >> 1][cl]  = paddr;
                }
                addr += stride;
            }
            break;
        case 1: // UnlockVA
            for(int i = 0; i < repeat; i++)
            {
                // If not masked
                if(!tm || tmask_pass(i))
                {
                    uint64_t paddr = virt_to_phys_emu(addr, Mem_Access_Store);
                    uint64_t state  = (op_value >> 59) & 0x1;

                    set = (paddr >> 6) & 0xF;
                    cl = set << 2;

                    // For all the ways
                    for(int w = 0; w < 4; w++)
                    {
                        // Check if locked and same PADDR
                        if(scp_locked[current_thread >> 1][cl + w] && (scp_trans[current_thread >> 1][cl + w] == paddr))
                        {
                            DEBUG_EMU(gprintf("\tDoing UnlockVA: %016X, Way: %X, FinalState: %01X\n", addr, w, state);)
                            scp_locked[current_thread >> 1][cl + w] = false;
                        }
                    }
                }
                addr += stride;
            }
            break;
        default:
           DEBUG_EMU(gprintf("\tUnknown CacheOp Opcode!\n");)
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto messaging extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// setup functions to retrieve data written to port and to query if there is
// data available from RTL

typedef uint32_t (*func_ret_msg_port_data_t)  (uint32_t, uint32_t, uint32_t);
typedef bool (*func_query_msg_port_data_t)  (uint32_t, uint32_t);
typedef void (*func_msg_port_data_request_t) (uint32_t, uint32_t);

static func_ret_msg_port_data_t     retrieve_msg_port_data = NULL;
static func_query_msg_port_data_t   query_msg_port_data    = NULL;
static func_msg_port_data_request_t newMsgPortDataRequest  = NULL;

// get data from RTL and write into scratchpad
static uint64_t get_msg_port_offset(uint32_t id)
{
    uint32_t offset = msg_ports[current_thread][id].rd_ptr << msg_ports[current_thread][id].logsize;
    msg_ports[current_thread][id].rd_ptr++;
    msg_ports[current_thread][id].rd_ptr %= (msg_ports[current_thread][id].max_msgs + 1);
    msg_ports_pending_offset[current_thread][id] = offset;
    if (in_sysemu == 1)
    {
        if (newMsgPortDataRequest == NULL)
        {
            gprintf("id = %d, offset = %d, current_thread = %d\n", id, offset, current_thread);
            gprintf("ERROR: newMsgPortDataRequest == NULL");
            exit(-1);
        }
        newMsgPortDataRequest(current_thread, id);
    }
    return offset;
}

static void write_msg_port_data(uint32_t thread, uint32_t id)
{
    if ( retrieve_msg_port_data != NULL)
    {
        int wr_words = (1<<msg_ports[thread][id].logsize) >> 2;
        uint32_t *data = new uint32_t [wr_words];
        for(int i = 0; i < wr_words; i++)
            data[i] =  retrieve_msg_port_data ( thread, id, i );
        write_msg_port_data_(thread, id, data);
        delete [] data;
    }
    else {
        gprintf("ERROR: no data provider for msg port %d emulation has been configured\n", id);
        exit(-1);
    }
}

static uint64_t msg_port_csr(uint32_t id, uint64_t wdata, bool umode)
{
    // FIXME: Raise "illegal instruction" exception if:
    //   * we are in U-mode and port[id].umode is 0.
    //   * we do MSG_PGET(NB) and port[id].enabled is 0.
    msg_port_conf_action action =  (msg_port_conf_action) (wdata & 0xF);
    switch (action)
    {
        case MSG_ENABLE:
            msg_ports[current_thread][id].enabled = true;
            msg_ports[current_thread][id].stall = false;
            msg_ports[current_thread][id].umode = (((wdata >> 4) & 0x1) != 0);
            msg_ports[current_thread][id].logsize = (wdata >> 5)  & 0x7;
            msg_ports[current_thread][id].max_msgs = (wdata >> 8) & 0xF;
            msg_ports[current_thread][id].use_scp = (((wdata >> 15) & 0x1) != 0);
            msg_ports[current_thread][id].scp_set = (wdata >> 16) & 0xF;
            msg_ports[current_thread][id].scp_way = (wdata >> 24) & 0x3;
            msg_ports[current_thread][id].enable_oop = (((wdata >> 32) & 0x1) != 0);
            msg_ports[current_thread][id].rd_ptr = 0;
            msg_ports[current_thread][id].wr_ptr = 0;
            return 0;
        case MSG_DISABLE:
            msg_ports[current_thread][id].enabled = false;
            return 0;
        case MSG_PGET:
            // if in sysemu stop thread if no data for port.. comparing rd_ptr and wr_ptr
            if (in_sysemu == 1 && query_msg_port_data(current_thread, id) == 0)
            {
                msg_ports[current_thread][id].stall = true;
                return 0;
            }
            return get_msg_port_offset(id);
        case MSG_PGETNB:
            if (query_msg_port_data(current_thread, id))
                return get_msg_port_offset(id);
            return -1;
        default:
            DEBUG_EMU(gprintf("ERROR Unimplemented port msg conf mode %d!!\n", (int) action);)
            return 0;
    }
}

void set_msg_port_data_func( void* f, void *g, void *h)
{
    retrieve_msg_port_data = (func_ret_msg_port_data_t) f;
    query_msg_port_data = (func_query_msg_port_data_t) g;
    newMsgPortDataRequest = (func_msg_port_data_request_t) h;
}

bool get_msg_port_stall(uint32_t thread, uint32_t id)
{
    return msg_ports[thread][id].stall;
}

void update_msg_port_data()
{
    for ( uint32_t port_id = 0 ; port_id < NR_MSG_PORTS; port_id ++)
    {
        if (msg_ports_pending_offset[current_thread][port_id] >= 0 )
            write_msg_port_data(current_thread, port_id);
    }
}

void write_msg_port_data_(uint32_t thread, uint32_t id, uint32_t *data)
{
    // write to scratchpad
    uint64_t base_addr = scp_trans[thread >> 1][(msg_ports[thread][id].scp_set << 2) | msg_ports[thread][id].scp_way];
    base_addr += msg_ports_pending_offset[thread][id];
    msg_ports_pending_offset[thread][id] = -1;
    int wr_words = (1<<msg_ports[thread][id].logsize) >> 2;
    for(int i = 0; i < wr_words; i++)
    {
        uint32_t ret = data[i];
        DEBUG_EMU(gprintf("Writing MSG_PORT (m%d p%d) data %08X to addr %016llX\n", thread, id, ret, base_addr + 4 * i););
        memwrite32 ( base_addr + 4 * i, ret );
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Esperanto tensor extension emulation
//
////////////////////////////////////////////////////////////////////////////////

// ----- TensorConvolution emulation -------------------------------------------

// Moves one step the position of the convolution sampling based on the configuration register
static void conv_move_pointer(int64_t * conv_row_pos, int64_t * conv_col_pos, uint64_t conv_row_step_offset, uint64_t conv_col_step_offset)
{
    * conv_row_pos = (* conv_row_pos) + conv_row_step_offset;
    * conv_col_pos = (* conv_col_pos) + conv_col_step_offset;
}

// Returns if there something that needs to be processed or not based on current position and configuration
static bool conv_skip_pass(int64_t conv_row_pos, int64_t conv_col_pos, uint64_t conv_row_size, uint64_t conv_col_size)
{
    DEBUG_EMU(printf("Doing Conv skip pass check for:\n");)
    DEBUG_EMU(printf("\tRow Pos:  %016" PRIx64 "\n", conv_row_pos);)
    DEBUG_EMU(printf("\tCol Pos:  %016" PRIx64 "\n", conv_col_pos);)
    DEBUG_EMU(printf("\tRow Size: %016" PRIx64 "\n", conv_row_size);)
    DEBUG_EMU(printf("\tCol Size: %016" PRIx64 "\n", conv_col_size);)
    // Negative position
    bool skip = 0;
    if(conv_col_pos < 0) skip = 1;
    if(conv_row_pos < 0) skip = 1;
    // Outside position
    if(conv_col_pos >= int64_t(conv_col_size)) skip = 1;
    if(conv_row_pos >= int64_t(conv_row_size)) skip = 1;

    if(skip)

    {
        DEBUG_EMU(gprintf("\tSkip conv_row_pos %d conv_col_pos %d conv_row_size%d conv_col_size%d\n", conv_row_pos, conv_col_pos, conv_row_size, conv_col_size);)

    }
    return skip;
}

// Update to the tensor Mask due a convolution CSR write
static void tmask_conv()
{
    uint64_t tmask_value = 0;

    // Gets the sizes of the convolution
    uint64_t tconvsizereg         = csrget(csr_tconvsize);
    uint64_t conv_row_step_offset = (tconvsizereg & 0xFF00000000000000ULL) >> 56;
    uint64_t conv_row_size        = (tconvsizereg & 0x0000FFFF00000000ULL) >> 32; // Convolution size in rows
    uint64_t conv_col_step_offset = (tconvsizereg & 0x00000000FF000000ULL) >> 24;
    uint64_t conv_col_size        = (tconvsizereg & 0x000000000000FFFFULL);       // Convolution size in cols

    // Gets the positions of the convolution
    uint64_t tconvctrlreg = csrget(csr_tconvctrl);
    int64_t  conv_row_pos = (tconvctrlreg & 0x0000FFFF00000000ULL) >> 32; // Convolution pos in rows
    int64_t  conv_col_pos = (tconvctrlreg & 0x000000000000FFFFULL);       // Convolution pos in cols

    // Sign extend
    if(conv_row_pos & 0x8000) conv_row_pos = conv_row_pos | 0xFFFFFFFFFFFF0000ULL;
    if(conv_col_pos & 0x8000) conv_col_pos = conv_col_pos | 0xFFFFFFFFFFFF0000ULL;

    // Goes through the 16 elements of the tensormap
    for(int i = 0; i < 16; i++)
    {
        // Sets a 1 if convolution passes
        if(!conv_skip_pass(conv_row_pos, conv_col_pos, conv_row_size, conv_col_size))
            tmask_value |= 1 << i;
        conv_move_pointer(&conv_row_pos, &conv_col_pos, conv_row_step_offset, conv_col_step_offset);
    }
    
    csrset(csr_tmask, tmask_value);
}
static void tcoop()
{
    uint64_t tcoopreg         = csrget(csr_tcoop);
    
    uint64_t warl                 = (tcoopreg >> 24) & 0xFFFFFFFFFF;
    uint64_t timeout              = (tcoopreg >> 16) & 0x1FF;
    uint64_t coop_mask            = (tcoopreg >> 8) & 0xFF;
    uint64_t coop_id              = (tcoopreg >> 0) & 0xFF;
    //TODO implement functionality checking the addresses and tcoop of every use of Tensor Load 
    DEBUG_EMU(gprintf("\tSetting Tensor Cooperation:  Warl [%040X] . Timeout %d . Coop Mask %08X . Coop ID : %d\n",warl, timeout , coop_mask ,coop_id  );)
    
}

// ----- TensorLoad emulation --------------------------------------------------

#if 0
static void tensorload(uint64_t control)
{
    //uint64_t dst     = control & 0x3F;
    //int      rows    = ((control >> 48) & 0x1F) + 1;
    //uint64_t tm      = (control >> 53) & 0x1;
    //uint64_t base    = control & 0xFFFFFFFFFFC0ULL;


    uint64_t stride  = XREGS[31].x;

    uint64_t tm                 = (control >> 63) & 0x1;
    uint64_t use_coop           = (control >> 62) & 0x1;
    //uint64_t reserved           = (control >> 59) & 0x7;
    uint64_t dst                = (control >> 53) & 0x3F;
    uint64_t read_sc            = (control >> 52) & 0x1;
   // uint64_t write_any          = (control >> 48) & 0xF;
    //uint64_t virtual_addr_l2_sc = (control >>  6) & 0x3FFFFFFFFFF;
    uint64_t base                 = control & 0xFFFFFFFFFFC0ULL;
    //uint64_t boffset            = (control >>  4) & 0x03;
    int rows                    = ((control      ) & 0xF) + 1;
    
    scp_entry[current_thread] = dst;
    scp_size[current_thread]  = rows;
    scp_tm                    = tm;
    uint64_t addr             = base;
    DEBUG_EMU(gprintf("Tensor Load confs : tmask %d , use_coop %d,  dst %d, read_sc %d, base 0x%016X, rows %d \n", tm ,use_coop, dst, read_sc, base, rows);)

    //TODO at this moment a behavior equal to the old Tensor Load is assumed, the new features will be implemented.
    if( read_sc )
    {
        DEBUG_EMU(gprintf("ERROR Read from SC not currently supported in EMU !!\n");)
    }    

    for ( int i = 0; i < rows; i++ )
    {
        bool skip_load = 0;
        // Checks if needs to skip the current load due tensormask CSR
        if(tm)
            skip_load = !tmask_pass(i);

        if(!skip_load)
        {
            if(addr & 0x3F)
            {
                DEBUG_EMU(gprintf("ERROR Tensor Load not aligned to cache line!!\n");)
            }
            for ( int j = 0; j < 4; j++ )
            {
                for ( int k = 0; k < 4; k++ )
                {
                    uint64_t addr_final = addr+j*16+k*4;
                    uint32_t val32 = memread32(addr_final);
                    float32_t fval32 = cast_uint32_to_float32(val32);

                    SCP[dst + i][j].f[k] = fval32;
                    DEBUG_EMU(gprintf("\tScratchpad tensor load MEM[%016X]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)\n", addr_final, dst+i,j,k,SCP[dst+i][j].u[k],SCP[dst+i][j].u[k]);)
                }
            }
        }
        else
        {
            DEBUG_EMU(gprintf("Tensor Load skipped !!\n");)
        }
        addr += stride;
    }
}
#endif
void tensorload(uint64_t control)//Transtensorload
{
    uint64_t stride  = XREGS[31].x;

    uint64_t tm                 = (control >> 63) & 0x1;
    uint64_t use_coop           = (control >> 62) & 0x1;
    uint64_t trans              = (control >> 59) & 0x7;
    uint64_t dst                = (control >> 53) & 0x3F;
    uint64_t read_sc            = (control >> 52) & 0x1;
    //uint64_t virtual_addr_l2_sc = (control >>  6) & 0x3FFFFFFFFFF;
    uint64_t base                 = control & 0xFFFFFFFFFFC0ULL;
    uint64_t boffset            = (control >>  4) & 0x03;
    int rows                    = ((control      ) & 0xF) + 1;

    scp_entry[current_thread] = dst;
    scp_size[current_thread]  = rows;
    uint64_t addr             = base;
    scp_tm                    = tm;

    DEBUG_EMU(gprintf("Tensor Load: Trans:%d - rows:%d - tm:%d - use_coop:%d - dst:%d - read_sc:%d - boffset:%d - addr:0x%16X\n", trans, rows, tm, use_coop, dst, read_sc, boffset, addr);)

    if (read_sc) {
       DEBUG_EMU(gprintf("ERROR Read from SC not currently supported in EMU !!\n");)
    }    

    //NO TRANS
    if (trans == 0x00) {
        DEBUG_EMU(gprintf("TensorLoad: No transformation\n");)
        for (int i = 0; i < rows; ++i) {
            if (!tm || tmask_pass(i)) {
                if (addr & 0x3F)
                {
                    DEBUG_EMU(gprintf("ERROR Tensor Load not aligned to cache line!!\n");)
                }
                for ( int j = 0; j < 4; j++ )
                {
                    for ( int k = 0; k < 4; k++ )
                    {
                        uint64_t addr_final = addr+j*16+k*4;
                        uint32_t val32 = memread32(addr_final);
                        float32_t fval32 = cast_uint32_to_float32(val32);

                        SCP[dst + i][j].f[k] = fval32;
                        DEBUG_EMU(gprintf("\tScratchpad tensor load MEM[%016X]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)\n", addr_final, dst+i,j,k,SCP[dst+i][j].u[k],SCP[dst+i][j].u[k]);)
                    }
                }    
            }
            DEBUG_EMU(gprintf("\t\tAddress = 0x%016x - Stride = 0x%016x\n",addr,stride);)
            addr += stride;
        }
    }
    //INTERLEAVE
    else if (trans == 0x01 || trans == 0x02) {
       
       DEBUG_EMU(gprintf("TensorLoad: Interleave\n");)
       uint8_t tmp_buffer[4][64];
       int size = trans & 0x03;
       int start;
       start=size==1 ?  boffset << 4 : (boffset & 0x02) << 5;
       int elements = 4 / size;
       
       DEBUG_EMU(gprintf("#rows:%d - size:%d - start:%d - elements:%d - boffset:%d\n",rows,size,start,elements,boffset);)
       for (int i = 0; i < rows; ++i) {
            if (!tm || tmask_pass(i)) {
                if (addr & 0x3F)
                {
                    DEBUG_EMU(gprintf("ERROR Tensor Load not aligned to cache line!!\n");)
                }
                for( int elem = 0; elem < elements; ++elem){
                    //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
                    for ( int j = 0; j < 8; j++ )
                    {
                        for ( int k = 0; k < 8; k++ )
                        {
                            uint64_t addr_final = addr+j*8+k;
                            uint8_t val = memread8(addr_final);
                            tmp_buffer[elem][j*8+k] = val;
                            DEBUG_EMU(gprintf("\tLoading into tmp_buffer - MEM[%016X]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)\n", addr_final, elem,j,k,tmp_buffer[elem][j*8+k],tmp_buffer[elem][j*8+k]);)
                        }
                    }
                    
                    DEBUG_EMU(gprintf("\t\tAddres = 0x%016x - Stride = 0x%016x\n",addr,stride);)
                    addr += stride;
                }
                for (int line = 0; line < 4; ++ line) {
                    for (int byte = 0; byte < 16; byte+=4) { // We interleve 32 bits each pass
                        if (elements == 2){
                            SCP[dst+i][line].b[byte] = tmp_buffer[0][start+line*8+byte/elements];
                            SCP[dst+i][line].b[byte+1] = tmp_buffer[0][start+line*8+byte/elements+1];
                            SCP[dst+i][line].b[byte+2] = tmp_buffer[1][start+line*8+byte/elements];
                            SCP[dst+i][line].b[byte+3] = tmp_buffer[1][start+line*8+byte/elements+1];
                        }
                        if(elements == 4){
                            SCP[dst+i][line].b[byte] = tmp_buffer[0][start+line*4+byte/elements];
                            SCP[dst+i][line].b[byte+1] = tmp_buffer[1][start+line*4+byte/elements];
                            SCP[dst+i][line].b[byte+2] = tmp_buffer[2][start+line*4+byte/elements];
                            SCP[dst+i][line].b[byte+3] = tmp_buffer[3][start+line*4+byte/elements];
                        }
                        
                        DEBUG_EMU(gprintf("SCP[%d][%d].u[%d] = 0x%08x\n",dst+i,line,byte/4,SCP[dst+i][line].u[byte/4]);)
                    }
                    
                }     
            }
        }
       //printSCP(addr,rows,stride,dst); 
    }
    //TRANSPOSE
    else if(trans == 0x05 || trans == 0x06 || trans==0x07){
        
        bool exist_conv = 0;
        for(int i=0; (i<rows) & (!exist_conv);++i)
            exist_conv = tmask_pass(i); 
        if(tm && !exist_conv){
            DEBUG_EMU(gprintf("Exit Condition Broken\n");)
             return;
        }
        int offset = (control >> 57) & 0x1F;
        uint8_t tmp_buffer[64][64];
        int size = (trans & 0x03);
        
        offset = (size==1) ?  (control & 0x30) : (control & 0x20) ;
        int elements = 64 >> (size-1);
        size = 1 << (size-1);
        DEBUG_EMU(gprintf("TensorLoad: Transpose - elements:%d size:%d offset:%d\n",elements,size,offset);)
        for( int elem = 0; elem < elements; ++elem){
            //Reading 512 bits ( 64 bytes - 16 passes reading 32 bits)
            for ( int j = 0; j < 8; j++ )
            {
                for ( int k = 0; k < 8; k++ )
                {
                    uint64_t addr_final = addr+j*8+k;
                    uint8_t val = memread8(addr_final);
                    tmp_buffer[elem][j*8+k]=val;
                    DEBUG_EMU(gprintf("\tLoading into tmp_buffer - MEM[%016X]: Row%d-Freg%d-Elem%d <= 0x%08x (%d)\n", addr_final, elem,j,k,tmp_buffer[elem][j*8+k],tmp_buffer[elem][j*8+k]);)
                }
            }
            addr += stride;
        }
        for (int  i =0 ;i < rows; ++i)
        {
             if(!tm || tmask_pass(i)){
                if(addr & 0x3F)
                {
                    DEBUG_EMU(gprintf("ERROR Tensor Load not aligned to cache line!!\n");)
                }
                for (int j = 0; j < elements; ++j) {
                    if (size == 4){
                        SCP[dst+i][j/4].b[(j*size)%16] = tmp_buffer[j][(i)*size+offset];
                        SCP[dst+i][j/4].b[(j*size+1)%16] = tmp_buffer[j][(i)*size+offset+1];
                        SCP[dst+i][j/4].b[(j*size+2)%16] = tmp_buffer[j][(i)*size+offset+2];
                        SCP[dst+i][j/4].b[(j*size+3)%16] = tmp_buffer[j][(i)*size+offset+3];
                        DEBUG_EMU(gprintf("\tI'm size 4 - b[0]=0x%02x b[1]=0x%02x\n",tmp_buffer[j][(i)*size+offset],tmp_buffer[j][(i)*size+offset+1]);)
                    }                        
                    else if (size == 2) {
                        SCP[dst+i][j/8].b[(j*size)%16] = tmp_buffer[j][(i)*size+offset];
                        SCP[dst+i][j/8].b[(j*size+1)%16] = tmp_buffer[j][(i)*size+offset+1];
                        DEBUG_EMU(gprintf("\tI'm size 2 - b[0]=0x%02x b[1]=0x%02x\n",tmp_buffer[j][(i)*size+offset],tmp_buffer[j][(i)*size+offset+1]);)
                    }
                    else if (size == 1) {
                        SCP[dst+i][j/16].b[(j*size)%16] = tmp_buffer[j][(i)*size+offset];
                        DEBUG_EMU(gprintf("\tI'm size 1 - b[0]=0x%02x b[1]=0x%02x\n",tmp_buffer[j][dst+(i)*size+offset],tmp_buffer[j][dst+(i)*size+offset+1]);)
                    }
                    else {
                        DEBUG_EMU(gprintf("ERROR Tensor Load element size not valid!!\n");)
                    }
                    
                }
                for (int x = 0; x < 4; ++x) {
                    for (int y = 0; y < 4; ++y) {
                         DEBUG_EMU(gprintf("SCP[%d][%d].u[%d] = 0x%08x\n",dst+i,x,y,SCP[dst+i][x].u[y]);)
                    }
                }
            }
            
        }
    }
}


// ----- TensorStore emulation -------------------------------------------------
static void tensorstore(uint64_t tstorereg)
{
    uint64_t srcinc   = ((tstorereg & 0xC00000000000000C) >> 62) + 1; // Increment done to register source
    uint64_t regstart =  (tstorereg & 0x3E00000000000000) >> 57;      // Start register to store
    uint64_t cols     = ((tstorereg & 0x0180000000000000) >> 55) + 1; // Number of register per col
    uint64_t rows     = ((tstorereg & 0x0070000000000000) >> 52) + 1; // Number of rows to store
    uint64_t addr     =  (tstorereg & 0x0000FFFFFFFFFFF0);            // Address where to store the results

    uint64_t stride   = XREGS[31].x & 0xFFFFFFFFFFF0UL;

    DEBUG_EMU(gprintf("\tStart Tensor Store with addr: %016llx, stride: %016llx, regstart: %d, rows: %d, cols: %d, srcinc: %d\n", addr, stride, regstart, rows, cols, srcinc);)

    uint64_t src = regstart;

    // For all the rows
    for(uint64_t row = 0; row < rows; row++)
    {
        // For all the cols
        for(uint64_t col = 0; col < cols; col++)
        {
            // For all the elements of the lane
            for(uint64_t i = 0; i < 4; i++)
            {
                uint32_t val = FREGS[src].u[i];
                memwrite32(addr + col * 16 + i * 4, val);
                DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",val,addr + col * 16 + i * 4);)
                //logmemwchange(0, 4, addr + col * 16 + i * 4, val); => Don't log mem changes!
            }
            src += srcinc;
        }
        addr += stride;
    }
}
#if 0
void tensorstore(uint64_t tstorereg)
{

    uint64_t regstart =  (tstorereg & 0x30000000000000) >> 52;      // Start register to store
    uint64_t rows     = ((tstorereg & 0x0F000000000000) >> 48) + 1; // Number of rows to store
    uint64_t addr     =  (tstorereg & 0x00FFFFFFFFFFC0);            // Address where to store the results
    uint64_t srcinc   = ((tstorereg & 0x0000000000000C) >>  2) + 1; // Increment done to register source
    uint64_t cols     =  (tstorereg & 0x00000000000003) + 1;        // Number of register per col

    uint64_t stride   = XREGS[31].x & 0xFFFFFFFFFFFFUL;

    uint64_t src = regstart * 16 % 48;
    DEBUG_EMU(gprintf("\tStart Tensor Store with addr: %016llx, stride: %016llx, regstart: %d, rows: %d, cols: %d, srcinc: %d\n", addr, stride, src, rows, cols, srcinc);)
    // For all the rows
    for(uint64_t row = 0; row < rows; row++)
    {
        // For all the cols
        for(uint64_t col = 0; col < cols; col++)
        {
            // For all the elements of the lane
            for(uint64_t j = 0; j < 4; j++)
            {
                for(uint64_t i = 0; i < 4; i++)
                {
                    uint32_t val = SCP[src][j].u[i];
                    memwrite32(addr + col * 64 + j * 16 + i * 4, val);
                    DEBUG_EMU(gprintf("\t0x%08x --> MEM[0x%016llx]\n",val,addr + col * 16 + i * 4);)
                    DEBUG_EMU(gprintf("\t\tSCP[%d][%d].u[%d]\n",src,j,i);)
                    //logmemwchange(0, 4, addr + col * 16 + i * 4, val); => Don't log mem changes!
                }
            }
            src += srcinc;
            src = src % 48;
        }
        addr += stride;
    }
}
#endif


// ----- TensorFMA emulation ---------------------------------------------------

static void tensorfma(uint64_t tfmareg)
{
    int tm         = (tfmareg & 0x8000000000000000) >> 63; // Is a Conv2D operation (use tensor conv register)
    int bcols      = (tfmareg & 0x0180000000000000) >> 55; // Number of B cols to be processed
    int arows      = (tfmareg & 0x0070000000000000) >> 52; // Number of A rows to be processed
    int acols      = (tfmareg & 0x000F000000000000) >> 48; // Number of A cols to be processed
    int aoffset    = (tfmareg & 0x0000F00000000000) >> 44; // A matrix 32b offset
    int bstart     = (tfmareg & 0x00000000000FF000) >> 12; // SCP entry where B is stored
    int astart     = (tfmareg & 0x0000000000000FF0) >>  4; // SCP entry where A is stored
    int type       = (tfmareg & 0x000000000000000E) >>  1; // Mode: 00 => FP32 | 01 => *FP16+FP32 | 10 => FP16 | 11 => *INT8+INT32
    int first_pass = (tfmareg & 0x0000000000000001);       // Doing a first pass op (do MUL instead of FMA)

    // Decodes fields
    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

    DEBUG_EMU(gprintf("\tStart Tensor FMA with tm: %d, aoffset: %d, Type: %d, First pass: %d, bcols: %d, acols: %d, arows: %d, bstart: %d, astart: %d\n", tm, aoffset, type, first_pass, bcols, acols, arows, bstart, astart);)

    tensorfma_size[current_thread] = arows * bcols / 4;
    tensorfma_passes[current_thread] = acols;

    // No mask skip by default
    for(int i = 0; i < 16; i++)
    {
        for(int j = 0; j < 8; j++)
        {
            tensorfma_mask_skip[i][j] = 0;
        }
    }

    // No zero skip by default
    for(int i = 0; i < 16; i++)
    {
        for(int j = 0; j < 32; j++)
        {
            for(int k = 0; k < 4; k++)
            {
                tensorfma_zero_skip[i][j][k] = 0;
            }
        }
    }

    // FP32 flow
    if(type == 0)
    {
        if(first_pass)
        {
            for ( int ar = 0; ar < arows; ar++ )
            {
                for ( int bc = 0; bc < bcols; bc++ )
                {
                    int bf = bc / 4;
                    int bm = bc % 4;

                    if(MREGS[0].b[bm*2] == 0) continue;

                    FREGS[4*ar+bf].f[bm] = 0.0f;
                    tensorfma_data[current_thread][4*ar+bf][bm][0] = 0;
                }
            }
        }

        for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if(tm)
            {
                if(!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for(int i = 0; i < 16; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if(first_pass)
                    {
                        tensorfma_mask_skip[0][ar] = 0;
                    }
                    continue;
                }
            }

            for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
            {
                int bf = bc / 4;
                int bm = bc % 4;
                if(MREGS[0].b[bm*2] == 0) continue;

                for ( int ac = 0; ac < acols; ac++ )     // A: traverse acols cols
                {
                    int af = (aoffset + ac) / 4;
                    int am = (aoffset + ac) % 4;
                    int br = bstart + ac;                // B: traverse acols rows
                    float32_t old = FREGS[4*ar+bf].f[bm];
                    uint32_t oldu = FREGS[4*ar+bf].u[bm];
                    FREGS[4*ar+bf].f[bm] = fmaf(SCP[astart+ar][af].f[am],SCP[br][bf].f[bm],old);
                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %f = %f + %f * %f\n",4*ar+bf,bm,FREGS[4*ar+bf].f[bm],old,SCP[astart+ar][af].f[am],SCP[br][bf].f[bm]);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%08x * 0x%08x\n",oldu,SCP[astart+ar][af].u[am],SCP[br][bf].u[bm]);)
                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][4*ar+bf][bm][ac] = FREGS[4*ar+bf].u[bm];

                    if ((first_pass == 0) || (ac != 0)) {                    
                    // If As are zeroes, we skip operation
                      if(SCP[astart+ar][af].f[am] == 0)
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    // If Bs are zeroes, we skip operation
                      if(SCP[br][bf].f[bm] == 0)
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    }
                }
            }
            DEBUG_EMU(gprintf("\tC row %d: f%d[%d] = 0x%08x (%f)\n",ar,4*ar  ,0,FREGS[4*ar+0].u[0],FREGS[4*ar+0].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,1,FREGS[4*ar+0].u[1],FREGS[4*ar+0].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,2,FREGS[4*ar+0].u[2],FREGS[4*ar+0].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,3,FREGS[4*ar+0].u[3],FREGS[4*ar+0].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,0,FREGS[4*ar+1].u[0],FREGS[4*ar+1].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,1,FREGS[4*ar+1].u[1],FREGS[4*ar+1].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,2,FREGS[4*ar+1].u[2],FREGS[4*ar+1].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,3,FREGS[4*ar+1].u[3],FREGS[4*ar+1].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,0,FREGS[4*ar+2].u[0],FREGS[4*ar+2].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,1,FREGS[4*ar+2].u[1],FREGS[4*ar+2].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,2,FREGS[4*ar+2].u[2],FREGS[4*ar+2].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,3,FREGS[4*ar+2].u[3],FREGS[4*ar+2].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,0,FREGS[4*ar+3].u[0],FREGS[4*ar+3].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,1,FREGS[4*ar+3].u[1],FREGS[4*ar+3].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,2,FREGS[4*ar+3].u[2],FREGS[4*ar+3].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,3,FREGS[4*ar+3].u[3],FREGS[4*ar+3].f[3]);)
        }
    }
    // *FP16+FP32
    else if(type == 1)
    {
        if(first_pass)
        {
            for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
            {
                for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
                {
                    int bf = bc / 4;
                    int bm = bc % 4;
                    if(MREGS[0].b[bm*2] == 0) continue;

                    FREGS[4*ar+bf].f[bm] = 0.0f;
                    tensorfma_data[current_thread][4*ar+bf][bm][0] = 0;
                }
            }
        }

        for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if(tm)
            {
                if(!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for(int i = 0; i < 16; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if(first_pass)
                        tensorfma_mask_skip[0][ar] = 0;
                    continue;
                }
            }

            for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
            {
                int bf = bc / 4;
                int bm = bc % 4;
                if(MREGS[0].b[bm*2] == 0) continue;

                for ( int ac = 0; ac < acols; ac++ )     // A: accumulate acols values
                {
                    int af = (aoffset + ac) / 4;
                    int am = (aoffset + ac) % 4;
                    int br = bstart + ac;                // B: traverse rows
                

                    // Doing two FMAs per lane and accumulating to previous results
                    float32_t accum      = FREGS[4*ar+bf].f[bm];

#ifdef USE_REAL_TXFMA
                    // 1st FMA
                    uint32_t  mul1_a_hex = SCP[astart+ar][af].h[am * 2];                            // get first operand
                    bool      mul1_a_den = ((mul1_a_hex & 0x7C00) == 0);                            // detect input denormal or zero
                    float32_t mul1_a     = mul1_a_den ? 0 : _cvtsh_ss(mul1_a_hex);                  // convert to fp32
                    int32_t   mul1_a_exp = ((mul1_a_hex >> 10) & 0x1F) - 15;                        // get exponent

                    uint32_t  mul1_b_hex = SCP[br][bf].h[bm * 2];                                   // get second operand
                    bool      mul1_b_den = ((mul1_b_hex & 0x7C00) == 0);                            // detect input denormal or zero
                    float32_t mul1_b     = mul1_b_den ? 0 : _cvtsh_ss(mul1_b_hex);                  // convert to fp32
                    int32_t   mul1_b_exp = ((mul1_b_hex >> 10) & 0x1F) - 15;                        // get exponent

                    float32_t fma1       = mul1_a * mul1_b;                                         // perform first mul

                    // 2nd FMA
                    uint32_t  mul2_a_hex = SCP[astart+ar][af].h[am * 2 + 1];                        // get third operand
                    bool      mul2_a_den = ((mul2_a_hex & 0x7C00) == 0);                            // detect input denormal or zero
                    float32_t mul2_a     = mul2_a_den ? 0 : _cvtsh_ss(mul2_a_hex);                  // convert to fp32
                    int32_t   mul2_a_exp = ((mul2_a_hex >> 10) & 0x1F) - 15;                        // get exponent

                    uint32_t  mul2_b_hex = SCP[br][bf].h[bm * 2 + 1];                               // get fourth operand
                    bool      mul2_b_den = ((mul2_b_hex & 0x7C00) == 0);                            // detect input denormal or zero
                    float32_t mul2_b     = mul2_b_den ? 0 : _cvtsh_ss(mul2_b_hex);                  // convert to fp32
                    int32_t   mul2_b_exp = ((mul2_b_hex >> 10) & 0x1F) - 15;                        // get exponent

                    float32_t fma2       = mul2_a * mul2_b;                                         // perform second mul

                    // Get hex value and exponents of three operands of final addition
                    uint32_t hex_accum  = * (uint32_t * ) &accum;
                    int32_t  accum_exp  = ((hex_accum >> 23) & 0xFF) - 127;
                    uint32_t hex_fma1   = * (uint32_t * ) &fma1;
                    int32_t  fma1_exp   = ((hex_fma1 >> 23) & 0xFF) - 127;
                    int32_t  fma1_exp_r = (mul1_a_den || mul1_b_den) ? -127 : mul1_a_exp + mul1_b_exp; // use exponent without shifting to match rtl
                    uint32_t hex_fma2   = * (uint32_t * ) &fma2;
                    int32_t  fma2_exp   = ((hex_fma2 >> 23) & 0xFF) - 127;
                    int32_t  fma2_exp_r = (mul2_a_den || mul2_b_den) ? -127 : mul2_a_exp + mul2_b_exp; // use exponent without shifting to match rtl

                    // Get max exponent that determines where we truncate other values
                    int32_t exp_max = ((accum_exp >= fma1_exp_r) && (accum_exp  >= fma2_exp_r)) ? accum_exp  :
                                      ((fma1_exp_r >= accum_exp) && (fma1_exp_r >= fma2_exp_r)) ? fma1_exp_r :
                                                                                                  fma2_exp_r;

                    // Truncate all values to (set truncate accordingly):
                    //    - 0: b23 (no rouding)
                    //    - 1: round bit
                    //    - 2: guard bit
                    int32_t  truncate = 1;
                    int32_t  accum_erase = exp_max - accum_exp - truncate;
                    uint32_t accum_trunc = (accum_erase > 23) ? (hex_accum &  0x80000000) :
                                           (accum_erase < 1 ) ? (hex_accum              ) :
                                                                (hex_accum & ((0xFFFFFFFF >> accum_erase) << accum_erase));
                    int32_t   fma1_erase = exp_max -  fma1_exp - truncate;
                    uint32_t  fma1_trunc = ( fma1_erase > 23) ? (hex_fma1  &  0x80000000) :
                                           ( fma1_erase < 1 ) ? (hex_fma1               ) :
                                                                (hex_fma1  & ((0xFFFFFFFF >>  fma1_erase) <<  fma1_erase));
                    int32_t   fma2_erase = exp_max -  fma2_exp - truncate;
                    uint32_t  fma2_trunc = ( fma2_erase > 23) ? (hex_fma2  &  0x80000000) :
                                           ( fma2_erase < 1 ) ? (hex_fma2               ) :
                                                                (hex_fma2  & ((0xFFFFFFFF >>  fma2_erase) <<  fma2_erase));

                    // Convert back to fp32 after truncation
                    float32_t accum_fp32 = * (float32_t *) &accum_trunc;
                    float32_t  fma1_fp32 = * (float32_t *) &fma1_trunc;
                    float32_t  fma2_fp32 = * (float32_t *) &fma2_trunc;

                    // Perform accumulation (first in fp64 to avoid uncontrolled rounding => then clip to fp32 with appropiate rounding)
                    float64_t res64       = (float64_t)accum_fp32 + (float64_t)fma1_fp32 + (float64_t)fma2_fp32;
                    uint64_t  hex_res64 = * (uint64_t * ) &res64;
                    hex_res64           = hex_res64 & 0xFFFFFFFFE0000000; // Cut mantissa down to 23 bits from original 52 bits of FP64
                    res64               = * (float64_t * ) &hex_res64;
                    float32_t res = (float32_t)res64;

                    // Finally, clear output denormals
                    uint32_t hex_res = * (uint32_t * ) &res;
                    hex_res          = ((hex_res & 0x7F800000) == 0) ? (hex_res & 0x80000000) : hex_res; // kill output denormal to zero (preserve sign)
                    res              = * (float32_t *) &hex_res;
                    FREGS[4*ar+bf].f[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %f = %f + %f * %f + %f * %f\n", 4 * ar + bf, bm, res, accum, mul1_a, mul1_b, mul2_a, mul2_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%04x * 0x%04x + 0x%04x * 0x%04x\n", * ((int *) &accum), mul1_a_hex, mul1_b_hex, mul2_a_hex, mul2_b_hex);)
                    // Uncomment to help debugging
                    //DEBUG_EMU(gprintf("\tBefore truncating accum 0x%08x, fma1 0x%08x, fma2 0x%08x\n", hex_accum, hex_fma1, hex_fma2);)
                    //DEBUG_EMU(gprintf("\tAfter truncating  accum 0x%08x, fma1 0x%08x, fma2 0x%08x\n", accum_trunc, fma1_trunc, fma2_trunc);)
                    //DEBUG_EMU(gprintf("\tExponents accum exp %d, fma1 exp %d, fma2 exp %d\n", accum_exp, fma1_exp, fma2_exp);)
                    //DEBUG_EMU(gprintf("\tRemove bits according to max exp %d, accum %d, fma1 %d, fma2 %d\n", exp_max, accum_erase, fma1_erase, fma2_erase);)
#else
                    // 1st FMA
                    uint32_t  mul_a_hex  = SCP[astart+ar][af].h[am * 2];
                    float32_t mul_a      = _cvtsh_ss(mul_a_hex);
                    uint32_t  mul_b_hex  = SCP[br][bf].h[bm * 2];
                    float32_t mul_b      = _cvtsh_ss(mul_b_hex);
                    float32_t res        = fmaf(mul_a, mul_b, accum);
                    FREGS[4*ar+bf].f[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %f = %f + %f * %f\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%04x * 0x%04x\n", cast_float32_to_uint32(accum), mul_a_hex, mul_b_hex);)

                    // 2nd FMA
                    mul_a_hex            = SCP[astart+ar][af].h[am * 2 + 1];
                    mul_a                = _cvtsh_ss(mul_a_hex);
                    mul_b_hex            = SCP[br][bf].h[bm * 2 + 1];
                    mul_b                = _cvtsh_ss(mul_b_hex);
                    accum                = FREGS[4*ar+bf].f[bm];
                    res                  = fmaf(mul_a, mul_b, accum);
                    FREGS[4*ar+bf].f[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %f = %f + %f * %f\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%04x * 0x%04x\n", cast_float32_to_uint32(accum), mul_a_hex, mul_b_hex);)
#endif

                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][4*ar+bf][bm][ac] = FREGS[4*ar+bf].u[bm];

                    if ((first_pass == 0) || (ac != 0)) {                    
                    // If both As are zeroes, we skip operation
                      if((SCP[astart+ar][af].h[am * 2] == 0) && (SCP[astart+ar][af].h[am * 2 + 1] == 0))
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    // If both Bs are zeroes, we skip operation
                      if((SCP[br][bf].h[bm * 2] == 0) && (SCP[br][bf].h[bm * 2 + 1] == 0))
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    }
                }
            }
            DEBUG_EMU(gprintf("\tC row %d: f%d[%d] = 0x%08x (%f)\n",ar,4*ar  ,0,FREGS[4*ar+0].u[0],FREGS[4*ar+0].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,1,FREGS[4*ar+0].u[1],FREGS[4*ar+0].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,2,FREGS[4*ar+0].u[2],FREGS[4*ar+0].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,3,FREGS[4*ar+0].u[3],FREGS[4*ar+0].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,0,FREGS[4*ar+1].u[0],FREGS[4*ar+1].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,1,FREGS[4*ar+1].u[1],FREGS[4*ar+1].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,2,FREGS[4*ar+1].u[2],FREGS[4*ar+1].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,3,FREGS[4*ar+1].u[3],FREGS[4*ar+1].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,0,FREGS[4*ar+2].u[0],FREGS[4*ar+2].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,1,FREGS[4*ar+2].u[1],FREGS[4*ar+2].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,2,FREGS[4*ar+2].u[2],FREGS[4*ar+2].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,3,FREGS[4*ar+2].u[3],FREGS[4*ar+2].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,0,FREGS[4*ar+3].u[0],FREGS[4*ar+3].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,1,FREGS[4*ar+3].u[1],FREGS[4*ar+3].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,2,FREGS[4*ar+3].u[2],FREGS[4*ar+3].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,3,FREGS[4*ar+3].u[3],FREGS[4*ar+3].f[3]);)
        }
    }
    // FP16
    else if(type == 2)
    {
        if(first_pass)
        {
            for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
            {
                for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
                {
                    int bf = bc / 8;
                    int bm = bc % 8;
                    if(MREGS[0].b[bm] == 0) continue;

                    FREGS[4*ar+bf].h[bm] = 0.0f;
                    tensorfma_data[current_thread][4*ar+bf][bm / 2][0] = 0;
                }
            }
        }

        for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if(tm)
            {
                if(!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for(int i = 0; i < 16; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if(first_pass)
                        tensorfma_mask_skip[0][ar] = 0;
                    continue;
                }
            }

            for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
            {
                int bf = bc / 8;
                int bm = bc % 8;
                if(MREGS[0].b[bm] == 0) continue;

                for ( int ac = 0; ac < acols; ac++ )     // A: accumulate acols values
                {
                    int af = (aoffset + ac) / 8;
                    int am = (aoffset + ac) % 8;
                    int br = bstart + ac;                // B: traverse rows

                    uint32_t  mul_a_hex = SCP[astart+ar][af].h[am];
                    float32_t mul_a     = _cvtsh_ss(mul_a_hex);
                    uint32_t  mul_b_hex = SCP[br][bf].h[bm];
                    float32_t mul_b     = _cvtsh_ss(mul_b_hex);
                    uint32_t  accum_hex = FREGS[4*ar+bf].h[bm];
                    float32_t accum     = _cvtsh_ss(accum_hex);
                    float32_t res       = fmaf(mul_a, mul_b, accum);
                    FREGS[4*ar+bf].h[bm] = _cvtss_sh(res, 0);

                    DEBUG_EMU(gprintf("\tTensor FMA f%d[%d]: %f = %f + %f * %f\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%04x + 0x%04x * 0x%04x\n", accum_hex, mul_a_hex, mul_b_hex);)

                    // For checker purposes we keep the data of all the passes
                    if(bm & 1)
                    {
                        tensorfma_data[current_thread][4*ar+bf][bm/2][ac] = FREGS[4*ar+bf].u[bm/2];
                    }
                }
            }
            DEBUG_EMU(gprintf("\tC row %d: f%d[%d] = 0x%08x (%f)\n",ar,4*ar  ,0,FREGS[4*ar+0].u[0],FREGS[4*ar+0].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,1,FREGS[4*ar+0].u[1],FREGS[4*ar+0].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,2,FREGS[4*ar+0].u[2],FREGS[4*ar+0].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar  ,3,FREGS[4*ar+0].u[3],FREGS[4*ar+0].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,0,FREGS[4*ar+1].u[0],FREGS[4*ar+1].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,1,FREGS[4*ar+1].u[1],FREGS[4*ar+1].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,2,FREGS[4*ar+1].u[2],FREGS[4*ar+1].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+1,3,FREGS[4*ar+1].u[3],FREGS[4*ar+1].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,0,FREGS[4*ar+2].u[0],FREGS[4*ar+2].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,1,FREGS[4*ar+2].u[1],FREGS[4*ar+2].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,2,FREGS[4*ar+2].u[2],FREGS[4*ar+2].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+2,3,FREGS[4*ar+2].u[3],FREGS[4*ar+2].f[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,0,FREGS[4*ar+3].u[0],FREGS[4*ar+3].f[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,1,FREGS[4*ar+3].u[1],FREGS[4*ar+3].f[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,2,FREGS[4*ar+3].u[2],FREGS[4*ar+3].f[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%f)\n",    4*ar+3,3,FREGS[4*ar+3].u[3],FREGS[4*ar+3].f[3]);)
        }
    }
    else if(type == 3) //INT8-INT32
    {

        if(first_pass)
        {
            for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
            {
                for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
                {
                    int bf = bc / 4;
                    int bm = bc % 4;
                    if(MREGS[0].b[bm*2] == 0) continue;

                    FREGS[4*ar+bf].u[bm] = 0;
                    tensorfma_data[current_thread][4*ar+bf][bm][0] = 0;
                }
            }
        }

        for ( int ar = 0; ar < arows; ar++ )             // A: traverse arows rows
        {
            // Checks if needs to skip the current pass due convolution
            if(tm)
            {
                if(!tmask_pass(ar))
                {
                    // Mark all passes as skipped
                    for(int i = 0; i < 16; i++)
                        tensorfma_mask_skip[i][ar] = 1;
                    // Except 1st if first pass
                    if(first_pass)
                        tensorfma_mask_skip[0][ar] = 0;
                    continue;
                }
            }

            int32_t w = (sizeof(int32_t) << 3) - 1;//used for the bitwise saturation
            for ( int bc = 0; bc < bcols; bc++ )         // B: process bcols cols
            {
                int bf = bc / 4;
                int bm = bc % 4;
                if(MREGS[0].b[bm*2] == 0) continue;

                for ( int ac = 0; ac < acols; ac++ )     // A: accumulate acols values
                {
                    int af = (aoffset + ac) / 4;
                    int am = (aoffset + ac) % 4;
                    int br = bstart + ac;                // B: traverse rows

                    // Doing four IMAs per lane and accumulating to previous results
                    int32_t accum     = FREGS[4*ar+bf].u[bm];

                    // 1st IMA
                    int32_t  mul_a    = sext8_2 (SCP[astart+ar][af].b[am * 4]);
                    int32_t  mul_b    = sext8_2 (SCP[br][bf].b[bm * 4]);
                    int32_t  res_mul  = mul_a * mul_b;
                    int32_t  res      = res_mul + accum;
                    //BITWISE SATURATION
                    res = (~((~(res_mul^accum)  & (res_mul^res)) >> w) & res) + (((~(res_mul^accum) & (res_mul^res)) >> w) & ((1<<w) ^ (res >> w)));

                    FREGS[4*ar+bf].u[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor IMA f%d[%d]: %d = %d + %d * %d\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%02x * 0x%02x\n", * ((int *) &accum), mul_a, mul_b);)

                    // 2nd IMA
                    mul_a    = sext8_2 (SCP[astart+ar][af].b[am * 4 + 1]);
                    mul_b    = sext8_2 (SCP[br][bf].b[bm * 4 + 1]);
                    accum    = FREGS[4*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;
                    //BITWISE SATURATION
                    res = (~((~(res_mul^accum)  & (res_mul^res)) >> w) & res) + (((~(res_mul^accum) & (res_mul^res)) >> w) & ((1<<w) ^ (res >> w)));

                    FREGS[4*ar+bf].u[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor IMA f%d[%d]: %d = %d + %d * %d\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%02x * 0x%02x\n", * ((int *) &accum), mul_a, mul_b);)

                   // 3rd IMA
                    mul_a    = sext8_2 (SCP[astart+ar][af].b[am * 4 + 2]);
                    mul_b    = sext8_2 (SCP[br][bf].b[bm * 4 + 2]);
                    accum    = FREGS[4*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;
                    //BITWISE SATURATION
                    res = (~((~(res_mul^accum)  & (res_mul^res)) >> w) & res) + (((~(res_mul^accum) & (res_mul^res)) >> w) & ((1<<w) ^ (res >> w)));

                    FREGS[4*ar+bf].u[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor IMA f%d[%d]: %d = %d + %d * %d\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%02x * 0x%02x\n", * ((int *) &accum), mul_a, mul_b);)

                    // 4th IMA
                    mul_a    = sext8_2 (SCP[astart+ar][af].b[am * 4 + 3]);
                    mul_b    = sext8_2 (SCP[br][bf].b[bm * 4 + 3]);
                    accum    = FREGS[4*ar+bf].u[bm];
                    res_mul  = mul_a * mul_b;
                    res      = res_mul + accum;
                    //BITWISE SATURATION
                    res = (~((~(res_mul^accum)  & (res_mul^res)) >> w) & res) + (((~(res_mul^accum) & (res_mul^res)) >> w) & ((1<<w) ^ (res >> w)));

                    FREGS[4*ar+bf].u[bm] = res;

                    DEBUG_EMU(gprintf("\tTensor IMA f%d[%d]: %d = %d + %d * %d\n", 4 * ar + bf, bm, res, accum, mul_a, mul_b);)
                    DEBUG_EMU(gprintf("\t                       = 0x%08x + 0x%02x * 0x%02x\n", * ((int *) &accum), mul_a, mul_b);)

                    // For checker purposes we keep the data of all the passes
                    tensorfma_data[current_thread][4*ar+bf][bm][ac] = FREGS[4*ar+bf].u[bm];

                    if ((first_pass == 0) || (ac != 0)) {                    
                    // If As are zeroes, we skip operation
                      if((SCP[astart+ar][af].b[am * 4] == 0) && (SCP[astart+ar][af].b[am * 4 + 1] == 0) && (SCP[astart+ar][af].b[am * 4 + 2] == 0) && (SCP[astart+ar][af].b[am * 4 + 3] == 0))
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    // If Bs are zeroes, we skip operation
                      if((SCP[br][bf].b[bm * 4] == 0) && (SCP[br][bf].b[bm * 4 + 1] == 0) && (SCP[br][bf].b[bm * 4 + 2] == 0) && (SCP[br][bf].b[bm * 4 + 3] == 0))
                          tensorfma_zero_skip[ac][ar*4+bc/4][bc%4] = 1;
                    }
                }
            }
            DEBUG_EMU(gprintf("\tC row %d: f%d[%d] = 0x%08x (%d)\n",ar,4*ar  ,0,FREGS[4*ar+0].u[0],FREGS[4*ar+0].u[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar  ,1,FREGS[4*ar+0].u[1],FREGS[4*ar+0].u[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar  ,2,FREGS[4*ar+0].u[2],FREGS[4*ar+0].u[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar  ,3,FREGS[4*ar+0].u[3],FREGS[4*ar+0].u[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+1,0,FREGS[4*ar+1].u[0],FREGS[4*ar+1].u[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+1,1,FREGS[4*ar+1].u[1],FREGS[4*ar+1].u[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+1,2,FREGS[4*ar+1].u[2],FREGS[4*ar+1].u[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+1,3,FREGS[4*ar+1].u[3],FREGS[4*ar+1].u[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+2,0,FREGS[4*ar+2].u[0],FREGS[4*ar+2].u[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+2,1,FREGS[4*ar+2].u[1],FREGS[4*ar+2].u[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+2,2,FREGS[4*ar+2].u[2],FREGS[4*ar+2].u[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+2,3,FREGS[4*ar+2].u[3],FREGS[4*ar+2].u[3]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+3,0,FREGS[4*ar+3].u[0],FREGS[4*ar+3].u[0]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+3,1,FREGS[4*ar+3].u[1],FREGS[4*ar+3].u[1]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+3,2,FREGS[4*ar+3].u[2],FREGS[4*ar+3].u[2]);)
            DEBUG_EMU(gprintf("\t         f%d[%d] = 0x%08x (%d)\n",    4*ar+3,3,FREGS[4*ar+3].u[3],FREGS[4*ar+3].u[3]);)
        }

    }
    else
    {
        DEBUG_EMU(gprintf("ERROR Unimplemented tensor FMA Type!!\n");)
    }
}

uint64_t get_tensorfma_value(int entry, int pass, int block, int * size, int * passes, bool * mask_skip)
{
    * size      = tensorfma_size[current_thread];
    * passes    = tensorfma_passes[current_thread];
    * mask_skip = tensorfma_mask_skip[pass][entry / 4] || tensorfma_zero_skip[pass][entry][block];
    return tensorfma_data[current_thread][entry][block][pass];
}

// ----- TensorReduce emulation ------------------------------------------------

static void tensorreduce(uint64_t value)
{
    uint64_t other_min;
    uint64_t action;

    get_reduce_info(value, &other_min, &action);

    reduce_size[current_thread] = 0;

    // Do nothing
    if(action == 2) return;
    // Send
    if(action == 0) return;
    // Receive

    //op = rs[35:32]
    int      start_reg = (value >> 57) & 0x1F;
    uint32_t operation = (value >> 24) & 0xF;
    int      num_reg   = (value >> 16) & 0xFF;

    // Info for checker
    reduce_size[current_thread]  = num_reg;
    reduce_entry[current_thread] = start_reg;

    if((start_reg + num_reg - 1) >= 32)
    {
        DEBUG_EMU(gprintf("ERROR accessing register out of bound in reduce: %016llx\n", value);)
    }
    for(int i = 0; i < num_reg; i++)
    {
        if((start_reg + i) >= 32) break;

        for(int j = 0; j < 4; j++)
        {
            if(operation == 0) // FADD
            {
                float old_val = FREGS[i + start_reg].f[j];
                uint32_t old_valu = FREGS[i + start_reg].u[j];
                FREGS[i + start_reg].f[j] = old_val + fregs[other_min<<1][i + start_reg].f[j];

                DEBUG_EMU(gprintf("\tReduce (fadd) f%d[%d]: %f = %f(m%d) + %f(m%d)\n", i + start_reg, j,
                FREGS[i + start_reg].f[j], old_val, current_thread>>1, fregs[other_min<<1][i + start_reg].f[j], other_min);)
                DEBUG_EMU(gprintf("\t                %08x = 0x%08x + 0x%08x\n", FREGS[i + start_reg].u[j], old_valu, fregs[other_min<<1][i + start_reg].u[j]);)
            }
            else if(operation == 4) // IADD
            {
                uint32_t old_valu = FREGS[i + start_reg].u[j];
                FREGS[i + start_reg].u[j] = old_valu + fregs[other_min<<1][i + start_reg].u[j];

                DEBUG_EMU(gprintf("\tReduce (iadd) f%d[%d]: %d = %d(m%d) + %d(m%d)\n", i + start_reg, j,
                FREGS[i + start_reg].u[j], old_valu, current_thread>>1, fregs[other_min<<1][i + start_reg].u[j], other_min);)
                DEBUG_EMU(gprintf("\t                %08x = 0x%08x + 0x%08x\n", FREGS[i + start_reg].u[j], old_valu, fregs[other_min<<1][i + start_reg].u[j]);)
            }

            else if(operation == 8) // FGET
            {
                FREGS[i + start_reg].f[j] = fregs[other_min<<1][i + start_reg].f[j];
                DEBUG_EMU(gprintf("\tReduce (get) f%d[%d]: <= %f(m%d)\n", i + start_reg, j, FREGS[i + start_reg].f[j],other_min);)
                DEBUG_EMU(gprintf("\t                    <= 0x%08x\n", FREGS[i + start_reg].u[j]);)
            }
            else
            {
                DEBUG_EMU(gprintf("ERROR reduce/broadcast operation = %d not yet supported in emu\n", operation);)
            }

            // Checker
            reduce_data[current_thread][i + start_reg][j] = FREGS[i + start_reg].u[j];
        }
    }
}

// Helper function that given the written value to the CSR, returns:
//   - what is the ID of the other minion of the reduce
//   - what is the action taken by the minion (send, receive, do nothing)
void get_reduce_info(uint64_t value, uint64_t * other_min, uint64_t * action)
{
    uint64_t level = (value >> 4) & 0xF;
    uint64_t type  = value & 3;
    uint64_t minion_id = current_thread >> 1;

    // REDUCE: Compute sender/receiver assuming recursive halving
    if(type == 3)
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if((minion_id & minion_mask) == distance)
        {
            * action    = 0; // sender
            * other_min = minion_id - distance;
        }
        else if((minion_id & minion_mask) == 0)
        {
            * action    = 1; // receiver
            * other_min = minion_id + distance;
        }
        else
        {
            * action    = 2; // do nothing
        }
    }
    // BROADCAST: Compute sender/receiver assuming recursive halving
    else if(type == 2)
    {
        uint64_t distance = 1 << level;
        uint64_t minion_mask = (1 << (level + 1)) - 1;
        if((minion_id & minion_mask) == distance)
        {
            * action    = 1; // sender
            * other_min = minion_id - distance;
        }
        else if((minion_id & minion_mask) == 0)
        {
            * action    = 0; // receiver
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

#define FL_UC_BASE_REGION 0xFFF00000

// Fast local barriers can be accessed through UC to do stores and loads,
// and also through the CSR that implement the fast local barrier function.
static uint64_t flbarrier(uint64_t value)
{
    uint64_t barrier = value & 0x7;
    uint64_t limit   = (value >> 3) & 0x7F;

    // Gets what is the address that the fast local barrier is mapped to
    uint64_t addr    = FL_UC_BASE_REGION + (barrier * 8);

    uint64_t orig_value = memread64(addr);
    uint64_t result = -1;
    printf("FastLocalBarrier: Shire %i: Minion %i Thread %i doing barrier %" PRIu64 " value  %" PRIu64 ", limit %" PRIu64 " \n", current_thread>>7, current_thread>>1 ,current_thread &1 , barrier ,orig_value , limit );
    // Last guy, return 1 and zero barrier
    if(orig_value == limit)
    {
        printf("FastLocalBarrier: last minion Shire %i!!\n", current_thread>>7 );
        memwrite64(addr, 0);
        result = 1;
    }
    // Not the last guy, return 0 and increment barrier
    else
    {
        printf("FastLocalBarrier: Limit %" PRIu64", Incrementing to %" PRIu64 "!!\n",limit , orig_value + 1);
        memwrite64(addr, orig_value + 1);
        result = 0;
    }
    return result;
}
