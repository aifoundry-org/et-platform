

// Collection of old implementations to use as reference for future developments.
//


//
// The *_base functions don't use the maskpopc_rast instruction.
//
#define EVALUATE_TILE_EQ(FSMP_BOTTOM, FSMP_TOP, FTMP, XTMP, SWIZZLE1, SWIZZLE2, LABEL, EQ, P1, P2, P3, P4) \
    mov_m_x(m0, x0, 0x05, "Set mask to (1, 1, 0, 0)"); \
    fswizz_ps(FTMP, FSMP_BOTTOM, SWIZZLE1, "Pack test result of (e" #EQ " < 0) for points (" #P1 ", " #P2 ", x, x)"); \
    mov_m_x(m0, x0, 0x50, "Set mask to (0, 0, 1, 1)"); \
    fswizz_ps(FTMP, FSMP_TOP, SWIZZLE2,  "Pack tests result of (e" #EQ " < 0) for tile (" #P1 ", " #P2 ",  " #P3 ", " #P4 ")"); \
    mov_m_x(m0, x0, 0x55, "Set mask to (1, 1, 1, 1)"); \
    fsetm_pi(m1, FTMP, "Store in mask registers tests result of (e" #EQ " < 0) for tile (" #P1 ", " #P2 ",  " #P3 ", " #P4 ")"); \
    maskpopc(XTMP, m1, "Count number of points passing (e" #EQ " < 0)"); \
    if (XREGS[XTMP].x == 8) \
        goto LABEL;

#define EVALUATE_TILE(FSMP_BOTTOM1, FSMP_BOTTOM2, FSMP_BOTTOM3, \
                      FSMP_TOP1, FSMP_TOP2, FSMP_TOP3, \
                      FTMP, XTMP, SWIZZLE1, SWIZZLE2, LABEL, P1, P2, P3, P4) \
    EVALUATE_TILE_EQ(FSMP_BOTTOM1, FSMP_TOP1, FTMP, XTMP, SWIZZLE1, SWIZZLE2, LABEL, 0, P1, P2, P3, P4) \
    EVALUATE_TILE_EQ(FSMP_BOTTOM2, FSMP_TOP2, FTMP, XTMP, SWIZZLE1, SWIZZLE2, LABEL, 1, P1, P2, P3, P4) \
    EVALUATE_TILE_EQ(FSMP_BOTTOM3, FSMP_TOP3, FTMP, XTMP, SWIZZLE1, SWIZZLE2, LABEL, 2, P1, P2, P3, P4)


void rasterize_Nx32_to_32x32_base()
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
    flt_pi(f20, f6, f13, "Test (e0 < 0) for points (0, 1, 2, 3)");
    flt_pi(f21, f7, f13, "Test (e1 < 0) for points (0, 1, 2, 3)");
    flt_pi(f22, f8, f13, "Test (e2 < 0) for points (0, 1, 2, 3)");

    flt_pi(f23, f10, f13, "Test (e0 < 0) for points (4, 5, 6, 7)");
    flt_pi(f24, f11, f13, "Test (e1 < 0) for points (4, 5, 6, 7)");
    flt_pi(f25, f12, f13, "Test (e2 < 0) for points (4, 5, 6, 7)");

    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x04, 0x40, tile1_Nx32, 0, 1, 4, 5)

    generate_triangle_with_tile_128x128_packet();

tile1_Nx32:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x09, 0x90, tile2_Nx32, 1, 2, 5, 6)

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
 
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x0e, 0xe0, tile3_Nx32, 2, 3, 6, 7)
   

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
    flt_pi(f3, f3, f16, "Test (e0 < 0) for points (3, 8, 7, 9)");
    flt_pi(f4, f4, f16, "Test (e1 < 0) for points (3, 8, 7, 9)");
    flt_pi(f5, f5, f16, "Test (e2 < 0) for points (3, 8, 7, 9)");

    fsetm_pi(m1, f3, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_Nx32;

    fsetm_pi(m1, f4, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_Nx32;

    fsetm_pi(m1, f5, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

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

void rasterize_32xN_to_32x32_base()
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
    flt_pi(f20, f6, f13, "Test (e0 < 0) for points (0, 1, 2, 3)");
    flt_pi(f21, f7, f13, "Test (e1 < 0) for points (0, 1, 2, 3)");
    flt_pi(f22, f8, f13, "Test (e2 < 0) for points (0, 1, 2, 3)");

    flt_pi(f23, f10, f13, "Test (e0 < 0) for points (4, 5, 6, 7)");
    flt_pi(f24, f11, f13, "Test (e1 < 0) for points (4, 5, 6, 7)");
    flt_pi(f25, f12, f13, "Test (e2 < 0) for points (4, 5, 6, 7)");

    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x04, 0x40, tile1_32xN, 0, 1, 4, 5)

    generate_triangle_with_tile_128x128_packet();

tile1_32xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x09, 0x90, tile2_32xN, 1, 2, 5, 6)

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
    
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x09, 0x90, tile3_32xN, 2, 3, 6, 7)

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
    flt_pi(f3, f3, f16, "Test (e0 < 0) for points (3, 8, 7, 9)");
    flt_pi(f4, f4, f16, "Test (e1 < 0) for points (3, 8, 7, 9)");
    flt_pi(f5, f5, f16, "Test (e2 < 0) for points (3, 8, 7, 9)");

    fsetm_pi(m1, f3, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_32xN;

    fsetm_pi(m1, f4, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_32xN;

    fsetm_pi(m1, f5, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

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


void rasterize_96xN_to_32x32_base()
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
    flt_pi(f9,  f0, f18, "Test (e0 < 0) for points (0, 1, 2, 3)");
    flt_pi(f10, f1, f18, "Test (e1 < 0) for points (0, 1, 2, 3)");
    flt_pi(f11, f2, f18, "Test (e2 < 0) for points (0, 1, 2, 3)");

    flt_pi(f12, f3, f18, "Test (e0 < 0) for points (4, 5, 6, 7)");
    flt_pi(f13, f4, f18, "Test (e1 < 0) for points (4, 5, 6, 7)");
    flt_pi(f14, f5, f18, "Test (e2 < 0) for points (4, 5, 6, 7)");

    flt_pi(f15, f6, f18, "Test (e0 < 0) for points (8, 9, 10, 11)");
    flt_pi(f16, f7, f18, "Test (e1 < 0) for points (8, 9, 10, 11)");
    flt_pi(f17, f8, f18, "Test (e2 < 0) for points (8, 9, 10, 11)");

    addi(x25, x21, 0, "Save tile x position for second row");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x04, 0x40, tile1_96xN, 0, 1, 4, 5)

    generate_triangle_with_tile_128x128_packet();

tile1_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x09, 0x90, tile2_96xN, 1, 2, 5, 6)

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
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x0e, 0xe0, tile3_96xN, 2, 3, 6, 7)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_96xN:

    addi(x21, x25, 0, "Restore tile x coordinate for second row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x04, 0x40, tile4_96xN, 4, 5, 8, 9)

    // Get second tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

tile4_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x09, 0x90, tile5_96xN, 5, 6, 9, 10)

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

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x0e, 0xe0, tile6_96xN, 6, 7, 10, 11)

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
    
    flt_pi(f9,  f0, f18, "Test (e0 < 0) for points (12, 13, 14, 15)");
    flt_pi(f10, f1, f18, "Test (e1 < 0) for points (12, 13, 14, 15)");
    flt_pi(f11, f2, f18, "Test (e2 < 0) for points (12, 13, 14, 15)");

    addi(x21, x25, 0, "Restore tile x coordinate for third row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x04, 0x40, tile7_96xN, 8, 9, 12, 13)

    // Get second tile initial sample position from point 8
    fmvs_x_ps(x16, f6, 0, "Copy e0 at point 8");
    fmvs_x_ps(x17, f7, 0, "Copy e1 at point 8");
    fmvs_x_ps(x18, f8, 0, "Copy e2 at point 8");
    generate_triangle_with_tile_128x128_packet();

tile7_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x09, 0x90, tile8_96xN, 9, 10, 13, 14)

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

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x0e, 0xe0, tile9_96xN, 10, 11, 14, 15)

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
    
    flt_pi(f12, f3, f18, "Test (e0 < 0) for points (16, 17, 18, 19)");
    flt_pi(f13, f4, f18, "Test (e1 < 0) for points (16, 17, 18, 19)");
    flt_pi(f14, f5, f18, "Test (e2 < 0) for points (16, 17, 18, 19)");

    addi(x21, x25, 0, "Restore tile x coordinate for fourth row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x04, 0x40, tile10_96xN, 12, 13, 16, 17)

    // Get second tile initial sample position from point 12
    fmvs_x_ps(x16, f0, 0, "Copy e0 at point 12");
    fmvs_x_ps(x17, f1, 0, "Copy e1 at point 12");
    fmvs_x_ps(x18, f2, 0, "Copy e2 at point 12");
    generate_triangle_with_tile_128x128_packet();

tile10_96xN:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x09, 0x90, tile11_96xN, 13, 14, 17, 18)

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

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x0e, 0xe0, end_96xN, 14, 15, 18, 19)

    // Get second tile initial sample position from point 14
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 14");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 14");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 14");
    generate_triangle_with_tile_128x128_packet();

