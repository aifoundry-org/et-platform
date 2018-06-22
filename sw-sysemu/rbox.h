#ifndef _RBOX_H
#define _RBOX_H

#include <cstddef>
#include <cstdio>

#include "emu.h"

typedef enum 
{
    RBOX_INPCKT_TRIANGLE_WITH_TILE_64x64   = 0,
    RBOX_INPCKT_TRIANGLE_WITH_TILE_128x128 = 1,
    RBOX_INPCKT_LARGE_TRIANGLE             = 2,
    RBOX_INPCKT_FULLY_COVERED_TILE         = 3,
    RBOX_INPCKT_LARGE_TRIANGLE_TILE        = 4,
    RBOX_INPCKT_RBOX_STATE                 = 5,
    RBOX_INPCKT_FRAG_SHADING_STATE         = 6
} RBOXInPcktType;

typedef enum
{
    RBOX_TILE_SIZE_64x64 = 0,
    RBOX_TILE_SIZE_64x32 = 1,
    RBOX_TILE_SIZE_32x32 = 2,
    RBOX_TILE_SIZE_16x16 = 3,
    RBOX_TILE_SIZE_8x8   = 4,
    RBOX_TILE_SIZE_4x4   = 5
} RBOXTileSize;

typedef enum
{
    RBOX_TRI_FACING_FRONT = 0,
    RBOX_TRI_FACING_BACK  = 1
} RBOXTriangleFacing;

#pragma pack(push, 1)
typedef struct 
{
    int32 a;    // 2's complement 9.14
    int32 b;    // 2's complement 9.14
} EdgeEq64x64;

#define EDGE_EQ_64X64_COEF_INT_BITS  9
#define EDGE_EQ_64X64_COEF_FRAC_BITS 14

typedef struct
{
    int32 a;    // 2's complement 9.15
    int32 b;    // 2's complement 9.15
} EdgeEq128x128;

#define EDGE_EQ_128X128_COEF_INT_BITS  9
#define EDGE_EQ_128X128_COEF_FRAC_BITS 15

typedef struct
{
    // Coefficients : 2's complement 9.25
    uint32 a_low;
    uint32 b_low;
    int16  a_high;
    int16  b_high;
} EdgeEqLargeTri;

#define EDGE_EQ_LARGE_TRI_COEF_INT_BITS  9
#define EDGE_EQ_LARGE_TRI_COEF_FRAC_BITS 25

typedef struct
{
    uint32 a;   // UNORM24 or FLOAT32
    uint32 b;   // UNORM24 or FLOAT32
} DepthEq;

typedef struct
{
    int32 e;       // 2's complement 15.14
} EdgeSample64x64;

#define EDGE_EQ_64X64_SAMPLE_INT_BITS  15
#define EDGE_EQ_64X64_SAMPLE_FRAC_BITS 14

typedef struct
{
    int32 e;       // 2's complement 15.15
} EdgeSample128x128;

#define EDGE_EQ_128X128_SAMPLE_INT_BITS  15
#define EDGE_EQ_128X128_SAMPLE_FRAC_BITS 15

typedef struct
{
    int64 e;        // 2's complement 15.25
} EdgeSampleLargeTri;

#define EDGE_EQ_LARGE_TRI_SAMPLE_INT_BITS  15
#define EDGE_EQ_LARGE_TRI_SAMPLE_FRAC_BITS 25

typedef union
{
    uint64 type :  3,
           qw0  : 61;
    uint64 qw;
} RBOXInPcktHeader;

typedef union
{
    struct
    {
        uint32 type : 3,
               dw0  : 29;
        uint32 dw[7];
    } header;
    uint64 qw[4];
} RBOXInPckt256b;

typedef union
{
    struct
    {
        uint32          type      :  3,
                        tile_left : 13,
                        tile_top  : 13,
                        tile_size :  3;
        EdgeSample64x64 edge[3];
        uint32          depth;
        uint32          unused2[2];
    } tile;
    uint64 qw[4];
} RBOXInPcktFullyCoveredTile;

