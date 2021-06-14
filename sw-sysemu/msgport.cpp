/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstring>

#include "cache.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "log.h"
#include "processor.h"
#include "system.h"
#include "traps.h"

namespace bemu {


// -----------------------------------------------------------------------------
//
// System methods
//
// -----------------------------------------------------------------------------

void System::write_msg_port_data_to_scp(Hart& cpu, unsigned id, uint32_t *data, uint8_t oob)
{
    // Drop the write if port not configured
    if (!cpu.portctrl[id].enabled)
        return;

    if (!cpu.core->scp_lock[cpu.portctrl[id].scp_set][cpu.portctrl[id].scp_way]) {
        LOG_AGENT(DEBUG, cpu, "PORT_WRITE Port cache line (s%d w%d) unlocked!",
                  cpu.portctrl[id].scp_set, cpu.portctrl[id].scp_way);
    }
    uint64_t base_addr = cpu.core->scp_addr[cpu.portctrl[id].scp_set][cpu.portctrl[id].scp_way];
    base_addr += cpu.portctrl[id].wr_ptr << cpu.portctrl[id].logsize;

    cpu.portctrl[id].stall = false;

    unsigned nwords = (1ULL << cpu.portctrl[id].logsize) / 4;

    LOG_AGENT(DEBUG, cpu, "Writing MSG_PORT (H%u p%u) wr_words %u, logsize %u",
              cpu.mhartid, id, nwords, cpu.portctrl[id].logsize);
    for (unsigned w = 0; w < nwords; w++) {
        LOG_AGENT(DEBUG, cpu, "Writing MSG_PORT (H%u p%u) data 0x%08" PRIx32 " to addr 0x%16" PRIx64,
                  cpu.mhartid, id, data[w], base_addr + 4 * w);
        memory.write(cpu, base_addr + 4 * w, sizeof(uint32_t), &data[w]);
    }

    if (cpu.portctrl[id].enable_oob) {
        cpu.portctrl[id].oob_data[cpu.portctrl[id].wr_ptr] = oob;
    }
    ++cpu.portctrl[id].size;
    cpu.portctrl[id].wr_ptr = (cpu.portctrl[id].wr_ptr + 1) % (cpu.portctrl[id].max_msgs + 1);

    if (msg_to_thread) {
        msg_to_thread(cpu.mhartid);
    }
}


void System::set_msg_funcs(msg_func_t fn)
{
    msg_to_thread = fn;
}


void System::write_msg_port_data(unsigned target_thread, unsigned id, unsigned source_thread, uint32_t *data)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = source_thread;
        port_write.target_thread = target_thread;
        port_write.target_port   = id;
        port_write.is_tbox       = false;
        port_write.is_rbox       = false;

        unsigned nwords = (1ULL << cpu[target_thread].portctrl[id].logsize) / 4;
        for (unsigned w = 0; w < nwords; w++) {
            port_write.data[w] = data[w];
        }
        port_write.oob = 0;
        msg_port_pending_writes[target_thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_AGENT(DEBUG, cpu[source_thread], "Delayed write on MSG_PORT (m%u p%u) from m%u", target_thread, id, source_thread);
        for (unsigned w = 0; w < nwords; ++w) {
            LOG_AGENT(DEBUG, cpu[source_thread], "                              data[%u] 0x%8" PRIx32, w, data[w]);
        }
    }
    else
    {
        write_msg_port_data_to_scp(cpu[target_thread], id, data, 0);
    }
}


