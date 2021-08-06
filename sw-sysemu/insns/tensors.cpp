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
#include "emu_defines.h"
#include "emu_gio.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "insn_util.h"
#include "log.h"
#include "mmu.h"
#include "system.h"
#include "tensor.h"
#include "tensor_error.h"
#include "traps.h"
#include "utility.h"
#ifdef SYS_EMU
#include "sys_emu.h"
#endif


#define FREGS cpu.fregs
#define TENC  cpu.core->tenc
#define SCP   cpu.core->scp


// SCP checks
#ifdef SYS_EMU
    #define SYS_EMU_PTR cpu.chip->emu()
    #define L1_SCP_CHECK_FILL(cpu, idx, id) do { \
        if (SYS_EMU_PTR->get_l1_scp_check()) { \
            SYS_EMU_PTR->get_l1_scp_checker().l1_scp_fill(hart_index(cpu), idx, id); \
        } \
    } while (0)
    #define L1_SCP_CHECK_READ(cpu, idx) do { \
        if (SYS_EMU_PTR->get_l1_scp_check()) { \
            SYS_EMU_PTR->get_l1_scp_checker().l1_scp_read(hart_index(cpu), idx); \
        } \
    } while (0)
    #define L2_SCP_CHECK_FILL(cpu, idx, id, addr) do { \
        if (SYS_EMU_PTR->get_l2_scp_check()) { \
            SYS_EMU_PTR->get_l2_scp_checker().l2_scp_fill(hart_index(cpu), idx, id, addr); \
        } \
    } while (0)
#else
    #define L1_SCP_CHECK_FILL(cpu, idx, id)  do { } while (0)
    #define L1_SCP_CHECK_READ(cpu, idx)      do { } while (0)
    #define L2_SCP_CHECK_FILL(cpu, idx, id, addr) do { } while (0)
#endif


namespace bemu {


static const char* get_rounding_mode(const Hart& cpu, int mode)
{
    static const char* rmnames[] = {
        "rne",      "rtz",       "rdn",       "rup",
        "rmm",      "res5",      "res6",      "dyn",
        "dyn(rne)", "dyn(rtz)",  "dyn(rdn)",  "dyn(rup)",
        "dyn(rmm)", "dyn(res5)", "dyn(res6)", "dyn(res7)",
    };

    return rmnames[(mode == rmdyn) ? (8 + cpu.frm()) : (mode & 7)];
}


static const char* get_reduce_state(Core::Reduce::State state)
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

void clear_l1scp(Hart& cpu)
{
    for (int i = 0; i < L1_SCP_ENTRIES; ++i) {
        cpu.core->scp[i].u8.fill(0);
    }
}


// ----- TensorConvolution emulation -------------------------------------------

// Update to the tensor Mask due a convolution CSR write
void tensor_mask_update(Hart& cpu)
{
    uint16_t tmask_value = 0;

    // Get the sizes of the convolution
    uint64_t tconvsizereg = cpu.tensor_conv_size;
    int srow =   int8_t((tconvsizereg >> 56) & 0xFF);
    int nrow = uint16_t((tconvsizereg >> 32) & 0xFFFF);
    int scol =   int8_t((tconvsizereg >> 24) & 0xFF);
    int ncol = uint16_t((tconvsizereg >>  0) & 0xFFFF);

    // Get the positions of the convolution
    uint64_t tconvctrlreg = cpu.tensor_conv_ctrl;
    int rowstart = int16_t((tconvctrlreg >> 32) & 0xFFFF);
    int colstart = int16_t((tconvctrlreg >>  0) & 0xFFFF);

    for (int i = 0; i < 16; ++i, rowstart += srow, colstart += scol)
    {
        if ((rowstart >= 0) && (rowstart < nrow) && (colstart >= 0) && (colstart < ncol))
        {
            LOG_HART(DEBUG, cpu, "TensorMask[%d] pass for row: %d, col: %d, nrow: %d, ncol %d",
                i, rowstart, colstart, nrow, ncol);
            tmask_value |= (1u << i);
        }
        else
        {
            LOG_HART(DEBUG, cpu, "TensorMask[%d] skip for row: %d, col: %d, nrow: %d, ncol %d",
                i, rowstart, colstart, nrow, ncol);
        }
    }
    cpu.tensor_mask = tmask_value;
    LOG_TENSOR_MASK("=");
}


void tensor_coop_write(Hart& cpu, uint64_t value)
{
    uint32_t neigh_mask  = (value >> 16) & 0xF;
    uint32_t minion_mask = (value >>  8) & 0xFF;
    uint32_t coop_id     = (value >>  0) & 0x1F;
    cpu.tensor_coop = neigh_mask << 16
                    | minion_mask << 8
                    | coop_id;
    // TODO: implement functionality checking the addresses and tcoop of every use of Tensor Load
    LOG_HART(DEBUG, cpu, "\tSetting Tensor Cooperation: coopneighmask=%02X, coopminmask=%02X, coopid=%d", neigh_mask, minion_mask, coop_id);
}


// ----- TensorLoad emulation --------------------------------------------------

void tensor_load_start(Hart& cpu, uint64_t control)
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
    LOG_HART(DEBUG, cpu, "\tStart TensorLoad with tm: %d, use_coop: %d, trans: %d, dst: %d, "
        "tenb: %d, addr: 0x%" PRIx64 ", boffset: %d, rows: %d, stride: 0x%" PRIx64 ", id: %d",
        tm, use_coop, trans, dst, tenb, addr, boffset, rows, stride, id);

#ifdef ZSIM
    bool txload_busy = (cpu.txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL);
    if (txload_busy) {
        if (cpu.shadow_txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_load_start() called while "
                                     "this thread's TensorLoad FSM is active");
    }
#else
    if (cpu.txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_load_start() called while "
                                 "this thread's TensorLoad FSM is active");
    }
#endif

    // Cooperative tensor loads require the shire to be in cooperative mode
    if (use_coop) {
        uint64_t shire = shire_index(cpu);
        if (!cpu.chip->shire_other_esrs[shire].shire_coop_mode)
            throw trap_illegal_instruction(cpu.inst.bits);
    }

    // Check if SCP is enabled
    if (cpu.core->mcache_control != 0x3) {
        LOG_HART(WARN, cpu, "%s", "\tTensorLoad with SCP disabled!!");
        update_tensor_error(cpu, 1 << 4);
        return;
    }

    if (tenb) {
        cpu.core->tensorload_setupb_topair = true;
        cpu.core->tensorload_setupb_numlines = rows;
    }
    else if ((trans == 0x3) || (trans == 0x4)) {
        // Invalid transformation
        LOG_HART(WARN, cpu, "%s", "\tTensorLoad with illegal transform!!");
        update_tensor_error(cpu, 1 << 1);
        return;
    }
    else if ((trans >= 0x5) && (trans <= 0x7)) {
        int size = 1 << ((trans & 0x3) - 1);
        if (size != 1 && size != 2 && size != 4) {
            LOG_HART(WARN, cpu, "\tTensorLoad element size (%d) not valid!", size);
            update_tensor_error(cpu, 1 << 1);
            return;
        }
    }

