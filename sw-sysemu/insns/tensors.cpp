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
    #define L1_SCP_CHECK_START(cpu, op, fp) do { \
        if (SYS_EMU_PTR->get_l1_scp_check()) { \
            SYS_EMU_PTR->get_l1_scp_checker().tensor_op_start(hart_index(cpu), op, fp); \
        } \
    } while (0)
    #define L1_SCP_CHECK_FILL(cpu, idx, id) do { \
        if (SYS_EMU_PTR->get_l1_scp_check()) { \
            SYS_EMU_PTR->get_l1_scp_checker().l1_scp_fill(hart_index(cpu), idx, id); \
        } \
    } while (0)
    #define L1_SCP_CHECK_READ(cpu, idx, op) do { \
        if (SYS_EMU_PTR->get_l1_scp_check()) { \
            SYS_EMU_PTR->get_l1_scp_checker().l1_scp_read(hart_index(cpu), idx, op); \
        } \
    } while (0)
    #define L2_SCP_CHECK_FILL(cpu, idx, id, addr) do { \
        if (SYS_EMU_PTR->get_l2_scp_check()) { \
            SYS_EMU_PTR->get_l2_scp_checker().l2_scp_fill(hart_index(cpu), idx, id, addr); \
        } \
    } while (0)
#else
    #define L1_SCP_CHECK_START(cpu, op, fp)       do { } while (0)
    #define L1_SCP_CHECK_FILL(cpu, idx, id)       do { (void)id; } while (0)
    #define L1_SCP_CHECK_READ(cpu, idx, op)       do { } while (0)
    #define L2_SCP_CHECK_FILL(cpu, idx, id, addr) do { } while (0)
#endif


