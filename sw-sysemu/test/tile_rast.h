
#ifndef _TILE_RAST_

#define _TILE_RAST_

#include <triangle_setup.h>

void generate_base_triangle_with_tile_128x128_packet();
void generate_triangle_with_tile_128x128_packet();
void generate_fully_covered_tile(uint32 tile_x, uint32 tile_y, uint64 tile_c[4], uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size);
void generate_large_tri_tile(uint32 tile_x, uint32 tile_y, uint64 tile_c[4], uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size);
void generate_large_tri(uint64 a[], uint64 b[], uint8 **rbox_input_buffer, uint32 *rbox_input_packets);

void rasterize_128x64_to_64x64();
void rasterize_64x128_to_64x64();
void rasterize_128x128_to_64x64();
void create_low_prec_triangle_rbox_packets_tile_size_64x64();

void rasterize_Nx32_to_32x32();
void rasterize_32xN_to_32x32();
void rasterize_96xN_to_32x32();
void rasterize_Nx96_to_32x32();
void rasterize_128x128_to_32x32();
void create_low_prec_triangle_rbox_packets_tile_size_32x32();

void rasterize_128x128_high_precision(uint32 tile_x, uint32 tile_y,             // Tile position (tiles)
                                      uint32 tile_dim_x, uint32 tile_dim_y,     // Tile dimension (tiles, max 4x4)
                                      uint64 tile_c_step_x[4], uint64 tile_c_step_y[4], uint64 tile_c[4],
                                      uint8 **rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size);
void create_high_prec_triangle_rbox_packets(TriangleSetupVector *triangle_setup_vector, uint8 *rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size);
void create_rbox_packets(TriangleSetupVector *triangle_setup_vector, uint8 *rbox_input_buffer, uint32 *rbox_packets, TileSize tile_size);

#endif