#ifdef ZSIM
    if (txload_busy) {
        cpu.shadow_txload[int(tenb)] = control;
        cpu.shadow_txstride[int(tenb)] = X31;
    } else {
        cpu.txload[int(tenb)] = control;
        cpu.txstride[int(tenb)] = X31;
    }
#else
    cpu.txload[int(tenb)] = control;
    cpu.txstride[int(tenb)] = X31;
    tensor_load_execute(cpu, tenb);
#endif
}


void tensor_load_execute(Hart& cpu, bool tenb)
{
    uint64_t txload = cpu.txload[int(tenb)];
    uint64_t stride = cpu.txstride[int(tenb)] & 0xFFFFFFFFFFC0ULL;
    int      id     = cpu.txstride[int(tenb)] & 1;

    int      tm       = (txload >> 63) & 0x1;
    int      use_coop = (txload >> 62) & 0x1;
    int      trans    = (txload >> 59) & 0x7;
    int      dst      = (txload >> 53) & 0x3F;
    uint64_t addr     = sext<48>(txload & 0xFFFFFFFFFFC0ULL);
    int      boffset  = (txload >>  4) & 0x03;
    int      rows     = ((txload     ) & 0xF) + 1;

    assert(int(tenb) == int((txload >> 52) & 0x1));

    LOG_HART(DEBUG, cpu, "\tExecute TensorLoad with tm: %d, use_coop: %d, trans: %d, dst: %d, "
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
        uint32_t thread = hart_index(cpu);
        uint32_t requested_mask;
        uint32_t present_mask;
        SYS_EMU_PTR->coop_tload_add(thread, tenb, tenb ? 0 : id, cpu.tensor_coop & 0xF, (cpu.tensor_coop >> 8) & 0xFF, cpu.tensor_coop >> 16);
        SYS_EMU_PTR->coop_tload_check(thread, false, id, requested_mask, present_mask);
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

    notify_tensor_load(cpu, trans, tenb, adj + (dst % L1_SCP_ENTRIES), tm ? cpu.tensor_mask.to_ulong() : 0xFFFF);

    //NO TRANS
    if (trans == 0x00)
    {
        LOG_HART(DEBUG, cpu, "%s", "\tTensorLoad: No transformation");
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || cpu.tensor_mask[i])
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                try {
                    uint64_t vaddr = sextVA(addr + i*stride);
                    assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
                    uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_TxLoad);
                    cpu.chip->memory.read(cpu, paddr, L1D_LINE_SIZE, SCP[idx].u32.data());
                    LOG_MEMREAD512(paddr, SCP[idx].u32.data());
                    LOG_SCP_32x16("=", idx);
                    L1_SCP_CHECK_FILL(cpu, idx, id);
                }
                catch (const sync_trap_t&) {
                    update_tensor_error(cpu, 1 << 7);
                    goto tensor_load_execute_done;
                }
                catch (const memory_error&) {
                    raise_bus_error_interrupt(cpu, 0);
                    continue;
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
            }
        }
    }
    //INTERLEAVE8
    else if (trans == 0x01)
    {
        LOG_HART(DEBUG, cpu, "%s", "\tTensorLoad: Interleave8");
        boffset *= 16;
        LOG_HART(DEBUG, cpu, "\t#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || cpu.tensor_mask[i])
            {
                bool dirty = false;
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(cpu, idx, id);
                for (int r = 0; r < 4; ++r)
                {
                    try {
                        Packed<128> tmp;
                        uint64_t vaddr = sextVA(addr + boffset + (4*i+r)*stride);
                        assert(addr_is_size_aligned(vaddr, 16));
                        uint64_t paddr = mmu_translate(cpu, vaddr, 16, Mem_Access_TxLoad);
                        cpu.chip->memory.read(cpu, paddr, 16, tmp.u32.data());
                        LOG_MEMREAD128(paddr, tmp.u32);
                        for (int c = 0; c < 16; ++c)
                            SCP[idx].u8[c*4 + r] = tmp.u8[c];
                    }
                    catch (const sync_trap_t&) {
                        update_tensor_error(cpu, 1 << 7);
                        goto tensor_load_execute_done;
                    }
                    catch (const memory_error&) {
                        raise_bus_error_interrupt(cpu, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty)
                {
                    notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                    LOG_SCP_32x16("=", idx);
                }
            }
        }
    }
    //INTERLEAVE16
    else if (trans == 0x02)
    {
        LOG_HART(DEBUG, cpu, "%s", "\tTensorLoad: Interleave16");
        boffset = (boffset & 0x2) * 16;
        LOG_HART(DEBUG, cpu, "\t#rows:%d - size:%d - start:%d - elements:%d - boffset:%d", rows, 1, boffset, 4, boffset/16);
        for (int i = 0; i < rows; ++i)
        {
            if (!tm || cpu.tensor_mask[i])
            {
                bool dirty = false;
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(cpu, idx, id);
                for (int r = 0; r < 2; ++r)
                {
                    try {
                        Packed<256> tmp;
                        uint64_t vaddr = sextVA(addr + boffset + (2*i+r)*stride);
                        assert(addr_is_size_aligned(vaddr, 32));
                        uint64_t paddr = mmu_translate(cpu, vaddr, 32, Mem_Access_TxLoad);
                        cpu.chip->memory.read(cpu, paddr, 32, tmp.u32.data());
                        LOG_MEMREAD256(paddr, tmp.u32);
                        for (int c = 0; c < 16; ++c)
                            SCP[idx].u16[c*2 + r] = tmp.u16[c];
                    }
                    catch (const sync_trap_t&) {
                        update_tensor_error(cpu, 1 << 7);
                        goto tensor_load_execute_done;
                    }
                    catch (const memory_error&) {
                        raise_bus_error_interrupt(cpu, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty)
                {
                    notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
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
        LOG_HART(DEBUG, cpu, "\tTensorLoad: Transpose - elements:%d size:%d offset:%d", elements, size, offset);
        for (int j = 0; j < elements; ++j)
        {
            uint64_t vaddr = sextVA(addr + j*stride);
            assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
            try {
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_TxLoad);
                cpu.chip->memory.read(cpu, paddr, L1D_LINE_SIZE, tmp[j].u32.data());
                LOG_MEMREAD512(paddr, tmp[j].u32);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(cpu, 1 << 7);
                goto tensor_load_execute_done;
            }
            catch (const memory_error&) {
                raise_bus_error_interrupt(cpu, 0);
                continue;
            }
            okay |= 1ull << j;
        }
        for (int i = 0; i < rows; ++i)
        {
            if (((okay >> i) & 1) && (!tm || cpu.tensor_mask[i]))
            {
                int idx = adj + ((dst + i) % L1_SCP_ENTRIES);
                L1_SCP_CHECK_FILL(cpu, idx, id);
                for (int j = 0; j < elements; ++j)
                {
                    switch (size) {
                    case 4: SCP[idx].u32[j] = tmp[j].u32[i+offset/4]; break;
                    case 2: SCP[idx].u16[j] = tmp[j].u16[i+offset/2]; break;
                    case 1: SCP[idx].u8[j] = tmp[j].u8[i+offset]; break;
                    }
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                LOG_SCP_32x16("=", idx);
            }
        }
    }

tensor_load_execute_done:
    cpu.txload[int(tenb)] = 0xFFFFFFFFFFFFFFFFULL;
#ifdef ZSIM
    if (cpu.shadow_txload[int(tenb)] != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu.txload[int(tenb)], cpu.shadow_txload[int(tenb)]);
        std::swap(cpu.txstride[int(tenb)], cpu.shadow_txstride[int(tenb)]);
    }
#endif
}


// ----- TensorLoadL2Scp emulation --------------------------------------------------

void tensor_load_l2_start(Hart& cpu, uint64_t control)
{
    uint64_t stride  = X31 & 0xFFFFFFFFFFC0ULL;
    uint32_t id      = X31 & 1ULL;

    int      tm      = (control >> 63) & 0x1;
    int      dst     = ((control >> 46) & 0x1FFFC)  + ((control >> 4)  & 0x3);
    uint64_t base    = control & 0xFFFFFFFFFFC0ULL;
    int      rows    = ((control     ) & 0xF) + 1;
    uint64_t addr    = sext<48>(base);

    LOG_REG(":", 31);
    LOG_HART(DEBUG, cpu, "TensorLoadL2SCP: rows:%d - tm:%d - dst:%d - addr:0x%" PRIx64 " - stride: 0x%" PRIx64 " - id: %d",
        rows, tm, dst, addr, stride, id);

    uint64_t shire = cpu.shireid();
    for (int i = 0; i < rows; ++i)
    {
        if (!tm || cpu.tensor_mask[i])
        {
            uint64_t l2scp_addr = L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * L1D_LINE_SIZE);
            uint64_t vaddr = sextVA(addr + i*stride);
            assert(addr_is_size_aligned(vaddr, L1D_LINE_SIZE));
            try {
                cache_line_t tmp;
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_TxLoadL2Scp);
                cpu.chip->memory.read(cpu, paddr, L1D_LINE_SIZE, tmp.u32.data());
                LOG_MEMREAD512(paddr, tmp.u32);
                cpu.chip->memory.write(cpu, l2scp_addr, L1D_LINE_SIZE, tmp.u32.data());
                LOG_MEMWRITE512(l2scp_addr, tmp.u32);
                L2_SCP_CHECK_FILL(cpu, dst + i, id, vaddr);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(cpu, 1 << 7);
                return;
            }
            catch (const memory_error&) {
                raise_bus_error_interrupt(cpu, 0);
            }
        }
    }
}


// ----- TensorQuant emulation -------------------------------------------------

void tensor_quant_start(Hart& cpu, uint64_t value)
{
    unsigned fstart = (value >> 57) & 0x1F;
    unsigned ncols  = (value >> 55) & 0x3;
    unsigned nrows  = (value >> 51) & 0xF;
    unsigned line   = (value >> 45) & 0x3F;

    ncols = (ncols + 1) * 4;
    nrows = nrows + 1;
    line = line % L1_SCP_ENTRIES;

    LOG_HART(DEBUG, cpu, "\tStart TensorQuant with line: %u, rows: %u, cols: %u, regstart: %u, frm: %s",
        line, nrows, ncols, fstart, get_rounding_mode(cpu, FRM));

#ifdef ZSIM
    bool txquant_busy = (cpu.core->txquant != 0xFFFFFFFFFFFFFFFFULL);
    if (txquant_busy) {
        if (cpu.core->shadow_txquant != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_quant_start() called while "
                                     "this thread's TensorQuant FSM is active");
    }
#else
    if (cpu.core->txquant != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_quant_start() called while "
                                 "this thread's TensorQuant FSM is active");
    }
#endif

    // TensorQuant raises illegal instruction exception when rounding mode is
    // invalid even if the transforms do not use FRM.
    set_rounding_mode(cpu, FRM);

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
            (cpu.core->mcache_control != 0x3)) {
            LOG_HART(DEBUG, cpu, "\tTransformation %d is %s but scratchpad is disabled",
                trans, get_quant_transform(funct));
            update_tensor_error(cpu, 1 << 4);
            // Error, don't activate the state machine
            return;
        }
    }

    // Activate the state machine
#ifdef ZSIM
    if (txquant_busy) {
        cpu.core->shadow_txquant = value;
    } else {
        cpu.core->txquant = value;
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::Quant);
        assert(enq_ret);
    }
