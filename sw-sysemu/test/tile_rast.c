#include <tile_rast.h>
#include <rbox.h>
#include <cstdlib>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

uint32 rast_lit_tile_size_64x64[] =
{
              0,    // +0
             64,    // +4
            128,    // +8
              0,    // +12
     0x80000000,    // +16
              0,
     0x80000000,
              0
};

uint32 rast_lit_tile_size_32x32[] =
{
              0,    // +0
             32,    // +4
             64,    // +8
             96,    // +12
     0x80000000,    // +16
              0,
     0x80000000,
              0
};

void generate_base_triangle_with_tile_128x128_packet()
{
    // 
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x11 -> rbox_input_buffer
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ei
    //
    // x21, x22 -> 64x64 tile position (tiles)
    //

    print_comment(">>> generate_base_triangle_with_tile_128x128_packet");

    sw(x1,   8, x11, "Store a0 in RBOX packet");
    sw(x4,  12, x11, "Store b0 in RBOX packet");
    sw(x2,  16, x11, "Store a1 in RBOX packet");
    sw(x5,  20, x11, "Store b1 in RBOX packet");
    sw(x3,  24, x11, "Store a2 in RBOX packet");
    sw(x6,  28, x11, "Store b2 in RBOX packet");
    sd(x0,  32, x11, "Store ad, bd in RBOX packet");
    sd(x0,  40, x11, "Store pointer to triangle data RBOX packet");

    flw_ps(f29,  0, x11, "Load pre-packed triangle information, first 128b block");
    flw_ps(f30, 16, x11, "Load pre-packed triangle information, second 128b block");
    flw_ps(f31, 32, x11, "Load pre-packed triangle information, third 128b block");

    print_comment("<<< generate_base_triangle_with_tile_128x128_packet");
}

void generate_triangle_with_tile_128x128_packet()
{
    // 
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets counter
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x17 - x18 -> ei
    //
    // x21, x22 -> 64x64 tile position (tiles)
    //
    // x24 is modified
    // 

    print_comment(">>> generate_triangle_with_tile_128x128_packet");

    fsw_ps(f29,  0, x11, "Store pre-packed triangle information, first 128b block");
    fsw_ps(f30, 16, x11, "Store pre-packed triangle information, second 128b block");
    fsw_ps(f31, 32, x11, "Store pre-packed triangle information, third 128b block");

    // Encode RBOX packet (each tile should go to a different Shire)
    slli(x24, x15, 13, "Pack RBOX, packet header, tile size and tile facing");
    or_(x24, x22, x24, "Pack RBOX packet header, tile y"); 
    slli(x24, x24, 13, "Pack RBOX packet header, tile y");
    or_(x24, x21, x24, "Pack RBOX packet header, tile x");
    slli(x24, x24, 3, "Pack RBOX packet header");
    ori(x24, x24, 1, "Pack RBOX packet header, packet type -> RBOX_INPCKT_TRIANGLE_WITH_TILE_128x128");
    sw(x24,  0, x11, "Store header in RBOX packet");

    sw(x16, 48, x11, "Store c0 in RBOX packet");
    sw(x17, 52, x11, "Store c1 in RBOX packet");
    sw(x18, 56, x11, "Store c2 in RBOX packet");
    sw(x0,  60, x11, "Store cd in RBOX packet");

    addi(x11, x11, 64, "Update pointer to RBOX input buffer");
    addi(x13, x13, 1, "Update RBOX packets counter");

    print_comment("<<< generate_triangle_with_tile_128x128_packet");
}


#define EVALUATE_TILE_EQ(MSMP_BOTTOM, MSMP_TOP, XTMP, MASK, LABEL, EQ) \
    maskpopc_rast(XTMP, MSMP_BOTTOM, MSMP_TOP, MASK, "Count number of points passing for (e" #EQ " < 0)"); \
    if (XREGS[XTMP].x == 8) \
        goto LABEL;

#define EVALUATE_TILE(MSMP_BOTTOM1, MSMP_BOTTOM2, MSMP_BOTTOM3, \
                      MSMP_TOP1,    MSMP_TOP2,    MSMP_TOP3, \
                      XTMP, MASK, LABEL) \
    EVALUATE_TILE_EQ(MSMP_BOTTOM1, MSMP_TOP1, XTMP, MASK, LABEL, 0) \
    EVALUATE_TILE_EQ(MSMP_BOTTOM2, MSMP_TOP2, XTMP, MASK, LABEL, 1) \
    EVALUATE_TILE_EQ(MSMP_BOTTOM3, MSMP_TOP3, XTMP, MASK, LABEL, 2)


void rasterize_128x64_to_64x64()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  128x64
    //
    //  3--5--6
    //  |  |  |
    //  0--1--2
    //

    print_comment(">>> rasterize_128x64");

    flw_ps(f9, 0, x23, "Load offsets (0, 64, 128, x)");
    fmul_pi(f0, f0, f9, "a0 * (0, 64, 128, x)");
    fmul_pi(f1, f1, f9, "a1 * (0, 64, 128, x)");
    fmul_pi(f2, f2, f9, "a2 * (0, 64, 128, x)");
    fadd_pi(f6, f6, f0, "Sample e0 at points (0, 1, 2, x)");
    fadd_pi(f7, f7, f1, "Sample e1 at points (0, 1, 2, x)");
    fadd_pi(f8, f8, f2, "Sample e2 at points (0, 1, 2, x)");

    fbci_pi(f9, 64, "Set offset 64");
    fmul_pi(f3, f3, f9, "b0 * 64");
    fmul_pi(f4, f4, f9, "b1 * 64");
    fmul_pi(f5, f5, f9, "b2 * 64");
    fadd_pi(f10, f6, f3, "Sample e0 at points (3, 4, 5, x)");
    fadd_pi(f11, f7, f4, "Sample e1 at points (3, 4, 5, x)");
    fadd_pi(f12, f8, f5, "Sample e2 at points (3, 4, 5, x)");
    
    fbci_pi(f13, 0, "Load 0");

    fltm_pi(m1, f6, f13, "Test (e0 < 0) for points (0, 1, 2, x)");
    fltm_pi(m2, f7, f13, "Test (e1 < 0) for points (0, 1, 2, x)");
    fltm_pi(m3, f8, f13, "Test (e2 < 0) for points (0, 1, 2, x)");

    fltm_pi(m4, f10, f13, "Test (e0 < 0) for points (3, 4, 5, x)");
    fltm_pi(m5, f11, f13, "Test (e1 < 0) for points (3, 4, 5, x)");
    fltm_pi(m6, f12, f13, "Test (e2 < 0) for points (3, 4, 5, x)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_128x64)

    generate_triangle_with_tile_128x128_packet();

tile1_128x64:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, end_128x64)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

end_128x64:

    print_comment("<<< rasterize_128x64");

    return;
}