end_96xN:

    print_comment("<<< rasterize_96xN");

    return;
}

void rasterize_Nx96_to_32x32_base()
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
    flt_pi(f9,  f0, f18, "Test (e0 < 0) for points (0, 1, 2, 3)");
    flt_pi(f10, f1, f18, "Test (e1 < 0) for points (0, 1, 2, 3)");
    flt_pi(f11, f2, f18, "Test (e2 < 0) for points (0, 1, 2, 3)");

    flt_pi(f12, f3, f18, "Test (e0 < 0) for points (4, 5, 6, 7)");
    flt_pi(f13, f4, f18, "Test (e1 < 0) for points (4, 5, 6, 7)");
    flt_pi(f14, f5, f18, "Test (e2 < 0) for points (4, 5, 6, 7)");

    flt_pi(f15, f6, f18, "Test (e0 < 0) for points (8, 9, 10, 11)");
    flt_pi(f16, f7, f18, "Test (e1 < 0) for points (8, 9, 10, 11)");
    flt_pi(f17, f8, f18, "Test (e2 < 0) for points (8, 9, 10, 11)");

    addi(x25, x22, 0, "Save tile y position for second column");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x04, 0x40, tile1_Nx96, 0, 1, 4, 5)

    generate_triangle_with_tile_128x128_packet();

tile1_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x09, 0x90, tile2_Nx96, 1, 2, 5, 6)

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
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x0e, 0xe0, tile3_Nx96, 2, 3, 6, 7)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_Nx96:

    addi(x22, x25, 0, "Restore tile y coordinate for second column");
    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x04, 0x40, tile4_Nx96, 4, 5, 8, 9)

    // Get second tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

tile4_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x09, 0x90, tile5_Nx96, 5, 6, 9, 10)

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

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f22, x24, 0x0e, 0xe0, tile6_Nx96, 6, 7, 10, 11)

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
    
    flt_pi(f9,  f0, f18, "Test (e0 < 0) for points (12, 13, 14, 15)");
    flt_pi(f10, f1, f18, "Test (e1 < 0) for points (12, 13, 14, 15)");
    flt_pi(f11, f2, f18, "Test (e2 < 0) for points (12, 13, 14, 15)");

    addi(x22, x25, 0, "Restore tile y coordinate for third column");
    addi(x21, x21, 1, "Advance tile x coordinate one position");

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x04, 0x40, tile7_Nx96, 8, 9, 12, 13)

    // Get second tile initial sample position from point 8
    fmvs_x_ps(x16, f6, 0, "Copy e0 at point 8");
    fmvs_x_ps(x17, f7, 0, "Copy e1 at point 8");
    fmvs_x_ps(x18, f8, 0, "Copy e2 at point 8");
    generate_triangle_with_tile_128x128_packet();

tile7_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x09, 0x90, tile8_Nx96, 9, 10, 13, 14)

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

    EVALUATE_TILE(f15, f16, f17, f9, f10, f11, f22, x24, 0x0e, 0xe0, tile9_Nx96, 10, 11, 14, 15)

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
    
    flt_pi(f12, f3, f18, "Test (e0 < 0) for points (16, 17, 18, 19)");
    flt_pi(f13, f4, f18, "Test (e1 < 0) for points (16, 17, 18, 19)");
    flt_pi(f14, f5, f18, "Test (e2 < 0) for points (16, 17, 18, 19)");

    addi(x21, x25, 0, "Restore tile x coordinate for second row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x04, 0x40, tile10_Nx96, 12, 13, 16, 17)

    // Get second tile initial sample position from point 12
    fmvs_x_ps(x16, f0, 0, "Copy e0 at point 12");
    fmvs_x_ps(x17, f1, 0, "Copy e1 at point 12");
    fmvs_x_ps(x18, f2, 0, "Copy e2 at point 12");
    generate_triangle_with_tile_128x128_packet();

tile10_Nx96:

    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x09, 0x90, tile11_Nx96, 13, 14, 17, 18)

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

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f22, x24, 0x0e, 0xe0, end_Nx96, 14, 15, 18, 19)

    // Get second tile initial sample position from point 14
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 14");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 14");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 14");
    generate_triangle_with_tile_128x128_packet();

