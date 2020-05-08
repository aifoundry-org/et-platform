/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include <cstdio>
#include <cfenv>

#include "emu_gio.h"
#include "memop.h"
#include "msgport.h"
#include "rbox.h"

namespace bemu {
namespace RBOX {


#if (EMU_RBOXES_PER_SHIRE > 1)
RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES][EMU_RBOXES_PER_SHIRE];
#else
RBOXEmu rbox[EMU_NUM_COMPUTE_SHIRES];
#endif


void RBOXEmu::reset(uint32_t id)
{
    rbox_id = id;

    cfg_esr.value         = 0;
    in_buf_pg_esr.value   = 0;
    in_buf_cfg_esr.value  = 0;
    out_buf_pg_esr.value  = 0;
    out_buf_cfg_esr.value = 0;
    status_esr.value      = 0;
    start_esr.value       = 0;
    consume_esr.value     = 0;
    
    started = false;
}

void RBOXEmu::run(bool step_mode)
{
    if (!started && start_esr.fields.start)
    {
        // Verify parameters are correct, otherwise stop and set an error status.
        bool config_error =  ((in_buf_cfg_esr.fields.start_offset < 0x8000) && !in_buf_pg_esr.fields.page0_enable)
                          || ((in_buf_cfg_esr.fields.start_offset > 0x8000) && !in_buf_pg_esr.fields.page1_enable)
                          || ((in_buf_cfg_esr.fields.start_offset + in_buf_cfg_esr.fields.buffer_size) > 0xffff)
                          || (   ((in_buf_cfg_esr.fields.start_offset + in_buf_cfg_esr.fields.buffer_size - 1) > 0x8000)
                              && !in_buf_pg_esr.fields.page0_enable)
                          || !out_buf_pg_esr.fields.page_enable
                          || ((out_buf_cfg_esr.fields.start_offset + (1 << out_buf_cfg_esr.fields.buffer_size) * MINION_HARTS_PER_RBOX) > 0x8000);

        if (!config_error)
        {
            started = true;
            if (in_buf_cfg_esr.fields.start_offset < 0x8000)
                next_in_pckt_addr = ((uint64_t(in_buf_pg_esr.fields.page0) << 15)
                                  + uint64_t(in_buf_cfg_esr.fields.start_offset)) << 6;
            else
                next_in_pckt_addr = ((uint64_t(in_buf_pg_esr.fields.page1) << 15)
                                  + (uint64_t(in_buf_cfg_esr.fields.start_offset) - 0x8000ULL)) << 6;

            base_out_buf_addr = ((out_buf_pg_esr.fields.page << 15) + out_buf_cfg_esr.fields.start_offset) << 6;

            in_pckt_count  = 0;
            out_pckt_count = 0;

            last_in_pckt = false;

            for (uint32_t m = 0; m < MINION_HARTS_PER_RBOX; m++)
            {
                hart_packet_credits[m] = (1 << out_buf_cfg_esr.fields.buffer_size);
                hart_msg_credits[m] = out_buf_cfg_esr.fields.max_msgs;
                hart_ptr[m] = 0;
                hart_sent_packets[m] = 0;
                end_of_phase_sent[m] = false;
                send_message[m] = false;
            }
            
            flush_drawcall = false;
            
            status_esr.fields.status = WORKING;

            LOG_AGENT(DEBUG, *this, "RBOX %d Setting status to WORKING", rbox_id);
        }
        else
        {
            status_esr.fields.status = CONFIG_ERROR;

            LOG_AGENT(DEBUG, *this, "RBOX %d Setting status to ERROR", rbox_id);
        }
        start_esr.fields.start = 0;
    }

    bool stall = false;

    // Send quads to minions as long as credits are available
    while (started && !stall)
    {
        while (output_quads.empty() && !last_in_pckt && (!flush_drawcall || output_packets.empty()))
        {
            // Process input packets to get quads.
            uint32_t packet_size = process_packet(next_in_pckt_addr);
            next_in_pckt_addr += packet_size * 8;
            in_pckt_count++;
        }

        bool stall_for_credits = false;

        while (!stall_for_credits && !output_quads.empty() && (!step_mode || output_packets.empty()))
            stall_for_credits = send_quad_packet(step_mode);

        if (flush_drawcall && !output_quads.empty())
        {
            flush_drawcall = false;
            for (uint32_t m = 0; m < MINION_HARTS_PER_RBOX; m++)
            {
                if (hart_sent_packets[m] > 0)
                {
                    LOG_AGENT(DEBUG, *this, "RBOX [%02d] Force send on drawcall flush message port write for remaining packets for Minion HART %02d", rbox_id, m);
                    send_message[m] =  true;
                }
            }
        }

        stall = stall_for_credits || (step_mode && !output_packets.empty());

        bool end_of_phase_pending = stall || !last_in_pckt || !output_quads.empty();

        if (last_in_pckt && output_quads.empty())
        {
            for (uint32_t m = 0; (m < MINION_HARTS_PER_RBOX) && !stall; m++)
            {
                if (!end_of_phase_sent[m])
                {
                    end_of_phase_pending = true;
                    stall = !send_end_of_phase_packet(m, step_mode);
                    end_of_phase_sent[m] = !stall;
                }
            }
        }

        if (!output_packets.empty())
            write_next_packet();

        for (uint32_t m = 0; m < MINION_HARTS_PER_RBOX; m++)
        {
            if (!end_of_phase_pending && (hart_sent_packets[m] > 0))
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%02d] Force send last message port write for remaining packets for Minion HART %02d", rbox_id, m);
                send_message[m] =  true;
            }
        }

        bool sent_message = false;

        for (uint32_t m = 0; (m < MINION_HARTS_PER_RBOX) && !sent_message; m++)
        {
            if (send_message[m])
            {
                send_message[m] = !report_packets(m);
                sent_message = step_mode;
            }
        }

        if (!end_of_phase_pending && !sent_message)
        {
            started = false;
            status_esr.fields.status = FINISHED;

            LOG_AGENT(DEBUG, *this, "RBOX %d Setting status to FINISHED", rbox_id);
        }
    }
}