#else
    cpu.core->txquant = value;
    tensor_quant_execute(cpu);
#endif
}


void tensor_quant_execute(Hart& cpu)
{
    uint64_t quant = cpu.core->txquant;
    unsigned fstart = (quant >> 57) & 0x1F;
    unsigned ncols  = (quant >> 55) & 0x3;
    unsigned nrows  = (quant >> 51) & 0xF;
    unsigned line   = (quant >> 45) & 0x3F;

#ifdef ZSIM
    if (cpu.core->active_tensor_op() != Core::Tensor::Quant) {
        throw std::runtime_error("tensor_quant_execute() called while this "
                                 "thread's TensorQuant FSM is inactive");
    }
#else
    if (cpu.core->txquant == 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_quant_execute() called while this "
                                 "thread's TensorQuant FSM is inactive");
    }
#endif

    ncols = (ncols + 1) * 4;
    nrows = nrows + 1;
    line = line % L1_SCP_ENTRIES;

    LOG_HART(DEBUG, cpu, "\tExecute TensorQuant with line: %u, rows: %u, cols: %u, regstart: %u, frm: %s",
        line, nrows, ncols, fstart, get_rounding_mode(cpu, FRM));

    set_rounding_mode(cpu, FRM);

    for (unsigned trans = 0; trans < TQUANT_MAX_TRANS; ++trans) {
        unsigned funct = (quant >> (trans*4)) & 0xF;
        LOG_HART(DEBUG, cpu, "\tTransformation %d: %s", trans, get_quant_transform(funct));
        if (!funct) {
            break;
        }
        // PACK_128B RTL operates on even registers first, and then on odd
        // registers, so it generates two writes to the destination register
        // when a row spans a vector.
        notify_tensor_quant_new_transform(cpu, (funct == 10) && (ncols > VLENW));

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
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case 2: // FP32_TO_INT32
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = fpu::f32_to_i32(FREGS[fd].f32[e]);
                    }
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
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
                    L1_SCP_CHECK_READ(cpu, line);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 5: // INT32_ADD_COL
                    LOG_SCP_32x1(":", line, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = FREGS[fd].i32[e] + SCP[line].i32[row];
                    }
                    L1_SCP_CHECK_READ(cpu, line);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case 6: // FP32_MUL_ROW
                    LOG_SCP(":", line, col);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e], SCP[line].f32[col+e]);
                    }
                    L1_SCP_CHECK_READ(cpu, line);
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case 7: // FP32_MUL_COL
                    LOG_SCP_32x1(":", line, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e], SCP[line].f32[row]);
                    }
                    L1_SCP_CHECK_READ(cpu, line);
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
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
                    notify_tensor_quant_write(cpu, trans, fd, mkmask(nelem/4) << (col/4), FREGS[fd]);
                } else {
                    notify_tensor_quant_write(cpu, trans, fd, mkmask(nelem), FREGS[fd]);
                }
            }
        }

        if ((funct >= 4) && (funct <= 7)) {
            line = (line + 1) % L1_SCP_ENTRIES;
        }
    }
    cpu.core->txquant = 0xFFFFFFFFFFFFFFFFULL;