end_Nx96:

    print_comment("<<< rasterize_Nx96");

    return;
}

void rasterize_128x128_to_32x32_base()
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

    flt_pi(f9,  f0, f24, "Test (e0 < 0) for points (0, 1, 2, 3)");
    flt_pi(f10, f1, f24, "Test (e1 < 0) for points (0, 1, 2, 3)");
    flt_pi(f11, f2, f24, "Test (e2 < 0) for points (0, 1, 2, 3)");

    flt_pi(f12, f3, f24, "Test (e0 < 0) for points (5, 6, 7, 8)");
    flt_pi(f13, f4, f24, "Test (e1 < 0) for points (5, 6, 7, 8)");
    flt_pi(f14, f5, f24, "Test (e2 < 0) for points (5, 6, 7, 8)");

    flt_pi(f15, f6, f24, "Test (e0 < 0) for points (3, 4, 8, 9)");
    flt_pi(f16, f7, f24, "Test (e1 < 0) for points (3, 4, 8, 9)");
    flt_pi(f17, f8, f24, "Test (e2 < 0) for points (3, 4, 8, 9)");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f25, x24, 0x04, 0x40, tile1_128x128, 0, 1, 5, 6)

    generate_triangle_with_tile_128x128_packet();

tile1_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f25, x24, 0x09, 0x90, tile2_128x128, 1, 2, 6, 7)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
 
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f25, x24, 0x0e, 0xe0, tile3_128x128, 2, 3, 7, 8)

    // Get third tile initial sample position from point 2
    fmvs_x_ps(x16, f0, 2, "Copy e0 at point 2");
    fmvs_x_ps(x17, f1, 2, "Copy e1 at point 2");
    fmvs_x_ps(x18, f2, 2, "Copy e2 at point 2");
    generate_triangle_with_tile_128x128_packet();

tile3_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");

    fsetm_pi(m1, f15, "Store in mask registers tests result of (e0 < 0) for first tile (3, 4, 8, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 0
    if (XREGS[x24].x == 8)
        goto end_row_128x128;

    fsetm_pi(m1, f16, "Store in mask registers tests result of (e0 < 0) for first tile (3, 4, 8, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

    // Trivially discard tile if all four tile corners are in the negative side of edge equation 1
    if (XREGS[x24].x == 8)
        goto end_row_128x128;

    fsetm_pi(m1, f17, "Store in mask registers tests result of (e0 < 0) for first tile (3, 8, 7, 9)");
    maskpopc(x24, m1, "Count number of points passing (e0 < 0)");

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

void rasterize_128x64_to_64x64_base()
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
    //  3--4--5
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
    flt_pi(f20, f6, f13, "Test (e0 < 0) for points (0, 1, 2, x)");
    flt_pi(f21, f7, f13, "Test (e1 < 0) for points (0, 1, 2, x)");
    flt_pi(f22, f8, f13, "Test (e2 < 0) for points (0, 1, 2, x)");

    flt_pi(f23, f10, f13, "Test (e0 < 0) for points (3, 4, 5, x)");
    flt_pi(f24, f11, f13, "Test (e1 < 0) for points (3, 4, 5, x)");
    flt_pi(f25, f12, f13, "Test (e2 < 0) for points (3, 4, 5, x)");

    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x04, 0x40, tile1_128x64, 0, 1, 3, 4)

    generate_triangle_with_tile_128x128_packet();

tile1_128x64:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x09, 0x90, end_128x64, 1, 2, 4, 5)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

end_128x64:

    print_comment("<<< rasterize_128x64");

    return;
}

void rasterize_64x128_to_64x64_base()
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
    flt_pi(f20, f6, f13, "Test (e0 < 0) for points (0, 1, 2, x)");
    flt_pi(f21, f7, f13, "Test (e1 < 0) for points (0, 1, 2, x)");
    flt_pi(f22, f8, f13, "Test (e2 < 0) for points (0, 1, 2, x)");

    flt_pi(f23, f10, f13, "Test (e0 < 0) for points (3, 4, 5, x)");
    flt_pi(f24, f11, f13, "Test (e1 < 0) for points (3, 4, 5, x)");
    flt_pi(f25, f12, f13, "Test (e2 < 0) for points (3, 4, 5, x)");

    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x04, 0x40, tile1_64x128, 0, 1, 3, 4)

    generate_triangle_with_tile_128x128_packet();

tile1_64x128:

    addi(x22, x22, 1, "Advance tile y coordinate one position");
    
    EVALUATE_TILE(f20, f21, f22, f23, f24, f25, f13, x24, 0x09, 0x90, end_64x128, 1, 2, 4, 5)

    // Get second tile initial sample position from point 2
    fmvs_x_ps(x16, f6, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f7, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f8, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

end_64x128:

    print_comment("<<< rasterize_64x128");

    return;
}

void rasterize_128x128_to_64x64_base()
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

    fadd_pi(f3, f0, f21, "Sample e0 at points (3, 4, 5, *)");
    fadd_pi(f4, f1, f22, "Sample e1 at points (3, 4, 5, *)");
    fadd_pi(f5, f2, f23, "Sample e2 at points (3, 4, 5, *)");

    fadd_pi(f6, f3, f21, "Sample e0 at points (3, 4, 5, *)");
    fadd_pi(f7, f4, f22, "Sample e1 at points (3, 4, 5, *)");
    fadd_pi(f8, f5, f23, "Sample e2 at points (3, 4, 5, *)");

    flt_pi(f9,  f0, f24, "Test (e0 < 0) for points (0, 1, 2, x)");
    flt_pi(f10, f1, f24, "Test (e1 < 0) for points (0, 1, 2, x)");
    flt_pi(f11, f2, f24, "Test (e2 < 0) for points (0, 1, 2, x)");

    flt_pi(f12, f3, f24, "Test (e0 < 0) for points (3, 4, 5, x)");
    flt_pi(f13, f4, f24, "Test (e1 < 0) for points (3, 4, 5, x)");
    flt_pi(f14, f5, f24, "Test (e2 < 0) for points (3, 4, 5, x)");

    flt_pi(f15, f6, f24, "Test (e0 < 0) for points (6, 7, 8, x)");
    flt_pi(f16, f7, f24, "Test (e1 < 0) for points (6, 7, 8, x)");
    flt_pi(f17, f8, f24, "Test (e2 < 0) for points (6, 7, 8, x)");

    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f25, x24, 0x04, 0x40, tile1_128x128, 0, 1, 3, 4)

    generate_triangle_with_tile_128x128_packet();

