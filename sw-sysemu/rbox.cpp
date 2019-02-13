/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>

#include "rbox.h"
#include "emu_gio.h"
#include "emu_memop.h"

void RBOX::RBOXEmu::reset(uint32_t id)
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

void RBOX::RBOXEmu::run()
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
                          || ((out_buf_cfg_esr.fields.start_offset + (1 << out_buf_cfg_esr.fields.buffer_size) * MINIONS_PER_RBOX) > 0x8000);

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

            for (uint32_t m = 0; m < MINIONS_PER_RBOX; m++)
            {
                minion_credits[m] = (1 << out_buf_cfg_esr.fields.buffer_size);
                minion_ptr[m] = 0;
            }
            
            status_esr.fields.status = WORKING;

            LOG_NOTHREAD(DEBUG, "RBOX %d Setting status to WORKING", rbox_id);
        }
        else
        {
            status_esr.fields.status = CONFIG_ERROR;

            LOG_NOTHREAD(DEBUG, "RBOX %d Setting status to ERROR", rbox_id);
        }
        start_esr.fields.start = 0;
    }

    bool blocked = false;

    // Send quads to minions as long as credits are available
    while (started && !blocked)
    {
        while (output_quads.empty() && !last_in_pckt)
        {
            // Process input packets to get quads.
            uint32_t packet_size = process_packet(next_in_pckt_addr);
            next_in_pckt_addr += packet_size * 8;
            in_pckt_count++;
        }

        while (!blocked)
        {
            blocked = send_quad_packet();
        }

        if (last_in_pckt && output_quads.empty())
        {
            started = false;
            status_esr.fields.status = FINISHED;
        }
    }
}

void RBOX::RBOXEmu::write_esr(uint32_t esr_id, uint64_t data)
{
    switch (esr_id)
    {
        case CONFIG_ESR               : cfg_esr.value = data; break;
        case INPUT_BUFFER_PAGES_ESR   :
                                        {
                                            in_buf_pg_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Input Buffer Pages = {.page0 = %08" PRIx64
                                                       ", .page0_enable = %" PRId64 ", .page1 = %08" PRIx64
                                                       ", .page1_enable = %" PRId64 "}", rbox_id,
                                                       in_buf_pg_esr.fields.page0, in_buf_pg_esr.fields.page0_enable,
                                                       in_buf_pg_esr.fields.page1, in_buf_pg_esr.fields.page1_enable);
                                            break;
                                        }
        case INPUT_BUFFER_CONFIG_ESR  : 
                                        {
                                            in_buf_cfg_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Input Buffer Config = {.start_offset = %08" PRIx64
                                                       ", .buffer_size = %08" PRIx64 "}", rbox_id,
                                                       in_buf_cfg_esr.fields.start_offset, in_buf_cfg_esr.fields.buffer_size);
                                            break;
                                        }
        case OUTPUT_BUFFER_PAGE_ESR   :
                                        {
                                            out_buf_pg_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Output Buffer Page = {.page = %08" PRIx64
                                                       ", .page_enable = %" PRId64 "}", rbox_id,
                                                       out_buf_pg_esr.fields.page, out_buf_pg_esr.fields.page_enable);
                                            break;
                                        }
        case OUTPUT_BUFFER_CONFIG_ESR :
                                        {
                                            out_buf_cfg_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Output Buffer Config = {.start_offset = %08" PRIx64
                                                       ", .buffer_size = %08" PRIx64 ", .port_id = %" PRId64 "}", rbox_id,
                                                       out_buf_cfg_esr.fields.start_offset, out_buf_cfg_esr.fields.buffer_size,
                                                       out_buf_cfg_esr.fields.port_id);
                                            break;
                                        }
        case STATUS_ESR               : /* Read Only */ break;
        case START_ESR                : 
                                        {
                                            start_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Start = {.start = %d}", rbox_id, (uint32_t) start_esr.fields.start);
                                            break;
                                        }
        case CONSUME_ESR              :
                                        {
                                            consume_esr.value = data;
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Consume = {.consumed = %03" PRId64 ", .minion_id = %02" PRId64 "}", rbox_id,
                                                        consume_esr.fields.consumed, consume_esr.fields.minion_id);
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Minion %02" PRId64 " Credits %03d -> %03d", rbox_id,
                                                        consume_esr.fields.minion_id, minion_credits[consume_esr.fields.minion_id],
                                                        minion_credits[consume_esr.fields.minion_id] + consume_esr.fields.consumed);
                                            minion_credits[consume_esr.fields.minion_id] += consume_esr.fields.consumed;
                                            break;
                                        }
        default                       :
                                        {
                                            LOG_NOTHREAD(DEBUG, "RBOX %d Write to undefined ESR %d", rbox_id, esr_id);
                                            break;
                                        }

    }
}