#ifdef ZSIM
    auto deq_ret = cpu.core->dequeue_tensor_op();
    assert(deq_ret == Core::Tensor::Quant);
    if (cpu.core->shadow_txquant != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu.core->txquant, cpu.core->shadow_txquant);
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::Quant);
        assert(enq_ret);
    }
#endif
}


// ----- TensorStore emulation -------------------------------------------------

void tensor_store_start(Hart& cpu, uint64_t tstorereg)
{
    uint64_t tstore_scp = (tstorereg >> 48) & 0x1;

    if (tstore_scp)
    {
        int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to scratchpad source
        int      scpstart =  (tstorereg & 0x3F00000000000000ULL) >> 56;      // Start scratchpad entry to store
        int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
        uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFC0ULL);     // Address where to store the results
        int      src      = scpstart % L1_SCP_ENTRIES;

        uint64_t stride   = sext<48>(X31 & 0x0000FFFFFFFFFFC0ULL);

        LOG_REG(":", 31);
        LOG_HART(DEBUG, cpu, "\tStart TensorStoreFromScp with addr: %016" PRIx64 ", stride: %016" PRIx64 ", rows: %d, scpstart: %d, srcinc: %d", addr, stride, rows, src, srcinc);

        notify_tensor_store(cpu, true, rows, 4, 1);

        // Check if L1 SCP is enabled
        if (cpu.core->mcache_control != 0x3)
        {
            update_tensor_error(cpu, 1 << 4);
            notify_tensor_store_error(cpu, 1 << 4);
            return;
        }

        // For all the rows
        for (int row = 0; row < rows; row++)
        {
            assert(addr_is_size_aligned(addr, L1D_LINE_SIZE));
            LOG_SCP_32x16(":", src);
            try {
                uint64_t vaddr = sextVA(addr + row * stride);
                uint64_t paddr = mmu_translate(cpu, vaddr, L1D_LINE_SIZE, Mem_Access_TxStore);
                cpu.chip->memory.write(cpu, paddr, L1D_LINE_SIZE, SCP[src].u32.data());
                LOG_MEMWRITE512(paddr, SCP[src].u32);
                for (int col=0; col < 16; col++) {
                    notify_tensor_store_write(cpu, paddr + col*4, SCP[src].u32[col]);
                }
                L1_SCP_CHECK_READ(cpu, src);
            }
            catch (const sync_trap_t&) {
                update_tensor_error(cpu, 1 << 7);
                notify_tensor_store_error(cpu, 1 << 7);
                return;
            }
            catch (const memory_error&) {
                raise_bus_error_interrupt(cpu, 0);
            }
            src = (src + srcinc) % L1_SCP_ENTRIES;
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

        uint64_t stride   = sext<48>(X31 & 0x0000FFFFFFFFFFF0ULL);

        LOG_REG(":", 31);
        LOG_HART(DEBUG, cpu, "\tStart TensorStore with addr: %016" PRIx64 ", stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, srcinc: %d, coop: %d",
            addr, stride, regstart, rows, cols, srcinc, coop);

        // Check legal coop combination
        // xs[50:49]/xs[56:55]
        static const bool coop_comb[4*4] = {
            true,  true,  false, true,
            true,  true,  false, false,
            false, false, false, false,
            true,  false, false, false
        };

        notify_tensor_store(cpu, false, rows, cols, coop);

        if (!coop_comb[4*(coop-1)+(cols-1)])
        {
            update_tensor_error(cpu, 1 << 8);
            notify_tensor_store_error(cpu, 1 << 8);
            return;
        }

        // Cooperative tensor stores require the shire to be in cooperative mode
        if (coop > 1)
        {
            uint64_t shire = shire_index(cpu);
            if (!cpu.chip->shire_other_esrs[shire].shire_coop_mode)
                throw trap_illegal_instruction(cpu.inst.bits);
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
                    uint64_t paddr = mmu_translate(cpu, vaddr + col*16, 16, Mem_Access_TxStore);
                    if (!(col & 1)) LOG_FREG(":", src);
                    const uint32_t* ptr = &FREGS[src].u32[(col & 1) * 4];
                    cpu.chip->memory.write(cpu, paddr, 16, ptr);
                    LOG_MEMWRITE128(paddr, ptr);
                    for (int w=0; w < 4; w++) {
                        notify_tensor_store_write(cpu, paddr + w*4, *(ptr+w));
                    }
                }
                catch (const sync_trap_t&) {
                    update_tensor_error(cpu, 1 << 7);
                    notify_tensor_store_error(cpu, 1 << 7);
                    return;
                }
                catch (const memory_error&) {
                    raise_bus_error_interrupt(cpu, 0);
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

static void tensor_fma32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->txfma >> 63) & 0x1;
    int  bcols      = (cpu.core->txfma >> 55) & 0x3;
    int  arows      = (cpu.core->txfma >> 51) & 0xF;
    int  acols      = (cpu.core->txfma >> 47) & 0xF;
    int  aoffset    = (cpu.core->txfma >> 43) & 0xF;
    bool tenb       = (cpu.core->txfma >> 20) & 0x1;
    int  bstart     = (cpu.core->txfma >> 12) & 0x3F;
    int  astart     = (cpu.core->txfma >>  4) & 0x3F;
    bool first_pass = (cpu.core->txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        uint32_t thread = hart_index(cpu);
        bool resolved = SYS_EMU_PTR->coop_tload_check(thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu.wait.state = Hart::Wait::State::TxFMA;
            cpu.wait.value = 0; // Marks FMA32 for replay
            LOG_HART(DEBUG, cpu, "TensorFMA32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu.wait.state = Hart::Wait::State::Idle;
        }
    }
    else
    {
        cpu.wait.state = Hart::Wait::State::Idle;
    }
#endif

    LOG_HART(DEBUG, cpu, "\tExecute TensorFMA32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(cpu, FRM));
    LOG_TENSOR_MASK(":");

    set_rounding_mode(cpu, FRM);
    for (int k = 0; k < acols; ++k)
    {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k)%L1_SCP_ENTRIES));

        for (int i = 0; i < arows; ++i)
        {
            bool written[2] = { false, false };

            // Skip computation for this row
            if (usemsk && !cpu.tensor_mask[i])
            {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = 0;
                        notify_tensor_fma_write(cpu, 0, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, 0);
                        written[j/VLENW] = true;
                    }
                    if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
                    if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
                }
                continue;
            }

            uint32_t a_scp_entry = (astart+i) % L1_SCP_ENTRIES;
            float32_t a = SCP[a_scp_entry].f32[(aoffset+k) % (L1D_LINE_SIZE/4)];
            L1_SCP_CHECK_READ(cpu, a_scp_entry);
            LOG_SCP_32x1(":", a_scp_entry, ((aoffset+k) % (L1D_LINE_SIZE/4)));

            // If first_pass is 1 and this is the first iteration we do FMUL
            // instead of FMA
            if (first_pass && !k)
            {
                for (int j = 0; j < bcols; ++j)
                {
                    float32_t b = tmpb.f32[j];
                    float32_t c = fpu::f32_mul(a, b);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    notify_tensor_fma_write(cpu, k, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                    written[j/VLENW] = true;
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
                    notify_tensor_fma_write(cpu, k, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                    written[j/VLENW] = true;
                }
            }
            if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
            if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
        }
    }

    set_fp_exceptions(cpu);
    dirty_fp_state();
}