namespace bemu {


// Tensor reduce constants
enum : uint8_t {
    reduce_function_fadd            = 0,
    reduce_function_reserved_1      = 1,
    reduce_function_fmax            = 2,
    reduce_function_fmin            = 3,
    reduce_function_add             = 4,
    reduce_function_reserved_5      = 5,
    reduce_function_max             = 6,
    reduce_function_min             = 7,
    reduce_function_move            = 8,
    reduce_function_reserved_9      = 9,
    reduce_function_reserved_10     = 10,
    reduce_function_reserved_11     = 11,
    reduce_function_reserved_12     = 12,
    reduce_function_reserved_13     = 13,
    reduce_function_reserved_14     = 14,
    reduce_function_reserved_15     = 15,
};


// Tensor load constants
enum {
    tload_cmd_load          = 0,
    tload_cmd_interleave8   = 1,
    tload_cmd_interleave16  = 2,
    tload_cmd_reserved_3    = 3,
    tload_cmd_reserved_4    = 4,
    tload_cmd_transpose8    = 5,
    tload_cmd_transpose16   = 6,
    tload_cmd_transpose32   = 7,
};


// Tensor fma constants
enum {
    tfma_type_fp32   = 0,
    tfma_type_fp16   = 1,
    tfma_type_rsvd_2 = 2,
    tfma_type_int8   = 3,
    tfma_type_rsvd_4 = 4,
    tfma_type_rsvd_5 = 5,
    tfma_type_rsvd_6 = 6,
    tfma_type_rsvd_7 = 7,
};


// Tensor quant constants
enum {
    tquant_funct_last          = 0,
    tquant_funct_int32_to_fp32 = 1,
    tquant_funct_fp32_to_int32 = 2,
    tquant_funct_int32_relu    = 3,
    tquant_funct_int32_add_row = 4,
    tquant_funct_int32_add_col = 5,
    tquant_funct_fp32_mul_row  = 6,
    tquant_funct_fp32_mul_col  = 7,
    tquant_funct_sat_int8      = 8,
    tquant_funct_sat_uint8     = 9,
    tquant_funct_pack_128      = 10,
    tquant_funct_reserved_11   = 11,
    tquant_funct_reserved_12   = 12,
    tquant_funct_reserved_13   = 13,
    tquant_funct_reserved_14   = 14,
    tquant_funct_reserved_15   = 15,
};


static const char* get_rounding_mode(const Hart& cpu, int mode)
{
    static const char* rmnames[] = {
        "rne",      "rtz",       "rdn",       "rup",
        "rmm",      "res5",      "res6",      "dyn",
        "dyn(rne)", "dyn(rtz)",  "dyn(rdn)",  "dyn(rup)",
        "dyn(rmm)", "dyn(res5)", "dyn(res6)", "dyn(res7)",
    };

    return rmnames[(mode == 7) ? (8 + cpu.frm()) : (mode & 7)];
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

    for (int i = 0; i < 16; ++i) {
        unsigned bit = !!((rowstart >= 0) && (rowstart < nrow)
                          && (colstart >= 0) && (colstart < ncol));
        tmask_value |= (bit << i);
        rowstart += srow;
        colstart += scol;
    }
    cpu.tensor_mask = tmask_value;
    LOG_TENSOR_MASK("=");
}


// ----- TensorLoad emulation --------------------------------------------------

#ifdef SYS_EMU
static int coop_tload_leader_neigh(uint64_t paddr, uint8_t neighmask)
{
    switch ((paddr >> 6) & 0x3) {
    case 0:
        if (neighmask & 1) return 0;
        if (neighmask & 8) return 3;
        if (neighmask & 2) return 1;
        return 2;
    case 1:
        if (neighmask & 2) return 1;
        if (neighmask & 1) return 0;
        if (neighmask & 4) return 2;
        return 3;
    case 2:
        if (neighmask & 4) return 2;
        if (neighmask & 2) return 1;
        if (neighmask & 8) return 3;
        return 0;
    case 3:
        if (neighmask & 8) return 3;
        if (neighmask & 4) return 2;
        if (neighmask & 1) return 0;
        return 1;
    }
    return -1;
}
#endif


#ifdef SYS_EMU
static Coop_minion_mask coop_tload_minion_mask(uint32_t tcoop)
{
    uint8_t          neighs_in_shire  = (tcoop >> 16) & 0xF;
    Coop_minion_mask minions_in_neigh = (tcoop >>  8) & 0xFF;
    Coop_minion_mask minions_in_shire = 0;

    while (neighs_in_shire) {
        if ((neighs_in_shire & 1) != 0) {
            minions_in_shire |= minions_in_neigh;
        }
        minions_in_neigh <<= EMU_MINIONS_PER_NEIGH;
        neighs_in_shire >>= 1;
    }

    return minions_in_shire;
}
#endif


#ifdef SYS_EMU
static TLoad& coop_tload_find_partner(Hart& hart, const TLoad& tload)
{
    const bool tenb = (tload.value >> 52) & 0x1;
    const int  id   = tload.stride & 0x1;
    auto& other = tenb ? hart.core->tload_b : hart.core->tload_a[id];
    assert(other.state == TLoad::State::waiting_coop);
    if (tload.tcoop != other.tcoop) {
        LOG_HART(ERR, hart, "coop tload: tensor_coop does not match: expected 0x%08x, found 0x%08x", tload.tcoop, other.tcoop);
    }
    if (tload.value != other.value) {
        WARN_HART(tensors, hart, "coop tload: CSR does not match: expected 0x%016lx, found 0x%016lx", tload.value, other.value);
    }
    if (tload.stride != other.stride) {
        WARN_HART(tensors, hart, "coop tload: x31 does not match: expected 0x%016lx, found 0x%016lx", tload.stride, other.stride);
    }
    return other;
}
#endif


void tensor_load_start(Hart& cpu, uint64_t control)
{
    uint64_t stride  = X31 & 0xFFFFFFFFFFC0ULL;
    int      id      = X31 & 1;

    bool     msk     = (control >> 63) & 0x1;
    bool     coop    = (control >> 62) & 0x1;
    int      cmd     = (control >> 59) & 0x7;
    int      start   = (control >> 53) & 0x3F;
    int      tenb    = (control >> 52) & 0x1;
    uint64_t addr    = sext<48>(control & 0xFFFFFFFFFFC0ULL);
    unsigned boffset = (control >>  4) & 0x03;
    int      rows    = ((control     ) & 0xF) + 1;

    if (tenb != 0) {
        start = 0;
        msk   = false;
        cmd   = tload_cmd_load;
    }

    TLoad& tload = tenb ? cpu.core->tload_b : cpu.core->tload_a[id];

    if (tload.state != TLoad::State::idle) {
        // tload[0] should wait for previous tload[0] to finish, but
        // tload[1] should wait only if previous tload[1] is paired, otherwise
        // it can proceed (by canceling the previous tload[1]).
        if (!tenb || tload.paired) {
            if (tenb) {
                cpu.start_waiting(Hart::Waiting::tload_tenb);
            } else if (id == 0) {
                cpu.start_waiting(Hart::Waiting::tload_0);
            } else {
                cpu.start_waiting(Hart::Waiting::tload_1);
            }
            cpu.npc = cpu.pc;
            throw instruction_restart();
        }
        if (tload.state == TLoad::State::waiting_coop) {
            // back-to-back loads to tenb are fine, but not if the first one
            // is cooperative!
            throw std::runtime_error("tensor_load_start() called while there "
                                     "is an active cooperative tensor load "
                                     "to tenb");
        }
        tload.clear();
    }

    // Cooperative tensor loads require the shire to be in cooperative mode
    if (coop) {
        auto shire = shire_index(cpu);
        if (!cpu.chip->shire_other_esrs[shire].shire_coop_mode) {
            throw trap_illegal_instruction(cpu.inst.bits);
        }
    }

    // Check if L1SCP is enabled
    if (cpu.core->mcache_control != 0x3) {
        WARN_HART(tensors, cpu, "%s", "\tTensorLoad with L1SCP disabled!!");
        update_tensor_error(cpu, 1 << 4);
        return;
    }

    // Check for invalid transformation
    if ((cmd == tload_cmd_reserved_3) || (cmd == tload_cmd_reserved_4)) {
        WARN_HART(tensors, cpu, "%s", "\tTensorLoad with illegal transform!!");
        update_tensor_error(cpu, 1 << 1);
        return;
    }

    LOG_REG(":", 31);

    const auto uuid = (tload.uuid = ++(cpu.core->tensor_uuid));
    LOG_HART(DEBUG, cpu, "\t(TL-H%u-%lu) Start TensorLoad with msk: %d, coop: %d, cmd: %d, "
             "start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: %u, rows: %d, "
             "stride: 0x%" PRIx64 ", id: %d", cpu.mhartid, uuid, int(msk), int(coop),
             cmd, start, tenb, addr, boffset, rows, stride, id);

    tload.value  = control;
    tload.stride = X31;
    tload.paired = false;

    if (msk) {
        LOG_TENSOR_MASK(":");
        tload.tmask = cpu.tensor_mask;
    } else {
        tload.tmask = 0xffff;
    }

    if (coop) {
        LOG_TENSOR_COOP(":");
        tload.tcoop = cpu.tensor_coop;
        tload.state = TLoad::State::waiting_coop;
    } else {
        tload.tcoop = 0;
        tload.state = TLoad::State::ready;
    }

#if defined(SYS_EMU)
    // Update cooperative state
    if (coop) {
        unsigned neigh0 = shire_index(cpu) * EMU_NEIGH_PER_SHIRE;

        unsigned neighs = (tload.tcoop >> 16) & 0xF;
        unsigned group  = tload.tcoop % 32;
        unsigned leader = coop_tload_leader_neigh(addr, neighs);
        auto     all    = coop_tload_minion_mask(tload.tcoop);

        unsigned minion = core_index(cpu) % EMU_MINIONS_PER_SHIRE;
        auto     pending = all;

        // Install an entry in every cooperating neighborhood
        pending[minion] = false;

        for (unsigned n = 0; n < EMU_NEIGH_PER_SHIRE; ++n) {
            if (((neighs >> n) & 1) == 0) {
                continue;
            }
            auto& coop_tloads = cpu.chip->coop_tloads[neigh0 + n];
            auto& entry = tenb ? coop_tloads.tload_b[group] : coop_tloads.tload_a[id][group];
            if (entry.all.none()) {
                entry.all = entry.pending = all;
            }
            if (entry.all != all) {
                throw std::runtime_error("tensor_load_start() with coop group "
                                         "already in use");
            }
            if (n == leader) {
                entry.pending &= pending;
                pending = entry.pending;
            }
        }

        LOG_HART(DEBUG, cpu,
                 "\tLeader-neigh: %d, minion-mask=0x%08lx, pending-mask=0x%08lx",
                 leader, all.to_ulong(), pending.to_ulong());

        if (pending.none()) {
            // Clear every cooperating neighborhood entry
            for (unsigned n = 0; n < EMU_NEIGH_PER_SHIRE; ++n) {
                if (((neighs >> n) & 1) == 1) {
                    auto& coop_tloads = cpu.chip->coop_tloads[neigh0 + n];
                    auto& entry = tenb ? coop_tloads.tload_b[group] : coop_tloads.tload_a[id][group];
                    entry.all.reset();
                }
            }

            // Wake-up cooperating harts
            unsigned hart0 = shire_index(cpu) * EMU_THREADS_PER_SHIRE;
            for (unsigned m = 0; m < EMU_MINIONS_PER_SHIRE; ++m) {
                if (all[m] == false) {
                    continue;
                }
                Hart& hart = cpu.chip->cpu[hart0 + m * EMU_THREADS_PER_MINION];
                auto& other_tload = coop_tload_find_partner(hart, tload);
                other_tload.state = TLoad::State::ready;
            }
        }
    } else {
        tensor_load_execute(cpu, id, tenb);
    }
#elif !defined(ZSIM)
    // CoSim lets the DUT handle synchronization; for now stay of the way
    tload.state = TLoad::State::ready;
    tensor_load_execute(cpu, id, tenb);
#endif
}


void tensor_load_execute(Hart& cpu, int tlid, bool tenb)
{
    assert(tlid == 0 || tlid == 1);
    TLoad& tload = tenb ? cpu.core->tload_b : cpu.core->tload_a[tlid];

    if (tload.state != TLoad::State::ready) {
        throw std::runtime_error("tensor_load_execute() called while "
                                 "this thread's TensorLoad FSM is inactive");
    }

    assert(int(tenb) == int((tload.value >> 52) & 0x1));

    uint64_t stride  = tload.stride & 0xFFFFFFFFFFC0ULL;
    int      id      = tload.stride & 1;
    bool     msk     = (tload.value >> 63) & 0x1;
    bool     coop    = (tload.value >> 62) & 0x1;
    int      cmd     = (tload.value >> 59) & 0x7;
    int      start   = (tload.value >> 53) & 0x3F;
    uint64_t addr    = sext<48>(tload.value & 0xFFFFFFFFFFC0ULL);
    unsigned boffset = (tload.value >>  4) & 0x03;
    int      rows    = ((tload.value     ) & 0xF) + 1;
    int      adj     = 0;

    assert(tenb || tlid == id);

    if (tenb) {
        // TenB is modelled as an extension to the SCP (these entries are not
        // accessible otherwise)
        start = 0;
        adj   = L1_SCP_ENTRIES;
        cmd   = tload_cmd_load;
        msk   = false;

        // Tensor load to TenB stays in 'loading' state until the paired
        // (future) txfma executes. Here we must signal said txfma that we are
        // ready so it can become ready too.
        tload.state = TLoad::State::loading;
        if (tload.paired && cpu.core->tmul.state == TMul::State::waiting_tenb) {
            cpu.core->tmul.state = TMul::State::ready;
        }
    } else {
        assert(tload.paired == false);
        tload.clear();
    }

    notify_tensor_load(cpu, cmd, tenb, adj + (start % L1_SCP_ENTRIES),
                       tload.tmask.to_ulong());

    std::array<cache_line_t, L1D_LINE_SIZE> tmp;
    std::bitset<L1D_LINE_SIZE>              okay;

    switch (cmd) {
    case tload_cmd_load:
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoad with msk: %d, coop: %d, "
                 "start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: %u, "
                 "rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        for (int i = 0; i < rows; ++i) {
            if (!msk || tload.tmask[i]) {
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                try {
                    mmu_tensor_load512(cpu, addr + i*stride, SCP[idx].u32.data(), Mem_Access_TxLoad);
                    LOG_SCP_32x16("=", idx);
                    L1_SCP_CHECK_FILL(cpu, idx, id);
                }
                catch (const Exception&) {
                    update_tensor_error(cpu, 1 << 7);
                    goto tload_exit;
                }
                catch (const memory_error&) {
                    cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                    continue;
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
            }
        }
        break;
    case tload_cmd_interleave8:
        boffset *= 16;
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoadInterleave8 with msk: %d, "
                 "coop: %d, start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: "
                 "%u, rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        for (int i = 0; i < rows; ++i) {
            if (!msk || tload.tmask[i]) {
                bool dirty = false;
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                for (int r = 0; r < 4; ++r) {
                    try {
                        Packed<128> tmp;
                        mmu_tensor_load128(cpu, addr + boffset + (4*i+r)*stride, tmp.u32.data(), Mem_Access_TxLoad);
                        for (int c = 0; c < 16; ++c) {
                            SCP[idx].u8[c*4 + r] = tmp.u8[c];
                        }
                    }
                    catch (const Exception&) {
                        update_tensor_error(cpu, 1 << 7);
                        goto tload_exit;
                    }
                    catch (const memory_error&) {
                        cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty) {
                    notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                    LOG_SCP_32x16("=", idx);
                }
                L1_SCP_CHECK_FILL(cpu, idx, id);
            }
        }
        break;
    case tload_cmd_interleave16:
        boffset = (boffset & 0x2) * 16;
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoadInterleave16 with msk: %d, "
                 "coop: %d, start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: "
                 "%u, rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        for (int i = 0; i < rows; ++i) {
            if (!msk || tload.tmask[i]) {
                bool dirty = false;
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                for (int r = 0; r < 2; ++r) {
                    try {
                        Packed<256> tmp;
                        mmu_tensor_load256(cpu, addr + boffset + (2*i+r)*stride, tmp.u32.data(), Mem_Access_TxLoad);
                        for (int c = 0; c < 16; ++c) {
                            SCP[idx].u16[c*2 + r] = tmp.u16[c];
                        }
                    }
                    catch (const Exception&) {
                        update_tensor_error(cpu, 1 << 7);
                        goto tload_exit;
                    }
                    catch (const memory_error&) {
                        cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                        continue;
                    }
                    dirty = true;
                }
                if (dirty) {
                    notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                    LOG_SCP_32x16("=", idx);
                }
                L1_SCP_CHECK_FILL(cpu, idx, id);
            }
        }
        break;
    case tload_cmd_transpose8:
        boffset *= 16;
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoadTranspose8 with msk: %d, "
                 "coop: %d, start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: "
                 "%u, rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        okay.reset();
        for (int j = 0; j < L1D_LINE_SIZE; ++j) {
            try {
                mmu_tensor_load512(cpu, addr + j*stride, tmp[j].u32.data(), Mem_Access_TxLoad);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                goto tload_exit;
            }
            catch (const memory_error&) {
                cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                continue;
            }
            okay[j] = true;
        }
        for (int i = 0; i < rows; ++i) {
            if (okay[i] && (!msk || tload.tmask[i])) {
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                for (int j = 0; j < L1D_LINE_SIZE; ++j) {
                    SCP[idx].u8[j] = tmp[j].u8[i + boffset];
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                LOG_SCP_32x16("=", idx);
                L1_SCP_CHECK_FILL(cpu, idx, id);
            }
        }
        break;
    case tload_cmd_transpose16:
        boffset = (boffset & 0x2) * 8;
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoadTranspose16 with msk: %d, "
                 "coop: %d, start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: "
                 "%u, rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        okay.reset();
        for (int j = 0; j < (L1D_LINE_SIZE / 2); ++j) {
            try {
                mmu_tensor_load512(cpu, addr + j*stride, tmp[j].u32.data(), Mem_Access_TxLoad);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                goto tload_exit;
            }
            catch (const memory_error&) {
                cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                continue;
            }
            okay[j] = true;
        }
        for (int i = 0; i < rows; ++i) {
            if (okay[i] && (!msk || tload.tmask[i])) {
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                for (int j = 0; j < L1D_LINE_SIZE / 2; ++j) {
                    SCP[idx].u16[j] = tmp[j].u16[i + boffset];
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                LOG_SCP_32x16("=", idx);
                L1_SCP_CHECK_FILL(cpu, idx, id);
            }
        }
        break;
    case tload_cmd_transpose32:
        LOG_HART(DEBUG, cpu, "(TL-H%u-%lu) Execute TensorLoadTranspose32 with msk: %d, "
                 "coop: %d, start: %d, tenb: %d, addr: 0x%" PRIx64 ", boffset: "
                 "%u, rows: %d, stride: 0x%" PRIx64 ", id: %d, tmask: 0x%lx",
                 cpu.mhartid, tload.uuid, int(msk), int(coop), start, tenb, addr, boffset,
                 rows, stride, id, tload.tmask.to_ulong());
        okay.reset();
        for (int j = 0; j < (L1D_LINE_SIZE / 4); ++j) {
            try {
                mmu_tensor_load512(cpu, addr + j*stride, tmp[j].u32.data(), Mem_Access_TxLoad);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                goto tload_exit;
            }
            catch (const memory_error&) {
                cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
                continue;
            }
            okay[j] = true;
        }
        for (int i = 0; i < rows; ++i) {
            if (okay[i] && (!msk || tload.tmask[i])) {
                int idx = adj + ((start + i) % L1_SCP_ENTRIES);
                for (int j = 0; j < L1D_LINE_SIZE / 4; ++j) {
                    SCP[idx].u32[j] = tmp[j].u32[i/*+ boffset==0*/];
                }
                notify_tensor_load_scp_write(cpu, i, &SCP[idx].u64[0]);
                LOG_SCP_32x16("=", idx);
                L1_SCP_CHECK_FILL(cpu, idx, id);
            }
        }
        break;
    }

tload_exit:
    if (!tenb) {
        if (id == 0) {
            cpu.stop_waiting(Hart::Waiting::tload_0);
        } else {
            cpu.stop_waiting(Hart::Waiting::tload_1);
        }
    }
}


// ----- TensorLoadL2Scp emulation --------------------------------------------------

void tensor_load_l2_start(Hart& cpu, uint64_t control)
{
    uint64_t stride  = X31 & 0xFFFFFFFFFFC0ULL;
    uint32_t id      = X31 & 1ULL;

    int      msk     = (control >> 63) & 0x1;
    int      dst     = ((control >> 46) & 0x1FFFC)  + ((control >> 4)  & 0x3);
    uint64_t base    = control & 0xFFFFFFFFFFC0ULL;
    int      rows    = ((control     ) & 0xF) + 1;
    uint64_t addr    = sext<48>(base);

    LOG_REG(":", 31);
    LOG_HART(DEBUG, cpu, "\tStart/Execute TensorLoadL2SCP with msk:%d, start: %d, "
             "addr: 0x%" PRIx64 ", rows: %d, stride: 0x%" PRIx64 ", id: %d",
             msk, dst, addr, rows, stride, id);

    uint64_t shire = cpu.shireid();
    for (int i = 0; i < rows; ++i) {
        if (!msk || cpu.tensor_mask[i]) {
            uint64_t l2scp_addr = L2_SCP_BASE + shire * L2_SCP_OFFSET + ((dst + i) * L1D_LINE_SIZE);
            try {
                cache_line_t tmp;
                const uint64_t vaddr = sextVA(addr + i*stride);
                mmu_tensor_load512(cpu, vaddr, tmp.u32.data(), Mem_Access_TxLoadL2Scp);
                cpu.chip->memory.write(cpu, l2scp_addr, L1D_LINE_SIZE, tmp.u32.data());
                LOG_MEMWRITE512(l2scp_addr, tmp.u32);
                L2_SCP_CHECK_FILL(cpu, dst + i, id, vaddr);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                break;
            }
            catch (const memory_error&) {
                cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
            }
        }
    }

    if (id == 0) {
        cpu.stop_waiting(Hart::Waiting::tload_L2_0);
    } else {
        cpu.stop_waiting(Hart::Waiting::tload_L2_1);
    }
}


// ----- TensorQuant emulation -------------------------------------------------

void tensor_quant_start(Hart& cpu, uint64_t value)
{
    unsigned freg  = (value >> 57) & 0x1F;
    unsigned acols = (value >> 55) & 0x3;
    unsigned arows = (value >> 51) & 0xF;
    unsigned start = (value >> 45) & 0x3F;

    acols = (acols + 1) * 4;
    arows = arows + 1;
    start = start% L1_SCP_ENTRIES;

    if (cpu.core->tquant.state != TQuant::State::idle) {
        cpu.start_waiting(Hart::Waiting::tquant);
        cpu.npc = cpu.pc;
        throw instruction_restart();
    }

    // TensorQuant raises illegal instruction exception when rounding mode is
    // invalid even if the transforms do not use FRM.
    set_rounding_mode(cpu, FRM);

    const auto uuid = (cpu.core->tquant.uuid = ++(cpu.core->tensor_uuid));
    LOG_HART(DEBUG, cpu, "\t(TQ-H%u-%lu) Start TensorQuant with start %u, arows: %u, "
             "acols: %u, freg: %u, frm: %s", cpu.mhartid, uuid, start, arows, acols, freg,
             get_rounding_mode(cpu, FRM));

    // If a transformation needs the scratchpad, and the scratchpad is
    // disabled, then we set tensor_error and do nothing.
    for (int trans = 0; trans < TQUANT_MAX_TRANS; ++trans) {
        int funct = (value >> (trans * 4)) & 0xF;
        if (funct == tquant_funct_last) {
            if (trans == 0) {
                // Let's the checker know that the TensorQuant is starting, even if the instruction does nothing
                L1_SCP_CHECK_START(cpu, tensor_op_type::TensorQuant, true);

                // Nothing to do, don't activate the state machine
                return;
            }
            break;
        }
        if ((funct >= tquant_funct_int32_add_row)
            && (funct <= tquant_funct_fp32_mul_col)
            && (cpu.core->mcache_control != 0x3))
        {
            LOG_HART(DEBUG, cpu,
                     "\tTransformation %d is %s but L1SCP is disabled",
                     trans, get_quant_transform(funct));
            update_tensor_error(cpu, 1 << 4);

            // Let's the checker know that the TensorQuant is starting, even if the instruction fails
            L1_SCP_CHECK_START(cpu, tensor_op_type::TensorQuant, true);

            // Error, don't activate the state machine
            return;
        }
    }

    // Let's the checker know that the TensorQuant is starting, even if the instruction does nothing
    L1_SCP_CHECK_START(cpu, tensor_op_type::TensorQuant, true);

    // Activate the state machine
    cpu.core->tquant.value = value;
    cpu.core->tquant.frm   = FRM;
    cpu.core->tquant.state = TQuant::State::ready;
#if defined(ZSIM) || defined(SYS_EMU)
    cpu.core->tqueue.push(TQueue::Instruction::tquant);
#else
    tensor_quant_execute(cpu);
#endif
}


void tensor_quant_execute(Hart& cpu)
{
    if (cpu.core->tquant.state == TQuant::State::idle) {
        throw std::runtime_error("tensor_quant_execute() called while this "
                                 "thread's TensorQuant FSM is inactive");
    }

    uint64_t quant = cpu.core->tquant.value;
    unsigned freg  = (quant >> 57) & 0x1F;
    unsigned acols = (quant >> 55) & 0x3;
    unsigned arows = (quant >> 51) & 0xF;
    unsigned start = (quant >> 45) & 0x3F;

    acols = (acols + 1) * 4;
    arows = arows + 1;
    start = start % L1_SCP_ENTRIES;

    const auto uuid = cpu.core->tquant.uuid;
    LOG_HART(DEBUG, cpu, "(TQ-H%u-%lu) Execute TensorQuant with start: %u, arows: %u, "
             "acols: %u, freg: %u, frm: %s", cpu.mhartid, uuid, start, arows, acols, freg,
             get_rounding_mode(cpu, cpu.core->tquant.frm));

    set_rounding_mode(cpu, cpu.core->tquant.frm);

    for (unsigned trans = 0; trans < TQUANT_MAX_TRANS; ++trans) {
        unsigned funct = (quant >> (trans * 4)) & 0xF;
        LOG_HART(DEBUG, cpu, "\tTransformation %d: %s",
                 trans, get_quant_transform(funct));
        if (funct == tquant_funct_last) {
            break;
        }
        bool pack128 = (funct == tquant_funct_pack_128);

        // PACK_128B RTL operates on even registers first, and then on odd
        // registers, so it generates two writes to the destination register
        // when a row spans a vector.
        notify_tensor_quant_new_transform(cpu, pack128 && (acols > VLENW));

        for (unsigned row = 0; row < arows; ++row) {
            for (unsigned col = 0; col < acols; col += VLENW) {
                unsigned nelem = std::min(acols - col, unsigned(VLENW));
                unsigned fs1 = (freg + row*2 + col/VLENW) % NFREGS;
                unsigned fd = pack128 ? ((freg + row*2) % NFREGS) : fs1;
                switch (funct) {
                case tquant_funct_int32_to_fp32:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::i32_to_f32(FREGS[fd].i32[e]);
                    }
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case tquant_funct_fp32_to_int32:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = fpu::f32_to_i32(FREGS[fd].f32[e]);
                    }
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case tquant_funct_int32_relu:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = std::max(int32_t(0),
                                                    FREGS[fd].i32[e]);
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case tquant_funct_int32_add_row:
                    LOG_SCP(":", start, col);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = FREGS[fd].i32[e]
                                         + SCP[start].i32[col+e];
                    }
                    L1_SCP_CHECK_READ(cpu, start, tensor_op_type::TensorQuant);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case tquant_funct_int32_add_col:
                    LOG_SCP_32x1(":", start, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].i32[e] = FREGS[fd].i32[e]
                                         + SCP[start].i32[row];
                    }
                    L1_SCP_CHECK_READ(cpu, start, tensor_op_type::TensorQuant);
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case tquant_funct_fp32_mul_row:
                    LOG_SCP(":", start, col);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e],
                                                        SCP[start].f32[col+e]);
                    }
                    L1_SCP_CHECK_READ(cpu, start, tensor_op_type::TensorQuant);
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case tquant_funct_fp32_mul_col:
                    LOG_SCP_32x1(":", start, row);
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].f32[e] = fpu::f32_mul(FREGS[fd].f32[e],
                                                        SCP[start].f32[row]);
                    }
                    L1_SCP_CHECK_READ(cpu, start, tensor_op_type::TensorQuant);
                    LOG_FREG("=", fd);
                    set_fp_exceptions(cpu);
                    dirty_fp_state();
                    break;
                case tquant_funct_sat_int8:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        int32_t tmp = std::max(int32_t(-128), FREGS[fd].i32[e]);
                        FREGS[fd].i32[e] = std::min(int32_t(127), tmp) & 0xFF;
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case tquant_funct_sat_uint8:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        int32_t tmp = std::max(int32_t(0), FREGS[fd].i32[e]);
                        FREGS[fd].i32[e] = std::min(int32_t(255), tmp) & 0xFF;
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                case tquant_funct_pack_128:
                    LOG_FREG(":", fd);
                    for (unsigned e = 0; e < nelem; ++e) {
                        FREGS[fd].u8[col + e] = uint8_t(FREGS[fs1].u32[e]);
                    }
                    LOG_FREG("=", fd);
                    dirty_fp_state();
                    break;
                default:
                    throw std::runtime_error("Illegal TensorQuant transform!");
                }

                // Notify the checker
                if (pack128) {
                    notify_tensor_quant_write(cpu, trans, fd,
                                              mkmask(nelem/4) << (col/4),
                                              FREGS[fd]);
                } else {
                    notify_tensor_quant_write(cpu, trans, fd,
                                              mkmask(nelem),
                                              FREGS[fd]);
                }
            }
        }

        if ((funct >= tquant_funct_int32_add_row)
            && (funct <= tquant_funct_fp32_mul_col))
        {
            start = (start + 1) % L1_SCP_ENTRIES;
        }
    }