uint64_t RBOX::RBOXEmu::read_esr(uint32_t esr_id)
{
    switch (esr_id)
    {
        case CONFIG_ESR               : LOG_NOTHREAD(DEBUG, "RBOX %d Read CONFIG ESR with value %016" PRIx64, rbox_id, cfg_esr.value);
                                        return cfg_esr.value;
        case INPUT_BUFFER_PAGES_ESR   : LOG_NOTHREAD(DEBUG, "RBOX %d Read INPUT BUFFER PAGES ESR with value %016" PRIx64,
                                                    rbox_id, in_buf_pg_esr.value);
                                        return in_buf_pg_esr.value; 
        case INPUT_BUFFER_CONFIG_ESR  : LOG_NOTHREAD(DEBUG, "RBOX %d Read INPUT BUFFER CONFIG ESR with value %016" PRIx64,
                                                   rbox_id, in_buf_cfg_esr.value);
                                        return in_buf_cfg_esr.value;
        case OUTPUT_BUFFER_PAGE_ESR   : LOG_NOTHREAD(DEBUG, "RBOX %d Read OUTPUT BUFFER PAGE ESR with value %016" PRIx64,
                                                   rbox_id, out_buf_pg_esr.value);
                                        return out_buf_pg_esr.value;
        case OUTPUT_BUFFER_CONFIG_ESR : LOG_NOTHREAD(DEBUG, "RBOX %d Read OUTPUT BUFFER CONFIG ESR with value %016" PRIx64,
                                                   rbox_id, out_buf_cfg_esr.value);
                                        return out_buf_cfg_esr.value;
        case STATUS_ESR               : LOG_NOTHREAD(DEBUG, "RBOX %d Read STATUS ESR with value %016" PRIx64,
                                                   rbox_id, status_esr.value);
                                        return status_esr.value;
        case START_ESR                : LOG_NOTHREAD(DEBUG, "RBOX %d Read START ESR with value %016" PRIx64,
                                                   rbox_id, start_esr.value);
                                        return start_esr.value;
        case CONSUME_ESR              : LOG_NOTHREAD(DEBUG, "RBOX %d Read CONSUME ESR with value %016" PRIx64,
                                                   rbox_id, consume_esr.value);
                                        return consume_esr.value;
        default                       : LOG_NOTHREAD(DEBUG, "RBOX %d Read to UNDEFINED ESR", rbox_id);
                                        return 0;
    }
}

