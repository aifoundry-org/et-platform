#include <cstdio>

#include "rbox.h"
#include "emu_gio.h"
#include "emu_memop.h"

using emu::gprintf;
using emu::gsprintf;
using emu::gfprintf;

RingBuffer input;
RingBuffer output;

#define RBOX_STATE_BUFFER_SIZE 8

RBOXState rbox_state[RBOX_STATE_BUFFER_SIZE];
uint32_t rbox_state_idx;
FragmentShaderState frag_shader_state;
TriangleInfo current_triangle;
bool new_frag_shader_state;

void set_rbox(uint64_t inStream, uint32_t inStreamSz, uint64_t outStream, uint32_t outStreamSz)
{
    input.initialize(inStream, inStreamSz);
    output.initialize(outStream, outStreamSz);
    rbox_state_idx = RBOX_STATE_BUFFER_SIZE - 1;
    new_frag_shader_state = false;
}

void reset_input_stream()
{
    input.reset();
}

void push_packet()
{
    input.push_packet();
}

void reset_output_stream()
{
    output.reset();
}

bool consume_packet()
{
    return output.consume_packet();
}

void process()
{
    uint64_t packet;
    packet = input.read_packet();
    while (packet)
    {
        uint32_t packet_size;
        packet_size = process_packet(packet);
        input.consume_packet();
        packet = input.read_next_packet(packet_size);
    }
}

