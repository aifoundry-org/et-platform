
#ifndef _RBOX_PI_H
#define _RBOX_PI_H

namespace RBOX
{

    typedef enum 
    {
        INPCKT_TRIANGLE_WITH_TILE_64x64   = 0,
        INPCKT_TRIANGLE_WITH_TILE_128x128 = 1,
        INPCKT_LARGE_TRIANGLE             = 2,
        INPCKT_FULLY_COVERED_TILE         = 3,
        INPCKT_LARGE_TRIANGLE_TILE        = 4,
        INPCKT_RBOX_STATE                 = 5,
        INPCKT_FRAG_SHADING_STATE         = 6,
        INPCKT_END_OF_INPUT_BUFFER        = 7
    } InPcktTypeT;
    
    typedef enum
    {
        TILE_SIZE_64x64 = 0,
        TILE_SIZE_64x32 = 1,
        TILE_SIZE_32x32 = 2,
        TILE_SIZE_16x16 = 3,
        TILE_SIZE_8x8   = 4,
        TILE_SIZE_4x4   = 5
    } TileSizeT;
    
    typedef enum
    {
        TRI_FACING_FRONT = 0,
        TRI_FACING_BACK  = 1
    } TriangleFacingT;
    
    #pragma pack(push, 1)
    
    typedef struct 
    {
        int32_t a;    // 2's complement 9.14
        int32_t b;    // 2's complement 9.14
    } EdgeEq64x64T;
    
    static const uint32_t EDGE_EQ_64X64_COEF_INT_BITS  =  9;
    static const uint32_t EDGE_EQ_64X64_COEF_FRAC_BITS = 14;
    
    typedef struct
    {
        int32_t a;    // 2's complement 9.15
        int32_t b;    // 2's complement 9.15
    } EdgeEq128x128T;
    
    static const uint32_t EDGE_EQ_128X128_COEF_INT_BITS  =  9;
    static const uint32_t EDGE_EQ_128X128_COEF_FRAC_BITS = 15;
    
    typedef struct
    {
        // Coefficients : 2's complement 9.25
        uint32_t a_low;
        uint32_t b_low;
        int16_t  a_high;
        int16_t  b_high;
    } EdgeEqLargeTriT;
    
    static const uint32_t EDGE_EQ_LARGE_TRI_COEF_INT_BITS  =  9;
    static const uint32_t EDGE_EQ_LARGE_TRI_COEF_FRAC_BITS = 25;
    
    typedef struct
    {
        uint32_t a;   // UNORM24 or FLOAT32
        uint32_t b;   // UNORM24 or FLOAT32
    } DepthEqT;
    
    typedef struct
    {
        int32_t e;       // 2's complement 15.14
    } EdgeSample64x64T;
    
    static const uint32_t EDGE_EQ_64X64_SAMPLE_INT_BITS  = 15;
    static const uint32_t EDGE_EQ_64X64_SAMPLE_FRAC_BITS = 14;
    
    typedef struct
    {
        int32_t e;       // 2's complement 15.15
    } EdgeSample128x128T;
    
    static const uint32_t EDGE_EQ_128X128_SAMPLE_INT_BITS  = 15;
    static const uint32_t EDGE_EQ_128X128_SAMPLE_FRAC_BITS = 15;
    
    typedef struct
    {
        int64_t e;        // 2's complement 15.25
    } EdgeSampleLargeTriT;
    
    static const uint32_t EDGE_EQ_LARGE_TRI_SAMPLE_INT_BITS  = 15;
    static const uint32_t EDGE_EQ_LARGE_TRI_SAMPLE_FRAC_BITS = 25;
    
    typedef union
    {
        uint64_t type :  3,
                 qw0  : 61;
        uint64_t qw;
    } InPcktHeaderT;
    
    typedef union
    {
        struct
        {
            uint32_t type : 3,
                     dw0  : 29;
            uint32_t dw[7];
        } header;
        uint64_t qw[4];
    } InPckt256bT;
    
    typedef union
    {
        struct
        {
            uint64_t type :  3,
                     qw0  : 61;
            uint64_t qw[7];
        } header;
        uint64_t qw[8];
    } InputPacket512bT;
    
    
    // Triangle inside 64x64 tile + Tile with up to 64x64 size
    typedef union
    {
        struct
        {
            uint64_t         type       :  3,
                             tile_left  : 13,
                             tile_top   : 13,
                             tri_facing :  1,
                             tile_size  :  3,
                             unused     : 31;
            EdgeEq64x64T     edge_eqs[3];
            DepthEqT         depth_eq;
            uint64_t         triangle_data_ptr;
            EdgeSample64x64T edge[3];
            uint32_t         depth;
        } tri_with_tile_64x64;
        uint64_t qw[8];
    } InPcktTriWithTile64x64T;
    