tile1_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f9, f10, f11, f12, f13, f14, f25, x24, 0x09, 0x90, tile2_128x128, 1, 2, 4, 5)

    // Get second tile initial sample position from point 1
    fmvs_x_ps(x16, f0, 1, "Copy e0 at point 1");
    fmvs_x_ps(x17, f1, 1, "Copy e1 at point 1");
    fmvs_x_ps(x18, f2, 1, "Copy e2 at point 1");
    generate_triangle_with_tile_128x128_packet();

tile2_128x128:

    addi(x26, x26, 1, "Update tile row");
    addi(x21, x25, 0, "Restore tile x for next row");
    addi(x22, x22, 1, "Advance tile y coordinate one position");

    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f25, x24, 0x04, 0x40, tile3_128x128, 3, 4, 6, 7)

    // Get third tile initial sample position from point 3
    fmvs_x_ps(x16, f3, 0, "Copy e0 at point 3");
    fmvs_x_ps(x17, f4, 0, "Copy e1 at point 3");
    fmvs_x_ps(x18, f5, 0, "Copy e2 at point 3");
    generate_triangle_with_tile_128x128_packet();

tile3_128x128:

    addi(x21, x21, 1, "Advance tile x coordinate one position");
    
    EVALUATE_TILE(f12, f13, f14, f15, f16, f17, f25, x24, 0x09, 0x90, end_128x128, 4, 5, 7, 8)

    // Get fourth tile initial sample position from point 4
    fmvs_x_ps(x16, f3, 1, "Copy e0 at point 4");
    fmvs_x_ps(x17, f4, 1, "Copy e1 at point 4");
    fmvs_x_ps(x18, f5, 1, "Copy e2 at point 4");
    generate_triangle_with_tile_128x128_packet();

end_128x128:

    print_comment("<<< rasterize_128x128");

    return;
}

#undef EVALUATE_TILE_EQ
#undef EVALUATE_TILE

#define EVALUATE_TILE_EQ(SMP_LEFT, SMP_RIGHT, EQ, LABEL) \
    maskpopc_rast(x29, SMP_LEFT, SMP_RIGHT, 3, "Count number of points (e " #EQ " < 0)"); \
    if (XREGS[x29].x == 8) \
        goto LABEL; \
    add(x23, x29, x23, "Update the counter used to check for fully covered tiles");

#define EVALUATE_TILE(SMP_E0_LEFT,  SMP_E1_LEFT,  SMP_E2_LEFT, \
                      SMP_E0_RIGHT, SMP_E1_RIGHT, SMP_E2_RIGHT, \
                      LABEL) \
    addi(x23, x0, 0, "Initialize counter used to check for fully covered tiles"); \
    EVALUATE_TILE_EQ(SMP_E0_LEFT, SMP_E0_RIGHT, 0, LABEL) \
    EVALUATE_TILE_EQ(SMP_E1_LEFT, SMP_E1_RIGHT, 1, LABEL) \
    EVALUATE_TILE_EQ(SMP_E2_LEFT, SMP_E2_RIGHT, 2, LABEL)