#if defined(ZSIM) || defined(SYS_EMU)
    assert(cpu.core->tqueue.front() == TQueue::Instruction::tquant);
    cpu.core->tqueue.pop();
    if (cpu.core->tqueue.front() == TQueue::Instruction::reduce) {
        cpu.core->reduce.state =
            (cpu.core->reduce.state == TReduce::State::waiting_to_receive)
            ? TReduce::State::ready_to_receive
            : TReduce::State::ready_to_send;
    }
#endif
    cpu.core->tquant.state = TQuant::State::idle;
    cpu.stop_waiting(Hart::Waiting::tquant);
}


// ----- TensorStore emulation -------------------------------------------------

static void tensor_store_from_scp(Hart& cpu, uint64_t tstorereg)
{
    int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to scratchpad source
    int      scpstart =  (tstorereg & 0x3F00000000000000ULL) >> 56;      // Start scratchpad entry to store
    int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
    uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFC0ULL);     // Address where to store the results
    int      src      = scpstart % L1_SCP_ENTRIES;

    uint64_t stride   = sext<48>(X31 & 0x0000FFFFFFFFFFC0ULL);
 
    LOG_REG(":", 31);
    LOG_HART(DEBUG, cpu, "\tStart/Execute TensorStoreFromScp with addr: %016" PRIx64 ", stride: %016" PRIx64 ", rows: %d, scpstart: %d, srcinc: %d", addr, stride, rows, src, srcinc);

    notify_tensor_store(cpu, true, rows, 4, 1);

    // Check if L1 SCP is enabled
    if (cpu.core->mcache_control != 0x3) {
        update_tensor_error(cpu, 1 << 4);
        notify_tensor_store_error(cpu, 1 << 4);
        return;
    }

    // For all the rows
    for (int row = 0; row < rows; row++) {
        LOG_SCP_32x16(":", src);
        try {
            mmu_tensor_store512(cpu, addr + row*stride, SCP[src].u32.data(), Mem_Access_TxStore);
            L1_SCP_CHECK_READ(cpu, src, tensor_op_type::TensorStore);
        }
        catch (const Exception&) {
            update_tensor_error(cpu, 1 << 7);
            notify_tensor_store_error(cpu, 1 << 7);
            return;
        }
        catch (const memory_error&) {
            cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
        }
        src = (src + srcinc) % L1_SCP_ENTRIES;
    }
}