uint32_t RBOX::RBOXEmu::process_packet(uint64_t packet)
{
    InPcktHeaderT header;
    header.qw = pmemread64(packet);

    uint32_t packet_size = 0;

    switch (header.type)
    {
        case INPCKT_FULLY_COVERED_TILE:
            {
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing fully covered tile packet", rbox_id);
                InPcktFullyCoveredTileT fully_covered_tile_pckt;
                
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    fully_covered_tile_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, fully_covered_tile_pckt.qw[qw]);
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = fully_covered_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_64X64_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = fully_covered_tile_pckt.tile.depth;
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
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing partially covered tile packet", rbox_id);
                InPcktLargeTriTileT large_tri_tile_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    large_tri_tile_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, large_tri_tile_pckt.qw[qw]);
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = large_tri_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_LARGE_TRI_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = large_tri_tile_pckt.tile.depth;
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
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing triangle with tile, triangle inside 64x64 tile aligned 64x64 tile", rbox_id);
                InPcktTriWithTile64x64T tri_with_tile_64x64_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_64x64_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, tri_with_tile_64x64_pckt.qw[qw]);
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
                uint32_t depth_sample = tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth;
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
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing triangle with tile, triangle inside 64x64 tile aligned 128x128 tile", rbox_id);
                InPcktTriWithTile128x128T tri_with_tile_128x128_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_128x128_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, tri_with_tile_128x128_pckt.qw[qw]);
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
                uint32_t depth_sample = tri_with_tile_128x128_pckt.tri_with_tile_128x128.depth;
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
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing large triangle packet", rbox_id);
                InPcktLargeTriT large_tri_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    large_tri_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, large_tri_pckt.qw[qw]);
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
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing RBOX state packet", rbox_id);
                InPcktRBOXStateT rbox_state_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    rbox_state_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw, rbox_state_pckt.qw[qw]);
                }
                rbox_state = rbox_state_pckt.state;
                packet_size = 8;
            }
            break;
        case INPCKT_FRAG_SHADING_STATE:
            {
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Processing fragment shading state packet", rbox_id);
                InPcktFrgmtShdrStateT frag_shader_state_pckt;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Packet Data", rbox_id);
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    frag_shader_state_pckt.qw[qw] = pmemread64(packet + qw * 8);
                    LOG_NOTHREAD(DEBUG, "\t[%u] = %016" PRIx64, qw,frag_shader_state_pckt.qw[qw]);
                }
                frag_shader_state = frag_shader_state_pckt.state;
                packet_size = 4;

                for (uint32_t m = 0; m < MINIONS_PER_RBOX; m++)
                    fsh_state_sent[m] = false;
            }
            break;
        case INPCKT_END_OF_INPUT_BUFFER:
            {
                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Received end of input buffer packet", rbox_id);
                last_in_pckt = true;
            }
            break;
        default :
            break;
    }

    return packet_size;
}

const uint32_t RBOX::RBOXEmu::tile_dimensions[6][2] = {{64, 64}, {64, 32}, {32, 32}, {16, 16}, {8, 8}, {4, 4}};

void RBOX::RBOXEmu::generate_tile(uint32_t tile_x, uint32_t tile_y, int64_t edge_samples[3], uint32_t depth_sample, TileSizeT tile_sz)
{
    TriangleSampleT tile_sample;
    for (uint32_t eq = 0; eq < 3; eq++)
        tile_sample.edge[eq] = edge_samples[eq];
    tile_sample.depth = depth_sample;

    TriangleSampleT row_sample = tile_sample;

    uint32_t generated_quads_in_tile = 0;

    for (uint32_t y = 0; y < tile_dimensions[tile_sz][1]; y += 2)
    {
        TriangleSampleT quad_sample = row_sample;

        for (uint32_t x = 0; x < tile_dimensions[tile_sz][0]; x += 2)
        {
            QuadInfoT quad;

            sample_quad(tile_x + x, tile_y + y, quad_sample, quad);

            bool quad_coverage = test_quad(quad);

            if (quad_coverage)
            {
                uint32_t target_minion = compute_target_minion(tile_x + x, tile_y + y);

                output_quads.push_back(quad);

                generated_quads_in_tile++;

                LOG_NOTHREAD(DEBUG, "RBOX [%d] : Generated packet for quad at (%d, %d) for minion %d", rbox_id, tile_x + x, tile_y + y, target_minion);
            }

            sample_next_quad(quad_sample);
        }
        sample_next_row(row_sample);
    }

    LOG_NOTHREAD(DEBUG, "RBOX [%d] : Generated %d quads in tile", rbox_id, generated_quads_in_tile);
}

void RBOX::RBOXEmu::sample_next_row(TriangleSampleT &sample)
{
    for (uint32_t eq = 0; eq < 3; eq++)
        sample.edge[eq] += 2 * current_triangle.edge_eqs[eq].b;
    sample.depth += 2 * current_triangle.depth_eq.b;
}

void RBOX::RBOXEmu::sample_next_quad(TriangleSampleT &sample)
{
    for (uint32_t eq = 0; eq < 3; eq++)
        sample.edge[eq] += 2 * current_triangle.edge_eqs[eq].a;
    sample.depth += 2 * current_triangle.depth_eq.a;
}