void RBOXEmu::write_esr(uint32_t esr_id, uint64_t data)
{
    switch (esr_id)
    {
        case CONFIG_ESR               : {
                                            cfg_esr.value = data;
                                            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Config = {.enable = %d}",
                                                         rbox_id, (uint32_t) cfg_esr.fields.enable);
                                            break;
                                        }
        case INPUT_BUFFER_PAGES_ESR   :
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                in_buf_pg_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Input Buffer Pages = {.page0 = %08x"
                                                           ", .page0_enable = %u, .page1 = %08x"
                                                           ", .page1_enable = %u}", rbox_id,
                                                           unsigned(in_buf_pg_esr.fields.page0), unsigned(in_buf_pg_esr.fields.page0_enable),
                                                           unsigned(in_buf_pg_esr.fields.page1), unsigned(in_buf_pg_esr.fields.page1_enable));
                                            }
                                            break;
                                        }
        case INPUT_BUFFER_CONFIG_ESR  : 
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                in_buf_cfg_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Input Buffer Config = {.start_offset = %08x"
                                                           ", .buffer_size = %08x}", rbox_id,
                                                           unsigned(in_buf_cfg_esr.fields.start_offset), unsigned(in_buf_cfg_esr.fields.buffer_size));
                                            }
                                            break;
                                        }
        case OUTPUT_BUFFER_PAGE_ESR   :
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                out_buf_pg_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Output Buffer Page = {.page = %08x"
                                                           ", .page_enable = %u}", rbox_id,
                                                           unsigned(out_buf_pg_esr.fields.page), unsigned(out_buf_pg_esr.fields.page_enable));
                                            }
                                            break;
                                        }
        case OUTPUT_BUFFER_CONFIG_ESR :
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                out_buf_cfg_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Output Buffer Config = {.start_offset = %08x"
                                                           ", .buffer_size = %08x, .port_id = %u"
                                                           ", .max_msgs = %u, .max_pckts_msg = %02u}", rbox_id,
                                                           unsigned(out_buf_cfg_esr.fields.start_offset), unsigned(out_buf_cfg_esr.fields.buffer_size),
                                                           unsigned(out_buf_cfg_esr.fields.port_id), unsigned(out_buf_cfg_esr.fields.max_msgs),
                                                           unsigned(out_buf_cfg_esr.fields.max_pckts_msg));
                                            }
                                            break;
                                        }
        case STATUS_ESR               : /* Read Only */ break;
        case START_ESR                : 
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                start_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Start = {.start = %d}", rbox_id, (uint32_t) start_esr.fields.start);
                                            }
                                            break;
                                        }
        case CONSUME_ESR              :
                                        {
                                            if (cfg_esr.fields.enable)
                                            {
                                                consume_esr.value = data;
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Consume = {.packet_credits = %03u, .msg_credits = %u"
                                                                    ", .hart_id = %02u}",
                                                            rbox_id, unsigned(consume_esr.fields.packet_credits),
                                                            unsigned(consume_esr.fields.msg_credits), unsigned(consume_esr.fields.hart_id));
                                                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Minion HART %02u Packet Credits %03" PRIu32 " -> %03" PRIu32
                                                                    "  Message Credits %" PRIu32 " -> %" PRIu32,
                                                            rbox_id, unsigned(consume_esr.fields.hart_id),
                                                            hart_packet_credits[consume_esr.fields.hart_id],
                                                            hart_packet_credits[consume_esr.fields.hart_id] + uint32_t(consume_esr.fields.packet_credits),
                                                            hart_msg_credits[consume_esr.fields.hart_id],
                                                            hart_msg_credits[consume_esr.fields.hart_id]    + uint32_t(consume_esr.fields.msg_credits));
                                                hart_packet_credits[consume_esr.fields.hart_id] += consume_esr.fields.packet_credits;
                                                hart_msg_credits[consume_esr.fields.hart_id]    += consume_esr.fields.msg_credits;
                                            }
                                            break;
                                        }
        default                       :
                                        {
                                            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Write to undefined ESR %d", rbox_id, esr_id);
                                            break;
                                        }

    }
}

uint64_t RBOXEmu::read_esr(uint32_t esr_id)
{
    switch (esr_id)
    {
        case CONFIG_ESR               : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read CONFIG ESR with value %016" PRIx64, rbox_id, cfg_esr.value);
                                        return cfg_esr.value;
        case INPUT_BUFFER_PAGES_ESR   : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read INPUT BUFFER PAGES ESR with value %016" PRIx64,
                                                    rbox_id, in_buf_pg_esr.value);
                                        return in_buf_pg_esr.value; 
        case INPUT_BUFFER_CONFIG_ESR  : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read INPUT BUFFER CONFIG ESR with value %016" PRIx64,
                                                   rbox_id, in_buf_cfg_esr.value);
                                        return in_buf_cfg_esr.value;
        case OUTPUT_BUFFER_PAGE_ESR   : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read OUTPUT BUFFER PAGE ESR with value %016" PRIx64,
                                                   rbox_id, out_buf_pg_esr.value);
                                        return out_buf_pg_esr.value;
        case OUTPUT_BUFFER_CONFIG_ESR : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read OUTPUT BUFFER CONFIG ESR with value %016" PRIx64,
                                                   rbox_id, out_buf_cfg_esr.value);
                                        return out_buf_cfg_esr.value;
        case STATUS_ESR               : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read STATUS ESR with value %016" PRIx64,
                                                   rbox_id, status_esr.value);
                                        return status_esr.value;
        case START_ESR                : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read START ESR with value %016" PRIx64,
                                                   rbox_id, start_esr.value);
                                        return start_esr.value;
        case CONSUME_ESR              : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read CONSUME ESR with value %016" PRIx64,
                                                   rbox_id, consume_esr.value);
                                        return consume_esr.value;
        default                       : LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Read to UNDEFINED ESR", rbox_id);
                                        return 0;
    }
}