void tensor_store_start(Hart& cpu, uint64_t tstorereg)
{
    uint64_t tstore_scp = (tstorereg >> 48) & 0x1;

    if (tstore_scp) {
        L1_SCP_CHECK_START(cpu, tensor_op_type::TensorStore, false);
  
        // If we execute a TensorStoreFromScp, we don't need to enqueue this operation
        return tensor_store_from_scp(cpu, tstorereg);
    }

    if (cpu.core->tstore.state != TStore::State::idle) {
        cpu.start_waiting(Hart::Waiting::tstore);
        cpu.npc = cpu.pc;
        throw instruction_restart();
    }

    int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to register source
    int      regstart =  (tstorereg & 0x3E00000000000000ULL) >> 57;      // Start register to store
    int      cols     = ((tstorereg & 0x0180000000000000ULL) >> 55) + 1; // Number of register per col
    int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
    int      coop     = ((tstorereg & 0x0006000000000000ULL) >> 49) + 1; // Number of cooperative minions
    uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFF0ULL);     // Address where to store the results

    uint64_t stride   = sext<48>(X31 & 0x0000FFFFFFFFFFF0ULL);

    L1_SCP_CHECK_START(cpu, tensor_op_type::TensorStore, true);

    LOG_REG(":", 31);

    const auto uuid = (cpu.core->tstore.uuid = ++(cpu.core->tensor_uuid));
    LOG_HART(DEBUG, cpu, "\t(TS-H%u-%lu) Start TensorStore with addr: %016" PRIx64 ", "
             "stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, "
             "srcinc: %d, coop: %d", cpu.mhartid, uuid, addr, stride, regstart, rows,
             cols, srcinc, coop);

    // Check legal coop combination
    // xs[50:49]/xs[56:55]
    static const bool coop_comb[4*4] = {
        true,  true,  false, true,
        true,  true,  false, false,
        false, false, false, false,
        true,  false, false, false
    };

    if (!coop_comb[4*(coop-1)+(cols-1)]) {
        update_tensor_error(cpu, 1 << 8);
        notify_tensor_store_error(cpu, 1 << 8);
        return;
    }
 
    // Cooperative tensor stores require the shire to be in cooperative mode
    if (coop > 1) {
        uint64_t shire = shire_index(cpu);
        if (!cpu.chip->shire_other_esrs[shire].shire_coop_mode)
            throw trap_illegal_instruction(cpu.inst.bits);
    }

    cpu.core->tstore.value  = tstorereg;
    cpu.core->tstore.stride = stride;
    cpu.core->tstore.state  = TStore::State::ready;
#if defined(ZSIM) || defined (SYS_EMU)
    cpu.core->tqueue.push(TQueue::Instruction::tstore);
#else
    tensor_store_execute(cpu);
#endif
}