//  Old version using 64-bit vector instructions.
//  Use the scalar version in C instead.
void rasterize_128x128_to_64x64_high_precision()
{
    uint64_t saved_registers[32];
    
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    //
    // x1  - x3 -> ai * 64
    // x4  - x6 -> bi * 64
    // x7  - x9 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //
    
    //  128x128
    //
    //  6--7--8
    //  |  |  |
    //  3--4--5
    //  |  |  |
    //  0--1--2

    print_comment(">>> rasterize_128x128_high_precision");

    init(x30, (uint64_t) saved_registers);
    init(x31, (uint64_t) rast_lit);

    sd(x19, 19 * 8, x30, "Save register x19");
    sd(x20, 20 * 8, x30, "Save register x20");
    sd(x21, 21 * 8, x30, "Save register x21");
    sd(x22, 22 * 8, x30, "Save register x22");
    sd(x23, 23 * 8, x30, "Save register x23");
    sd(x24, 24 * 8, x30, "Save register x24");
    sd(x25, 25 * 8, x30, "Save register x25");
    sd(x26, 26 * 8, x30, "Save register x26");
    sd(x27, 27 * 8, x30, "Save register x27");
    sd(x28, 28 * 8, x30, "Save register x28");
    sd(x29, 29 * 8, x30, "Save register x29");

    slti(x23, x19, 128, "Min remaining triangle BB area to rasterize in the horizontal dimension and 128");
    slti(x24, x20, 128, "Min remaining triangle BB area to rasterize in the horizontal dimension and 128");
    sub(x23, x0, x23, "Convert 1 -> 0xFFFFFFFF");
    sub(x24, x0, x24, "Convert 1 -> 0xFFFFFFFF");
    xori(x25, x23, -1, "Invert mask");
    xori(x26, x24, -1, "Invert mask");
    and_(x19, x23, x19, "min(a, b) -> a < b");
    andi(x25, x25, 128, "min(a, b) -> a >= b");
    or_(x19, x19, x25, "min(a, b)");
    and_(x20, x24, x19, "min(a, b) -> a < b");
    andi(x26, x26, 128, "min(a, b) -> a >= b");
    or_(x20, x20, x26, "min(a, b)");

    add(x24, x0, x19, "Save horizontal tile dimension");
    add(x25, x0, x21, "Save horizontal tile position");

    add(x26, x7, x4, "Sample e0 at point 3");
    add(x27, x8, x5, "Sample e1 at point 3");
    add(x28, x9, x6, "Sample e2 at point 3");

    fbcx_pq(f1, x1, "Load (a0, a0)");
    fbcx_pq(f2, x2, "Load (a1, a1)");
    fbcx_pq(f3, x3, "Load (a2, a2)");

    fbcx_pq(f4, x4, "Load (b0, b0)");
    fbcx_pq(f5, x5, "Load (b1, b1)");
    fbcx_pq(f6, x6, "Load (b2, b2)");

    mov_m_x(m0, x0, 0x01, "Load mask to set only the lower 64-bit of the register");
    fbcx_pq(f7, x7, "Load e0 at points (0, *)");
    fbcx_pq(f8, x8, "Load e1 at points (0, *)");
    fbcx_pq(f9, x9, "Load e2 at points (0, *)");

    mov_m_x(m0, x0, 0x10, "Load mask to set only the lower 64-bit of the register");
    fbcx_pq(f7, x26, "Load e0 at points (0, 3)");
    fbcx_pq(f8, x27, "Load e1 at points (0, 3)");
    fbcx_pq(f9, x28, "Load e2 at points (0, 3)");

    mov_m_x(m0, x0, 0xff, "Restore mask to all components");

    or_(x26, x0, x7, "Copy e0 for point 0");
    or_(x27, x0, x8, "Copy e1 for point 0");
    or_(x28, x0, x9, "Copy e2 for point 0");

tile0_128x128:

    fadd_pq(f10, f7, f1, "Sample e0 at points (1, 4)");
    fadd_pq(f11, f8, f2, "Sample e1 at points (1, 4)");
    fadd_pq(f12, f9, f3, "Sample e2 at points (1, 4)");
    
    fltm_pi(m1, f7, f0, "Evaluate (e0 < 0) for points (0, 3)");
    fltm_pi(m2, f8, f0, "Evaluate (e1 < 0) for points (0, 3)");
    fltm_pi(m3, f9, f0, "Evaluate (e1 < 0) for points (0, 3)");

    fltm_pi(m4, f10, f0, "Evaluate (e0 < 0) for points (1, 4)");
    fltm_pi(m5, f11, f0, "Evaluate (e1 < 0) for points (1, 4)");
    fltm_pi(m6, f12, f0, "Evaluate (e1 < 0) for points (1, 4)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, tile1_128x128)

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 64x64 tile position (pixels)
    // x26 - x28 -> e0 - e2
 
    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

tile1_128x128:

    if (XREGS[x19].x <= 64)
        goto tile2_128x128;

    addi(x19, x19, -64, "One less 64x64 tile to rasterize");
    addi(x21, x21, 1, "Advance one 64x64 tile in the horizontal direction");

    fadd_pq(f13, f10, f1, "Sample e0 at points (2, 5)");
    fadd_pq(f14, f11, f2, "Sample e1 at points (2, 5)");
    fadd_pq(f15, f12, f3, "Sample e2 at points (2, 5)");
    
    fltm_pi(m1, f13, f0, "Evaluate (e0 < 0) for points (2, 5)");
    fltm_pi(m2, f14, f0, "Evaluate (e1 < 0) for points (2, 5)");
    fltm_pi(m3, f15, f0, "Evaluate (e1 < 0) for points (2, 5)");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, tile2_128x128)

    fmv_x_d(x26, f10, "Copy e0 at point 1");
    fmv_x_d(x27, f11, "Copy e1 at point 1");
    fmv_x_d(x28, f12, "Copy e2 at point 1");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 64x64 tile position (pixels)
    // x26 - x28 -> e0 - e2

    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

tile2_128x128:

    if (XREGS[x20].x <= 64)
        goto end_scalar_128x128;

    add(x19, x0, x24, "Restore horizontal tile dimension");
    add(x21, x0, x25, "Restore horizontal tile position");

    addi(x20, x20, -64, "One less 64x64 tile row to rasterize");
    addi(x22, x22, 1, "Advance one 64x64 tile in the vertical direction");

    fadd_pq(f7, f7, f4, "Sample e0 at (3, 6)");
    fadd_pq(f8, f8, f5, "Sample e1 at (3, 6)");
    fadd_pq(f9, f9, f6, "Sample e2 at (3, 6)");

    fmv_x_d(x26, f7, "Copy e0 at point 3");
    fmv_x_d(x27, f8, "Copy e1 at point 3");
    fmv_x_d(x28, f9, "Copy e2 at point 3");

    fadd_pq(f10, f7, f1, "Sample e0 at points (4, 7)");
    fadd_pq(f11, f8, f2, "Sample e1 at points (4, 7)");
    fadd_pq(f12, f9, f3, "Sample e2 at points (4, 7)");
    
    fltm_pi(m1, f7, f0, "Evaluate (e0 < 0) for points (3, 6)");
    fltm_pi(m2, f8, f0, "Evaluate (e1 < 0) for points (3, 6)");
    fltm_pi(m3, f9, f0, "Evaluate (e1 < 0) for points (3, 6)");

    fltm_pi(m4, f10, f0, "Evaluate (e0 < 0) for points (4, 7)");
    fltm_pi(m5, f11, f0, "Evaluate (e1 < 0) for points (4, 7)");
    fltm_pi(m6, f12, f0, "Evaluate (e1 < 0) for points (4, 7)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, tile3_128x128)

    fmv_x_d(x26, f7, "Copy e0 at point 3");
    fmv_x_d(x27, f8, "Copy e1 at point 3");
    fmv_x_d(x28, f9, "Copy e2 at point 3");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 64x64 tile position (pixels)
    // x26 - x28 -> e0 - e2
 
    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

tile3_128x128:

    if (XREGS[x19].x <= 64)
        goto end_scalar_128x128;

    addi(x19, x19, -64, "One less 64x64 tile to rasterize");
    addi(x21, x21, 1, "Advance one 64x64 tile in the horizontal direction");

    fadd_pq(f13, f10, f1, "Sample e0 at points (5, 8)");
    fadd_pq(f14, f11, f2, "Sample e1 at points (5, 8)");
    fadd_pq(f15, f12, f3, "Sample e2 at points (5, 8)");
    
    fltm_pi(m1, f13, f0, "Evaluate (e0 < 0) for points (5, 8)");
    fltm_pi(m2, f14, f0, "Evaluate (e1 < 0) for points (5, 8)");
    fltm_pi(m3, f15, f0, "Evaluate (e1 < 0) for points (5, 8)");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, end_scalar_128x128)

    fmv_x_d(x26, f10, "Copy e0 at point 4");
    fmv_x_d(x27, f11, "Copy e1 at point 4");
    fmv_x_d(x28, f12, "Copy e2 at point 4");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 64x64 tile position (pixels)
    // x26 - x28 -> e0 - e2

    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

end_scalar_128x128:

    ld(x19, 19 * 8, x30, "Restore register x19");
    ld(x20, 20 * 8, x30, "Restore register x20");
    ld(x21, 21 * 8, x30, "Restore register x21");
    ld(x22, 22 * 8, x30, "Restore register x22");
    ld(x23, 23 * 8, x30, "Restore register x23");
    ld(x24, 24 * 8, x30, "Restore register x24");
    ld(x25, 25 * 8, x30, "Restore register x25");
    ld(x26, 26 * 8, x30, "Restore register x26");
    ld(x27, 27 * 8, x30, "Restore register x27");
    ld(x28, 28 * 8, x30, "Restore register x28");
    ld(x29, 29 * 8, x30, "Restore register x29");

    print_comment("<<< rasterize_128x128_high_precision");

    return;
}