    // Triangle inside 128x128 + Tile up to 64x64 size
    typedef union
    {
        struct
        {
            uint64_t           type       :  3,
                               tile_left  : 13,
                               tile_top   : 13,
                               tri_facing :  1,
                               tile_size  :  3,
                               unused     : 31; 
            EdgeEq128x128T     edge_eqs[3];
            DepthEqT           depth_eq;
            uint64_t           triangle_data_ptr;
            EdgeSample128x128T edge[3];
            uint32_t           depth;
        } tri_with_tile_128x128;
        uint64_t qw[8];
    } InPcktTriWithTile128x128T;
    
    // Large triangle (triangle bounding box is larger than 128x128)
    typedef union
    {
        struct
        {
            uint64_t        type       :  3,
                            tri_facing :  1,
                            unused1    : 60; 
            EdgeEqLargeTriT edge_eqs[3];
            DepthEqT        depth_eq;
            uint32_t        unused2;
            uint64_t        triangle_data_ptr;
        } triangle;
        uint64_t qw[8];
    } InPcktLargeTriT;
    
    // Fully covered tile up to 64x64 size
    typedef union
    {
        struct
        {
            uint32_t        type      :  3,
                            tile_left : 13,
                            tile_top  : 13,
                            tile_size :  3;
            EdgeSample64x64T edge[3];
            uint32_t        depth;
            uint32_t        unused2[2];
        } tile;
        uint64_t qw[4];
    } InPcktFullyCoveredTileT;
    
    // Tile in large triangle, up to 64x64 size
    typedef union
    {
        struct
        {
            uint32_t           type      :  3,
                               tile_left : 13,
                               tile_top  : 13,
                               tile_size :  3;
            uint32_t           depth;
            EdgeSampleLargeTriT edge[3];
        } tile;
        uint64_t qw[4];
    } InPcktLargeTriTileT;
    
    
    typedef struct
    {
        uint64_t fail_op       :  3,
                 pass_op       :  3,
                 depth_fail_op :  3,
                 compare_op    :  3,
                 compare_mask  :  8,
                 write_mask    :  8,
                 ref           :  8,
                 unused3       : 28;
    } StencilStateT; 
    
    typedef enum
    {
        FORMAT_D16_UNORM                  = 124,
        FORMAT_X8_D24_UNORM_PACK32        = 125,
        FORMAT_D32_SFLOAT                 = 126,
        FORMAT_D16_UNORM_S8_UINT          = 128,
        FORMAT_D24_UNORM_S8_UINT          = 129,
        FORMAT_D32_SFLOAT_S8_UINT         = 130
    } DepthStencilFormatT;
    
    typedef enum
    {
        COMPARE_OP_NEVER            = 0,
        COMPARE_OP_LESS             = 1,
        COMPARE_OP_EQUAL            = 2,
        COMPARE_OP_LESS_OR_EQUAL    = 3,
        COMPARE_OP_GREATER          = 4,
        COMPARE_OP_NOT_EQUAL        = 5,
        COMPARE_OP_GREATER_OR_EQUAL = 6,
        COMPARE_OP_ALWAYS           = 7,
        MAX_COMPARE_OPERATION       = 7
    } CompareOpT;
    
    typedef enum
    {
        STENCIL_OP_KEEP      = 0,
        STENCIL_OP_ZERO      = 1,
        STENCIL_OP_REPLACE   = 2,
        STENCIL_OP_INC_CLAMP = 3,
        STENCIL_OP_DEC_CLAMP = 4,
        STENCIL_OP_INVERT    = 5,
        STENCIL_OP_INC_WRAP  = 6,
        STENCIL_OP_DEC_WRAP  = 7,
        MAX_STENCIL_OP       = 7
    } StencilOpT;
    
    typedef struct
    {
        // QW0
        uint8_t  type    : 3,
                 unused1 : 5;
        uint8_t  msaa_enable             :  1,
                 msaa_samples            :  4,
                 msaa_shading_enable     :  1,
                 msaa_alpha_to_coverage  :  1,
                 msaa_alpha_to_one       :  1;
        uint16_t msaa_sample_mask        : 16;
        uint32_t msaa_min_sample_shading : 32;
        // QW1
        uint64_t depth_stencil_buffer_ptr;
        // QW2
        uint64_t depth_stencil_buffer_format    :  9,
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
                 fragment_shader_reads_bary     :  1,
                 fragment_shader_reads_depth    :  1,
                 fragment_shader_reads_coverage :  1,
                 fragment_shader_per_sample     :  1,
                 minion_hart_tile_width         :  2,
                 minion_hart_tile_height        :  2,
                 shire_layout_width             :  2,
                 shire_layout_height            :  2,
                 unused2                        : 20;
        // QW3
        uint32_t depth_bound_min;
        uint32_t depth_bound_max;
        // QW4
        uint32_t depth_min;
        uint32_t depth_max;
        // QW5
        uint64_t scissor_start_x : 14,
                 scissor_start_y : 14,
                 scissor_height  : 14,
                 scissor_width   : 14;
        // QW6
        StencilStateT stencil_front_state;
        // QW7
        StencilStateT stencil_back_state;
    } StateT;
    