void tensor_fma32_start(Hart& cpu, uint64_t tfmareg)
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

    LOG_HART(DEBUG, cpu, "\tStart TensorFMA32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, get_rounding_mode(cpu, FRM));

#ifdef ZSIM
    bool txfma_busy = (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu.core->shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_fma32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Illegal instruction exception has higher priority than other errors
    set_rounding_mode(cpu, FRM);

    // Unpair the last TensorLoadSetupB
    bool load_tenb = cpu.core->tensorload_setupb_topair;
    int  brows_tenb = cpu.core->tensorload_setupb_numlines;
    cpu.core->tensorload_setupb_topair = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // rows/columns size, or not tenb and orphaned TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu.core->mcache_control != 3)
            update_tensor_error(cpu, (1 << 4) | (1 << 6));
        else
            update_tensor_error(cpu, 1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu.core->mcache_control != 3) {
        update_tensor_error(cpu, 1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu.core->shadow_txfma = tfmareg;
    } else {
        cpu.core->txfma = tfmareg;
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::FMA);
        assert(enq_ret);
    }
#elif SYS_EMU
    cpu.core->txfma = tfmareg;
    cpu.wait.state = Hart::Wait::State::TxFMA;
#else
    cpu.core->txfma = tfmareg;
    tensor_fma32_execute(cpu);
    cpu.core->txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}


static void tensor_fma16a32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->txfma >> 63) & 0x1;
    int  bcols      = (cpu.core->txfma >> 55) & 0x3;
    int  arows      = (cpu.core->txfma >> 51) & 0xF;
    int  acols      = (cpu.core->txfma >> 47) & 0xF;
    int  aoffset    = (cpu.core->txfma >> 43) & 0xF;
    bool tenb       = (cpu.core->txfma >> 20) & 0x1;
    int  bstart     = (cpu.core->txfma >> 12) & 0x3F;
    int  astart     = (cpu.core->txfma >>  4) & 0x3F;
    bool first_pass = (cpu.core->txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 2;
    aoffset = aoffset * 2;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        uint32_t thread = hart_index(cpu);
        bool resolved = SYS_EMU_PTR->coop_tload_check(thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu.wait.state = Hart::Wait::State::TxFMA;
            cpu.wait.value = 1; // Marks FMA16A32 for replay
            LOG_HART(DEBUG, cpu, "TensorFMA16A32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu.wait.state = Hart::Wait::State::Idle;
        }
    }
    else
    {
        cpu.wait.state = Hart::Wait::State::Idle;
    }
#endif

    LOG_HART(DEBUG, cpu, "\tExecute TensorFMA16A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, "rtz");

    LOG_TENSOR_MASK(":");

    set_rounding_mode(cpu, rtz);
    for (int k = 0; k < acols; k += 2)
    {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k/2)%L1_SCP_ENTRIES));

        for (int i = 0; i < arows; ++i)
        {
            bool written[2] = { false, false };

            // Skip computation for this row
            if (usemsk && !cpu.tensor_mask[i])
            {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = 0;
                        notify_tensor_fma_write(cpu, 0, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, 0);
                        written[j/VLENW] = true;
                    }
                    if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
                    if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
                }
                continue;
            }

            uint32_t a_scp_entry = (astart+i) % L1_SCP_ENTRIES;
            float16_t a1 = SCP[a_scp_entry].f16[(aoffset+k+0) % (L1D_LINE_SIZE/2)];
            float16_t a2 = SCP[a_scp_entry].f16[(aoffset+k+1) % (L1D_LINE_SIZE/2)];
            LOG_SCP_32x1(":", a_scp_entry, ((aoffset+k+0) % (L1D_LINE_SIZE/2)) / 2);
            L1_SCP_CHECK_READ(cpu, a_scp_entry);

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
                    notify_tensor_fma_write(cpu, k/2, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                    written[j/VLENW] = true;
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
                    notify_tensor_fma_write(cpu, k/2, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                    written[j/VLENW] = true;
                }
            }
            if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
            if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
        }
    }

    set_fp_exceptions(cpu);
    dirty_fp_state();
}


void tensor_fma16a32_start(Hart& cpu, uint64_t tfmareg)
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

    LOG_HART(DEBUG, cpu, "\tStart TensorFMA16A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, tenb: %d, bstart: %d, astart: %d, rm: %s",
        usemsk, aoffset, first_pass, bcols, acols, arows, tenb, bstart, astart, "rtz");

#ifdef ZSIM
    bool txfma_busy = (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu.core->shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_fma16a32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma16a32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Unpair the last TensorLoadSetupB
    bool load_tenb = cpu.core->tensorload_setupb_topair;
    int  brows_tenb = 2 * cpu.core->tensorload_setupb_numlines;
    cpu.core->tensorload_setupb_topair = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // combination of rows and columns length, or not tenb and orphaned
    // TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu.core->mcache_control != 3)
            update_tensor_error(cpu, (1 << 4) | (1 << 6));
        else
            update_tensor_error(cpu, 1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu.core->mcache_control != 3) {
        update_tensor_error(cpu, 1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu.core->shadow_txfma = tfmareg;
    } else {
        cpu.core->txfma = tfmareg;
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::FMA);
        assert(enq_ret);
    }
#elif SYS_EMU
    cpu.core->txfma = tfmareg;
    cpu.wait.state = Hart::Wait::State::TxFMA;
#else
    cpu.core->txfma = tfmareg;
    tensor_fma16a32_execute(cpu);
    cpu.core->txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}