uint32_t process_packet(uint64_t packet)
{
    RBOXInPcktHeader header;
    header.qw = vmemread64(packet);

    uint32_t packet_size = 0;

    switch (header.type)
    {
        case RBOX_INPCKT_FULLY_COVERED_TILE:
            {
                DEBUG_EMU( gprintf("RBOX : Processing fully covered tile packet\n"); )
                RBOXInPcktFullyCoveredTile fully_covered_tile_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    fully_covered_tile_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 3) gprintf("%016lx_", fully_covered_tile_pckt.qw[qw]);
                        else        gprintf("%016lx\n", fully_covered_tile_pckt.qw[qw]);
                    )
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = fully_covered_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_64X64_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = fully_covered_tile_pckt.tile.depth;
                RBOXTileSize tile_size = (RBOXTileSize) fully_covered_tile_pckt.tile.tile_size;
                uint32_t tile_x = fully_covered_tile_pckt.tile.tile_left;
                uint32_t tile_y = fully_covered_tile_pckt.tile.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 4;
            }
            break;
        case RBOX_INPCKT_LARGE_TRIANGLE_TILE:
            {
                DEBUG_EMU( gprintf("RBOX : Processing partially covered tile packet\n"); )
                RBOXInPcktLargeTriTile large_tri_tile_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    large_tri_tile_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 3) gprintf("%016lx_", large_tri_tile_pckt.qw[qw]);
                        else        gprintf("%016lx\n", large_tri_tile_pckt.qw[qw]);
                    )
                }

                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = large_tri_tile_pckt.tile.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_LARGE_TRI_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = large_tri_tile_pckt.tile.depth;
                RBOXTileSize tile_size = (RBOXTileSize) large_tri_tile_pckt.tile.tile_size;
                uint32_t tile_x = large_tri_tile_pckt.tile.tile_left;
                uint32_t tile_y = large_tri_tile_pckt.tile.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 4;
            }
            break;
        case RBOX_INPCKT_TRIANGLE_WITH_TILE_64x64:
            {
                DEBUG_EMU(gprintf("RBOX : Processing triangle with tile, triangle inside 64x64 tile aligned 64x64 tile\n"); )
                RBOXInPcktTriWithTile64x64 tri_with_tile_64x64_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_64x64_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 7) gprintf("%016lx_", tri_with_tile_64x64_pckt.qw[qw]);
                        else        gprintf("%016lx\n", tri_with_tile_64x64_pckt.qw[qw]);
                    )
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
                current_triangle.back_facing = (tri_with_tile_64x64_pckt.tri_with_tile_64x64.tri_facing == RBOX_TRI_FACING_BACK);
                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_64X64_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth;
                RBOXTileSize tile_size = (RBOXTileSize) tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_size;
                uint32_t tile_x = tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_left;
                uint32_t tile_y = tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 8;
            }
            break;
        case RBOX_INPCKT_TRIANGLE_WITH_TILE_128x128:
            {
                DEBUG_EMU( gprintf("RBOX : Processing triangle with tile, triangle inside 64x64 tile aligned 128x128 tile\n"); )
                RBOXInPcktTriWithTile128x128 tri_with_tile_128x128_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    tri_with_tile_128x128_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 7) gprintf("%016lx_", tri_with_tile_128x128_pckt.qw[qw]);
                        else        gprintf("%016lx\n", tri_with_tile_128x128_pckt.qw[qw]);
                    )
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
                current_triangle.back_facing = (tri_with_tile_128x128_pckt.tri_with_tile_128x128.tri_facing == RBOX_TRI_FACING_BACK);
                int64_t edge_samples[3];
                for (uint32_t eq = 0; eq < 3; eq++)
                    edge_samples[eq] = tri_with_tile_128x128_pckt.tri_with_tile_128x128.edge[eq].e
                                     << (EDGE_EQ_SAMPLE_FRAC_BITS - EDGE_EQ_128X128_SAMPLE_FRAC_BITS);
                uint32_t depth_sample = tri_with_tile_128x128_pckt.tri_with_tile_128x128.depth;
                RBOXTileSize tile_size = (RBOXTileSize) tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_size;
                uint32_t tile_x = tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_left;
                uint32_t tile_y = tri_with_tile_128x128_pckt.tri_with_tile_128x128.tile_top;
                tile_position_to_pixels(tile_x, tile_y, tile_size);
                generate_tile(tile_x, tile_y, edge_samples, depth_sample, tile_size);
                packet_size = 8;
            }
            break;
        case RBOX_INPCKT_LARGE_TRIANGLE:
            {
                DEBUG_EMU( gprintf("RBOX : Processing large triangle packet\n"); )
                RBOXInPcktLargeTri large_tri_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    large_tri_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 7) gprintf("%016lx_", large_tri_pckt.qw[qw]);
                        else        gprintf("%016lx\n", large_tri_pckt.qw[qw]);
                    )
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
                current_triangle.back_facing = (large_tri_pckt.triangle.tri_facing == RBOX_TRI_FACING_BACK);
                packet_size = 8;
            }
            break;
        case RBOX_INPCKT_RBOX_STATE:
            {
                DEBUG_EMU( gprintf("RBOX : Processing RBOX state packet\n"); )
                RBOXInPcktRBOXState rbox_state_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 8; qw++)
                {
                    rbox_state_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 7) gprintf("%016lx_", rbox_state_pckt.qw[qw]);
                        else        gprintf("%016lx\n", rbox_state_pckt.qw[qw]);
                    )
                }

                rbox_state_idx++;
                if (rbox_state_idx == RBOX_STATE_BUFFER_SIZE)
                    rbox_state_idx = 0;
                rbox_state[rbox_state_idx] = rbox_state_pckt.state;
                packet_size = 8;
            }
            break;
        case RBOX_INPCKT_FRAG_SHADING_STATE:
            {
                DEBUG_EMU( gprintf("RBOX : Processing fragment shading state packet\n"); )
                RBOXInPcktFrgmtShdrState frag_shader_state_pckt;

                DEBUG_EMU( gprintf("RBOX : Packet Data "); )
                for (uint32_t qw = 0; qw < 4; qw++)
                {
                    frag_shader_state_pckt.qw[qw] = vmemread64(packet + qw * 8);
                    DEBUG_EMU(
                        if (qw < 3) gprintf("%016lx_", frag_shader_state_pckt.qw[qw]);
                        else        gprintf("%016lx\n", frag_shader_state_pckt.qw[qw]);
                    )
                }

                frag_shader_state = frag_shader_state_pckt.state;
                new_frag_shader_state = true;
                packet_size = 4;
            }
            break;
        default :
            break;
    }

    return packet_size;
}

uint32_t tile_dimensions[][2] = {{64, 64}, {64, 32}, {32, 32}, {16, 16}, {8, 8}, {4, 4}};