void rasterize_64x128_to_64x64()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  64x128
    //
    //  2--5
    //  |  |
    //  1--4
    //  |  |
    //  0--3
    //

    print_comment(">>> rasterize_64x128");

    flw_ps(f9, 0, x23, "Load offsets (0, 64, 128, x)");
    fmul_pi(f3, f3, f9, "b0 * (0, 64, 128, x)");
    fmul_pi(f4, f4, f9, "b1 * (0, 64, 128, x)");
    fmul_pi(f5, f5, f9, "b2 * (0, 64, 128, x)");
    fadd_pi(f6, f6, f3, "Sample e0 at points (0, 1, 2, x)");
    fadd_pi(f7, f7, f4, "Sample e1 at points (0, 1, 2, x)");
    fadd_pi(f8, f8, f5, "Sample e2 at points (0, 1, 2, x)");

    fbci_pi(f9, 64, "Set offset 64");
    fmul_pi(f0, f0, f9, "a0 * 64");
    fmul_pi(f1, f1, f9, "a1 * 64");
    fmul_pi(f2, f2, f9, "a2 * 64");
    fadd_pi(f10, f6, f0, "Sample e0 at points (3, 4, 5, x)");
    fadd_pi(f11, f7, f1, "Sample e1 at points (3, 4, 5, x)");
    fadd_pi(f12, f8, f2, "Sample e2 at points (3, 4, 5, x)");
    
    fbci_pi(f13, 0, "Load 0");
    fltm_pi(m1, f6, f13, "Test (e0 < 0) for points (0, 1, 2, x)");
    fltm_pi(m2, f7, f13, "Test (e1 < 0) for points (0, 1, 2, x)");
    fltm_pi(m3, f8, f13, "Test (e2 < 0) for points (0, 1, 2, x)");

    fltm_pi(m4, f10, f13, "Test (e0 < 0) for points (3, 4, 5, x)");
    fltm_pi(m5, f11, f13, "Test (e1 < 0) for points (3, 4, 5, x)");
    fltm_pi(m6, f12, f13, "Test (e2 < 0) for points (3, 4, 5, x)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_64x128)

    generate_triangle_with_tile_128x128_packet();

tile1_64x128:

    addi(x22, x22, 1, "Advance tile y coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, end_64x128)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

end_64x128:

    print_comment("<<< rasterize_64x128");

    return;
}

void rasterize_128x128_to_64x64()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  128x128
    //
    //  6--7--8
    //  |  |  |
    //  3--4--5
    //  |  |  |
    //  0--1--2
    //

    //
    // ei (0, 1, 2, x) -> f0 - f2
    // ei (3, 4, 5, x) -> f3 - f5
    // ei (6, 7, 8, x) -> f6 - f8
    //
    // test (0, 1, 2, x) -> f9 - f11
    // test (3, 4, 5, x) -> f12 - f14
    // test (6, 7, 8, x) -> f15 - f17
    //

    print_comment(">>> rasterize_128x128");

    flw_ps(f20, 0, x23, "Load offsets (0, 64, 128, x)");
    fmul_pi(f21, f0, f20, "a0 * (0, 64, 128, x)");
    fmul_pi(f22, f1, f20, "a1 * (0, 64, 128, x)");
    fmul_pi(f23, f2, f20, "a2 * (0, 64, 128, x)");

    fbci_pi(f20, 64, "Set offset 64");
    fmul_pi(f18, f0, f20, "a0 * 64");
    fmul_pi(f19, f1, f20, "a1 * 64");
    fmul_pi(f20, f2, f20, "a2 * 64");

    fadd_pi(f0, f6, f21, "Sample e0 at points (0, 1, 2, x)");
    fadd_pi(f1, f7, f22, "Sample e1 at points (0, 1, 2, x)");
    fadd_pi(f2, f8, f23, "Sample e2 at points (0, 1, 2, x)");

    fbci_pi(f24, 64, "Set offset 64");
    fmul_pi(f21, f3, f20, "b0 * 64");
    fmul_pi(f22, f4, f20, "b1 * 64");
    fmul_pi(f23, f5, f20, "b2 * 64");

    fbci_pi(f24, 0, "Load 0");

    addi(x25, x21, 0, "Save tile x position");
    addi(x26, x0, 0, "Initilize tile row to 0");

    fadd_pi(f3, f0, f21, "Sample e0 at points (3, 4, 5, x)");
    fadd_pi(f4, f1, f22, "Sample e1 at points (3, 4, 5, x)");
    fadd_pi(f5, f2, f23, "Sample e2 at points (3, 4, 5, x)");
    
    fadd_pi(f6, f3, f21, "Sample e0 at points (6, 7, 8, x)");
    fadd_pi(f7, f4, f22, "Sample e1 at points (6, 7, 8, x)");
    fadd_pi(f8, f5, f23, "Sample e2 at points (6, 7, 8, x)");

    fltm_pi(m1, f0, f24, "Test (e0 < 0) for points (0, 1, 2, x)");
    fltm_pi(m2, f1, f24, "Test (e1 < 0) for points (0, 1, 2, x)");
    fltm_pi(m3, f2, f24, "Test (e2 < 0) for points (0, 1, 2, x)");

    fltm_pi(m4, f3, f24, "Test (e0 < 0) for points (3, 4, 5, x)");
    fltm_pi(m5, f4, f24, "Test (e1 < 0) for points (3, 4, 5, x)");
    fltm_pi(m6, f5, f24, "Test (e2 < 0) for points (3, 4, 5, x)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_128x128)

    generate_triangle_with_tile_128x128_packet();

tile1_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_128x128)

    // Get second tile initial sample position from point 1
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_128x128:

    addi(x26, x26, 1, "Update tile row");
    addi(x21, x25, 0, "Restore tile x for next row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    fltm_pi(m1, f6, f24, "Test (e0 < 0) for points (6, 7, 8, x)");
    fltm_pi(m2, f7, f24, "Test (e1 < 0) for points (6, 7, 8, x)");
    fltm_pi(m3, f8, f24, "Test (e2 < 0) for points (6, 7, 8, x)");
 
    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 0, tile3_128x128)

    // Get third tile initial sample position from point 3
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 3");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 3");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 3");
    generate_triangle_with_tile_128x128_packet();

tile3_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 1, end_128x128)

    // Get fourth tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 1, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 1, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 1, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

end_128x128:

    print_comment("<<< rasterize_128x128");

    return;
}