void System::commit_msg_port_data(unsigned target_thread, unsigned port_id, unsigned source_thread)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG_AGENT(INFO, cpu[source_thread], "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG_AGENT(DEBUG, cpu[source_thread], "Commit write on MSG_PORT (h%u p%u) from h%u", target_thread, port_id, source_thread);
            write_msg_port_data_to_scp(cpu[target_thread], port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG_AGENT(DEBUG, cpu[source_thread], "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
        }
    }
    else
    {
        LOG_AGENT(DEBUG, cpu[source_thread], "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
    }
}


// -----------------------------------------------------------------------------
//
// Hart methods
//
// -----------------------------------------------------------------------------

unsigned get_msg_port_write_width(const Hart& cpu, unsigned port)
{
    return 1ULL << cpu.portctrl[port].logsize;
}


uint64_t read_port_base_address(const Hart& cpu, unsigned id)
{
    auto scp_set = cpu.portctrl[id].scp_set;
    auto scp_way = cpu.portctrl[id].scp_way;
    return cpu.core->scp_addr[scp_set][scp_way];
}


int64_t read_port_head(Hart& cpu, unsigned id, bool block)
{
    if (((cpu.prv == PRV_U) && !cpu.portctrl[id].umode) || !cpu.portctrl[id].enabled) {
        throw trap_illegal_instruction(cpu.inst.bits);
    }

    if (cpu.portctrl[id].size == 0) {
        LOG_HART(DEBUG, cpu, "Blocking MSG_PORT%s (M%u p%u) wr_ptr=%d, rd_ptr=%d",
                 block ? "" : "NB", cpu.mhartid, id,
                 cpu.portctrl[id].wr_ptr,
                 cpu.portctrl[id].rd_ptr);

        if (!block)
            return -1;

#ifdef SYS_EMU
        // if in sysemu stop thread if no data for port.. comparing rd_ptr and wr_ptr
        LOG_HART(DEBUG, cpu, "Stalling MSG_PORT (H%u p%u)", cpu.mhartid, id);
        cpu.portctrl[id].stall = true;
        return 0;
#endif
    }

    int32_t offset = cpu.portctrl[id].rd_ptr << cpu.portctrl[id].logsize;

    if (cpu.portctrl[id].enable_oob) {
        offset |= cpu.portctrl[id].oob_data[cpu.portctrl[id].rd_ptr];
    }

    if (++cpu.portctrl[id].rd_ptr > cpu.portctrl[id].max_msgs) {
        cpu.portctrl[id].rd_ptr = 0;
    }
    --cpu.portctrl[id].size;

    return offset;
}


void configure_port(Hart& cpu, unsigned id, uint32_t wdata)
{
    cpu.portctrl[id].enabled    = (wdata >>  0) & 0x1;
    cpu.portctrl[id].enable_oob = (wdata >>  1) & 0x1;
    cpu.portctrl[id].umode      = (wdata >>  4) & 0x1;
    cpu.portctrl[id].logsize    = (wdata >>  5) & 0x7;
    cpu.portctrl[id].max_msgs   = (wdata >>  8) & 0xF;
    cpu.portctrl[id].scp_set    = (wdata >> 16) & 0xFF;
    cpu.portctrl[id].scp_way    = (wdata >> 24) & 0xFF;

    cpu.portctrl[id].stall      = false;
    cpu.portctrl[id].rd_ptr     = 0;
    cpu.portctrl[id].wr_ptr     = 0;
    cpu.portctrl[id].size       = 0;

    cpu.portctrl[id].oob_data.reset();
}


uint32_t read_port_control(const Hart& cpu, unsigned id)
{
    return ((cpu.portctrl[id].enabled    ? 1 : 0) <<  0)
         | ((cpu.portctrl[id].enable_oob ? 1 : 0) <<  1)
         | ((cpu.portctrl[id].umode      ? 1 : 0) <<  4)
         | ((cpu.portctrl[id].logsize     & 0x07) <<  5)
         | ((cpu.portctrl[id].max_msgs    & 0x0F) <<  8)
         | ((cpu.portctrl[id].scp_set     & 0xFF) << 16)
         | ((cpu.portctrl[id].scp_way     & 0xFF) << 24)
         | 0x8000;
}


// -----------------------------------------------------------------------------
//
// Free-standing methods
//
// -----------------------------------------------------------------------------

uint32_t legalize_portctrl(uint32_t wdata)
{
    int logsize = (wdata >> 5)  & 0x07;
    int scp_set = (wdata >> 16) & 0xFF;
    int scp_way = (wdata >> 24) & 0xFF;

    logsize = std::max(PORT_LOG2_MIN_SIZE, std::min(PORT_LOG2_MAX_SIZE, logsize));
    scp_set = scp_set % L1D_NUM_SETS;
    scp_way = scp_way % L1D_NUM_WAYS;

    return (wdata & 0x00000F13)
         | (logsize << 5)
         | (scp_set << 16)
         | (scp_way << 24)
         | 0x00008000;
}


} // namespace bemu
