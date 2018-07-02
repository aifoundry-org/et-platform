#include <rbox.h>
#include <triangle_setup.h>
#include <tile_rast.h>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>

uint64_t get_abs_integral_fxp(int64_t v, uint32_t int_bits, uint32_t frac_bits)
{
    uint64_t v_abs = (v < 0) ? uint64_t(-v) : uint64_t(v);
    uint64_t v_int_abs = v_abs >> frac_bits;
    
    return v_int_abs;
}

int64_t get_integral_fxp(int64_t v, uint32_t int_bits, uint32_t frac_bits)
{
    uint64_t v_int_abs = get_abs_integral_fxp(v, int_bits, frac_bits);
    return (v < 0) ? -int64_t(v_int_abs) : int64_t(v_int_abs);
}

uint64_t get_fractional_fxp(int64_t v, uint32_t frac_bits, uint32_t digits)
{
    uint64_t v_abs = (v < 0) ? uint64_t(-v) : uint64_t(v);
    uint64_t v_abs_frac = v_abs & ((1 << frac_bits) - 1);
    uint64_t v_abs_dec_frac = 0;
    while ((v_abs_frac > 0) && (digits > 0))
    {
        v_abs_frac = v_abs_frac * 10;
        v_abs_dec_frac = v_abs_dec_frac * 10 + (v_abs_frac >> frac_bits);
        v_abs_frac = v_abs_frac & ((1ULL << frac_bits) -1);
        digits--;
    }
    return v_abs_dec_frac;
}

int64_t extend_sign(uint64_t v, uint32_t sign_bit)
{
    return int64_t(v) | (int64_t(0) - (1LL << sign_bit));
}

char *fxp_2_dec_str(char *str, int64_t v, uint32_t int_bits, uint32_t frac_bits, uint32_t frac_dec_digits)
{
    char format_str[32];
    
    if (v < 0)
        sprintf(format_str, "-%%lld.%%0%dlld", frac_dec_digits);
    else
        sprintf(format_str, "%%lld.%%0%dlld", frac_dec_digits);
   
    sprintf(str, format_str, get_abs_integral_fxp(v, int_bits, frac_bits), get_fractional_fxp(v, frac_bits, frac_dec_digits));

    return str;
}

float32 fxp_2_float32(int64_t v, uint32_t int_bits, uint32_t frac_bits)
{
    uint64_t v_abs = (v < 0) ? uint64_t(-v) : uint64_t(v);
    float32 v_abs_fp = float32(v_abs) / float32(1ULL << frac_bits);
    
    return (v < 0) ? -v_abs_fp : v_abs_fp;
}

char *fxp_2_hex_str(char *str, int64_t v, uint32_t int_bits, uint32_t frac_bits)
{
    char format_str[32];
    uint32_t frac_hex_digits = frac_bits >> 2 + (((frac_bits & 0x3) > 0) ? 1 : 0);

    sprintf(format_str, "%%llx.%%0%dllx", frac_hex_digits);
    
    sprintf(str, format_str, v >> frac_bits, v & ((1 << frac_bits) - 1));

    return str;
}