static void tensor_ima8a32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->txfma >> 63) & 0x1;
    int  bcols      = (cpu.core->txfma >> 55) & 0x3;
    int  arows      = (cpu.core->txfma >> 51) & 0xF;
    int  acols      = (cpu.core->txfma >> 47) & 0xF;
    int  aoffset    = (cpu.core->txfma >> 43) & 0xF;
    bool tenc2rf    = (cpu.core->txfma >> 23) & 0x1;
    bool ub         = (cpu.core->txfma >> 22) & 0x1;
    bool ua         = (cpu.core->txfma >> 21) & 0x1;
    bool tenb       = (cpu.core->txfma >> 20) & 0x1;
    int  bstart     = (cpu.core->txfma >> 12) & 0x3F;
    int  astart     = (cpu.core->txfma >>  4) & 0x3F;
    bool first_pass = (cpu.core->txfma >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 4;
    aoffset = aoffset * 4;

#ifdef SYS_EMU
    if(tenb)
    {
        uint32_t requested_mask;
        uint32_t present_mask;
        uint32_t thread = hart_index(cpu);
        bool resolved = SYS_EMU_PTR->coop_tload_check(thread, true, 0, requested_mask, present_mask); // TENB
        // If not resolved, put the thread to sleep
        if(!resolved)
        {
            cpu.wait.state = Hart::Wait::State::TxFMA;
            cpu.wait.value = 3; // Marks IMA8A32 for replay
            LOG_HART(DEBUG, cpu, "TensorIMA8A32 TenB => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", requested_mask, present_mask);
            return;
        }
        else
        {
            cpu.wait.state = Hart::Wait::State::Idle;
        }
    }
    else
    {
        cpu.wait.state = Hart::Wait::State::Idle;
    }
#endif

#ifdef ZSIM
    LOG_HART(DEBUG, cpu, "\tExecute TensorIMA8A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc2rf: %d, tenb: %d, bstart: %d, astart: %d",
        usemsk, aoffset, first_pass, bcols, acols, arows, ub, ua, tenc2rf, tenb, bstart, astart);
#endif

    LOG_TENSOR_MASK(":");

    for (int k = 0; k < acols; k += 4)
    {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k/4)%L1_SCP_ENTRIES));

        bool write_freg = (tenc2rf && (k+4 == acols));
        freg_t* dst = write_freg ? FREGS.data() : TENC.data();

        for (int i = 0; i < arows; ++i)
        {
            bool written[2] = { false, false };

            // We should skip computation for this row, but:
            // * if first_pass is set and this is the first iteration then we still set TenC to 0
            // * if tenc2rf is set and we are in the last pass then we must copy TenC to FREGS even for this row.
            if (usemsk && !cpu.tensor_mask[i])
            {
                if (write_freg)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = (first_pass && !k) ? 0 : TENC[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW];
                        notify_tensor_fma_write(cpu, k/4, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW]);
                        written[j/VLENW] = true;
                    }
                    if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
                    if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
                }
                else if (first_pass && !k)
                {
                    for (int j = 0; j < bcols; ++j)
                    {
                        TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = 0;
                        notify_tensor_fma_write(cpu, 0, false, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                        written[j/VLENW] = true;
                    }
                    if (written[0]) LOG_CREG("=", i*TFMA_REGS_PER_ROW);
                    if (written[1]) LOG_CREG("=", i*TFMA_REGS_PER_ROW + 1);
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
                LOG_SCP_32x1(":", (astart+i) % L1_SCP_ENTRIES, ((aoffset+k) % L1D_LINE_SIZE) / 4);
                L1_SCP_CHECK_READ(cpu, (astart+i) % L1_SCP_ENTRIES);
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
                    notify_tensor_fma_write(cpu, k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, uint32_t(c));
                    written[j/VLENW] = true;
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
                    notify_tensor_fma_write(cpu, k/4, write_freg, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, uint32_t(c));
                    written[j/VLENW] = true;
                }
            }
            if (write_freg)
            {
                if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
                if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
            }
            else
            {
                if (written[0]) LOG_CREG("=", i*TFMA_REGS_PER_ROW);
                if (written[1]) LOG_CREG("=", i*TFMA_REGS_PER_ROW + 1);
            }
        }
    }
    if (tenc2rf)
        dirty_fp_state();
}


void tensor_ima8a32_start(Hart& cpu, uint64_t tfmareg)
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

    LOG_HART(DEBUG, cpu, "\tStart TensorIMA8A32 with tm: %d, aoffset: %d, first_pass: %d, bcols: %d, acols: %d, arows: %d, ub: %d, ua: %d, tenc2rf: %d, tenb: %d, bstart: %d, astart: %d",
        usemsk, aoffset, first_pass, bcols, acols, arows, ub, ua, tenc2rf, tenb, bstart, astart);

#ifdef ZSIM
    bool txfma_busy = (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL);
    if (txfma_busy) {
        if (cpu.core->shadow_txfma != 0xFFFFFFFFFFFFFFFFULL)
            throw std::runtime_error("tensor_ima8a32_start() called while "
                                     "this thread's TensorFMA FSM is active");
    }
#else
    if (cpu.core->txfma != 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_ima8a32_start() called while "
                                 "this thread's TensorFMA FSM is active");
    }
#endif

    // Unpair the last TensorLoadSetupB
    bool load_tenb = cpu.core->tensorload_setupb_topair;
    int  brows_tenb = 4 * cpu.core->tensorload_setupb_numlines;
    cpu.core->tensorload_setupb_topair = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // combination of rows and columns length, or not tenb and orphaned
    // TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        // If L1 SCP is disabled we should set multiple bits in tesnor_error
        if (cpu.core->mcache_control != 3)
            update_tensor_error(cpu, (1 << 4) | (1 << 6));
        else
            update_tensor_error(cpu, 1 << 6);
        return;
    }

    // Check if L1 SCP is enabled
    if (cpu.core->mcache_control != 3) {
        update_tensor_error(cpu, 1 << 4);
        return;
    }

#ifdef ZSIM
    if (txfma_busy) {
        cpu.core->shadow_txfma = tfmareg;
    } else {
        cpu.core->txfma = tfmareg;
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::FMA);
        assert(enq_ret);
    }
#elif SYS_EMU
    cpu.core->txfma = tfmareg;
    cpu.wait.state = Hart::Wait::State::TxFMA;
#else
    cpu.core->txfma = tfmareg;
    tensor_ima8a32_execute(cpu);
    cpu.core->txfma = 0xFFFFFFFFFFFFFFFFULL;
#endif
}


void tensor_fma_execute(Hart& cpu)
{
#ifdef ZSIM
    if (cpu.core->active_tensor_op() != Core::Tensor::FMA) {
        throw std::runtime_error("tensor_fma_execute() called while "
                                 "TensorFMA FSM is inactive");
    }
#else
    if (cpu.core->txfma == 0xFFFFFFFFFFFFFFFFULL) {
        throw std::runtime_error("tensor_fma_execute() called while "
                                 "this thread's TensorFMA FSM is inactive");
    }
#endif
    switch ((cpu.core->txfma >> 1) & 0x7)
    {
    case 0: tensor_fma32_execute(cpu); break;
    case 1: tensor_fma16a32_execute(cpu); break;
    case 3: tensor_ima8a32_execute(cpu); break;
    default: throw std::runtime_error("Illegal tensor_fma configuration");
    }
    if(cpu.wait.state != Hart::Wait::State::TxFMA) {
        cpu.core->txfma = 0xFFFFFFFFFFFFFFFFULL;
    }
#ifdef ZSIM
    auto deq_ret = cpu.core->dequeue_tensor_op();
    assert(deq_ret == Core::Tensor::FMA);
    if (cpu.core->shadow_txfma != 0xFFFFFFFFFFFFFFFFULL) {
        std::swap(cpu.core->txfma, cpu.core->shadow_txfma);
        auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::FMA);
        assert(enq_ret);
    }
#endif
}


// ----- TensorReduce emulation ------------------------------------------------

