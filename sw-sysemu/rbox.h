#ifndef _RBOX_H
#define _RBOX_H

#include <cstddef>
#include <cstdio>
#include <vector>
#include <utility>

#include "emu.h"
#include "emu_defines.h"

#include "rbox_pi.h"

namespace RBOX
{

    class RBOXEmu
    {
    
    private :
        
        typedef struct
        {
            // Evaluation adder precision is 2's complement 18.24
            int64_t a;
            int64_t b;
        } EdgeEqT;
        
        typedef struct
        {
            EdgeEqT  edge_eqs[3];
            DepthEqT depth_eq;
            bool     back_facing;
            uint64_t triangle_data_ptr;
            bool     top_or_left_edge[3];
        } TriangleInfoT;
        
        typedef struct
        {
            int64_t edge[3];     // Evaluation adder precision is 2's complement 18.24
            DepthT  depth;
        } TriangleSampleT;
        
        typedef struct
        {
            float i;
            float j;
        } BarycentricCoordT;
        
        typedef struct
        {
            TriangleSampleT sample;
            bool coverage;
            BarycentricCoordT coord;
        } FragmentInfoT;
        
        typedef struct
        {
            uint32_t x;
            uint32_t y;
            FragmentInfoT fragment[4];
            uint64_t triangle_data_ptr;
        } QuadInfoT;
        
        static const uint32_t RBOX_FRAGS_PER_PACKET = 8;
        
        // Evaluators operate on 2's complement 18.25 (43 bits)
        // 
        //  - Sign bit        :  42
        //  - Integer bits    : [42:25]
        //  - Fractional bits : [24:0]
        //
        static const uint64_t EDGE_EQ_SAMPLE_SIGN_MASK = 0x0000040000000000ULL;
        static const uint32_t EDGE_EQ_SAMPLE_INT_BITS  = 18;
        static const uint64_t EDGE_EQ_SAMPLE_INT_MASK  = 0x000003fffe000000ULL;
        static const uint32_t EDGE_EQ_SAMPLE_FRAC_BITS = 25;
        static const uint64_t EDGE_EQ_SAMPLE_FRAG_MASK = 0x0000000001ffffffULL;
        static const uint32_t EDGE_EQ_COEF_INT_BITS    =  9;
        static const uint32_t EDGE_EQ_COEF_FRAC_BITS   = 25;

        static const uint32_t MINION_HARTS_PER_RBOX = 32;

        static const uint32_t tile_dimensions[6][2];

        uint32_t             rbox_id;

        ESRCfgT              cfg_esr;
        ESRInBufPgT          in_buf_pg_esr;
        ESRInBufCfgT         in_buf_cfg_esr;
        ESROutBufPgT         out_buf_pg_esr;
        ESROutBufCfgT        out_buf_cfg_esr;
        ESRStatusT           status_esr;
        ESRStartT            start_esr;
        ESRConsumeT          consume_esr;

        bool                 started;
        bool                 last_in_pckt;
        bool                 flush_drawcall;
        uint32_t             in_pckt_count;
        uint32_t             out_pckt_count;
        uint64_t             next_in_pckt_addr;
        uint64_t             base_out_buf_addr;
    

        uint32_t             hart_packet_credits[MINION_HARTS_PER_RBOX];
        uint32_t             hart_msg_credits[MINION_HARTS_PER_RBOX];
        uint32_t             hart_ptr[MINION_HARTS_PER_RBOX];
        bool                 fsh_state_sent[MINION_HARTS_PER_RBOX];
        bool                 end_of_phase_sent[MINION_HARTS_PER_RBOX];
        uint32_t             hart_sent_packets[MINION_HARTS_PER_RBOX];
        uint32_t             hart_sent_ptr[MINION_HARTS_PER_RBOX];
        bool                 send_message[MINION_HARTS_PER_RBOX];

        StateT               rbox_state;
        FragmentShaderStateT frag_shader_state;
        TriangleInfoT        current_triangle;
        bool                 new_frag_shader_state;

        std::vector<QuadInfoT>                     output_quads;
        std::vector<std::pair<uint32_t, uint64_t>> output_packets;

        uint32_t process_packet(uint64_t packet);
        void generate_tile(uint32_t tile_x, uint32_t tile_y, int64_t edge_samples[3], DepthT depth_sample, TileSizeT tile_sz);
        void sample_next_row(TriangleSampleT &sample);
        void sample_next_quad(TriangleSampleT &sample);
        void sample_quad(uint32_t x, uint32_t y, TriangleSampleT quad_sample, QuadInfoT &quad);
        bool test_quad(QuadInfoT &quad);
        bool sample_inside_triangle(TriangleSampleT sample);
        uint64_t compute_depth_stencil_buffer_address(uint32_t x, uint32_t y);
        bool do_depth_test(uint32_t frag_depth, uint32_t sample_depth);
        bool do_stencil_test(uint8_t frag_stencil);
        bool do_depth_bound_test(uint32_t frag_depth);
        bool do_scissor_test(int32_t x, int32_t y);
        uint8_t stencil_update(uint8_t frag_stencil, bool stencil_test, bool depth_test);
        float convert_edge_to_fp32(int64_t edge);
        float convert_depth_to_fp32(uint32_t depth);
        void tile_position_to_pixels(uint32_t &tile_x, uint32_t &tile_y, TileSizeT tile_size);
        uint32_t compute_target_minion_hart(uint32_t x, uint32_t y);
        uint64_t compute_minion_hart_out_addr(uint32_t target_minion_hart);
        uint64_t compute_minion_hart_out_off(uint32_t target_minion_hart);
        void update_minion_hart_out_ptr(uint32_t target_minion_hart);
        bool send_frag_shader_state_packet(uint32_t target_minion_hart, bool step_mode);
        bool send_end_of_phase_packet(uint32_t target_minion_hart, bool step_mode);
        bool send_quad_packet(bool step_mode);
        void send_packet(uint32_t minion_hart_id, uint64_t packet[4], uint64_t &out_addr, bool step_mode);
        bool report_packets(uint32_t minion_hart_id);
        void write_next_packet();

    public:

        void write_esr(uint32_t esr_id, uint64_t data);
        uint64_t read_esr(uint32_t esr_id);

        void reset(uint32_t id);
        void run(bool step_mode);
    
    }; // class RBOXEmu

} // namespace RBOX

#endif // _RBOX_H
