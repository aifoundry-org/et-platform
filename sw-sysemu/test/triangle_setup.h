
#ifndef _TRIANGLE_SETUP_
#define _TRIANGLE_SETUP_

#include <emu.h>

#define VECTOR_LENGTH 8

typedef struct
{
    uint32_t *x;          // +0
    uint32_t *y;          // +8
    uint32_t *z;          // +16
    uint64_t padding;     // +24
} VertexPositionArray;

typedef struct
{
    VertexPositionArray vertex_array;       // +0
    uint32_t v0_indices[VECTOR_LENGTH];     // +32
    uint32_t v1_indices[VECTOR_LENGTH];     // +32+VECTOR_LENGHT*4
    uint32_t v2_indices[VECTOR_LENGTH];     // +32+VECTOR_LENGTH*4*2
    uint32_t mask;                          // +32+VECTOR_LENGHT*4*3
} TriangleVector;

typedef struct
{
    uint64_t a0[VECTOR_LENGTH];                 // +0
    uint64_t a1[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*1
    uint64_t a2[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*2
    uint64_t b0[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*3
    uint64_t b1[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*4
    uint64_t b2[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*5
    uint64_t c0[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*6
    uint64_t c1[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*7
    uint64_t c2[VECTOR_LENGTH];                 // +VECTOR_LENGHT*8*8
    uint32_t tile_bound_minx[VECTOR_LENGTH];    // +VECTOR_LENGHT*8*9
    uint32_t tile_bound_maxx[VECTOR_LENGTH];    // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*1
    uint32_t tile_bound_miny[VECTOR_LENGTH];    // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*2
    uint32_t tile_bound_maxy[VECTOR_LENGTH];    // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*3
    uint32_t tile_bound_dimx[VECTOR_LENGTH];    // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*4
    uint32_t tile_bound_dimy[VECTOR_LENGTH];    // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*5
    uint32_t high_precision_mask;               // +VECTOR_LENGTH*8*9+VECTOR_LENGHT*4*6     // Stores ET vector mask, check bits[VECTOR_LENGTH-1:0]
    uint32_t low_precision_mask;                // +VECTOR_LENGTH*8*9+VECTOR_LENGTH*4*6+4   // Stores ET vector mask, check bits[VECTOR_LENGTH-1:0]
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
    TILE_SIZE_64x32 = 1,
    TILE_SIZE_32x32 = 2 
} TileSize;

void do_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector, uint64_t index);
void scalar_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);
void vector_triangle_setup_ccw_front(TriangleVector *triangle_vector, TriangleSetupVector *triangle_setup_vector);

#endif