uint32_t RBOXEmu::process_packet(uint64_t packet)
{
    InPcktHeaderT header;
    header.qw = bemu::pmemread<uint64_t>(*this, packet);

    uint32_t packet_size = 0;

    switch (header.type)
    {
        case INPCKT_FULLY_COVERED_TILE:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing fully covered tile packet", rbox_id);
                InPcktFullyCoveredTileT fully_covered_tile_pckt;
                
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    fully_covered_tile_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, fully_covered_tile_pckt.qw[qw]);
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = fully_covered_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_64X64_SAMPLE_FRAC_BITS);
                DepthT depth_sample = fully_covered_tile_pckt.tile.depth;
                TileSizeT tile_size = (TileSizeT) fully_covered_tile_pckt.tile.tile_size;
                uint32_t tile_x = fully_covered_tile_pckt.tile.tile_left;
                uint32_t tile_y = fully_covered_tile_pckt.tile.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 4;
            }
            break;
        case INPCKT_LARGE_TRIANGLE_TILE:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing partially covered tile packet", rbox_id);
                InPcktLargeTriTileT large_tri_tile_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    large_tri_tile_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, large_tri_tile_pckt.qw[qw]);
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = large_tri_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_LARGE_TRI_SAMPLE_FRAC_BITS);
                DepthT depth_sample = large_tri_tile_pckt.tile.depth;
                TileSizeT tile_size = (TileSizeT) large_tri_tile_pckt.tile.tile_size;
                uint32_t tile_x = large_tri_tile_pckt.tile.tile_left;
                uint32_t tile_y = large_tri_tile_pckt.tile.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 4;
            }
            break;
        case INPCKT_TRIANGLE_WITH_TILE_64x64:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing triangle with tile, triangle inside 64x64 tile aligned 64x64 tile", rbox_id);
                InPcktTriWithTile64x64T tri_with_tile_64x64_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_64x64_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, tri_with_tile_64x64_pckt.qw[qw]);
                }

                for (uint32_t eq = 0; eq < 3; eq++)
                {
                    current_triangle.edge_eqs[eq].a = tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge_eqs[eq].a
                                                    << (EDGE_EQ_COEF_FRAC_BITS - EDGE_EQ_64X64_COEF_FRAC_BITS);
                    current_triangle.edge_eqs[eq].b = tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge_eqs[eq].b
                                                    << (EDGE_EQ_COEF_FRAC_BITS - EDGE_EQ_64X64_COEF_FRAC_BITS);

                    current_triangle.top_or_left_edge[eq] =
                        (current_triangle.edge_eqs[eq].a == 0) ? ((current_triangle.edge_eqs[eq].b & EDGE_EQ_SAMPLE_SIGN_MASK) != 0)
                                                               : ((current_triangle.edge_eqs[eq].a & EDGE_EQ_SAMPLE_SIGN_MASK) == 0);
                }
                current_triangle.depth_eq = tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth_eq;
                current_triangle.triangle_data_ptr = tri_with_tile_64x64_pckt.tri_with_tile_64x64.triangle_data_ptr;
                current_triangle.back_facing = (tri_with_tile_64x64_pckt.tri_with_tile_64x64.tri_facing == TRI_FACING_BACK);
                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_64X64_SAMPLE_FRAC_BITS);
                DepthT depth_sample = tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth;
                TileSizeT tile_size = (TileSizeT) tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_size;
                uint32_t tile_x = tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_left;
                uint32_t tile_y = tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 8;
            }
            break;
        case INPCKT_TRIANGLE_WITH_TILE_128x128:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing triangle with tile, triangle inside 64x64 tile aligned 128x128 tile", rbox_id);
                InPcktTriWithTile128x128T tri_with_tile_128x128_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_128x128_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, tri_with_tile_128x128_pckt.qw[qw]);
                }
                for (uint32_t eq = 0; eq < 3; eq++)
                {
                    current_triangle.edge_eqs[eq].a = tri_with_tile_128x128_pckt.tri_with_tile_128x128.edge_eqs[eq].a
                                                     << (EDGE_EQ_COEF_FRAC_BITS - EDGE_EQ_128X128_COEF_FRAC_BITS);
                    current_triangle.edge_eqs[eq].b = tri_with_tile_128x128_pckt.tri_with_tile_128x128.edge_eqs[eq].b
                                                     << (EDGE_EQ_COEF_FRAC_BITS - EDGE_EQ_128X128_COEF_FRAC_BITS);

                    current_triangle.top_or_left_edge[eq] =
                        (current_triangle.edge_eqs[eq].a == 0) ? ((current_triangle.edge_eqs[eq].b & EDGE_EQ_SAMPLE_SIGN_MASK) != 0)
                                                               : ((current_triangle.edge_eqs[eq].a & EDGE_EQ_SAMPLE_SIGN_MASK) == 0);
                }
                current_triangle.depth_eq = tri_with_tile_128x128_pckt.tri_with_tile_128x128.depth_eq;
                current_triangle.triangle_data_ptr = tri_with_tile_128x128_pckt.tri_with_tile_128x128.triangle_data_ptr;
                current_triangle.back_facing = (tri_with_tile_128x128_pckt.tri_with_tile_128x128.tri_facing == TRI_FACING_BACK);
                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = tri_with_tile_128x128_pckt.tri_with_tile_128x128.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_128X128_SAMPLE_FRAC_BITS);
                DepthT depth_sample = tri_with_tile_128x128_pckt.tri_with_tile_128x128.depth;
                TileSizeT tile_size = (TileSizeT) tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_size;
                uint32_t tile_x = tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_left;
                uint32_t tile_y = tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 8;
            }
            break;
        case INPCKT_LARGE_TRIANGLE:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing large triangle packet", rbox_id);
                InPcktLargeTriT large_tri_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    large_tri_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, large_tri_pckt.qw[qw]);
                }

                for (uint32_t eq = 0; eq < 3; eq++)
                {
                    current_triangle.edge_eqs[eq].a = (uint64_t(large_tri_pckt.triangle.edge_eqs[eq].a_high) << 32)
                                                    |  (int64_t) large_tri_pckt.triangle.edge_eqs[eq].a_low;
                    current_triangle.edge_eqs[eq].b = (uint64_t(large_tri_pckt.triangle.edge_eqs[eq].b_high) << 32)
                                                    |  (int64_t) large_tri_pckt.triangle.edge_eqs[eq].b_low;

                    current_triangle.top_or_left_edge[eq] =
                        (current_triangle.edge_eqs[eq].a == 0) ? ((current_triangle.edge_eqs[eq].b & EDGE_EQ_SAMPLE_SIGN_MASK) != 0)
                                                               : ((current_triangle.edge_eqs[eq].a & EDGE_EQ_SAMPLE_SIGN_MASK) == 0);
                }
                current_triangle.depth_eq = large_tri_pckt.triangle.depth_eq;
                current_triangle.triangle_data_ptr = large_tri_pckt.triangle.triangle_data_ptr;
                current_triangle.back_facing = (large_tri_pckt.triangle.tri_facing == TRI_FACING_BACK);
                packet_size = 8;
            }
            break;
        case INPCKT_RBOX_STATE:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing RBOX state packet", rbox_id);
                InPcktRBOXStateT rbox_state_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    rbox_state_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw, rbox_state_pckt.qw[qw]);
                }
                rbox_state = rbox_state_pckt.state;
                packet_size = 8;
            }
            break;
        case INPCKT_FRAG_SHADING_STATE:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Processing fragment shading state packet", rbox_id);
                InPcktFrgmtShdrStateT frag_shader_state_pckt;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    frag_shader_state_pckt.qw[qw] = bemu::pmemread<uint64_t>(*this, packet + qw * 8);
                    LOG_AGENT(DEBUG, *this, "\t[%u] = %016" PRIx64, qw,frag_shader_state_pckt.qw[qw]);
                }
                frag_shader_state = frag_shader_state_pckt.state;
                packet_size = 4;

                for (uint32_t m = 0; m < MINION_HARTS_PER_RBOX; m++)
                    fsh_state_sent[m] = false;

                flush_drawcall = true;
            }
            break;
        case INPCKT_END_OF_INPUT_BUFFER:
            {
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Received end of input buffer packet", rbox_id);
                last_in_pckt = true;
            }
            break;
        default :
            break;
    }

    return packet_size;
}