void RBOX::RBOXEmu::sample_quad(uint32_t x, uint32_t y, TriangleSampleT quad_sample, QuadInfoT &quad)
{
    LOG_NOTHREAD(DEBUG, "RBOX [%d] : Sampling quad at (%d, %d) -> start sample = (%016" PRIx64 ", %016" PRIx64 ", %016" PRIx64 ", %08x)\n"
                      "\t\tequation coefficients = (\n\t\t\t(%016" PRIx64 ", %016" PRIx64 "),"
                      "\n\t\t\t(%016" PRIx64 ", %016" PRIx64 "),\n\t\t\t(%016" PRIx64 ", %016" PRIx64 "),\n\t\t\t(%08x, %08x)\n\t\t)\n"
                      "\t\ttop_or_left_edges = (%d, %d, %d)",
                       rbox_id, x, y, quad_sample.edge[0], quad_sample.edge[1], quad_sample.edge[2], quad_sample.depth,
                       current_triangle.edge_eqs[0].a, current_triangle.edge_eqs[0].b,
                       current_triangle.edge_eqs[1].a, current_triangle.edge_eqs[1].b,
                       current_triangle.edge_eqs[2].a, current_triangle.edge_eqs[2].b,
                       current_triangle.depth_eq.a, current_triangle.depth_eq.b,
                       current_triangle.top_or_left_edge[0], current_triangle.top_or_left_edge[1], current_triangle.top_or_left_edge[2]);

    quad.fragment[0].sample = quad_sample;
    for (uint32_t eq = 0; eq < 3; eq++)
    {
        quad.fragment[1].sample.edge[eq] = quad.fragment[0].sample.edge[eq] + current_triangle.edge_eqs[eq].a;
        quad.fragment[2].sample.edge[eq] = quad.fragment[0].sample.edge[eq] + current_triangle.edge_eqs[eq].b;
        quad.fragment[3].sample.edge[eq] = quad.fragment[1].sample.edge[eq] + current_triangle.edge_eqs[eq].b;
    }
    quad.fragment[1].sample.depth = quad.fragment[0].sample.depth + current_triangle.depth_eq.a;
    quad.fragment[2].sample.depth = quad.fragment[0].sample.depth + current_triangle.depth_eq.b;
    quad.fragment[3].sample.depth = quad.fragment[1].sample.depth + current_triangle.depth_eq.b;

    for (uint32_t f = 0; f < 4; f++)
    {
        quad.fragment[f].coverage = sample_inside_triangle(quad.fragment[f].sample);
        LOG_NOTHREAD(DEBUG, "RBOX [%d] => Fragment %d Sample (%016" PRIx64 ", %016" PRIx64 ", %016" PRIx64 " ,%08x) Coverage = %d", rbox_id, f,
                          quad.fragment[f].sample.edge[0], quad.fragment[f].sample.edge[1], quad.fragment[f].sample.edge[2],
                          quad.fragment[f].sample.depth, quad.fragment[f].coverage);
    }

    quad.x = x;
    quad.y = y;
    quad.triangle_data_ptr = current_triangle.triangle_data_ptr;
}

bool RBOX::RBOXEmu::test_quad(QuadInfoT &quad)
{
    bool quad_coverage = false;

    for (uint32_t f = 0; f < 4; f++)
    {
        int32_t x = quad.x + (f & 0x1);
        int32_t y = quad.y + ((f >> 1) & 0x1);
        bool scissor_test = do_scissor_test(x, y);
        quad.fragment[f].coverage = quad.fragment[f].coverage && scissor_test;

        if (quad.fragment[f].coverage)
        {
            uint64_t frag_depth_stencil_address = compute_depth_stencil_buffer_address(x, y);
            uint32_t frag_depth_stencil = pmemread32(frag_depth_stencil_address);

            uint8_t frag_stencil = frag_depth_stencil >> 24;
            uint32_t frag_depth = frag_depth_stencil & 0x00FFFFFF;

            LOG_NOTHREAD(DEBUG, "RBOX [%d] => Testing fragment at (%d, %d) address = %016" PRIx64 " sample_depth = %08x fragment_depth_stencil = %08x",
                              rbox_id, x, y, frag_depth_stencil_address, quad.fragment[f].sample.depth, frag_depth_stencil);

            bool depth_bound_test =  do_depth_bound_test(frag_depth);
            bool stencil_test = do_stencil_test(frag_stencil);
            bool depth_test = do_depth_test(frag_depth, quad.fragment[f].sample.depth);

            LOG_NOTHREAD(DEBUG, "RBOX [%d] => Test results : depth_bound = %d stencil = %d depth = %d", rbox_id, depth_bound_test, stencil_test, depth_test);

            uint8_t out_stencil = stencil_update(frag_stencil, stencil_test, depth_test);

            uint32_t out_depth;
            if (depth_bound_test)
            {
                if (stencil_test)
                    out_depth = rbox_state.depth_test_write_enable ? quad.fragment[f].sample.depth : frag_depth;
                else
                    out_depth = frag_depth;

                uint32_t out_depth_stencil = (out_stencil << 24) | out_depth;
                pmemwrite32(frag_depth_stencil_address, out_depth_stencil);
            }

            quad.fragment[f].coverage = quad.fragment[f].coverage && depth_bound_test && stencil_test && depth_test;
        }

        quad_coverage |= quad.fragment[f].coverage;
    }

    return quad_coverage;
}