void tensor_reduce_start(Hart& cpu, uint64_t value)
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

    if ((cpu.core->reduce.state != Core::Reduce::State::Idle) &&
        (cpu.core->reduce.state != Core::Reduce::State::Skip))
        throw std::runtime_error("tensor_reduce_start() called while "
                                 "this thread's TensorReduce FSM is active");

    if ((type != 0) && (operation == 0)) {
        // TensorRecv, TensorBroadcast and TensorReduce should raise illegal
        // instruction if the encoded operation is FADD and FRM does not hold
        // a valid rounding mode
        set_rounding_mode(cpu, FRM);
    }

    cpu.core->reduce.regid  = (value >> 57) & 0x1F;
    cpu.core->reduce.count  = (value >> 16) & 0x7F;
    cpu.core->reduce.optype = operation | (type << 4);

    if (type == 0) {
        cpu.core->reduce.state = Core::Reduce::State::Send;
        cpu.core->reduce.thread = (value >> 2) & 0x3FFE;
    } else if (type == 1) {
        cpu.core->reduce.state = Core::Reduce::State::Recv;
        cpu.core->reduce.thread = (value >> 2) & 0x3FFE;
    } else if (type == 2) {
        // Broadcast: compute sender/receiver using recursive halving
        unsigned minion = core_index(cpu);
        if ((minion & minmask) == distance) {
            cpu.core->reduce.state = Core::Reduce::State::Recv;
            cpu.core->reduce.thread = EMU_THREADS_PER_MINION * (minion - distance);
        } else if ((minion & minmask) == 0) {
            cpu.core->reduce.state = Core::Reduce::State::Send;
            cpu.core->reduce.thread = EMU_THREADS_PER_MINION * (minion + distance);
        } else {
            cpu.core->reduce.state = Core::Reduce::State::Skip;
        }
    } else {
        // Reduce: compute sender/receiver using recursive halving
        unsigned minion = core_index(cpu);
        if ((minion & minmask) == distance) {
            cpu.core->reduce.state = Core::Reduce::State::Send;
            cpu.core->reduce.thread = EMU_THREADS_PER_MINION * (minion - distance);
        } else if ((minion & minmask) == 0) {
            cpu.core->reduce.state = Core::Reduce::State::Recv;
            cpu.core->reduce.thread = EMU_THREADS_PER_MINION * (minion + distance);
        } else {
            cpu.core->reduce.state = Core::Reduce::State::Skip;
        }
    }

    if (cpu.core->reduce.state == Core::Reduce::State::Skip) {
        LOG_HART(DEBUG, cpu, "\t%s(skip) with level: %u, distance: %u, minmask: 0x%08u",
            reducecmd[type], level, distance, minmask);
        return;
    }

    // Sending and receiving from the same minion should fail immediately
    if (cpu.core->reduce.thread == cpu.mhartid) {
        cpu.core->reduce.state = Core::Reduce::State::Skip;
        LOG_HART(DEBUG, cpu, "\t%s(error) other_thread: %u, start_reg: %u, num_reg: %u", reducecmd[type],
            cpu.mhartid, cpu.core->reduce.regid, cpu.core->reduce.count);
        update_tensor_error(cpu, 1 << 9);
        return;
    }

    // Illegal operation on a receiving minion should fail immediately
    if ((cpu.core->reduce.state == Core::Reduce::State::Recv) &&
        (operation == 1 || operation == 5 || operation > 8))
    {
        cpu.core->reduce.state = Core::Reduce::State::Skip;
        LOG_HART(DEBUG, cpu, "\t%s(error) illegal operation: %u", reducecmd[type], operation);
        update_tensor_error(cpu, 1 << 9);
        return;
    }

    // Sending or receiving 0 registers means do nothing
    // NB: This check has lower priority than other errors because
    // tensor_error[9] should be set even when "count" == 0".
    if (cpu.core->reduce.count == 0) {
        cpu.core->reduce.state = Core::Reduce::State::Skip;
        LOG_HART(DEBUG, cpu, "\t%s(skip) num_reg: 0", reducecmd[type]);
        return;
    }

    auto enq_ret = cpu.core->enqueue_tensor_op(Core::Tensor::Reduce);
    assert(enq_ret);
    (void) enq_ret;

    notify_tensor_reduce(cpu, cpu.core->reduce.state == Core::Reduce::State::Recv,
                      cpu.core->reduce.regid, cpu.core->reduce.count);

#if !defined(SYS_EMU) && !defined(ZSIM)
    tensor_reduce_execute(cpu);
#endif
}


void tensor_reduce_step(Hart& rcv_cpu, Hart& snd_cpu)
{
    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    Core::Reduce& send = snd_cpu.core->reduce;
    Core::Reduce& recv = rcv_cpu.core->reduce;

    unsigned type = (recv.optype >> 4);

    if (send.count-- == 0) {
        throw std::runtime_error("Tensor reduce sender register count is 0");
    }
    if (!send.count) {
        auto deq_ret = snd_cpu.core->dequeue_tensor_op();
        assert(deq_ret == Core::Tensor::Reduce);
        (void) deq_ret;
        snd_cpu.core->reduce.state = Core::Reduce::State::Idle;
    }
    if (recv.count-- == 0) {
        LOG_HART(WARN, rcv_cpu, "%s", "Mismatched tensor reduce register count");
        send.regid = (send.regid + 1) % NFREGS;
        recv.count = 0;
        return;
    }
    if (!recv.count) {
        auto deq_ret = rcv_cpu.core->dequeue_tensor_op();
        assert(deq_ret == Core::Tensor::Reduce);
        (void) deq_ret;
        rcv_cpu.core->reduce.state = Core::Reduce::State::Idle;
    }

    switch (recv.optype & 0xF) {
    case 0: // fadd
        set_rounding_mode(rcv_cpu, rcv_cpu.frm());
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=fadd sender=H%u rounding_mode=%s", reducecmd[type], snd_cpu.mhartid, get_rounding_mode(rcv_cpu, rcv_cpu.frm()));
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].f32[j] = fpu::f32_add(snd_cpu.fregs[send.regid].f32[j],
                                                            rcv_cpu.fregs[recv.regid].f32[j]);
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        set_fp_exceptions(rcv_cpu);
        break;
    case 2: // fmax
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=fmax sender=H%u", reducecmd[type], snd_cpu.mhartid);
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].f32[j] = fpu::f32_maximumNumber(snd_cpu.fregs[send.regid].f32[j],
                                                                      rcv_cpu.fregs[recv.regid].f32[j]);
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        set_fp_exceptions(rcv_cpu);
        break;
    case 3: // fmin
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=fmin sender=H%u", reducecmd[type], snd_cpu.mhartid);
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].f32[j] = fpu::f32_minimumNumber(snd_cpu.fregs[send.regid].f32[j],
                                                                      rcv_cpu.fregs[recv.regid].f32[j]);
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        set_fp_exceptions(rcv_cpu);
        break;
    case 4: // iadd
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=iadd sender=H%u", reducecmd[type], snd_cpu.mhartid);
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].u32[j] = snd_cpu.fregs[send.regid].u32[j]
                                             + rcv_cpu.fregs[recv.regid].u32[j];
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        break;
    case 6: // imax
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=imax sender=H%u", reducecmd[type], snd_cpu.mhartid);
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].i32[j] = std::max(snd_cpu.fregs[send.regid].i32[j],
                                                        rcv_cpu.fregs[recv.regid].i32[j]);
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        break;
    case 7: // imin
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=imin sender=H%u", reducecmd[type], snd_cpu.mhartid);
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) :", recv.regid);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.regid].i32[j] = std::min(snd_cpu.fregs[send.regid].i32[j],
                                                        rcv_cpu.fregs[recv.regid].i32[j]);
        }
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        break;
    case 8: // fget
        LOG_HART(DEBUG, rcv_cpu, "\t%s(recv) op=fget sender=H%u", reducecmd[type], snd_cpu.mhartid);
        rcv_cpu.fregs[recv.regid] = snd_cpu.fregs[send.regid];
        LOG_FREG_HART(snd_cpu, "(othr) :", send.regid);
        LOG_FREG_HART(rcv_cpu, "(this) =", recv.regid);
        break;
    default:
        throw std::runtime_error("TensorReduce with illegal operation code!");
    }
    dirty_fp_state();
    notify_tensor_reduce_write(rcv_cpu, recv.regid, rcv_cpu.fregs[recv.regid]);

    send.regid = (send.regid + 1) % NFREGS;
    recv.regid = (recv.regid + 1) % NFREGS;
}