void rasterize_128x128_to_32x32_high_precision()
{
    uint64_t saved_registers[32];
    
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    //
    // x1  - x3 -> ai * 32
    // x4  - x6 -> bi * 32
    // x7  - x9 -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
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

    print_comment(">>> rasterize_128x128_high_precision");

    init(x30, (uint64_t) saved_registers);
    init(x31, (uint64_t) rast_lit);

    sd(x19, 19 * 8, x30, "Save register x19");
    sd(x20, 20 * 8, x30, "Save register x20");
    sd(x21, 21 * 8, x30, "Save register x21");
    sd(x22, 22 * 8, x30, "Save register x22");
    sd(x23, 23 * 8, x30, "Save register x23");
    sd(x24, 24 * 8, x30, "Save register x24");
    sd(x25, 25 * 8, x30, "Save register x25");
    sd(x26, 26 * 8, x30, "Save register x26");
    sd(x27, 27 * 8, x30, "Save register x27");
    sd(x28, 28 * 8, x30, "Save register x28");
    sd(x29, 29 * 8, x30, "Save register x29");

    flw_ps(f0, 16, x31, "Load values for edge test (0, -1, 0, -1)");

    slti(x23, x19, 128, "Min remaining triangle BB area to rasterize in the horizontal dimension and 128");
    slti(x24, x20, 128, "Min remaining triangle BB area to rasterize in the horizontal dimension and 128");
    sub(x23, x0, x23, "Convert 1 -> 0xFFFFFFFF");
    sub(x24, x0, x24, "Convert 1 -> 0xFFFFFFFF");
    xori(x25, x23, -1, "Invert mask");
    xori(x26, x24, -1, "Invert mask");
    and_(x19, x23, x19, "min(a, b) -> a < b");
    andi(x25, x25, 128, "min(a, b) -> a >= b");
    or_(x19, x19, x25, "min(a, b)");
    and_(x20, x24, x19, "min(a, b) -> a < b");
    andi(x26, x26, 128, "min(a, b) -> a >= b");
    or_(x20, x20, x26, "min(a, b)");

    add(x24, x0, x19, "Save horizontal tile dimension");
    add(x25, x0, x21, "Save horizontal tile position");

    add(x26, x7, x4, "Sample e0 at point 5");
    add(x27, x8, x5, "Sample e1 at point 5");
    add(x28, x9, x6, "Sample e2 at point 5");

    fbcx_pq(f1, x1, "Load (a0, a0)");
    fbcx_pq(f2, x2, "Load (a1, a1)");
    fbcx_pq(f3, x3, "Load (a2, a2)");

    fbcx_pq(f4, x4, "Load (b0, b0)");
    fbcx_pq(f5, x5, "Load (b1, b1)");
    fbcx_pq(f6, x6, "Load (b2, b2)");

    mov_m_x(m0, x0, 0x01, "Load mask to set only the lower 64-bit of the register");
    fbcx_pq(f7, x7, "Load e0 at points (0, *)");
    fbcx_pq(f8, x8, "Load e1 at points (0, *)");
    fbcx_pq(f9, x9, "Load e2 at points (0, *)");

    mov_m_x(m0, x0, 0x10, "Load mask to set only the lower 64-bit of the register");
    fbcx_pq(f7, x26, "Load e0 at points (0, 5)");
    fbcx_pq(f8, x27, "Load e1 at points (0, 5)");
    fbcx_pq(f9, x28, "Load e2 at points (0, 5)");

    mov_m_x(m0, x0, 0xff, "Restore mask to all components");

    or_(x26, x0, x7, "Copy e0 for point 0");
    or_(x27, x0, x8, "Copy e1 for point 0");
    or_(x28, x0, x9, "Copy e2 for point 0");

tile0_128x128:

    fadd_pq(f10, f7, f1, "Sample e0 at points (1, 6)");
    fadd_pq(f11, f8, f2, "Sample e1 at points (1, 6)");
    fadd_pq(f12, f9, f3, "Sample e2 at points (1, 6)");
    
    fltm_pi(m1, f7, f0, "Evaluate (e0 < 0) for points (0, 5)");
    fltm_pi(m2, f8, f0, "Evaluate (e1 < 0) for points (0, 5)");
    fltm_pi(m3, f9, f0, "Evaluate (e1 < 0) for points (0, 5)");

    fltm_pi(m4, f10, f0, "Evaluate (e0 < 0) for points (1, 6)");
    fltm_pi(m5, f11, f0, "Evaluate (e1 < 0) for points (1, 6)");
    fltm_pi(m6, f12, f0, "Evaluate (e1 < 0) for points (1, 6)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, tile1_128x128)

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 32x32 tile position (pixels)
    // x26 - x28 -> e0 - e2
 
    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

tile1_128x128:

    if (XREGS[x19].x <= 32)
        goto next_row_128x128;

    addi(x19, x19, -32, "One less 32x32 tile to rasterize");
    addi(x21, x21, 1, "Advance one 32x32 tile in the horizontal direction");

    fadd_pq(f13, f10, f1, "Sample e0 at points (2, 7)");
    fadd_pq(f14, f11, f2, "Sample e1 at points (2, 7)");
    fadd_pq(f15, f12, f3, "Sample e2 at points (2, 7)");
    
    fltm_pi(m1, f13, f0, "Evaluate (e0 < 0) for points (2, 7)");
    fltm_pi(m2, f14, f0, "Evaluate (e1 < 0) for points (2, 7)");
    fltm_pi(m3, f15, f0, "Evaluate (e1 < 0) for points (2, 7)");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, tile2_128x128)

    fmv_x_d(x26, f10, "Copy e0 at point 1");
    fmv_x_d(x27, f11, "Copy e1 at point 1");
    fmv_x_d(x28, f12, "Copy e2 at point 1");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 32x32 tile position (pixels)
    // x26 - x28 -> e0 - e2

    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

tile2_128x128:

    if (XREGS[x19].x <= 32)
        goto next_row_128x128;

    addi(x19, x19, -32, "One less 32x32 tile to rasterize");
    addi(x21, x21, 1, "Advance one 32x32 tile in the horizontal direction");

    fadd_pq(f10, f13, f1, "Sample e0 at points (3, 8)");
    fadd_pq(f11, f14, f2, "Sample e1 at points (3, 8)");
    fadd_pq(f12, f15, f3, "Sample e2 at points (3, 8)");
    
    fltm_pi(m4, f10, f0, "Evaluate (e0 < 0) for points (3, 8)");
    fltm_pi(m5, f11, f0, "Evaluate (e1 < 0) for points (3, 8)");
    fltm_pi(m6, f12, f0, "Evaluate (e1 < 0) for points (3, 8)");

    EVALUATE_TILE(m1, m2, m3, m4, m5, m6, tile3_128x128)

    fmv_x_d(x26, f13, "Copy e0 at point 2");
    fmv_x_d(x27, f14, "Copy e1 at point 2");
    fmv_x_d(x28, f15, "Copy e2 at point 2");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 32x32 tile position (pixels)
    // x26 - x28 -> e0 - e2

    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();


tile3_128x128:

    if (XREGS[x19].x <= 32)
        goto next_row_128x128;

    addi(x19, x19, -32, "One less 32x32 tile to rasterize");
    addi(x21, x21, 1, "Advance one 32x32 tile in the horizontal direction");

    fadd_pq(f13, f10, f1, "Sample e0 at points (4, 9)");
    fadd_pq(f14, f11, f2, "Sample e1 at points (4, 9)");
    fadd_pq(f15, f12, f3, "Sample e2 at points (4, 9)");
    
    fltm_pi(m1, f13, f0, "Evaluate (e0 < 0) for points (4, 9)");
    fltm_pi(m2, f14, f0, "Evaluate (e1 < 0) for points (4, 9)");
    fltm_pi(m3, f15, f0, "Evaluate (e1 < 0) for points (4, 9)");

    EVALUATE_TILE(m4, m5, m6, m1, m2, m3, next_row_128x128)

    fmv_x_d(x26, f10, "Copy e0 at point 3");
    fmv_x_d(x27, f11, "Copy e1 at point 3");
    fmv_x_d(x28, f12, "Copy e2 at point 3");

    //
    // x11 -> rbox_input_buffer
    // x13 -> RBOX packets
    // x21, x22 -> 32x32 tile position (pixels)
    // x26 - x28 -> e0 - e2

    // Check trivial accept.
    if (XREGS[x23].x == 0)
        generate_fully_covered_tile();
    else
        generate_large_tri_tile();

next_row_128x128:

    if (XREGS[x20].x <= 32)
        goto end_scalar_128x128;

    add(x19, x0, x24, "Restore horizontal tile dimension");
    add(x21, x0, x25, "Restore horizontal tile position");

    addi(x20, x20, -32, "One less 32x32 tile row to rasterize");
    addi(x22, x22, 1, "Advance one 32x32 tile in the vertical direction");

    fadd_pq(f7, f7, f4, "Sample e0 at (5, 10)");
    fadd_pq(f8, f8, f5, "Sample e1 at (5, 10)");
    fadd_pq(f9, f9, f6, "Sample e2 at (5, 10)");

    fmv_x_d(x26, f7, "Copy e0 at point 5");
    fmv_x_d(x27, f8, "Copy e1 at point 5");
    fmv_x_d(x28, f9, "Copy e2 at point 5");

    goto tile0_128x128;

end_scalar_128x128:

    ld(x19, 19 * 8, x30, "Restore register x19");
    ld(x20, 20 * 8, x30, "Restore register x20");
    ld(x21, 21 * 8, x30, "Restore register x21");
    ld(x22, 22 * 8, x30, "Restore register x22");
    ld(x23, 23 * 8, x30, "Restore register x23");
    ld(x24, 24 * 8, x30, "Restore register x24");
    ld(x25, 25 * 8, x30, "Restore register x25");
    ld(x26, 26 * 8, x30, "Restore register x26");
    ld(x27, 27 * 8, x30, "Restore register x27");
    ld(x28, 28 * 8, x30, "Restore register x28");
    ld(x29, 29 * 8, x30, "Restore register x29");

    print_comment("<<< rasterize_128x128_high_precision");

    return;
}