    // Render state
    typedef union
    {
        StateT state;
        uint64_t qw[8];
    } InPcktRBOXStateT;
    
    typedef struct
    {
        // QW0
        uint64_t type    : 3,
                 unused1 : 61;
        // QW1
        uint64_t frag_shader_function_ptr;
        // QW2
        uint64_t frag_shader_state_ptr;
        // QW3
        uint64_t unused2;
    } FragmentShaderStateT;
    
    // Drawcall state
    typedef union
    {
        FragmentShaderStateT state;
        uint64_t qw[4];
    } InPcktFrgmtShdrStateT;
    
    //  End of Input Buffer mark
    typedef union
    {
        struct
        {
            uint64_t type    :  3,
                     unused1 : 61;
            uint64_t unused2[3];
        } header;
        uint64_t qw[4];
    } InPcktEndOfInBufT;
    
    typedef enum
    {
        OUTPCKT_STATE_INFO = 0,
        OUTPCKT_QUAD_INFO  = 1,
        OUTPCKT_END_PHASE  = 2
    } OutPcktTypeT;
    
    typedef union
    {
        struct 
        {
            uint64_t type :  2,
                     qw0  : 62;
            uint64_t qw1;
            uint64_t qw2;
            uint64_t qw3;
        } header;
        uint64_t qw[4];
    } OutPckt256bT;
    
    typedef union
    {
        struct
        {
            uint64_t type      :  2,
                     state_idx :  3,
                     unused1   : 59;
            uint64_t frg_shdr_func_ptr;
            uint64_t frg_shdr_state_ptr;
            uint64_t unused2;
        } state;
        uint64_t qw[4];
    } OutPcktFrgShdrStateT;
    
    typedef union
    {
        struct
        {
            uint16_t type;
            uint16_t x[2];
            uint16_t y[2];
            uint8_t  smpl_idx;
            uint8_t  mask;
            uint64_t triangle_data_ptr[2];
            uint32_t unused;
        } quad_info;
        uint64_t qw[4];
    } OutPcktQuadInfoT;
    
    typedef union
    {
        uint64_t  qw[4];
        uint32_t  dw[8];
        uint16_t  hw[16];
        float ps[8];
    } OutPcktQuadDataT;
    
    typedef enum
    {
        CONFIG_ESR               = 0,
        INPUT_BUFFER_PAGES_ESR   = 1,
        INPUT_BUFFER_CONFIG_ESR  = 2,
        OUTPUT_BUFFER_PAGE_ESR   = 3,
        OUTPUT_BUFFER_CONFIG_ESR = 4,
        STATUS_ESR               = 5,
        START_ESR                = 6,
        CONSUME_ESR              = 7
    } ESRIdT;
    
    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t unused;
        } fields;
    } ESRCfgT;

    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t page0        : 26,
                     reserved0    :  5,
                     page0_enable :  1,
                     page1        : 26,
                     reserved1    :  5,
                     page1_enable :  1; 
        }  fields;
    } ESRInBufPgT;
    
    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t start_offset : 16,
                     reserved0    : 16,
                     buffer_size  : 16,
                     reserved1    : 16;
        }  fields;
    } ESRInBufCfgT;
    
    typedef union
    {
        uint64_t value;
        struct {
            uint64_t page        : 26,
                     reserved0   :  5,
                     page_enable :  1,
                     reserved1   : 32;
        } fields;
    } ESROutBufPgT;
    
    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t start_offset  : 15,
                     reserved0     : 17,
                     buffer_size   :  3,
                     port_id       :  2,
                     max_msgs      :  2,
                     max_pckts_msg :  3,
                     reserved1     : 22;
        } fields;
    } ESROutBufCfgT;
    
    typedef enum
    {
        READY        = 0x00,
        WORKING      = 0x01,
        FINISHED     = 0x02,
        CONFIG_ERROR = 0x40
    } StatusT;

    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t status   :  8,
                     reserved : 56;
        } fields;
    } ESRStatusT;
    
    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t start    :  1,
                     reserved : 63;
        } fields;
    } ESRStartT;
    
    typedef union
    {
        uint64_t value;
        struct
        {
            uint64_t packet_credits :  8,
                     msg_credits    :  2,
                     reserved0      :  6,
                     hart_id        :  8,
                     reserved1      :  8;
        } fields;
    } ESRConsumeT;

    #pragma pack(pop)

}

#endif