void tensor_reduce_execute(Hart& this_cpu)
{
    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    // Get information from receiver
    unsigned other_thread   = this_cpu.core->reduce.thread;
    unsigned this_start_reg = this_cpu.core->reduce.regid;
    unsigned this_num_reg   = this_cpu.core->reduce.count;
    unsigned type           = this_cpu.core->reduce.optype >> 4;

    Hart& other_cpu = this_cpu.chip->cpu[other_thread];

    if (this_cpu.core->reduce.state == Core::Reduce::State::Skip) {
        return;
    }
    if (this_cpu.core->active_tensor_op() != Core::Tensor::Reduce) {
        throw std::runtime_error("tensor_reduce_execute() called while "
                                 "this thread's TensorReduce FSM is inactive");
    }
#ifdef SYS_EMU
    if (other_cpu.core->active_tensor_op() != Core::Tensor::Reduce) {
        throw std::runtime_error("tensor_reduce_execute() called while "
                                 "the other thread's TensorReduce FSM is inactive");
    }
#endif

    if (this_cpu.core->reduce.state != Core::Reduce::State::Recv) {
#ifndef SYS_EMU
        if (this_cpu.core->reduce.state == Core::Reduce::State::Send) {
            LOG_HART(DEBUG, this_cpu, "\t%s(send) receiver=H%u, start_reg=%u, num_reg=%u", reducecmd[type], other_thread, this_start_reg, this_num_reg);
            for (unsigned i = 0; i < this_num_reg; ++i) {
                unsigned this_op_reg = (i + this_start_reg) % NFREGS;
                LOG_FREG_HART(this_cpu, "(this) :", this_op_reg);
            }
        }
#endif
        if (this_cpu.core->reduce.state == Core::Reduce::State::Skip)
            this_cpu.core->reduce.state = Core::Reduce::State::Idle;
        return;
    }

    // Get information from sender
    unsigned this_thread     = other_cpu.core->reduce.thread;
    unsigned other_start_reg = other_cpu.core->reduce.regid;
    unsigned other_num_reg   = other_cpu.core->reduce.count;

    if (this_thread != this_cpu.mhartid) {
        LOG_HART(WARN, this_cpu, "\t%s(recv) sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, get_reduce_state(other_cpu.core->reduce.state));
        throw std::runtime_error("Mismatched tensor reduce sender target minion");
    }
    if (other_cpu.core->reduce.state != Core::Reduce::State::Send) {
        LOG_HART(WARN, this_cpu, "\t%s(recv) with sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, get_reduce_state(other_cpu.core->reduce.state));
        throw std::runtime_error("Mismatched tensor reduce sender state");
    }
    if (other_num_reg != this_num_reg) {
        LOG_HART(WARN, this_cpu, "\t%s(recv) with sender=H%u receiver=H%u sender_start_reg=%u sender_num_reg=%u receiver_start_reg=%u receiver_num_reg=%u sender_state=%s",
            reducecmd[type], other_thread, this_thread, other_start_reg, other_num_reg, this_start_reg, this_num_reg, get_reduce_state(other_cpu.core->reduce.state));
    }

    unsigned count = std::min(this_num_reg, other_num_reg);
    while (count--)
        tensor_reduce_step(this_cpu, other_cpu);
}


// ----- TensorWait emulation ------------------------------------------------

// Starts a tensor wait, checks for stall conditions
void tensor_wait_start(Hart& cpu, uint64_t value)
{
    value = value & 0xF;
    cpu.wait.id = value;
    cpu.wait.value = value;
    cpu.wait.state = Hart::Wait::State::WaitReady;
#ifdef SYS_EMU
    // TensorLoad
    if (value < 2) {
        uint64_t id = value & 0x1;
        uint32_t requested_mask;
        uint32_t present_mask;
        uint32_t thread = hart_index(cpu);
        bool resolved = SYS_EMU_PTR->coop_tload_check(thread, false, id, requested_mask, present_mask);
        // If not resolved, put the thread to sleep
        if (!resolved) {
            cpu.wait.state = Hart::Wait::State::Wait;
            LOG_HART(DEBUG, cpu, "TensorWait with id %i not ready => minion cooperative mask: 0x%08X, minion ready mask: 0x%08x", (int) id, requested_mask, present_mask);
        }
    }
#if 0
    // TensorLoad L2
    else if(value < 4) {
    }
    // Prefetch
    else if(value < 6) {
    }
    // CacheOp
    else if(value == 6) {
    }
    // TensorFMA
    else if(value == 7) {
    }
    // TensorStore
    else if(value == 8) {
    }
    // TensorReduce
    else if(value == 9) {
    }
    // TensorQuant
    else if(value == 10) {
    }
#endif
#endif

    // Execute tensorwait right away for non sys_emu envs
#ifndef SYS_EMU
    tensor_wait_execute(cpu);
#endif
}


// Actual execution of tensor wait
void tensor_wait_execute(Hart& cpu)
{
#ifdef SYS_EMU
    if(SYS_EMU_PTR->get_l1_scp_check())
    {
        uint32_t thread = hart_index(cpu);
        // TensorWait TLoad Id0
        if     ((cpu.wait.id) == 0) { SYS_EMU_PTR->get_l1_scp_checker().l1_scp_wait(thread, 0); }
        else if((cpu.wait.id) == 1) { SYS_EMU_PTR->get_l1_scp_checker().l1_scp_wait(thread, 1); }
    }
    if(SYS_EMU_PTR->get_l2_scp_check())
    {
        uint32_t thread = hart_index(cpu);
        if((cpu.wait.id) == 2) { SYS_EMU_PTR->get_l2_scp_checker().l2_scp_wait(thread, 0); }
        else if((cpu.wait.id) == 3) { SYS_EMU_PTR->get_l2_scp_checker().l2_scp_wait(thread, 1); }
    }
#endif
    cpu.wait.state = Hart::Wait::State::Idle;
    notify_tensor_error_value(cpu, cpu.tensor_error);
}


} // namespace bemu
