
#ifndef _TRIANGLE_SETUP_
#define _TRIANGLE_SETUP_

#include <emu.h>

typedef struct
{
    uint32_t *x;          // +0
    uint32_t *y;          // +8
    uint32_t *z;          // +16
    uint64_t padding;     // +24
} VertexPositionArray;

typedef struct
{
    VertexPositionArray vertex_array;   // +0
    uint32_t v0_indices[4];               // +32
    uint32_t v1_indices[4];               // +48
    uint32_t v2_indices[4];               // +64
    uint32_t mask;                        // +80
} TriangleVector;

typedef struct
{
    uint64_t a0[4];               // +0
    uint64_t a1[4];               // +32
    uint64_t a2[4];               // +64
    uint64_t b0[4];               // +96
    uint64_t b1[4];               // +128
    uint64_t b2[4];               // +160
    uint64_t c0[4];               // +192
    uint64_t c1[4];               // +224
    uint64_t c2[4];               // +256
    uint32_t tile_bound_minx[4];  // +288
    uint32_t tile_bound_maxx[4];  // +304
    uint32_t tile_bound_miny[4];  // +320
    uint32_t tile_bound_maxy[4];  // +336
    uint32_t tile_bound_dimx[4];  // +352
    uint32_t tile_bound_dimy[4];  // +368
    uint32_t high_precision_mask; // +384     // Stores ET vector mask, check bits 0, 2, 4, 6
    uint32_t low_precision_mask;  // +388     // Stores ET vector mask, check bits 0, 2, 4, 6
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

void do_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector, uint64_t index);
void scalar_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);
void vector_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);

#endif