void create_high_prec_triangle_rbox_packets_tile_size_32x32()
{
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    //

    print_comment(">>> create_high_prec_triangle_rbox_packets");

    slli(x15, x12, 2, "triangle index * 4");
    add(x16, x15, x15, "triangle index * 8");
   
    add(x16, x10, x16, "Pointer to triangle coefficients");

    lw(x1,   0, x16, "Load a0");
    lw(x2,  32, x16, "Load a1");
    lw(x3,  64, x16, "Load a2");

    lw(x4,  96, x16, "Load b0");
    lw(x5, 128, x16, "Load b1");
    lw(x6, 160, x16, "Load b2");

    generate_large_tri();

    lw(x7, 192, x16, "Load c0");
    lw(x8, 224, x16, "Load c1");
    lw(x9, 256, x16, "Load c2");

    add(x15, x10, x15, "Pointer to triangle parameters");

    lw(x19, 352, x15, "Load tile bound dim x");
    lw(x20, 368, x15, "Load tile bound dim y");

    addi(x29, x0, x19, "Save tile bound dim x");

    lwu(x21, 288, x15, "Load tile bound min x");
    lwu(x22, 320, x15, "Load tile bound max x");

    srli(x21, x21, 13, "Compute tile x position in 32x32 tiles");
    srli(x22, x22, 13, "Compute tile y position in 32x32 tiles");

    slli(x23, x1, 7, "a0 * 128");
    slli(x24, x2, 7, "a1 * 128");
    slli(x25, x3, 7, "a2 * 128");

    slli(x26, x4, 7, "b0 * 128");
    slli(x27, x5, 7, "b1 * 128");
    slli(x28, x6, 7, "b2 * 128");

    slli(x1, x1, 5, "a0 * 32");
    slli(x2, x2, 5, "a1 * 32");
    slli(x3, x3, 5, "a2 * 32");

    slli(x4, x4, 5, "a0 * 32");
    slli(x5, x5, 5, "a1 * 32");
    slli(x6, x6, 5, "a2 * 32");


rasterize_tile_128x128:

    // x1  - x3  -> ai * 32
    // x4  - x6  -> bi * 32
    // x7  - x9  -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 32x32 tile position (tiles)
    //

    rasterize_128x128_high_precision();

    print_comment("Check if completed triangle traverse for this row");
    if (XREGS[x19].x <= 128)
        goto next_128x128_row;

    addi(x19, x19, -128, "One 128x128 tile was rasterized in the horizontal direction");
    addi(x21, x21, 4, "Advance to the next 128x128 tile in the horizontal direction");

    add(x7, x7, x23, "Sample e0 for next 128x128 tile in the horizontal direction");
    add(x8, x8, x24, "Sample e1 for next 128x128 tile in the horizontal direction");
    add(x9, x9, x25, "Sample e2 for next 128x128 tile in the horizontal direction");

    goto rasterize_tile_128x128;

next_128x128_row:

    print_comment("Check if completed triangle traverse.");
    if (XREGS[x20].x <= 128)
        goto end_large_tri;
    
    addi(x20, x20, -128, "One row of 128x128 tiles was rasterized");
    addi(x19, x0, x29, "Restore tile dim x for next row");
    addi(x22, x22, 4, "Advance to the next 128x128 tile in the vertical direction");

    add(x7, x7, x26, "Sample e0 for next 128x128 tile in the vertical direction");
    add(x8, x8, x27, "Sample e1 for next 128x128 tile in the vertical direction");
    add(x9, x9, x28, "Sample e2 for next 128x128 tile in the vertical direction");

    goto rasterize_tile_128x128;

end_large_tri:

    print_comment("<<< create_high_prec_triangle_rbox_packets");

    return;
}