void tensor_store_execute(Hart& cpu)
{
    if (cpu.core->tstore.state == TStore::State::idle) {
        throw std::runtime_error("tensor_store_execute() called while this "
                                 "thread's TensorStore FSM is inactive");
    }

#if defined(ZSIM) || defined(SYS_EMU)
    assert(cpu.core->tqueue.front() == TQueue::Instruction::tstore);
    cpu.core->tqueue.pop();
    if (cpu.core->tqueue.front() == TQueue::Instruction::reduce) {
        cpu.core->reduce.state =
            (cpu.core->reduce.state == TReduce::State::waiting_to_receive)
            ? TReduce::State::ready_to_receive
            : TReduce::State::ready_to_send;
    }
#endif
    cpu.core->tstore.state = TStore::State::idle;

    const auto tstorereg = cpu.core->tstore.value;

    int      srcinc   = ((tstorereg & 0xC00000000000000CULL) >> 62) + 1; // Increment done to register source
    int      regstart =  (tstorereg & 0x3E00000000000000ULL) >> 57;      // Start register to store
    int      cols     = ((tstorereg & 0x0180000000000000ULL) >> 55) + 1; // Number of register per col
    int      rows     = ((tstorereg & 0x0078000000000000ULL) >> 51) + 1; // Number of rows to store
    int      coop     = ((tstorereg & 0x0006000000000000ULL) >> 49) + 1; // Number of cooperative minions
    uint64_t addr     = sext<48>(tstorereg & 0x0000FFFFFFFFFFF0ULL);     // Address where to store the results

    const auto stride = cpu.core->tstore.stride;

    notify_tensor_store(cpu, false, rows, cols, coop);
 
    const auto uuid = cpu.core->tstore.uuid;
    LOG_HART(DEBUG, cpu, "(TS-H%u-%lu) Execute TensorStore with addr: %016" PRIx64 ", "
             "stride: %016" PRIx64 ", regstart: %d, rows: %d, cols: %d, srcinc: %d, "
             "coop: %d", cpu.mhartid, uuid, addr, stride, regstart, rows, cols, srcinc, coop);

    // For all the rows
    int src = regstart;
    uint64_t mask = ~(16ull*cols - 1ull);
    for (int row = 0; row < rows; row++) {
        // For all the blocks of 128b
        for (int col = 0; col < cols; col++) {
            try {
                if (!(col & 1)) LOG_FREG(":", src);
                const uint32_t* ptr = &FREGS[src].u32[(col & 1) * 4];
                const uint64_t eaddr = (addr + row * stride) & mask;
                mmu_tensor_store128(cpu, eaddr + col*16, ptr, Mem_Access_TxStore);
            }
            catch (const Exception&) {
                update_tensor_error(cpu, 1 << 7);
                notify_tensor_store_error(cpu, 1 << 7);
                return;
            }
            catch (const memory_error&) {
                cpu.raise_interrupt(BUS_ERROR_INTERRUPT, 0);
            }
            // For 128b stores, move to next desired register immediately.
            // For 256b and 512b stores, move to next desired register
            // when 256b are written
            if ((cols == 1) || (col & 1)) src = (src + srcinc) % NFREGS;
        }
    }

#ifdef SYS_EMU
    bool tstoreCheck = SYS_EMU_PTR->get_tstore_check();
    auto& tstoreChecker = SYS_EMU_PTR->get_tstore_checker();

    // After all the checks are done, report the TStore access, notice that TStores that
    // are partially done due a tensor error won't be captured...
    if ((tstoreCheck) && (coop > 1)) {
        tstoreChecker.execute(hart_index(cpu), addr, stride, coop, rows, cols);
        tstoreChecker.check_and_drain(hart_index(cpu));
    }
#endif

    // Need to notify the stop when it is done
    cpu.stop_waiting(Hart::Waiting::tstore);
}


// ----- TensorFMA emulation ---------------------------------------------------