uint64_t RBOX::RBOXEmu::compute_depth_stencil_buffer_address(uint32_t x, uint32_t y)
{
    uint32_t row = y / 4;
    uint32_t line = x / 4;
    uint64_t depth_stencil_buffer_offset = row * rbox_state.depth_stencil_buffer_row_pitch
                                       + line * 64 + (y & 0x3) * 16 + (x & 0x03) * 4;
    return rbox_state.depth_stencil_buffer_ptr + depth_stencil_buffer_offset;
}

bool RBOX::RBOXEmu::sample_inside_triangle(TriangleSampleT sample)
{
    return ((sample.edge[0] == 0) ? current_triangle.top_or_left_edge[0] : ((sample.edge[0] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[1] == 0) ? current_triangle.top_or_left_edge[1] : ((sample.edge[1] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[2] == 0) ? current_triangle.top_or_left_edge[2] : ((sample.edge[2] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0));
}

bool RBOX::RBOXEmu::do_scissor_test(int32_t x, int32_t y)
{
    LOG_NOTHREAD(DEBUG, "RBOX [%d] => Scissor test for fragment at (%d, %d) scissor rectangle (%" PRId64 ", %" PRId64 ", %" PRId64 ", %" PRId64 ")",
                      rbox_id, x, y, rbox_state.scissor_start_x, rbox_state.scissor_start_y,
                      rbox_state.scissor_width, rbox_state.scissor_height);

    return    (x >=  rbox_state.scissor_start_x)
           && (y >=  rbox_state.scissor_start_y)
           && (x <= (rbox_state.scissor_start_x + rbox_state.scissor_width))
           && (y <= (rbox_state.scissor_start_y + rbox_state.scissor_height));
}

bool RBOX::RBOXEmu::do_depth_bound_test(uint32_t frag_depth)
{
    return   !rbox_state.depth_bound_enable
           || (    (frag_depth >= rbox_state.depth_bound_min)
               &&  (frag_depth <= rbox_state.depth_bound_max));
}

bool RBOX::RBOXEmu::do_stencil_test(uint8_t frag_stencil)
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
                break;
            case COMPARE_OP_LESS :
                return ref_stencil_masked < frag_stencil_masked;
                break;
            case COMPARE_OP_EQUAL :
                return ref_stencil_masked == frag_stencil_masked;
                break;
            case COMPARE_OP_LESS_OR_EQUAL :
                return ref_stencil_masked <= frag_stencil_masked;
                break;
            case COMPARE_OP_GREATER :
                return ref_stencil_masked > frag_stencil_masked;
                break;
            case COMPARE_OP_NOT_EQUAL :
                return ref_stencil_masked != frag_stencil_masked;
                break;
            case COMPARE_OP_GREATER_OR_EQUAL :
                return ref_stencil_masked >= frag_stencil_masked;
                break;
            case COMPARE_OP_ALWAYS :
                return true;
                break;
        }
    }
    else
        return true;
}