void create_rbox_packets(TriangleSetupVector *triangle_setup_vector, uint32_t *rbox_input_buffer, uint32_t *rbox_packets)
{
    print_comment(">>> create_rbox_packets");

    init(x10, (uint64_t) triangle_setup_vector);
    init(x11, (uint64_t) rbox_input_buffer);
   
    addi(x12, x0, 0, "Set triangle index to 0");
    addi(x13, x0, 0, "Set RBOX packet counter to 0");

    lwu(x14, 388, x10, "Load low precision setup triangle mask");

    //
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
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

    addi(x12, x0, 0, "Set triangle index to 0");

    lwu(x14, 384, x10, "Load high precision setup triangle mask");

    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask

    andi(x5, x14, 0x01, "Check first triangle");
    if (XREGS[x5].x > 0)
        create_high_prec_triangle_rbox_packets();
 
    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x14].x == 0)
        goto return_num_packets;

    addi(x12, x12, 1, "Set triangle index to 1");

    andi(x5, x14, 0x01, "Check second triangle");
    if (XREGS[x5].x > 0)
        create_high_prec_triangle_rbox_packets();
 
    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x14].x == 0)
        goto return_num_packets;

    addi(x12, x12, 1, "Set triangle index to 2");

    andi(x5, x14, 0x01, "Check third triangle");
    if (XREGS[x5].x > 0)
        create_high_prec_triangle_rbox_packets();
 
    srli(x14, x14, 2, "Get next triangle bit");
    if (XREGS[x14].x == 0)
        goto return_num_packets;

    addi(x12, x12, 1, "Set triangle index to 3");

    andi(x5, x14, 0x01, "Check fourth triangle");
    if (XREGS[x5].x > 0)
        create_high_prec_triangle_rbox_packets();
    
return_num_packets:

    *rbox_packets = XREGS[x13].x;

    print_comment("<<< create_rbox_packets");
}

//  Old version to use with the version of rasterize_128x128_high_precision that uses
//  64-bit vector instructions that are no longer available.
//  Use the scalar version written in C.
void create_high_prec_triangle_rbox_packets_tile_size_64x64()
{
    // x10 -> triangle_setup_vector
    // x11 -> rbox_input_buffer
    // x12 -> triangle index
    // x13 -> RBOX packets
    // x14 -> triangle mask
    //

    print_comment(">>> create_high_prec_triangle_rbox_packets");

    slli(x15, x12, 2, "triangle index * 4");
    add(x16, x15, x15, "triangle index * 8");
   
    add(x16, x10, x16, "Pointer to triangle coefficients");

    lw(x1,   0, x16, "Load a0");
    lw(x2,  32, x16, "Load a1");
    lw(x3,  64, x16, "Load a2");

    lw(x4,  96, x16, "Load b0");
    lw(x5, 128, x16, "Load b1");
    lw(x6, 160, x16, "Load b2");

    generate_large_tri();

    lw(x7, 192, x16, "Load c0");
    lw(x8, 224, x16, "Load c1");
    lw(x9, 256, x16, "Load c2");

    add(x15, x10, x15, "Pointer to triangle parameters");

    lw(x19, 352, x15, "Load tile bound dim x");
    lw(x20, 368, x15, "Load tile bound dim y");

    addi(x29, x0, x19, "Save tile bound dim x");

    lwu(x21, 288, x15, "Load tile bound min x");
    lwu(x22, 320, x15, "Load tile bound max x");

    srli(x21, x21, 14, "Compute tile x position in 64x64 tiles");
    srli(x22, x22, 14, "Compute tile y position in 64x64 tiles");

    slli(x23, x1, 7, "a0 * 128");
    slli(x24, x2, 7, "a1 * 128");
    slli(x25, x3, 7, "a2 * 128");

    slli(x26, x4, 7, "b0 * 128");
    slli(x27, x5, 7, "b1 * 128");
    slli(x28, x6, 7, "b2 * 128");

    slli(x1, x1, 6, "a0 * 64");
    slli(x2, x2, 6, "a1 * 64");
    slli(x3, x3, 6, "a2 * 64");

    slli(x4, x4, 6, "a0 * 64");
    slli(x5, x5, 6, "a1 * 64");
    slli(x6, x6, 6, "a2 * 64");


rasterize_tile_128x128:

    // x1  - x3  -> ai * 64
    // x4  - x6  -> bi * 64
    // x7  - x9  -> ci/ei
    //
    // x19, x20 -> triangle BB dimensions (pixels)
    // x21, x22 -> 64x64 tile position (tiles)
    //

    rasterize_128x128_high_precision();

    print_comment("Check if completed triangle traverse for this row");
    if (XREGS[x19].x <= 128)
        goto next_128x128_row;

    addi(x19, x19, -128, "One 128x128 tile was rasterized in the horizontal direction");
    addi(x21, x21, 2, "Advance to the next 128x128 tile in the horizontal direction");

    add(x7, x7, x23, "Sample e0 for next 128x128 tile in the horizontal direction");
    add(x8, x8, x24, "Sample e1 for next 128x128 tile in the horizontal direction");
    add(x9, x9, x25, "Sample e2 for next 128x128 tile in the horizontal direction");

    goto rasterize_tile_128x128;

next_128x128_row:

    print_comment("Check if completed triangle traverse.");
    if (XREGS[x20].x <= 128)
        goto end_large_tri;
    
    addi(x20, x20, -128, "One row of 128x128 tiles was rasterized");
    addi(x19, x0, x29, "Restore tile dim x for next row");
    addi(x22, x22, 2, "Advance to the next 128x128 tile in the vertical direction");

    add(x7, x7, x26, "Sample e0 for next 128x128 tile in the vertical direction");
    add(x8, x8, x27, "Sample e1 for next 128x128 tile in the vertical direction");
    add(x9, x9, x28, "Sample e2 for next 128x128 tile in the vertical direction");

    goto rasterize_tile_128x128;

end_large_tri:

    print_comment("<<< create_high_prec_triangle_rbox_packets");

    return;
}