void rasterize_Nx32_to_32x32()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  [64|96|128]x32
    //
    //  4--5--6--7--9
    //  |  |  |  |  |
    //  0--1--2--3--8
    //

    print_comment(">>> rasterize_Nx32");

    flw_ps(f9, 0, x23, "Load offsets (0, 32, 64, 96)");
    fmul_pi(f0, f0, f9, "a0 * (0, 32, 64, 96)");
    fmul_pi(f1, f1, f9, "a1 * (0, 32, 64, 96)");
    fmul_pi(f2, f2, f9, "a2 * (0, 32, 64, 96)");
    fadd_pi(f6, f6, f0, "Sample e0 at points (0, 1, 2, 3)");
    fadd_pi(f7, f7, f1, "Sample e1 at points (0, 1, 2, 3)");
    fadd_pi(f8, f8, f2, "Sample e2 at points (0, 1, 2, 3)");

    fbci_pi(f9, 32, "Set offset 32");
    fmul_pi(f3, f3, f9, "b0 * 32");
    fmul_pi(f4, f4, f9, "b1 * 32");
    fmul_pi(f5, f5, f9, "b2 * 32");
    fadd_pi(f10, f6, f3, "Sample e0 at points (4, 5, 6, 7)");
    fadd_pi(f11, f7, f4, "Sample e1 at points (4, 5, 6, 7)");
    fadd_pi(f12, f8, f5, "Sample e2 at points (4, 5, 6, 7)");
    
    fbci_pi(f13, 0, "Load 0");

    fltm_pi(m1, f6, f13, "Test (e0 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m2, f7, f13, "Test (e1 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m3, f8, f13, "Test (e2 < 0) for points (0, 1, 2, 3)");

    fltm_pi(m4, f10, f13, "Test (e0 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m5, f11, f13, "Test (e1 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m6, f12, f13, "Test (e2 < 0) for points (4, 5, 6, 7)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_Nx32)

    generate_triangle_with_tile_128x128_packet();

tile1_Nx32:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_Nx32)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_Nx32:

    // Check if horizontal dimension is larger than 64 and there are 3 tiles
    if (XREGS[x19].x <= 64)
        goto end_Nx32;

    addi(x21, x21, 1, "Advance tile x coordinate one position");
 
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile3_Nx32)

    // Get third tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f7, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f8, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_Nx32:

    // Check if horizontal dimension is larger than 96 and there are 4 tiles
    if (XREGS[x19].x <= 96)
        goto end_Nx32;

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    mov_m_x(m0, x0, 0x05, "Set mask to (1, 1, 0, 0)");
    fswizz_ps(f13, f6, 0x0f, "Copy initial value for e0 for fourth tile (3, 3, x, x)");
    fswizz_ps(f14, f7, 0x0f, "Copy initial value for e1 for fourth tile (3, 3, x, x)");
    fswizz_ps(f15, f8, 0x0f, "Copy initial value for e2 for fourth tile (3, 3, x, x)");
    mov_m_x(m0, x0, 0x50, "Set mask to (0, 0, 1, 1)");
    fswizz_ps(f13, f10, 0xf0, "Copy initial value for e0 for fourth tile (3, 3, 7, 7)");
    fswizz_ps(f14, f11, 0xf0, "Copy initial value for e1 for fourth tile (3, 3, 7, 7)");
    fswizz_ps(f15, f12, 0xf0, "Copy initial value for e2 for fourth tile (3, 3, 7, 7)");
    mov_m_x(m0, x0, 0x44, "Set mask to (0, 1, 0, 1)");
    fbci_pi(f16, 32, "Set offset 32");
    fmul_pi(f3, f0, f16, "a0 * 32");
    fmul_pi(f4, f1, f16, "a1 * 32");
    fmul_pi(f5, f2, f16, "a2 * 32");
    fadd_pi(f3, f13, f3, "Evaluate e0 at points (3, 8, 7, 9)");
    fadd_pi(f4, f14, f4, "Evaluate e1 at points (3, 8, 7, 9)");
    fadd_pi(f5, f15, f5, "Evaluate e2 at points (3, 8, 7, 9)");
    fbci_pi(f16, 0, "Set to zero");
    fltm_pi(m1, f3, f16, "Test (e0 < 0) for points (3, 8, 7, 9)");
    fltm_pi(m2, f4, f16, "Test (e1 < 0) for points (3, 8, 7, 9)");
    fltm_pi(m3, f5, f16, "Test (e2 < 0) for points (3, 8, 7, 9)");

    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_Nx32;

    maskpopc(x24, m2, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_Nx32;

    maskpopc(x24, m3, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 2
    if (XREGS[x24].x == 8)
        goto end_Nx32;

    // Get fourth tile initial sample position from point 3
    fmvs_x_ps(x16, f6, 3, "Copy e0 at point 3");
    fmvs_x_ps(x17, f7, 3, "Copy e1 at point 3");
    fmvs_x_ps(x18, f8, 3, "Copy e2 at point 3");
    generate_triangle_with_tile_128x128_packet();

end_Nx32:

    print_comment("<<< rasterize_Nx32");

    return;
}

void rasterize_32xN_to_32x32()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  32x[64|96|128]
    //
    //  8--9
    //  |  |
    //  3--7
    //  |  |
    //  2--6
    //  |  |
    //  1--5
    //  |  |
    //  0--4
    //

    print_comment(">>> rasterize_32xN");

    flw_ps(f9, 0, x23, "Load offsets (0, 32, 64, 96)");
    fmul_pi(f3, f3, f9, "b0 * (0, 32, 64, 96)");
    fmul_pi(f4, f4, f9, "b1 * (0, 32, 64, 96)");
    fmul_pi(f5, f5, f9, "b2 * (0, 32, 64, 96)");
    fadd_pi(f6, f6, f3, "Sample e0 at points (0, 1, 2, 3)");
    fadd_pi(f7, f7, f4, "Sample e1 at points (0, 1, 2, 3)");
    fadd_pi(f8, f8, f5, "Sample e2 at points (0, 1, 2, 3)");

    fbci_pi(f9, 32, "Set offset 32");
    fmul_pi(f0, f0, f9, "a0 * 32");
    fmul_pi(f1, f1, f9, "a1 * 32");
    fmul_pi(f2, f2, f9, "a2 * 32");
    fadd_pi(f10, f6, f0, "Sample e0 at points (4, 5, 6, 7)");
    fadd_pi(f11, f7, f1, "Sample e1 at points (4, 5, 6, 7)");
    fadd_pi(f12, f8, f2, "Sample e2 at points (4, 5, 6, 7)");
    
    fbci_pi(f13, 0, "Load 0");
    fltm_pi(m1, f6, f13, "Test (e0 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m2, f7, f13, "Test (e1 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m3, f8, f13, "Test (e2 < 0) for points (0, 1, 2, 3)");

    fltm_pi(m4, f10, f13, "Test (e0 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m5, f11, f13, "Test (e1 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m6, f12, f13, "Test (e2 < 0) for points (4, 5, 6, 7)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_32xN)

    generate_triangle_with_tile_128x128_packet();

tile1_32xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_32xN)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_32xN:

    // Check if vertical dimension is larger than 64 and there are 3 tiles
    if (XREGS[x20].x <= 64)
        goto end_32xN;

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile3_32xN)

    // Get third tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f7, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f8, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_32xN:

    // Check if vertical dimension is larger than 96 and there are 4 tiles
    if (XREGS[x20].x <= 96)
        goto end_32xN;

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    mov_m_x(m0, x0, 0x05, "Set mask to (1, 1, 0, 0)");
    fswizz_ps(f13, f6, 0x0f, "Copy initial value for e0 for fourth tile (3, 3, x, x)");
    fswizz_ps(f14, f7, 0x0f, "Copy initial value for e1 for fourth tile (3, 3, x, x)");
    fswizz_ps(f15, f8, 0x0f, "Copy initial value for e2 for fourth tile (3, 3, x, x)");
    mov_m_x(m0, x0, 0x50, "Set mask to (0, 0, 1, 1)");
    fswizz_ps(f13, f10, 0xf0, "Copy initial value for e0 for fourth tile (3, 3, 7, 7)");
    fswizz_ps(f14, f11, 0xf0, "Copy initial value for e1 for fourth tile (3, 3, 7, 7)");
    fswizz_ps(f15, f12, 0xf0, "Copy initial value for e2 for fourth tile (3, 3, 7, 7)");
    mov_m_x(m0, x0, 0x44, "Set mask to (0, 1, 0, 1)");
    fbci_pi(f16, 32, "Set offset 32");
    fmul_pi(f3, f3, f16, "b0 * 32");
    fmul_pi(f4, f4, f16, "b1 * 32");
    fmul_pi(f5, f5, f16, "b2 * 32");
    fadd_pi(f3, f13, f3, "Evaluate e0 at points (3, 8, 7, 9)");
    fadd_pi(f4, f14, f4, "Evaluate e1 at points (3, 8, 7, 9)");
    fadd_pi(f5, f15, f5, "Evaluate e2 at points (3, 8, 7, 9)");
    fbci_pi(f16, 0, "Set to zero");
    fltm_pi(m1, f3, f16, "Test (e0 < 0) for points (3, 8, 7, 9)");
    fltm_pi(m2, f4, f16, "Test (e1 < 0) for points (3, 8, 7, 9)");
    fltm_pi(m3, f5, f16, "Test (e2 < 0) for points (3, 8, 7, 9)");

    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_32xN;

    maskpopc(x24, m2, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_32xN;

    maskpopc(x24, m3, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 2
    if (XREGS[x24].x == 8)
        goto end_32xN;

    // Get fourth tile initial sample position from point 3
    fmvs_x_ps(x16, f6, 3, "Copy e0 at point 3");
    fmvs_x_ps(x17, f7, 3, "Copy e1 at point 3");
    fmvs_x_ps(x18, f8, 3, "Copy e2 at point 3");
    generate_triangle_with_tile_128x128_packet();

end_32xN:

    print_comment("<<< rasterize_32xN");

    return;
}


void rasterize_96xN_to_32x32()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  96x[64|96|128]
    //
    //  16-17-18-19
    //  |  |  |  |
    //  12-13-14-15
    //  |  |  |  |
    //  8--9--10-11
    //  |  |  |  |
    //  4--5--6--7
    //  |  |  |  |
    //  0--1--2--3
    //

    // ei ( 0,  1,  2,  3) -> f0 - f2
    // ei ( 4,  5,  6,  7) -> f3 - f5
    // ei ( 8,  9, 10, 11) -> f6 - f8
    //
    // test ( 0,  1,  2,  3) -> f9  - f11
    // test ( 4,  5,  6,  7) -> f12 - f14
    // test ( 8,  9, 10, 11) -> f15 - f17
    //

    print_comment(">>> rasterize_96xN");

    flw_ps(f18, 0, x23, "Load offsets (0, 32, 64, 96)");
    fmul_pi(f19, f0, f18, "a0 * (0, 32, 64, 96)");
    fmul_pi(f20, f1, f18, "a1 * (0, 32, 64, 96)");
    fmul_pi(f21, f2, f18, "a2 * (0, 32, 64, 96)");

    fadd_pi(f0, f6, f19, "Sample e0 at points (0, 1, 2, 3)");
    fadd_pi(f1, f7, f20, "Sample e1 at points (0, 1, 2, 3)");
    fadd_pi(f2, f8, f21, "Sample e2 at points (0, 1, 2, 3)");

    fbci_pi(f18, 32, "Set offset 32");
    fmul_pi(f19, f3, f18, "b0 * 32");
    fmul_pi(f20, f4, f18, "b1 * 32");
    fmul_pi(f21, f5, f18, "b2 * 32");

    fadd_pi(f3, f0, f19, "Sample e0 at points (4, 5, 6, 7)");
    fadd_pi(f4, f1, f20, "Sample e1 at points (4, 5, 6, 7)");
    fadd_pi(f5, f2, f21, "Sample e2 at points (4, 5, 6, 7)");
    
    fadd_pi(f6, f3, f19, "Sample e0 at points (8, 9, 10, 11)");
    fadd_pi(f7, f4, f20, "Sample e1 at points (8, 9, 10, 11)");
    fadd_pi(f8, f5, f21, "Sample e2 at points (8, 9, 10, 11)");

    fbci_pi(f18, 0, "Load 0");

    fltm_pi(m1, f0, f18, "Test (e0 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m2, f1, f18, "Test (e1 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m3, f2, f18, "Test (e2 < 0) for points (0, 1, 2, 3)");

    fltm_pi(m4, f3, f18, "Test (e0 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m5, f4, f18, "Test (e1 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m6, f5, f18, "Test (e2 < 0) for points (4, 5, 6, 7)");

    addi(x25, x21, 0, "Save tile x position for second row");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_96xN)

    generate_triangle_with_tile_128x128_packet();

tile1_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_96xN)

    // Get second tile initial sample position from point 1
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_96xN:

    // Check if horizontal dimension is larger than 64 and there are 3 tiles in the row
    if (XREGS[x19].x <= 64)
        goto tile3_96xN;

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile3_96xN)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_96xN:

    fltm_pi(m1, f6, f18, "Test (e0 < 0) for points (8, 9, 10, 11)");
    fltm_pi(m2, f7, f18, "Test (e1 < 0) for points (8, 9, 10, 11)");
    fltm_pi(m3, f8, f18, "Test (e2 < 0) for points (8, 9, 10, 11)");

    addi(x21, x25, 0, "Restore tile x coordinate for second row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 0, tile4_96xN)

    // Get second tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

tile4_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 1, tile5_96xN)

    // Get second tile initial sample position from point 5
    fmvs_x_ps(x16, f3, 1, "Copy e0 at point 5");
    fmvs_x_ps(x17, f4, 1, "Copy e1 at point 5");
    fmvs_x_ps(x18, f5, 1, "Copy e2 at point 5");
    generate_triangle_with_tile_128x128_packet();

tile5_96xN:

    // Check if horizontal dimension is larger than 64 and there are 3 tiles in the row
    if (XREGS[x19].x <= 64)
        goto tile6_96xN;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 2, tile6_96xN)

    // Get second tile initial sample position from point 6
    fmvs_x_ps(x16, f3, 2, "Copy e0 at point 6");
    fmvs_x_ps(x17, f4, 2, "Copy e1 at point 6");
    fmvs_x_ps(x18, f5, 2, "Copy e2 at point 6");
    generate_triangle_with_tile_128x128_packet();

tile6_96xN:

    // Check if vertical dimension is larger than 64 and there are 3 rows
    if (XREGS[x20].x <= 64)
        goto end_96xN;

    fadd_pi(f0, f6, f19, "Sample e0 at points (12, 13, 14, 15)");
    fadd_pi(f1, f7, f20, "Sample e1 at points (12, 13, 14, 15)");
    fadd_pi(f2, f8, f21, "Sample e2 at points (12, 13, 14, 15)");
    
    fltm_pi(m4,  f0, f18, "Test (e0 < 0) for points (12, 13, 14, 15)");
    fltm_pi(m5, f1, f18, "Test (e1 < 0) for points (12, 13, 14, 15)");
    fltm_pi(m6, f2, f18, "Test (e2 < 0) for points (12, 13, 14, 15)");

    addi(x21, x25, 0, "Restore tile x coordinate for third row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile7_96xN)

    // Get second tile initial sample position from point 8
    fmvs_x_ps(x16, f6, 0, "Copy e0 at point 8");
    fmvs_x_ps(x17, f7, 0, "Copy e1 at point 8");
    fmvs_x_ps(x18, f8, 0, "Copy e2 at point 8");
    generate_triangle_with_tile_128x128_packet();

tile7_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile8_96xN)

    // Get second tile initial sample position from point 9
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 9");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 9");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 9");
    generate_triangle_with_tile_128x128_packet();

tile8_96xN:

    // Check if horizontal dimension is larger than 64 and there are 3 tiles in the row
    if (XREGS[x19].x <= 64)
        goto tile9_96xN;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile9_96xN)

    // Get second tile initial sample position from point 10
    fmvs_x_ps(x16, f6, 2, "Copy e0 at point 10");
    fmvs_x_ps(x17, f7, 2, "Copy e1 at point 10");
    fmvs_x_ps(x18, f8, 2, "Copy e2 at point 10");
    generate_triangle_with_tile_128x128_packet();