bool RBOX::RBOXEmu::do_depth_test(uint32_t frag_depth, uint32_t sample_depth)
{
    if (rbox_state.depth_test_enable)
    {
        switch (rbox_state.depth_test_compare_op)
        {
            case COMPARE_OP_NEVER :
                return false;
                break;
            case COMPARE_OP_LESS :
                return sample_depth < frag_depth;
                break;
            case COMPARE_OP_EQUAL :
                return sample_depth == frag_depth;
                break;
            case COMPARE_OP_LESS_OR_EQUAL :
                return sample_depth <= frag_depth;
                break;
            case COMPARE_OP_GREATER :
                return sample_depth > frag_depth;
                break;
            case COMPARE_OP_NOT_EQUAL :
                return sample_depth != frag_depth;
                break;
            case COMPARE_OP_GREATER_OR_EQUAL :
                return sample_depth >= frag_depth;
                break;
            case COMPARE_OP_ALWAYS :
                return true;
                break;
        }
    }
    else
        return true;
}

uint8_t RBOX::RBOXEmu::stencil_update(uint8_t frag_stencil, bool stencil_test, bool depth_test)
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
                break;
            case STENCIL_OP_ZERO :
                return 0;
                break;
            case STENCIL_OP_REPLACE :
                return stencil_state.ref & stencil_state.write_mask;
                break;
            case STENCIL_OP_INC_CLAMP :
                return ((frag_stencil == 255) ? 255 : (frag_stencil + 1)) & stencil_state.write_mask;
                break;
            case STENCIL_OP_DEC_CLAMP :
                return ((frag_stencil == 0) ? 0 : (frag_stencil - 1)) & stencil_state.write_mask;
                break;
            case STENCIL_OP_INVERT :
                return (~frag_stencil) & stencil_state.write_mask;
                break;
            case STENCIL_OP_INC_WRAP :
                return (frag_stencil + 1) & stencil_state.write_mask;
                break;
            case STENCIL_OP_DEC_WRAP :
                return (frag_stencil - 1) & stencil_state.write_mask;
                break;
            default :
                return 0;
                break;
        }
    }
    else
        return frag_stencil;
}

