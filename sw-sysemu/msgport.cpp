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
#include "decode.h"
#include "emu_defines.h"
#include "emu_gio.h"
#include "log.h"
#include "memop.h"
#include "processor.h"
#include "traps.h"

namespace bemu {


extern std::array<Hart,EMU_NUM_THREADS> cpu;

// MsgPort defines
#define PORT_LOG2_MIN_SIZE   2
#define PORT_LOG2_MAX_SIZE   5

extern bool scp_locked[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];
extern uint64_t scp_trans[EMU_NUM_MINIONS][L1D_NUM_SETS][L1D_NUM_WAYS];


enum msg_port_conf_action {
    MSG_ENABLE = 7,
    MSG_DISABLE = 3,
    MSG_PGET = 0,
    MSG_PGETNB = 1,
};


struct msg_port_conf_t {
    bool    enabled;
    bool    stall;
    bool    umode;
    bool    use_scp;
    bool    enable_oob;
    uint8_t logsize;
    uint8_t max_msgs;
    uint8_t scp_set;
    uint8_t scp_way;
    uint8_t rd_ptr;
    uint8_t wr_ptr;
    uint8_t size;
    int32_t offset;
};


struct msg_port_write_t {
    uint32_t source_thread;
    uint32_t target_thread;
    uint32_t target_port;
    bool     is_remote;
    bool     is_tbox;
    bool     is_rbox;
    uint32_t data[(1 << PORT_LOG2_MAX_SIZE)/4];
    uint8_t  oob;
};


static std::array<std::array<msg_port_conf_t,NR_MSG_PORTS>,EMU_NUM_THREADS>      msg_ports;
static std::array<std::array<std::deque<uint8_t>,NR_MSG_PORTS>,EMU_NUM_THREADS>  msg_ports_oob;

static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_tbox;
static std::array<std::vector<msg_port_write_t>,EMU_NUM_SHIRES> msg_port_pending_writes_rbox;

static bool msg_port_delayed_write = false;


void none_msg_to_thread(int /*thread_id*/) { }


void (*msg_to_thread)(int) = none_msg_to_thread;


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


void reset_msg_ports(unsigned thread)
{
    for (int i = 0; i < NR_MSG_PORTS; ++i) {
        memset(&msg_ports[thread][i], 0, sizeof(msg_port_conf_t));
        msg_ports[thread][i].offset = -1;
    }
}


void set_msg_funcs(void (*func_msg_to_thread)(int))
{
    msg_to_thread = func_msg_to_thread;
}


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


int64_t port_get(unsigned id, bool block)
{
    if (((PRV == PRV_U) && !msg_ports[current_thread][id].umode) || !msg_ports[current_thread][id].enabled)
    {
        throw trap_illegal_instruction(cpu[current_thread].inst.bits);
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


void configure_port(unsigned id, uint32_t wdata)
{
    int logsize = (wdata >> 5)  & 0x07;
    int scp_set = (wdata >> 16) & 0xFF;
    int scp_way = (wdata >> 24) & 0xFF;

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


} // namespace bemu
