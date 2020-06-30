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
#include <cstring>
#include <deque>
#include <vector>

#include "cache.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "log.h"
#include "memory/main_memory.h"
#include "processor.h"
#include "traps.h"

namespace bemu {


extern MainMemory memory;


extern std::array<Hart,EMU_NUM_THREADS> cpu;

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5


enum msg_port_conf_action {
    MSG_ENABLE = 7,
    MSG_DISABLE = 3,
    MSG_PGET = 0,
    MSG_PGETNB = 1,
};


struct msg_port_write_t {
    uint32_t source_thread;
    uint32_t target_thread;
    uint32_t target_port;
    bool     is_remote;
    bool     is_tbox;
    bool     is_rbox;
    uint8_t  oob;
    uint32_t data[(1 << PORT_LOG2_MAX_SIZE)/4];
};


static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_tbox;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_rbox;

static bool msg_port_delayed_write = false;


void none_msg_to_thread(unsigned /*thread_id*/) { }


void (*msg_to_thread)(unsigned) = none_msg_to_thread;


static void write_msg_port_data_to_scp(Hart& cpu, unsigned id, uint32_t *data, uint8_t oob)
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

    msg_to_thread(cpu.mhartid);
}


void set_msg_funcs(void (*func_msg_to_thread)(unsigned))
{
    msg_to_thread = func_msg_to_thread;
}


bool get_msg_port_stall(unsigned thread, unsigned id)
{
    return cpu[thread].portctrl[id].stall;
}


uint64_t read_port_base_address(unsigned thread, unsigned id)
{
    auto scp_set = cpu[thread].portctrl[id].scp_set;
    auto scp_way = cpu[thread].portctrl[id].scp_way;
    return cpu[thread].core->scp_addr[scp_set][scp_way];
}


void set_delayed_msg_port_write(bool f)
{
    msg_port_delayed_write = f;
}


unsigned get_msg_port_write_width(unsigned thread, unsigned port)
{
    return 1ULL << cpu[thread].portctrl[port].logsize;
}


void write_msg_port_data(unsigned target_thread, unsigned id, unsigned source_thread, uint32_t *data)
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

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from m%u", target_thread, id, source_thread);
        for (unsigned w = 0; w < nwords; ++w) {
            LOG_NOTHREAD(DEBUG, "                              data[%u] 0x%8" PRIx32, w, data[w]);
        }
    }
    else
    {
        write_msg_port_data_to_scp(cpu[target_thread], id, data, 0);
    }
}


void write_msg_port_data_from_tbox(unsigned target_thread, unsigned id, unsigned tbox_id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = tbox_id;
        port_write.target_thread = target_thread;
        port_write.target_port   = id;
        port_write.is_tbox       = true;
        port_write.is_rbox       = false;

        unsigned nwords = (1ULL << cpu[target_thread].portctrl[id].logsize) / 4;
        for (unsigned w = 0; w < nwords; w++) {
            port_write.data[w] = data[w];
        }
        port_write.oob = oob;
        msg_port_pending_writes[target_thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from tbox%u", target_thread, id, tbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(cpu[target_thread], id, data, oob);
    }
}


void write_msg_port_data_from_rbox(unsigned target_thread, unsigned id, unsigned rbox_id, uint32_t *data, uint8_t oob)
{
    if (msg_port_delayed_write)
    {
        msg_port_write_t port_write;
        port_write.source_thread = rbox_id;
        port_write.target_thread = target_thread;
        port_write.target_port   = id;
        port_write.is_tbox       = false;
        port_write.is_rbox       = true;

        unsigned nwords = (1ULL << cpu[target_thread].portctrl[id].logsize) / 4;
        for (unsigned w = 0; w < nwords; w++) {
            port_write.data[w] = data[w];
        }
        port_write.oob = oob;
        msg_port_pending_writes[target_thread / EMU_THREADS_PER_SHIRE].push_back(port_write);

        LOG_NOTHREAD(DEBUG, "Delayed write on MSG_PORT (m%u p%u) from rbox%u", target_thread, id, rbox_id);
    }
    else
    {
        write_msg_port_data_to_scp(cpu[target_thread], id, data, oob);
    }
}


void commit_msg_port_data(unsigned target_thread, unsigned port_id, unsigned source_thread)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG_NOTHREAD(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG_NOTHREAD(DEBUG, "Commit write on MSG_PORT (h%u p%u) from h%u", target_thread, port_id, source_thread);
            write_msg_port_data_to_scp(cpu[target_thread], port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
        }
    }
    else
    {
        LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (h%u p%u) from h%u not found!!", target_thread, port_id, source_thread);
    }
}


void commit_msg_port_data_from_tbox(unsigned target_thread, unsigned port_id, unsigned tbox_id)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG_NOTHREAD(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG_NOTHREAD(DEBUG, "Commit write on MSG_PORT (m%u p%u) from tbox%u oob %d", target_thread, port_id, tbox_id, port_write.oob);
            write_msg_port_data_to_scp(cpu[target_thread], port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from tbox%u not found!!", target_thread, port_id, tbox_id);
        }
    }
    else
    {
        LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from tbox%u not found!!", target_thread, port_id, tbox_id);
    }
}


void commit_msg_port_data_from_rbox(unsigned target_thread, unsigned port_id, unsigned rbox_id)
{
    unsigned shire = target_thread / EMU_THREADS_PER_SHIRE;
    if (!msg_port_pending_writes[shire].empty())
    {
        msg_port_write_t port_write;
        bool found = false;

        LOG_NOTHREAD(INFO, "Pending MSG_PORT writes for Shire %u is %zu", shire, msg_port_pending_writes[shire].size());

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
            LOG_NOTHREAD(DEBUG, "Commit write on MSG_PORT (m%u p%u) from rbox%u", target_thread, port_id, rbox_id);
            write_msg_port_data_to_scp(cpu[target_thread], port_id, (uint32_t *) port_write.data, port_write.oob);
        }
        else
        {
            LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from rbox%u not found!!", target_thread, port_id, rbox_id);
        }
    }
    else
    {
        LOG_NOTHREAD(DEBUG, "ERROR Commit write on MSG_PORT (m%u p%u) from rbox%u not found!!", target_thread, port_id, rbox_id);
    }
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


} // namespace bemu