bool RBOX::RBOXEmu::send_quad_packet()
{
    if (!rbox_state.fragment_shader_disabled && !output_quads.empty())
    {
        QuadInfoT quad[2];

        bool insert_fake_quad = false;

        if (output_quads.size() > 1)
        {
            uint32_t target_minion[2];

            target_minion[0] = compute_target_minion(output_quads[0].x, output_quads[0].y);
            target_minion[1] = compute_target_minion(output_quads[1].x, output_quads[1].y);

            insert_fake_quad = (target_minion[0] == target_minion[1]);
        }
        else 
            insert_fake_quad = true;

        if (insert_fake_quad)
        {
            quad[0] = quad[1] = output_quads[0];
            quad[1].fragment[0].coverage = false;
            quad[1].fragment[1].coverage = false;
            quad[1].fragment[2].coverage = false;
            quad[1].fragment[3].coverage = false;

            output_quads.erase(output_quads.begin());
        }
        else
        {
            quad[0] = output_quads[0];
            quad[1] = output_quads[1];

            output_quads.erase(output_quads.begin());
            output_quads.erase(output_quads.begin());
        }

        uint32_t target_minion = compute_target_minion(quad[0].x, quad[1].y);
    
        if (!fsh_state_sent[target_minion])
            fsh_state_sent[target_minion] = send_frag_shader_state_packet(target_minion);

        if (!fsh_state_sent[target_minion])
            return false;

        uint32_t packets_per_sample = rbox_state.fragment_shader_reads_bary * 2
                                    + rbox_state.fragment_shader_reads_depth 
                                    + rbox_state.fragment_shader_reads_coverage;
        uint32_t num_packets = ((rbox_state.fragment_shader_per_sample && rbox_state.msaa_enable) ? (packets_per_sample * rbox_state.msaa_samples)
                                                                                                   :  packets_per_sample) + 1;
    
        LOG_NOTHREAD(DEBUG, "RBOX [%d] => Target Minion %02d Credits %03d Required Packets Per Sample %d Total Packets per Quad Message %d",
                   rbox_id, target_minion, minion_credits[target_minion], packets_per_sample, num_packets);

        if (minion_credits[target_minion] >= num_packets)
        {
            OutPcktQuadInfoT quad_info_pckt;
            
            LOG_NOTHREAD(DEBUG, "RBOX [%d] => Generate quad packet", rbox_id);
            
            for (uint32_t qw = 0; qw < 4; qw++)
                quad_info_pckt.qw[qw] = 0;
            
            quad_info_pckt.quad_info.type = OUTPCKT_QUAD_INFO;
            for (uint32_t q = 0; q < 2; q++)
            {
                quad_info_pckt.quad_info.x[q] = quad[q].x >> 1;
                quad_info_pckt.quad_info.y[q] = quad[q].y >> 1;
                quad_info_pckt.quad_info.triangle_data_ptr[q] = quad[q].triangle_data_ptr;
            }
            quad_info_pckt.quad_info.smpl_idx = 0;
            for (uint32_t f = 0; f < (2 * 4); f++)
                quad_info_pckt.quad_info.mask = quad[f / 4].fragment[f % 4].coverage ? (quad_info_pckt.quad_info.mask | (1 << f)) : quad_info_pckt.quad_info.mask;
    
            uint64_t minion_out_off = compute_minion_out_off(target_minion);
            uint64_t minion_out_addr = compute_minion_out_addr(target_minion);

            for (uint32_t qw = 0; qw < 4; qw++)
            {
                LOG_NOTHREAD(DEBUG, "RBOX [%d] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, quad_info_pckt.qw[qw], minion_out_addr);
                pmemwrite64(minion_out_addr, quad_info_pckt.qw[qw]);
                minion_out_addr = minion_out_addr + 8;
            }

            update_minion_out_ptr(target_minion);
            
            if (rbox_state.fragment_shader_reads_bary)
            {
                OutPcktQuadDataT quad_data_pckt;
                
                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = convert_edge_to_fp32(quad[f / 4].fragment[f % 4].sample.edge[1]);
                
                minion_out_addr = compute_minion_out_addr(target_minion);

                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    LOG_NOTHREAD(DEBUG, "RBOX [%d] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, quad_data_pckt.qw[qw], minion_out_addr);
                    pmemwrite64(minion_out_addr, quad_data_pckt.qw[qw]);
                    minion_out_addr = minion_out_addr + 8;
                }
                
                update_minion_out_ptr(target_minion);

                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = convert_edge_to_fp32(quad[f / 4].fragment[f % 4].sample.edge[2]);
                
                minion_out_addr = compute_minion_out_addr(target_minion);

                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    LOG_NOTHREAD(DEBUG, "RBOX [%d] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, quad_data_pckt.qw[qw], minion_out_addr);
                    pmemwrite64(minion_out_addr, quad_data_pckt.qw[qw]);
                    minion_out_addr = minion_out_addr + 8;
                }
                
                update_minion_out_ptr(target_minion);
            }

            if (rbox_state.fragment_shader_reads_depth)
            {
                OutPcktQuadDataT quad_data_pckt;
                
                for (uint32_t f = 0; f < (4 * 2); f++)
                    quad_data_pckt.ps[f] = convert_depth_to_fp32(quad[f / 4].fragment[f % 4].sample.depth);
                
                minion_out_addr = compute_minion_out_addr(target_minion);

                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    LOG_NOTHREAD(DEBUG, "RBOX [%d] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, quad_data_pckt.qw[qw], minion_out_addr);
                    pmemwrite64(minion_out_addr, quad_data_pckt.qw[qw]);
                    minion_out_addr = minion_out_addr + 8;

                }
                
                update_minion_out_ptr(target_minion);
            }

            minion_credits[target_minion] -= num_packets;

            uint64_t msg_data = (minion_out_off << 16) | num_packets;

            write_msg_port_data_from_rbox(target_minion, out_buf_cfg_esr.fields.port_id, rbox_id, (uint32_t*) &msg_data, 0);
         
            return false;   
        }

        return true;
    }

    return true;
}

float RBOX::RBOXEmu::convert_edge_to_fp32(int64_t edge)
{
    uint8_t sign = edge & EDGE_EQ_SAMPLE_SIGN_MASK;
    uint64_t edge_abs = sign ? -edge : edge;
    float edge_abs_fp32 = float(edge_abs) / float(1 << EDGE_EQ_SAMPLE_FRAC_BITS);
    return (sign ? -edge_abs_fp32 : edge_abs_fp32);
}

float RBOX::RBOXEmu::convert_depth_to_fp32(uint32_t depth)
{
    return float(depth) / float((1 << 24) - 1);
}

bool RBOX::RBOXEmu::send_frag_shader_state_packet(uint32_t target_minion)
{

    if (minion_credits[target_minion] > 0)
    {
        OutPcktFrgShdrStateT f_sh_pckt;

        LOG_NOTHREAD(DEBUG, "RBOX [%d] => Generate Fragment Shader State Packet", rbox_id);

        for (uint32_t qw = 0; qw < 4; qw++)
            f_sh_pckt.qw[qw] = 0;

        f_sh_pckt.state.type = OUTPCKT_STATE_INFO;
        f_sh_pckt.state.frg_shdr_func_ptr = frag_shader_state.frag_shader_function_ptr;
        f_sh_pckt.state.frg_shdr_state_ptr = frag_shader_state.frag_shader_state_ptr;

        uint64_t minion_out_addr = compute_minion_out_addr(target_minion);
        uint64_t minion_out_off  = compute_minion_out_off(target_minion);

        for (uint32_t qw = 0; qw < 4; qw++)
        {
            LOG_NOTHREAD(DEBUG, "RBOX [%d] => Writing QW %016" PRIx64 " at address %016" PRIx64, rbox_id, f_sh_pckt.qw[qw], minion_out_addr + qw * 8);
            pmemwrite64(minion_out_addr + qw * 8, f_sh_pckt.qw[qw]);
        }

        update_minion_out_ptr(target_minion);

        minion_credits[target_minion]--;

        uint64_t msg_data = (minion_out_off << 16) | 1;

        write_msg_port_data_from_rbox(target_minion, out_buf_cfg_esr.fields.port_id, rbox_id, (uint32_t*) &msg_data, 0);

        return true;
    }
    else
        return false;
}


void RBOX::RBOXEmu::tile_position_to_pixels(uint32_t &tile_x, uint32_t &tile_y, TileSizeT tile_size)
{
    switch(tile_size)
    {
        case TILE_SIZE_64x64 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 6;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 64x64 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_64x32 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 5;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 64x32 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_32x32 :
            tile_x = tile_x << 5;
            tile_y = tile_y << 5;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 32x32 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_16x16 :
            tile_x = tile_x << 4;
            tile_y = tile_y << 4;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 16x16 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_8x8 :
            tile_x = tile_x << 3;
            tile_y = tile_y << 3;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 8x8 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
        case TILE_SIZE_4x4 :
            tile_x = tile_x << 2;
            tile_y = tile_y << 2;
            LOG_NOTHREAD(DEBUG, "RBOX [%d] : Tile Size 4x4 Position (%d, %d)", rbox_id, tile_x, tile_y);
            break;
    }
}

uint32_t RBOX::RBOXEmu::compute_target_minion(uint32_t x, uint32_t y)
{
    // Minions in a Shire are distributed in rows of Shire Layout With.
    
    return    ((x >> (rbox_state.minion_tile_width  + 1)) & ((1 << rbox_state.shire_layout_width)  - 1)) +
           (  ((y >> (rbox_state.minion_tile_height + 1)) & ((1 << rbox_state.shire_layout_height) - 1))
            * (1 << rbox_state.shire_layout_width));
}

uint64_t RBOX::RBOXEmu::compute_minion_out_off(uint32_t target_minion)
{
    uint64_t minion_out_offset = target_minion * (out_buf_cfg_esr.fields.buffer_size << 5)
                               + (minion_ptr[target_minion] << 5);

    return minion_out_offset;
}

uint64_t RBOX::RBOXEmu::compute_minion_out_addr(uint32_t target_minion)
{
    uint64_t minion_out_addr = uint64_t(out_buf_pg_esr.fields.page << 21)
                             + uint64_t(out_buf_cfg_esr.fields.start_offset << 6)
                             + target_minion * (out_buf_cfg_esr.fields.buffer_size << 5)
                             + (minion_ptr[target_minion] << 5);

    return minion_out_addr;
}

void RBOX::RBOXEmu::update_minion_out_ptr(uint32_t target_minion)
{
    minion_ptr[target_minion] = (minion_ptr[target_minion] + 1) % out_buf_cfg_esr.fields.buffer_size;
}