const uint32_t RBOXEmu::tile_dimensions[6][2] = {{64, 64}, {64, 32}, {32, 32}, {16, 16}, {8, 8}, {4, 4}};

void RBOXEmu::generate_tile(uint32_t tile_x, uint32_t tile_y, int64_t edge_samples[3], DepthT depth_sample, TileSizeT tile_sz)
{
    TriangleSampleT tile_sample;
    for (uint32_t eq = 0; eq < 3; eq++)
        tile_sample.edge[eq] = edge_samples[eq];
    tile_sample.depth = depth_sample;

    TriangleSampleT row_sample[4];

    row_sample[0] = tile_sample;
    row_sample[1] = tile_sample;
    row_sample[2] = tile_sample;
    row_sample[3] = tile_sample;

    sample_first_quad(row_sample);

    uint32_t generated_quads_in_tile = 0;

    for (uint32_t y = 0; y < tile_dimensions[tile_sz][1]; y += 2)
    {
        TriangleSampleT quad_sample[4];
        quad_sample[0] = row_sample[0];
        quad_sample[1] = row_sample[1];
        quad_sample[2] = row_sample[2];
        quad_sample[3] = row_sample[3];

        for (uint32_t x = 0; x < tile_dimensions[tile_sz][0]; x += 2)
        {
            QuadInfoT quad;

            sample_quad(tile_x + x, tile_y + y, quad_sample, quad);

            bool quad_coverage = test_quad(quad);

            if (quad_coverage)
            {
                uint32_t target_minion_hart = compute_target_minion_hart(tile_x + x, tile_y + y);

                output_quads.push_back(quad);

                generated_quads_in_tile++;

                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Generated packet for quad at (%d, %d) for minion hart %d", rbox_id, tile_x + x, tile_y + y, target_minion_hart);
            }

            sample_next_quad(quad_sample);
        }
        sample_next_row(row_sample);
    }

    LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Generated %d quads in tile", rbox_id, generated_quads_in_tile);
}

void RBOXEmu::sample_next_row(TriangleSampleT sample[4])
{
    for (uint32_t fr = 0; fr < 4; fr++)
    {
        for (uint32_t eq = 0; eq < 3; eq++)
            sample[fr].edge[eq] += 2 * current_triangle.edge_eqs[eq].b;

        int saved_round_mode = std::fegetround();
        std::fesetround(FE_TOWARDZERO);
        sample[fr].depth.f += 2 * current_triangle.depth_eq.b.f;
        std::fesetround(saved_round_mode);
    }
}

void RBOXEmu::sample_next_quad(TriangleSampleT sample[4])
{
    for(uint32_t fr = 0; fr < 4; fr++)
    {
        for (uint32_t eq = 0; eq < 3; eq++)
            sample[fr].edge[eq] += 2 * current_triangle.edge_eqs[eq].a;

        int saved_round_mode = std::fegetround();
        std::fesetround(FE_TOWARDZERO);
        sample[fr].depth.f += 2.0f * current_triangle.depth_eq.a.f;
        std::fesetround(saved_round_mode);
    }
}

void RBOXEmu::sample_first_quad(TriangleSampleT sample[4])
{
    for (uint32_t eq = 0; eq < 3; eq++)
    {
        sample[1].edge[eq] +=  current_triangle.edge_eqs[eq].a;
        sample[2].edge[eq] +=  current_triangle.edge_eqs[eq].b;
        sample[3].edge[eq] += (current_triangle.edge_eqs[eq].a + current_triangle.edge_eqs[eq].b);
    }

    int saved_round_mode = std::fegetround();
    std::fesetround(FE_TOWARDZERO);
    sample[1].depth.f +=  current_triangle.depth_eq.a.f;
    sample[2].depth.f +=  current_triangle.depth_eq.b.f;
    sample[3].depth.f += (current_triangle.depth_eq.a.f + current_triangle.depth_eq.b.f);
    std::fesetround(saved_round_mode);
}

