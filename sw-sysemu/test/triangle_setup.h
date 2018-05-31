
#ifndef _TRIANGLE_SETUP_
#define _TRIANGLE_SETUP_

#include <emu.h>

typedef struct
{
    uint32 *x;          // +0
    uint32 *y;          // +8
    uint32 *z;          // +16
    uint64 padding;     // +24
} VertexPositionArray;

typedef struct
{
    VertexPositionArray vertex_array;   // +0
    uint32 v0_indices[4];               // +32
    uint32 v1_indices[4];               // +48
    uint32 v2_indices[4];               // +64
    uint32 mask;                        // +80
} TriangleVector;

typedef struct
{
    uint64 a0[4];               // +0
    uint64 a1[4];               // +32
    uint64 a2[4];               // +64
    uint64 b0[4];               // +96
    uint64 b1[4];               // +128
    uint64 b2[4];               // +160
    uint64 c0[4];               // +192
    uint64 c1[4];               // +224
    uint64 c2[4];               // +256
    uint32 tile_bound_minx[4];  // +288
    uint32 tile_bound_maxx[4];  // +304
    uint32 tile_bound_miny[4];  // +320
    uint32 tile_bound_maxy[4];  // +336
    uint32 tile_bound_dimx[4];  // +352
    uint32 tile_bound_dimy[4];  // +368
    uint32 high_precision_mask; // +384     // Stores ET vector mask, check bits 0, 2, 4, 6
    uint32 low_precision_mask;  // +388     // Stores ET vector mask, check bits 0, 2, 4, 6
} TriangleSetupVector;

// We need to support three different tile sizes for rasterization
// based on the bytes per pixel of the attached render targets.
//
//  Bpt    width     height
//        (pixels)  (pixels)
//   1      64        64
//   2      64        32
//   4+     32        32
//
// The most common size should be 32x32.
//
typedef enum
{
    TILE_SIZE_64x64 = 0,
    TILE_SIZE_64x32 = 01,
    TILE_SIZE_32x32 = 02 
} TileSize;

void do_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector, uint64 index);
void scalar_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);
void vector_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);

#endif

