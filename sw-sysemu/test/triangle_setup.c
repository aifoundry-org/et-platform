#include <triangle_setup.h>

uint32 triangle_setup_lit[] = 
{
    0,              // Indices for low 32-bits of coefficients, +0
    8,
    16,
    24,
    0x00000000,     // 1.0f, +16
    0x3ff00000,
    0x00000000,     // 2^51, +24
    0x43200000
};

void do_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector, uint64 index)
{
    print_comment(">>> do_triangle_setup_ccw_front");

    init(x10, (uint64) triangle_vector);
    init(x11, (uint64) triangle_setup_vector);
    init(x12,          index);
    init(x13, (uint64) triangle_setup_lit);

    //
    // NOTE : Preserve x3 for for scalar_triangle_setup_ccw_front
    //
    
    slli(x4, x12, 2, "Scale index by x4 to address tile bounds");
    slli(x5, x12, 3, "Scale index by x8 to address coefficients");

    add(x6, x10, x4, "Compute address to access vertex indices");
    lwu(x7, 32, x6, "Access v0 index");
    lwu(x8, 48, x6, "Access v1 index");
    lwu(x9, 64, x6, "Access v2 index");

    ld(x14, 0, x10, "Load pointer to v.x");
    ld(x15, 8, x10, "Load pointer to v.y");

    add(x16, x7, x14, "Compute address to access v0.x");
    add(x17, x8, x14, "Compute address to access v1.x");
    add(x18, x9, x14, "Compute address to access v2.x");

    lw(x20, 0, x16, "Load v0.x");
    lw(x21, 0, x17, "Load v1.x");
    lw(x22, 0, x18, "Load v2.x");

    add(x7, x7, x15, "Compute address to access v0.y");
    add(x8, x8, x15, "Compute address to access v1.y");
    add(x9, x9, x15, "Compute address to access v3.y");

    lw(x23, 0, x7, "Load v0.y");
    lw(x24, 0, x8, "Load v1.y");
    lw(x25, 0, x9, "Load v2.y");

    add(x6, x11, x4, "Compute address to access tile bound min");
    lw(x7, 288, x6, "Load tile bound min x");
    lw(x8, 320, x6, "Load tile bound min y");

    sub(x20, x20, x7, "Rebase v0.x to tile bound min x");
    sub(x21, x21, x7, "Rebase v1.x to tile bound min x");
    sub(x22, x22, x7, "Rebase v2.x to tile bound min x");
    
    sub(x23, x23, x8, "Rebase v0.y to tile bound min y");
    sub(x24, x24, x8, "Rebase v1.y to tile bound min y");
    sub(x25, x25, x8, "Rebase v2.y to tile bound min y");

    sub(x14, x23, x24, "a0 = y0 - y1");
    sub(x15, x24, x25, "a1 = y1 - y2");
    sub(x16, x25, x23, "a2 = y2 - y0");
    
    sub(x17, x21, x20, "b0 = x1 - x0");
    sub(x18, x22, x21, "b1 = x2 - x1");
    sub(x19, x20, x22, "b2 = x0 - x2");

    mul(x6, x20, x24, "x0 * y1");
    mul(x7, x23, x21, "y0 * x1");
    sub(x6, x6,   x7, "c0 = x0 * y1 - y0 * x1");
    mul(x7, x21, x25, "x1 * y2");
    mul(x8, x24, x22, "y1 * x2");
    sub(x7,  x7,  x8, "c1 = x1 * y2 - y1 * x2");
    mul(x8, x22, x23, "x2 * y0");
    mul(x9, x25, x20, "y2 * x0");
    sub(x8,  x8,  x9, "c2 = x2 * y0 - y2 * x0");

    add(x9, x7, x8, "c0 + c1");
    add(x9, x9, x8, "tri_2x_area = c0 + c1 + c2");

    if (XREGS[x9].x < 0)
    {
        lwu(x3, 384, x11, "Load high-precision setup triangles mask");
        addi(x4, x0, 1, "Set bit 0 to 1");
        add(x5, x12, x12, "Index * 2");
        sll(x4, x4, x5, "Set bit mask for triangle");
        xor_(x3, x3, x4, "Clear triangle bit from high-precision setup triangles mask");
        sw(x3, 384, x11, "Store high-precision setup triangles mask");

        print_comment("<<< do_triangle_setup_ccw_front");
        return;
    }

    fcvt_d_l(f0, (freg) x9, "Convert tri_2x_area to 64-bit float point");
    fld(f1, 16, x13, "Load 1.0f");
    fdiv_d(f0, f1, f0, "1.0f / tri_2x_area");
    fld(f2, 24, x13, "Load 2^51");
    fmul_d(f0, f0, f2, "Get 51 bits of precision for integer conversion");
    fcvt_l_d((freg) x9, f0, "Convert back to 64-bit integer, precision 17.34");

    mul(x14, x14, x9, "an0 = a0 * (1.0f / tri_2x_area)");
    mul(x15, x15, x9, "an1 = a1 * (1.0f / tri_2x_area)");
    mul(x16, x16, x9, "an2 = a2 * (1.0f / tri_2x_area)");

    mul(x17, x17, x9, "bn0 = b0 * (1.0f / tri_2x_area)");
    mul(x18, x18, x9, "bn1 = b1 * (1.0f / tri_2x_area)");
    mul(x19, x19, x9, "bn2 = b2 * (1.0f / tri_2x_area)");

    mul(x6, x6, x9, "cn0 = c0 * (1.0f / tri_2x_area)");
    mul(x7, x7, x9, "cn1 = c1 * (1.0f / tri_2x_area)");
    mul(x8, x8, x9, "cn2 = c2 * (1.0f / tri_2x_area)");

    srai(x14, x14, 17, "Reduce an0 precision to 9.25 from 35.42");
    srai(x15, x15, 17, "Reduce an1 precision to 9.25 from 35.42");
    srai(x16, x16, 17, "Reduce an2 precision to 9.25 from 35.42");

    srai(x17, x17, 17, "Reduce bn0 precision to 9.25 from 35.42");
    srai(x18, x18, 17, "Reduce bn1 precision to 9.25 from 35.42");
    srai(x19, x19, 17, "Reduce bn2 precision to 9.25 from 35.42");

    srai(x6, x6, 25, "Reduce cn0 precision to 10.25 from 51.50");
    srai(x7, x7, 25, "Reduce cn1 precision to 10.25 from 51.50");
    srai(x8, x8, 25, "Reduce cn2 precision to 10.25 from 51.50");

    srai(x20, x14, 1, "a0 * 0.5");
    srai(x21, x15, 1, "a1 * 0.5");
    srai(x22, x16, 1, "a2 * 0.5");

    srai(x23, x17, 1, "b0 * 0.5");
    srai(x24, x18, 1, "b1 * 0.5");
    srai(x25, x19, 1, "b2 * 0.5");

    add(x6, x6, x20, "Move sample point to (0.5, x)");
    add(x7, x7, x21, "Move sample point to (0.5, x)");
    add(x8, x8, x22, "Move sample point to (0.5, x)");

    add(x6, x6, x23, "Move sample point to (0.5, 0.5)");
    add(x7, x7, x24, "Move sample point to (0.5, 0.5)");
    add(x8, x8, x25, "Move sample point to (0.5, 0.5)");

    add(x20, x11, x5, "Compute address to store coefficients");
    sd(x14,   0, x20, "Store an0");
    sd(x15,  32, x20, "Store an1");
    sd(x16,  64, x20, "Store an2");

    sd(x17,  96, x20, "Store bn0");
    sd(x18, 128, x20, "Store bn1");
    sd(x19, 160, x20, "Store bn2");

    sd(x6, 192, x20, "Store cn0");
    sd(x7, 224, x20, "Store cn1");
    sd(x8, 256, x20, "Store cn2");

    print_comment("<<< do_triangle_setup_ccw_front");
}