//typedef union
//{
//    struct
//    {
//        uint32 type      :  3,
//               tile_left : 10,
//               tile_top  : 10,
//               unused1   :  9;
//        MediumTriEdgeSample edge[3];
//        uint32 depth;
//        uint32 unused2[3];
//    } tile;
//    uint64 qw[4];
//} RBOXInPcktMediumTriTile;

typedef union
{
    struct
    {
        uint32             type      :  3,
                           tile_left : 13,
                           tile_top  : 13,
                           tile_size :  3;
        uint32             depth;
        EdgeSampleLargeTri edge[3];
    } tile;
    uint64 qw[4];
} RBOXInPcktLargeTriTile;


typedef union
{
    struct
    {
        uint64 type :  3,
               qw0  : 61;
        uint64 qw[7];
    } header;
    uint64 qw[8];
} RBOXInputPacket512b;


typedef union
{
    struct
    {
        uint64          type       :  3,
                        tile_left  : 13,
                        tile_top   : 13,
                        tri_facing :  1,
                        tile_size  :  3,
                        unused     : 31;
        EdgeEq64x64     edge_eqs[3];
        DepthEq         depth_eq;
        uint64          triangle_data_ptr;
        EdgeSample64x64 edge[3];
        uint32          depth;
    } tri_with_tile_64x64;
    uint64 qw[8];
} RBOXInPcktTriWithTile64x64;

typedef union
{
    struct
    {
        uint64            type       :  3,
                          tile_left  : 13,
                          tile_top   : 13,
                          tri_facing :  1,
                          tile_size  :  3,
                          unused     : 31; 
        EdgeEq128x128     edge_eqs[3];
        DepthEq           depth_eq;
        uint64            triangle_data_ptr;
        EdgeSample128x128 edge[3];
        uint32            depth;
    } tri_with_tile_128x128;
    uint64 qw[8];
} RBOXInPcktTriWithTile128x128;

typedef union
{
    struct
    {
        uint64         type       :  3,
                       tri_facing :  1,
                       unused1    : 60; 
        EdgeEqLargeTri edge_eqs[3];
        DepthEq        depth_eq;
        uint32         unused2;
        uint64         triangle_data_ptr;
    } triangle;
    uint64 qw[8];
} RBOXInPcktLargeTri;

typedef struct
{
    uint64 fail_op       :  3,
           pass_op       :  3,
           depth_fail_op :  3,
           compare_op    :  3,
           compare_mask  :  8,
           write_mask    :  8,
           ref           :  8,
           unused3       : 28;
} RBOXStencilState; 

typedef enum
{
    FORMAT_D16_UNORM                  = 124,
    FORMAT_X8_D24_UNORM_PACK32        = 125,
    FORMAT_D32_SFLOAT                 = 126,
    FORMAT_D16_UNORM_S8_UINT          = 128,
    FORMAT_D24_UNORM_S8_UINT          = 129,
    FORMAT_D32_SFLOAT_S8_UINT         = 130
} RBOXDepthStencilFormat;

typedef enum
{
    RBOX_COMPARE_OP_NEVER            = 0,
    RBOX_COMPARE_OP_LESS             = 1,
    RBOX_COMPARE_OP_EQUAL            = 2,
    RBOX_COMPARE_OP_LESS_OR_EQUAL    = 3,
    RBOX_COMPARE_OP_GREATER          = 4,
    RBOX_COMPARE_OP_NOT_EQUAL        = 5,
    RBOX_COMPARE_OP_GREATER_OR_EQUAL = 6,
    RBOX_COMPARE_OP_ALWAYS           = 7,
    RBOX_MAX_COMPARE_OPERATION       = 7
} RBOXCompareOp;