void RBOXEmu::sample_quad(uint32_t x, uint32_t y, TriangleSampleT quad_sample[4], QuadInfoT &quad)
{
    LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Sampling quad at (%d, %d) -> start sample = (%016" PRIx64 ", %016" PRIx64 ", %016" PRIx64 ", %f [%08x] )\n"
                        "                          Edge 0 Equation = {%016" PRIx64 ", %016" PRIx64 "}\n"
                        "                          Edge 1 Equation = {%016" PRIx64 ", %016" PRIx64 "}\n"
                        "                          Edge 2 Equation = {%016" PRIx64 ", %016" PRIx64 "}\n"
                        "                          Depth Equation  = {%f [%08x], %f [%08x]}\n"
                        "                          top_or_left_edges = (%d, %d, %d)",
                       rbox_id, x, y,
                       quad_sample[0].edge[0], quad_sample[0].edge[1], quad_sample[0].edge[2],
                       quad_sample[0].depth.f, quad_sample[0].depth.u,
                       current_triangle.edge_eqs[0].a, current_triangle.edge_eqs[0].b,
                       current_triangle.edge_eqs[1].a, current_triangle.edge_eqs[1].b,
                       current_triangle.edge_eqs[2].a, current_triangle.edge_eqs[2].b,
                       current_triangle.depth_eq.a.f, current_triangle.depth_eq.a.u,
                       current_triangle.depth_eq.b.f, current_triangle.depth_eq.b.u,
                       current_triangle.top_or_left_edge[0],
                       current_triangle.top_or_left_edge[1],
                       current_triangle.top_or_left_edge[2]);

    for (uint32_t fr = 0; fr < 4; fr++)
    {
        for (uint32_t eq = 0; eq < 3; eq++)
            quad.fragment[fr].sample.edge[eq] = quad_sample[fr].edge[eq];
        quad.fragment[fr].sample.depth.f = quad_sample[fr].depth.f;

        quad.fragment[fr].coverage = sample_inside_triangle(quad.fragment[fr].sample);
        LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Fragment %d Sample (%016" PRIx64 ", %016" PRIx64 ", %016" PRIx64 ", %f [%08x]) Coverage = %d", rbox_id, fr,
                          quad.fragment[fr].sample.edge[0], quad.fragment[fr].sample.edge[1], quad.fragment[fr].sample.edge[2],
                          quad.fragment[fr].sample.depth.f, quad.fragment[fr].sample.depth.u, quad.fragment[fr].coverage);
    }

    quad.x = x;
    quad.y = y;
    quad.triangle_data_ptr = current_triangle.triangle_data_ptr;
}

bool RBOXEmu::test_quad(QuadInfoT &quad)
{
    bool quad_coverage = false;

    //
    // ONLY UNORM24 IMPLEMENTED
    //

    for (uint32_t f = 0; f < 4; f++)
    {
        int32_t x = quad.x + (f & 0x1);
        int32_t y = quad.y + ((f >> 1) & 0x1);
        bool scissor_test = do_scissor_test(x, y);
        quad.fragment[f].coverage = quad.fragment[f].coverage && scissor_test;

        if (quad.fragment[f].coverage)
        {
            uint64_t frag_depth_stencil_address = compute_depth_stencil_buffer_address(x, y);
            uint32_t frag_depth_stencil = bemu::pmemread<uint32_t>(*this, frag_depth_stencil_address);

            uint8_t frag_stencil = frag_depth_stencil >> 24;
            uint32_t frag_depth = frag_depth_stencil & 0x00FFFFFF;
    
            uint32_t sample_depth = uint32_t(quad.fragment[f].sample.depth.f * ((1 << 24) - 1));

            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Testing fragment at (%d, %d) address = %016" PRIx64 " sample_depth = %08x fragment_depth_stencil = %08x",
                              rbox_id, x, y, frag_depth_stencil_address, sample_depth, frag_depth_stencil);

            bool depth_bound_test =  do_depth_bound_test(frag_depth);
            bool stencil_test = do_stencil_test(frag_stencil);
            bool depth_test = do_depth_test(frag_depth, sample_depth);

            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Test results : depth_bound = %d stencil = %d depth = %d", rbox_id, depth_bound_test, stencil_test, depth_test);

            uint8_t out_stencil = stencil_update(frag_stencil, stencil_test, depth_test);

            uint32_t out_depth;
            if (depth_bound_test)
            {
                if (stencil_test)
                    out_depth = rbox_state.depth_test_write_enable ? sample_depth : frag_depth;
                else
                    out_depth = frag_depth;

                uint32_t out_depth_stencil = (out_stencil << 24) | out_depth;
                bemu::pmemwrite<uint32_t>(*this, frag_depth_stencil_address, out_depth_stencil);
            }

            quad.fragment[f].coverage = quad.fragment[f].coverage && depth_bound_test && stencil_test && depth_test;
        }

        quad_coverage |= quad.fragment[f].coverage;
    }

    return quad_coverage;
}

uint64_t RBOXEmu::compute_depth_stencil_buffer_address(uint32_t x, uint32_t y)
{
    uint32_t row = y / 4;
    uint32_t line = x / 4;
    uint64_t depth_stencil_buffer_offset = row * rbox_state.depth_stencil_buffer_row_pitch
                                       + line * 64 + (y & 0x3) * 16 + (x & 0x03) * 4;
    return rbox_state.depth_stencil_buffer_ptr + depth_stencil_buffer_offset;
}