void scalar_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector)
{
    print_comment(">>> scalar_triangle_setup_ccw_front");

    init(x10, (uint64) triangle_vector);
    init(x11, (uint64) triangle_setup_vector);

    lwu(x3, 384, x11, "Load high-precision setup triangles mask");

    andi(x4, x3, 0x01, "Check first triangle");
    if (XREGS[x4].x > 0)
        do_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector, 0);
 
    srli(x3, x3, 2, "Get next triangle bit");
    if (XREGS[x3].x == 0)
    {
        print_comment("<<< scalar_triangle_setup_ccw_front");
        return;
    }

    andi(x4, x3, 0x01, "Check second triangle");
    if (XREGS[x4].x > 0)
        do_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector, 1);
    
    srli(x3, x3, 2, "Get next triangle bit");
    if (XREGS[x3].x == 0)
    {
        print_comment("<<< scalar_triangle_setup_ccw_front");
        return;
    }

    andi(x4, x3, 0x01, "Check third triangle");
    if (XREGS[x4].x > 0)
        do_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector, 2);

    srli(x3, x3, 2, "Get next triangle bit");
    if (XREGS[x3].x == 0)
    {
        print_comment("<<< scalar_triangle_setup_ccw_front");
        return;
    }

    andi(x4, x3, 0x01, "Check fourth triangle");
    if (XREGS[x4].x > 0)
        do_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector, 3);


    print_comment("<<< scalar_triangle_setup_ccw_front");
}