void setup_triangle(float32 vpos_x[3], float32 vpos_y[3], float32 vdepth[3], int64_t a[4], int64_t b[4], int64_t c[4])
{
    char str1[256], str2[256], str3[256], str4[256], str5[256], str6[256];

    int64_t vpos_x_fxp_16_8[3];
    int64_t vpos_y_fxp_16_8[3];
    int32_t vdepth_unorm24[3];

    for (uint32_t v = 0; v < 3; v++)
    {
        vpos_x_fxp_16_8[v] = int64_t(vpos_x[v] * 256.0);
        vpos_y_fxp_16_8[v] = int64_t(vpos_y[v] * 256.0);
        vdepth_unorm24[v] = int32_t(vdepth[v] * float32((1 << 24) - 1));

        printf("vertex %d -> x = %f [%s] (%s) y = %f [%s] (%s) z = %f (%08llx)\n", v,
            vpos_x[v],
            fxp_2_hex_str(str1, vpos_x_fxp_16_8[v], 16, 8),
            fxp_2_dec_str(str2, vpos_x_fxp_16_8[v], 16, 8, 6),
            vpos_y[v],
            fxp_2_hex_str(str3, vpos_y_fxp_16_8[v], 16, 8),
            fxp_2_dec_str(str4, vpos_y_fxp_16_8[v], 16, 8, 6),
            vdepth[v], vdepth_unorm24[v]);
    }

    int64_t c_fxp_33_16[3];
    c_fxp_33_16[0] = vpos_x_fxp_16_8[0] * vpos_y_fxp_16_8[1] - vpos_y_fxp_16_8[0] * vpos_x_fxp_16_8[1];
    c_fxp_33_16[1] = vpos_x_fxp_16_8[1] * vpos_y_fxp_16_8[2] - vpos_y_fxp_16_8[1] * vpos_x_fxp_16_8[2];
    c_fxp_33_16[2] = vpos_x_fxp_16_8[2] * vpos_y_fxp_16_8[0] - vpos_y_fxp_16_8[2] * vpos_x_fxp_16_8[0];

    int64_t tri_2x_area_fxp_35_16 = c_fxp_33_16[0] + c_fxp_33_16[1] + c_fxp_33_16[2];

    printf("tri 2x area = %f [%s] (%s)\n",
        fxp_2_float32(tri_2x_area_fxp_35_16, 35, 16),
        fxp_2_hex_str(str1, tri_2x_area_fxp_35_16, 35, 16),
        fxp_2_dec_str(str2, tri_2x_area_fxp_35_16, 35, 16, 6));

    int64_t a_fxp_17_8[3];
    int64_t b_fxp_17_8[3];

    a_fxp_17_8[0] = vpos_y_fxp_16_8[0] - vpos_y_fxp_16_8[1];
    a_fxp_17_8[1] = vpos_y_fxp_16_8[1] - vpos_y_fxp_16_8[2];
    a_fxp_17_8[2] = vpos_y_fxp_16_8[2] - vpos_y_fxp_16_8[0];

    b_fxp_17_8[0] = vpos_x_fxp_16_8[1] - vpos_x_fxp_16_8[0];
    b_fxp_17_8[1] = vpos_x_fxp_16_8[2] - vpos_x_fxp_16_8[1];
    b_fxp_17_8[2] = vpos_x_fxp_16_8[0] - vpos_x_fxp_16_8[2];

    int64_t an_fxp_9_24[3];
    int64_t bn_fxp_9_24[3];
    int64_t cn_fxp_26_24[3];

    for (uint32_t e = 0; e < 3; e++)
    {
        int64_t recip_tri_2x_area_fxp_16_35 = ((1LL << 51) / tri_2x_area_fxp_35_16);

        printf("recip 2x area = %f [%s] (%s)\n", 
            fxp_2_float32(recip_tri_2x_area_fxp_16_35, 16, 35),
            fxp_2_hex_str(str1, recip_tri_2x_area_fxp_16_35, 16, 35),
            fxp_2_dec_str(str2, recip_tri_2x_area_fxp_16_35, 16, 35, 6));

        an_fxp_9_24[e] = (a_fxp_17_8[e] * recip_tri_2x_area_fxp_16_35) >> 19;
        bn_fxp_9_24[e] = (b_fxp_17_8[e] * recip_tri_2x_area_fxp_16_35) >> 19;
        cn_fxp_26_24[e] = (c_fxp_33_16[e] * recip_tri_2x_area_fxp_16_35) >> 27;

        printf("edge eq %d => a = %f [%s] (%s) b = %f [%s] (%s) c = %f [%s] (%s)\n", e,
                fxp_2_float32(a_fxp_17_8[e], 17, 8),
                fxp_2_hex_str(str1, a_fxp_17_8[e], 17, 8),
                fxp_2_dec_str(str2, a_fxp_17_8[e], 17, 8, 6),
                fxp_2_float32(b_fxp_17_8[e], 17, 8),
                fxp_2_hex_str(str3, b_fxp_17_8[e], 17, 8),
                fxp_2_dec_str(str4, b_fxp_17_8[e], 17, 8, 6),
                fxp_2_float32(c_fxp_33_16[e], 33, 16),
                fxp_2_hex_str(str5, c_fxp_33_16[e], 33, 16),
                fxp_2_dec_str(str6, c_fxp_33_16[e], 33, 16, 6));
        printf("norm edge eq %d => a = %f [%s] (%s) b = %f [%s] (%s) c = %f [%s] (%s)\n", e,
                fxp_2_float32(an_fxp_9_24[e], 9, 24),
                fxp_2_hex_str(str1, an_fxp_9_24[e], 9, 24),
                fxp_2_dec_str(str2, an_fxp_9_24[e], 9, 24, 6),
                fxp_2_float32(bn_fxp_9_24[e], 9, 24),
                fxp_2_hex_str(str3, bn_fxp_9_24[e], 9, 24),
                fxp_2_dec_str(str4, bn_fxp_9_24[e], 9, 24, 6),
                fxp_2_float32(cn_fxp_26_24[e], 24, 24),
                fxp_2_hex_str(str5, cn_fxp_26_24[e], 26, 24),
                fxp_2_dec_str(str6, cn_fxp_26_24[e], 26, 24, 6));

        a[e] = an_fxp_9_24[e];
        b[e] = bn_fxp_9_24[e];
        c[e] = cn_fxp_26_24[e];
    }

    int64_t a_depth, b_depth, c_depth;

    //a_depth = vdepth_unorm24[0] * an_fxp_9_24[0]  + vdepth_unorm24[1] * an_fxp_9_24[1]  + vdepth_unorm24[2] * an_fxp_9_24[2];
    //b_depth = vdepth_unorm24[0] * bn_fxp_9_24[0]  + vdepth_unorm24[1] * bn_fxp_9_24[1]  + vdepth_unorm24[2] * bn_fxp_9_24[2];
    //c_depth = vdepth_unorm24[0] * cn_fxp_26_24[0] + vdepth_unorm24[1] * cn_fxp_26_24[1] + vdepth_unorm24[2] * cn_fxp_26_24[2];
    
    a_depth = (vdepth_unorm24[0] - vdepth_unorm24[2]) * an_fxp_9_24[1] 
            + (vdepth_unorm24[1] - vdepth_unorm24[2]) * an_fxp_9_24[2];
    b_depth = (vdepth_unorm24[0] - vdepth_unorm24[2]) * bn_fxp_9_24[1] 
            + (vdepth_unorm24[1] - vdepth_unorm24[2]) * bn_fxp_9_24[2];
    c_depth = (vdepth_unorm24[0] - vdepth_unorm24[2]) * cn_fxp_26_24[1] 
            + (vdepth_unorm24[1] - vdepth_unorm24[2]) * cn_fxp_26_24[2]
            + (int64_t(vdepth_unorm24[2]) << 24);

    printf("depth eq => a = %f [%s] (%s) b = %f [%s] (%s) c = %f [%s] (%s)\n",
        fxp_2_float32(a_depth, 10, 48), fxp_2_hex_str(str1, a_depth, 10, 48), fxp_2_dec_str(str2, a_depth, 10, 48, 6),
        fxp_2_float32(b_depth, 10, 48), fxp_2_hex_str(str3, b_depth, 10, 48), fxp_2_dec_str(str4, b_depth, 10, 48, 6),
        fxp_2_float32(c_depth, 16, 48), fxp_2_hex_str(str5, c_depth, 16, 48), fxp_2_dec_str(str6, b_depth, 16, 48, 6));

    a_depth = (a_depth >> 24);
    b_depth = (b_depth >> 24);
    c_depth = (c_depth >> 24);

    printf("depth eq => a = %f [%x] b = %f [%x] c = %f [%x]\n",
        float32(a_depth) / float32((1 << 24) - 1), a_depth,
        float32(b_depth) / float32((1 << 24) - 1), b_depth,
        float32(c_depth) / float32((1 << 24) - 1), c_depth);

    a[3] = a_depth;
    b[3] = b_depth;
    c[3] = c_depth;
}