typedef enum
{
    RBOX_STENCIL_OP_KEEP      = 0,
    RBOX_STENCIL_OP_ZERO      = 1,
    RBOX_STENCIL_OP_REPLACE   = 2,
    RBOX_STENCIL_OP_INC_CLAMP = 3,
    RBOX_STENCIL_OP_DEC_CLAMP = 4,
    RBOX_STENCIL_OP_INVERT    = 5,
    RBOX_STENCIL_OP_INC_WRAP  = 6,
    RBOX_STENCIL_OP_DEC_WRAP  = 7,
    RBOX_MAX_STENCIL_OP       = 7
} RBOXStencilOp;

typedef struct
{
    // QW0
    uint8 type    : 3,
          unused1 : 5;
    uint8  msaa_enable             :  1,
           msaa_samples            :  4,
           msaa_shading_enable     :  1,
           msaa_alpha_to_coverage  :  1,
           msaa_alpha_to_one       :  1;
    uint16 msaa_sample_mask        : 16;
    uint32 msaa_min_sample_shading : 32;
    // QW1
    uint64 depth_stencil_buffer_ptr;
    // QW2
    uint64 depth_stencil_buffer_format    :  9,
           depth_stencil_buffer_tile_mode :  1,
           depth_stencil_buffer_row_pitch : 13,
           depth_clamp_enable             :  1,
           depth_bound_enable             :  1,
           depth_test_enable              :  1,
           depth_test_write_enable        :  1,
           depth_test_compare_op          :  3,
           early_frag_tests_enable        :  1,
           stencil_test_enable            :  1,
           fragment_shader_disabled       :  1,
           unused2                        : 31;
    // QW3
    uint32 depth_bound_min;
    uint32 depth_bound_max;
    // QW4
    uint32 depth_min;
    uint32 depth_max;
    // QW5
    uint64 scissor_start_x : 14,
           scissor_start_y : 14,
           scissor_height  : 14,
           scissor_width   : 14;
    // QW6
    RBOXStencilState stencil_front_state;
    // QW7
    RBOXStencilState stencil_back_state;
} RBOXState;

typedef union
{
    RBOXState state;
    uint64 qw[8];
} RBOXInPcktRBOXState;

typedef struct
{
    // QW0
    uint64 type    : 3,
           unused1 : 61;
    // QW1
    uint64 frag_shader_function_ptr;
    // QW2
    uint64 frag_shader_state_ptr;
    // QW3
    uint64 unused2;
} FragmentShaderState;

typedef union
{
    FragmentShaderState state;
    uint64 qw[4];
} RBOXInPcktFrgmtShdrState;

typedef enum
{
    RBOX_OUTPCKT_STATE_INFO    = 0,
    RBOX_OUTPCKT_QUAD_INFO     = 1
} RBOXOutPcktType;

typedef union
{
    struct 
    {
        uint64 type :  2,
               qw0  : 62;
        uint64 qw1;
    } header;
    uint64 qw[2];
} RBOXOutPckt128b;

typedef union
{
    struct
    {
        uint64 type      :  2,
               state_idx :  3,
               unused1   : 59;
        uint64 frg_shdr_func_ptr;
        uint64 frg_shdr_state_ptr;
        uint64 unused2;
    } state;
    uint64 qw[4];
} RBOXOutPcktFrgShdrState;

typedef union
{
    struct
    {
        uint16 type;
        uint16 x;
        uint16 y;
        uint8 smpl_idx;
        uint8 mask;
        uint64 triangle_data_ptr;
    } quad_info;
    uint64 qw[2];
} RBOXOutPcktQuadInfo;

typedef union
{
    uint64 qw[2];
    uint32 dw[4];
    uint16 hw[8];
    float32 ps[4];
} RBOXOutPcktQuadData;
#pragma pack(pop)