tile9_96xN:

    // Check if vertical dimension is larger than 96 and there are 4 rows
    if (XREGS[x20].x <= 96)
        goto end_96xN;

    fadd_pi(f3, f0, f19, "Sample e0 at points (16, 17, 18, 19)");
    fadd_pi(f4, f1, f20, "Sample e1 at points (16, 17, 18, 19)");
    fadd_pi(f5, f2, f21, "Sample e2 at points (16, 17, 18, 19)");
    
    fltm_pi(m1, f3, f18, "Test (e0 < 0) for points (16, 17, 18, 19)");
    fltm_pi(m2, f4, f18, "Test (e1 < 0) for points (16, 17, 18, 19)");
    fltm_pi(m3, f5, f18, "Test (e2 < 0) for points (16, 17, 18, 19)");

    addi(x21, x25, 0, "Restore tile x coordinate for fourth row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 0, tile10_96xN)

    // Get second tile initial sample position from point 12
    fmvs_x_ps(x16, f0, 0, "Copy e0 at point 12");
    fmvs_x_ps(x17, f1, 0, "Copy e1 at point 12");
    fmvs_x_ps(x18, f2, 0, "Copy e2 at point 12");
    generate_triangle_with_tile_128x128_packet();

tile10_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 1, tile11_96xN)

    // Get second tile initial sample position from point 13
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 13");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 13");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 13");
    generate_triangle_with_tile_128x128_packet();

tile11_96xN:

    // Check if horizontal dimension is larger than 64 and there are 3 tiles in the row
    if (XREGS[x19].x <= 64)
        goto end_96xN;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 2, end_96xN)

    // Get second tile initial sample position from point 14
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 14");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 14");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 14");
    generate_triangle_with_tile_128x128_packet();

end_96xN:

    print_comment("<<< rasterize_96xN");

    return;
}

void rasterize_Nx96_to_32x32()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  [64|96|128]x96
    //
    //  3--7--11-15-19
    //  |  |  |  |  |
    //  2--6--10-14-18
    //  |  |  |  |  |
    //  1--5--9--13-17
    //  |  |  |  |  |
    //  0--4--8--12-16
    //

    // ei (0, 1,  2,  3) -> f0 - f2
    // ei (4, 5,  6,  7) -> f3 - f5
    // ei (8, 9, 10, 11) -> f6 - f8
    //
    // test (0,  1,  2,  3) -> f9 - f11
    // test (4,  6,  7,  8) -> f12 - f14
    // test (9, 10, 11, 12) -> f15 - f17
    //

    print_comment(">>> rasterize_Nx96");

    fbci_pi(f18, 32, "Set offset 32");
    fmul_pi(f19, f0, f18, "a0 * 32");
    fmul_pi(f20, f1, f18, "a1 * 32");
    fmul_pi(f21, f2, f18, "a2 * 32");

    flw_ps(f18, 0, x23, "Load offsets (0, 32, 64, 96)");
    fmul_pi(f22, f3, f18, "b0 * (0, 32, 64, 96)");
    fmul_pi(f23, f4, f18, "b1 * (0, 32, 64, 96)");
    fmul_pi(f24, f5, f18, "b2 * (0, 32, 64, 96)");

    fadd_pi(f0, f6, f22, "Sample e0 at points (0, 1, 2, 3)");
    fadd_pi(f1, f7, f23, "Sample e1 at points (0, 1, 2, 3)");
    fadd_pi(f2, f8, f24, "Sample e2 at points (0, 1, 2, 3)");

    fadd_pi(f3, f0, f19, "Sample e0 at points (4, 5, 6, 7)");
    fadd_pi(f4, f1, f20, "Sample e1 at points (4, 5, 6, 7)");
    fadd_pi(f5, f2, f21, "Sample e2 at points (4, 5, 6, 7)");
    
    fadd_pi(f6, f3, f19, "Sample e0 at points (8, 9, 10, 11)");
    fadd_pi(f7, f4, f20, "Sample e1 at points (8, 9, 10, 11)");
    fadd_pi(f8, f5, f21, "Sample e2 at points (8, 9, 10, 11)");

    fbci_pi(f18, 0, "Load 0");
    fltm_pi(m1, f0, f18, "Test (e0 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m2, f1, f18, "Test (e1 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m3, f2, f18, "Test (e2 < 0) for points (0, 1, 2, 3)");

    fltm_pi(m4, f3, f18, "Test (e0 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m5, f4, f18, "Test (e1 < 0) for points (4, 5, 6, 7)");
    fltm_pi(m6, f5, f18, "Test (e2 < 0) for points (4, 5, 6, 7)");

    addi(x25, x22, 0, "Save tile y position for second column");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_Nx96)

    generate_triangle_with_tile_128x128_packet();

tile1_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_Nx96)

    // Get second tile initial sample position from point 1
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_Nx96:

    // Check if vertical dimension is larger than 64 and there are 3 tiles in the column
    if (XREGS[x20].x <= 64)
        goto tile3_Nx96;

    addi(x22, x22, 1, "Advance tile y coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile3_Nx96)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_Nx96:

    fltm_pi(m1, f6, f18, "Test (e0 < 0) for points (8, 9, 10, 11)");
    fltm_pi(m2, f7, f18, "Test (e1 < 0) for points (8, 9, 10, 11)");
    fltm_pi(m3, f8, f18, "Test (e2 < 0) for points (8, 9, 10, 11)");

    addi(x22, x25, 0, "Restore tile y coordinate for second column");
    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 0, tile4_Nx96)

    // Get second tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

tile4_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 1, tile5_Nx96)

    // Get second tile initial sample position from point 5
    fmvs_x_ps(x16, f3, 1, "Copy e0 at point 5");
    fmvs_x_ps(x17, f4, 1, "Copy e1 at point 5");
    fmvs_x_ps(x18, f5, 1, "Copy e2 at point 5");
    generate_triangle_with_tile_128x128_packet();

tile5_Nx96:

    // Check if vertical dimension is larger than 64 and there are 3 tiles in the column
    if (XREGS[x20].x <= 64)
        goto tile6_Nx96;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 2, tile6_Nx96)

    // Get second tile initial sample position from point 6
    fmvs_x_ps(x16, f3, 2, "Copy e0 at point 6");
    fmvs_x_ps(x17, f4, 2, "Copy e1 at point 6");
    fmvs_x_ps(x18, f5, 2, "Copy e2 at point 6");
    generate_triangle_with_tile_128x128_packet();

tile6_Nx96:

    // Check if horizontal dimension is larger than 64 and there are 3 columns
    if (XREGS[x19].x <= 64)
        goto end_Nx96;

    fadd_pi(f0, f6, f19, "Sample e0 at points (12, 13, 14, 15)");
    fadd_pi(f1, f7, f20, "Sample e1 at points (12, 13, 14, 15)");
    fadd_pi(f2, f8, f21, "Sample e2 at points (12, 13, 14, 15)");
    
    fltm_pi(m4,  f0, f18, "Test (e0 < 0) for points (12, 13, 14, 15)");
    fltm_pi(m5, f1, f18, "Test (e1 < 0) for points (12, 13, 14, 15)");
    fltm_pi(m6, f2, f18, "Test (e2 < 0) for points (12, 13, 14, 15)");

    addi(x22, x25, 0, "Restore tile y coordinate for third column");
    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile7_Nx96)

    // Get second tile initial sample position from point 8
    fmvs_x_ps(x16, f6, 0, "Copy e0 at point 8");
    fmvs_x_ps(x17, f7, 0, "Copy e1 at point 8");
    fmvs_x_ps(x18, f8, 0, "Copy e2 at point 8");
    generate_triangle_with_tile_128x128_packet();

tile7_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile8_Nx96)

    // Get second tile initial sample position from point 9
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 9");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 9");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 9");
    generate_triangle_with_tile_128x128_packet();