static void tensor_fma32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->tmul.value >> 63) & 0x1;
    int  bcols      = (cpu.core->tmul.value >> 55) & 0x3;
    int  arows      = (cpu.core->tmul.value >> 51) & 0xF;
    int  acols      = (cpu.core->tmul.value >> 47) & 0xF;
    int  aoffset    = (cpu.core->tmul.value >> 43) & 0xF;
    bool tenb       = (cpu.core->tmul.value >> 20) & 0x1;
    int  bstart     = (cpu.core->tmul.value >> 12) & 0x3F;
    int  astart     = (cpu.core->tmul.value >>  4) & 0x3F;
    bool first_pass = (cpu.core->tmul.value >>  0) & 1;
    auto tmask      = cpu.core->tmul.tmask;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;
    if (tenb) {
        bstart = 0;
    }

    const auto uuid = cpu.core->tmul.uuid;
    LOG_HART(DEBUG, cpu, "(TM-H%u-%lu) Execute TensorFMA32 with msk: %d, bcols: %d, "
             "arows: %d, acols: %d, aoffset: %d, tenb: %d, bstart: %d, "
             "astart: %d, mul: %d, rm: %s, tmask: 0x%lx", cpu.mhartid, uuid, usemsk,
             bcols, arows, acols, aoffset, tenb, bstart, astart, first_pass,
             get_rounding_mode(cpu, cpu.core->tmul.frm), tmask.to_ulong());

    set_rounding_mode(cpu, cpu.core->tmul.frm);
    for (int k = 0; k < acols; ++k) {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? (k+L1_SCP_ENTRIES) : ((bstart+k)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k)%L1_SCP_ENTRIES), tensor_op_type::TensorFMA);

        for (int i = 0; i < arows; ++i) {
            bool written[2] = { false, false };

            // Skip computation for this row
            if (usemsk && !tmask[i]) {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k) {
                    for (int j = 0; j < bcols; ++j) {
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
            L1_SCP_CHECK_READ(cpu, a_scp_entry, tensor_op_type::TensorFMA);
            LOG_SCP_32x1(":", a_scp_entry, ((aoffset+k) % (L1D_LINE_SIZE/4)));

            // If first_pass is 1 and this is the first iteration we do FMUL
            // instead of FMA
            if (first_pass && !k) {
                for (int j = 0; j < bcols; ++j) {
                    float32_t b = tmpb.f32[j];
                    float32_t c = fpu::f32_mul(a, b);
                    FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = fpu::UI32(c);
                    notify_tensor_fma_write(cpu, k, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                    written[j/VLENW] = true;
                }
            } else {
                // If the product will be 0, we can skip the operation
                if (fpu::UI32(a) == 0)
                    continue;

                for (int j = 0; j < bcols; ++j) {
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


static void tensor_fma16a32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->tmul.value >> 63) & 0x1;
    int  bcols      = (cpu.core->tmul.value >> 55) & 0x3;
    int  arows      = (cpu.core->tmul.value >> 51) & 0xF;
    int  acols      = (cpu.core->tmul.value >> 47) & 0xF;
    int  aoffset    = (cpu.core->tmul.value >> 43) & 0xF;
    bool tenb       = (cpu.core->tmul.value >> 20) & 0x1;
    int  bstart     = (cpu.core->tmul.value >> 12) & 0x3F;
    int  astart     = (cpu.core->tmul.value >>  4) & 0x3F;
    bool first_pass = (cpu.core->tmul.value >>  0) & 1;
    auto tmask      = cpu.core->tmul.tmask;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 2;
    aoffset = aoffset * 2;
    if (tenb) {
        bstart = 0;
    }

    const auto uuid = cpu.core->tmul.uuid;
    LOG_HART(DEBUG, cpu, "(TM-H%u-%lu) Execute TensorFMA16A32 with msk: %d, bcols: %d, "
             "arows: %d, acols: %d, aoffset: %d, tenb: %d, bstart: %d, "
             "astart: %d, mul: %d, rm: rtz, tmask: 0x%lx", cpu.mhartid, uuid, usemsk,
             bcols, arows, acols, aoffset, tenb, bstart, astart, first_pass,
             tmask.to_ulong());

    set_rounding_mode(cpu, softfloat_round_minMag);
    for (int k = 0; k < acols; k += 2) {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? ((k/2)+L1_SCP_ENTRIES) : ((bstart+k/2)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k/2)%L1_SCP_ENTRIES), tensor_op_type::TensorFMA);

        for (int i = 0; i < arows; ++i) {
            bool written[2] = { false, false };

            // Skip computation for this row
            if (usemsk && !tmask[i]) {
                // If first_pass is 1 and this is the first iteration we skip
                // the computation but we still set f[i] to 0.0
                if (first_pass && !k) {
                    for (int j = 0; j < bcols; ++j) {
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
            L1_SCP_CHECK_READ(cpu, a_scp_entry, tensor_op_type::TensorFMA);

            // If first_pass is 1 and this is the first iteration we do
            // a1*b1+a2*b2 instead of a1*b1+a2*b2+c0
            if (first_pass && !k) {
                for (int j = 0; j < bcols; ++j) {
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
            else if ((fpu::UI16(a1) != 0) || (fpu::UI16(a2) != 0)) {
                for (int j = 0; j < bcols; ++j) {
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

static void tensor_ima8a32_execute(Hart& cpu)
{
    bool usemsk     = (cpu.core->tmul.value >> 63) & 0x1;
    int  bcols      = (cpu.core->tmul.value >> 55) & 0x3;
    int  arows      = (cpu.core->tmul.value >> 51) & 0xF;
    int  acols      = (cpu.core->tmul.value >> 47) & 0xF;
    int  aoffset    = (cpu.core->tmul.value >> 43) & 0xF;
    bool tenc2rf    = (cpu.core->tmul.value >> 23) & 0x1;
    bool ub         = (cpu.core->tmul.value >> 22) & 0x1;
    bool ua         = (cpu.core->tmul.value >> 21) & 0x1;
    bool tenb       = (cpu.core->tmul.value >> 20) & 0x1;
    int  bstart     = (cpu.core->tmul.value >> 12) & 0x3F;
    int  astart     = (cpu.core->tmul.value >>  4) & 0x3F;
    bool first_pass = (cpu.core->tmul.value >>  0) & 1;
    auto tmask      = cpu.core->tmul.tmask;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = (acols + 1) * 4;
    aoffset = aoffset * 4;
    if (tenb) {
        bstart = 0;
    }

    const auto uuid = cpu.core->tmul.uuid;
    LOG_HART(DEBUG, cpu, "(TM-H%u-%lu) Execute TensorIMA8A32 with msk: %d, bcols: %d, "
             "arows: %d, acols: %d, aoffset: %d, dst: %d, ub: %d, ua: %d, "
             "tenb: %d, bstart: %d, astart: %d mul: %d, tmask: 0x%lx", cpu.mhartid, uuid,
             usemsk, bcols, arows, acols, aoffset, tenc2rf, ub, ua, tenb,
             bstart, astart, first_pass, tmask.to_ulong());

    for (int k = 0; k < acols; k += 4) {
        notify_tensor_fma_new_pass(cpu);

        // Model TenB as an extension of the scratchpad
        cache_line_t& tmpb = SCP[tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES)];
        LOG_SCP_32x16(":", tenb ? ((k/4)+L1_SCP_ENTRIES) : ((bstart+k/4)%L1_SCP_ENTRIES));
        if (!tenb)
            L1_SCP_CHECK_READ(cpu, ((bstart+k/4)%L1_SCP_ENTRIES), tensor_op_type::TensorFMA);

        bool write_freg = (tenc2rf && (k+4 == acols));
        freg_t* dst = write_freg ? FREGS.data() : TENC.data();

        for (int i = 0; i < arows; ++i) {
            bool written[2] = { false, false };

            // Always reading the data from the A matrix except if masked
            if (!usemsk || tmask[i]) {
                L1_SCP_CHECK_READ(cpu, (astart+i) % L1_SCP_ENTRIES, tensor_op_type::TensorFMA);
            }

            // We should skip computation for this row, but:
            // * if first_pass is set and this is the first iteration then we still set TenC to 0
            // * if tenc2rf is set and we are in the last pass then we must copy TenC to FREGS even for this row.
            if (usemsk && !tmask[i]) {
                if (write_freg) {
                    for (int j = 0; j < bcols; ++j) {
                        FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW] = (first_pass && !k) ? 0 : TENC[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW];
                        notify_tensor_fma_write(cpu, k/4, true, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, FREGS[i*TFMA_REGS_PER_ROW + j/VLENW].u32[j%VLENW]);
                        written[j/VLENW] = true;
                    }
                }
                else if (first_pass && !k) {
                    for (int j = 0; j < bcols; ++j) {
                        TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW] = 0;
                        notify_tensor_fma_write(cpu, 0, false, i*TFMA_REGS_PER_ROW+j/VLENW, j%VLENW, TENC[i*TFMA_REGS_PER_ROW+j/VLENW].u32[j%VLENW]);
                        written[j/VLENW] = true;
                    }
                }
            }

            // If first_pass is 1 and this is the first iteration we do
            // a1*b1+a2*b2+a3*b3+a4*b4 instead of c0+a1*b1+a2*b2+a3*b3+a4*b4
            else if (first_pass && !k) {
#define ASRC(x) SCP[(astart+i) % L1_SCP_ENTRIES].u8[(aoffset+k+(x)) % L1D_LINE_SIZE]
                int32_t a1 = ua ? ASRC(0) : sext8_2(ASRC(0));
                int32_t a2 = ua ? ASRC(1) : sext8_2(ASRC(1));
                int32_t a3 = ua ? ASRC(2) : sext8_2(ASRC(2));
                int32_t a4 = ua ? ASRC(3) : sext8_2(ASRC(3));
#undef ASRC
                LOG_SCP_32x1(":", (astart+i) % L1_SCP_ENTRIES, ((aoffset+k) % L1D_LINE_SIZE) / 4);
                for (int j = 0; j < bcols; ++j) {
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
            else if (write_freg || SCP[(astart+i) % L1_SCP_ENTRIES].u32[((aoffset+k)/4) % (L1D_LINE_SIZE/4)]) {
#define ASRC(x) SCP[(astart+i) % L1_SCP_ENTRIES].u8[(aoffset+k+(x)) % L1D_LINE_SIZE]
                int32_t a1 = ua ? ASRC(0) : sext8_2(ASRC(0));
                int32_t a2 = ua ? ASRC(1) : sext8_2(ASRC(1));
                int32_t a3 = ua ? ASRC(2) : sext8_2(ASRC(2));
                int32_t a4 = ua ? ASRC(3) : sext8_2(ASRC(3));
#undef ASRC
                LOG_SCP_32x1(":", (astart+i) % L1_SCP_ENTRIES, ((aoffset+k) % L1D_LINE_SIZE) / 4);
                LOG_CREG(":", i*TFMA_REGS_PER_ROW);
                if (bcols > 1) LOG_CREG(":", i*TFMA_REGS_PER_ROW + 1);
                for (int j = 0; j < bcols; ++j) {
#define BSRC(x) tmpb.u8[j*4+(x)]
                    int32_t b1 = ub ? BSRC(0) : sext8_2(BSRC(0));
                    int32_t b2 = ub ? BSRC(1) : sext8_2(BSRC(1));
                    int32_t b3 = ub ? BSRC(2) : sext8_2(BSRC(2));
                    int32_t b4 = ub ? BSRC(3) : sext8_2(BSRC(3));
#undef BSRC
                    // If all products are 0 for both column @j and column @j+8 or @j-8, we can skip the
                    // operation, except if TenC must be copied to FREGS and this is the last iteration.
                    // NB: The detection is done at 32-bit granularity, not at element (8-bit) granularity
                    if (j >= 8) {
                        if (!write_freg && (tmpb.u32[j] == 0) && (tmpb.u32[j-8] == 0))
                            continue;
                    } else {
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

            if (write_freg) {
                if (written[0]) LOG_FREG("=", i*TFMA_REGS_PER_ROW);
                if (written[1]) LOG_FREG("=", i*TFMA_REGS_PER_ROW + 1);
            } else {
                if (written[0]) LOG_CREG("=", i*TFMA_REGS_PER_ROW);
                if (written[1]) LOG_CREG("=", i*TFMA_REGS_PER_ROW + 1);
            }
        }
    }
    if (tenc2rf) {
        dirty_fp_state();
    }
}


void tensor_fma_execute(Hart& cpu)
{
    if (cpu.core->tmul.state == TMul::State::idle) {
        throw std::runtime_error("tensor_fma_execute() called while "
                                 "this thread's TensorFMA FSM is inactive");
    }

#if defined(ZSIM) || defined(SYS_EMU)
    assert(cpu.core->tqueue.front() == TQueue::Instruction::tfma);
    cpu.core->tqueue.pop();
    if (cpu.core->tqueue.front() == TQueue::Instruction::reduce) {
        cpu.core->reduce.state =
            (cpu.core->reduce.state == TReduce::State::waiting_to_receive)
            ? TReduce::State::ready_to_receive
            : TReduce::State::ready_to_send;
    }
#endif
    cpu.core->tmul.state = TMul::State::idle;
    if (cpu.core->tload_b.paired) {
        // Paired txfma; complete previous load to tenb
        cpu.core->tload_b.clear();
        cpu.stop_waiting(Hart::Waiting::tload_tenb);
    }

    switch ((cpu.core->tmul.value >> 1) & 0x7) {
    case tfma_type_fp32: tensor_fma32_execute(cpu); break;
    case tfma_type_fp16: tensor_fma16a32_execute(cpu); break;
    case tfma_type_int8: tensor_ima8a32_execute(cpu); break;
    default:             throw std::runtime_error("tensor_fma_execute() with illegal type");
    }
    
    cpu.stop_waiting(Hart::Waiting::tfma);
}


void tensor_fma_start(Hart& cpu, uint64_t control)
{
    bool msk     = (control >> 63) & 1;
    int  bcols   = (control >> 55) & 3;
    int  arows   = (control >> 51) & 0xF;
    int  acols   = (control >> 47) & 0xF;
    int  aoffset = (control >> 43) & 0xF;
    bool dst     = (control >> 23) & 0x1;
    bool ub      = (control >> 22) & 0x1;
    bool ua      = (control >> 21) & 0x1;
    bool tenb    = (control >> 20) & 1;
    int  bstart  = (control >> 12) & 0x3F;
    int  astart  = (control >>  4) & 0x3F;
    int  type    = (control >>  1) & 7;
    bool mul     = (control >>  0) & 1;

    bcols = (bcols + 1) * 4;
    arows = arows + 1;
    acols = acols + 1;
    if (tenb) {
        bstart = 0;
    }

    if (cpu.core->tmul.state != TMul::State::idle) {
        cpu.start_waiting(Hart::Waiting::tfma);
        cpu.npc = cpu.pc;
        throw instruction_restart();
    }

    const auto uuid = (cpu.core->tmul.uuid = ++(cpu.core->tensor_uuid));

    switch (type) {
    case tfma_type_fp32:
        // Illegal instruction exception has higher priority than other errors
        set_rounding_mode(cpu, FRM);
        LOG_HART(DEBUG, cpu, "\t(TM-H%u-%lu) Start TensorFMA32 with msk: %d, bcols: %d, "
                 "arows: %d, acols: %d, aoffset: %d, tenb: %d, bstart: %d, "
                 "astart: %d, mul: %d, rm: %s", cpu.mhartid, uuid, msk, bcols, arows, acols,
                 aoffset, tenb, bstart, astart, mul,
                 get_rounding_mode(cpu, FRM));
        break;
    case tfma_type_fp16:
        LOG_HART(DEBUG, cpu, "\t(TM-H%u-%lu) Start TensorFMA16A32 with msk: %d, bcols: %d, "
                 "arows: %d, acols: %d, aoffset: %d, tenb: %d, bstart: %d, "
                 "astart: %d, mul: %d, rm: rtz", cpu.mhartid, uuid, msk, bcols, arows, acols*2,
                 aoffset*2, tenb, bstart, astart, mul);
        break;
    case tfma_type_int8:
        LOG_HART(DEBUG, cpu, "\t(TM-H%u-%lu) Start TensorIMA8A32 with msk: %d, bcols: %d, "
                 "arows: %d, acols: %d, aoffset: %d, dst: %d, ub: %d, ua: %d, "
                 "tenb: %d, bstart: %d, astart: %d mul: %d", cpu.mhartid, uuid, msk, bcols,
                 arows, acols*4, aoffset*4, dst, ub, ua, tenb, bstart, astart, mul);
        break;
    default:
        throw trap_illegal_instruction(cpu.inst.bits);
    }

    // Unpair the last TensorLoadSetupB
    bool load_tenb = (cpu.core->tload_b.state != TLoad::State::idle);
    bool wait_tenb = (cpu.core->tload_b.state == TLoad::State::waiting_coop);
    int  brows_tenb = (cpu.core->tload_b.value & 0xF) + 1;

    if (load_tenb) {
        cpu.core->tload_b.paired = tenb;
    }

    cpu.core->tmul.value = control;
    cpu.core->tmul.frm = FRM;
    if (msk) {
        LOG_TENSOR_MASK(":");
        cpu.core->tmul.tmask = cpu.tensor_mask;
    } else {
        cpu.core->tmul.tmask = 0xffff;
    }

    bool failed = false;

    // tenb and no TensorLoadSetupB to pair, or tenb and incompatible
    // rows/columns size, or not tenb and orphaned TensorLoadSetupB
    if ((tenb && (!load_tenb || (brows_tenb != acols))) || (!tenb && load_tenb)) {
        update_tensor_error(cpu, 1 << 6);
        failed = true;
    }

    // Check if L1SCP is enabled
    if (cpu.core->mcache_control != 3) {
        update_tensor_error(cpu, 1 << 4);
        failed = true;
    }

    if (wait_tenb) {
        cpu.core->tmul.state = failed
            ? TMul::State::idle
            : (tenb ? TMul::State::waiting_tenb : TMul::State::ready);
    } else {
        cpu.core->tmul.state = failed
            ? TMul::State::idle
            : TMul::State::ready;
    }

    // Let's the checker know that the TensorFMA is starting, even if the instruction failed
    // This is before the stop_waiting below on purpose
    bool tenc2rf = (cpu.core->tmul.value >> 23) & 0x1;
    (void)tenc2rf;
    L1_SCP_CHECK_START(cpu, tensor_op_type::TensorFMA, tenc2rf || (type != tfma_type_int8));

    if (failed || !tenb) {
        if (wait_tenb) {
            // Canceling a cooperative tensor load will lead to problems!
            throw std::runtime_error("tensor_fma_start() canceling a "
                                     "cooperative tensor load to tenb");
        }
        // Non-paired txfma; cancel previous load to tenb
        cpu.core->tload_b.state = TLoad::State::idle;
        cpu.stop_waiting(Hart::Waiting::tload_tenb);
    }

    if (failed) {
        return;
    }

#if defined(ZSIM) || defined(SYS_EMU)
    cpu.core->tqueue.push(TQueue::Instruction::tfma);
#else
    tensor_fma_execute(cpu);
#endif
}


// ----- TensorReduce emulation ------------------------------------------------

void tensor_reduce_start(Hart& cpu, uint64_t value)
{
    enum class Command {
        send = 0,
        receive = 1,
        broadcast = 2,
        reduce = 3,
    };

    static const char* reducecmd[4] = {
        "TensorSend", "TensorRecv",
        "TensorBroadcast", "TensorReduce"
    };

    if (cpu.core->reduce.state != TReduce::State::idle) {
        cpu.start_waiting(Hart::Waiting::reduce);
        cpu.npc = cpu.pc;
        throw instruction_restart();
    }

    Command  command  = static_cast<Command>(value & 3);
    unsigned height   = (value >> 3) & 0xF;
    unsigned distance = 1 << height;
    unsigned minmask  = (1 << (height + 1)) - 1;
    unsigned minion   = core_index(cpu);

    TReduce& reduce = cpu.core->reduce;

    reduce.freg  = (value >> 57) & 0x1F;
    reduce.count = (value >> 16) & 0x7F;
    reduce.funct = (value >> 24) & 0xF;
    reduce.frm   = FRM;

    if (command != Command::send
        && reduce.funct == reduce_function_fadd)
    {
        // TensorRecv, TensorBroadcast and TensorReduce should raise illegal
        // instruction if the encoded function is FADD and FRM does not hold
        // a valid rounding mode
        set_rounding_mode(cpu, reduce.frm);
    }

    switch (command) {
    case Command::send:
        reduce.state = TReduce::State::waiting_to_send;
        reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * ((value >> 3) & 0x1FFF)];
        break;
    case Command::receive:
        reduce.state = TReduce::State::waiting_to_receive;
        reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * ((value >> 3) & 0x1FFF)];
        break;
    case Command::broadcast:
        if ((minion & minmask) == distance) {
            reduce.state = TReduce::State::waiting_to_receive;
            reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * (minion - distance)];
        } else if ((minion & minmask) == 0) {
            reduce.state = TReduce::State::waiting_to_send;
            reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * (minion + distance)];
        } else {
            reduce.state = TReduce::State::idle;
            reduce.hart  = &cpu;
        }
        break;
    case Command::reduce:
        if ((minion & minmask) == distance) {
            reduce.state = TReduce::State::waiting_to_send;
            reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * (minion - distance)];
        } else if ((minion & minmask) == 0) {
            reduce.state = TReduce::State::waiting_to_receive;
            reduce.hart  = &cpu.chip->cpu[EMU_THREADS_PER_MINION * (minion + distance)];
        } else {
            reduce.state = TReduce::State::idle;
            reduce.hart  = &cpu;
        }
        break;
    }

    if (reduce.state == TReduce::State::idle) {
        LOG_HART(DEBUG, cpu, "\t%s(skip) with height: %u, distance: %u",
                 reducecmd[static_cast<int>(command)], height, distance);
        return;
    }

    // Sending and receiving from the same minion should fail immediately
    if (reduce.hart == &cpu) {
        reduce.state = TReduce::State::idle;
        LOG_HART(DEBUG, cpu, "\t%s(fail) with partner: H%u, freg: %u, count: %u",
                 reducecmd[static_cast<int>(command)],
                 reduce.hart->mhartid, reduce.freg, reduce.count);
        update_tensor_error(cpu, 1 << 9);

        // Let's the checker know that the TensorReduce is starting, even if the instruction failed
        L1_SCP_CHECK_START(cpu, tensor_op_type::TensorReduce, true);

        return;
    }

    // Illegal function on a receiving minion should fail immediately
    if (reduce.state == TReduce::State::waiting_to_receive) {
        if (reduce.funct == reduce_function_reserved_1
            || reduce.funct == reduce_function_reserved_5
            || reduce.funct >=  reduce_function_reserved_9)
        {
            reduce.state = TReduce::State::idle;
            LOG_HART(DEBUG, cpu, "\t%s(fail) with function: %d",
                     reducecmd[static_cast<int>(command)], int(reduce.funct));
            update_tensor_error(cpu, 1 << 9);

            // Let's the checker know that the TensorReduce is starting, even if the instruction failed
            L1_SCP_CHECK_START(cpu, tensor_op_type::TensorReduce, true);

            return;
        }
    }

    // Sending or receiving 0 registers means do nothing
    // NB: This check has lower priority than other errors because
    // tensor_error[9] should be set even when "count" == 0".
    if (reduce.count == 0) {
        reduce.state = TReduce::State::idle;
        LOG_HART(DEBUG, cpu, "\t%s(skip) with count: 0",
                 reducecmd[static_cast<int>(command)]);

        // Let's the checker know that the TensorReduce is starting, even if the instruction failed
        L1_SCP_CHECK_START(cpu, tensor_op_type::TensorReduce, true);

        return;
    }

    const auto uuid = (reduce.uuid = ++(cpu.core->tensor_uuid));
    LOG_HART(DEBUG, cpu, "\t(TR-H%u-%lu) Start %s(%s) with partner: H%u, freg: %u, "
             "count: %u", cpu.mhartid, uuid, reducecmd[static_cast<int>(command)],
             ((reduce.state == TReduce::State::waiting_to_receive)
              ? "recv" : "send"), reduce.hart->mhartid, reduce.freg,
             reduce.count);

    notify_tensor_reduce(
        cpu, reduce.state == TReduce::State::waiting_to_receive,
        reduce.freg, reduce.count);

    // Let's the checker know that the TensorReduce is starting, even if the instruction failed
    L1_SCP_CHECK_START(cpu, tensor_op_type::TensorReduce, true);

#if defined(ZSIM) || defined(SYS_EMU)
    cpu.core->tqueue.push(TQueue::Instruction::reduce);
    if (cpu.core->tqueue.front() == TQueue::Instruction::reduce) {
        // If we are the head of the queue then we are actually ready to send
        // and receive, not just waiting.
        reduce.state = (reduce.state == TReduce::State::waiting_to_receive)
            ? TReduce::State::ready_to_receive
            : TReduce::State::ready_to_send;
    }
#else
    if (reduce.state == TReduce::State::waiting_to_receive) {
        reduce.state = TReduce::State::ready_to_receive;
        // CoSim calls this when the receiver is scheduled
        tensor_reduce_execute(cpu);
    } else {
        reduce.state = TReduce::State::ready_to_send;
    }
#endif
}


void tensor_reduce_step(Hart& rcv_cpu, Hart& snd_cpu)
{
#ifdef ZSIM
    static const char* fnctnm[] = {
        "fadd",   "rsvd1",  "fmax",   "fmin",
        "add",    "rsvd5",  "max",    "min",
        "move",   "rsvd9",  "rsvd10", "rsvd11",
        "rsvd12", "rsvd13", "rsvd14", "rsvd15",
    };
#endif

    TReduce& send = snd_cpu.core->reduce;
    TReduce& recv = rcv_cpu.core->reduce;

    if (--send.count == 0) {
#if defined(ZSIM) || defined(SYS_EMU)
        assert(snd_cpu.core->tqueue.front() == TQueue::Instruction::reduce);
        snd_cpu.core->tqueue.pop();
#endif
        snd_cpu.core->reduce.state = TReduce::State::idle;
        snd_cpu.stop_waiting(Hart::Waiting::reduce);
    }
    if (--recv.count == 0) {
#if defined(ZSIM) || defined(SYS_EMU)
        assert(rcv_cpu.core->tqueue.front() == TQueue::Instruction::reduce);
        rcv_cpu.core->tqueue.pop();
#endif
        rcv_cpu.core->reduce.state = TReduce::State::idle;
        rcv_cpu.stop_waiting(Hart::Waiting::reduce);
    }

#ifdef ZSIM
    LOG_HART(DEBUG, rcv_cpu,
             "\tTensor reduce step with sender=H%u funct=%s count=%u rmode=%s",
             snd_cpu.mhartid, fnctnm[recv.funct], recv.count,
             get_rounding_mode(rcv_cpu, 7));
#endif

    LOG_FREG_HART(snd_cpu, ":", send.freg);
    if (recv.funct != reduce_function_move) {
        LOG_FREG_HART(rcv_cpu, ":", recv.freg);
    }
    switch (recv.funct) {
    case reduce_function_fadd:
        set_rounding_mode(rcv_cpu, recv.frm);
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].f32[j] =
                fpu::f32_add(snd_cpu.fregs[send.freg].f32[j],
                             rcv_cpu.fregs[recv.freg].f32[j]);
        }
        set_fp_exceptions(rcv_cpu);
        break;
    case reduce_function_fmax:
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].f32[j] =
                fpu::f32_maximumNumber(snd_cpu.fregs[send.freg].f32[j],
                                       rcv_cpu.fregs[recv.freg].f32[j]);
        }
        set_fp_exceptions(rcv_cpu);
        break;
    case reduce_function_fmin:
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].f32[j] =
                fpu::f32_minimumNumber(snd_cpu.fregs[send.freg].f32[j],
                                       rcv_cpu.fregs[recv.freg].f32[j]);
        }
        set_fp_exceptions(rcv_cpu);
        break;
    case reduce_function_add:
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].u32[j] =
                snd_cpu.fregs[send.freg].u32[j] +
                rcv_cpu.fregs[recv.freg].u32[j];
        }
        break;
    case reduce_function_max:
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].i32[j] =
                std::max(snd_cpu.fregs[send.freg].i32[j],
                         rcv_cpu.fregs[recv.freg].i32[j]);
        }
        break;
    case reduce_function_min:
        for (unsigned j = 0; j < VLENW; j++) {
            rcv_cpu.fregs[recv.freg].i32[j] =
                std::min(snd_cpu.fregs[send.freg].i32[j],
                         rcv_cpu.fregs[recv.freg].i32[j]);
        }
        break;
    case reduce_function_move:
        rcv_cpu.fregs[recv.freg] = snd_cpu.fregs[send.freg];
        break;
    default:
        throw std::runtime_error("Tensor reduce with illegal function code!");
    }
    LOG_FREG_HART(rcv_cpu, "=", recv.freg);
    dirty_fp_state();
    notify_tensor_reduce_write(rcv_cpu, recv.freg, rcv_cpu.fregs[recv.freg]);

    send.freg = (send.freg + 1) % NFREGS;
    recv.freg = (recv.freg + 1) % NFREGS;
}