void generate_tile(uint32_t tile_x, uint32_t tile_y, int64_t edge_samples[3], uint32_t depth_sample, RBOXTileSize tile_sz)
{
    TriangleSample tile_sample;
    for (uint32_t eq = 0; eq < 3; eq++)
        tile_sample.edge[eq] = edge_samples[eq];
    tile_sample.depth = depth_sample;

    TriangleSample row_sample = tile_sample;

    uint32_t generated_quads_in_tile = 0;

    for (uint32_t y = 0; y < tile_dimensions[tile_sz][1]; y += 2)
    {
        TriangleSample quad_sample = row_sample;

        for (uint32_t x = 0; x < tile_dimensions[tile_sz][0]; x += 2)
        {
            QuadInfo quad;

            sample_quad(tile_x + x, tile_y + y, quad_sample, quad);

            bool quad_coverage = test_quad(quad);

            if (quad_coverage)
            {
                if (new_frag_shader_state)
                {
                    generate_frag_shader_state_packet();
                    new_frag_shader_state = false;
                }

                DEBUG_EMU( gprintf("RBOX : Generated packet for quad at (%d, %d)\n", tile_x + x, tile_y + y); )

                generate_quad_packet(quad);
                generated_quads_in_tile++;
            }

            sample_next_quad(quad_sample);
        }
        sample_next_row(row_sample);
    }

    DEBUG_EMU( gprintf("RBOX : Generated %d quads in tile\n", generated_quads_in_tile); )
}

void sample_next_row(TriangleSample &sample)
{
    for (uint32_t eq = 0; eq < 3; eq++)
        sample.edge[eq] += 2 * current_triangle.edge_eqs[eq].b;
    sample.depth += 2 * current_triangle.depth_eq.b;
}

void sample_next_quad(TriangleSample &sample)
{
    for (uint32_t eq = 0; eq < 3; eq++)
        sample.edge[eq] += 2 * current_triangle.edge_eqs[eq].a;
    sample.depth += 2 * current_triangle.depth_eq.a;
}

void sample_quad(uint32_t x, uint32_t y, TriangleSample quad_sample, QuadInfo &quad)
{
    DEBUG_EMU( gprintf("RBOX : Sampling quad at (%d, %d) -> start sample = (%llx, %llx, %llx, %llx)\n"
                      "\t\tequation coefficients = (\n\t\t\t(%llx, %llx),\n\t\t\t(%llx, %llx),\n\t\t\t(%llx, %llx),\n\t\t\t(%llx, %llx)\n\t\t)\n"
                      "\t\ttop_or_left_edges = (%d, %d, %d)\n",
                       x, y, quad_sample.edge[0], quad_sample.edge[1], quad_sample.edge[2], quad_sample.depth,
                       current_triangle.edge_eqs[0].a, current_triangle.edge_eqs[0].b,
                       current_triangle.edge_eqs[1].a, current_triangle.edge_eqs[1].b,
                       current_triangle.edge_eqs[2].a, current_triangle.edge_eqs[2].b,
                       current_triangle.depth_eq.a, current_triangle.depth_eq.b,
                       current_triangle.top_or_left_edge[0], current_triangle.top_or_left_edge[1], current_triangle.top_or_left_edge[2]); )

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
        DEBUG_EMU( gprintf("RBOX => Fragment %d Sample (%llx, %llx, %llx, %llx) Coverage = %d\n", f,
                          quad.fragment[f].sample.edge[0], quad.fragment[f].sample.edge[1], quad.fragment[f].sample.edge[2],
                          quad.fragment[f].sample.depth, quad.fragment[f].coverage); )
    }

    quad.x = x;
    quad.y = y;
}

bool test_quad(QuadInfo &quad)
{
    bool quad_coverage = false;

    for (uint32_t f = 0; f < 4; f++)
    {
        uint32_t x = quad.x + (f & 0x1);
        uint32_t y = quad.y + ((f >> 1) & 0x1);
        bool scissor_test = do_scissor_test(x, y);
        quad.fragment[f].coverage = quad.fragment[f].coverage && scissor_test;

        if (quad.fragment[f].coverage)
        {
            uint64_t frag_depth_stencil_address = compute_depth_stencil_buffer_address(x, y);
            uint32_t frag_depth_stencil = vmemread32(frag_depth_stencil_address);

            uint8_t frag_stencil = frag_depth_stencil >> 24;
            uint32_t frag_depth = frag_depth_stencil & 0x00FFFFFF;

            DEBUG_EMU( gprintf("RBOX => Testing fragment at (%d, %d) address = %016llx sample_depth = %08x fragment_depth_stencil = %08x\n",
                              x, y, frag_depth_stencil_address, quad.fragment[f].sample.depth, frag_depth_stencil); )

            bool depth_bound_test =  do_depth_bound_test(frag_depth);
            bool stencil_test = do_stencil_test(frag_stencil);
            bool depth_test = do_depth_test(frag_depth, quad.fragment[f].sample.depth);

            DEBUG_EMU( gprintf("RBOX => Test results : depth_bound = %d stencil = %d depth = %d\n", depth_bound_test, stencil_test, depth_test); )

            uint8_t out_stencil = stencil_update(frag_stencil, stencil_test, depth_test);

            uint32_t out_depth;
            if (depth_bound_test)
            {
                if (stencil_test)
                    out_depth = rbox_state[rbox_state_idx].depth_test_write_enable ? quad.fragment[f].sample.depth : frag_depth;
                else
                    out_depth = frag_depth;

                uint32_t out_depth_stencil = (out_stencil << 24) | out_depth;
                vmemwrite32(frag_depth_stencil_address, out_depth_stencil);
            }

            quad.fragment[f].coverage = quad.fragment[f].coverage && depth_bound_test && stencil_test && depth_test;
        }

        quad_coverage |= quad.fragment[f].coverage;
    }

    return quad_coverage;
}