void flush_on_seg_signal(int signal, siginfo_t *si, void *arg)
{
   printf("ERROR!!! Segmentation fault detected\n");
   fflush(stdout);
   exit(-1);
}

int main()
{
    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(struct sigaction));
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_sigaction = flush_on_seg_signal;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &signal_action, NULL);

    print_debug = 1;

    printf("RBOX Basic Tester\n");

    uint32_t depth_stencil_buffer[1024 * 1024];

    uint32_t in_buffer_ptr = 0;

    for (uint32_t y = 0; y < 1024; y++)
        for (uint32_t x = 0; x < 1024; x++)
            depth_stencil_buffer[y * 1024 + x] = 0xFFFFFF;

    uint64_t in_buffer[1024];
    uint64_t *out_buffer = new uint64_t[1024 * 1024];

    set_rbox((uint64_t) in_buffer, 1024, (uint64_t) out_buffer, 1024 * 1024);

    reset_input_stream();
    reset_output_stream();

    RBOXInPcktRBOXState          rbox_state_pckt;
    RBOXInPcktFrgmtShdrState     frg_shdr_state_pckt;
    RBOXInPcktTriWithTile64x64   tri_with_tile_64x64_pckt;
    RBOXInPcktTriWithTile128x128 tri_with_tile_128x128_pckt;
    RBOXInPcktLargeTri           large_tri_pckt;
    RBOXInPcktLargeTriTile       large_tri_tile_pckt;
    RBOXInPcktFullyCoveredTile   fully_cov_tile_pckt;

    printf("Size of RBOXInPcktRBOXState = %d\n",          sizeof(RBOXInPcktRBOXState));
    printf("Size of RBOXInPcktFrgmtShdrState = %d\n",     sizeof(RBOXInPcktFrgmtShdrState));
    printf("Size of RBOXInPcktTriWithTile64x64 = %d\n",   sizeof(RBOXInPcktTriWithTile64x64));
    printf("Size of RBOXInPcktTriWithTile128x128 = %d\n", sizeof(RBOXInPcktTriWithTile128x128));
    printf("Size of RBOXInPcktLargeTri = %d\n",           sizeof(RBOXInPcktLargeTri));
    printf("Size of RBOXInPcktLargeTriTile = %d\n",       sizeof(RBOXInPcktLargeTriTile));
    printf("Size of RBOXInPcktFullyCoveredTile = %d\n",   sizeof(RBOXInPcktFullyCoveredTile));

    for(uint32_t qw = 0; qw < 8; qw++)
        rbox_state_pckt.qw[qw] = 0;

    rbox_state_pckt.state.type = RBOX_INPCKT_RBOX_STATE;
    rbox_state_pckt.state.msaa_enable = 0;
    rbox_state_pckt.state.depth_stencil_buffer_ptr = (uint64_t) depth_stencil_buffer;
    rbox_state_pckt.state.depth_stencil_buffer_format = FORMAT_D24_UNORM_S8_UINT;
    rbox_state_pckt.state.depth_stencil_buffer_tile_mode = 1;
    rbox_state_pckt.state.depth_stencil_buffer_row_pitch = 256;
    rbox_state_pckt.state.depth_clamp_enable = 1;
    rbox_state_pckt.state.depth_bound_enable = 0;
    rbox_state_pckt.state.depth_test_enable = 1;
    rbox_state_pckt.state.depth_test_write_enable = 1;
    rbox_state_pckt.state.depth_test_compare_op = RBOX_COMPARE_OP_LESS;
    rbox_state_pckt.state.early_frag_tests_enable = 1;
    rbox_state_pckt.state.stencil_test_enable = 0;
    rbox_state_pckt.state.fragment_shader_disabled = 0;
    rbox_state_pckt.state.depth_min = 0;
    rbox_state_pckt.state.depth_max = 0xFFFFFF;
    rbox_state_pckt.state.scissor_start_x = 0;
    rbox_state_pckt.state.scissor_start_y = 0;
    rbox_state_pckt.state.scissor_height = 1024;
    rbox_state_pckt.state.scissor_width  = 1024;

    for (uint32_t qw = 0; qw < 8; qw++)
        in_buffer[in_buffer_ptr++] = rbox_state_pckt.qw[qw];

    push_packet();

    for (uint32_t qw = 0; qw < 4; qw++)
        frg_shdr_state_pckt.qw[qw] = 0;

    frg_shdr_state_pckt.state.type = RBOX_INPCKT_FRAG_SHADING_STATE;
    frg_shdr_state_pckt.state.frag_shader_function_ptr = 0xA0000000;
    frg_shdr_state_pckt.state.frag_shader_state_ptr = 0xB0000000;

    for (uint32_t qw = 0; qw < 4; qw++)
        in_buffer[in_buffer_ptr++] = frg_shdr_state_pckt.qw[qw];

    push_packet();

    float32 vpos_x[3];
    float32 vpos_y[3];
    float32 vdepth[3];
    
    vpos_x[0] =  0.30; vpos_y[0] =  1.00; vdepth[0] = 0.1;
    vpos_x[1] = 20.70; vpos_y[1] =  5.09; vdepth[1] = 0.5;
    vpos_x[2] = 14.65; vpos_y[2] = 30.23; vdepth[2] = 0.3;

    int64_t a[4], b[4], c[4];

    setup_triangle(vpos_x, vpos_y, vdepth, a, b, c);

    for (uint32_t qw = 0; qw < 8; qw++)
        tri_with_tile_64x64_pckt.qw[qw] = 0;

    tri_with_tile_64x64_pckt.tri_with_tile_64x64.type = RBOX_INPCKT_TRIANGLE_WITH_TILE_64x64;
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_left = 0;
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_top = 0;
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.tri_facing = RBOX_TRI_FACING_FRONT;
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.tile_size = RBOX_TILE_SIZE_64x64;

    for (uint32_t eq = 0; eq < 3; eq++)
    {
        printf("Edge eq %d -> a = %llx b = %llx c = %llx\n", eq, a[eq], b[eq], c[eq]);
        tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge_eqs[eq].a = (uint64_t) (a[eq] >> 11);
        tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge_eqs[eq].b = (uint64_t) (b[eq] >> 11);
        tri_with_tile_64x64_pckt.tri_with_tile_64x64.edge[eq].e     = (uint64_t) (c[eq] >> 11);
    }
    printf("Depth eq => a = %llx b = %llx c = %llx\n", a[3], b[3], c[3]);
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth_eq.a = uint32_t(a[3]);
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth_eq.b = uint32_t(b[3]);
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.depth      = uint32_t(c[3]);
    tri_with_tile_64x64_pckt.tri_with_tile_64x64.triangle_data_ptr = 0;

    for (uint32_t qw = 0; qw < 8; qw++)
        in_buffer[in_buffer_ptr++] = tri_with_tile_64x64_pckt.qw[qw];

    push_packet();

    process();

    RBOXOutPcktFrgShdrState out_frg_shdr_state_pckt;
    RBOXOutPcktQuadInfo     out_quad_info_pckt;
    RBOXOutPcktQuadData     out_quad_data_pckt;

    printf("Size of RBOXOutPcktFrgShdrState = %d\n", sizeof(RBOXOutPcktFrgShdrState));
    printf("Size of RBOXOutPcktQuadInfo = %d\n",     sizeof(RBOXOutPcktQuadInfo));
    printf("Size of RBOXOutPcktQuadData = %d\n",     sizeof(RBOXOutPcktQuadData));

    uint32_t out_buffer_ptr = 0;
    uint32_t out_packets = 0;

    while(consume_packet())
    {
        RBOXOutPckt128b rbox_out_packet;

        rbox_out_packet.qw[0] = out_buffer[out_buffer_ptr];

        switch(rbox_out_packet.header.type)
        {
            case RBOX_OUTPCKT_STATE_INFO    :
                printf("Reading RBOX Output Packet %d with Fragment Shader Information from %016llx\n", out_packets, &out_buffer[out_buffer_ptr]);

                for (uint32_t qw = 0; qw < 4; qw++)
                    out_frg_shdr_state_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                out_packets++;

                break;
            case RBOX_OUTPCKT_QUAD_INFO     :
                printf("Reading RBOX Output Packet %d with Quad Information from %016llx\n", out_packets, &out_buffer[out_buffer_ptr]);

                for (uint32_t qw = 0; qw < 2; qw++)
                    out_quad_info_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                for (uint32_t qw = 0; qw < 2; qw++)
                    out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                for (uint32_t qw = 0; qw < 2; qw++)
                    out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                for (uint32_t qw = 0; qw < 2; qw++)
                    out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                out_packets++;

                break;
            default :
                printf("Error processing RBOX Output Packet!!!\n");
                break;
        }

        printf("Total RBOX output packets : %d\n", out_packets);
    }

    printf("Test Triangle Setup code\n");  

    uint32_t vposx[] = {0,         0,  64 * 256, 64 * 256, 318 * 256, 318 * 256};
    uint32_t vposy[] = {0, 128 * 256, 128 * 256,        0,         0, 128 * 256};

    printf("@vpox %p @vposy %p\n", vposx, vposy);

    TriangleVector drawcall;
    drawcall.vertex_array.x = vposx;
    drawcall.vertex_array.y = vposy;
    drawcall.v0_indices[0] = 0 * 4;
    drawcall.v0_indices[1] = 0 * 4;
    drawcall.v0_indices[2] = 3 * 4;
    drawcall.v0_indices[3] = 3 * 4;
    drawcall.v1_indices[0] = 2 * 4;
    drawcall.v1_indices[1] = 3 * 4;
    drawcall.v1_indices[2] = 5 * 4;
    drawcall.v1_indices[3] = 4 * 4;
    drawcall.v2_indices[0] = 1 * 4;
    drawcall.v2_indices[1] = 2 * 4;
    drawcall.v2_indices[2] = 2 * 4;
    drawcall.v2_indices[3] = 5 * 4;
    drawcall.mask = 0x55;

    TriangleSetupVector setup_output;
    setup_output.high_precision_mask = 0;
    setup_output.low_precision_mask = 0;

    vector_triangle_setup_ccw_front(&drawcall, &setup_output);

    printf("high precision mask = %x  low precision mask = %x\n",
        setup_output.high_precision_mask,
        setup_output.low_precision_mask);

    uint64_t rbox_input_buffer[1024];

    uint32_t rbox_packets = 0;

    set_rbox((uint64_t) rbox_input_buffer, 1024, (uint64_t) out_buffer, 1024 * 1024);

    reset_input_stream();
    reset_output_stream();

    in_buffer_ptr = 0;

    for(uint32_t qw = 0; qw < 8; qw++)
        rbox_state_pckt.qw[qw] = 0;

    rbox_state_pckt.state.type = RBOX_INPCKT_RBOX_STATE;
    rbox_state_pckt.state.msaa_enable = 0;
    rbox_state_pckt.state.depth_stencil_buffer_ptr = (uint64_t) depth_stencil_buffer;
    rbox_state_pckt.state.depth_stencil_buffer_format = FORMAT_D24_UNORM_S8_UINT;
    rbox_state_pckt.state.depth_stencil_buffer_tile_mode = 1;
    rbox_state_pckt.state.depth_stencil_buffer_row_pitch = 256;
    rbox_state_pckt.state.depth_clamp_enable = 1;
    rbox_state_pckt.state.depth_bound_enable = 0;
    rbox_state_pckt.state.depth_test_enable = 0;
    rbox_state_pckt.state.depth_test_write_enable = 1;
    rbox_state_pckt.state.depth_test_compare_op = RBOX_COMPARE_OP_LESS;
    rbox_state_pckt.state.early_frag_tests_enable = 1;
    rbox_state_pckt.state.stencil_test_enable = 0;
    rbox_state_pckt.state.fragment_shader_disabled = 0;
    rbox_state_pckt.state.depth_min = 0;
    rbox_state_pckt.state.depth_max = 0xFFFFFF;
    rbox_state_pckt.state.scissor_start_x = 0;
    rbox_state_pckt.state.scissor_start_y = 0;
    rbox_state_pckt.state.scissor_height = 1024;
    rbox_state_pckt.state.scissor_width  = 1024;

    for (uint32_t qw = 0; qw < 8; qw++)
        rbox_input_buffer[in_buffer_ptr++] = rbox_state_pckt.qw[qw];

    push_packet();

    for (uint32_t qw = 0; qw < 4; qw++)
        frg_shdr_state_pckt.qw[qw] = 0;

    frg_shdr_state_pckt.state.type = RBOX_INPCKT_FRAG_SHADING_STATE;
    frg_shdr_state_pckt.state.frag_shader_function_ptr = 0xA0000000;
    frg_shdr_state_pckt.state.frag_shader_state_ptr = 0xB0000000;

    for (uint32_t qw = 0; qw < 4; qw++)
        rbox_input_buffer[in_buffer_ptr++] = frg_shdr_state_pckt.qw[qw];

    push_packet();

    create_rbox_packets(&setup_output, (uint8_t *) &rbox_input_buffer[in_buffer_ptr], &rbox_packets, TILE_SIZE_32x32);

    printf("Number of RBOX packets generated : %d\n", rbox_packets);

    for(uint32_t p = 0; p < rbox_packets; p++)
        push_packet();

    process();

    out_buffer_ptr = 0;
    out_packets = 0;

    uint8_t framebuffer[1024 * 1024];

    for (uint32_t b = 0; b < (1024 * 1024); b++)
        framebuffer[b] = 0;

    uint32_t tile_x;
    uint32_t tile_y;

    while(consume_packet())
    {
        RBOXOutPckt128b rbox_out_packet;

        rbox_out_packet.qw[0] = out_buffer[out_buffer_ptr];

        switch(rbox_out_packet.header.type)
        {
            case RBOX_OUTPCKT_STATE_INFO :
                printf("Reading RBOX Output Packet %d with Fragment Shader Information from %016llx\n", out_packets, &out_buffer[out_buffer_ptr]);

                for (uint32_t qw = 0; qw < 4; qw++)
                    out_frg_shdr_state_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                out_packets++;

                break;
            case RBOX_OUTPCKT_QUAD_INFO :
                {
                    printf("Reading RBOX Output Packet %d with Quad Information from %016llx\n", out_packets, &out_buffer[out_buffer_ptr]);

                    for (uint32_t qw = 0; qw < 2; qw++)
                        out_quad_info_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                    out_packets++;

                    uint32_t x = out_quad_info_pckt.quad_info.x << 1;
                    uint32_t y = out_quad_info_pckt.quad_info.y << 1;

                    printf("Quad at (%d, %d) mask = 0x%08x\n", x, y, out_quad_info_pckt.quad_info.mask);

                    if (out_quad_info_pckt.quad_info.mask & 0x01)
                        framebuffer[y * 1024 + x]++;
                    if (out_quad_info_pckt.quad_info.mask & 0x04)
                        framebuffer[y * 1024 + x + 1]++;
                    if (out_quad_info_pckt.quad_info.mask & 0x10)
                        framebuffer[(y + 1) * 1024 + x]++;
                    if (out_quad_info_pckt.quad_info.mask & 0x40)
                        framebuffer[(y + 1) * 1024 + x + 1]++;

                    for (uint32_t qw = 0; qw < 2; qw++)
                        out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                    for (uint32_t qw = 0; qw < 2; qw++)
                        out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];

                    for (uint32_t qw = 0; qw < 2; qw++)
                        out_quad_data_pckt.qw[qw] = out_buffer[out_buffer_ptr++];
                }

                break;
            default :
                printf("Error processing RBOX Output Packet!!!\n");
                break;
        }

        printf("Total RBOX output packets : %d\n", out_packets);
        
    }

    for(uint32_t y = 0; y < 1024; y++)
    {
        for(uint32_t x = 0; x < 1024; x++)
            printf("%d", framebuffer[y * 1024 + x]);
        printf("\n");
    }
}