void tensor_reduce_execute(Hart& cpu)
{
#ifndef ZSIM
    static const char* fnctnm[] = {
        "fadd",   "rsvd1",  "fmax",   "fmin",
        "add",    "rsvd5",  "max",    "min",
        "move",   "rsvd9",  "rsvd10", "rsvd11",
        "rsvd12", "rsvd13", "rsvd14", "rsvd15",
    };
#endif

    if (cpu.core->reduce.state != TReduce::State::ready_to_receive) {
        // Nothing to do for now
        return;
    }

    Hart& snd_cpu = *cpu.core->reduce.hart;

#ifdef SYS_EMU
    if (snd_cpu.core->reduce.state != TReduce::State::ready_to_send) {
        // Receiver is ready, but sender is not ready yet...
        return;
    }
#endif

    if (cpu.core->reduce.hart->mhartid != snd_cpu.mhartid
        || snd_cpu.core->reduce.hart->mhartid != cpu.mhartid)
    {
        WARN_HART(tensors, cpu,
                 "\tTensor reduce hart mismatch with sender=H%u receiver=H%u "
                 "sender_partner=H%u receiver_partner=H%u",
                 snd_cpu.mhartid, cpu.mhartid,
                 snd_cpu.core->reduce.hart->mhartid,
                 cpu.core->reduce.hart->mhartid);
#ifndef SYS_EMU
        throw std::runtime_error("Mismatched tensor reduce send/receive hart");
#endif
        return;
    }

    if (snd_cpu.core->reduce.count != cpu.core->reduce.count) {
        WARN_HART(tensors, cpu,
                 "\tTensor reduce count mismatch with sender=H%u receiver=H%u "
                 "sender_count=%u receiver_count=%u",
                 snd_cpu.mhartid, cpu.mhartid,
                 snd_cpu.core->reduce.count, cpu.core->reduce.count);
        throw std::runtime_error("Mismatched tensor reduce send/receive count");
    }

#ifndef ZSIM
    const auto uuid = cpu.core->reduce.uuid;
    LOG_HART(DEBUG, cpu,
             "(TR-H%u-%lu) Execute tensor reduce with sender=H%u funct=%s count=%u rmode=%s",
             cpu.mhartid, uuid,
             snd_cpu.mhartid, fnctnm[cpu.core->reduce.funct],
             cpu.core->reduce.count,
             get_rounding_mode(cpu, cpu.core->reduce.frm));
#endif

    while (cpu.core->reduce.count) {
        tensor_reduce_step(cpu, snd_cpu);
    }
}


