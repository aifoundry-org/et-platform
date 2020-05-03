/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <array>
#include <cassert>
#include <stdexcept>

#include "cache.h"
#include "decode.h"
#include "emu.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "log.h"
#include "memop.h"
#include "mmu.h"
#include "processor.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif
#include "tensor.h"
#include "tensor_error.h"
#include "tensor_mask.h"
#include "traps.h"
#include "utility.h"


#define FREGS cpu[current_thread].fregs
#define TENC  cpu[current_thread].tenc
#define SCP   scp[current_thread]


// SCP checks
#ifdef SYS_EMU
    #define L1_SCP_CHECK_FILL(thread, idx, id) do { \
        if (sys_emu::get_l1_scp_check()) \
            sys_emu::get_l1_scp_checker().l1_scp_fill(thread, idx, id); \
    } while (0)
    #define L1_SCP_CHECK_READ(thread, idx) do { \
        if (sys_emu::get_l1_scp_check()) \
            sys_emu::get_l1_scp_checker().l1_scp_read(thread, idx); \
    } while (0)
#else
    #define L1_SCP_CHECK_FILL(thread, idx, id)  do { } while (0)
    #define L1_SCP_CHECK_READ(thread, idx)      do { } while (0)
#endif


namespace bemu {


extern std::array<Processor,EMU_NUM_THREADS> cpu;
extern uint32_t current_inst;


// Scratchpad
// FIXME: The scratchpad is shared among all threads of a minion. We should
// really just have one scratchpad per minion, not one per thread. This all
// works for now because only thread0 of a minion can access the scratchpad.
std::array<std::array<cache_line_t,L1_SCP_ENTRIES+TFMA_MAX_AROWS>,EMU_NUM_THREADS> scp;


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


// ----- Scratchpad emulation --------------------------------------------------

void clear_l1scp(unsigned minion)
{
    unsigned thread = minion * EMU_THREADS_PER_MINION;
    for (int i = 0; i < L1_SCP_ENTRIES; ++i) {
        scp[thread][i].u8.fill(0);
    }
}


// ----- TensorConvolution emulation -------------------------------------------

// Update to the tensor Mask due a convolution CSR write
void tensor_mask_update()
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


void tensor_coop_write(uint64_t value)
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
    uint64_t stride  = X31 & 0xFFFFFFFFFFC0ULL;
    int      id      = X31 & 1;

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
        if (!bemu::shire_other_esrs[shire].shire_coop_mode)
            throw trap_illegal_instruction(current_inst);
    }

    // Check if SCP is enabled
    if (cpu[current_thread].mcache_control != 0x3) {
        LOG(WARN, "%s", "\tTensorLoad with SCP disabled!!");
        update_tensor_error(1 << 4);
        return;
    }

    if (tenb) {
        cpu[current_thread].tensorload_setupb_topair = true;
        cpu[current_thread].tensorload_setupb_numlines = rows;
        if ((current_thread^1) < EMU_NUM_THREADS) {
            cpu[current_thread^1].tensorload_setupb_topair = true;
            cpu[current_thread^1].tensorload_setupb_numlines = rows;
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
        cpu[current_thread].shadow_txstride[int(tenb)] = X31;
    } else {
        cpu[current_thread].txload[int(tenb)] = control;
        cpu[current_thread].txstride[int(tenb)] = X31;
    }
#else
    cpu[current_thread].txload[int(tenb)] = control;
    cpu[current_thread].txstride[int(tenb)] = X31;
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

void tensor_load_l2_start(uint64_t control)
{
    uint64_t stride  = X31 & 0xFFFFFFFFFFC0ULL;
    uint32_t id      = X31 & 1ULL;

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
                if(sys_emu::get_l2_scp_check())
                {
                    sys_emu::get_l2_scp_checker().l2_scp_fill(current_thread, dst + i, id, paddr);
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

void tensor_quant_start(uint64_t value)
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
        log_tensor_quant_new_transform((funct == 10) && (ncols > VLENW));

        for (unsigned row = 0; row < nrows; ++row) {
            for (unsigned col = 0; col < ncols; col += VLENW) {
                unsigned nelem = std::min(ncols - col, unsigned(VLENW));
                unsigned fs1 = (fstart + row*2 + col/VLENW) % NFREGS;
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

void tensor_store_start(uint64_t tstorereg)
{
    uint64_t tstore_scp = (tstorereg >> 48) & 0x1;

    if (tstore_scp)
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to scratchpad source
        int      scpstart =  (tstorereg & 0x3F00000000000000ULL) >> 56;      // Start scratchpad entry to store
        int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
        uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFC0ULL);     // Address where to store the results
        int      src      = scpstart % L1_SCP_ENTRIES;

        uint64_t stride   = X31 & 0x0000FFFFFFFFFFC0ULL;

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

        uint64_t stride   = X31 & 0x0000FFFFFFFFFFF0ULL;

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
            if (!bemu::shire_other_esrs[shire].shire_coop_mode)
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
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = 0;
                        LOG(DEBUG, "\tTensorFMA32(0) f%u[%u] = 0x0", i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW);
                        log_tensor_fma_write(0, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, 0);
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
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    LOG(DEBUG, "\tTensorFMA32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, fpu::UI32(c), fpu::UI32(a), fpu::UI32(b));
                    log_tensor_fma_write(k, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
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
                    float32_t c0 = FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].f32[j%VLENW];
                    float32_t c = fpu::f32_mulAdd(a, b, c0);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    LOG(DEBUG, "\tTensorFMA32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + 0x%08" PRIx32 " * 0x%08" PRIx32,
                        k, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, fpu::UI32(c), fpu::UI32(c0), fpu::UI32(a), fpu::UI32(b));
                    log_tensor_fma_write(k, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%u[%u] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}


void tensor_fma32_start(uint64_t tfmareg)
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
    bool load_tenb = cpu[current_thread].tensorload_setupb_topair;
    int  brows_tenb = cpu[current_thread].tensorload_setupb_numlines;
    cpu[current_thread].tensorload_setupb_topair = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        cpu[current_thread^1].tensorload_setupb_topair = false;

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
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = 0;
                        LOG(DEBUG, "\tTensorFMA16A32(0) f%u[%u] = 0x0", i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW);
                        log_tensor_fma_write(0, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, 0);
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
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%u[%u]: 0x%08" PRIx32 " = (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, fpu::UI32(c), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                    log_tensor_fma_write(k/2, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
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
                    float32_t c0 = FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].f32[j%VLENW];
                    float32_t c = fpu::f1632_mulAdd3(a1, b1, a2, b2, c0);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    LOG(DEBUG, "\tTensorFMA16A32(%d) f%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%04" PRIx16 " * 0x%04" PRIx16 ") + (0x%04" PRIx16 " * 0x%04" PRIx16 ")",
                        k/2, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, fpu::UI32(c), fpu::UI32(c0), fpu::UI16(a1), fpu::UI16(b1), fpu::UI16(a2), fpu::UI16(b2));
                    log_tensor_fma_write(k/2, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                }
            }
        }
    }

    // logging
    for (int i = 0; i < arows; ++i)
    {
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: f%u[%u] = 0x%08" PRIx32, i, j,
                i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
    }

    set_fp_exceptions();
    dirty_fp_state();
}


void tensor_fma16a32_start(uint64_t tfmareg)
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
    bool load_tenb = cpu[current_thread].tensorload_setupb_topair;
    int  brows_tenb = 2 * cpu[current_thread].tensorload_setupb_numlines;
    cpu[current_thread].tensorload_setupb_topair = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        cpu[current_thread^1].tensorload_setupb_topair = false;

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
        freg_t* dst = write_freg ? FREGS.data() : TENC.data();
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
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = (first_pass && !k) ? 0 : TENC[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW];
                        LOG(DEBUG, "\tTensorIMA8A32(%d) f%u[%u] = 0x%08" PRIx32, k/4, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                        log_tensor_fma_write(k/4, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW]);
                    }
                }
                else if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = 0;
                        log_tensor_fma_write(0, false, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
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
                    dst[i*TFMA_REGS_PER_ROW+j/VLENW].i32[j%VLENW] = c;
                    LOG(DEBUG, "\tTensorIMA8A32(%d) %s%u[%u]: 0x%08" PRIx32 " = (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ")",
                        k/4, dname, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, c, uint8_t(a1), uint8_t(b1), uint8_t(a2), uint8_t(b2), uint8_t(a3), uint8_t(b3), uint8_t(a4), uint8_t(b4));
                    log_tensor_fma_write(k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, uint32_t(c));
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
                    int32_t c0 = TENC[i*TFMA_REGS_PER_ROW+j/VLENW].i32[j%VLENW];
                    int32_t c = c0 + (a1 * b1) + (a2 * b2) + (a3 * b3) + (a4 * b4);
                    dst[i*TFMA_REGS_PER_ROW+j/VLENW].i32[j%VLENW] = c;
                    LOG(DEBUG, "\tTensorIMA8A32(%d) %s%u[%u]: 0x%08" PRIx32 " = 0x%08" PRIx32 " + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ") + (0x%02" PRIx8 " * 0x%02" PRIx8 ")",
                        k/4, dname, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, c, c0, uint8_t(a1), uint8_t(b1), uint8_t(a2), uint8_t(b2), uint8_t(a3), uint8_t(b3), uint8_t(a4), uint8_t(b4));
                    log_tensor_fma_write(k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, uint32_t(c));
                }
            }
        }
    }
    if (tenc2rf)
        dirty_fp_state();

    // logging
    for (int i = 0; i < arows; ++i)
    {
        const freg_t* dst = tenc2rf ? FREGS.data() : TENC.data();
        const char* dname = tenc2rf ? "f" : "TenC";
        for (int j = 0; j < bcols; ++j)
            LOG(DEBUG, "\tC[%d][%d]: %s%u[%u] = 0x%08" PRIx32, i, j, dname,
                i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, dst[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
    }
}


void tensor_ima8a32_start(uint64_t tfmareg)
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
    bool load_tenb = cpu[current_thread].tensorload_setupb_topair;
    int  brows_tenb = 4 * cpu[current_thread].tensorload_setupb_numlines;
    cpu[current_thread].tensorload_setupb_topair = false;
    if ((current_thread^1) < EMU_NUM_THREADS)
        cpu[current_thread^1].tensorload_setupb_topair = false;

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

void tensor_reduce_start(uint64_t value)
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
        for (unsigned j = 0; j < VLENW; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_add(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 2: // fmax
        LOG(DEBUG, "\t%s(recv) op=fmax sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_maximumNumber(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 3: // fmin
        LOG(DEBUG, "\t%s(recv) op=fmin sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            FREGS[recv.regid].f32[j] = fpu::f32_minimumNumber(cpu[thread].fregs[send.regid].f32[j], FREGS[recv.regid].f32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        set_fp_exceptions();
        break;
    case 4: // iadd
        LOG(DEBUG, "\t%s(recv) op=iadd sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            FREGS[recv.regid].u32[j] = cpu[thread].fregs[send.regid].u32[j] + FREGS[recv.regid].u32[j];
        }
        LOG_FREG("(this) =", recv.regid);
        break;
    case 6: // imax
        LOG(DEBUG, "\t%s(recv) op=imax sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            FREGS[recv.regid].i32[j] = std::max(cpu[thread].fregs[send.regid].i32[j], FREGS[recv.regid].i32[j]);
        }
        LOG_FREG("(this) =", recv.regid);
        break;
    case 7: // imin
        LOG(DEBUG, "\t%s(recv) op=imin sender=H%u", reducecmd[type], thread);
        LOG_FREG_OTHER(thread, "(othr) :", send.regid);
        LOG_FREG("(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
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
    cpu[current_thread].wait.state = Processor::Wait::State::WaitReady;
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
    if(sys_emu::get_l1_scp_check())
    {
        // TensorWait TLoad Id0
        if     ((cpu[current_thread].wait.id) == 0) { sys_emu::get_l1_scp_checker().l1_scp_wait(current_thread, 0); }
        else if((cpu[current_thread].wait.id) == 1) { sys_emu::get_l1_scp_checker().l1_scp_wait(current_thread, 1); }
    }
    if(sys_emu::get_l2_scp_check())
    {
        if((cpu[current_thread].wait.id) == 2) { sys_emu::get_l2_scp_checker().l2_scp_wait(current_thread, 0); }
        else if((cpu[current_thread].wait.id) == 3) { sys_emu::get_l2_scp_checker().l2_scp_wait(current_thread, 1); }
    }
#endif
    cpu[current_thread].wait.state = Processor::Wait::State::Idle;
    log_tensor_error_value(cpu[current_thread].tensor_error);
}


} // namespace bemu