uint64_t compute_depth_stencil_buffer_address(uint32_t x, uint32_t y)
{
    uint32_t row = y / 4;
    uint32_t line = x / 4;
    uint64_t depth_stencil_buffer_offset = row * rbox_state[rbox_state_idx].depth_stencil_buffer_row_pitch
                                       + line * 64 + (y & 0x3) * 16 + (x & 0x03) * 4;
    return rbox_state[rbox_state_idx].depth_stencil_buffer_ptr + depth_stencil_buffer_offset;
}

bool sample_inside_triangle(TriangleSample sample)
{
    return ((sample.edge[0] == 0) ? current_triangle.top_or_left_edge[0] : ((sample.edge[0] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[1] == 0) ? current_triangle.top_or_left_edge[1] : ((sample.edge[1] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0)) &&
           ((sample.edge[2] == 0) ? current_triangle.top_or_left_edge[2] : ((sample.edge[2] & EDGE_EQ_SAMPLE_SIGN_MASK) == 0));
}

bool do_scissor_test(uint32_t x, uint32_t y)
{
    DEBUG_EMU( gprintf("RBOX => Scissor test for fragment at (%d, %d) scissor rectangle (%d, %d, %d, %d)\n",
                      x, y, rbox_state[rbox_state_idx].scissor_start_x, rbox_state[rbox_state_idx].scissor_start_y,
                      rbox_state[rbox_state_idx].scissor_width, rbox_state[rbox_state_idx].scissor_height); )

    return    (x >= rbox_state[rbox_state_idx].scissor_start_x)
           && (y >= rbox_state[rbox_state_idx].scissor_start_y)
           && (x <= (rbox_state[rbox_state_idx].scissor_start_x + rbox_state[rbox_state_idx].scissor_width))
           && (y <= (rbox_state[rbox_state_idx].scissor_start_y + rbox_state[rbox_state_idx].scissor_height));
}

bool do_depth_bound_test(uint32_t frag_depth)
{
    return   !rbox_state[rbox_state_idx].depth_bound_enable
           || (    (frag_depth >= rbox_state[rbox_state_idx].depth_bound_min)
               &&  (frag_depth <= rbox_state[rbox_state_idx].depth_bound_max));
}

bool do_stencil_test(uint8_t frag_stencil)
{
    RBOXStencilState stencil_state;
    if (current_triangle.back_facing)
        stencil_state = rbox_state[rbox_state_idx].stencil_back_state;
    else
        stencil_state = rbox_state[rbox_state_idx].stencil_front_state;

    if (rbox_state[rbox_state_idx].stencil_test_enable)
    {
        uint8_t ref_stencil_masked = stencil_state.ref & stencil_state.compare_mask;
        uint8_t frag_stencil_masked = frag_stencil & stencil_state.compare_mask;
        switch (stencil_state.compare_op)
        {
            case RBOX_COMPARE_OP_NEVER :
                return false;
                break;
            case RBOX_COMPARE_OP_LESS :
                return ref_stencil_masked < frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_EQUAL :
                return ref_stencil_masked == frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_LESS_OR_EQUAL :
                return ref_stencil_masked <= frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_GREATER :
                return ref_stencil_masked > frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_NOT_EQUAL :
                return ref_stencil_masked != frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_GREATER_OR_EQUAL :
                return ref_stencil_masked >= frag_stencil_masked;
                break;
            case RBOX_COMPARE_OP_ALWAYS :
                return true;
                break;
        }
    }
    else
        return true;
}

bool do_depth_test(uint32_t frag_depth, uint32_t sample_depth)
{
    if (rbox_state[rbox_state_idx].depth_test_enable)
    {
        switch (rbox_state[rbox_state_idx].depth_test_compare_op)
        {
            case RBOX_COMPARE_OP_NEVER :
                return false;
                break;
            case RBOX_COMPARE_OP_LESS :
                return sample_depth < frag_depth;
                break;
            case RBOX_COMPARE_OP_EQUAL :
                return sample_depth == frag_depth;
                break;
            case RBOX_COMPARE_OP_LESS_OR_EQUAL :
                return sample_depth <= frag_depth;
                break;
            case RBOX_COMPARE_OP_GREATER :
                return sample_depth > frag_depth;
                break;
            case RBOX_COMPARE_OP_NOT_EQUAL :
                return sample_depth != frag_depth;
                break;
            case RBOX_COMPARE_OP_GREATER_OR_EQUAL :
                return sample_depth >= frag_depth;
                break;
            case RBOX_COMPARE_OP_ALWAYS :
                return true;
                break;
        }
    }
    else
        return true;
}

uint8_t stencil_update(uint8_t frag_stencil, bool stencil_test, bool depth_test)
{
    RBOXStencilState stencil_state;
    if (current_triangle.back_facing)
        stencil_state = rbox_state[rbox_state_idx].stencil_back_state;
    else
        stencil_state = rbox_state[rbox_state_idx].stencil_front_state;

    RBOXStencilOp stencil_op;
    if (rbox_state[rbox_state_idx].stencil_test_enable)
    {
        if (stencil_test && depth_test)
            stencil_op = (RBOXStencilOp) stencil_state.pass_op;
        else if (stencil_test && !depth_test)
            stencil_op = (RBOXStencilOp) stencil_state.depth_fail_op;
        else
            stencil_op = (RBOXStencilOp) stencil_state.fail_op;

        switch (stencil_op)
        {
            case RBOX_STENCIL_OP_KEEP :
                return frag_stencil & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_ZERO :
                return 0;
                break;
            case RBOX_STENCIL_OP_REPLACE :
                return stencil_state.ref & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_INC_CLAMP :
                return ((frag_stencil == 255) ? 255 : (frag_stencil + 1)) & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_DEC_CLAMP :
                return ((frag_stencil == 0) ? 0 : (frag_stencil - 1)) & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_INVERT :
                return (~frag_stencil) & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_INC_WRAP :
                return (frag_stencil + 1) & stencil_state.write_mask;
                break;
            case RBOX_STENCIL_OP_DEC_WRAP :
                return (frag_stencil - 1) & stencil_state.write_mask;
                break;
        }
    }
    else
        return frag_stencil;
}

void generate_quad_packet(QuadInfo quad)
{
    RBOXOutPcktQuadInfo quad_info_pckt;

    DEBUG_EMU( gprintf("RBOX => Generate quad packet\n"); )

    uint64_t packet = output.next_write_packet();

    for (uint32_t qw = 0; qw < 2; qw++)
        quad_info_pckt.qw[qw] = 0;

    quad_info_pckt.quad_info.type = RBOX_OUTPCKT_QUAD_INFO;
    quad_info_pckt.quad_info.x = quad.x >> 1;
    quad_info_pckt.quad_info.y = quad.y >> 1;
    quad_info_pckt.quad_info.smpl_idx = 0;
    quad_info_pckt.quad_info.triangle_data_ptr = current_triangle.triangle_data_ptr;
    for (uint32_t f = 0; f < 4; f++)
        quad_info_pckt.quad_info.mask = quad.fragment[f].coverage ? (quad_info_pckt.quad_info.mask | (3 << (2 * f))) : quad_info_pckt.quad_info.mask;

    for (uint32_t qw = 0; qw < 2; qw++)
    {
        DEBUG_EMU( gprintf("RBOX => Writing QW %016llx at address %llx\n", quad_info_pckt.qw[qw], packet); )
        vmemwrite64(packet, quad_info_pckt.qw[qw]);
        packet = packet + 8;
    }

    RBOXOutPcktQuadData quad_data_pckt;

    for (uint32_t f = 0; f < 4; f++)
        quad_data_pckt.ps[f] = convert_edge_to_fp32(quad.fragment[f].sample.edge[1]);

    for (uint32_t qw = 0; qw < 2; qw++)
    {
        DEBUG_EMU( gprintf("RBOX => Writing QW %016llx at address %llx\n", quad_data_pckt.qw[qw], packet); )
        vmemwrite64(packet, quad_data_pckt.qw[qw]);
        packet = packet + 8;
    }

    for (uint32_t f = 0; f < 4; f++)
        quad_data_pckt.ps[f] = convert_edge_to_fp32(quad.fragment[f].sample.edge[2]);

    for (uint32_t qw = 0; qw < 2; qw++)
    {
        DEBUG_EMU( gprintf("RBOX => Writing QW %016llx at address %llx\n", quad_data_pckt.qw[qw], packet); )
        vmemwrite64(packet, quad_data_pckt.qw[qw]);
        packet = packet + 8;
    }

    for (uint32_t f = 0; f < 4; f++)
        quad_data_pckt.ps[f] = convert_depth_to_fp32(quad.fragment[f].sample.depth);

    for (uint32_t qw = 0; qw < 2; qw++)
    {
        DEBUG_EMU( gprintf("RBOX => Writing QW %016llx at address %llx\n", quad_data_pckt.qw[qw], packet); )
        vmemwrite64(packet, quad_data_pckt.qw[qw]);
        packet = packet + 8;
    }

    output.write_packet(8);
    output.push_packet();
}

float32_t convert_edge_to_fp32(int64_t edge)
{
    uint8_t sign = edge & EDGE_EQ_SAMPLE_SIGN_MASK;
    uint64_t edge_abs = sign ? -edge : edge;
    float32_t edge_abs_fp32 = float32_t(edge_abs) / float32_t(1 << EDGE_EQ_SAMPLE_FRAC_BITS);
    return (sign ? -edge_abs_fp32 : edge_abs_fp32);
}

float32_t convert_depth_to_fp32(uint32_t depth)
{
    return float32_t(depth) / float32_t((1 << 24) - 1);
}

void generate_frag_shader_state_packet()
{
    RBOXOutPcktFrgShdrState f_sh_pckt;

    DEBUG_EMU( gprintf("RBOX => Generate Fragment Shader State Packet\n"); )

    for (uint32_t qw = 0; qw < 4; qw++)
        f_sh_pckt.qw[qw] = 0;

    f_sh_pckt.state.type = RBOX_OUTPCKT_STATE_INFO;
    f_sh_pckt.state.state_idx = rbox_state_idx;
    f_sh_pckt.state.frg_shdr_func_ptr = frag_shader_state.frag_shader_function_ptr;
    f_sh_pckt.state.frg_shdr_state_ptr = frag_shader_state.frag_shader_state_ptr;

    uint64_t packet = output.next_write_packet();

    for (uint32_t qw = 0; qw < 4; qw++)
    {
        DEBUG_EMU( gprintf("RBOX => Writing QW %016llx at address %llx\n", f_sh_pckt.qw[qw], packet + qw * 8); )
        vmemwrite64(packet + qw * 8, f_sh_pckt.qw[qw]);
    }

    output.write_packet(4);
    output.push_packet();
}

void tile_position_to_pixels(uint32_t &tile_x, uint32_t &tile_y, RBOXTileSize tile_size)
{
    switch(tile_size)
    {
        case RBOX_TILE_SIZE_64x64 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 6;
            DEBUG_EMU( gprintf("RBOX : Tile Size 64x64 Position (%d, %d)\n", tile_x, tile_y); )
            break;
        case RBOX_TILE_SIZE_64x32 :
            tile_x = tile_x << 6;
            tile_y = tile_y << 5;
            DEBUG_EMU( gprintf("RBOX : Tile Size 64x32 Position (%d, %d)\n", tile_x, tile_y); )
            break;
        case RBOX_TILE_SIZE_32x32 :
            tile_x = tile_x << 5;
            tile_y = tile_y << 5;
            DEBUG_EMU( gprintf("RBOX : Tile Size 32x32 Position (%d, %d)\n", tile_x, tile_y); )
            break;
        case RBOX_TILE_SIZE_16x16 :
            tile_x = tile_x << 4;
            tile_y = tile_y << 4;
            DEBUG_EMU( gprintf("RBOX : Tile Size 16x16 Position (%d, %d)\n", tile_x, tile_y); )
            break;
        case RBOX_TILE_SIZE_8x8 :
            tile_x = tile_x << 3;
            tile_y = tile_y << 3;
            DEBUG_EMU( gprintf("RBOX : Tile Size 8x8 Position (%d, %d)\n", tile_x, tile_y); )
            break;
        case RBOX_TILE_SIZE_4x4 :
            tile_x = tile_x << 2;
            tile_y = tile_y << 2;
            DEBUG_EMU( gprintf("RBOX : Tile Size 4x4 Position (%d, %d)\n", tile_x, tile_y); )
            break;
    }
}