void vector_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector)
{

    // Four input triangles, (x, y, z) projected and mapped to viewport.

    // Compute triangle bounding box.
    
    print_comment(">>> vector_triangle_setup_ccw_front");

    //init_stack();
    init(x10, (uint64) triangle_vector);
    init(x11, (uint64) triangle_setup_vector);
    init(x12, (uint64) triangle_setup_lit);

    lw(x3, 80, x10, "Load triangle quad mask");
    mov_m_x(m0, x3, 0, "Set triangle quad mask");

    ld(x4,  0, x10, "Load pointer to v.x");
    ld(x5,  8, x10, "Load pointer to v.y");

    flw_ps(f6, 32, x10, "Load v0 indices");
    flw_ps(f7, 48, x10, "Load v1 indices");
    flw_ps(f8, 64, x10, "Load v1 indices");

    fgw_ps(f0, f6, x4, "Load v0.x vector");
    fgw_ps(f1, f7, x4, "Load v1.x vector");
    fgw_ps(f2, f8, x4, "Load v2.x vector");

    fgw_ps(f3, f6, x5, "Load v0.y vector");
    fgw_ps(f4, f7, x5, "Load v1.y vector");
    fgw_ps(f5, f8, x5, "Load v2.y vector");

    fmin_pi(f6, f0, f1, "Compute vminx");
    fmin_pi(f6, f6, f2, "compute vminx");
    fmax_pi(f7, f0, f1, "Compute vmaxx");
    fmax_pi(f7, f7, f2, "Compute vmaxx");

    fmin_pi(f8, f3, f4, "Compute vminy");
    fmin_pi(f8, f8, f5, "compute vminy");
    fmax_pi(f9, f3, f4, "Compute vmaxy");
    fmax_pi(f9, f9, f5, "Compute vmaxy");

    fbci_pi(f10, 0x01fff, "Load 32x32 tile offset mask");
    fbci_pi(f11, 0xfe000, "Load 32x32 tile clear mask");

    fand_pi(f6, f6, f11, "Get tile bound min x");
    fand_pi(f8, f8, f11, "Get tile bound min y");

    fand_pi(f12, f7, f10, "Get tile offset for max x");
    fand_pi(f13, f9, f10, "Get tile offset for max y");

    fbci_pi(f14, 0, "Load zero");

    feq_pi(f15, f12, f14, "Compute tile bound round for max x");
    feq_pi(f16, f13, f14, "Compute tile bound round for max y");

    faddi_pi(f15, f15, 1, "Compute tile bound round for max x");
    faddi_pi(f16, f16, 1, "Compute tile bound round for max y");

    fslli_pi(f15, f15, 13, "Multiply by tile granularity 32 * 256");
    fslli_pi(f16, f16, 13, "Multiply by tile granularity 32 * 256");

    fand_pi(f7, f7, f11, "Get tile bound max x");
    fand_pi(f9, f9, f11, "Get tile bound max y");

    fadd_pi(f7, f7, f15, "Add tile bound round for max x");
    fadd_pi(f9, f9, f16, "Add tile bound round for max y");

    fsub_pi(f10, f7, f6, "Compute tile bound dim x");
    fsub_pi(f11, f9, f8, "Compute tile bound dim y");

    fsrli_pi(f10, f10, 8, "Tile bound dim x in pixels");
    fsrli_pi(f11, f11, 8, "Tile bound dim y in pixels");

    fsw_ps(f6,  288, x11, "Store tile bound min x");
    fsw_ps(f7,  304, x11, "Store tile bound max x");
    fsw_ps(f8,  320, x11, "Store tile bound min y");
    fsw_ps(f9,  336, x11, "Store tile bound max y");
    fsw_ps(f10, 352, x11, "Store tile bound dim x");
    fsw_ps(f11, 368, x11, "Store tile bound dim y");

    // Check if the triangle needs 64-bit precision to compute ci and triangle area.
    // If that's the case then work in scalar mode.

    fsub_pi(f16, f7, f6, "Compute triangle tile bound horizontal dimension");
    fsub_pi(f17, f9, f8, "Compute triangle tile bound vertical dimension");

    fbci_pi(f18, 0x08001, "Load largest tile dimension that can be setup with 32-bit precision");
    fmax_pi(f16, f16, f17, "Determine maximum tile bound dimension");
    fmin_pi(f16, f16, f18, "Clamp to maximum tile dimension for 32-bit setup");
    
    feq_pi(f16, f16, f18, "Detect if triangle tiled BB is too large for 32-bit setup");
    fsetm_ps(m1, f16, "Set mask with the comparison result");

    maskand(m2, m0, m1, "Combine with triangle mask");
    
    maskpopc(x15, m2, "Count number of triangles that need setup with 64-bit precision");

    if (XREGS[x15].x > 0)
    {
        mova_x_m(x14, "Get mask registers");
        srli(x14, x14, 16, "Shift to get m2");
        andi(x14, x14, 0x55, "Clear higher bits");

        sw(x14, 384, x11, "Store high-precision setup triangle mask");

        if (XREGS[x13].x == 4)   
        {
            // All triangles need to be processed with 64-bit precision.
            scalar_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector);
            print_comment("<<< vector_triangle_setup_ccw_front");
            return;
        }
        else
        {
            masknot(m2, m1, "Get mask of triangles that can be processed with 32-bit precision");
            maskand(m0, m0, m2, "Combine with triangle mask");
        }
    }
    
    fsub_pi(f0, f0, f6, "Rebase to tile bound min x");
    fsub_pi(f1, f1, f6, "Rebase to tile bound min x");
    fsub_pi(f2, f2, f6, "Rebase to tile bound min x");

    fsub_pi(f3, f3, f8, "Rebase to tile bound min y");
    fsub_pi(f4, f4, f8, "Rebase to tile bound min y");
    fsub_pi(f5, f5, f8, "Rebase to tile bound min y");

    fsub_pi(f10, f3, f4, "a0 = y0 - y1");
    fsub_pi(f11, f4, f5, "a1 = y1 - y2");
    fsub_pi(f12, f5, f3, "a2 = y2 - y0");
    
    fsub_pi(f13, f1, f0, "b0 = x1 - x0");
    fsub_pi(f14, f2, f1, "b1 = x2 - x1");
    fsub_pi(f15, f0, f2, "b2 = x0 - x2");

    fmul_pi(f16, f0, f4, "x0 * y1");
    fmul_pi(f17, f3, f1, "y0 * x1");
    fsub_pi(f16, f17, f17, "c0 = x0 * y1 - y0 * x1");
    fmul_pi(f17, f1, f5, "x1 * y2");
    fmul_pi(f18, f4, f2, "y1 * x2");
    fsub_pi(f17, f17, f18, "c1 = x1 * y2 - y1 * x2");
    fmul_pi(f18, f1, f3, "x2 * y0");
    fmul_pi(f19, f5, f0, "y2 * x0");
    fsub_pi(f18, f18, f19, "c2 = x2 * y0 - y2 * x0");

    fadd_pi(f19, f16, f17, "c0 + c1");
    fadd_pi(f19, f18, f19, "tri_2x_area = c0 + c1 + c2");

    // Check area sign to detect back facing triangles
    fbci_pi(f20, 0, "Load zero");
    flt_pi(f21, f19, f20, "area < 0");
    fsetm_ps(m1, f21, "Set mask with comparison result");

    masknot(m1, m1, "Negate mask");
    maskand(m0, m0, m1, "Cull back facing triangles");

    maskpopc(x13, m0, "Count how many triangles remain");

    // If 0 front facing triangles quit here.  No need to store coefficients.
    if (XREGS[x13].x == 0)
    {
        print_comment("<<< vector_triangle_setup_ccw_front");
        return;
    }

    mova_x_m(x13, "Get m0 mask register");
    andi(x13, x13, 0x55, "Clear upper bits");
    sd(x13, 388, x11, "Store low-precision setup triangle mask");

    frcpfxp_ps(f19, f19, "Compute reciprocal of 2xtriangle area");

    fmul_pi(f10, f10, f19, "Normalize a0, lower 32-bits");
    fmul_pi(f11, f11, f19, "Normalize a1, lower 32-bits");
    fmul_pi(f12, f12, f19, "Normalize a2, lower 32-bits");

    fmul_pi(f13, f13, f19, "Normalize b0, lower 32-bits");
    fmul_pi(f14, f14, f19, "Normalize b1, lower 32-bits");
    fmul_pi(f15, f15, f19, "Normalize b2, lower 32-bits");

    fmul_pi(f16, f16, f19, "Normalize c0, lower 32-bits");
    fmul_pi(f17, f17, f19, "Normalize c1, lower 32-bits");
    fmul_pi(f18, f18, f19, "Normalize c2, lower 32-bits");

    fsrai_pi(f10, f10, 6, "Normalize a0, set precision to 9.15");
    fsrai_pi(f11, f11, 6, "Normalize a1, set precision to 9.15");
    fsrai_pi(f12, f12, 6, "Normalize a2, set precision to 9.15");

    fsrai_pi(f13, f13, 6, "Normalize b0, set precision to 9.15");
    fsrai_pi(f14, f14, 6, "Normalize b1, set precision to 9.15");
    fsrai_pi(f15, f15, 6, "Normalize b2, set precision to 9.15");

    fsrai_pi(f16, f16, 14, "Normalize c0, set precision to 1.15");
    fsrai_pi(f17, f17, 14, "Normalize c1, set precision to 1.15");
    fsrai_pi(f18, f18, 14, "Normalize c2, set precision to 1.15");

    fsrai_pi(f19, f10, 1, "a0 * 0.5");
    fsrai_pi(f20, f11, 1, "a1 * 0.5");
    fsrai_pi(f21, f12, 1, "a2 * 0.5");

    fsrai_pi(f22, f13, 1, "b0 * 0.5");
    fsrai_pi(f23, f14, 1, "b1 * 0.5");
    fsrai_pi(f24, f15, 1, "b2 * 0.5");

    fadd_pi(f16, f16, f19, "Move sample point to (0.5, x)");
    fadd_pi(f17, f17, f20, "Move sample point to (0.5, x)");
    fadd_pi(f18, f18, f21, "Move sample point to (0.5, x)");

    fadd_pi(f16, f16, f22, "Move sample point to (0.5, 0.5)");
    fadd_pi(f17, f17, f23, "Move sample point to (0.5, 0.5)");
    fadd_pi(f18, f18, f24, "Move sample point to (0.5, 0.5)");

    flw_ps(f0,  0, x12, "Load indices for the coefficients low 32-bits");

    fbci_pi(f2, 0, "Load zero");

    fscw_ps(f10, f0, x11, "Store a0 low 32-bits");

    addi(x13, x11, 32, "Pointer to a1");
    fscw_ps(f11, f0, x13, "Store a1 low 32-bits");

    addi(x13, x11, 64, "Pointer to a2");
    fscw_ps(f12, f0, x13, "Store a2 low 32-bits");

    addi(x13, x11, 96, "Pointer to b0");
    fscw_ps(f13, f0, x13, "Store b0 low 32-bits");

    addi(x13, x11, 128, "Pointer to b1");
    fscw_ps(f14, f0, x13, "Store b1 low 32-bits");

    addi(x13, x11, 160, "Pointer to b2");
    fscw_ps(f15, f0, x13, "Store b2 low 32-bits");

    addi(x13, x11, 192, "Pointer to c0");
    fscw_ps(f16, f0, x13, "Store c0 low 32-bits");

    addi(x13, x11, 224, "Pointer to c1");
    fscw_ps(f17, f0, x13, "Store c1 low 32-bits");

    addi(x13, x11, 256, "Pointer to c2");
    fscw_ps(f18, f0, x13, "Store c2 low 32-bits");

    mov_m_x(m0, x0, 0x55, "Set mask to (1, 1, 1, 1) after triangle processing");

    // Process now the triangles that need 64-bit setup.
    if (XREGS[x15].x > 0)
        scalar_triangle_setup_ccw_front(triangle_vector, triangle_setup_vector);

    print_comment("<<< vector_triangle_setup_ccw_front");
}