// ----- TensorWait emulation ------------------------------------------------


// Checks if the corresponding tensor FSM is idle.
//
//  - Only asynchronous operations can be non-idle
//    (tload, tfma, reduce, tquant, tstore)
//  - The rest is always considered idle
static bool tensor_wait_check_idle(const Hart& cpu, Hart::Waiting what)
{
    switch (what) {
    case Hart::Waiting::tload_0:
        return cpu.core->tload_a[0].state == TLoad::State::idle;
    case Hart::Waiting::tload_1:
        return cpu.core->tload_a[1].state == TLoad::State::idle;
    case Hart::Waiting::tfma:
        return cpu.core->tmul.state == TMul::State::idle;
    case Hart::Waiting::reduce:
        return cpu.core->reduce.state == TReduce::State::idle;
    case Hart::Waiting::tquant:
        return cpu.core->tquant.state == TQuant::State::idle;
    case Hart::Waiting::tstore:
        return cpu.core->tstore.state == TStore::State::idle;
    default:
        return true;
    }
}


// Executes a TensorWait.
//
// In the current model, a TensorWait "executes" once the hart
// stops waiting for an event, that has been TensorWait'ed.
// This notifies the checkers of the TensorWait.
// The only reason the TensorWait is "asynchronous", is to
// ensure the checkers see the correct order of operations.
void tensor_wait_execute(Hart& cpu, Hart::Waiting what)
{
#ifdef SYS_EMU
    if (SYS_EMU_PTR->get_l1_scp_check()) {
        const auto thread = hart_index(cpu);
        // TensorWait propagation
        SYS_EMU_PTR->get_l1_scp_checker().tensor_wait(thread, what);
    }
    if ((what == Hart::Waiting::tload_L2_0 || what == Hart::Waiting::tload_L2_1)
        && SYS_EMU_PTR->get_l2_scp_check()) {
        const auto thread = hart_index(cpu);
        const auto id = what == Hart::Waiting::tload_L2_0 ? 0 : 1;
        SYS_EMU_PTR->get_l2_scp_checker().l2_scp_wait(thread, id);
    }
#endif

    cpu.twait &= ~what;
}


// Starts a TensorWait.
//
// If the corresponding FSM is currently idle, this behaves like a NOP.
// Otherwise, the hart starts waiting for the given event.
// At this point we also notify of any change to the tensor_error.
void tensor_wait_start(Hart& cpu, uint64_t value)
{
    if (cpu.debug_mode) {
        WARN_HART(tensors, cpu, "%s", "Executing a TensorWait from debug mode has undefined behavior");
        return; // treat as a nop just in case
    }

    const uint64_t event = value & 0xF;
    if (event > 10) return; // Invalid events are treated as a NOP
    const auto what = static_cast<Hart::Waiting>(1 << event);
    const auto idle = tensor_wait_check_idle(cpu, what);
    if (((cpu.mhartid % EMU_THREADS_PER_MINION) == 0) ||
        (what == Hart::Waiting::tload_L2_0 || what == Hart::Waiting::tload_L2_1)) {
        if (idle) {
            // Execute the tensor_wait already
            tensor_wait_execute(cpu, what);
        } else {
            // Start waiting and mark the pending tensor_wait
            cpu.start_waiting(what);
            cpu.twait |= what;
        }
    }

    notify_tensor_error_value(cpu, cpu.tensor_error);
}


} // namespace bemu