bool RBOXEmu::sample_inside_triangle(TriangleSampleT sample)
{
    return ((sample.edge[0] == 0) ? current_triangle.top_or_left_edge[0] : ((sample.edge[0] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[1] == 0) ? current_triangle.top_or_left_edge[1] : ((sample.edge[1] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[2] == 0) ? current_triangle.top_or_left_edge[2] : ((sample.edge[2] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0));
}

bool RBOXEmu::do_scissor_test(int32_t x, int32_t y)
{
    LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Scissor test for fragment at (%" PRId32 ", %" PRId32 ") scissor rectangle (%u, %u, %u, %u)",
                      rbox_id, x, y, unsigned(rbox_state.scissor_start_x), unsigned(rbox_state.scissor_start_y),
                      unsigned(rbox_state.scissor_width), unsigned(rbox_state.scissor_height));

    return    (x >=  rbox_state.scissor_start_x)
           && (y >=  rbox_state.scissor_start_y)
           && (x <= (rbox_state.scissor_start_x + rbox_state.scissor_width))
           && (y <= (rbox_state.scissor_start_y + rbox_state.scissor_height));
}

bool RBOXEmu::do_depth_bound_test(uint32_t frag_depth)
{
    return   !rbox_state.depth_bound_enable
           || (    (frag_depth >= rbox_state.depth_bound_min)
               &&  (frag_depth <= rbox_state.depth_bound_max));
}

bool RBOXEmu::do_stencil_test(uint8_t frag_stencil)
{
    StencilStateT stencil_state;
    if (current_triangle.back_facing)
        stencil_state = rbox_state.stencil_back_state;
    else
        stencil_state = rbox_state.stencil_front_state;

    if (rbox_state.stencil_test_enable)
    {
        uint8_t ref_stencil_masked = stencil_state.ref & stencil_state.compare_mask;
        uint8_t frag_stencil_masked = frag_stencil & stencil_state.compare_mask;
        switch (stencil_state.compare_op)
        {
            case COMPARE_OP_NEVER :
                return false;
            case COMPARE_OP_LESS :
                return ref_stencil_masked < frag_stencil_masked;
            case COMPARE_OP_EQUAL :
                return ref_stencil_masked == frag_stencil_masked;
            case COMPARE_OP_LESS_OR_EQUAL :
                return ref_stencil_masked <= frag_stencil_masked;
            case COMPARE_OP_GREATER :
                return ref_stencil_masked > frag_stencil_masked;
            case COMPARE_OP_NOT_EQUAL :
                return ref_stencil_masked != frag_stencil_masked;
            case COMPARE_OP_GREATER_OR_EQUAL :
                return ref_stencil_masked >= frag_stencil_masked;
            case COMPARE_OP_ALWAYS :
                return true;
        }
    }
    return true;
}

bool RBOXEmu::do_depth_test(uint32_t frag_depth, uint32_t sample_depth)
{
    if (rbox_state.depth_test_enable)
    {
        switch (rbox_state.depth_test_compare_op)
        {
            case COMPARE_OP_NEVER :
                return false;
            case COMPARE_OP_LESS :
                return sample_depth < frag_depth;
            case COMPARE_OP_EQUAL :
                return sample_depth == frag_depth;
            case COMPARE_OP_LESS_OR_EQUAL :
                return sample_depth <= frag_depth;
            case COMPARE_OP_GREATER :
                return sample_depth > frag_depth;
            case COMPARE_OP_NOT_EQUAL :
                return sample_depth != frag_depth;
            case COMPARE_OP_GREATER_OR_EQUAL :
                return sample_depth >= frag_depth;
            case COMPARE_OP_ALWAYS :
                return true;
        }
    }
    return true;
}

uint8_t RBOXEmu::stencil_update(uint8_t frag_stencil, bool stencil_test, bool depth_test)
{
    StencilStateT stencil_state;
    if (current_triangle.back_facing)
        stencil_state = rbox_state.stencil_back_state;
    else
        stencil_state = rbox_state.stencil_front_state;

    StencilOpT stencil_op;
    if (rbox_state.stencil_test_enable)
    {
        if (stencil_test && depth_test)
            stencil_op = (StencilOpT) stencil_state.pass_op;
        else if (stencil_test && !depth_test)
            stencil_op = (StencilOpT) stencil_state.depth_fail_op;
        else
            stencil_op = (StencilOpT) stencil_state.fail_op;

        switch (stencil_op)
        {
            case STENCIL_OP_KEEP :
                return frag_stencil & stencil_state.write_mask;
            case STENCIL_OP_ZERO :
                return 0;
            case STENCIL_OP_REPLACE :
                return stencil_state.ref & stencil_state.write_mask;
            case STENCIL_OP_INC_CLAMP :
                return ((frag_stencil == 255) ? 255 : (frag_stencil + 1)) & stencil_state.write_mask;
            case STENCIL_OP_DEC_CLAMP :
                return ((frag_stencil == 0) ? 0 : (frag_stencil - 1)) & stencil_state.write_mask;
            case STENCIL_OP_INVERT :
                return (~frag_stencil) & stencil_state.write_mask;
            case STENCIL_OP_INC_WRAP :
                return (frag_stencil + 1) & stencil_state.write_mask;
            case STENCIL_OP_DEC_WRAP :
                return (frag_stencil - 1) & stencil_state.write_mask;
            default :
                return 0;
        }
    }
    return frag_stencil;
}

bool RBOXEmu::send_quad_packet(bool step_mode)
{
    if (!rbox_state.fragment_shader_disabled && !output_quads.empty())
    {
        QuadInfoT quad[2];

        bool insert_fake_quad = false;

        if (output_quads.size() > 1)
        {
            uint32_t target_minion_hart[2];

            target_minion_hart[0] = compute_target_minion_hart(output_quads[0].x, output_quads[0].y);
            target_minion_hart[1] = compute_target_minion_hart(output_quads[1].x, output_quads[1].y);

            insert_fake_quad = (target_minion_hart[0] != target_minion_hart[1]);
        }
        else 
            insert_fake_quad = true;

        if (insert_fake_quad)
        {
            quad[0] = output_quads[0];
            quad[1].x = 0;
            quad[1].y = 0;
            quad[1].triangle_data_ptr = 0;

            for (uint32_t f = 0; f < 4; f++)
            {
                quad[1].fragment[f].coverage = false;
                for (uint32_t e = 0; e < 3; e++)
                    quad[1].fragment[f].sample.edge[e] = 0;
                quad[1].fragment[f].sample.depth.u = 0;
            }

            output_quads.erase(output_quads.begin());
        }
        else
        {
            quad[0] = output_quads[0];
            quad[1] = output_quads[1];

            output_quads.erase(output_quads.begin());
            output_quads.erase(output_quads.begin());
        }

        uint32_t target_minion_hart = compute_target_minion_hart(quad[0].x, quad[0].y);
    
        if (insert_fake_quad)
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => For Minion HART %02d Insert Fake Quad Pair", rbox_id, target_minion_hart);

        if (!fsh_state_sent[target_minion_hart])
            fsh_state_sent[target_minion_hart] = send_frag_shader_state_packet(target_minion_hart, step_mode);

        if (!fsh_state_sent[target_minion_hart])
        {
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => For Minion HART %02d STALL : Drawcall state packet couldn't be send", rbox_id, target_minion_hart);
            return true;
        }

        uint32_t packets_per_sample = rbox_state.fragment_shader_reads_bary * 2
                                    + rbox_state.fragment_shader_reads_depth 
                                    + rbox_state.fragment_shader_reads_coverage;
        uint32_t num_packets = ((rbox_state.fragment_shader_per_sample && rbox_state.msaa_enable) ? (packets_per_sample * rbox_state.msaa_samples)
                                                                                                   :  packets_per_sample) + 1;
    
        LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Target Minion HART %02d Packet Credits %03d Required Packets Per Sample %d Total Packets per Quad Message %d",
                   rbox_id, target_minion_hart, hart_packet_credits[target_minion_hart], packets_per_sample, num_packets);

        if ((hart_packet_credits[target_minion_hart] >= num_packets) &&
            ((hart_sent_packets[target_minion_hart] + num_packets) <= (1U << out_buf_cfg_esr.fields.max_pckts_msg)))
        {
            OutPcktQuadInfoT quad_info_pckt;
            
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Generate quad packet for position (%05d, %05d)", rbox_id, quad[0].x, quad[0].y);
            
            for (uint32_t qw = 0; qw < 4; qw++)
                quad_info_pckt.qw[qw] = 0;
           
            quad_info_pckt.quad_info.type = OUTPCKT_QUAD_INFO;
            for (uint32_t q = 0; q < 2; q++)
            {
                quad_info_pckt.quad_info.x[q] = quad[q].x;
                quad_info_pckt.quad_info.y[q] = quad[q].y;
                quad_info_pckt.quad_info.triangle_data_ptr[q] = quad[q].triangle_data_ptr;
            }
            quad_info_pckt.quad_info.smpl_idx = 0;
            for (uint32_t f = 0; f < (2 * 4); f++)
                quad_info_pckt.quad_info.mask = quad[f / 4].fragment[f % 4].coverage ? (quad_info_pckt.quad_info.mask | (1 << f)) : quad_info_pckt.quad_info.mask;
    
            uint64_t minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

            send_packet(target_minion_hart, quad_info_pckt.qw, minion_hart_out_addr, step_mode);

            if (rbox_state.fragment_shader_reads_bary)
            {
                OutPcktQuadDataT quad_data_pckt;
                
                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = convert_edge_to_fp32(quad[f / 4].fragment[f % 4].sample.edge[0]);
                
                minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

                send_packet(target_minion_hart, quad_data_pckt.qw, minion_hart_out_addr, step_mode);

                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = convert_edge_to_fp32(quad[f / 4].fragment[f % 4].sample.edge[1]);
                
                minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

                send_packet(target_minion_hart, quad_data_pckt.qw, minion_hart_out_addr, step_mode);
            }

            if (rbox_state.fragment_shader_reads_depth)
            {
                OutPcktQuadDataT quad_data_pckt;
                
                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = quad[f / 4].fragment[f % 4].sample.depth.f;
                
                minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

                send_packet(target_minion_hart, quad_data_pckt.qw, minion_hart_out_addr, step_mode);
            }

            hart_packet_credits[target_minion_hart] -= num_packets;

            return false;   
        }
        else
        {
            if (hart_packet_credits[target_minion_hart] < num_packets) 
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => For Minion HART %02d STALL : No packet credits available for next quad", rbox_id, target_minion_hart);

            if ((hart_sent_packets[target_minion_hart] + num_packets) > (1U << out_buf_cfg_esr.fields.max_pckts_msg))
                LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => For Minion HART %02d STALL : Pending message port write", rbox_id, target_minion_hart);

            send_message[target_minion_hart] = ((hart_sent_packets[target_minion_hart] + num_packets) > (1U << out_buf_cfg_esr.fields.max_pckts_msg));
            return true;
        }
    }

    return false;
}

bool RBOXEmu::send_frag_shader_state_packet(uint32_t target_minion_hart, bool step_mode)
{
    if ((hart_packet_credits[target_minion_hart] > 0) &&
        (hart_sent_packets[target_minion_hart] < (1U << out_buf_cfg_esr.fields.max_pckts_msg)))
    {
        OutPcktFrgShdrStateT f_sh_pckt;

        LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Generate Fragment Shader State Packet", rbox_id);

        for (uint32_t qw = 0; qw < 4; qw++)
            f_sh_pckt.qw[qw] = 0;

        f_sh_pckt.state.type = OUTPCKT_STATE_INFO;
        f_sh_pckt.state.frg_shdr_func_ptr = frag_shader_state.frag_shader_function_ptr;
        f_sh_pckt.state.frg_shdr_state_ptr = frag_shader_state.frag_shader_state_ptr;

        uint64_t minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

        send_packet(target_minion_hart, f_sh_pckt.qw, minion_hart_out_addr, step_mode);
        
        hart_packet_credits[target_minion_hart]--;

        return true;
    }
    else
    {
        send_message[target_minion_hart] = (hart_sent_packets[target_minion_hart] >= (1U << out_buf_cfg_esr.fields.max_pckts_msg));

        return false;
    }
}

bool RBOXEmu::send_end_of_phase_packet(uint32_t target_minion_hart, bool step_mode)
{
    if ((hart_packet_credits[target_minion_hart] > 0) &&
        (hart_sent_packets[target_minion_hart] < (1U << out_buf_cfg_esr.fields.max_pckts_msg)))
    {
        OutPckt256bT end_phase_pckt;

        LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Generate End of Phase Packet for HART %02d", rbox_id, target_minion_hart);

        for (uint32_t qw = 0; qw < 4; qw++)
            end_phase_pckt.qw[qw] = 0;

        end_phase_pckt.header.type = OUTPCKT_END_PHASE;

        uint64_t minion_hart_out_addr = compute_minion_hart_out_addr(target_minion_hart);

        send_packet(target_minion_hart, end_phase_pckt.qw, minion_hart_out_addr, step_mode);
        
        hart_packet_credits[target_minion_hart]--;

        return true;
    }
    else
    {
        send_message[target_minion_hart] = (hart_sent_packets[target_minion_hart] >= (1U << out_buf_cfg_esr.fields.max_pckts_msg));

        return false;
    }
}


float RBOXEmu::convert_edge_to_fp32(int64_t edge)
{
    bool sign = ((edge & EDGE_EQ_SAMPLE_SIGN_MASK) != 0);
    uint64_t edge_abs = sign ? -edge : edge;
    float edge_abs_fp32 = float(edge_abs) / float(1 << EDGE_EQ_SAMPLE_FRAC_BITS);
    return (sign ? -edge_abs_fp32 : edge_abs_fp32);
}

float RBOXEmu::convert_depth_to_fp32(uint32_t depth)
{
    return float(depth) / float((1 << 24) - 1);
}

void RBOXEmu::tile_position_to_pixels(uint32_t &tile_x, uint32_t &tile_y, TileSizeT tile_size)
{
    switch(tile_size)
    {
        case TILE_SIZE_64x64 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 6;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 64x64 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_64x32 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 5;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 64x32 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_32x32 :
            tile_x = tile_x << 5;
            tile_y = tile_y << 5;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 32x32 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_16x16 :
            tile_x = tile_x << 4;
            tile_y = tile_y << 4;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 16x16 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_8x8 :
            tile_x = tile_x << 3;
            tile_y = tile_y << 3;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 8x8 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_4x4 :
            tile_x = tile_x << 2;
            tile_y = tile_y << 2;
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Tile Size 4x4 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
    }
}

uint32_t RBOXEmu::compute_target_minion_hart(uint32_t x, uint32_t y)
{
    // Minions in a Shire are distributed in rows of Shire Layout With.
    
    return    ((x >> (rbox_state.minion_hart_tile_width  + 1)) & ((1 << rbox_state.shire_layout_width)  - 1)) +
           (  ((y >> (rbox_state.minion_hart_tile_height + 1)) & ((1 << rbox_state.shire_layout_height) - 1))
            * (1 << rbox_state.shire_layout_width));
}

uint64_t RBOXEmu::compute_minion_hart_out_addr(uint32_t target_minion_hart)
{
    uint64_t minion_hart_out_addr = (uint64_t(out_buf_pg_esr.fields.page) << 21)
                                  + (uint64_t(out_buf_cfg_esr.fields.start_offset) << 6)
                                  + target_minion_hart * ((1 << out_buf_cfg_esr.fields.buffer_size) << 5)
                                  + (hart_ptr[target_minion_hart] << 5);

    return minion_hart_out_addr;
}

void RBOXEmu::update_minion_hart_out_ptr(uint32_t target_minion_hart)
{
    hart_ptr[target_minion_hart] = (hart_ptr[target_minion_hart] + 1) % (1 << out_buf_cfg_esr.fields.buffer_size);
}

void RBOXEmu::send_packet(uint32_t minion_hart_id, uint64_t packet[4], uint64_t &out_addr, bool step_mode)
{
    if (step_mode)
    {
        for (uint32_t qw = 0; qw < 4; qw++)
            output_packets.push_back(std::make_pair(minion_hart_id, packet[qw]));
    }
    else
    {
        for (uint32_t qw = 0; qw < 4; qw++)
        {
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, packet[qw], out_addr);
            bemu::pmemwrite<uint64_t>(*this, out_addr, packet[qw]);
            out_addr = out_addr + 8;
        }

        hart_sent_packets[minion_hart_id]++;

        update_minion_hart_out_ptr(minion_hart_id);
    }
}