tile8_Nx96:

    // Check if vertical dimension is larger than 64 and there are 3 tiles in the column
    if (XREGS[x20].x <= 64)
        goto tile9_Nx96;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile9_Nx96)

    // Get second tile initial sample position from point 10
    fmvs_x_ps(x16, f6, 2, "Copy e0 at point 10");
    fmvs_x_ps(x17, f7, 2, "Copy e1 at point 10");
    fmvs_x_ps(x18, f8, 2, "Copy e2 at point 10");
    generate_triangle_with_tile_128x128_packet();

tile9_Nx96:

    // Check if horizontal dimension is larger than 96 and there are 4 columns
    if (XREGS[x19].x <= 96)
        goto end_Nx96;

    fadd_pi(f3, f0, f19, "Sample e0 at points (16, 17, 18, 19)");
    fadd_pi(f4, f1, f20, "Sample e1 at points (16, 17, 18, 19)");
    fadd_pi(f5, f2, f21, "Sample e2 at points (16, 17, 18, 19)");
    
    fltm_pi(m1, f3, f18, "Test (e0 < 0) for points (16, 17, 18, 19)");
    fltm_pi(m2, f4, f18, "Test (e1 < 0) for points (16, 17, 18, 19)");
    fltm_pi(m3, f5, f18, "Test (e2 < 0) for points (16, 17, 18, 19)");

    addi(x21, x25, 0, "Restore tile x coordinate for second row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 0, tile10_Nx96)

    // Get second tile initial sample position from point 12
    fmvs_x_ps(x16, f0, 0, "Copy e0 at point 12");
    fmvs_x_ps(x17, f1, 0, "Copy e1 at point 12");
    fmvs_x_ps(x18, f2, 0, "Copy e2 at point 12");
    generate_triangle_with_tile_128x128_packet();

tile10_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 1, tile11_Nx96)

    // Get second tile initial sample position from point 13
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 13");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 13");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 13");
    generate_triangle_with_tile_128x128_packet();

tile11_Nx96:

    // Check if vertical dimension is larger than 64 and there are 3 tiles in the column
    if (XREGS[x20].x <= 64)
        goto end_Nx96;

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, x24, 2, end_Nx96)

    // Get second tile initial sample position from point 14
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 14");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 14");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 14");
    generate_triangle_with_tile_128x128_packet();

end_Nx96:

    print_comment("<<< rasterize_Nx96");

    return;
}