class RingBuffer
{
private : 
    uint64 buffer;
    uint32 size;    // In 64b words
    uint32 packets;
    uint64 next_packet;

public :
    RingBuffer() : buffer(0), size(0), packets(0), next_packet(0) {}
    void initialize(uint64 b, uint32 sz) {buffer = b; size = sz;}
    void reset() {next_packet = buffer; packets = 0;}
    void push_packet() {packets++;}
    bool consume_packet()
    {
        if (packets > 0)
        {
            packets--;
    
            return true;
        }
        else
            return false;
    }
    uint64 read_next_packet(uint32 sz)
    {
        if (packets == 0) return 0;
        if ((next_packet + sz * 8) >= (buffer + size * 8))
            next_packet = buffer;
        else
            next_packet += sz * 8;
        return next_packet;
    }
    uint64 read_packet()
    {
        if (packets == 0)
            return 0;
        else
            return next_packet;
    }
    uint64 next_write_packet()
    {
        return next_packet;
    }
    uint64 write_packet(uint32 sz)
    {
        if ((next_packet + sz * 8) >= (buffer + size * 8))
            next_packet = buffer;
        else
            next_packet += sz * 8;
        return next_packet;
    }
};

typedef struct
{
    // Evaluation adder precision is 2's complement 18.24
    int64 a;
    int64 b;
} EdgeEq;

typedef struct
{
    EdgeEq edge_eqs[3];
    DepthEq depth_eq;
    bool back_facing;
    uint64 triangle_data_ptr;
    bool top_or_left_edge[3];
} TriangleInfo;

typedef struct
{
    int64 edge[3];     // Evaluation adder precision is 2's complement 18.24
    uint32 depth;
} TriangleSample;

typedef struct
{
    float32 i;
    float32 j;
} BarycentricCoord;

typedef struct
{
    TriangleSample sample;
    bool coverage;
    BarycentricCoord coord;
} FragmentInfo;

typedef struct
{
    uint32 x;
    uint32 y;
    FragmentInfo fragment[4];
} QuadInfo;

// Evaluators operate on 2's complement 18.25 (43 bits)
// 
//  - Sign bit        :  42
//  - Integer bits    : [42:25]
//  - Fractional bits : [24:0]
//
#define EDGE_EQ_SAMPLE_SIGN_MASK 0x0000040000000000ULL
#define EDGE_EQ_SAMPLE_INT_BITS  18
#define EDGE_EQ_SAMPLE_INT_MASK  0x000003fffe000000ULL
#define EDGE_EQ_SAMPLE_FRAC_BITS 25
#define EDGE_EQ_SAMPLE_FRAG_MASK 0x0000000001ffffffULL
#define EDGE_EQ_COEF_INT_BITS  9
#define EDGE_EQ_COEF_FRAC_BITS 25

void set_rbox(uint64 inStream, uint32 inStreamSz, uint64 outStream, uint32 outStreamSz);
void reset_input_stream();
void push_packet();
void reset_output_stream();
bool consume_packet();
void process();
uint32 process_packet(uint64 packet);
void generate_tile(uint32 tile_x, uint32 tile_y, int64 edge_samples[3], uint32 depth_sample, RBOXTileSize tile_sz);
void sample_next_row(TriangleSample &sample);
void sample_next_quad(TriangleSample &sample);
void sample_quad(uint32 x, uint32 y, TriangleSample quad_sample, QuadInfo &quad);
bool test_quad(QuadInfo &quad);
bool sample_inside_triangle(TriangleSample sample);
uint64 compute_depth_stencil_buffer_address(uint32 x, uint32 y);
bool do_depth_test(uint32 frag_depth, uint32 sample_depth);
bool do_stencil_test(uint8 frag_stencil);
bool do_depth_bound_test(uint32 frag_depth);
bool do_scissor_test(uint32 x, uint32 y);
uint8 stencil_update(uint8 frag_stencil, bool stencil_test, bool depth_test);
void generate_quad_packet(QuadInfo quad);
float32 convert_edge_to_fp32(int64 edge);
float32 convert_depth_to_fp32(uint32 depth);
void generate_frag_shader_state_packet();
void tile_position_to_pixels(uint32 &tile_x, uint32 &tile_y, RBOXTileSize tile_size);

#endif // _RBOX_H