bool RBOXEmu::report_packets(uint32_t minion_hart_id)
{
    if (hart_sent_packets[minion_hart_id] > 0)
    {
        if (hart_msg_credits[minion_hart_id] > 0)
        {
            uint64_t msg_data = (uint64_t(minion_hart_id) << 32) | hart_sent_packets[minion_hart_id];
            
            LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Report Packets %02d to Minion HART %d", rbox_id, hart_sent_packets[minion_hart_id], minion_hart_id);

            hart_sent_packets[minion_hart_id] = 0;

            write_msg_port_data_from_rbox(minion_hart_id, out_buf_cfg_esr.fields.port_id, rbox_id, (uint32_t*) &msg_data, 0);
            
            hart_msg_credits[minion_hart_id]--;
        
            return true;
        }
        else
        {
            LOG_AGENT(DEBUG, *this, "RBOX[%02" PRIu32 "] => For Minion HART %02d STALL : No message credits", rbox_id, minion_hart_id);
            return false;
        }
    }
    else
        return true;
    
}

void RBOXEmu::write_next_packet()
{
    uint32_t packet_hart_id = output_packets[0].first;
    uint64_t minion_hart_out_addr = compute_minion_hart_out_addr(packet_hart_id);

    for (uint32_t p = 0; p < 4; p++)
    {
        LOG_AGENT(DEBUG, *this, "RBOX [%" PRIu32 "] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, output_packets[0].second, minion_hart_out_addr);
        bemu::pmemwrite<uint64_t>(*this, minion_hart_out_addr, output_packets[0].second);
        output_packets.erase(output_packets.begin());
        minion_hart_out_addr += 8;
    }

    hart_sent_packets[packet_hart_id]++;

    update_minion_hart_out_ptr(packet_hart_id);
}


} // namespace RBOX
} // namespace bemu