void rasterize_128x128_to_32x32()
{
    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    //  128x128
    //
    //  20-21-22-23-24
    //  |  |  |  |  |
    //  15-16-17-18-19
    //  |  |  |  |  |
    //  10-11-12-13-14
    //  |  |  |  |  |
    //  5--6--7--8--9
    //  |  |  |  |  |
    //  0--1--2--3--4
    //

    //
    // ei (0, 1, 2, 3) -> f0 - f2
    // ei (5, 6, 7, 8) -> f3 - f5
    // ei (3, 4, 8, 9) -> f6 - f8
    //
    // test (0, 1, 2, 3) -> f9 - f11
    // test (5, 6, 7, 8) -> f12 - f14
    // test (3, 4, 8, 9) -> f15 - f17
    //

    print_comment(">>> rasterize_128x128");

    flw_ps(f20, 0, x23, "Load offsets (0, 32, 64, 96)");
    fmul_pi(f21, f0, f20, "a0 * (0, 32, 64, 96)");
    fmul_pi(f22, f1, f20, "a1 * (0, 32, 64, 96)");
    fmul_pi(f23, f2, f20, "a2 * (0, 32, 64, 96)");

    fbci_pi(f20, 32, "Set offset 32");
    fmul_pi(f18, f0, f20, "a0 * 32");
    fmul_pi(f19, f1, f20, "a1 * 32");
    fmul_pi(f20, f2, f20, "a2 * 32");

    fadd_pi(f0, f6, f21, "Sample e0 at points (0, 1, 2, 3)");
    fadd_pi(f1, f7, f22, "Sample e1 at points (0, 1, 2, 3)");
    fadd_pi(f2, f8, f23, "Sample e2 at points (0, 1, 2, 3)");

    fbci_pi(f24, 32, "Set offset 32");
    fmul_pi(f21, f3, f20, "b0 * 32");
    fmul_pi(f22, f4, f20, "b1 * 32");
    fmul_pi(f23, f5, f20, "b2 * 32");

    fbci_pi(f24, 0, "Load 0");

    addi(x25, x21, 0, "Save tile x position");
    addi(x26, x0, 0, "Initilize tile row to 0");

row_128x128:

    fadd_pi(f3, f0, f21, "Sample e0 at points (5, 6, 7, 8)");
    fadd_pi(f4, f1, f22, "Sample e1 at points (5, 6, 7, 8)");
    fadd_pi(f5, f2, f23, "Sample e2 at points (5, 6, 7, 8)");
    
    mov_m_x(m0, x0, 0x05, "Set mask to (1, 1, 0, 0)");
    fswizz_ps(f6, f0, 0x0f, "Copy initial value for e0 for fourth tile (3, 3, x, x)");
    fswizz_ps(f7, f1, 0x0f, "Copy initial value for e1 for fourth tile (3, 3, x, x)");
    fswizz_ps(f8, f2, 0x0f, "Copy initial value for e2 for fourth tile (3, 3, x, x)");
    mov_m_x(m0, x0, 0x50, "Set mask to (0, 0, 1, 1)");
    fswizz_ps(f6, f3, 0xf0, "Copy initial value for e0 for fourth tile (3, 3, 8, 8)");
    fswizz_ps(f7, f4, 0xf0, "Copy initial value for e1 for fourth tile (3, 3, 8, 8)");
    fswizz_ps(f8, f5, 0xf0, "Copy initial value for e2 for fourth tile (3, 3, 8, 8)");
    mov_m_x(m0, x0, 0x44, "Set mask to (0, 1, 0, 1)");

    fadd_pi(f6, f6, f18, "Evaluate e0 at points (3, 4, 8, 9)");
    fadd_pi(f7, f7, f19, "Evaluate e1 at points (3, 4, 8, 9)");
    fadd_pi(f8, f8, f20, "Evaluate e2 at points (3, 4, 8, 9)");

    mov_m_x(m0, x0, 0x55, "Set mask to (1, 1, 1, 1)");

    fltm_pi(m1,  f0, f24, "Test (e0 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m2, f1, f24, "Test (e1 < 0) for points (0, 1, 2, 3)");
    fltm_pi(m3, f2, f24, "Test (e2 < 0) for points (0, 1, 2, 3)");

    fltm_pi(m4, f3, f24, "Test (e0 < 0) for points (5, 6, 7, 8)");
    fltm_pi(m5, f4, f24, "Test (e1 < 0) for points (5, 6, 7, 8)");
    fltm_pi(m6, f5, f24, "Test (e2 < 0) for points (5, 6, 7, 8)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 0, tile1_128x128)

    generate_triangle_with_tile_128x128_packet();

tile1_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 1, tile2_128x128)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
 
    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, x24, 2, tile3_128x128)

    // Get third tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    fltm_pi(m1, f6, f24, "Test (e0 < 0) for points (3, 4, 8, 9)");
    fltm_pi(m2, f7, f24, "Test (e1 < 0) for points (3, 4, 8, 9)");
    fltm_pi(m3, f8, f24, "Test (e2 < 0) for points (3, 4, 8, 9)");

    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_row_128x128;

    maskpopc(x24, m2, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_row_128x128;

    maskpopc(x24, m3, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 2
    if (XREGS[x24].x == 8)
        goto end_row_128x128;

    // Get fourth tile initial sample position from point 3
    fmvs_x_ps(x16, f0, 3, "Copy e0 at point 3");
    fmvs_x_ps(x17, f1, 3, "Copy e1 at point 3");
    fmvs_x_ps(x18, f2, 3, "Copy e2 at point 3");
    generate_triangle_with_tile_128x128_packet();

end_row_128x128:

    // Check if last tile row reached
    if (XREGS[x26].x == 3)
        goto end_128x128;

    addi(x26, x26, 1, "Update tile row");
    addi(x21, x25, 0, "Restore tile x for next row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    faddi_pi(f0, f3, 0, "Top row e0 samples from previous row become bottom row samples for new row");
    faddi_pi(f1, f4, 0, "Top row e1 samples from previous row become bottom row samples for new row");
    faddi_pi(f2, f5, 0, "Top row e2 samples from previous row become bottom row samples for new row");

    goto row_128x128;

end_128x128:

    print_comment("<<< rasterize_128x128");

    return;
}


void create_low_prec_triangle_rbox_packets_tile_size_64x64()
{
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    // x15 -> tile size and triangle facing for RBOX tile packets
    //

    print_comment(">>> create_low_prec_triangle_rbox_packets");

    slli(x7, x12, 2, "triangle index * 4");
    add(x8, x7, x7, "triangle index * 8");
   
    add(x8, x10, x8, "Pointer to triangle coefficients");

    lw(x1,    0, x8, "Load a0");
    lw(x2,   32, x8, "Load a1");
    lw(x3,   64, x8, "Load a2");

    lw(x4,   96, x8, "Load b0");
    lw(x5,  128, x8, "Load b1");
    lw(x6,  160, x8, "Load b2");

    lw(x16, 192, x8, "Load c0");
    lw(x17, 224, x8, "Load c1");
    lw(x18, 256, x8, "Load c2");

    add(x7, x10, x7, "Pointer to triangle coefficients");

    lw(x19, 352, x7, "Load tile bound dim x");
    lw(x20, 368, x7, "Load tile bound dim y");

    mul(x21, x19, x20, "Compute tile bound size");
    
    if (XREGS[x21].x == 0x1000)
    {
        //  Triangle inside 64x64 tile.

        // Reduce precision to 9.14 as triangle is inside a 64x64 tile
        srai(x1, x1, 1, "Reduce a0 precision to 9.14");
        srai(x2, x2, 1, "Reduce a1 precision to 9.14");
        srai(x3, x3, 1, "Reduce a2 precision to 9.14");

        srai(x4, x4, 1, "Reduce b0 precision to 9.14");
        srai(x5, x5, 1, "Reduce b1 precision to 9.14");
        srai(x6, x6, 1, "Reduce b1 precision to 9.14");

        // Reduce precision to 1.14 as triangle is inside a 64x64 tile
        srai(x16, x16, 1, "Reduce c0 precision to 1.14");
        srai(x17, x17, 1, "Reduce c0 precision to 1.14");
        srai(x18, x18, 1, "Reduce c0 precision to 1.14");

        // Encode packet header.
        
        slli(x21, x15, 13, "Pack RBOX packet header, tile size and triangle facing");
        lwu(x19, 288, x7, "Load tile bound min x");
        lwu(x20, 320, x7, "Load tile bound max x");
        srli(x19, x19, 14, "Get 64x64 tile x position");
        srli(x20, x20, 14, "Get 64x64 tile y position");
        or_(x21, x20, x21, "Pack RBOX packet header, tile y");
        slli(x21, x21, 13, "Pack RBOX packet header, tile x");
        or_(x21, x19, x21, "Pack RBOX packet header, tile x");
        slli(x21, x21, 3, "Pack RBOX packet header");
        ori(x21, x21, 0, "Pack RBOX packet header, packet type -> RBOX_INPCKT_TRIANGLE_WITH_TILE_64x64");
        sw(x21,  0, x11, "Store header in RBOX packet");
        sw(x1,   8, x11, "Store a0 in RBOX packet");
        sw(x4,  12, x11, "Store b0 in RBOX packet");
        sw(x2,  16, x11, "Store a1 in RBOX packet");
        sw(x5,  20, x11, "Store b1 in RBOX packet");
        sw(x3,  24, x11, "Store a2 in RBOX packet");
        sw(x6,  28, x11, "Store b2 in RBOX packet");
        sd(x0,  32, x11, "Store ad, bd in RBOX packet");
        sd(x0,  40, x11, "Store pointer to triangle data RBOX packet");
        sw(x16, 48, x11, "Store c0 in RBOX packet");
        sw(x17, 52, x11, "Store c1 in RBOX packet");
        sw(x18, 56, x11, "Store c2 in RBOX packet");
        sw(x0,  60, x11, "Store cd in RBOX packet");

        addi(x11, x11, 64, "Update pointer to RBOX input buffer");

        print_comment("<<< create_low_prec_triangle_rbox_packets");

        return;
    }

    // 
    // 128x128 -> 2x2 x 64x64
    //
    // Evaluate the 4 64x64 tiles in a 128x128 area
    //
    // Each of the tiles should be assigned to a different Shire, so all RBOX packets
    // are of the Triangle With Tile 128x128 type.
    //
    //  - Trivial tile rejection : for any  of the edge equations the four tile corners
    //    are negative.
    //  - Use bottom left corner, offset (0, 0) as initial sample point for the trile
    //
    // For MSAA the tile corners have to be adjusted to cover all MSAA sample points!!
    //

    generate_base_triangle_with_tile_128x128_packet();

    fbcx_ps(f0, x1, "Broadcast a0");
    fbcx_ps(f1, x2, "Broadcast a1");
    fbcx_ps(f2, x3, "Broadcast a2");

    fbcx_ps(f3, x4, "Broadcast b0");
    fbcx_ps(f4, x5, "Broadcast b1");
    fbcx_ps(f5, x6, "Broadcast b2");

    fbcx_ps(f6, x16, "Broadcast c0");
    fbcx_ps(f7, x17, "Broadcast c1");
    fbcx_ps(f8, x18, "Broadcast c2");

    lwu(x21, 288, x7, "Load tile bound min x");
    lwu(x22, 320, x7, "Load tile bound max x");
    srli(x21, x21, 14, "Get 64x64 tile x position");
    srli(x22, x22, 14, "Get 64x64 tile y position");

    init(x23, (uint64) rast_lit_tile_size_64x64);

    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    // Check tile bound vertical dimension
    if (XREGS[x20].x > 64)
        goto triangle_not_128x64;

    rasterize_128x64_to_64x64();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_128x64:

    if (XREGS[x19].x > 64)
        goto triangle_not_64x128;

    rasterize_64x128_to_64x64();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_64x128:

    rasterize_128x128_to_64x64();

    print_comment("<<< create_low_prec_triangle_rbox_packets");
}

void create_low_prec_triangle_rbox_packets_tile_size_32x32()
{
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    // x15 -> tile size and triangle facing for RBOX tile packets
    //

    print_comment(">>> create_low_prec_triangle_rbox_packets");

    slli(x7, x12, 2, "triangle index * 4");
    add(x8, x7, x7, "triangle index * 8");
   
    add(x8, x10, x8, "Pointer to triangle coefficients");

    lw(x1,    0, x8, "Load a0");
    lw(x2,   32, x8, "Load a1");
    lw(x3,   64, x8, "Load a2");

    lw(x4,   96, x8, "Load b0");
    lw(x5,  128, x8, "Load b1");
    lw(x6,  160, x8, "Load b2");

    lw(x16, 192, x8, "Load c0");
    lw(x17, 224, x8, "Load c1");
    lw(x18, 256, x8, "Load c2");

    add(x7, x10, x7, "Pointer to triangle coefficients");

    lw(x19, 352, x7, "Load tile bound dim x");
    lw(x20, 368, x7, "Load tile bound dim y");

    mul(x21, x19, x20, "Compute tile bound size");
    
    if (XREGS[x21].x == 0x400)
    {
        //  Triangle inside 32x32 tile.

        // Reduce precision to 9.13 as triangle is inside a 32x32 tile
        srai(x1, x1, 2, "Reduce a0 precision to 9.13");
        srai(x2, x2, 2, "Reduce a1 precision to 9.13");
        srai(x3, x3, 2, "Reduce a2 precision to 9.13");

        srai(x4, x4, 2, "Reduce b0 precision to 9.13");
        srai(x5, x5, 2, "Reduce b1 precision to 9.13");
        srai(x6, x6, 2, "Reduce b1 precision to 9.13");

        // Reduce precision to 1.13 as triangle is inside a 32x32 tile
        srai(x16, x16, 2, "Reduce c0 precision to 1.13");
        srai(x17, x17, 2, "Reduce c0 precision to 1.13");
        srai(x18, x18, 2, "Reduce c0 precision to 1.13");

        // Encode packet header.
        
        slli(x21, x15, 13, "Pack RBOX packet header, tile size and triangle");
        lwu(x19, 288, x7, "Load tile bound min x");
        lwu(x20, 320, x7, "Load tile bound max x");
        srli(x19, x19, 13, "Get 32x32 tile x position");
        srli(x20, x20, 13, "Get 32x32 tile y position");
        or_(x21, x20, x21, "Pack RBOX packet, header, tile y");
        slli(x21, x21, 13, "Pack RBOX packet header, tile x");
        or_(x21, x19, x21, "Pack RBOX packet header, tile x");
        slli(x21, x21, 3, "Pack RBOX packet header");
        ori(x21, x21, 0, "Pack RBOX packet header, packet type -> RBOX_INPCKT_TRIANGLE_WITH_TILE_32x32");
        sw(x21,  0, x11, "Store header in RBOX packet");
        sw(x1,   8, x11, "Store a0 in RBOX packet");
        sw(x4,  12, x11, "Store b0 in RBOX packet");
        sw(x2,  16, x11, "Store a1 in RBOX packet");
        sw(x5,  20, x11, "Store b1 in RBOX packet");
        sw(x3,  24, x11, "Store a2 in RBOX packet");
        sw(x6,  28, x11, "Store b2 in RBOX packet");
        sd(x0,  32, x11, "Store ad, bd in RBOX packet");
        sd(x0,  40, x11, "Store pointer to triangle data RBOX packet");
        sw(x16, 48, x11, "Store c0 in RBOX packet");
        sw(x17, 52, x11, "Store c1 in RBOX packet");
        sw(x18, 56, x11, "Store c2 in RBOX packet");
        sw(x0,  60, x11, "Store cd in RBOX packet");

        addi(x11, x11, 64, "Update pointer to RBOX input buffer");

        print_comment("<<< create_low_prec_triangle_rbox_packets");

        return;
    }

    // 
    // 128x128 -> 4x4 x 32x32
    //
    // Evaluate the 16 32x32 tiles in a 128x128 area
    //
    // Each of the tiles should be assigned to a different Shire, so all RBOX packets
    // are of the Triangle With Tile 128x128 type.
    //
    //  - Trivial tile rejection : for any  of the edge equations the four tile corners
    //    are negative.
    //  - Use bottom left corner, offset (0, 0) as initial sample point for the trile
    //
    // For MSAA the tile corners have to be adjusted to cover all MSAA sample points!!
    //

    generate_base_triangle_with_tile_128x128_packet();

    fbcx_ps(f0, x1, "Broadcast a0");
    fbcx_ps(f1, x2, "Broadcast a1");
    fbcx_ps(f2, x3, "Broadcast a2");

    fbcx_ps(f3, x4, "Broadcast b0");
    fbcx_ps(f4, x5, "Broadcast b1");
    fbcx_ps(f5, x6, "Broadcast b2");

    fbcx_ps(f6, x16, "Broadcast c0");
    fbcx_ps(f7, x17, "Broadcast c1");
    fbcx_ps(f8, x18, "Broadcast c2");

    lwu(x21, 288, x7, "Load tile bound min x");
    lwu(x22, 320, x7, "Load tile bound max x");
    srli(x21, x21, 13, "Get 32x32 tile x position");
    srli(x22, x22, 13, "Get 32x32 tile y position");

    init(x23, (uint64) rast_lit_tile_size_32x32);

    // 
    // f0 - f1 -> ai (broadcast)
    // f3 - f5 -> bi (broadcast)
    // f6 - f8 -> ci (broadcast)
    //
    // f29 - f31 -> pre-packet triangle information (48B)
    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    // x15 -> tile size and triangle facing for RBOX tile packets
    //
    // x1  - x3  -> ai
    // x4  - x6  -> bi
    // x17 - x18 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //
    // x23 -> rast_lit
    //

    // Check tile bound vertical dimension
    if (XREGS[x20].x > 32)
        goto triangle_not_Nx32;

    rasterize_Nx32_to_32x32();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_Nx32:

    if (XREGS[x19].x > 32)
        goto triangle_not_32xN;

    rasterize_32xN_to_32x32();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_32xN:

    if ((XREGS[x19].x != 96) && (XREGS[x20].x == 96))
        goto triangle_not_96xN;

    rasterize_96xN_to_32x32();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_96xN:

    if ((XREGS[x19].x == 128) && (XREGS[x20].x == 128))
        goto triangle_not_Nx96;

    rasterize_Nx96_to_32x32();

    print_comment("<<< create_low_prec_triangle_rbox_packets");

    return;

triangle_not_Nx96:

    rasterize_128x128_to_32x32();

    print_comment("<<< create_low_prec_triangle_rbox_packets");
}

void generate_fully_covered_tile(uint32 tile_x, uint32 tile_y, uint64 tile_c[4], uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size)
{
    print_comment(">>> generate_fully_covered_tile");

    RBOXInPcktFullyCoveredTile *tile_pckt = (RBOXInPcktFullyCoveredTile *) *rbox_input_buffer;

    tile_pckt->tile.type = RBOX_INPCKT_FULLY_COVERED_TILE;
    tile_pckt->tile.tile_left = tile_x;
    tile_pckt->tile.tile_top  = tile_y;
    tile_pckt->tile.tile_size = tile_size;
    // Reduce precision from x.25 to x.14
    for (uint32 eq = 0; eq < 3; eq++)
        tile_pckt->tile.edge[eq].e = (uint32) ((tile_c[eq] >> 11) & 0xFFFFFFFF);
    tile_pckt->tile.depth = (uint32) (tile_c[3] & 0xFFFFFFFF);

    (*rbox_input_buffer) += sizeof(RBOXInPcktFullyCoveredTile);
    (*rbox_packets)++;

    print_comment("<<< generate_fully_covered_tile");
}

void generate_large_tri_tile(uint32 tile_x, uint32 tile_y, uint64 tile_c[4], uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size)
{
    print_comment(">>> generate_large_tri_tile");

    RBOXInPcktLargeTriTile *tile_pckt = (RBOXInPcktLargeTriTile *) *rbox_input_buffer;

    tile_pckt->tile.type = RBOX_INPCKT_LARGE_TRIANGLE_TILE;
    tile_pckt->tile.tile_left = tile_x;
    tile_pckt->tile.tile_top  = tile_y;
    tile_pckt->tile.tile_size = tile_size;
    for (uint32 eq = 0; eq < 3; eq++)
        tile_pckt->tile.edge[eq].e = tile_c[eq];
    tile_pckt->tile.depth = (uint32) (tile_c[3] & 0xFFFFFFFF);

    (*rbox_input_buffer) += sizeof(RBOXInPcktLargeTriTile);
    (*rbox_packets)++;

    print_comment("<<< generate_large_tri_tile");
}


void rasterize_128x128_high_precision(uint32 tile_x, uint32 tile_y,             // Tile position (tiles)
                                      uint32 tile_dim_x, uint32 tile_dim_y,     // Tile dimension (tiles, max 4x4)
                                      uint64 tile_c_step_x[4], uint64 tile_c_step_y[4], uint64 tile_c[4],
                                      uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size)
{
    print_comment(">>> rasterize_128x128_high_precision");

    uint64 row_c[4];

    for(uint32 eq = 0; eq < 4; eq++)
        row_c[eq] = tile_c[eq];

    for(uint32 r = 0; r < tile_dim_y; r++)
    {
        uint64 tile_corner_c[4][4];

        for(uint32 eq = 0; eq < 4; eq++)
        {
            tile_corner_c[1][eq] = row_c[eq];
            tile_corner_c[3][eq] = row_c[eq] = row_c[eq] + tile_c_step_y[eq];
        }
        
        uint32 saved_tile_x = tile_x;

        bool tile_in_row = false;

        for(uint32 c = 0; c < tile_dim_x; c++)
        {
            for (uint32 eq = 0; eq < 4; eq++)
            {
                tile_corner_c[0][eq] = tile_corner_c[1][eq];
                tile_corner_c[2][eq] = tile_corner_c[3][eq];
                tile_corner_c[1][eq] = tile_corner_c[0][eq] + tile_c_step_x[eq];
                tile_corner_c[3][eq] = tile_corner_c[2][eq] + tile_c_step_x[eq];
            }

            bool trivial_discard = false;
            bool trivial_accept = true;
            for (uint32 eq = 0; (eq < 3) && !trivial_discard; eq++)
            {
                bool inside[4];
                for(uint32 c = 0; c < 4; c++)
                    inside[c] = ((tile_corner_c[c][eq] & (1ULL << 63)) == 0);

                trivial_discard = !inside[0] && !inside[1] && !inside[2] && !inside[3];
                trivial_accept = trivial_accept
                               && inside[0] && inside[1] && inside[2] && inside[3];
            }

            tile_in_row = tile_in_row || !trivial_discard;

            if (tile_in_row && trivial_discard)
                break;

            if (!trivial_discard)
            {
                if (trivial_accept)
                    generate_fully_covered_tile(tile_x, tile_y, tile_corner_c[0], rbox_input_buffer, rbox_packets, tile_size);
                else
                    generate_large_tri_tile(tile_x, tile_y, tile_corner_c[0], rbox_input_buffer, rbox_packets, tile_size);
            }

            tile_x++;
        }

        tile_x = saved_tile_x;
        tile_y++;
    }

    print_comment("<<< rasterize_128x128_high_precision");
}

void generate_large_tri(uint64 a[], uint64 b[], uint8 **rbox_input_buffer, uint32 *rbox_input_packets)
{
    print_comment(">>> generate_large_tri");

    RBOXInPcktLargeTri *large_tri_pckt = (RBOXInPcktLargeTri *) *rbox_input_buffer;

    large_tri_pckt->triangle.type = RBOX_INPCKT_LARGE_TRIANGLE;
    large_tri_pckt->triangle.tri_facing = 0;
    for (uint32 eq = 0; eq < 3; eq++)
    {
        large_tri_pckt->triangle.edge_eqs[eq].a_low  = (uint32)  (a[eq] & 0xFFFFFFFF);
        large_tri_pckt->triangle.edge_eqs[eq].b_low  = (uint32)  (b[eq] & 0xFFFFFFFF);
        large_tri_pckt->triangle.edge_eqs[eq].a_high = (uint32) ((a[eq] >> 32) & 0xFFFF);
        large_tri_pckt->triangle.edge_eqs[eq].b_high = (uint32) ((b[eq] >> 32) & 0xFFFF);
    }
    large_tri_pckt->triangle.depth_eq.a = (uint32) (a[3] & 0xFFFFFFFF);
    large_tri_pckt->triangle.depth_eq.b = (uint32) (b[3] & 0xFFFFFFFF);
    large_tri_pckt->triangle.triangle_data_ptr = 0;

    (*rbox_input_buffer) += sizeof(RBOXInPcktLargeTri);

    (*rbox_input_packets)++;

    print_comment("<<< generate_large_tri");
}

void create_high_prec_triangle_rbox_packets(TriangleSetupVector *triangle_setup_vector, uint8 *rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size)
{
    print_comment(">>> create_high_prec_triangle_rbox_packets");

    uint32 tile_dim_x;
    uint32 tile_dim_y;
    switch(tile_size)
    {
        case TILE_SIZE_64x64 : tile_dim_x = 6; tile_dim_y = 6; break;
        case TILE_SIZE_64x32 : tile_dim_x = 6; tile_dim_y = 5; break;
        case TILE_SIZE_32x32 : tile_dim_x = 5; tile_dim_y = 5; break;
        default : print_comment("Undefined tile size"); exit(-1); break;
    }

    uint8 *rbox_in_buff_ptr = rbox_input_buffer;

    for (uint32 idx = 0; idx < 4; idx++)
    {
        if ((triangle_setup_vector->high_precision_mask & (1 << (2 * idx))) != 0)
        {
            // Tile position in subpixel precision (8 bits). Align to tile size.
            uint32 tile_x = triangle_setup_vector->tile_bound_minx[idx] >> (tile_dim_x + 8);
            uint32 tile_y = triangle_setup_vector->tile_bound_miny[idx] >> (tile_dim_y + 8);
            
            // Tile dimensions in pixels.  Align to tile size;
            uint32 tri_dim_x = triangle_setup_vector->tile_bound_dimx[idx] >> tile_dim_x;
            uint32 tri_dim_y = triangle_setup_vector->tile_bound_dimy[idx] >> tile_dim_y;

            uint64 a[4];
            uint64 b[4];
            uint64 c[4];
                
            a[0] = triangle_setup_vector->a0[idx];
            a[1] = triangle_setup_vector->a1[idx];
            a[2] = triangle_setup_vector->a2[idx];
            b[0] = triangle_setup_vector->b0[idx];
            b[1] = triangle_setup_vector->b1[idx];
            b[2] = triangle_setup_vector->b2[idx];
            c[0] = triangle_setup_vector->c0[idx];
            c[1] = triangle_setup_vector->c1[idx];
            c[2] = triangle_setup_vector->c2[idx];

            generate_large_tri(a, b, &rbox_in_buff_ptr, rbox_packets);

            // Tile points
            //
            // |  |
            // 2--3--
            // |  |
            // 0--1--
            uint64 tile_c[4];
            uint64 tile_c_step_x[4];
            uint64 tile_c_step_y[4];

            for (uint32 eq = 0; eq < 4; eq++)
            {
                tile_c_step_x[eq] = a[eq] * (1 << tile_dim_x);
                tile_c_step_y[eq] = b[eq] * (1 << tile_dim_y);
            }

            // Compute tile equations at point 0
            for (uint32 eq = 0; eq < 4; eq++)
                tile_c[eq] = c[eq];

            uint32 tile_step_x = 1 << (7 - tile_dim_x);
            uint32 tile_step_y = 1 << (7 - tile_dim_y);
            uint32 tile_cols = max(1, min(tri_dim_x, tile_step_x));
            uint32 tile_rows = max(1, min(tri_dim_y, tile_step_y));
        
            rasterize_128x128_high_precision(tile_x, tile_y, tile_cols, tile_rows,
                                             tile_c_step_x, tile_c_step_y, tile_c,
                                             &rbox_in_buff_ptr, rbox_packets, tile_size);

            uint32 tile_traverse_iterations = max(max(1, tri_dim_x >> (7 - tile_dim_x)), max(1, tri_dim_y >> (7 - tile_dim_y)));

            if (tile_traverse_iterations > 1)
            {
                uint64 traverse_c_step_x[4];
                uint64 traverse_c_step_y[4];

                for(uint32 eq = 0; eq < 4; eq++)
                {
                    traverse_c_step_x[eq] = a[eq] * 128;
                    traverse_c_step_y[eq] = b[eq] * 128;
                }

                uint64 row_c[4];
                uint64 column_c[4];

                for(uint32 eq = 0; eq < 4; eq++)
                    row_c[eq] = column_c[eq] = tile_c[eq];

                uint32 row_tile_x; 
                uint32 row_tile_y = tile_y;
                uint32 col_tile_x = tile_x;
                uint32 col_tile_y;

                for(uint32 it = 1; it < tile_traverse_iterations; it++)
                {
                    uint32 tri_pending_tiles_x = tri_dim_x;
                    uint32 tri_pending_tiles_y = tri_dim_y;

                    if (it < max(1, tri_dim_y >> (7 - tile_dim_x)))
                    {
                        row_tile_x           = tile_x;
                        row_tile_y          += tile_step_y;
                        tri_pending_tiles_y -= tile_step_y;
                        for(uint32 eq = 0; eq < 4; eq++)
                            tile_c[eq] = row_c[eq] = row_c[eq] + traverse_c_step_y[eq];

                        uint32 tile_cols = max(1, min(tri_pending_tiles_x, tile_step_x));
                        uint32 tile_rows = max(1, min(tri_pending_tiles_y, tile_step_y));

                        rasterize_128x128_high_precision(row_tile_x, row_tile_y, tile_cols, tile_rows,
                                                         tile_c_step_x, tile_c_step_y, tile_c,
                                                         &rbox_in_buff_ptr, rbox_packets, tile_size);

                        for (uint32 col = 0; (col < it) && (col << max(1, tri_dim_x >> (7 - tile_dim_x))); col++)
                        {
                            row_tile_x          += tile_step_x;
                            tri_pending_tiles_x -= tile_step_x;
                            for(uint32 eq = 0; eq < 4; eq++)
                                tile_c[eq] += traverse_c_step_x[eq];

                            uint32 tile_cols = max(1, min(tri_pending_tiles_x, tile_step_x));
                            uint32 tile_rows = max(1, min(tri_pending_tiles_y, tile_step_y));

                            rasterize_128x128_high_precision(row_tile_x, row_tile_y, tile_cols, tile_rows,
                                                             tile_c_step_x, tile_c_step_y, tile_c,
                                                             &rbox_in_buff_ptr, rbox_packets, tile_size);
                        }
                    }

                    if (it < max(1, tri_dim_x >> (7 - tile_dim_x)))
                    {
                        col_tile_x          += tile_step_x;
                        col_tile_y           = tile_y;
                        tri_pending_tiles_x -= tile_step_x;
                        for(uint32 eq = 0; eq < 4; eq++)
                            tile_c[eq] = column_c[eq] = column_c[eq] + traverse_c_step_x[eq];

                        uint32 tile_cols = max(1, min(tri_pending_tiles_x, tile_step_x));
                        uint32 tile_rows = max(1, min(tri_pending_tiles_y, tile_step_y));

                        rasterize_128x128_high_precision(col_tile_x, col_tile_y, tile_cols, tile_rows,
                                                         tile_c_step_x, tile_c_step_y, tile_c,
                                                         &rbox_in_buff_ptr, rbox_packets, tile_size);
                        
                        for (uint32 row = 0; (row < it) && (row << max(1, tri_dim_y >> (7 - tile_dim_y))); row++)
                        {
                            col_tile_y          += tile_step_y;
                            tri_pending_tiles_y -= tile_step_y;
                            for(uint32 eq = 0; eq < 4; eq++)
                                tile_c[eq] += traverse_c_step_y[eq];

                            uint32 tile_cols = max(1, min(tri_pending_tiles_x, tile_step_x));
                            uint32 tile_rows = max(1, min(tri_pending_tiles_y, tile_step_y));

                            rasterize_128x128_high_precision(col_tile_x, col_tile_y, tile_cols, tile_rows,
                                                             tile_c_step_x, tile_c_step_y, tile_c,
                                                             &rbox_in_buff_ptr, rbox_packets, tile_size);
                        }
                    }
                }
            }
        }
    }

    print_comment("<<< create_high_prec_triangle_rbox_packets");
}

void create_rbox_packets(TriangleSetupVector *triangle_setup_vector, uint8 *rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size)
{
    print_comment(">>> create_rbox_packets");

    void (*create_low_prec_triangle_rbox_packets)();

    switch(tile_size)
    {
        case TILE_SIZE_64x64 : create_low_prec_triangle_rbox_packets = create_low_prec_triangle_rbox_packets_tile_size_64x64; break;
        case TILE_SIZE_32x32 : create_low_prec_triangle_rbox_packets = create_low_prec_triangle_rbox_packets_tile_size_32x32; break;
    }

    init(x10, (uint64) triangle_setup_vector);
    init(x11, (uint64) rbox_input_buffer);
   
    addi(x12, x0, 0, "Set triangle index to 0");
    addi(x13, x0, 0, "Set RBOX packet counter to 0");

    lwu(x14, 388, x10, "Load low precision setup triangle mask");

    init(x15, (uint64) (tile_size << 1));

    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    // x15 -> tile size and triangle facing for RBOX tile packets
    //

    andi(x5, x14, 0x01, "Check first triangle");
    if (XREGS[x5].x > 0)
        create_low_prec_triangle_rbox_packets();

    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x14].x == 0)
        goto high_precision_setup_triangles;

    addi(x12, x12, 1, "Set triangle index to 1");

    andi(x5, x14, 0x01, "Check second triangle");
    if (XREGS[x5].x > 0)
        create_low_prec_triangle_rbox_packets();
 
    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x4].x == 0)
        goto high_precision_setup_triangles;

    addi(x12, x12, 1, "Set triangle index to 2");

    andi(x5, x14, 0x01, "Check third triangle");
    if (XREGS[x5].x > 0)
        create_low_prec_triangle_rbox_packets();
 
    addi(x12, x12, 1, "Set triangle index to 3");

    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x14].x == 0)
        goto high_precision_setup_triangles;

    andi(x5, x14, 0x01, "Check fourth triangle");
    if (XREGS[x5].x > 0)
        create_low_prec_triangle_rbox_packets();
 
high_precision_setup_triangles:

    *rbox_packets = XREGS[x13].x;

    if (triangle_setup_vector->high_precision_mask != 0)
    {
        uint8 *rbox_in_buff_ptr = (uint8 *) XREGS[x11].x;
        create_high_prec_triangle_rbox_packets(triangle_setup_vector, rbox_in_buff_ptr, rbox_packets, tile_size);
    }
    print_comment("<<< create_rbox_packets");
}