void vector_triangle_setup_ccw_back(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector)
{
    print_comment(">>> vector_triangle_setup_ccw_back");

    // Code for negating equation coefficients
    fxor_pi(f10, f10, f19, "(area < 0) ? -a0 : a0");
    fxor_pi(f11, f11, f19, "(area < 0) ? -a1 : a1");
    fxor_pi(f12, f12, f19, "(area < 0) ? -a2 : a2");
    fxor_pi(f13, f13, f19, "(area < 0) ? -b0 : b0");
    fxor_pi(f14, f14, f19, "(area < 0) ? -b1 : b1");
    fxor_pi(f15, f15, f19, "(area < 0) ? -b2 : b2");
    fnot_pi(f19, f19, "");
    faddi_pi(f19, f19, 1, "");
    fadd_pi(f10, f10, f19, "(area < 0) ? -a0 : a0");
    fadd_pi(f11, f11, f19, "(area < 0) ? -a1 : a1");
    fadd_pi(f12, f12, f19, "(area < 0) ? -a2 : a2");
    fadd_pi(f13, f13, f19, "(area < 0) ? -b0 : b0");
    fadd_pi(f14, f14, f19, "(area < 0) ? -b1 : b1");
    fadd_pi(f15, f15, f19, "(area < 0) ? -b2 : b2");

    print_comment("<<< vector_triangle_setup_ccw_back");
}


