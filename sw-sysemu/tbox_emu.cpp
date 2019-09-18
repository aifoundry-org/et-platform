#include <cmath>
#include <cstring>
#include <stdexcept>

#include "emu_gio.h"
#include "fpu/fpu.h"
#include "fpu/fpu_casts.h"
#include "memop.h"
#include "tbox_emu.h"
#ifndef TBOX_MINION_SIM
#include "emu.h"
#endif

#define min(a, b) (((a) <= (b)) ? (a) : (b))
#define max(a, b) (((a) >= (b)) ? (a) : (b))

const uint32_t TBOX::TBOXEmu::BYTES_PER_TEXEL_IN_MEMORY[] = {
    0,  // FORMAT_UNDEFINED
    1,  // FORMAT_R4G4_UNORM_PACK8
    2,  // FORMAT_R4G4B4A4_UNORM_PACK16
    2,  // FORMAT_B4G4R4A4_UNORM_PACK16
    2,  // FORMAT_R5G6B5_UNORM_PACK16
    2,  // FORMAT_B5G6R5_UNORM_PACK16
    2,  // FORMAT_R5G5B5A1_UNORM_PACK16
    2,  // FORMAT_B5G5R5A1_UNORM_PACK16
    2,  // FORMAT_A1R5G5B5_UNORM_PACK16
    1,  // FORMAT_R8_UNORM
    1,  // FORMAT_R8_SNORM
    1,  // FORMAT_R8_USCALED
    1,  // FORMAT_R8_SSCALED
    1,  // FORMAT_R8_UINT
    1,  // FORMAT_R8_SINT
    1,  // FORMAT_R8_SRGB
    2,  // FORMAT_R8G8_UNORM
    2,  // FORMAT_R8G8_SNORM
    2,  // FORMAT_R8G8_USCALED
    2,  // FORMAT_R8G8_SSCALED
    2,  // FORMAT_R8G8_UINT
    2,  // FORMAT_R8G8_SINT
    2,  // FORMAT_R8G8_SRGB
    3,  // FORMAT_R8G8B8_UNORM
    3,  // FORMAT_R8G8B8_SNORM
    3,  // FORMAT_R8G8B8_USCALED
    3,  // FORMAT_R8G8B8_SSCALED
    3,  // FORMAT_R8G8B8_UINT
    3,  // FORMAT_R8G8B8_SINT
    3,  // FORMAT_R8G8B8_SRGB
    3,  // FORMAT_B8G8R8_UNORM
    3,  // FORMAT_B8G8R8_SNORM
    3,  // FORMAT_B8G8R8_USCALED
    3,  // FORMAT_B8G8R8_SSCALED
    3,  // FORMAT_B8G8R8_UINT
    3,  // FORMAT_B8G8R8_SINT
    3,  // FORMAT_B8G8R8_SRGB
    4,  // FORMAT_R8G8B8A8_UNORM
    4,  // FORMAT_R8G8B8A8_SNORM
    4,  // FORMAT_R8G8B8A8_USCALED
    4,  // FORMAT_R8G8B8A8_SSCALED
    4,  // FORMAT_R8G8B8A8_UINT
    4,  // FORMAT_R8G8B8A8_SINT
    4,  // FORMAT_R8G8B8A8_SRGB
    4,  // FORMAT_B8G8R8A8_UNORM
    4,  // FORMAT_B8G8R8A8_SNORM
    4,  // FORMAT_B8G8R8A8_USCALED
    4,  // FORMAT_B8G8R8A8_SSCALED
    4,  // FORMAT_B8G8R8A8_UINT
    4,  // FORMAT_B8G8R8A8_SINT
    4,  // FORMAT_B8G8R8A8_SRGB
    4,  // FORMAT_A8B8G8R8_UNORM_PACK32
    4,  // FORMAT_A8B8G8R8_SNORM_PACK32
    4,  // FORMAT_A8B8G8R8_USCALED_PACK32
    4,  // FORMAT_A8B8G8R8_SSCALED_PACK32
    4,  // FORMAT_A8B8G8R8_UINT_PACK32
    4,  // FORMAT_A8B8G8R8_SINT_PACK32
    4,  // FORMAT_A8B8G8R8_SRGB_PACK32
    4,  // FORMAT_A2R10G10B10_UNORM_PACK32
    4,  // FORMAT_A2R10G10B10_SNORM_PACK32
    4,  // FORMAT_A2R10G10B10_USCALED_PACK32
    4,  // FORMAT_A2R10G10B10_SSCALED_PACK32
    4,  // FORMAT_A2R10G10B10_UINT_PACK32
    4,  // FORMAT_A2R10G10B10_SINT_PACK32
    4,  // FORMAT_A2B10G10R10_UNORM_PACK32
    4,  // FORMAT_A2B10G10R10_SNORM_PACK32
    4,  // FORMAT_A2B10G10R10_USCALED_PACK32
    4,  // FORMAT_A2B10G10R10_SSCALED_PACK32
    4,  // FORMAT_A2B10G10R10_UINT_PACK32
    4,  // FORMAT_A2B10G10R10_SINT_PACK32
    2,  // FORMAT_R16_UNORM
    2,  // FORMAT_R16_SNORM
    2,  // FORMAT_R16_USCALED
    2,  // FORMAT_R16_SSCALED
    2,  // FORMAT_R16_UINT
    2,  // FORMAT_R16_SINT
    2,  // FORMAT_R16_SFLOAT
    4,  // FORMAT_R16G16_UNORM
    4,  // FORMAT_R16G16_SNORM
    4,  // FORMAT_R16G16_USCALED
    4,  // FORMAT_R16G16_SSCALED
    4,  // FORMAT_R16G16_UINT
    4,  // FORMAT_R16G16_SINT
    4,  // FORMAT_R16G16_SFLOAT
    6,  // FORMAT_R16G16B16_UNORM
    6,  // FORMAT_R16G16B16_SNORM
    6,  // FORMAT_R16G16B16_USCALED
    6,  // FORMAT_R16G16B16_SSCALED
    6,  // FORMAT_R16G16B16_UINT
    6,  // FORMAT_R16G16B16_SINT
    6,  // FORMAT_R16G16B16_SFLOAT
    8,  // FORMAT_R16G16B16A16_UNORM
    8,  // FORMAT_R16G16B16A16_SNORM
    8,  // FORMAT_R16G16B16A16_USCALED
    8,  // FORMAT_R16G16B16A16_SSCALED
    8,  // FORMAT_R16G16B16A16_UINT
    8,  // FORMAT_R16G16B16A16_SINT
    8,  // FORMAT_R16G16B16A16_SFLOAT
    4,  // FORMAT_R32_UINT
    4,  // FORMAT_R32_SINT
    4,  // FORMAT_R32_SFLOAT
    8,  // FORMAT_R32G32_UINT
    8,  // FORMAT_R32G32_SINT
    8,  // FORMAT_R32G32_SFLOAT
    12, // FORMAT_R32G32B32_UINT
    12, // FORMAT_R32G32B32_SINT
    12, // FORMAT_R32G32B32_SFLOAT
    16, // FORMAT_R32G32B32A32_UINT
    16, // FORMAT_R32G32B32A32_SINT
    16, // FORMAT_R32G32B32A32_SFLOAT
    8,  // FORMAT_R64_UINT
    8,  // FORMAT_R64_SINT
    8,  // FORMAT_R64_SFLOAT
    16, // FORMAT_R64G64_UINT
    16, // FORMAT_R64G64_SINT
    16, // FORMAT_R64G64_SFLOAT
    24, // FORMAT_R64G64B64_UINT
    24, // FORMAT_R64G64B64_SINT
    24, // FORMAT_R64G64B64_SFLOAT
    32, // FORMAT_R64G64B64A64_UINT
    32, // FORMAT_R64G64B64A64_SINT
    32, // FORMAT_R64G64B64A64_SFLOAT
    4,  // FORMAT_B10G11R11_UFLOAT_PACK32
    4,  // FORMAT_E5B9G9R9_UFLOAT_PACK32
    2,  // FORMAT_D16_UNORM
    4,  // FORMAT_X8_D24_UNORM_PACK32
    4,  // FORMAT_D32_SFLOAT
    1,  // FORMAT_S8_UINT
    3,  // FORMAT_D16_UNORM_S8_UINT
    4,  // FORMAT_D24_UNORM_S8_UINT
    5,  // FORMAT_D32_SFLOAT_S8_UINT
    1,  // FORMAT_BC1_RGB_UNORM_BLOCK
    1,  // FORMAT_BC1_RGB_SRGB_BLOCK
    1,  // FORMAT_BC1_RGBA_UNORM_BLOCK
    1,  // FORMAT_BC1_RGBA_SRGB_BLOCK
    1,  // FORMAT_BC2_UNORM_BLOCK
    1,  // FORMAT_BC2_SRGB_BLOCK
    1,  // FORMAT_BC3_UNORM_BLOCK
    1,  // FORMAT_BC3_SRGB_BLOCK
    1,  // FORMAT_BC4_UNORM_BLOCK
    1,  // FORMAT_BC4_SNORM_BLOCK
    1,  // FORMAT_BC5_UNORM_BLOCK
    1,  // FORMAT_BC5_SNORM_BLOCK
    1,  // FORMAT_BC6H_UFLOAT_BLOCK
    1,  // FORMAT_BC6H_SFLOAT_BLOCK
    1,  // FORMAT_BC7_UNORM_BLOCK
    1,  // FORMAT_BC7_SRGB_BLOCK
    1,  // FORMAT_ETC2_R8G8B8_UNORM_BLOCK
    1,  // FORMAT_ETC2_R8G8B8_SRGB_BLOCK
    1,  // FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK
    1,  // FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK
    1,  // FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
    1,  // FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    1,  // FORMAT_EAC_R11_UNORM_BLOCK
    1,  // FORMAT_EAC_R11_SNORM_BLOCK
    1,  // FORMAT_EAC_R11G11_UNORM_BLOCK
    1,  // FORMAT_EAC_R11G11_SNORM_BLOCK
    1,  // FORMAT_ASTC_4x4_UNORM_BLOCK
    1,  // FORMAT_ASTC_4x4_SRGB_BLOCK
    1,  // FORMAT_ASTC_5x4_UNORM_BLOCK
    1,  // FORMAT_ASTC_5x4_SRGB_BLOCK
    1,  // FORMAT_ASTC_5x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_5x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_6x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_6x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_6x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_6x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x8_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x8_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x8_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x8_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x10_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x10_SRGB_BLOCK
    1,  // FORMAT_ASTC_12x10_UNORM_BLOCK
    1,  // FORMAT_ASTC_12x10_SRGB_BLOCK
    1,  // FORMAT_ASTC_12x12_UNORM_BLOCK
    1,  // FORMAT_ASTC_12x12_SRGB_BLOCK
    0   // MAX_IMAGE_FORMAT
};

const uint32_t TBOX::TBOXEmu::BYTES_PER_TEXEL_IN_L1[] = {
    0,  // FORMAT_UNDEFINED
    4,  // FORMAT_R4G4_UNORM_PACK8
    4,  // FORMAT_R4G4B4A4_UNORM_PACK16
    4,  // FORMAT_B4G4R4A4_UNORM_PACK16
    4,  // FORMAT_R5G6B5_UNORM_PACK16
    4,  // FORMAT_B5G6R5_UNORM_PACK16
    4,  // FORMAT_R5G5B5A1_UNORM_PACK16
    4,  // FORMAT_B5G5R5A1_UNORM_PACK16
    4,  // FORMAT_A1R5G5B5_UNORM_PACK16
    4,  // FORMAT_R8_UNORM
    4,  // FORMAT_R8_SNORM
    4,  // FORMAT_R8_USCALED
    4,  // FORMAT_R8_SSCALED
    4,  // FORMAT_R8_UINT
    4,  // FORMAT_R8_SINT
    4,  // FORMAT_R8_SRGB
    4,  // FORMAT_R8G8_UNORM
    4,  // FORMAT_R8G8_SNORM
    4,  // FORMAT_R8G8_USCALED
    4,  // FORMAT_R8G8_SSCALED
    4,  // FORMAT_R8G8_UINT
    4,  // FORMAT_R8G8_SINT
    4,  // FORMAT_R8G8_SRGB
    4,  // FORMAT_R8G8B8_UNORM
    4,  // FORMAT_R8G8B8_SNORM
    4,  // FORMAT_R8G8B8_USCALED
    4,  // FORMAT_R8G8B8_SSCALED
    4,  // FORMAT_R8G8B8_UINT
    4,  // FORMAT_R8G8B8_SINT
    8,  // FORMAT_R8G8B8_SRGB
    4,  // FORMAT_B8G8R8_UNORM
    4,  // FORMAT_B8G8R8_SNORM
    4,  // FORMAT_B8G8R8_USCALED
    4,  // FORMAT_B8G8R8_SSCALED
    4,  // FORMAT_B8G8R8_UINT
    4,  // FORMAT_B8G8R8_SINT
    8,  // FORMAT_B8G8R8_SRGB
    4,  // FORMAT_R8G8B8A8_UNORM
    4,  // FORMAT_R8G8B8A8_SNORM
    4,  // FORMAT_R8G8B8A8_USCALED
    4,  // FORMAT_R8G8B8A8_SSCALED
    4,  // FORMAT_R8G8B8A8_UINT
    4,  // FORMAT_R8G8B8A8_SINT
    8,  // FORMAT_R8G8B8A8_SRGB
    4,  // FORMAT_B8G8R8A8_UNORM
    4,  // FORMAT_B8G8R8A8_SNORM
    4,  // FORMAT_B8G8R8A8_USCALED
    4,  // FORMAT_B8G8R8A8_SSCALED
    4,  // FORMAT_B8G8R8A8_UINT
    4,  // FORMAT_B8G8R8A8_SINT
    8,  // FORMAT_B8G8R8A8_SRGB
    4,  // FORMAT_A8B8G8R8_UNORM_PACK32
    4,  // FORMAT_A8B8G8R8_SNORM_PACK32
    4,  // FORMAT_A8B8G8R8_USCALED_PACK32
    4,  // FORMAT_A8B8G8R8_SSCALED_PACK32
    4,  // FORMAT_A8B8G8R8_UINT_PACK32
    4,  // FORMAT_A8B8G8R8_SINT_PACK32
    8,  // FORMAT_A8B8G8R8_SRGB_PACK32
    4,  // FORMAT_A2R10G10B10_UNORM_PACK32
    4,  // FORMAT_A2R10G10B10_SNORM_PACK32
    4,  // FORMAT_A2R10G10B10_USCALED_PACK32
    4,  // FORMAT_A2R10G10B10_SSCALED_PACK32
    4,  // FORMAT_A2R10G10B10_UINT_PACK32
    4,  // FORMAT_A2R10G10B10_SINT_PACK32
    4,  // FORMAT_A2B10G10R10_UNORM_PACK32
    4,  // FORMAT_A2B10G10R10_SNORM_PACK32
    4,  // FORMAT_A2B10G10R10_USCALED_PACK32
    4,  // FORMAT_A2B10G10R10_SSCALED_PACK32
    4,  // FORMAT_A2B10G10R10_UINT_PACK32
    4,  // FORMAT_A2B10G10R10_SINT_PACK32
    4,  // FORMAT_R16_UNORM
    4,  // FORMAT_R16_SNORM
    4,  // FORMAT_R16_USCALED
    4,  // FORMAT_R16_SSCALED
    4,  // FORMAT_R16_UINT
    4,  // FORMAT_R16_SINT
    4,  // FORMAT_R16_SFLOAT
    8,  // FORMAT_R16G16_UNORM
    8,  // FORMAT_R16G16_SNORM
    4,  // FORMAT_R16G16_USCALED
    4,  // FORMAT_R16G16_SSCALED
    4,  // FORMAT_R16G16_UINT
    4,  // FORMAT_R16G16_SINT
    4,  // FORMAT_R16G16_SFLOAT
    16, // FORMAT_R16G16B16_UNORM
    16, // FORMAT_R16G16B16_SNORM
    8,  // FORMAT_R16G16B16_USCALED
    8,  // FORMAT_R16G16B16_SSCALED
    8,  // FORMAT_R16G16B16_UINT
    8,  // FORMAT_R16G16B16_SINT
    8,  // FORMAT_R16G16B16_SFLOAT
    16, // FORMAT_R16G16B16A16_UNORM
    16, // FORMAT_R16G16B16A16_SNORM
    8,  // FORMAT_R16G16B16A16_USCALED
    8,  // FORMAT_R16G16B16A16_SSCALED
    8,  // FORMAT_R16G16B16A16_UINT
    8,  // FORMAT_R16G16B16A16_SINT
    8,  // FORMAT_R16G16B16A16_SFLOAT
    4,  // FORMAT_R32_UINT
    4,  // FORMAT_R32_SINT
    4,  // FORMAT_R32_SFLOAT
    8,  // FORMAT_R32G32_UINT
    8,  // FORMAT_R32G32_SINT
    8,  // FORMAT_R32G32_SFLOAT
    16, // FORMAT_R32G32B32_UINT
    16, // FORMAT_R32G32B32_SINT
    16, // FORMAT_R32G32B32_SFLOAT
    16, // FORMAT_R32G32B32A32_UINT
    16, // FORMAT_R32G32B32A32_SINT
    16, // FORMAT_R32G32B32A32_SFLOAT
    8,  // FORMAT_R64_UINT
    8,  // FORMAT_R64_SINT
    8,  // FORMAT_R64_SFLOAT
    16, // FORMAT_R64G64_UINT
    16, // FORMAT_R64G64_SINT
    16, // FORMAT_R64G64_SFLOAT
    24, // FORMAT_R64G64B64_UINT
    24, // FORMAT_R64G64B64_SINT
    24, // FORMAT_R64G64B64_SFLOAT
    32, // FORMAT_R64G64B64A64_UINT
    32, // FORMAT_R64G64B64A64_SINT
    32, // FORMAT_R64G64B64A64_SFLOAT
    8,  // FORMAT_B10G11R11_UFLOAT_PACK32
    8,  // FORMAT_E5B9G9R9_UFLOAT_PACK32
    4,  // FORMAT_D16_UNORM
    4,  // FORMAT_X8_D24_UNORM_PACK32
    4,  // FORMAT_D32_SFLOAT
    4,  // FORMAT_S8_UINT
    4,  // FORMAT_D16_UNORM_S8_UINT
    4,  // FORMAT_D24_UNORM_S8_UINT
    8,  // FORMAT_D32_SFLOAT_S8_UINT
    4,  // FORMAT_BC1_RGB_UNORM_BLOCK
    8,  // FORMAT_BC1_RGB_SRGB_BLOCK
    4,  // FORMAT_BC1_RGBA_UNORM_BLOCK
    8,  // FORMAT_BC1_RGBA_SRGB_BLOCK
    4,  // FORMAT_BC2_UNORM_BLOCK
    8,  // FORMAT_BC2_SRGB_BLOCK
    4,  // FORMAT_BC3_UNORM_BLOCK
    8,  // FORMAT_BC3_SRGB_BLOCK
    4,  // FORMAT_BC4_UNORM_BLOCK
    4,  // FORMAT_BC4_SNORM_BLOCK
    4,  // FORMAT_BC5_UNORM_BLOCK
    4,  // FORMAT_BC5_SNORM_BLOCK
    8,  // FORMAT_BC6H_UFLOAT_BLOCK
    8,  // FORMAT_BC6H_SFLOAT_BLOCK
    4,  // FORMAT_BC7_UNORM_BLOCK
    8,  // FORMAT_BC7_SRGB_BLOCK
    4,  // FORMAT_ETC2_R8G8B8_UNORM_BLOCK
    8,  // FORMAT_ETC2_R8G8B8_SRGB_BLOCK
    4,  // FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK
    8,  // FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK
    4,  // FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
    8,  // FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
    4,  // FORMAT_EAC_R11_UNORM_BLOCK
    4,  // FORMAT_EAC_R11_SNORM_BLOCK
    8,  // FORMAT_EAC_R11G11_UNORM_BLOCK
    8,  // FORMAT_EAC_R11G11_SNORM_BLOCK
    1,  // FORMAT_ASTC_4x4_UNORM_BLOCK
    1,  // FORMAT_ASTC_4x4_SRGB_BLOCK
    1,  // FORMAT_ASTC_5x4_UNORM_BLOCK
    1,  // FORMAT_ASTC_5x4_SRGB_BLOCK
    1,  // FORMAT_ASTC_5x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_5x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_6x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_6x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_6x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_6x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_8x8_UNORM_BLOCK
    1,  // FORMAT_ASTC_8x8_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x5_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x5_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x6_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x6_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x8_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x8_SRGB_BLOCK
    1,  // FORMAT_ASTC_10x10_UNORM_BLOCK
    1,  // FORMAT_ASTC_10x10_SRGB_BLOCK
    1,  // FORMAT_ASTC_12x10_UNORM_BLOCK
    1,  // FORMAT_ASTC_12x10_SRGB_BLOCK
    1,  // FORMAT_ASTC_12x12_UNORM_BLOCK
    1,  // FORMAT_ASTC_12x12_SRGB_BLOCK
    0   // MAX_IMAGE_FORMAT
};

const TBOX::TBOXEmu::EncodeSRGBFP16 TBOX::TBOXEmu::SRGB2LINEAR_TABLE[256] =
{
    {{0, 0x00, 0x0, 0}},
    {{0, 0x20, 0x3, 0}},
    {{0, 0x20, 0x4, 0}},
    {{0, 0x70, 0x4, 0}},
    {{0, 0x20, 0x5, 0}},
    {{0, 0x48, 0x5, 0}},
    {{0, 0x70, 0x5, 0}},
    {{0, 0x0c, 0x6, 0}},
    {{0, 0x20, 0x6, 0}},
    {{0, 0x34, 0x6, 0}},
    {{0, 0x48, 0x6, 0}},
    {{0, 0x5c, 0x6, 0}},
    {{0, 0x70, 0x6, 0}},
    {{0, 0x04, 0x7, 0}},
    {{0, 0x10, 0x7, 0}},
    {{0, 0x1c, 0x7, 0}},
    {{0, 0x2a, 0x7, 0}},
    {{0, 0x38, 0x7, 0}},
    {{0, 0x46, 0x7, 0}},
    {{0, 0x56, 0x7, 0}},
    {{0, 0x66, 0x7, 0}},
    {{0, 0x76, 0x7, 0}},
    {{0, 0x03, 0x8, 0}},
    {{0, 0x0c, 0x8, 0}},
    {{0, 0x16, 0x8, 0}},
    {{0, 0x1f, 0x8, 0}},
    {{0, 0x29, 0x8, 0}},
    {{0, 0x34, 0x8, 0}},
    {{0, 0x3e, 0x8, 0}},
    {{0, 0x49, 0x8, 0}},
    {{0, 0x55, 0x8, 0}},
    {{0, 0x60, 0x8, 0}},
    {{0, 0x6c, 0x8, 0}},
    {{0, 0x7a, 0x8, 0}},
    {{0, 0x03, 0x9, 0}},
    {{0, 0x0a, 0x9, 0}},
    {{0, 0x11, 0x9, 0}},
    {{0, 0x18, 0x9, 0}},
    {{0, 0x1f, 0x9, 0}},
    {{0, 0x26, 0x9, 0}},
    {{0, 0x2e, 0x9, 0}},
    {{0, 0x36, 0x9, 0}},
    {{0, 0x3e, 0x9, 0}},
    {{0, 0x46, 0x9, 0}},
    {{0, 0x4e, 0x9, 0}},
    {{0, 0x57, 0x9, 0}},
    {{0, 0x60, 0x9, 0}},
    {{0, 0x69, 0x9, 0}},
    {{0, 0x72, 0x9, 0}},
    {{0, 0x7c, 0x9, 0}},
    {{0, 0x03, 0xa, 0}},
    {{0, 0x08, 0xa, 0}},
    {{0, 0x0d, 0xa, 0}},
    {{0, 0x12, 0xa, 0}},
    {{0, 0x17, 0xa, 0}},
    {{0, 0x1c, 0xa, 0}},
    {{0, 0x22, 0xa, 0}},
    {{0, 0x28, 0xa, 0}},
    {{0, 0x2d, 0xa, 0}},
    {{0, 0x33, 0xa, 0}},
    {{0, 0x39, 0xa, 0}},
    {{0, 0x3f, 0xa, 0}},
    {{0, 0x45, 0xa, 0}},
    {{0, 0x4c, 0xa, 0}},
    {{0, 0x52, 0xa, 0}},
    {{0, 0x58, 0xa, 0}},
    {{0, 0x60, 0xa, 0}},
    {{0, 0x66, 0xa, 0}},
    {{0, 0x6c, 0xa, 0}},
    {{0, 0x74, 0xa, 0}},
    {{0, 0x7a, 0xa, 0}},
    {{0, 0x01, 0xb, 0}},
    {{0, 0x05, 0xb, 0}},
    {{0, 0x08, 0xb, 0}},
    {{0, 0x0c, 0xb, 0}},
    {{0, 0x10, 0xb, 0}},
    {{0, 0x14, 0xb, 0}},
    {{0, 0x18, 0xb, 0}},
    {{0, 0x1c, 0xb, 0}},
    {{0, 0x20, 0xb, 0}},
    {{0, 0x24, 0xb, 0}},
    {{0, 0x29, 0xb, 0}},
    {{0, 0x2d, 0xb, 0}},
    {{0, 0x31, 0xb, 0}},
    {{0, 0x36, 0xb, 0}},
    {{0, 0x3a, 0xb, 0}},
    {{0, 0x3f, 0xb, 0}},
    {{0, 0x43, 0xb, 0}},
    {{0, 0x48, 0xb, 0}},
    {{0, 0x4d, 0xb, 0}},
    {{0, 0x51, 0xb, 0}},
    {{0, 0x56, 0xb, 0}},
    {{0, 0x5b, 0xb, 0}},
    {{0, 0x60, 0xb, 0}},
    {{0, 0x65, 0xb, 0}},
    {{0, 0x6a, 0xb, 0}},
    {{0, 0x70, 0xb, 0}},
    {{0, 0x74, 0xb, 0}},
    {{0, 0x7a, 0xb, 0}},
    {{0, 0x00, 0xc, 0}},
    {{0, 0x02, 0xc, 0}},
    {{0, 0x05, 0xc, 0}},
    {{0, 0x08, 0xc, 0}},
    {{0, 0x0b, 0xc, 0}},
    {{0, 0x0e, 0xc, 0}},
    {{0, 0x11, 0xc, 0}},
    {{0, 0x14, 0xc, 0}},
    {{0, 0x17, 0xc, 0}},
    {{0, 0x1a, 0xc, 0}},
    {{0, 0x1d, 0xc, 0}},
    {{0, 0x20, 0xc, 0}},
    {{0, 0x23, 0xc, 0}},
    {{0, 0x26, 0xc, 0}},
    {{0, 0x29, 0xc, 0}},
    {{0, 0x2c, 0xc, 0}},
    {{0, 0x30, 0xc, 0}},
    {{0, 0x33, 0xc, 0}},
    {{0, 0x36, 0xc, 0}},
    {{0, 0x3a, 0xc, 0}},
    {{0, 0x3d, 0xc, 0}},
    {{0, 0x40, 0xc, 0}},
    {{0, 0x44, 0xc, 0}},
    {{0, 0x47, 0xc, 0}},
    {{0, 0x4b, 0xc, 0}},
    {{0, 0x4e, 0xc, 0}},
    {{0, 0x52, 0xc, 0}},
    {{0, 0x56, 0xc, 0}},
    {{0, 0x59, 0xc, 0}},
    {{0, 0x5d, 0xc, 0}},
    {{0, 0x61, 0xc, 0}},
    {{0, 0x65, 0xc, 0}},
    {{0, 0x68, 0xc, 0}},
    {{0, 0x6c, 0xc, 0}},
    {{0, 0x70, 0xc, 0}},
    {{0, 0x74, 0xc, 0}},
    {{0, 0x78, 0xc, 0}},
    {{0, 0x7c, 0xc, 0}},
    {{0, 0x00, 0xd, 0}},
    {{0, 0x02, 0xd, 0}},
    {{0, 0x04, 0xd, 0}},
    {{0, 0x06, 0xd, 0}},
    {{0, 0x08, 0xd, 0}},
    {{0, 0x0a, 0xd, 0}},
    {{0, 0x0d, 0xd, 0}},
    {{0, 0x0f, 0xd, 0}},
    {{0, 0x11, 0xd, 0}},
    {{0, 0x13, 0xd, 0}},
    {{0, 0x15, 0xd, 0}},
    {{0, 0x18, 0xd, 0}},
    {{0, 0x1a, 0xd, 0}},
    {{0, 0x1c, 0xd, 0}},
    {{0, 0x1e, 0xd, 0}},
    {{0, 0x21, 0xd, 0}},
    {{0, 0x23, 0xd, 0}},
    {{0, 0x25, 0xd, 0}},
    {{0, 0x28, 0xd, 0}},
    {{0, 0x2a, 0xd, 0}},
    {{0, 0x2d, 0xd, 0}},
    {{0, 0x2f, 0xd, 0}},
    {{0, 0x32, 0xd, 0}},
    {{0, 0x34, 0xd, 0}},
    {{0, 0x36, 0xd, 0}},
    {{0, 0x39, 0xd, 0}},
    {{0, 0x3c, 0xd, 0}},
    {{0, 0x3e, 0xd, 0}},
    {{0, 0x41, 0xd, 0}},
    {{0, 0x43, 0xd, 0}},
    {{0, 0x46, 0xd, 0}},
    {{0, 0x48, 0xd, 0}},
    {{0, 0x4b, 0xd, 0}},
    {{0, 0x4e, 0xd, 0}},
    {{0, 0x51, 0xd, 0}},
    {{0, 0x53, 0xd, 0}},
    {{0, 0x56, 0xd, 0}},
    {{0, 0x59, 0xd, 0}},
    {{0, 0x5b, 0xd, 0}},
    {{0, 0x5e, 0xd, 0}},
    {{0, 0x61, 0xd, 0}},
    {{0, 0x64, 0xd, 0}},
    {{0, 0x67, 0xd, 0}},
    {{0, 0x6a, 0xd, 0}},
    {{0, 0x6d, 0xd, 0}},
    {{0, 0x70, 0xd, 0}},
    {{0, 0x72, 0xd, 0}},
    {{0, 0x76, 0xd, 0}},
    {{0, 0x78, 0xd, 0}},
    {{0, 0x7c, 0xd, 0}},
    {{0, 0x7e, 0xd, 0}},
    {{0, 0x01, 0xe, 0}},
    {{0, 0x02, 0xe, 0}},
    {{0, 0x04, 0xe, 0}},
    {{0, 0x05, 0xe, 0}},
    {{0, 0x07, 0xe, 0}},
    {{0, 0x09, 0xe, 0}},
    {{0, 0x0a, 0xe, 0}},
    {{0, 0x0c, 0xe, 0}},
    {{0, 0x0d, 0xe, 0}},
    {{0, 0x0f, 0xe, 0}},
    {{0, 0x11, 0xe, 0}},
    {{0, 0x12, 0xe, 0}},
    {{0, 0x14, 0xe, 0}},
    {{0, 0x16, 0xe, 0}},
    {{0, 0x17, 0xe, 0}},
    {{0, 0x19, 0xe, 0}},
    {{0, 0x1b, 0xe, 0}},
    {{0, 0x1c, 0xe, 0}},
    {{0, 0x1e, 0xe, 0}},
    {{0, 0x20, 0xe, 0}},
    {{0, 0x21, 0xe, 0}},
    {{0, 0x23, 0xe, 0}},
    {{0, 0x25, 0xe, 0}},
    {{0, 0x27, 0xe, 0}},
    {{0, 0x29, 0xe, 0}},
    {{0, 0x2a, 0xe, 0}},
    {{0, 0x2c, 0xe, 0}},
    {{0, 0x2e, 0xe, 0}},
    {{0, 0x30, 0xe, 0}},
    {{0, 0x32, 0xe, 0}},
    {{0, 0x33, 0xe, 0}},
    {{0, 0x35, 0xe, 0}},
    {{0, 0x37, 0xe, 0}},
    {{0, 0x39, 0xe, 0}},
    {{0, 0x3b, 0xe, 0}},
    {{0, 0x3d, 0xe, 0}},
    {{0, 0x3f, 0xe, 0}},
    {{0, 0x41, 0xe, 0}},
    {{0, 0x43, 0xe, 0}},
    {{0, 0x45, 0xe, 0}},
    {{0, 0x47, 0xe, 0}},
    {{0, 0x49, 0xe, 0}},
    {{0, 0x4b, 0xe, 0}},
    {{0, 0x4d, 0xe, 0}},
    {{0, 0x4f, 0xe, 0}},
    {{0, 0x51, 0xe, 0}},
    {{0, 0x53, 0xe, 0}},
    {{0, 0x55, 0xe, 0}},
    {{0, 0x57, 0xe, 0}},
    {{0, 0x59, 0xe, 0}},
    {{0, 0x5b, 0xe, 0}},
    {{0, 0x5d, 0xe, 0}},
    {{0, 0x5f, 0xe, 0}},
    {{0, 0x61, 0xe, 0}},
    {{0, 0x63, 0xe, 0}},
    {{0, 0x65, 0xe, 0}},
    {{0, 0x68, 0xe, 0}},
    {{0, 0x6a, 0xe, 0}},
    {{0, 0x6c, 0xe, 0}},
    {{0, 0x6e, 0xe, 0}},
    {{0, 0x70, 0xe, 0}},
    {{0, 0x72, 0xe, 0}},
    {{0, 0x74, 0xe, 0}},
    {{0, 0x76, 0xe, 0}},
    {{0, 0x7a, 0xe, 0}},
    {{0, 0x7c, 0xe, 0}},
    {{0, 0x7e, 0xe, 0}},
    {{0, 0x00, 0xf, 0}}
};

uint16_t TBOX::TBOXEmu::cast_bytes_to_uint16(uint8_t *src)
{
    return uint16_t(src[0]) | (uint16_t(src[1]) << 8);
}

uint32_t TBOX::TBOXEmu::cast_bytes_to_uint24(uint8_t *src)
{
    return uint32_t(src[0]) | (uint32_t(src[1]) << 8) | (uint32_t(src[2]) << 16);
}

uint32_t TBOX::TBOXEmu::cast_bytes_to_uint32(uint8_t *src)
{
    return uint32_t(src[0]) | (uint32_t(src[1]) << 8) | (uint32_t(src[2]) << 16) | (uint32_t(src[3]) << 24);
}

uint64_t TBOX::TBOXEmu::cast_bytes_to_uint64(uint8_t *src)
{
    return (uint64_t(src[0]) <<  0) | (uint64_t(src[1]) <<  8) | (uint64_t(src[2]) << 16) | (uint64_t(src[3]) << 24)
         | (uint64_t(src[4]) << 32) | (uint64_t(src[5]) << 40) | (uint64_t(src[6]) << 48) | (uint64_t(src[7]) << 56);
}

float TBOX::TBOXEmu::cast_bytes_to_float(uint8_t *src)
{
    return fpu::FLT(cast_bytes_to_uint32(src));
}

void TBOX::TBOXEmu::memcpy_uint16(uint8_t *dst, uint16_t src)
{
    dst[0] = (src >>  0) & 0xff;
    dst[1] = (src >>  8) & 0xff;
}

void TBOX::TBOXEmu::memcpy_uint32(uint8_t *dst, uint32_t src)
{
    dst[0] = (src >>  0) & 0xff;
    dst[1] = (src >>  8) & 0xff;
    dst[2] = (src >> 16) & 0xff;
    dst[3] = (src >> 24) & 0xff;
}

void TBOX::TBOXEmu::memcpy_uint64(uint8_t *dst, uint64_t src)
{
    dst[0] = (src >>  0) & 0xff;
    dst[1] = (src >>  8) & 0xff;
    dst[2] = (src >> 16) & 0xff;
    dst[3] = (src >> 24) & 0xff;
    dst[4] = (src >> 32) & 0xff;
    dst[5] = (src >> 40) & 0xff;
    dst[6] = (src >> 48) & 0xff;
    dst[7] = (src >> 56) & 0xff;
}


TBOX::TBOXEmu::TBOXEmu()
{
    imageTableAddress = 0;
    num_total_l2_requests = 0;
    for (uint32_t t = 0; t < EMU_NUM_THREADS; t++)
    {
        request_pending[t] = false;
        num_new_l2_requests[t] = 0;
        num_created_l2_requests[t] = 0;
        num_pending_l2_requests[t] = 0;
    }
    for (uint32_t e = 0; e < MAX_L2_REQUESTS; e++)
        l2_requests[e].free = true;
}

/* Receives two parts of 64 bits */
void TBOX::TBOXEmu::set_request_header(uint32_t thread, uint64_t src1, uint64_t src2)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    currentRequest[thread].data[0] = src2;
    currentRequest[thread].data[1] = src1;
}

/* Receives the header of the Sample Request */
void TBOX::TBOXEmu::set_request_header(uint32_t thread, SampleRequest header)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    currentRequest[thread] = header;
}

void TBOX::TBOXEmu::set_request_coordinates(uint32_t thread, uint32_t idx, freg_t coord)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    if (idx > 5)
        throw std::runtime_error("Unsupported coordinate index (> 5)");

    input[thread][idx] = coord;
}

/* idx == component R (0), G (1), B (2) or A (3)*/
freg_t TBOX::TBOXEmu::get_request_results(uint32_t thread, uint32_t idx)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    if (idx > 4)
        throw std::runtime_error("Unsupported result index (> 4)");

    return output[thread][idx];
}

/* 
    This function return the TBOX results with packed channels 
*/
unsigned TBOX::TBOXEmu::get_request_results(uint32_t thread, freg_t* data)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    
    unsigned out_channel = 0;
    bool channel_in_result = true; // Packing disabled
    for(unsigned channel = 0; channel < 4; channel++)
    {
        if(channel_in_result)
        {
            data[out_channel++] = output[thread][channel];
        }
    }
    return out_channel;
}

void TBOX::TBOXEmu::set_request_pending(uint32_t thread, bool value)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    request_pending[thread] = value;
}

bool TBOX::TBOXEmu::check_request_pending(uint32_t thread)
{
    if (thread >= EMU_NUM_THREADS)
        throw std::runtime_error("Thread id out-of-range");

    return request_pending[thread];
}

void TBOX::TBOXEmu::set_image_table_address(uint64_t address) { imageTableAddress = address; }

uint64_t TBOX::TBOXEmu::get_image_table_address() { return imageTableAddress; }

void TBOX::TBOXEmu::print_sample_request(uint32_t thread) const { print_sample_request(currentRequest[thread]); }

void TBOX::TBOXEmu::texture_cache_initialize()
{
    for (uint32_t b = 0; b < TEXTURE_CACHE_BANKS; b++)
        for (uint32_t l = 0; l < TEXTURE_CACHE_LINES_PER_BANK; l++)
        {
            textureCacheValid[b][l] = false;
            textureCacheLRU[b][l] = l;
        }
}

void TBOX::TBOXEmu::texture_cache_update_lru(uint32_t bank, uint32_t access_way)
{
    bool found_access = false;
    uint32_t l;
    for (l = 0; !found_access && (l < TEXTURE_CACHE_LINES_PER_BANK); l++)
        found_access = (textureCacheLRU[bank][l] == access_way);

    for (l--; l < (TEXTURE_CACHE_LINES_PER_BANK - 1); l++)
        textureCacheLRU[bank][l] = textureCacheLRU[bank][l + 1];
    textureCacheLRU[bank][TEXTURE_CACHE_LINES_PER_BANK - 1] = access_way;
}

uint32_t TBOX::TBOXEmu::texture_cache_get_lru(uint32_t bank)
{
    uint32_t lru_way = textureCacheLRU[bank][0];
    for (uint32_t l = 0; l < (TEXTURE_CACHE_LINES_PER_BANK - 1); l++)
        textureCacheLRU[bank][l] = textureCacheLRU[bank][l + 1];
    textureCacheLRU[bank][TEXTURE_CACHE_LINES_PER_BANK - 1] = lru_way;

    return lru_way;
}

bool TBOX::TBOXEmu::texture_cache_lookup(int32_t bank, uint64_t tag, uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE])
{
    bool hit = false;
    uint32_t l = 0;
    for (; !hit && (l < TEXTURE_CACHE_LINES_PER_BANK); l++)
        hit = textureCacheValid[bank][l] && (textureCacheTags[bank][l] == tag);

    if (hit)
    {
        uint32_t access_way = l - 1;

        LOG(DEBUG, "\tTexture Cache hit at bank %d way %u for tag %" PRIx64, bank, access_way, tag);
        LOG(DEBUG, "%s", "\tData:");
        for (uint32_t q = 0; q < TEXTURE_CACHE_QWORDS_PER_LINE; q++)
        {
            LOG(DEBUG, "\t%" PRIx64, textureCacheData[bank][access_way][q]);
            data[q] = textureCacheData[bank][access_way][q];
        }

        texture_cache_update_lru(bank, access_way);
    }

    return hit;
}

void TBOX::TBOXEmu::texture_cache_fill(int32_t bank, uint64_t tag, uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE])
{
    uint8_t victim_way;
    bool invalid_way = false;
    for (uint32_t l = 0; !invalid_way && (l < TEXTURE_CACHE_LINES_PER_BANK); l++)
    {
        if (!textureCacheValid[bank][l])
        {
            victim_way = l;
            invalid_way = true;
        }
    }

    if (invalid_way)
        texture_cache_update_lru(bank, victim_way);
    else
        victim_way = texture_cache_get_lru(bank);

    LOG(DEBUG, "\tTexture Cache fill at bank %d way %d with tag %016lx", bank, victim_way, tag);

    textureCacheValid[bank][victim_way] = true;
    textureCacheTags[bank][victim_way] = tag;

    for (uint32_t q = 0; q < TEXTURE_CACHE_QWORDS_PER_LINE; q++)
        textureCacheData[bank][victim_way][q] = data[q];
}

void TBOX::TBOXEmu::image_info_cache_initialize()
{
    for (uint32_t l = 0; l < IMAGE_INFO_CACHE_SIZE; l++)
    {
        imageInfoCacheValid[l] = false;
        imageInfoCacheLRU[l] = l;
    }
}

void TBOX::TBOXEmu::image_info_cache_update_lru(uint32_t access_way)
{
    bool found_access = false;
    uint32_t l;
    for (l = 0; !found_access && (l < IMAGE_INFO_CACHE_SIZE); l++)
        found_access = (imageInfoCacheLRU[l] == access_way);

    for (l--; l < (IMAGE_INFO_CACHE_SIZE - 1); l++)
        imageInfoCacheLRU[l] = imageInfoCacheLRU[l + 1];
    imageInfoCacheLRU[IMAGE_INFO_CACHE_SIZE - 1] = access_way;
}

uint32_t TBOX::TBOXEmu::image_info_cache_get_lru()
{
    uint32_t lru_way = imageInfoCacheLRU[0];
    for (uint32_t l = 0; l < (IMAGE_INFO_CACHE_SIZE - 1); l++)
        imageInfoCacheLRU[l] = imageInfoCacheLRU[l + 1];
    imageInfoCacheLRU[IMAGE_INFO_CACHE_SIZE - 1] = lru_way;

    return lru_way;
}

bool TBOX::TBOXEmu::image_info_cache_lookup(uint32_t tag, ImageInfo &data)
{
    bool hit = false;
    uint32_t l = 0;
    for (; !hit && (l < IMAGE_INFO_CACHE_SIZE); l++)
        hit = imageInfoCacheValid[l] && (imageInfoCacheTags[l] == tag);

    if (hit)
    {
        uint32_t access_way = l - 1;

        LOG(DEBUG, "\tImage Descriptor Cache hit at way %d for tag %d", access_way, tag);

        data = imageInfoCache[access_way];
        image_info_cache_update_lru(access_way);
    }

    return hit;
}

void TBOX::TBOXEmu::image_info_cache_fill(uint32_t tag, ImageInfo data)
{
    uint8_t victim_way;
    bool invalid_way = false;
    for (uint32_t l = 0; !invalid_way && (l < IMAGE_INFO_CACHE_SIZE); l++)
    {
        if (!imageInfoCacheValid[l])
        {
            victim_way = l;
            invalid_way = true;
        }
    }

    if (invalid_way)
        image_info_cache_update_lru(victim_way);
    else
        victim_way = image_info_cache_get_lru();

    LOG(DEBUG, "\tImage Descriptor Cache fill at way %d with tag %d", victim_way, tag);

    imageInfoCacheValid[victim_way] = true;
    imageInfoCacheTags[victim_way] = tag;

    imageInfoCache[victim_way] = data;
}

bool TBOX::TBOXEmu::access_memory(uint64_t address, uint64_t &data)
{
#ifdef TBOX_MINION_SIM
    return access_l2(address, data);
#else
    data = bemu::pmemread<uint64_t>(address);
    LOG(DEBUG, "\t\t %016lx <- PMEM64[%016lx]", data, address);
    return true;
#endif
}

bool TBOX::TBOXEmu::access_memory(uint64_t address, uint32_t &data)
{
#ifdef TBOX_MINION_SIM
    return access_l2(address, data);
#else
    data = bemu::pmemread<uint32_t>(address);
    LOG(DEBUG, "\t\t %08x <- PMEM32[%016lx]", data, address);
    return true;
#endif
}

bool TBOX::TBOXEmu::access_l2(uint64_t address, uint64_t &data)
{
    LOG(DEBUG, "\t64-bit L2 access for address %016lx thread %d", address, request_hart);

    uint64_t req_addr_lo =  address      & ~0x3fUL;
    uint64_t req_addr_hi = (address + 7) & ~0x3fUL;

    if (req_addr_lo == req_addr_hi)
        return get_l2_data(address, data);
    else
    {
        LOG(DEBUG, "%s", "\tL2 access split cache line access");

        uint32_t unaligned_size = 8 - (address & 0x7UL);

        uint64_t data_lo, data_hi;
        bool data_ready_hi, data_ready_lo;

        data_ready_lo = get_l2_data( address & ~0x07UL      , data_lo);
        data_ready_hi = get_l2_data((address & ~0x3fUL) + 64, data_hi);

        bool data_ready = data_ready_hi && data_ready_lo;

        if (data_ready)
        {
            data = (data_lo >> (8 * unaligned_size)) + (data_hi << (8 * (8 - unaligned_size)));
            LOG(DEBUG, "\tSplit data ready %016lx", data);
        }

        return data_ready;
    }
}

bool TBOX::TBOXEmu::access_l2(uint64_t address, uint32_t &data)
{
    LOG(DEBUG, "\t32-bit L2 access for address %016lx thread %d", address, request_hart);

    uint64_t req_addr_lo =  address      & ~0x3fUL;
    uint64_t req_addr_hi = (address + 3) & ~0x3fUL;

    if (req_addr_lo == req_addr_hi)
        return get_l2_data(address, data);
    else
    {
        LOG(DEBUG, "%s", "\tL2 access split cache line access");

        uint32_t unaligned_size = 4 - (address & 0x3UL);

        uint32_t data_lo, data_hi;
        bool data_ready_lo, data_ready_hi;

        data_ready_lo = get_l2_data( address & ~0x03UL      , data_lo);
        data_ready_hi = get_l2_data((address & ~0x3fUL) + 64, data_hi);

        bool data_ready = data_ready_hi && data_ready_lo;

        if (data_ready)
        {
            data = (data_lo >> (8 * unaligned_size)) + (data_hi << (8 * (4 - unaligned_size)));
            LOG(DEBUG, "\tSplit data ready %08x", data);
        }

        return data_ready;
    }
}

bool TBOX::TBOXEmu::get_l2_data(uint64_t address, uint64_t &data)
{
    bool found = false;

    // Find matching request to the same cache line
    uint32_t req = 0;
    for (uint32_t proc_reqs = 0; !found && (proc_reqs < num_total_l2_requests) && (req < MAX_L2_REQUESTS); req++)
    {
        if (!l2_requests[req].free)
        {
            found = (l2_requests[req].address == (address & ~0x3fUL));
            proc_reqs++;
        }
    }

    if (found)
    {
        LOG(DEBUG, "\tFound existing L2 request %d for address %016lx", req - 1, address);
        l2_requests[req - 1].thread_mask |= (1 << request_hart);
        if (l2_requests[req - 1].ready)
        {
            uint8_t *data_ptr = &((uint8_t *) l2_requests[req - 1].data)[(address & 0x3fUL)];
            data = *((uint64_t *) data_ptr);
            LOG(DEBUG, "\tData ready %016lx", data);
            return true;
        }

        num_pending_l2_requests[request_hart]++;

        return false;
    }
    else
    {
        create_l2_request(address);
        return false;
    }
}

bool TBOX::TBOXEmu::get_l2_data(uint64_t address, uint32_t &data)
{
    bool found = false;

    // Find matching request to the same cache line
    uint32_t req = 0;
    for (uint32_t proc_reqs = 0; !found && (proc_reqs < num_total_l2_requests) && (req < MAX_L2_REQUESTS); req++)
    {
        if (!l2_requests[req].free)
        {
            found = (l2_requests[req].address == (address & ~0x3fUL));
            proc_reqs++;
        }
    }

    if (found)
    {
        LOG(DEBUG, "\tFound existing L2 request %d for address %016lx", req - 1, address);
        l2_requests[req - 1].thread_mask |= (1 << request_hart);
        if (l2_requests[req - 1].ready)
        {
            uint8_t *data_ptr = &((uint8_t *) l2_requests[req - 1].data)[address & 0x3fUL];
            data = *((uint32_t *) data_ptr);
            LOG(DEBUG, "\tData ready %08x", data);
            return true;
        }

        num_pending_l2_requests[request_hart]++;

        return false;
    }
    else
    {
        create_l2_request(address);
        return false;
    }
}

bool TBOX::TBOXEmu::get_l2_data(uint64_t address, ImageInfo &data)
{
    bool found = false;

    // Find matching request to the same cache line
    uint32_t req = 0;
    for (uint32_t proc_reqs = 0; !found && (proc_reqs < num_total_l2_requests) && (req < MAX_L2_REQUESTS); req++)
    {
        if (!l2_requests[req].free)
        {
            found = (l2_requests[req].address == (address & ~0x3fUL));
            proc_reqs++;
        }
    }

    if (found)
    {
        LOG(DEBUG, "\tFound existing L2 request %d for address %016lx", req - 1, address);
        l2_requests[req - 1].thread_mask |= (1 << request_hart);
        if (l2_requests[req - 1].ready)
        {
            data.data[0] = l2_requests[req - 1].data[4 * ((address >> 5) & 1) + 0];
            data.data[1] = l2_requests[req - 1].data[4 * ((address >> 5) & 1) + 1];
            data.data[2] = l2_requests[req - 1].data[4 * ((address >> 5) & 1) + 2];
            data.data[3] = l2_requests[req - 1].data[4 * ((address >> 5) & 1) + 3];
            return true;
        }

        num_pending_l2_requests[request_hart]++;

        return false;
    }
    else
    {
        create_l2_request(address);
        return false;
    }
}

void TBOX::TBOXEmu::create_l2_request(uint64_t address)
{
    if (num_total_l2_requests == MAX_L2_REQUESTS)
        throw std::runtime_error("No more L2 requests can be stored.");

    uint32_t free_entry = 0;
    while (!l2_requests[free_entry].free && (free_entry < MAX_L2_REQUESTS))
        free_entry++;

    if (free_entry == MAX_L2_REQUESTS)
        throw std::runtime_error("No free L2 request entry found.");

    LOG(DEBUG, "\tCreated new L2 request %d for address %016lx thread %d", free_entry, address & ~0x3fUL, request_hart);

    l2_requests[free_entry].thread_mask  = (1 << request_hart);
    l2_requests[free_entry].address = address & ~0x3fUL;
    l2_requests[free_entry].ready   = false;
    l2_requests[free_entry].free    = false;
    num_total_l2_requests++;
    num_pending_l2_requests[request_hart]++;
    num_created_l2_requests[request_hart]++;
    num_new_l2_requests[request_hart]++;
}

void TBOX::TBOXEmu::clear_l2_requests(uint32_t thread)
{
    uint32_t num_cleared_l2_requests = 0;

    for (uint32_t e = 0; e < MAX_L2_REQUESTS; e++)
    {
        if (!l2_requests[e].free)
        {
            if (l2_requests[e].thread_mask & (1 << thread)) LOG(DEBUG, "\tClear L2 request %d for thread %d", e, thread);

            l2_requests[e].thread_mask &= ~(1 << thread);

            if (l2_requests[e].thread_mask == 0)
            {
                LOG(DEBUG, "\tFree L2 request %d as all threads are cleared", e);
                l2_requests[e].free = true;
                num_total_l2_requests--;
                num_cleared_l2_requests++;
            }
        }
    }
    LOG(DEBUG, "\tClear Thread %d L2 Requests,  Total %d Created %d Cleared %d",
                      thread, num_total_l2_requests, num_created_l2_requests[thread], num_cleared_l2_requests);
    num_new_l2_requests[thread] = 0;
    num_created_l2_requests[thread] = 0;
    num_pending_l2_requests[thread] = 0;
}

void TBOX::TBOXEmu::reset_l2_requests_counters(uint32_t thread)
{
    num_new_l2_requests[thread] = 0;
    num_pending_l2_requests[thread] = 0;
}

//TBOX::TBOXEmu::L2Request* TBOX::TBOXEmu::get_l2_request_queue() const { return l2_requests; }

uint32_t TBOX::TBOXEmu::get_num_new_l2_requests(uint32_t thread) const { return num_new_l2_requests[thread]; }

uint32_t TBOX::TBOXEmu::get_num_pending_l2_requests(uint32_t thread) const { return num_pending_l2_requests[thread]; }

bool TBOX::TBOXEmu::read_image_info_cache_line(uint64_t address, ImageInfo &data)
{
#ifdef TBOX_MINION_SIM
    return get_l2_data(address, data);
#else
    data.data[0] = bemu::pmemread<uint64_t>(address + 0);
    data.data[1] = bemu::pmemread<uint64_t>(address + 8);
    data.data[2] = bemu::pmemread<uint64_t>(address + 16);
    data.data[3] = bemu::pmemread<uint64_t>(address + 24);
    return true;
#endif
}

bool TBOX::TBOXEmu::read_texture_cache_line(ImageInfo currentImage, uint64_t address[4], uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE])
{
    bool data_ready;

    uint64_t readData[TEXTURE_CACHE_QWORDS_PER_LINE];

    data_ready = read_texture_cache_line_data(currentImage, address, readData);

    if (data_ready)
    {
        uint32_t startTexel = 2 * (address[0] & 0x01);
        decompress_texture_cache_line_data(currentImage, startTexel, readData, data);
        return true;
    }
    else
        return false;
}

bool TBOX::TBOXEmu::read_texture_cache_line_data(ImageInfo currentImage, uint64_t address[4], uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE])
{
    ImageFormat fmt = (ImageFormat)currentImage.info.format;

    bool data_ready = true;

    switch (fmt)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
            data_ready = data_ready && access_memory(address[0] & ~0x01, data[0]);
            break;
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC3_UNORM_BLOCK:
            data_ready = data_ready && access_memory( address[0] & ~0x01     , data[0]);
            data_ready = data_ready && access_memory((address[0] & ~0x01) + 8, data[1]);
            break;
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
        case FORMAT_BC4_UNORM_BLOCK:
        case FORMAT_BC4_SNORM_BLOCK:
            data_ready = data_ready && access_memory(address[0] & ~0x01, data[0]);
            break;
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
        case FORMAT_BC5_UNORM_BLOCK:
        case FORMAT_BC5_SNORM_BLOCK:
        case FORMAT_BC6H_UFLOAT_BLOCK:
        case FORMAT_BC6H_SFLOAT_BLOCK:
        case FORMAT_BC7_UNORM_BLOCK:
        case FORMAT_BC7_SRGB_BLOCK:
            data_ready = data_ready && access_memory( address[0] & ~0x01     , data[0]);
            data_ready = data_ready && access_memory((address[0] & ~0x01) + 8, data[1]);
            break;
        case FORMAT_R4G4_UNORM_PACK8:
        case FORMAT_R8_UNORM:
        case FORMAT_R8_SNORM:
        case FORMAT_R8_SRGB:
        case FORMAT_R16G16_UNORM:
        case FORMAT_R16G16_SNORM:
            for (uint32_t l = 0; l < 4; l++)
                data_ready = data_ready && access_memory(address[l], ((uint32_t *) data)[l]);
            break;

        // 4 texels per line, 4 x 16 = 64b
        case FORMAT_R4G4B4A4_UNORM_PACK16:
        case FORMAT_B4G4R4A4_UNORM_PACK16:
        case FORMAT_R5G6B5_UNORM_PACK16:
        case FORMAT_B5G6R5_UNORM_PACK16:
        case FORMAT_R5G5B5A1_UNORM_PACK16:
        case FORMAT_B5G5R5A1_UNORM_PACK16:
        case FORMAT_A1R5G5B5_UNORM_PACK16:
        case FORMAT_R8G8_UNORM:
        case FORMAT_R8G8_SNORM:
        case FORMAT_R8G8_SRGB:
        case FORMAT_R16_UNORM:
        case FORMAT_R16_SNORM:
        case FORMAT_R16_SFLOAT:
        case FORMAT_D16_UNORM:
        // sRGB expanded to FLOAT16 4 channel, 2 texels per line, 2 x 32 = 64b
        case FORMAT_R8G8B8_SRGB:
        case FORMAT_B8G8R8_SRGB:
        case FORMAT_R8G8B8A8_SRGB:
        case FORMAT_B8G8R8A8_SRGB:
        case FORMAT_A8B8G8R8_SRGB_PACK32:
            for (uint32_t l = 0; l < 4; l++)
                data_ready = data_ready && access_memory(address[l], data[l]);
            break;
        case FORMAT_R8G8B8_UNORM:
        case FORMAT_R8G8B8_SNORM:
        case FORMAT_B8G8R8_UNORM:
        case FORMAT_B8G8R8_SNORM:
        case FORMAT_A8B8G8R8_UNORM_PACK32:
        case FORMAT_A8B8G8R8_SNORM_PACK32:
        case FORMAT_A2R10G10B10_UNORM_PACK32:
        case FORMAT_A2R10G10B10_SNORM_PACK32:
        case FORMAT_A2B10G10R10_UNORM_PACK32:
        case FORMAT_A2B10G10R10_SNORM_PACK32:
        case FORMAT_B10G11R11_UFLOAT_PACK32:
        case FORMAT_E5B9G9R9_UFLOAT_PACK32:
        case FORMAT_R16G16_SFLOAT:
        case FORMAT_D16_UNORM_S8_UINT:
        case FORMAT_X8_D24_UNORM_PACK32:
        case FORMAT_D24_UNORM_S8_UINT:
        case FORMAT_R32_SFLOAT:
        case FORMAT_D32_SFLOAT:
            for (uint32_t l = 0; l < 4; l++)
            {
                data_ready = data_ready && access_memory(address[l]    , data[l * 2 + 0]);
                data_ready = data_ready && access_memory(address[l] + 8, data[l * 2 + 1]);
            }
            break;
        case FORMAT_R16G16B16_UNORM:
        case FORMAT_R16G16B16_SNORM:
        case FORMAT_R16G16B16A16_UNORM:
        case FORMAT_R16G16B16A16_SNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                data_ready = data_ready && access_memory(address[l]     , data[l * 2 + 0]);
                data_ready = data_ready && access_memory(address[l] +  8, data[l * 2 + 1]);
            }
            break;
        case FORMAT_R16G16B16_SFLOAT:
        case FORMAT_R16G16B16A16_SFLOAT:
        case FORMAT_R32G32_SFLOAT:
            for (uint32_t l = 0; l < 4; l++)
            {
                data_ready = data_ready && access_memory(address[l]    , data[l * 2 + 0]);
                data_ready = data_ready && access_memory(address[l] + 8, data[l * 2 + 1]);
            }
            break;
        case FORMAT_R32G32B32_SFLOAT:
        case FORMAT_R32G32B32A32_SFLOAT:
            for (uint32_t l = 0; l < 4; l++)
            {
                data_ready = data_ready && access_memory(address[l]     , data[l * 4 + 0]);
                data_ready = data_ready && access_memory(address[l] +  8, data[l * 4 + 1]);
                data_ready = data_ready && access_memory(address[l] + 16, data[l * 4 + 2]);
                data_ready = data_ready && access_memory(address[l] + 24, data[l * 4 + 3]);
            }
            break;
        default:
            for (uint32_t l = 0; l < 4; l++)
                for (uint32_t q = 0; q < 2; q++)
                    data_ready = data_ready && access_memory(address[l] + q * 8, data[l * 2 + q]);
            break;
    }

    return data_ready;
}

/* startTexel: in sRGB selects left or right half*/
void TBOX::TBOXEmu::decompress_texture_cache_line_data(ImageInfo currentImage, uint32_t startTexel,
                                                       uint64_t inData[TEXTURE_CACHE_QWORDS_PER_LINE],
                                                       uint64_t outData[TEXTURE_CACHE_QWORDS_PER_LINE])
{
    ImageFormat fmt = (ImageFormat)currentImage.info.format;
    bool tiled = currentImage.info.tiled;

    LOG(DEBUG, "Decompress Texture Cache Line format %x tiled %d", fmt, tiled);

    switch (fmt)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
            {
                bool hasAlpha = false;
                decode_BC1((uint8_t *) inData, (uint8_t *) outData, hasAlpha);
                break;
            }
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
            {
                bool hasAlpha = true;
                decode_BC1((uint8_t *) inData, (uint8_t *) outData, hasAlpha);
                break;
            }
        case FORMAT_BC2_UNORM_BLOCK:
            decode_BC2((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC3_UNORM_BLOCK:
            decode_BC3((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC4_UNORM_BLOCK:
            decode_BC4_UNORM((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC4_SNORM_BLOCK:
            decode_BC4_SNORM((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC5_UNORM_BLOCK:
            decode_BC5_UNORM((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC5_SNORM_BLOCK:
            decode_BC5_SNORM((uint8_t *) inData, (uint8_t *) outData);
            break;
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
            {
                uint8_t decompressedData[64];
                bool hasAlpha = fmt == FORMAT_BC1_RGBA_SRGB_BLOCK;
                decode_BC1((uint8_t *) inData, (uint8_t *) decompressedData, hasAlpha);
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t r_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 0];
                        uint8_t g_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 1];
                        uint8_t b_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 2];
                        uint8_t a_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 3];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        uint16_t a_fp16 = SRGB2LINEAR_TABLE[a_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                    }
            }
            break;
        case FORMAT_BC2_SRGB_BLOCK:
            {
                uint8_t decompressedData[64];
                decode_BC2((uint8_t *) inData, (uint8_t *) decompressedData);
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t r_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 0];
                        uint8_t g_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 1];
                        uint8_t b_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 2];
                        uint8_t a_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 3];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        uint16_t a_fp16 = SRGB2LINEAR_TABLE[a_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                    }
            }
            break;
        case FORMAT_BC3_SRGB_BLOCK:
            {
                uint8_t decompressedData[64];
                decode_BC3((uint8_t *) inData, (uint8_t *) decompressedData);
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t r_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 0];
                        uint8_t g_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 1];
                        uint8_t b_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 2];
                        uint8_t a_un8 = decompressedData[l * 16 + (startTexel + t) * 4 + 3];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        uint16_t a_fp16 = SRGB2LINEAR_TABLE[a_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                    }
            }
            break;
        case FORMAT_BC6H_UFLOAT_BLOCK:
            {
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        float decompressedTexel[4];
                        fetch_bptc_rgb_unsigned_float((uint8_t *) inData, 0, startTexel + t, l, decompressedTexel);
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[0])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[1])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[2])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[3])));
                    }
            }
            break;
        case FORMAT_BC6H_SFLOAT_BLOCK:
            {
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        float decompressedTexel[4];
                        fetch_bptc_rgb_signed_float((uint8_t *) inData, 0, startTexel + t, l, decompressedTexel);
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[0])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[1])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[2])));
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = fpu::UI16(fpu::f32_to_f16(fpu::F2F32(decompressedTexel[3])));
                    }
            }
            break;
        case FORMAT_BC7_UNORM_BLOCK:
            {
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint8_t decompressedTexel[4];
                        fetch_bptc_rgba_unorm_bytes((uint8_t *) inData, 0, t, l, decompressedTexel);
                        ((uint32_t *) outData)[l * 4 + t] = cast_bytes_to_uint32(decompressedTexel);
                    }
            }
            break;
        case FORMAT_BC7_SRGB_BLOCK:
            {
                for (uint32_t l = 0; l < 4; l++)
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t decompressedTexel[4];
                        fetch_bptc_rgba_unorm_bytes((uint8_t *) inData, 0, t, l, decompressedTexel);
                        uint8_t r_un8 = decompressedTexel[0];
                        uint8_t g_un8 = decompressedTexel[1];
                        uint8_t b_un8 = decompressedTexel[2];
                        uint8_t a_un8 = decompressedTexel[3];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        uint16_t a_fp16 = SRGB2LINEAR_TABLE[a_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                    }
            }
            break;
        case FORMAT_R4G4_UNORM_PACK8:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint8_t *) inData)[l * 4 + t] >> 4) & 0xf;
                    uint8_t g =  ((uint8_t *) inData)[l * 4 + t]       & 0xf;
                    r = (r << 4) | r;
                    g = (g << 4) | g;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | 0xFF000000;
                }
            }
            break;
        case FORMAT_R4G4B4A4_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >> 12) & 0xf;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  8) & 0xf;
                    uint8_t b = (((uint16_t *) inData)[l * 4 + t] >>  4) & 0xf;
                    uint8_t a =  ((uint16_t *) inData)[l * 4 + t]        & 0xf;
                    r = (r << 4) | r;
                    g = (g << 4) | g;
                    b = (b << 4) | b;
                    a = (a << 4) | a;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
                }
            }
            break;
        case FORMAT_B4G4R4A4_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >>  4) & 0xf;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  8) & 0xf;
                    uint8_t b = (((uint16_t *) inData)[l * 4 + t] >> 12) & 0xf;
                    uint8_t a =  ((uint16_t *) inData)[l * 4 + t]        & 0xf;
                    r = (r << 4) | r;
                    b = (b << 4) | b;
                    g = (g << 4) | g;
                    a = (a << 4) | a;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
                }
            }
            break;
        case FORMAT_R5G6B5_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >> 11) & 0x1f;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  5) & 0x3f;
                    uint8_t b =  ((uint16_t *) inData)[l * 4 + t]        & 0x1f;
                    r = (r << 3) | ((r >> 2) & 0x7);
                    g = (g << 2) | ((g >> 4) & 0x3);
                    b = (b << 3) | ((b >> 2) & 0x7);
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000;
                }
            }
            break;
        case FORMAT_B5G6R5_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r =  ((uint16_t *) inData)[l * 4 + t]        & 0x1f;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  5) & 0x3f;
                    uint8_t b = (((uint16_t *) inData)[l * 4 + t] >> 11) & 0x1f;
                    r = (r << 3) | ((r >> 2) & 0x7);
                    g = (g << 2) | ((g >> 4) & 0x3);
                    b = (b << 3) | ((b >> 2) & 0x7);
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000;
                }
            }
            break;
        case FORMAT_R5G5B5A1_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >> 11) & 0x1f;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  6) & 0x1f;
                    uint8_t b = (((uint16_t *) inData)[l * 4 + t] >>  1) & 0x1f;
                    uint8_t a =  ((uint16_t *) inData)[l * 4 + t]        & 0x01;
                    r = (r << 3) | ((r >> 2) & 0x7);
                    g = (g << 3) | ((g >> 2) & 0x7);
                    b = (b << 3) | ((b >> 2) & 0x7);
                    a = -a;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
                }
            }
            break;
        case FORMAT_B5G5R5A1_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >>  1) & 0x1f;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  6) & 0x1f;
                    uint8_t b = (((uint16_t *) inData)[l * 4 + t] >> 11) & 0x1f;
                    uint8_t a =  ((uint16_t *) inData)[l * 4 + t]        & 0x01;
                    r = (r << 3) | ((r >> 2) & 0x7);
                    g = (g << 3) | ((g >> 2) & 0x7);
                    b = (b << 3) | ((b >> 2) & 0x7);
                    a = -a;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
                }
            }
            break;
        case FORMAT_A1R5G5B5_UNORM_PACK16:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = (((uint16_t *) inData)[l * 4 + t] >> 10) & 0x1f;
                    uint8_t g = (((uint16_t *) inData)[l * 4 + t] >>  5) & 0x1f;
                    uint8_t b =  ((uint16_t *) inData)[l * 4 + t]        & 0x1f;
                    uint8_t a = (((uint16_t *) inData)[l * 4 + t] >> 15) & 0x01;
                    r = (r << 3) | ((r >> 2) & 0x7);
                    g = (g << 3) | ((g >> 2) & 0x7);
                    b = (b << 3) | ((b >> 2) & 0x7);
                    a = -a;
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
                }
            }
            break;
        case FORMAT_R8_UNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = ((uint8_t *) inData)[l * 4 + t];
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | 0xFF000000;
                }
            }
            break;
        case FORMAT_R8_SNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = ((uint8_t *) inData)[l * 4 + t];
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | 0x7F000000;
                }
            }
            break;
        case FORMAT_R8_SRGB:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r_un8 = ((uint8_t *) inData)[l * 4 + t];
                    float32_t r_fp32 = fpu::un8_to_f32(r_un8);
                    r_fp32 = fpu::F2F32(powf(fpu::FLT(r_fp32), 2.2));
                    float16_t r_fp16 = fpu::f32_to_f16(r_fp32);
                    ((uint16_t *) outData)[l * 8 + t * 2 + 0] = fpu::UI16(r_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 2 + 1] = 0;
                }
            }
            break;
        case FORMAT_R8G8_UNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = ((uint8_t *) inData)[l * 8 + t * 2];
                    uint8_t g = ((uint8_t *) inData)[l * 8 + t * 2 + 1];
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | 0xFF000000;
                }
            }
            break;
        case FORMAT_R8G8_SNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r = ((uint8_t *) inData)[l * 8 + t * 2];
                    uint8_t g = ((uint8_t *) inData)[l * 8 + t * 2 + 1];
                    ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | 0x7F000000;
                }
            }
            break;
        case FORMAT_R8G8_SRGB:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint8_t r_un8 = ((uint8_t *) inData)[l * 8 + t * 2];
                    uint8_t g_un8 = ((uint8_t *) inData)[l * 8 + t * 2 + 1];
                    uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                    uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                    ((uint16_t *) outData)[l * 8 + t * 2 + 0] = r_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 2 + 1] = g_fp16;
                }
            }
            break;
        case FORMAT_R8G8B8_UNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint8_t r = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        uint8_t g = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000;
                    }
                }
            }
            break;
        case FORMAT_R8G8B8_SNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint8_t r = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        uint8_t g = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        ((uint32_t *) outData)[l * 4 + t] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0x7F000000;
                    }
                }
            }
            break;
        case FORMAT_R8G8B8_SRGB:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t r_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        uint8_t g_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = 0x3c00;
                    }
                }
            }
            break;
        case FORMAT_B8G8R8_UNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint8_t r = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        uint8_t g = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        ((uint32_t *) outData)[l * 4 + t] = (uint32_t) r | ((uint32_t) g << 8) | ((uint32_t) b << 16) | 0xFF000000;
                    }
                }
            }
            break;
        case FORMAT_B8G8R8_SNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint8_t r = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        uint8_t g = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        ((uint32_t *) outData)[l * 4 + t] = (uint32_t) r | ((uint32_t) g << 8) | ((uint32_t) b << 16) | 0x7F000000;
                    }
                }
            }
            break;
        case FORMAT_B8G8R8_SRGB:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint8_t r_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 2];
                        uint8_t g_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 1];
                        uint8_t b_un8 = ((uint8_t *) inData)[l * 4 * texel_size + t * texel_size + 0];
                        uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                        uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                        uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = 0x3c00;
                    }
                }
            }
            break;
        case FORMAT_R8G8B8A8_SRGB:
        case FORMAT_A8B8G8R8_SRGB_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint8_t r_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 0];
                    uint8_t g_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 1];
                    uint8_t b_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 2];
                    uint8_t a_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 3];
                    uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                    uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                    uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                    uint16_t a_fp16 = fpu::UI16(fpu::f32_to_f16(fpu::un8_to_f32(a_un8)));
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                }
            }
            break;
        case FORMAT_B8G8R8A8_SRGB:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint8_t r_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 2];
                    uint8_t g_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 1];
                    uint8_t b_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 0];
                    uint8_t a_un8 = ((uint8_t *) inData)[l * 16 + t * 4 + 3];
                    uint16_t r_fp16 = SRGB2LINEAR_TABLE[r_un8].value;
                    uint16_t g_fp16 = SRGB2LINEAR_TABLE[g_un8].value;
                    uint16_t b_fp16 = SRGB2LINEAR_TABLE[b_un8].value;
                    uint16_t a_fp16 = fpu::UI16(fpu::f32_to_f16(fpu::un8_to_f32(a_un8)));
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = a_fp16;
                }
            }
            break;
        case FORMAT_A2R10G10B10_UNORM_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t r_un10 = (((uint32_t *) inData)[l * 2 + t] >> 20) & 0x03FF;
                    uint32_t g_un10 = (((uint32_t *) inData)[l * 2 + t] >> 10) & 0x03FF;
                    uint32_t b_un10 =  ((uint32_t *) inData)[l * 2 + t]        & 0x03FF;
                    uint32_t a_un2 =  (((uint32_t *) inData)[l * 2 + t] >> 30) & 0x03;
                    float32_t r_fp32 = fpu::un10_to_f32(r_un10);
                    float32_t g_fp32 = fpu::un10_to_f32(g_un10);
                    float32_t b_fp32 = fpu::un10_to_f32(b_un10);
                    float32_t a_fp32 = fpu::un2_to_f32(a_un2);
                    float16_t r_fp16 = fpu::f32_to_f16(r_fp32);
                    float16_t g_fp16 = fpu::f32_to_f16(g_fp32);
                    float16_t b_fp16 = fpu::f32_to_f16(b_fp32);
                    float16_t a_fp16 = fpu::f32_to_f16(a_fp32);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = fpu::UI16(r_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = fpu::UI16(g_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = fpu::UI16(b_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = fpu::UI16(a_fp16);
                }
            }
            break;
        case FORMAT_A2R10G10B10_SNORM_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t r_un10 = (((uint32_t *) inData)[l * 2 + t] >> 20) & 0x03FF;
                    uint32_t g_un10 = (((uint32_t *) inData)[l * 2 + t] >> 10) & 0x03FF;
                    uint32_t b_un10 =  ((uint32_t *) inData)[l * 2 + t]        & 0x03FF;
                    uint32_t a_un2 =  (((uint32_t *) inData)[l * 2 + t] >> 30) & 0x03;
                    float32_t r_fp32 = fpu::sn10_to_f32(r_un10);
                    float32_t g_fp32 = fpu::sn10_to_f32(g_un10);
                    float32_t b_fp32 = fpu::sn10_to_f32(b_un10);
                    float32_t a_fp32 = fpu::sn2_to_f32(a_un2);
                    float16_t r_fp16 = fpu::f32_to_f16(r_fp32);
                    float16_t g_fp16 = fpu::f32_to_f16(g_fp32);
                    float16_t b_fp16 = fpu::f32_to_f16(b_fp32);
                    float16_t a_fp16 = fpu::f32_to_f16(a_fp32);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = fpu::UI16(r_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = fpu::UI16(g_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = fpu::UI16(b_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = fpu::UI16(a_fp16);
                }
            }
            break;
        case FORMAT_A2B10G10R10_UNORM_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t r_un10 =  ((uint32_t *) inData)[l * 2 + t]        & 0x03FF;
                    uint32_t g_un10 = (((uint32_t *) inData)[l * 2 + t] >> 10) & 0x03FF;
                    uint32_t b_un10 = (((uint32_t *) inData)[l * 2 + t] >> 20) & 0x03FF;
                    uint32_t a_un2 =  (((uint32_t *) inData)[l * 2 + t] >> 30) & 0x03;
                    float32_t r_fp32 = fpu::un10_to_f32(r_un10);
                    float32_t g_fp32 = fpu::un10_to_f32(g_un10);
                    float32_t b_fp32 = fpu::un10_to_f32(b_un10);
                    float32_t a_fp32 = fpu::un2_to_f32(a_un2);
                    float16_t r_fp16 = fpu::f32_to_f16(r_fp32);
                    float16_t g_fp16 = fpu::f32_to_f16(g_fp32);
                    float16_t b_fp16 = fpu::f32_to_f16(b_fp32);
                    float16_t a_fp16 = fpu::f32_to_f16(a_fp32);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = fpu::UI16(r_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = fpu::UI16(g_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = fpu::UI16(b_fp16);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = fpu::UI16(a_fp16);
                }
            }
            break;
        case FORMAT_A2B10G10R10_SNORM_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t r_un10 =  ((uint32_t *) inData)[l * 2 + t]        & 0x03FF;
                    uint32_t g_un10 = (((uint32_t *) inData)[l * 2 + t] >> 10) & 0x03FF;
                    uint32_t b_un10 = (((uint32_t *) inData)[l * 2 + t] >> 20) & 0x03FF;
                    uint32_t a_un2 =  (((uint32_t *) inData)[l * 2 + t] >> 30) & 0x03;
                    float32_t r_fp32 = fpu::sn10_to_f32(r_un10);
                    float32_t g_fp32 = fpu::sn10_to_f32(g_un10);
                    float32_t b_fp32 = fpu::sn10_to_f32(b_un10);
                    float32_t a_fp32 = fpu::sn2_to_f32(a_un2);
                    float16_t r_fp16 = fpu::f32_to_f16(r_fp32);
                    float16_t g_fp16 = fpu::f32_to_f16(g_fp32);
                    float16_t b_fp16 = fpu::f32_to_f16(b_fp32);
                    float16_t a_fp16 = fpu::f32_to_f16(a_fp32);
                    ((uint16_t *) inData)[l * 8 + t * 4 + 0] = fpu::UI16(r_fp16);
                    ((uint16_t *) inData)[l * 8 + t * 4 + 1] = fpu::UI16(g_fp16);
                    ((uint16_t *) inData)[l * 8 + t * 4 + 2] = fpu::UI16(b_fp16);
                    ((uint16_t *) inData)[l * 8 + t * 4 + 3] = fpu::UI16(a_fp16);
                }
            }
            break;
        case FORMAT_B10G11R11_UFLOAT_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t texel = ((uint32_t *) inData)[l * 2 + t];
                    uint16_t r_fp11 =  texel        & 0x07FF;
                    uint16_t g_fp11 = (texel >> 11) & 0x07FF;
                    uint16_t b_fp10 = (texel >> 22) & 0x03FF;
                    uint16_t r_fp16 = r_fp11 << 5;
                    uint16_t g_fp16 = g_fp11 << 5;
                    uint16_t b_fp16 = b_fp10 << 6;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = 0x3c00;
                }
            }
            break;
        case FORMAT_E5B9G9R9_UFLOAT_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 2; t++)
                {
                    uint32_t texel = ((uint32_t *) inData)[l * 2 + t];
                    uint16_t r_mant9    =  texel        & 0x01FF;
                    uint16_t g_mant9    = (texel >>  9) & 0x01FF;
                    uint16_t b_mant9    = (texel >> 18) & 0x01FF;
                    uint16_t common_exp = (texel >> 27) & 0x01F;
                    uint16_t r_fp16 = sharedexp_to_float16(common_exp, r_mant9);
                    uint16_t g_fp16 = sharedexp_to_float16(common_exp, g_mant9);
                    uint16_t b_fp16 = sharedexp_to_float16(common_exp, b_mant9);
                    ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b_fp16;
                    ((uint16_t *) outData)[l * 8 + t * 4 + 3] = 0x3c00;
                }
            }
            break;

            break;
        case FORMAT_X8_D24_UNORM_PACK32:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint32_t r_un24 = ((uint32_t *) inData)[l * 4 + t] & 0x00FFFFFF;
                    float32_t r_fp32 = fpu::un24_to_f32(r_un24);
                    ((uint32_t *) outData)[l * 4 + t] = fpu::UI32(r_fp32);
                }
            }
            break;
        case FORMAT_R16_UNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint16_t r_un16 = ((uint16_t *) inData)[l * 4 + t];
                    float32_t r_fp32 = fpu::un16_to_f32(r_un16);
                    ((uint32_t *) outData)[l * 4 + t] = fpu::UI32(r_fp32);
                }
            }
            break;
        case FORMAT_R16_SNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint16_t r_sn16 = ((uint16_t *) inData)[l * 4 + t];
                    float32_t r_fp32 = fpu::sn16_to_f32(r_sn16);
                    ((uint32_t *) outData)[l * 4 + t] = fpu::UI32(r_fp32);
                }
            }
            break;
        case FORMAT_R16_SFLOAT:
            for (uint32_t l = 0; l < 4; l++)
            {
                for (uint32_t t = 0; t < 4; t++)
                {
                    uint16_t r_un16 = ((uint16_t *) inData)[l * 4 + t];
                    ((uint16_t *) outData)[l * 8 + t * 2 + 0] = r_un16;
                    ((uint16_t *) outData)[l * 8 + t * 2 + 1] = 0;
                }
            }
            break;
        case FORMAT_R16G16_UNORM:
            {
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint16_t r_un16 = ((uint16_t *) inData)[l * 4 + t * 2 + 0];
                        uint16_t g_un16 = ((uint16_t *) inData)[l * 4 + t * 2 + 1];
                        float32_t r_fp32 = fpu::un16_to_f32(r_un16);
                        float32_t g_fp32 = fpu::un16_to_f32(g_un16);
                        ((uint32_t *) outData)[l * 4 + t * 2 + 0] = fpu::UI32(r_fp32);
                        ((uint32_t *) outData)[l * 4 + t * 2 + 1] = fpu::UI32(g_fp32);
                    }
                }
            }
            break;
        case FORMAT_R16G16_SNORM:
            {
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint16_t r_sn16 = ((uint16_t *) inData)[l * 4 + t * 2 + 0];
                        uint16_t g_sn16 = ((uint16_t *) inData)[l * 4 + t * 2 + 1];
                        float32_t r_fp32 = fpu::sn16_to_f32(r_sn16);
                        float32_t g_fp32 = fpu::sn16_to_f32(g_sn16);
                        ((uint32_t *) outData)[l * 4 + t * 2 + 0] = fpu::UI32(r_fp32);
                        ((uint32_t *) outData)[l * 4 + t * 2 + 1] = fpu::UI32(g_fp32);
                    }
                }
            }
            break;
        case FORMAT_R16G16_SFLOAT:
            {
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 4; t++)
                    {
                        uint16_t r = ((uint16_t *) inData)[l * 8 + t * 2 + 0];
                        uint16_t g = ((uint16_t *) inData)[l * 8 + t * 2 + 1];
                        ((uint16_t *) outData)[l * 8 + t * 2 + 0] = r;
                        ((uint16_t *) outData)[l * 8 + t * 2 + 1] = g;
                    }
                }
            }
            break;
        case FORMAT_R16G16B16_UNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    uint16_t r_un16 = ((uint16_t *) inData)[l * texel_size + 0];
                    uint16_t g_un16 = ((uint16_t *) inData)[l * texel_size + 1];
                    uint16_t b_un16 = ((uint16_t *) inData)[l * texel_size + 2];
                    float32_t r_fp32 = fpu::un16_to_f32(r_un16);
                    float32_t g_fp32 = fpu::un16_to_f32(g_un16);
                    float32_t b_fp32 = fpu::un16_to_f32(b_un16);
                    ((uint32_t *) outData)[l * 4 + 0] = fpu::UI32(r_fp32);
                    ((uint32_t *) outData)[l * 4 + 1] = fpu::UI32(g_fp32);
                    ((uint32_t *) outData)[l * 4 + 2] = fpu::UI32(b_fp32);
                    ((uint32_t *) outData)[l * 4 + 3] = fpu::UI32(fpu::F2F32(1.0));
                }
            }
            break;
        case FORMAT_R16G16B16_SNORM:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    uint16_t r_sn16 = ((uint16_t *) inData)[l * texel_size + 0];
                    uint16_t g_sn16 = ((uint16_t *) inData)[l * texel_size + 1];
                    uint16_t b_sn16 = ((uint16_t *) inData)[l * texel_size + 2];
                    float32_t r_fp32 = fpu::sn16_to_f32(r_sn16);
                    float32_t g_fp32 = fpu::sn16_to_f32(g_sn16);
                    float32_t b_fp32 = fpu::sn16_to_f32(b_sn16);
                    ((uint32_t *) outData)[l * 4 + 0] = fpu::UI32(r_fp32);
                    ((uint32_t *) outData)[l * 4 + 1] = fpu::UI32(g_fp32);
                    ((uint32_t *) outData)[l * 4 + 2] = fpu::UI32(b_fp32);
                    ((uint32_t *) outData)[l * 4 + 3] = fpu::UI32(fpu::F2F32(1.0));
                }
            }
            break;
        case FORMAT_R16G16B16_SFLOAT:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    for (uint32_t t = 0; t < 2; t++)
                    {
                        uint16_t r = ((uint16_t *) inData)[l * 2 * texel_size + t * texel_size + 0];
                        uint16_t g = ((uint16_t *) inData)[l * 2 * texel_size + t * texel_size + 1];
                        uint16_t b = ((uint16_t *) inData)[l * 2 * texel_size + t * texel_size + 2];
                        ((uint16_t *) outData)[l * 8 + t * 4 + 0] = r;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 1] = g;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 2] = b;
                        ((uint16_t *) outData)[l * 8 + t * 4 + 3] = 0x3c00;
                    }
                }
            }
            break;
        case FORMAT_R32G32B32_SFLOAT:
            {
                uint32_t texel_size = tiled ? 4 : 3;
                for (uint32_t l = 0; l < 4; l++)
                {
                    uint32_t r_fp32 = ((uint32_t *) inData)[l * texel_size + 0];
                    uint32_t g_fp32 = ((uint32_t *) inData)[l * texel_size + 1];
                    uint32_t b_fp32 = ((uint32_t *) inData)[l * texel_size + 2];
                    ((uint32_t *) outData)[l * 4 + 0] = r_fp32;
                    ((uint32_t *) outData)[l * 4 + 1] = g_fp32;
                    ((uint32_t *) outData)[l * 4 + 2] = b_fp32;
                    ((uint32_t *) outData)[l * 4 + 3] = 0x3f800000;
                }
            }
            break;
        case FORMAT_R16G16B16A16_UNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                uint16_t r_un16 = ((uint16_t *) inData)[l * 4 + 0];
                uint16_t g_un16 = ((uint16_t *) inData)[l * 4 + 1];
                uint16_t b_un16 = ((uint16_t *) inData)[l * 4 + 2];
                uint16_t a_un16 = ((uint16_t *) inData)[l * 4 + 3];
                float32_t r_fp32 = fpu::un16_to_f32(r_un16);
                float32_t g_fp32 = fpu::un16_to_f32(g_un16);
                float32_t b_fp32 = fpu::un16_to_f32(b_un16);
                float32_t a_fp32 = fpu::un16_to_f32(a_un16);
                ((uint32_t *) outData)[l * 4 + 0] = fpu::UI32(r_fp32);
                ((uint32_t *) outData)[l * 4 + 1] = fpu::UI32(g_fp32);
                ((uint32_t *) outData)[l * 4 + 2] = fpu::UI32(b_fp32);
                ((uint32_t *) outData)[l * 4 + 3] = fpu::UI32(a_fp32);
            }
            break;
        case FORMAT_R16G16B16A16_SNORM:
            for (uint32_t l = 0; l < 4; l++)
            {
                uint16_t r_sn16 = ((uint16_t *) inData)[l * 4 + 0];
                uint16_t g_sn16 = ((uint16_t *) inData)[l * 4 + 1];
                uint16_t b_sn16 = ((uint16_t *) inData)[l * 4 + 2];
                uint16_t a_sn16 = ((uint16_t *) inData)[l * 4 + 3];
                float32_t r_fp32 = fpu::sn16_to_f32(r_sn16);
                float32_t g_fp32 = fpu::sn16_to_f32(g_sn16);
                float32_t b_fp32 = fpu::sn16_to_f32(b_sn16);
                float32_t a_fp32 = fpu::sn16_to_f32(a_sn16);
                ((uint32_t *) outData)[l * 4 + 0] = fpu::UI32(r_fp32);
                ((uint32_t *) outData)[l * 4 + 1] = fpu::UI32(g_fp32);
                ((uint32_t *) outData)[l * 4 + 2] = fpu::UI32(b_fp32);
                ((uint32_t *) outData)[l * 4 + 3] = fpu::UI32(a_fp32);
            }
            break;
        case FORMAT_R32G32B32A32_SFLOAT:
            for (uint32_t l = 0; l < 4; l++)
            {
                uint32_t r_fp32 = ((uint32_t *) inData)[l * 4 + 0];
                uint32_t g_fp32 = ((uint32_t *) inData)[l * 4 + 1];
                uint32_t b_fp32 = ((uint32_t *) inData)[l * 4 + 2];
                uint32_t a_fp32 = ((uint32_t *) inData)[l * 4 + 3];
                ((uint32_t *) outData)[l * 4 + 0] = r_fp32;
                ((uint32_t *) outData)[l * 4 + 1] = g_fp32;
                ((uint32_t *) outData)[l * 4 + 2] = b_fp32;
                ((uint32_t *) outData)[l * 4 + 3] = a_fp32;
            }
            break;
        default:
            for (uint32_t q = 0; q < 8; q++)
                outData[q] = inData[q];
            break;
    }
}

void TBOX::TBOXEmu::sample_quad(uint32_t thread, bool output_result)
{
    LOG(DEBUG, "%s", "\tTBOX => Sample Quad");

    ImageInfo currentImage;

    //  Also sets request_hart so there is no need to do it here.
    get_image_info(thread, currentImage);

    sample_quad(currentRequest[thread], currentImage, input[thread], output[thread], output_result);
}

void TBOX::TBOXEmu::sample_quad(SampleRequest currentRequest, freg_t input[], freg_t output[])
{
    LOG(DEBUG, "%s", "\tTBOX => Sample Quad");

    return;

    ImageInfo currentImage;

    get_image_info(currentRequest, currentImage);

    sample_quad(currentRequest, currentImage, input, output, true);
}

bool TBOX::TBOXEmu::get_image_info(uint32_t thread, ImageInfo &currentImage)
{
    request_hart = thread;
    return TBOXEmu::get_image_info(currentRequest[thread], currentImage);
}

bool TBOX::TBOXEmu::get_image_info(SampleRequest request, ImageInfo &currentImage)
{

#ifdef TBOX_IMAGE_INFO_CACHE
    bool hit = image_info_cache_lookup(request.info.imageid, currentImage);
    if (!hit)
    {
        uint64_t imageInfoAddress = imageTableAddress + request.info.imageid * 32;

        bool data_ready = read_image_info_cache_line(imageInfoAddress, currentImage);
        if (data_ready)
        {
            image_info_cache_fill(request.info.imageid, currentImage);

            LOG(DEBUG, "\tRead Image Descriptor with ID %ld from Address %016lx", request.info.imageid, imageInfoAddress);
        }
        else
        {
            LOG(DEBUG, "\tRead Image Descriptor with ID %ld. Data not available for address %lx",
                               request.info.imageid, imageInfoAddress);
            return false;
        }
    }
#endif

    uint64_t imageInfoAddress = imageTableAddress + request.info.imageid * 32;


    LOG(DEBUG, "\tRead Image Descriptor with ID %d from Address %016lx", request.info.imageid, imageInfoAddress);
    fflush(stdout);

    currentImage.data[0] = bemu::pmemread<uint64_t>(imageInfoAddress);
    currentImage.data[1] = bemu::pmemread<uint64_t>(imageInfoAddress + 8);
    currentImage.data[2] = bemu::pmemread<uint64_t>(imageInfoAddress + 16);
    currentImage.data[3] = bemu::pmemread<uint64_t>(imageInfoAddress + 24);

    LOG(DEBUG, "\tImage Info %016lx %016lx %016lx %016lx", currentImage.data[0],
               currentImage.data[1], currentImage.data[2], currentImage.data[3]);
    print_image_info(currentImage);

    return true;
}

void TBOX::TBOXEmu::sample_quad(SampleRequest currentRequest, ImageInfo currentImage, freg_t input[], freg_t output[], bool output_result)
{
    // Checks.
    if (((currentRequest.info.operation == SAMPLE_OP_SAMPLE)
         || (currentRequest.info.operation == SAMPLE_OP_SAMPLE_L))
        && !((currentRequest.info.magfilter == FILTER_TYPE_NEAREST)
             && (currentRequest.info.minfilter == FILTER_TYPE_NEAREST))
        && !filterSupported((ImageFormat)currentImage.info.format))
    {
        throw std::runtime_error("TBOX: Format does not support filtering.");
    }

    if (((currentRequest.info.operation == SAMPLE_OP_SAMPLE_C) ||
         (currentRequest.info.operation == SAMPLE_OP_SAMPLE_C_L) ||
         (currentRequest.info.operation == SAMPLE_OP_GATHER4_C)) &&
        !comparisonSupported((ImageFormat)currentImage.info.format))
    {
        throw std::runtime_error("TBOX: Format does not support comparison filter.");
    }

    // set unused bytes to 0, so that it can be compared with RTL
    memset(output, 0, sizeof(freg_t)*4);

    for (uint32_t quad = 0; quad < 2; quad++)
    {
        // Get LOD.
        uint32_t mip_level[4];
        uint32_t mip_beta[4];
        FilterType pixel_filter[4];

        switch (currentRequest.info.operation)
        {
            case SAMPLE_OP_SAMPLE:
            case SAMPLE_OP_SAMPLE_C:
                {
                    float lod = fpu::FLT(fpu::f16_to_f32(fpu::F16(currentRequest.info.lodaniso.lodanisoq.lod[quad])));
                    uint32_t lod_fxp = min(max(uint32_t(currentImage.info.basemip << 8),
                                               uint32_t(floor(max(lod, 0.0) * 256.0))),
                                           uint32_t((currentImage.info.mipcount - 1) << 8));
                    uint32_t quad_mip_level = lod_fxp >> 8;
                    uint32_t quad_mip_beta = (currentRequest.info.mipfilter == FILTER_TYPE_LINEAR) ? (lod_fxp & 0xFF) : 0;
                    FilterType quad_filter = (FilterType)((lod <= 0.0) ? currentRequest.info.magfilter : currentRequest.info.minfilter);
                    for (uint32_t req = 0; req < 4; req++)
                    {
                        mip_level[req] = quad_mip_level;
                        mip_beta[req] = quad_mip_beta;
                        pixel_filter[req] = quad_filter;
                    }
                }
                break;
            case SAMPLE_OP_SAMPLE_L:
            case SAMPLE_OP_SAMPLE_C_L:
                for (uint32_t req = 0; req < 4; req++)
                {
                    float pixel_lod = fpu::FLT(fpu::f16_to_f32(fpu::F16(currentRequest.info.lodaniso.lod_array[quad][req])));
                    uint32_t pixel_lod_fxp = min(max(uint32_t(currentImage.info.basemip << 8),
                                                     uint32_t(floor(pixel_lod * 256.0))),
                                                 uint32_t((currentImage.info.mipcount - 1) << 8));
                    mip_level[req] = pixel_lod_fxp >> 8;
                    mip_beta[req] = (currentRequest.info.mipfilter == FILTER_TYPE_LINEAR) ? (pixel_lod_fxp & 0xFF) : 0;
                    pixel_filter[req] = (FilterType)((pixel_lod <= 0) ? currentRequest.info.magfilter
                                                                      : currentRequest.info.minfilter);
                }
                break;
            case SAMPLE_OP_GATHER4:
            case SAMPLE_OP_GATHER4_C:
                for (uint32_t req = 0; req < 4; req++)
                {
                    mip_level[req] = 0;
                    mip_beta[req] = 0;
                    pixel_filter[req] = FILTER_TYPE_LINEAR;
                }
                break;
            case SAMPLE_OP_LD:
                for (uint32_t req = 0; req < 4; req++)
                {
                    mip_level[req] = uint32_t(fpu::FLT(fpu::f16_to_f32(fpu::F16(currentRequest.info.lodaniso.lod_array[quad][req]))));
                    mip_beta[req] = 0;
                    pixel_filter[req] = FILTER_TYPE_NEAREST;
                }
                break;
            default:
                LOG(ERR, "%s", "Unsupported sample operation");
                break;
        }

        for (uint32_t req = 0; req < 4; req++)
            sample_pixel(currentRequest, input, output, quad, req, currentImage,
                         pixel_filter[req], mip_level[req], mip_beta[req], output_result);
    }
}

void TBOX::TBOXEmu::sample_pixel(SampleRequest currentRequest, freg_t input[], freg_t output[],
                                 uint32_t quad, uint32_t pixel,
                                 ImageInfo currentImage, FilterType filter, uint32_t mip_level,
                                 uint32_t mip_beta, bool output_result)
{
    uint32_t num_mips = (mip_beta == 0) || (mip_level == uint32_t(currentImage.info.mipcount - 1)) ? 1 : 2;
    float mip_beta_fp = 1.0 - (float(mip_beta) / 256.0);

    LOG(DEBUG, "\tsample pixel %d with filter %s mip level %d mip beta %02x", pixel,
                      toStrFilterType(filter), mip_level, mip_beta);

    float red     = 0.0;
    float green   = 0.0;
    float blue    = 0.0;
    float alpha   = 0.0;

    float aniso_ratio = fpu::FLT(fpu::f16_to_f32(fpu::F16(currentRequest.info.lodaniso.lodanisoq.anisoratio[quad])));

    float aniso_weight = 1.0f;
    uint32_t aniso_count = 1;
    float aniso_deltas = fpu::FLT(fpu::sn8_to_f32(currentRequest.info.lodaniso.lodanisoq.anisodeltas[quad]));
    float aniso_deltat = fpu::FLT(fpu::sn8_to_f32(currentRequest.info.lodaniso.lodanisoq.anisodeltat[quad]));

    if (((currentRequest.info.operation == SAMPLE_OP_SAMPLE)
         || (currentRequest.info.operation == SAMPLE_OP_SAMPLE_C))
        && (currentRequest.info.aniso == 1) && (aniso_ratio > 1.0f))
    {
        aniso_count = round(aniso_ratio);
        aniso_count = (aniso_count >> 1) + (aniso_count & 0x01);
        aniso_count = aniso_count * 2;
        if (aniso_count == 0)
            LOG(WARN, "Aniso count is 0!! aniso_ratio = %f round(aniso_ratio) = %f", aniso_ratio, round(aniso_ratio));
        
        aniso_weight = 1.0f / float(aniso_count);
        
        LOG(DEBUG, "\taniso_count = %d aniso_weight = %f", aniso_count, aniso_weight);

    }

    for (uint32_t aniso_sample_idx = 0; aniso_sample_idx < aniso_count; aniso_sample_idx++)
    {
        if (aniso_count > 1) LOG(DEBUG, "\taniso sample %d out of %d", aniso_sample_idx, aniso_count);

        uint32_t sample_mip_level = mip_level;
        float sample_mip_beta = mip_beta_fp;

        for (uint32_t mip = 0; mip < num_mips; mip++)
        {
            if (num_mips > 1) LOG(DEBUG, "\tmip sample %d", mip);
            uint32_t num_slices = (currentImage.info.type == IMAGE_TYPE_3D) ? 2 : 1;

            freg_t s = input[0];
            freg_t t = input[1];
            freg_t r = input[2];

            for (uint32_t slice = 0; slice < num_slices; slice++)
            {
                if (num_slices > 1) LOG(DEBUG, "\tslice sample %d", slice);
                sample_bilinear(currentRequest, s, t, r, quad * 4 + pixel, currentImage, filter, slice, sample_mip_level,
                                sample_mip_beta, aniso_sample_idx, aniso_weight, aniso_deltas, aniso_deltat, red,
                                green, blue, alpha, output_result);
            }
            sample_mip_beta = 1.0 - sample_mip_beta;
            sample_mip_level = sample_mip_level + 1;
        }
    }

    //  Apply image swizzle.
    float red_swz;
    float green_swz;
    float blue_swz;
    float alpha_swz;

    red_swz     = apply_component_swizzle((ComponentSwizzle)currentImage.info.swizzler, red, red, green, blue, alpha);
    green_swz   = apply_component_swizzle((ComponentSwizzle)currentImage.info.swizzleg, green, red, green, blue, alpha);
    blue_swz    = apply_component_swizzle((ComponentSwizzle)currentImage.info.swizzleb, blue, red, green, blue, alpha);
    alpha_swz   = apply_component_swizzle((ComponentSwizzle)currentImage.info.swizzlea, alpha, red, green, blue, alpha);

    // Apply request swizzle.
    red     = apply_component_swizzle((ComponentSwizzle)currentRequest.info.swizzler, red_swz, red_swz, green_swz,
                                  blue_swz, alpha_swz);
    green   = apply_component_swizzle((ComponentSwizzle)currentRequest.info.swizzleg, green_swz, red_swz,
                                    green_swz, blue_swz, alpha_swz);
    blue    = apply_component_swizzle((ComponentSwizzle)currentRequest.info.swizzleb, blue_swz, red_swz,
                                   green_swz, blue_swz, alpha_swz);
    alpha   = apply_component_swizzle((ComponentSwizzle)currentRequest.info.swizzlea, alpha_swz, red_swz,
                                    green_swz, blue_swz, alpha_swz);

    output[0].u32[quad * 4 + pixel] = fpu::F2UI32(red);
    output[1].u32[quad * 4 + pixel] = fpu::F2UI32(green);
    output[2].u32[quad * 4 + pixel] = fpu::F2UI32(blue);
    output[3].u32[quad * 4 + pixel] = fpu::F2UI32(alpha);
}

/*

    This method contains two main phases:
        1. Compute i, j, k texel coordinates and their corresponding betas
        2. Create Texture Cache Tags and read texels:
            a) From Texture Cache or b) From Main Memory (thought Virtual Address module)

*/
void TBOX::TBOXEmu::sample_bilinear(SampleRequest currentRequest, freg_t s, freg_t t, freg_t r, uint32_t req,
                                    ImageInfo currentImage, FilterType filter, uint32_t slice,
                                    uint32_t sample_mip_level, float sample_mip_beta, uint32_t aniso_sample,
                                    float aniso_weight, float aniso_deltas, float aniso_deltat, float &red,
                                    float &green, float &blue, float &alpha, bool output_result)
{
    uint32_t mip_width  = max(1, (currentImage.info.width  + 1) >> sample_mip_level);
    uint32_t mip_height = max(1, (currentImage.info.height + 1) >> sample_mip_level);
    uint32_t mip_depth  = max(1, (currentImage.info.depth  + 1) >> sample_mip_level);

    uint32_t i[2];
    uint32_t j[2];
    uint32_t k[2];
    uint32_t l;
    bool out_of_bounds = false;

    float betai = 0.0, betaj = 0.0, betak = 0.0;

    // 1: Compute i, j, k texel coordinates and their corresponding betas
    if (currentRequest.info.operation == SAMPLE_OP_LD)
    {
        i[0] = s.u32[req];
        out_of_bounds = (i[0] >= mip_width);

        if ((currentImage.info.type == IMAGE_TYPE_2D) || (currentImage.info.type == IMAGE_TYPE_3D) ||
             (currentImage.info.type == IMAGE_TYPE_2D_ARRAY))
        {
            j[0] = t.u32[req];
            out_of_bounds = out_of_bounds || (j[0] >= mip_height);
        }
        else
            j[0] = 0;

        if (currentImage.info.type == IMAGE_TYPE_3D)
        {
            k[0] = r.u32[req];
            out_of_bounds = out_of_bounds || (k[0] >= mip_depth);
        }
        else
            k[0] = 0;

        if (currentImage.info.type == IMAGE_TYPE_1D_ARRAY)
        {
            l = t.u32[req];
            out_of_bounds = out_of_bounds || (l < currentImage.info.arraybase) || (l >= currentImage.info.arraycount);
        }
        else if (currentImage.info.type == IMAGE_TYPE_2D_ARRAY)
        {
            l = r.u32[req];
            out_of_bounds = out_of_bounds || (l < currentImage.info.arraybase) || (l >= currentImage.info.arraycount);
        }
        else
            l = 0;

        LOG(DEBUG, "%s", "\tLD operation (texel coordinates)");
        switch (currentImage.info.type)
            {
            case IMAGE_TYPE_1D:
                LOG(DEBUG, "\tmip width %u i %u", mip_width, i[0]);
                break;
            case IMAGE_TYPE_2D:
                LOG(DEBUG, "\tmip width %u mip height %u i %u j %u", mip_width, mip_height, i[0], j[0]);
                break;
            case IMAGE_TYPE_3D:
                LOG(DEBUG, "\tmip width %u mip height %u mip depth %u i %u j %u k %u",
                    mip_width, mip_height, mip_depth, i[0], j[0], k[0]);
                break;
            case IMAGE_TYPE_1D_ARRAY:
                LOG(DEBUG, "\tmip width %u i %u l %u", mip_width, i[0], l);
                break;
            case IMAGE_TYPE_CUBE:
            case IMAGE_TYPE_2D_ARRAY:
            case IMAGE_TYPE_CUBE_ARRAY:
                LOG(DEBUG, "\tmip width %u mip height %u i %u j %u l %u", mip_width, mip_height, i[0], j[0], l);
                break;
            }
    }
    else
    {
        float u, v, w;
        uint32_t a;
        
        /*
            aniso_sample: 0 1 2  3 4  5 6  7 8  9 10 11 12 13 14 15
            aniso_step:   0 0 1 -1 2 -2 3 -3 4 -4  5 -5  6 -6  7 -7
        */
        int side_step_sign = (aniso_sample & 0x1)? -1: 1;

        u = fpu::FLT(s.u32[req]) * mip_width + (aniso_sample>>1)*side_step_sign*aniso_deltas;

        if ((currentImage.info.type == IMAGE_TYPE_2D) || (currentImage.info.type == IMAGE_TYPE_CUBE)
            || (currentImage.info.type == IMAGE_TYPE_3D) || (currentImage.info.type == IMAGE_TYPE_2D_ARRAY)
            || (currentImage.info.type == IMAGE_TYPE_CUBE_ARRAY))

            v = fpu::FLT(t.u32[req]) * mip_height + (aniso_sample>>1)*side_step_sign*aniso_deltat;
        else
            v = 0.0;

        if (currentImage.info.type == IMAGE_TYPE_3D)
            w = fpu::FLT(r.u32[req]) * mip_depth;
        else
            w = 0;

        if (currentImage.info.type == IMAGE_TYPE_1D_ARRAY)
            a = uint32_t(fpu::FLT(t.u32[req]));
        else if ((currentImage.info.type == IMAGE_TYPE_2D_ARRAY) ||
                 (currentImage.info.type == IMAGE_TYPE_CUBE) ||
                 (currentImage.info.type == IMAGE_TYPE_CUBE_ARRAY))
            a = uint32_t(fpu::FLT(r.u32[req]));
        else
            a = 0;

        
        LOG(DEBUG, "%s", "\tSAMPLE operation (unnormalized coordinates)");
        switch (currentImage.info.type)
            {
            case IMAGE_TYPE_1D:
                LOG(DEBUG, "\tmip width %u u %f", mip_width, u);
                break;
            case IMAGE_TYPE_2D:
                LOG(DEBUG, "\tmip width %u mip height %u u %f v %f", mip_width, mip_height, u, v);
                break;
            case IMAGE_TYPE_3D:
                LOG(DEBUG, "\tmip width %u mip height %u mip depth %u u %f v %f w %f", mip_width, mip_height, mip_depth, u, v, w);
                break;
            case IMAGE_TYPE_1D_ARRAY:
                LOG(DEBUG, "\tmip width %u u %f a = %u", mip_width, u, a);
                break;
            case IMAGE_TYPE_CUBE:
            case IMAGE_TYPE_2D_ARRAY:
            case IMAGE_TYPE_CUBE_ARRAY:
                LOG(DEBUG, "\tmip width %u mip height %u u %f v %f a %u", mip_width, mip_height, u, v, a);
                break;
            }
        

        if (filter == FILTER_TYPE_LINEAR)
        {
            u -= 0.5f;
            v -= 0.5f;
            w -= 0.5f;
        }

        int32_t u_fxp = int32_t(floor(u * 256));
        int32_t v_fxp = int32_t(floor(v * 256));
        int32_t w_fxp = int32_t(floor(w * 256));

        int32_t i_ul = u_fxp >> 8;
        int32_t j_ul = v_fxp >> 8;
        int32_t k_ul = w_fxp >> 8;


        betai = float(u_fxp & 0xFF) / 256.0;
        betaj = float(v_fxp & 0xFF) / 256.0;
        betak = float(w_fxp & 0xFF) / 256.0;

        wrap_texel_coord(i, i_ul, mip_width,  (AddressMode) currentRequest.info.addrmodeu);
        wrap_texel_coord(j, j_ul, mip_height, (AddressMode) currentRequest.info.addrmodev);
        wrap_texel_coord(k, k_ul, mip_depth,  (AddressMode) currentRequest.info.addrmodew);

        l = min(max(currentImage.info.arraybase, a), currentImage.info.arraycount);
    }


    // 2: Create Texture Cache Tags and read texels
    if (filter == FILTER_TYPE_NEAREST)
    {
        float texel_ul[4];

        if ((currentRequest.info.operation == SAMPLE_OP_LD) && out_of_bounds)
        {
            LOG(ERR, "%s", "\tOut of bound access");

            red   = 0.0f;
            green = 0.0f;
            blue  = 0.0f;
            alpha = 0.0f;
        }
        else
        {
#ifdef TBOX_TEXTURE_CACHE
            uint32_t num_banks;
            int32_t banks[4];
            uint64_t tags[4];
            uint64_t address[4][4];
            banks[0] = -1;
            tags[0] = 0xFFFFFFFFFFFFFFFULL;

            create_texture_cache_tags(currentRequest, currentImage, filter, i, j, k[slice], l, sample_mip_level,
                                      num_banks, banks, tags, address);

            uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE];
            bool hit = texture_cache_lookup(banks[0], tags[0], data);
            bool data_ready = hit;
            if (!hit)
            {
                data_ready = read_texture_cache_line(currentImage, address[0], data);
                if (data_ready)
                    texture_cache_fill(banks[0], tags[0], data);
                else
                {
                    for(uint32_t qw = 0; qw < TEXTURE_CACHE_QWORDS_PER_LINE; qw++)
                        data[qw] = 0xdeadbeaffacecafeUL;
                }
            }

            output_result = output_result && data_ready;

            LOG(DEBUG, "%s", "\tTexture cache access");
            LOG(DEBUG, "\t\tBank = %d Tag = %" PRIx64 " Hit = %d", banks[0], tags[0], hit);
            for ( int i = 0; i< 4; i++) LOG(DEBUG, "\t\tAddress[%d] = %" PRIx64, i, address[0][i]);
            for ( int i = 0; i< 8;i++) LOG(DEBUG, "\t\tData[%d] = %" PRIx64, i, data[i]);;
            
            read_texel(currentImage, i[0], j[0], data, texel_ul, output_result);
#else
            read_texel(currentImage, i[0], j[0], k[0], l, sample_mip_level, texel_ul);
#endif
            if (   (currentRequest.info.operation == SAMPLE_OP_SAMPLE_C)
                || (currentRequest.info.operation == SAMPLE_OP_SAMPLE_C_L)
                || (currentRequest.info.operation == SAMPLE_OP_GATHER4_C))
            {
                texel_ul[0] = compare_texel((CompareOperation)currentRequest.info.compop, fpu::FLT(t.u32[req]), texel_ul[0]);
                texel_ul[1] = 0.0;
                texel_ul[2] = 0.0;
                texel_ul[3] = 0.0;
            }

            red = texel_ul[0];
            green = texel_ul[1];
            blue = texel_ul[2];
            alpha = texel_ul[3];
        }
    }
    else
    {
        float texel_ul[4];
        float texel_ur[4];
        float texel_ll[4];
        float texel_lr[4];

#ifdef TBOX_TEXTURE_CACHE
        uint32_t num_banks;
        int32_t banks[4];
        banks[0] = banks[1] = banks[2] = banks[3] = -1;
        uint64_t tags[4];
        uint64_t address[4][4];
        tags[0] = tags[1] = tags[2] = tags[3] = 0xFFFFFFFFFFFFFFFULL;

        create_texture_cache_tags(currentRequest, currentImage, filter, i, j, k[slice], l, sample_mip_level,
                                  num_banks, banks, tags, address);

        bool hits[4];
        uint64_t data[4][TEXTURE_CACHE_QWORDS_PER_LINE];
        for (uint32_t b = 0; b < num_banks; b++)
        {
            hits[b] = texture_cache_lookup(banks[b], tags[b], data[b]);
            if (!hits[b])
            {
                bool data_ready = read_texture_cache_line(currentImage, address[b], data[b]);
                if (data_ready)
                    texture_cache_fill(banks[b], tags[b], data[b]);
                else
                {
                    for(uint32_t qw = 0; qw < TEXTURE_CACHE_QWORDS_PER_LINE; qw++)
                        data[b][qw] = 0xdeadbeaffacecafeUL;
                }

                output_result = output_result && data_ready;
            }
        }

        LOG(DEBUG, "\tTexture cache banks accessed = %u", num_banks);
        for (uint32_t b = 0; b < num_banks; b++) {
            LOG(DEBUG, "\t\tBank = %d Tag = %" PRIx64 " Hit = %d", banks[b], tags[b], hits[b]);
            for ( int i = 0; i< 4; i++) LOG(DEBUG, "\t\tAddress[%d] = %" PRIx64, i, address[b][i]);
            for ( int i = 0; i< 8; i++) LOG(DEBUG, "\t\tData[%d] = %" PRIx64, i, data[b][i]);
        }

        switch (num_banks)
        {
            case 1:
                read_texel(currentImage, i[0], j[0], data[0], texel_ul, output_result);
                read_texel(currentImage, i[1], j[0], data[0], texel_ur, output_result);
                read_texel(currentImage, i[0], j[1], data[0], texel_ll, output_result);
                read_texel(currentImage, i[1], j[1], data[0], texel_lr, output_result);
                break;
            case 2:
                // Crossing cache line in the vertical dimension
                if ((j[0] & 3) == 3)
                {
                    read_texel(currentImage, i[0], j[0], data[0], texel_ul, output_result);
                    read_texel(currentImage, i[1], j[0], data[0], texel_ur, output_result);
                    read_texel(currentImage, i[0], j[1], data[1], texel_ll, output_result);
                    read_texel(currentImage, i[1], j[1], data[1], texel_lr, output_result);
                }
                else    // Crossing cache line in the horizontal dimension
                {
                    read_texel(currentImage, i[0], j[0], data[0], texel_ul, output_result);
                    read_texel(currentImage, i[1], j[0], data[1], texel_ur, output_result);
                    read_texel(currentImage, i[0], j[1], data[0], texel_ll, output_result);
                    read_texel(currentImage, i[1], j[1], data[1], texel_lr, output_result);
                }
                break;
            case 4:
                read_texel(currentImage, i[0], j[0], data[0], texel_ul, output_result);
                read_texel(currentImage, i[1], j[0], data[1], texel_ur, output_result);
                read_texel(currentImage, i[0], j[1], data[2], texel_ll, output_result);
                read_texel(currentImage, i[1], j[1], data[3], texel_lr, output_result);
                break;
        }

#else
        read_bilinear_texels(currentImage, i, j, k[slice], l, sample_mip_level, texel_ul, texel_ur, texel_ll, texel_lr);
#endif
        if ((currentRequest.info.operation == SAMPLE_OP_SAMPLE_C) ||
            (currentRequest.info.operation == SAMPLE_OP_SAMPLE_C_L) ||
            (currentRequest.info.operation == SAMPLE_OP_GATHER4_C))
        {
            texel_ul[0] = compare_texel((CompareOperation)currentRequest.info.compop, fpu::FLT(t.u32[req]), texel_ul[0]);
            texel_ur[0] = compare_texel((CompareOperation)currentRequest.info.compop, fpu::FLT(t.u32[req]), texel_ur[0]);
            texel_ll[0] = compare_texel((CompareOperation)currentRequest.info.compop, fpu::FLT(t.u32[req]), texel_ll[0]);
            texel_lr[0] = compare_texel((CompareOperation)currentRequest.info.compop, fpu::FLT(t.u32[req]), texel_lr[0]);
            texel_ul[1] = 0.0;
            texel_ur[1] = 0.0;
            texel_ll[1] = 0.0;
            texel_lr[1] = 0.0;
            texel_ul[2] = 0.0;
            texel_ur[2] = 0.0;
            texel_ll[2] = 0.0;
            texel_lr[2] = 0.0;
            texel_ul[3] = 0.0;
            texel_ur[3] = 0.0;
            texel_ll[3] = 0.0;
            texel_lr[3] = 0.0;
        }

        if (   (currentRequest.info.operation == SAMPLE_OP_GATHER4)
            || (currentRequest.info.operation == SAMPLE_OP_GATHER4_C))
        {
            red   = texel_ul[currentRequest.info.component];
            green = texel_ur[currentRequest.info.component];
            blue  = texel_ll[currentRequest.info.component];
            alpha = texel_lr[currentRequest.info.component];
        }
        else
        {
            if (currentImage.info.type == IMAGE_TYPE_3D)
            {
                betak = (slice == 0) ? (1.0 - betak) : betak;

                red += (((texel_ul[0] * (1.0 - betai) + texel_ur[0] * betai) * (1.0 - betaj)
                         + (texel_ll[0] * (1.0 - betai) + texel_lr[0] * betai) * betaj)
                        * (1.0 - betak))
                       * sample_mip_beta;
                green += (((texel_ul[1] * (1.0 - betai) + texel_ur[1] * betai) * (1.0 - betaj)
                           + (texel_ll[1] * (1.0 - betai) + texel_lr[1] * betai) * betaj)
                          * (1.0 - betak))
                         * sample_mip_beta;
                blue += (((texel_ul[2] * (1.0 - betai) + texel_ur[2] * betai) * (1.0 - betaj)
                          + (texel_ll[2] * (1.0 - betai) + texel_lr[2] * betai) * betaj)
                         * (1.0 - betak))
                        * sample_mip_beta;
                alpha += (((texel_ul[3] * (1.0 - betai) + texel_ur[3] * betai) * (1.0 - betaj)
                           + (texel_ll[3] * (1.0 - betai) + texel_lr[3] * betai) * betaj)
                          * (1.0 - betak))
                         * sample_mip_beta;
            }
            else
            {
                red += ((texel_ul[0] * (1.0 - betai) + texel_ur[0] * betai) * (1.0 - betaj)
                        + (texel_ll[0] * (1.0 - betai) + texel_lr[0] * betai) * betaj)
                       * sample_mip_beta * aniso_weight;
                green += ((texel_ul[1] * (1.0 - betai) + texel_ur[1] * betai) * (1.0 - betaj)
                          + (texel_ll[1] * (1.0 - betai) + texel_lr[1] * betai) * betaj)
                         * sample_mip_beta * aniso_weight;
                blue += ((texel_ul[2] * (1.0 - betai) + texel_ur[2] * betai) * (1.0 - betaj)
                         + (texel_ll[2] * (1.0 - betai) + texel_lr[2] * betai) * betaj)
                        * sample_mip_beta * aniso_weight;
                alpha += ((texel_ul[3] * (1.0 - betai) + texel_ur[3] * betai) * (1.0 - betaj)
                          + (texel_ll[3] * (1.0 - betai) + texel_lr[3] * betai) * betaj)
                         * sample_mip_beta * aniso_weight;
            }
        }
    }

    if (output_result) {
        LOG(DEBUG, "\tResult = {0x%08x (%f), 0x%08x (%f), 0x%08x (%f), 0x%08x (%f)}",
            fpu::F2UI32(red), red, fpu::F2UI32(green), green,
            fpu::F2UI32(blue), blue, fpu::F2UI32(alpha), alpha);
    }
}

float TBOX::TBOXEmu::apply_component_swizzle(ComponentSwizzle swizzle, float source, float red, float green,
                                             float blue, float alpha)
{
    float swizzled_component;

    switch (swizzle)
    {
        // case COMPONENT_SWIZZLE_NONE     : swizzled_component = 0.0f / 0.0f; break;
        case COMPONENT_SWIZZLE_NONE:
        case COMPONENT_SWIZZLE_IDENTITY:
            swizzled_component = source;
            break;
        case COMPONENT_SWIZZLE_ZERO:
            swizzled_component = 0.0f;
            break;
        case COMPONENT_SWIZZLE_ONE:
            swizzled_component = 1.0f;
            break;
        case COMPONENT_SWIZZLE_R:
            swizzled_component = red;
            break;
        case COMPONENT_SWIZZLE_G:
            swizzled_component = green;
            break;
        case COMPONENT_SWIZZLE_B:
            swizzled_component = blue;
            break;
        case COMPONENT_SWIZZLE_A:
            swizzled_component = alpha;
            break;
        default:
            throw std::runtime_error("Unsupported component swizzle mode.");
    }

    return swizzled_component;
}

void TBOX::TBOXEmu::wrap_texel_coord(uint32_t c[2], int32_t c_ul, uint32_t mip_dim, AddressMode addrmode)
{
    switch (addrmode)
    {
        case ADDRESS_MODE_CLAMP_TO_EDGE:
            c[0] = min(uint32_t(max(0, c_ul)), mip_dim - 1);
            c[1] = min(uint32_t(max(0, c_ul + 1)), mip_dim - 1);
            break;

        case ADDRESS_MODE_REPEAT:
            // mod with non-negative reminder (mip_dim always >= 1)
            c[0] = c_ul >= 0 ? c_ul % mip_dim : mip_dim - 1 - (-(c_ul + 1) % mip_dim);
            c[1] = (c_ul + 1) >= 0 ? (c_ul + 1) % mip_dim : mip_dim - 1 - (-(c_ul + 1 + 1) % mip_dim);
            break;

        case ADDRESS_MODE_CLAMP_TO_BORDER:
            LOG(WARN, "%s", "CLAMP_TO_BORDER implemented as CLAMP_TO_EDGE.");
            c[0] = min(uint32_t(max(0, c_ul)), mip_dim - 1);
            c[1] = min(uint32_t(max(0, c_ul + 1)), mip_dim - 1);
            break;

        default:
            LOG(ERR, "%s", "Unsupported address mode.");
            break;
    }
}

uint64_t TBOX::TBOXEmu::compute_mip_offset(uint32_t mip_pitch_l0, uint32_t mip_pitch_l1, uint32_t row_pitch, uint32_t rows, uint32_t mip_level)
{
	uint64_t mip_offset = 0;
	uint64_t mip_row_pitch = row_pitch;
    uint32_t mip_rows = rows;
	uint64_t mip_pitch = (1 << mip_pitch_l1);

	for (uint32_t l = 0; l < mip_level; l++)
    {
		if (l == 0)
			mip_offset = (1 << mip_pitch_l0);
		else if (l == 1)
			mip_offset = mip_offset + (1 << mip_pitch_l1);
		else if ((mip_row_pitch == 0) or (mip_rows == 0))
        {
			mip_pitch = max(1, mip_pitch >> 1);
			mip_offset = mip_offset + mip_pitch;
            if (mip_pitch == 1) return mip_offset;
        }
		else
        {
			mip_pitch = max(1, mip_pitch >> 2);
			mip_offset += mip_pitch;
            if (mip_pitch == 1) return mip_offset;
        }
        mip_rows = mip_rows >>  1;
		mip_row_pitch = mip_row_pitch >> 1;
    }
	return mip_offset;
}

void TBOX::TBOXEmu::compute_packed_mip_offset(ImageInfo currentImage,
                                              uint32_t bytesTexel __attribute__((unused)),
                                              bool isCompressed __attribute__((unused)),
                                              uint32_t tileWidthLog2, uint32_t tileHeightLog2,
                                              uint32_t mip_level, uint32_t mip_offset[])
{
    uint32_t packed_level = currentImage.info.packedlevel - (mip_level - currentImage.info.packedmip);

    uint32_t packed_dim;
    uint32_t side_step;

    if (currentImage.info.packedlayout == 0)
    {
        // Vertical packing.
        packed_dim = tileHeightLog2;
        uint32_t image_width = currentImage.info.width + 1;
        image_width = image_width >> 2;
        side_step = max(1, image_width >> (currentImage.info.packedmip + 1));
    }
    else
    {
        // Horizontal packing.
        packed_dim = tileWidthLog2;
        uint32_t image_height = currentImage.info.height + 1;
        image_height = image_height >> 2;
        side_step = max(1, image_height >> (currentImage.info.packedmip + 1));
    }

    // Vertical packing.
    switch (packed_dim)
    {
        case 8 :
            switch (packed_level)
            {
                case 7 : mip_offset[1] =   0; break;
                case 6 : mip_offset[1] = 128; break;
                case 5 : mip_offset[1] = 128; mip_offset[0] = side_step; break;
                case 4 : mip_offset[1] = 192; break;
                case 3 : mip_offset[1] = 208; break;
                case 2 : mip_offset[1] = 216; break;
                case 1 : mip_offset[1] = 220; break;
                case 0 : mip_offset[1] = 224; break;
                default : throw std::runtime_error("Undefined packed level.");
            }
            break;
        case 7 :
            switch (packed_level)
            {
                case 6 : mip_offset[1] =  0; break;
                case 5 : mip_offset[1] =  64; break;
                case 4 : mip_offset[1] =  64; mip_offset[0] = side_step; break;
                case 3 : mip_offset[1] =  96; break;
                case 2 : mip_offset[1] = 104; break;
                case 1 : mip_offset[1] = 108; break;
                case 0 : mip_offset[1] = 112; break;
                default : throw std::runtime_error("Undefined packed level.");
            }
            break;
        case 6 :
            switch (packed_level)
            {
                case 5 : mip_offset[1] =  0; break;
                case 4 : mip_offset[1] = 32; break;
                case 3 : mip_offset[1] = 32; mip_offset[0] = side_step; break;
                case 2 : mip_offset[1] = 48; break;
                case 1 : mip_offset[1] = 52; break;
                case 0 : mip_offset[1] = 56; break;
                default : throw std::runtime_error("Undefined packed level.");
            }
            break;
        default :
            throw std::runtime_error("Unsupported tile height.");
    }

    if (currentImage.info.packedlayout == 1)
    {
        uint32_t temp = mip_offset[0];
        mip_offset[0] = mip_offset[1];
        mip_offset[1] = temp;
    }
}

uint64_t TBOX::TBOXEmu::compute_tile_offset(uint32_t bytesTexel, uint32_t tile_i, uint32_t tile_j)
{
    uint64_t tile_offset = 0;
    switch(bytesTexel)
    {
	    // 256 x 256   XYXY_XYXY_YYYY_XXXX
        case 1 : tile_offset = ((tile_i & 0x080) << (15 - 7))
                             | ((tile_j & 0x080) << (14 - 7))
                             | ((tile_i & 0x040) << (13 - 6))
                             | ((tile_j & 0x040) << (12 - 6))
                             | ((tile_i & 0x020) << (11 - 5))
                             | ((tile_j & 0x020) << (10 - 5))
                             | ((tile_i & 0x010) << ( 9 - 4))
                             | ((tile_j & 0x010) << ( 8 - 4))
                             | ((tile_j & 0x00f) <<  4)
                             |  (tile_i & 0x00f); break;
	    // 256 x 128   XYXY_XYXY_XYYY_XXXT
        case 2 : tile_offset = ((tile_i & 0x080) << (15 - 7))
                             | ((tile_j & 0x040) << (14 - 6))
                             | ((tile_i & 0x040) << (13 - 6))
                             | ((tile_j & 0x020) << (12 - 5))
                             | ((tile_i & 0x020) << (11 - 5))
                             | ((tile_j & 0x010) << (10 - 4))
                             | ((tile_i & 0x010) << ( 9 - 4))
                             | ((tile_j & 0x008) << ( 8 - 3))
                             | ((tile_i & 0x008) << ( 7 - 3))
                             | ((tile_j & 0x007) <<       4)
                             | ((tile_i & 0x007) <<       1); break;
	    // 128 x 128   XYXY_XYXY_XYYY_XXTT
        case 4 : tile_offset = ((tile_i & 0x040) << (15 - 6))
                             | ((tile_j & 0x040) << (14 - 6))
                             | ((tile_i & 0x020) << (13 - 5))
                             | ((tile_j & 0x020) << (12 - 5))
                             | ((tile_i & 0x010) << (11 - 4))
                             | ((tile_j & 0x010) << (10 - 4))
                             | ((tile_i & 0x008) << ( 9 - 3))
                             | ((tile_j & 0x008) << ( 8 - 3))
                             | ((tile_i & 0x004) << ( 7 - 2))
                             | ((tile_j & 0x007) <<       4)
                             | ((tile_i & 0x003) <<       2); break;
	    // 128 x  64   XYXY_XYXY_XXYY_XTTT
        case 8 : tile_offset = ((tile_i & 0x040) << (15 - 6))
                             | ((tile_j & 0x020) << (14 - 5))
                             | ((tile_i & 0x020) << (13 - 5))
                             | ((tile_j & 0x010) << (12 - 4))
                             | ((tile_i & 0x010) << (11 - 4))
                             | ((tile_j & 0x008) << (10 - 3))
                             | ((tile_i & 0x008) << ( 9 - 3))
                             | ((tile_j & 0x004) << ( 8 - 2))
                             | ((tile_i & 0x006) << ( 6 - 1))
                             | ((tile_j & 0x003) <<       4)
                             | ((tile_i & 0x001) <<       3); break;
	    //  64 x  64   XYXY_XYXY_XXYY_TTTT
        case 16 : tile_offset = ((tile_i & 0x020) << (15 - 5))
                              | ((tile_j & 0x020) << (14 - 5))
                              | ((tile_i & 0x010) << (13 - 4))
                              | ((tile_j & 0x010) << (12 - 4))
                              | ((tile_i & 0x008) << (11 - 3))
                              | ((tile_j & 0x008) << (10 - 3))
                              | ((tile_i & 0x004) << ( 9 - 2))
                              | ((tile_j & 0x004) << ( 8 - 2))
                              | ((tile_i & 0x003) <<       6)
                              | ((tile_j & 0x003) <<       4); break;
        default : throw std::runtime_error("Unsupported bytes per texel.");
    }

    return tile_offset;
}


void TBOX::TBOXEmu::read_bilinear_texels(ImageInfo currentImage, uint32_t i[2], uint32_t j[2], uint32_t k, uint32_t l,
                                         uint32_t sample_mip_level, float *texel_ul, float *texel_ur,
                                         float *texel_ll, float *texel_lr)
{
    read_texel(currentImage, i[0], j[0], k, l, sample_mip_level, texel_ul);
    read_texel(currentImage, i[1], j[0], k, l, sample_mip_level, texel_ur);
    read_texel(currentImage, i[0], j[1], k, l, sample_mip_level, texel_ll);
    read_texel(currentImage, i[1], j[1], k, l, sample_mip_level, texel_lr);
}

#define CREATE_TAG(i, j) ((((((((currentRequest.info.imageid << 5) | uint64_t(mip_level)) << 16) | uint64_t(i)) << 14) | (uint64_t(j) >> 2)) << 12) | k | l)

void TBOX::TBOXEmu::create_texture_cache_tags(SampleRequest currentRequest, ImageInfo currentImage,
                                              FilterType filter, uint32_t i[2], uint32_t j[2], uint32_t k, uint32_t l,
                                              uint32_t mip_level, uint32_t &num_banks, int32_t banks[4], uint64_t tags[4],
                                              uint64_t address[4][4])
{
    uint32_t fmtBytesPerTexel;
    uint32_t decompBytesPerTexel;
    uint32_t comprBlockWidth;
    uint32_t comprBlockHeight;

    LOG(DEBUG, "\tCreate Texture Cache Tags for UL (%d, %d, %d) layer %d mip_level %d", i[0], j[0], k, l, mip_level);

    ImageFormat fmt = (ImageFormat)currentImage.info.format;

    bool fmtIsCompressed = isCompressedFormat(fmt);

    //  1. Compute address at the compressed block level.
    //  2. Get the actual texel after decompressing the compressed block.
    if (fmtIsCompressed)
    {
        // 1.1. Get compressed block horizontal and vertical coordinates.
        CompressedFormatInfo compInfo = getCompressedFormatInfo(fmt);

        comprBlockWidth = compInfo.blockWidth;
        comprBlockHeight = compInfo.blockHeight;
        uint32_t comprBlockI = i[0] >> comprBlockWidth;
        uint32_t comprBlockJ = j[0] >> comprBlockHeight;

        // 1.2. Get address at the compressed block level.
        address[0][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
        address[0][0] |= (i[0] >> 1) & 0x1;
        //-1.2.-------------------------------------------

        decompBytesPerTexel = BYTES_PER_TEXEL_IN_L1[fmt];

        if (decompBytesPerTexel == 4)
        {
            tags[0] = CREATE_TAG(i[0] & ~0x03, j[0]);

            num_banks = 1;
            banks[0] = ((i[0] >> 2) ^ (j[0] >> 2)) & 1;

            if (filter == FILTER_TYPE_LINEAR)
            {
                if (((i[0] & 3) == 3) && ((j[0] & 3) != 3))
                {
                    num_banks = 2;
                    banks[1] = ~banks[0] & 1;
                    tags[1] = CREATE_TAG(i[1] & ~0x03, j[0]);

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[0] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                }
                else if (((i[0] & 3) != 3) && ((j[0] & 3) == 3))
                {
                    num_banks = 2;
                    banks[1] = ~banks[0] & 1;
                    tags[1] = CREATE_TAG(i[0] & ~0x03, j[1]);

                    comprBlockI = i[0] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                }
                else if (((i[0] & 3) == 3) && ((j[0] & 3) == 3))
                {
                    num_banks = 4;
                    banks[1] = (~banks[0]) & 1;
                    banks[2] = banks[0];
                    banks[3] = banks[1];
                    tags[1] = CREATE_TAG(i[1] & ~0x03, j[0]);
                    tags[2] = CREATE_TAG(i[0] & ~0x03, j[1]);
                    tags[3] = CREATE_TAG(i[1] & ~0x03, j[1]);

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[0] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);

                    comprBlockI = i[0] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[2][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[3][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                }
            }
        }
        else if (decompBytesPerTexel == 8)
        {
            tags[0] = CREATE_TAG(i[0] & ~0x01, j[0]);

            num_banks = 1;
            banks[0] = ((i[0] >> 1) ^ (j[0] >> 2)) & 1;

            if (filter == FILTER_TYPE_LINEAR)
            {
                if (((i[0] & 1) == 1) && ((j[0] & 3) != 3))
                {
                    num_banks = 2;
                    banks[1] = ~banks[0] & 1;
                    tags[1] = CREATE_TAG(i[1] & ~0x01, j[0]);

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[0] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                    address[1][0] |= (i[1] >> 1) & 0x1;
                }
                else if (((i[0] & 1) != 1) && ((j[0] & 3) == 3))
                {
                    num_banks = 2;
                    banks[1] = ~banks[0] & 1;
                    tags[1] = CREATE_TAG(i[0] & ~0x01, j[1]);

                    comprBlockI = i[0] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                    address[1][0] |= (i[0] >> 1) & 0x1;
                }
                else if (((i[0] & 1) == 1) && ((j[0] & 3) == 3))
                {
                    num_banks = 4;
                    banks[1] = (~banks[0]) & 1;
                    banks[2] = banks[0];
                    banks[3] = banks[1];
                    tags[1] = CREATE_TAG(i[1] & ~0x01, j[0]);
                    tags[2] = CREATE_TAG(i[0] & ~0x01, j[1]);
                    tags[3] = CREATE_TAG(i[1] & ~0x01, j[1]);

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[0] >> comprBlockHeight;
                    address[1][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                    address[1][0] |= (i[1] >> 1) & 0x1;

                    comprBlockI = i[0] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[2][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                    address[2][0] |= (i[0] >> 1) & 0x1;

                    comprBlockI = i[1] >> comprBlockWidth;
                    comprBlockJ = j[1] >> comprBlockHeight;
                    address[3][0] = texel_virtual_address(currentImage, comprBlockI, comprBlockJ, k, l, mip_level);
                    address[3][0] |= (i[1] >> 1) & 0x1;
                }
            }
        }
    }
    else
    {
        fmtBytesPerTexel = BYTES_PER_TEXEL_IN_L1[fmt];

        switch (fmtBytesPerTexel)
        {
            case 4:
                {
                    address[0][0] = texel_virtual_address(currentImage, i[0] & ~0x03,  j[0] & ~0x03,      k, l, mip_level);
                    address[0][1] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[0] & ~0x03) + 1, k, l, mip_level);
                    address[0][2] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[0] & ~0x03) + 2, k, l, mip_level);
                    address[0][3] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[0] & ~0x03) + 3, k, l, mip_level);
                    tags[0] = CREATE_TAG(i[0] & ~0x03, j[0]);
                    num_banks = 1;
                    banks[0] = (((i[0] ^ j[0]) & 4) >> 2) & 1;

                    if (filter == FILTER_TYPE_LINEAR)
                    {
                        if (((i[0] & 3) == 3) && ((j[0] & 3) != 3))
                        {
                            num_banks = 2;
                            banks[1] = (~banks[0]) & 1;
                            address[1][0] = texel_virtual_address(currentImage, i[1] & ~0x03,  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1] & ~0x03, j[0]);
                        }
                        else if (((i[0] & 3) != 3) && ((j[0] & 3) == 3))
                        {
                            num_banks = 2;
                            banks[1] = (~banks[0]) & 1;
                            address[1][0] = texel_virtual_address(currentImage, i[0] & ~0x03,  j[1] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[0] & ~0x03, j[1]);
                        }
                        else if (((i[0] & 3) == 3) && ((j[0] & 3) == 3))
                        {
                            num_banks = 4;
                            banks[1] = (~banks[0]) & 1;
                            banks[2] = banks[1];
                            banks[3] = banks[0];
                            address[1][0] = texel_virtual_address(currentImage, i[1] & ~0x03,  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[0] & ~0x03) + 3, k, l, mip_level);
                            address[2][0] = texel_virtual_address(currentImage, i[0] & ~0x03,  j[1] & ~0x03     , k, l, mip_level);
                            address[2][1] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[2][2] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[2][3] = texel_virtual_address(currentImage, i[0] & ~0x03, (j[1] & ~0x03) + 3, k, l, mip_level);
                            address[3][0] = texel_virtual_address(currentImage, i[1] & ~0x03,  j[1] & ~0x03     , k, l, mip_level);
                            address[3][1] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[3][2] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[3][3] = texel_virtual_address(currentImage, i[1] & ~0x03, (j[1] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1] & ~0x03, j[0]);
                            tags[2] = CREATE_TAG(i[0] & ~0x03, j[1]);
                            tags[3] = CREATE_TAG(i[1] & ~0x03, j[1]);
                        }
                    }
                }
                break;
            case 8:
                {
                    address[0][0] = texel_virtual_address(currentImage, i[0] & ~0x01,  j[0] & ~0x03     , k, l, mip_level);
                    address[0][1] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[0] & ~0x03) + 1, k, l, mip_level);
                    address[0][2] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[0] & ~0x03) + 2, k, l, mip_level);
                    address[0][3] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[0] & ~0x03) + 3, k, l, mip_level);
                    tags[0] = CREATE_TAG(i[0] & ~0x01, j[0]);
                    num_banks = 1;
                    banks[0] = ((i[0] >> 1) ^ (j[0] >> 2)) & 1;

                    if (filter == FILTER_TYPE_LINEAR)
                    {
                        if (((i[0] & 1) == 1) && ((j[0] & 3) != 3))
                        {
                            num_banks = 2;
                            banks[1] = ~banks[0] & 1;
                            address[1][0] = texel_virtual_address(currentImage, i[1] & ~0x01,  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1] & ~0x01, j[0]);
                        }
                        else if (((i[0] & 1) != 1) && ((j[0] & 3) == 3))
                        {
                            num_banks = 2;
                            banks[1] = ~banks[0] & 1;
                            address[1][0] = texel_virtual_address(currentImage, i[0] & ~0x01,  j[1] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[0] & ~0x01, j[1]);
                        }
                        else if (((i[0] & 1) == 1) && ((j[0] & 3) == 3))
                        {
                            num_banks = 4;
                            banks[1] = (~banks[0]) & 1;
                            banks[2] = banks[0];
                            banks[3] = banks[1];
                            address[1][0] = texel_virtual_address(currentImage, i[1] & ~0x01,  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 1, k, l, mip_level);
                            address[1][2] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 2, k, l, mip_level);
                            address[1][3] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[0] & ~0x03) + 3, k, l, mip_level);
                            address[2][0] = texel_virtual_address(currentImage, i[0] & ~0x01,  j[1] & ~0x03     , k, l, mip_level);
                            address[2][1] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[2][2] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[2][3] = texel_virtual_address(currentImage, i[0] & ~0x01, (j[1] & ~0x03) + 3, k, l, mip_level);
                            address[3][0] = texel_virtual_address(currentImage, i[1] & ~0x01,  j[1] & ~0x03     , k, l, mip_level);
                            address[3][1] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[3][2] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[1] & ~0x03) + 2, k, l, mip_level);
                            address[3][3] = texel_virtual_address(currentImage, i[1] & ~0x01, (j[1] & ~0x03) + 3, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1] & ~0x01, j[0]);
                            tags[2] = CREATE_TAG(i[0] & ~0x01, j[1]);
                            tags[3] = CREATE_TAG(i[1] & ~0x01, j[1]);
                        }
                    }
                }
                break;
            case 16:
                {
                    address[0][0] = texel_virtual_address(currentImage, i[0],  j[0] & ~0x03     , k, l, mip_level);
                    address[0][1] = texel_virtual_address(currentImage, i[0], (j[0] & ~0x03) + 1, k, l, mip_level);
                    tags[0] = CREATE_TAG(i[0], j[0]);
                    num_banks = 1;
                    banks[0] = i[0] & 1;

                    if (filter == FILTER_TYPE_LINEAR)
                    {
                        if ((j[0] & 0x03) != 3)
                        {
                            num_banks = 2;
                            banks[1] = ~banks[0] & 1;
                            address[1][0] = texel_virtual_address(currentImage, i[1],  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1], (j[0] & ~0x03) + 1, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1], j[0]);
                        }
                        else
                        {
                            num_banks = 4;
                            banks[1] = (~banks[0]) & 1;
                            banks[2] = banks[0];
                            banks[3] = banks[1];
                            address[1][0] = texel_virtual_address(currentImage, i[1],  j[0] & ~0x03     , k, l, mip_level);
                            address[1][1] = texel_virtual_address(currentImage, i[1], (j[0] & ~0x03) + 1, k, l, mip_level);
                            address[2][0] = texel_virtual_address(currentImage, i[0],  j[1] & ~0x03     , k, l, mip_level);
                            address[2][1] = texel_virtual_address(currentImage, i[0], (j[1] & ~0x03) + 1, k, l, mip_level);
                            address[3][0] = texel_virtual_address(currentImage, i[1],  j[1] & ~0x03     , k, l, mip_level);
                            address[3][1] = texel_virtual_address(currentImage, i[1], (j[1] & ~0x03) + 1, k, l, mip_level);
                            tags[1] = CREATE_TAG(i[1], j[0]);
                            tags[2] = CREATE_TAG(i[0], j[1]);
                            tags[3] = CREATE_TAG(i[1], j[1]);
                        }
                    }
                }
                break;
        }
    }
}

uint64_t TBOX::TBOXEmu::texel_virtual_address(ImageInfo currentImage, uint32_t i, uint32_t j, uint32_t k, uint32_t l,
                                              uint32_t mip_level)
{
    uint32_t fmtBytesPerTexel = -1;
    uint32_t fmtTileWidthLog2 = -1;  // Outer tile width in texels
    uint32_t fmtTileHeightLog2 = -1; // Outer tile height in texels

    ImageFormat fmt = (ImageFormat)currentImage.info.format;

    bool fmtIsCompressed = isCompressedFormat(fmt);

    uint32_t height = currentImage.info.height + 1; // The height is coded from [0, height-1] in the table, but the range is [1, height]

    if (fmtIsCompressed)
    {
        //  Compute address at the compressed block level.
        //  Get the actual texel after decompressing the compressed block.
        CompressedFormatInfo compInfo = getCompressedFormatInfo(fmt);

        fmtBytesPerTexel = compInfo.blockBytes;

        fmt = compInfo.format;

        if (fmtBytesPerTexel == 8)
        {
            fmtTileWidthLog2  = 7;
            fmtTileHeightLog2 = 6;
        }
        else if (fmtBytesPerTexel == 16)
        {
            fmtTileWidthLog2  = 6;
            fmtTileHeightLog2 = 6;
        }

        height = max(1, height >> 2);
    }
    else
    {
        fmtBytesPerTexel = BYTES_PER_TEXEL_IN_MEMORY[fmt];

        switch (fmtBytesPerTexel)
        {
            case 1:
                fmtTileWidthLog2  = 8;
                fmtTileHeightLog2 = 8;
                break;
            case 2:
                fmtTileWidthLog2  = 8;
                fmtTileHeightLog2 = 7;
                break;
            case 3: // Standard Tile Layout padding. 24 to 32 bits
                if (currentImage.info.tiled == 1) fmtBytesPerTexel = 4;
                fmtTileWidthLog2  = 7;
                fmtTileHeightLog2 = 7;
                break;
            case 4:
                fmtTileWidthLog2  = 7;
                fmtTileHeightLog2 = 7;
                break;
            case 6: // Standard Tile Layout padding. 48 to 64 bits
                if (currentImage.info.tiled == 1) fmtBytesPerTexel = 8;
                fmtTileWidthLog2  = 7;
                fmtTileHeightLog2 = 6;
                break;
            case 8:
                fmtTileWidthLog2  = 7;
                fmtTileHeightLog2 = 6;
                break;
            case 12: // Standard Tile Layout padding. 96 to 128 bits
                if (currentImage.info.tiled == 1) fmtBytesPerTexel = 16;
                fmtTileWidthLog2  = 6;
                fmtTileHeightLog2 = 6;
                break;
            case 16:
                fmtTileWidthLog2  = 6;
                fmtTileHeightLog2 = 6;
                break;
            default:
                throw std::runtime_error("No Bytes Per Texel defined for format.");
        }
    }

    uint32_t rows;
    uint64_t texelAddress;

    uint64_t row_pitch = max(1, currentImage.info.rowpitch >> mip_level); // Number of columns in outer tile size (standard tile layout). Number of columns in 64 B tiles (linear layout)

    if (currentImage.info.tiled == 1) // Standard tile layout
    {
        rows = max(1, (height >> fmtTileHeightLog2) + (((height & ((1 << fmtTileHeightLog2) - 1)) != 0) ? 1 : 0)); // max(1, ceil(height/tileHeight))

        uint64_t mip_pitch;
        uint32_t mip_offset[2] = {0, 0};

        // Access larger mip
        if (mip_level < currentImage.info.packedmip)
        {
            mip_pitch = compute_mip_offset(currentImage.info.mippitchl0, currentImage.info.mippitchl1, currentImage.info.rowpitch, rows, mip_level);
        }
        // Access packed mip
        else
        {
            mip_pitch = compute_mip_offset(currentImage.info.mippitchl0, currentImage.info.mippitchl1, currentImage.info.rowpitch, rows, currentImage.info.packedmip); // offset of first packed mip
            compute_packed_mip_offset(currentImage, fmtBytesPerTexel, fmtIsCompressed, fmtTileWidthLog2, fmtTileHeightLog2, mip_level, mip_offset);                    // offset of target mip
        }

        uint64_t layout_i = i + mip_offset[0];
        uint64_t layout_j = j + mip_offset[1];
        uint64_t tile_i = layout_i >> fmtTileWidthLog2;
        uint64_t tile_j = layout_j >> fmtTileHeightLog2;
        uint64_t tile_offset = tile_j * row_pitch + tile_i;
        uint64_t pixel_offset = compute_tile_offset(fmtBytesPerTexel, layout_i & ((1 << fmtTileWidthLog2) - 1), layout_j & ((1 << fmtTileHeightLog2) - 1));

        LOG(DEBUG, "\t\telement_pitch = %ld mip_pitch = %ld mip_offset = (%d, %d) tile_offset = %ld pixel_offset = %ld",
                          currentImage.info.elementpitch, mip_pitch, mip_offset[0], mip_offset[1], tile_offset, pixel_offset);

        texelAddress = currentImage.info.address + (currentImage.info.elementpitch * l + mip_pitch + tile_offset) * 64 * 1024
                     + pixel_offset;
    }
    else    // Linear layout
    {
        rows = height;

        uint64_t mip_pitch = compute_mip_offset(currentImage.info.mippitchl0, currentImage.info.mippitchl1, currentImage.info.rowpitch, rows, mip_level);

        LOG(DEBUG, "\t\telement_pitch = %ld mip_pitch = %ld row_pitch = %ld",
                          currentImage.info.elementpitch, mip_pitch, row_pitch);

        texelAddress = currentImage.info.address + 
                     + (currentImage.info.elementpitch * l + mip_pitch + j * row_pitch) * 64
                     + i * fmtBytesPerTexel;
    }

    LOG(DEBUG, "\tcomputed virtual address %016lx for texel at (%d, %d, %d) layer %d level %d", texelAddress,
                      i, j, k, l, mip_level);

    return texelAddress;
}

void TBOX::TBOXEmu::read_texel(ImageInfo currentImage, uint32_t i, uint32_t j, uint32_t k, uint32_t l, uint32_t mip_level,
                              float *texel)
{
    uint32_t fmtBytesPerTexel;
    uint32_t comprBlockI = 0;
    uint32_t comprBlockJ = 0;

    LOG(DEBUG, "\tread texel at (%d, %d, %d) layer %d level %d", i, j, k, l, mip_level);

    ImageFormat fmt = (ImageFormat)currentImage.info.format;

    bool fmtIsCompressed = isCompressedFormat(fmt);
    bool fmtIsSRGB = isSRGBFormat(fmt);

    if (fmtIsCompressed)
    {
        //  Compute address at the compressed block level.
        //  Get the actual texel after decompressing the compressed block.
        CompressedFormatInfo compInfo = getCompressedFormatInfo(fmt);

        // Get horizontal and vertical coodinates inside the compressed block.
        comprBlockI = i & ((1 << compInfo.blockWidth) - 1);
        comprBlockJ = j & ((1 << compInfo.blockHeight) - 1);

        // Get compressed block horizontal and vertical coordinates.
        i = i >> compInfo.blockWidth;
        j = j >> compInfo.blockHeight;

        fmtBytesPerTexel = compInfo.blockBytes;

        fmt = compInfo.format;
    }
    else
    {
        fmtBytesPerTexel = BYTES_PER_TEXEL_IN_MEMORY[fmt];
    }

    uint64_t texelAddress = texel_virtual_address(currentImage, i, j, k, l, mip_level);

    uint8_t data[16];
    switch (fmtBytesPerTexel)
    {
        case 1:
            {
                data[0] = bemu::pmemread<uint8_t>(texelAddress);
                LOG(DEBUG, "\t\t%02x <- PMEM8[%016lx]", data[0], texelAddress);
            }
            break;
        case 2:
            {
                uint16_t texelData = bemu::pmemread<uint16_t>(texelAddress);
                memcpy_uint16(&data[0], texelData);
                LOG(DEBUG, "\t\t%04x <- PMEM16[%016lx]", texelData, texelAddress);
            }
            break;
        case 4:
            {
                uint32_t texelData = bemu::pmemread<uint32_t>(texelAddress);
                memcpy_uint32(&data[0], texelData);
                LOG(DEBUG, "\t\t%08x <- PMEM32[%016lx]", texelData, texelAddress);
            }
            break;
        case 8:
            {
                uint64_t texelData = bemu::pmemread<uint64_t>(texelAddress);
                memcpy_uint64(&data[0], texelData);
                LOG(DEBUG, "\t\t%016lx <- PMEM64[%016lx]", texelData, texelAddress);
            }
            break;
        case 16:
            {
                uint64_t texelData = bemu::pmemread<uint64_t>(texelAddress);
                memcpy_uint64(&data[0], texelData);
                LOG(DEBUG, "\t\t%016lx <- PMEM64[%016lx]", texelData, texelAddress);
                texelData = bemu::pmemread<uint64_t>(texelAddress + 8);
                memcpy_uint64(&data[8], texelData);
                LOG(DEBUG, "\t\t%016lx <- PMEM64[%016lx]", texelData, texelAddress + 8);
            }
            break;
        default:
            throw std::runtime_error("Unsupported Bytes Per Texel value.");
    }

    if (fmtIsCompressed)
        getCompressedTexel((ImageFormat)currentImage.info.format, comprBlockI, comprBlockJ, data);

    switch (fmt)
    {
        case FORMAT_R8_UNORM:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R8G8_UNORM:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[1] = fpu::FLT(fpu::un8_to_f32(data[1]));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R8G8B8A8_UNORM:
        case FORMAT_R8G8B8A8_SRGB:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[1] = fpu::FLT(fpu::un8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::un8_to_f32(data[2]));
            texel[3] = fpu::FLT(fpu::un8_to_f32(data[3]));
            break;
        case FORMAT_R8G8B8A8_SNORM:
            texel[0] = fpu::FLT(fpu::sn8_to_f32(data[0]));
            texel[1] = fpu::FLT(fpu::sn8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::sn8_to_f32(data[2]));
            texel[3] = fpu::FLT(fpu::sn8_to_f32(data[3]));
            break;
        case FORMAT_B8G8R8A8_UNORM:
        case FORMAT_B8G8R8A8_SRGB:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[2]));
            texel[1] = fpu::FLT(fpu::un8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[3] = fpu::FLT(fpu::un8_to_f32(data[3]));
            break;
        case FORMAT_R16_UNORM:
            texel[0] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(&data[0])));
            texel[1] = 0.0f;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_SNORM:
            texel[0] = fpu::FLT(fpu::sn16_to_f32(cast_bytes_to_uint16(&data[0])));
            texel[1] = fpu::FLT(fpu::sn16_to_f32(cast_bytes_to_uint16(&data[2])));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_UNORM:
            texel[0] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(&data[0])));
            texel[1] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(&data[2])));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_SFLOAT:
            texel[0] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[0]))));
            texel[1] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[2]))));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16B16A16_SFLOAT:
            texel[0] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[0]))));
            texel[1] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[2]))));
            texel[2] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[4]))));
            texel[3] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[6]))));
            break;
        case FORMAT_D24_UNORM_S8_UINT:
            texel[0] = fpu::FLT(fpu::un24_to_f32(cast_bytes_to_uint24(data)));
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R32_SFLOAT:
            texel[0] = cast_bytes_to_float(data);
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R32G32B32A32_SFLOAT:
            texel[0] = cast_bytes_to_float(&data[0]);
            texel[1] = cast_bytes_to_float(&data[4]);
            texel[2] = cast_bytes_to_float(&data[8]);
            texel[3] = cast_bytes_to_float(&data[12]);
            break;
        case FORMAT_B10G11R11_UFLOAT_PACK32:
            texel[0] = fpu::FLT(fpu::f11_to_f32(fpu::F11( cast_bytes_to_uint32(data)        & 0x7ff)));
            texel[1] = fpu::FLT(fpu::f11_to_f32(fpu::F11((cast_bytes_to_uint32(data) >> 11) & 0x7ff)));
            texel[2] = fpu::FLT(fpu::f10_to_f32(fpu::F10((cast_bytes_to_uint32(data) >> 22) & 0x3ff)));
            texel[3] = 1.0;
            break;
        case FORMAT_A2B10G10R10_UNORM_PACK32:
            texel[0] = fpu::FLT(fpu::un10_to_f32( cast_bytes_to_uint32(data)        & 0x3ff));
            texel[1] = fpu::FLT(fpu::un10_to_f32((cast_bytes_to_uint32(data) >> 10) & 0x3ff));
            texel[2] = fpu::FLT(fpu::un10_to_f32((cast_bytes_to_uint32(data) >> 20) & 0x3ff));
            texel[3] = fpu::FLT( fpu::un2_to_f32((cast_bytes_to_uint32(data) >> 29) & 0x3));
            break;
        default:
            texel[0] = texel[1] = texel[2] = texel[3] = 0.0;
            LOG(ERR, "Format %ld not supported", currentImage.info.format);
            break;
    }

    if (fmtIsSRGB)
    {
        texel[0] = fpu::FLT(fpu::f16_to_f32(fpu::F16(SRGB2LINEAR_TABLE[fpu::f32_to_un8(fpu::F2F32(texel[0]))].value)));
        texel[1] = fpu::FLT(fpu::f16_to_f32(fpu::F16(SRGB2LINEAR_TABLE[fpu::f32_to_un8(fpu::F2F32(texel[1]))].value)));
        texel[2] = fpu::FLT(fpu::f16_to_f32(fpu::F16(SRGB2LINEAR_TABLE[fpu::f32_to_un8(fpu::F2F32(texel[2]))].value)));
        texel[3] = texel[3];
    }

    LOG(DEBUG, "\t\tTexel value = (%f, %f, %f, %f)", texel[0], texel[1], texel[2], texel[3]);
}

void TBOX::TBOXEmu::read_texel(ImageInfo currentImage, uint32_t i, uint32_t j,
                              uint64_t line_data[TEXTURE_CACHE_QWORDS_PER_LINE], float *texel, bool data_ready)
{
    if (!data_ready)
    {
        texel[0] = -999.999;
        texel[1] =  999.999;
        texel[2] = -999.999;
        texel[3] =  999.999;
        return;
    }

    uint32_t fmtBytesPerTexel;

    LOG(DEBUG, "\tread texel at (%d, %d)", i, j);

    ImageFormat fmt = (ImageFormat)currentImage.info.format;

    bool fmtIsCompressed = isCompressedFormat(fmt);
    bool fmtIsSRGB = isSRGBFormat(fmt);

    fmtBytesPerTexel = BYTES_PER_TEXEL_IN_L1[fmt];

    if (fmtIsCompressed)
    {
        //  Compute address at the compressed block level.
        //  Get the actual texel after decompressing the compressed block.
        CompressedFormatInfo compInfo = getCompressedFormatInfoL1(fmt);
        fmt = compInfo.format;
    
        // sRGB conversion from unorm8 to float16 already performed for the data in the cache.
        if (fmtIsSRGB)
            fmt = FORMAT_R16G16B16A16_SFLOAT;
    }

    uint8_t data[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    switch (fmtBytesPerTexel)
    {
        case 4:
            {
                uint32_t texelData = ((uint32_t *)line_data)[((j & 3) << 2) + (i & 3)];
                memcpy_uint32(&data[0], texelData);
                LOG(DEBUG, "\t\tcache line texel (%d, %d) : %08" PRIx32, i & 3, j & 3, texelData);
            }
            break;
        case 8:
            {
                uint64_t texelData = line_data[((j & 3) << 1) + (i & 1)];
                memcpy_uint64(&data[0], texelData);
                LOG(DEBUG, "\t\tcache line texel (%d, %d) : %016" PRIx64, i & 1, j & 3, texelData);
            }
            break;
        case 16:
            {
                uint64_t texelData = line_data[((j & 3) << 1)];
                memcpy_uint64(&data[0], texelData);
                texelData = line_data[((j & 3) << 1) + 1];
                memcpy_uint64(&data[8], texelData);
                LOG(DEBUG, "\t\tcache line texel (0, %d) : %016" PRIx64 " %016" PRIx64, j & 3, cast_bytes_to_uint64(&data[0]), cast_bytes_to_uint64(&data[8]));
            }
            break;
        default:
            throw std::runtime_error("Unimplemented bytes per texel value.");
    }

    switch (fmt)
    {
        case FORMAT_R8G8B8A8_UNORM:
        case FORMAT_R8G8B8A8_SRGB:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[1] = fpu::FLT(fpu::un8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::un8_to_f32(data[2]));
            texel[3] = fpu::FLT(fpu::un8_to_f32(data[3]));
            break;
        case FORMAT_R8G8B8A8_SNORM:
            texel[0] = fpu::FLT(fpu::sn8_to_f32(data[0]));
            texel[1] = fpu::FLT(fpu::sn8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::sn8_to_f32(data[2]));
            texel[3] = fpu::FLT(fpu::sn8_to_f32(data[3]));
            break;
        case FORMAT_B8G8R8A8_UNORM:
        case FORMAT_B8G8R8A8_SRGB:
            texel[0] = fpu::FLT(fpu::un8_to_f32(data[2]));
            texel[1] = fpu::FLT(fpu::un8_to_f32(data[1]));
            texel[2] = fpu::FLT(fpu::un8_to_f32(data[0]));
            texel[3] = fpu::FLT(fpu::un8_to_f32(data[3]));
            break;
        case FORMAT_R16_UNORM:
            texel[0] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(data)));
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_SNORM:
            texel[0] = fpu::FLT(fpu::sn16_to_f32(cast_bytes_to_uint16(&data[0])));
            texel[1] = fpu::FLT(fpu::sn16_to_f32(cast_bytes_to_uint16(&data[2])));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_UNORM:
            texel[0] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(&data[0])));
            texel[1] = fpu::FLT(fpu::un16_to_f32(cast_bytes_to_uint16(&data[2])));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16_SFLOAT:
            texel[0] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[0]))));
            texel[1] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[2]))));
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R16G16B16A16_SFLOAT:
            texel[0] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[0]))));
            texel[1] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[2]))));
            texel[2] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[4]))));
            texel[3] = fpu::FLT(fpu::f16_to_f32(fpu::F16(cast_bytes_to_uint16(&data[6]))));
            break;
        case FORMAT_D24_UNORM_S8_UINT:
            texel[0] = fpu::FLT(fpu::un24_to_f32(cast_bytes_to_uint24(data)));
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R32_SFLOAT:
            texel[0] = fpu::FLT(data[0]);
            texel[1] = 0.0;
            texel[2] = 0.0;
            texel[3] = 1.0;
            break;
        case FORMAT_R32G32B32A32_SFLOAT:
            texel[0] = fpu::FLT(data[0]);
            texel[1] = fpu::FLT(data[1]);
            texel[2] = fpu::FLT(data[2]);
            texel[3] = fpu::FLT(data[3]);
            break;
        default:
            texel[0] = texel[1] = texel[2] = texel[3] = 0.0;
            LOG(ERR, "Format %ld not supported", currentImage.info.format);
            break;
    }

    LOG(DEBUG, "\t\tTexel value = (%f, %f, %f, %f)", texel[0], texel[1], texel[2], texel[3]);
}

float TBOX::TBOXEmu::compare_texel(CompareOperation compop, float reference, float input)
{
    switch (compop)
    {
        case COMPARE_OP_NEVER            : return 0.0;
        case COMPARE_OP_LESS             : return reference < input;
        case COMPARE_OP_EQUAL            : return reference == input;
        case COMPARE_OP_LESS_OR_EQUAL    : return reference <= input;
        case COMPARE_OP_GREATER          : return reference > input;
        case COMPARE_OP_NOT_EQUAL        : return reference != input;
        case COMPARE_OP_GREATER_OR_EQUAL : return reference >= input;
        case COMPARE_OP_ALWAYS           : return 1.0;
        default:
            throw std::runtime_error("Unsupported compare mode.");
    }
}

const char *TBOX::TBOXEmu::toStrSampleOperation(SampleOperation op)
{
    switch (op)
    {
        case SAMPLE_OP_SAMPLE     : return "SAMPLE";
        case SAMPLE_OP_SAMPLE_L   : return "SAMPLE_L";
        case SAMPLE_OP_SAMPLE_C   : return "SAMPLE_C";
        case SAMPLE_OP_SAMPLE_C_L : return "SAMPLE_C_L";
        case SAMPLE_OP_GATHER4    : return "GATHER4";
        case SAMPLE_OP_GATHER4_C  : return "GATHER4_C";
        case SAMPLE_OP_LD         : return "LD";
        default                   : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrFilterType(FilterType type)
{
    switch (type)
    {
        case FILTER_TYPE_NEAREST : return "NEAREST";
        case FILTER_TYPE_LINEAR  : return "LINEAR";
        default                  : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrAddressMode(AddressMode am)
{
    switch (am)
    {
        case ADDRESS_MODE_REPEAT               : return "REPEAT";
        case ADDRESS_MODE_MIRRORED_REPEAT      : return "MIRRORED_REPEAT";
        case ADDRESS_MODE_CLAMP_TO_EDGE        : return "CLAMP_TO_EDGE";
        case ADDRESS_MODE_CLAMP_TO_BORDER      : return "CLAMP_TO_BORDER";
        case ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : return "CLAMP_TO_EDGE";
        default                                : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrCompareOperation(CompareOperation cop)
{
    switch (cop)
    {
        case COMPARE_OP_NEVER            : return "NEVER";
        case COMPARE_OP_LESS             : return "LESS";
        case COMPARE_OP_EQUAL            : return "EQUAL";
        case COMPARE_OP_LESS_OR_EQUAL    : return "LESS_OR_EQUAL";
        case COMPARE_OP_NOT_EQUAL        : return "NOT_EQUAL";
        case COMPARE_OP_GREATER_OR_EQUAL : return "GREATER_OR_EQUAL";
        case COMPARE_OP_ALWAYS           : return "ALWAYS";
        default                          : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrComponentSwizzle(ComponentSwizzle swz)
{
    switch (swz)
    {
        case COMPONENT_SWIZZLE_NONE     : return "NONE";
        case COMPONENT_SWIZZLE_IDENTITY : return "IDENTITY";
        case COMPONENT_SWIZZLE_ZERO     : return "ZERO";
        case COMPONENT_SWIZZLE_ONE      : return "ONE";
        case COMPONENT_SWIZZLE_R        : return "R";
        case COMPONENT_SWIZZLE_G        : return "G";
        case COMPONENT_SWIZZLE_B        : return "B";
        case COMPONENT_SWIZZLE_A        : return "A";
        default                         : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrBorderColor(BorderColor bc)
{
    switch (bc)
    {
        case BORDER_COLOR_TRANSPARENT_BLACK     : return "TRANSPARENT_BLACK";
        case BORDER_COLOR_OPAQUE_BLACK          : return "OPAQUE_BLACK";
        case BORDER_COLOR_OPAQUE_WHITE          : return "OPAQUE_WHITE";
        case BORDER_COLOR_FROM_IMAGE_DESCRIPTOR : return "FROM_IMAGE_DESCRIPTOR";
        default                                 : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrImageType(ImageType type)
{
    switch (type)
    {
        case IMAGE_TYPE_1D         : return "1D";
        case IMAGE_TYPE_2D         : return "2D";
        case IMAGE_TYPE_3D         : return "3D";
        case IMAGE_TYPE_CUBE       : return "CUBE";
        case IMAGE_TYPE_1D_ARRAY   : return "1D_ARRAY";
        case IMAGE_TYPE_2D_ARRAY   : return "2D_ARRAY";
        case IMAGE_TYPE_CUBE_ARRAY : return "CUBE_ARRAY";
        default                    : return "UNDEFINED";
    }
}

const char *TBOX::TBOXEmu::toStrImageFormat(ImageFormat fmt)
{
    switch (fmt)
    {
        case FORMAT_R8_UNORM                 : return "R8_UNORM";
        case FORMAT_R8G8_UNORM               : return "R8G8_UNORM";
        case FORMAT_R8G8B8A8_UNORM           : return "R8G8B8A8_UNORM";
        case FORMAT_R8G8B8A8_SRGB            : return "R8G8B8A8_SRGB";
        case FORMAT_R8G8B8A8_SNORM           : return "R8G8B8A8_SNORM";
        case FORMAT_B8G8R8A8_UNORM           : return "B8G8R8A8_UNORM";
        case FORMAT_B8G8R8A8_SRGB            : return "B8G8R8A8_SRGB";
        case FORMAT_R16_UNORM                : return "R16_UNORM";
        case FORMAT_R16G16_SFLOAT            : return "R16G16_SFLOAT";
        case FORMAT_R32_SFLOAT               : return "R32_SFLOAT";
        case FORMAT_D24_UNORM_S8_UINT        : return "D24_UNORM_S8_UINT";
        case FORMAT_R16G16B16A16_SFLOAT      : return "R16G16B16A16_SFLOAT";
        case FORMAT_B10G11R11_UFLOAT_PACK32  : return "FORMAT_B10G11R11_UFLOAT_PACK32";
        case FORMAT_A2B10G10R10_UNORM_PACK32 : return "FORMAT_A2B10G10R10_UNORM_PACK32";
        case FORMAT_BC1_RGB_UNORM_BLOCK      : return "BC1_RGB_UNORM";
        case FORMAT_BC1_RGB_SRGB_BLOCK       : return "BC1_RGB_SRGB";
        case FORMAT_BC1_RGBA_UNORM_BLOCK     : return "BC1_RGBA_UNORM";
        case FORMAT_BC1_RGBA_SRGB_BLOCK      : return "BC1_RGBA_SRGB";
        case FORMAT_BC2_UNORM_BLOCK          : return "BC2_UNORM";
        case FORMAT_BC2_SRGB_BLOCK           : return "BC2_SRGB";
        case FORMAT_BC3_UNORM_BLOCK          : return "BC3_UNORM";
        case FORMAT_BC3_SRGB_BLOCK           : return "BC3_SRGB";
        case FORMAT_BC4_UNORM_BLOCK          : return "BC4_UNORM";
        case FORMAT_BC4_SNORM_BLOCK          : return "BC4_SNORM";
        case FORMAT_BC5_UNORM_BLOCK          : return "BC5_UNORM";
        case FORMAT_BC5_SNORM_BLOCK          : return "BC5_SNORM";
        case FORMAT_BC6H_UFLOAT_BLOCK        : return "BC6H_UFLOAT";
        case FORMAT_BC6H_SFLOAT_BLOCK        : return "BC6H_SFLOAT";
        case FORMAT_BC7_UNORM_BLOCK          : return "BC7_UNORM";
        case FORMAT_BC7_SRGB_BLOCK           : return "BC7_SRGB";
        default                              : return "UNDEFINED";
    }
}

void TBOX::TBOXEmu::print_sample_request(SampleRequest req)
{
    LOG(DEBUG, "Operation = %s | Image ID = %d | Border Color ID = %d | Delta I = %ld | Delta J = %ld | Delta K = %ld | ",
               toStrSampleOperation((SampleOperation)req.info.operation), req.info.imageid, req.info.borderid,
               req.info.ioffset, req.info.joffset, req.info.koffset);
    LOG(DEBUG, "Min = %s | Mag = %s | Mip = %s | Aniso = %s | ", toStrFilterType((FilterType)req.info.minfilter),
               toStrFilterType((FilterType)req.info.magfilter), toStrFilterType((FilterType)req.info.mipfilter),
               (req.info.aniso ? "Yes" : "No"));
    LOG(DEBUG, "Address Mode = %s %s %s | ", toStrAddressMode((AddressMode)req.info.addrmodeu),
               toStrAddressMode((AddressMode)req.info.addrmodev),
               toStrAddressMode((AddressMode)req.info.addrmodew));
    LOG(DEBUG, "Border Color = %s | ", toStrBorderColor((BorderColor)req.info.border));
    LOG(DEBUG, "Swizzle = %s %s %s %s | ", toStrComponentSwizzle((ComponentSwizzle)req.info.swizzler),
               toStrComponentSwizzle((ComponentSwizzle)req.info.swizzleg),
               toStrComponentSwizzle((ComponentSwizzle)req.info.swizzleb),
               toStrComponentSwizzle((ComponentSwizzle)req.info.swizzlea));
    switch(req.info.operation)
    {
        case SAMPLE_OP_GATHER4:
        case SAMPLE_OP_GATHER4_PO:
            LOG(DEBUG, "Component Selection = %d | ", (req.info.component & 0x3));
            break;
        case SAMPLE_OP_SAMPLE_C:
        case SAMPLE_OP_SAMPLE_C_L:
        case SAMPLE_OP_GATHER4_C:
            LOG(DEBUG, "Compare Operation = %s | ", toStrCompareOperation((CompareOperation)req.info.compop));
            break;
    }
    switch (req.info.operation)
    {
        case SAMPLE_OP_SAMPLE_L:
        case SAMPLE_OP_SAMPLE_C_L:
        case SAMPLE_OP_LD:
            LOG(DEBUG, "LODs = %f %f %f %f  %f %f %f %f",
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[0][0]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[0][1]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[0][2]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[0][3]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[1][0]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[1][1]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[1][2]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lod_array[1][3]))));
            break;
        case SAMPLE_OP_SAMPLE:
        case SAMPLE_OP_SAMPLE_C:
            for (uint32_t quad = 0; quad < 2; quad++)
                LOG(DEBUG, "Quad %d | LOD = %f | Aniso Ratio = %f | Aniso Delta S = %f | Aniso Delta T = %f",
                    quad,
                    fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lodanisoq.lod[quad]))),
                    fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lodanisoq.anisoratio[quad]))),
                    fpu::FLT(fpu::sn8_to_f32(req.info.lodaniso.lodanisoq.anisodeltas[quad])),
                    fpu::FLT(fpu::sn8_to_f32(req.info.lodaniso.lodanisoq.anisodeltat[quad])));
            break;
        case SAMPLE_OP_GATHER4:
        case SAMPLE_OP_GATHER4_PO:
        case SAMPLE_OP_GATHER4_C:
        case SAMPLE_OP_GATHER4_PO_C:
            LOG(DEBUG, "Quad 0 LOD = %f | Quad 1 LOD = %f",
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lodanisoq.lod[0]))),
                fpu::FLT(fpu::f16_to_f32(fpu::F16(req.info.lodaniso.lodanisoq.lod[1]))));
            break;
    }
 
}

void TBOX::TBOXEmu::print_image_info(ImageInfo in)
{
    LOG(DEBUG, "Addr = %016lx | Type = %s | Format = %s (%3ld) | Width = %ld | Height = %ld | Depth = %ld | "
               "Tiled = %ld | ",
               in.info.address, toStrImageType((ImageType)in.info.type),
               toStrImageFormat((ImageFormat)in.info.format), in.info.format, in.info.width, in.info.height,
               in.info.depth, in.info.tiled);
    LOG(DEBUG, "Array Base = %ld | Array Count = %ld | Base Mip = %ld | Mip Count = %ld | ", in.info.arraybase,
               in.info.arraycount, in.info.basemip, in.info.mipcount);
    LOG(DEBUG, "Swizzle = %s %s %s %s | ",
               toStrComponentSwizzle((ComponentSwizzle)in.info.swizzler),
               toStrComponentSwizzle((ComponentSwizzle)in.info.swizzleg),
               toStrComponentSwizzle((ComponentSwizzle)in.info.swizzleb),
               toStrComponentSwizzle((ComponentSwizzle)in.info.swizzlea));
    LOG(DEBUG, " Row Pitch = %ld | Mip Pitch L0 = %ld | Mip Pitch L1 = %ld | Element Pitch = %ld | ",
               in.info.rowpitch, in.info.mippitchl0, in.info.mippitchl1, in.info.elementpitch);
    LOG(DEBUG, " Tiled = %ld | Packed Layout = %ld | First Packed Mip = %ld | First Packed Mip Level = %ld | ",
               in.info.tiled, in.info.packedlayout, in.info.packedmip, in.info.packedlevel);
    LOG(DEBUG, " Mip Scale by 8 = %ld | Mip Scale by 4 = %ld",
               in.info.mipscale8, in.info.mipscale4);
}

//  Decode BC1.
void TBOX::TBOXEmu::decode_BC1(uint8_t *inBuffer, uint8_t *outBuffer, bool hasAlpha)
{
    uint32_t color0, color1;
    float RGBA0[4], RGBA1[4];
    float decodedColor[4];
    uint32_t code;
    uint32_t colorbits;

    //  Convert first reference color of the compressed block to RGBA.
    color0 = (inBuffer[1] << 8) + inBuffer[0];
    RGBA0[0] = ((float)(color0 >> 11)) * (1.0f / 31.0f);
    RGBA0[1] = ((float)((color0 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA0[2] = ((float)(color0 & 0x1f)) * (1.0f / 31.0f);

    //  Convert second reference color of the compressed block to RGBA.
    color1 = (inBuffer[3] << 8) + inBuffer[2];
    RGBA1[0] = ((float)(color1 >> 11)) * (1.0f / 31.0f);
    RGBA1[1] = ((float)((color1 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA1[2] = ((float)(color1 & 0x1f)) * (1.0f / 31.0f);

    //  Get the code bits for the color components in the block.
    colorbits
        = inBuffer[4] + uint32_t(inBuffer[5] << 8) + uint32_t(inBuffer[6] << 16) + uint32_t(inBuffer[7] << 24);

    //  Generate the decoded colors for all the texels in the block.
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            //  Get code for the texel.
            code = colorbits & 0x03;
            colorbits = colorbits >> 2;

            //  Determine if transparent or non-transparent encoding must be used.
            if (color0 > color1)
            {
                //  Use non transparent encoding.
                decode2BitRGB(code, RGBA0, RGBA1, decodedColor);

                //  Non transparent alpha.
                decodedColor[3] = 1.0f;
            }
            else
            {
                //  Use transparent encoding.
                decode2BitRGBTransparent(code, RGBA0, RGBA1, decodedColor);

                //  Patch special non transparent case.
                if (code != 0x03 || !hasAlpha)
                    decodedColor[3] = 1.0f;
                else
                    decodedColor[3] = 0.0f;
            }

            ((uint32_t *)outBuffer)[j * 4 + i] = convertTo_R8G8B8A8_UNORM(decodedColor);
        }
    }
}

//  Decode BC2.
void TBOX::TBOXEmu::decode_BC2(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint32_t color0, color1;
    float RGBA0[4], RGBA1[4];
    float decodedColor[4];
    uint32_t code;
    uint32_t colorbits;
    uint64_t alphabits;

    //  Convert first reference color of the compressed block to RGBA.
    color0 = (inBuffer[9] << 8) + inBuffer[8];
    RGBA0[0] = ((float)(color0 >> 11)) * (1.0f / 31.0f);
    RGBA0[1] = ((float)((color0 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA0[2] = ((float)(color0 & 0x1f)) * (1.0f / 31.0f);

    //  Convert second reference color of the compressed block to RGBA.
    color1 = (inBuffer[11] << 8) + inBuffer[10];
    RGBA1[0] = ((float)(color1 >> 11)) * (1.0f / 31.0f);
    RGBA1[1] = ((float)((color1 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA1[2] = ((float)(color1 & 0x1f)) * (1.0f / 31.0f);

    //  Get the code bits for the color components in the block.
    colorbits = inBuffer[12] + (inBuffer[13] << 8) + (inBuffer[14] << 16) + (inBuffer[15] << 24);

    //  Get the data bits for the alpha components in the block.
    alphabits = uint64_t(inBuffer[0]) + (uint64_t(inBuffer[1]) << 8) + (uint64_t(inBuffer[2]) << 16)
                + (uint64_t(inBuffer[3]) << 24) + (uint64_t(inBuffer[4]) << 32) + (uint64_t(inBuffer[5]) << 40)
                + (uint64_t(inBuffer[6]) << 48) + (uint64_t(inBuffer[7]) << 56);

    //  Generate the decoded colors for all the texels in the block.
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            //  Get code for the texel.
            code = colorbits & 0x03;
            colorbits = colorbits >> 2;

            //  Decode color using non-transparent encoding.  */
            decode2BitRGB(code, RGBA0, RGBA1, decodedColor);

            //  Decode alpha.
            decodedColor[3] = float(alphabits & 0x0f) * (1.0f / 15.0f);
            alphabits = alphabits >> 4;

            // Convert to R8G8B8A8_UNORM.
            ((uint32_t *)outBuffer)[j * 4 + i] = convertTo_R8G8B8A8_UNORM(decodedColor);
        }
    }
}

// Decode BC3_UNORM
void TBOX::TBOXEmu::decode_BC3(uint8_t *inBuffer, uint8_t *outBuffer)
{
    uint32_t color0, color1;
    float RGBA0[4], RGBA1[4];
    float alpha0, alpha1;
    float decodedColor[4];
    uint32_t code;
    uint32_t alphacode;
    uint64_t alphabits;
    uint32_t colorbits;

    //  Convert first reference color of the compressed block to RGBA.
    color0 = (inBuffer[9] << 8) + inBuffer[8];
    RGBA0[0] = ((float)(color0 >> 11)) * (1.0f / 31.0f);
    RGBA0[1] = ((float)((color0 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA0[2] = ((float)(color0 & 0x1f)) * (1.0f / 31.0f);
    // RGBA0[3] = 1.0f;

    //  Convert second reference color of the compressed block to RGBA.
    color1 = (inBuffer[11] << 8) + inBuffer[10];
    RGBA1[0] = ((float)(color1 >> 11)) * (1.0f / 31.0f);
    RGBA1[1] = ((float)((color1 >> 5) & 0x3f)) * (1.0f / 63.0f);
    RGBA1[2] = ((float)(color1 & 0x1f)) * (1.0f / 31.0f);
    // RGBA1[3] = 1.0f;

    // Get the code bits for the color components in the block.
    colorbits = inBuffer[12] + (inBuffer[13] << 8) + (inBuffer[14] << 16) + (inBuffer[15] << 24);

    //  Convert first reference alpha from the compressed block.
    alpha0 = float(inBuffer[0]) * (1.0f / 255.0f);

    // Convert second reference alpha from the compressed block.
    alpha1 = float(inBuffer[1]) * (1.0f / 255.0f);

    //  Get the code bits for the alpha components in the block.
    alphabits = ((uint64_t)inBuffer[2]) + (((uint64_t)inBuffer[3]) << 8) + (((uint64_t)inBuffer[4]) << 16)
                + (((uint64_t)inBuffer[5]) << 24) + (((uint64_t)inBuffer[6]) << 32)
                + (((uint64_t)inBuffer[7]) << 40);

    //  Generate the decoded colors for all the texels in the block.
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            //  Get code for the texel.  */
            code = colorbits & 0x03;
            colorbits = colorbits >> 2;

            //  Decode color using non-transparent encoding.
            decode2BitRGB(code, RGBA0, RGBA1, decodedColor);

            //  Get the three bit alpha code for the texel in the 4x4 block.
            alphacode = uint32_t(alphabits & 0x07);
            alphabits = alphabits >> 3;

            //  Decode alpha.
            decodedColor[3] = decode4BitComponent(alphacode, alpha0, alpha1, false);

            //  Convert to R8G8B8A8_UNORM.
            ((uint32_t *)outBuffer)[j * 4 + i] = convertTo_R8G8B8A8_UNORM(decodedColor);
        }
    }
}

// Decode BC4_UNORM
void TBOX::TBOXEmu::decode_BC4_UNORM(uint8_t *inBuffer, uint8_t *outBuffer) { decode_BC4(inBuffer, outBuffer, false); }

// Decode BC4_SNORM
void TBOX::TBOXEmu::decode_BC4_SNORM(uint8_t *inBuffer, uint8_t *outBuffer) { decode_BC4(inBuffer, outBuffer, true); }

// Decode BC4
void TBOX::TBOXEmu::decode_BC4(uint8_t *inBuffer, uint8_t *outBuffer, bool signedFormat)
{
    float red0, red1;
    float decodedColor[4];
    uint32_t redcode;
    uint64_t redbits;

    //  Convert first reference red color from the compressed block.
    red0 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[0])) : float(inBuffer[0]) * (1.0f / 255.0f);

    // Convert second reference red color from the compressed block.
    red1 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[1])) : float(inBuffer[1]) * (1.0f / 255.0f);

    //  Get the code bits for the red component in the block.
    redbits = ((uint64_t)inBuffer[2]) + (((uint64_t)inBuffer[3]) << 8) + (((uint64_t)inBuffer[4]) << 16)
              + (((uint64_t)inBuffer[5]) << 24) + (((uint64_t)inBuffer[6]) << 32) + (((uint64_t)inBuffer[7]) << 40);

    //  Generate the decoded colors for all the texels in the block.
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            //  Get the three bit color code for the texel in the 4x4 block.
            redcode = uint32_t(redbits & 0x07);
            redbits = redbits >> 3;

            //  Decode red component.
            decodedColor[0] = decode4BitComponent(redcode, red0, red1, signedFormat);
            decodedColor[1] = 0.0;
            decodedColor[2] = 0.0;
            decodedColor[3] = 1.0;

            //  Convert to R8G8B8A8_UNORM.
            ((uint32_t *)outBuffer)[j * 4 + i] = signedFormat ? convertTo_R8G8B8A8_SNORM(decodedColor)
                                                            : convertTo_R8G8B8A8_UNORM(decodedColor);
        }
    }
}

// Decode BC5_UNORM
void TBOX::TBOXEmu::decode_BC5_UNORM(uint8_t *inBuffer, uint8_t *outBuffer) { decode_BC5(inBuffer, outBuffer, false); }

// Decode BC5_SNORM
void TBOX::TBOXEmu::decode_BC5_SNORM(uint8_t *inBuffer, uint8_t *outBuffer) { decode_BC5(inBuffer, outBuffer, true); }

// Decode BC5
void TBOX::TBOXEmu::decode_BC5(uint8_t *inBuffer, uint8_t *outBuffer, bool signedFormat)
{
    float red0, red1, green0, green1;
    float decodedColor[4];
    uint32_t redcode, greencode;
    uint64_t redbits, greenbits;

    //  Convert first reference red color from the compressed block.
    red0 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[0])) : float(inBuffer[0]) * (1.0f / 255.0f);

    // Convert second reference red color from the compressed block.
    red1 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[1])) : float(inBuffer[1]) * (1.0f / 255.0f);

    //  Get the code bits for the red component in the block.
    redbits = ((uint64_t)inBuffer[2]) + (((uint64_t)inBuffer[3]) << 8) + (((uint64_t)inBuffer[4]) << 16)
              + (((uint64_t)inBuffer[5]) << 24) + (((uint64_t)inBuffer[6]) << 32) + (((uint64_t)inBuffer[7]) << 40);

    //  Convert first reference green color from the compressed block.
    green0 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[8])) : float(inBuffer[8]) * (1.0f / 255.0f);

    // Convert second reference green color from the compressed block.
    green1 = signedFormat ? fpu::FLT(fpu::sn8_to_f32(inBuffer[9])) : float(inBuffer[9]) * (1.0f / 255.0f);

    //  Get the code bits for the red component in the block.
    greenbits = ((uint64_t)inBuffer[10]) + (((uint64_t)inBuffer[11]) << 8) + (((uint64_t)inBuffer[12]) << 16)
                + (((uint64_t)inBuffer[13]) << 24) + (((uint64_t)inBuffer[14]) << 32)
                + (((uint64_t)inBuffer[15]) << 40);

    //  Generate the decoded colors for all the texels in the block.
    for (int j = 0; j < 4; j++)
    {
        for (int i = 0; i < 4; i++)
        {
            //  Get the three bit color code for the texel in the 4x4 block.
            redcode = uint32_t(redbits & 0x07);
            redbits = redbits >> 3;

            //  Get the three bit color code for the texel in the 4x4 block.
            greencode = uint32_t(greenbits & 0x07);
            greenbits = greenbits >> 3;

            //  Decode red component.
            decodedColor[0] = decode4BitComponent(redcode, red0, red1, signedFormat);
            decodedColor[1] = decode4BitComponent(greencode, green0, green1, signedFormat);
            decodedColor[2] = 0.0;
            decodedColor[3] = 1.0;

            //  Convert to R8G8B8A8_UNORM.
            ((uint32_t *)outBuffer)[j * 4 + i] = signedFormat ? convertTo_R8G8B8A8_SNORM(decodedColor)
                                                            : convertTo_R8G8B8A8_UNORM(decodedColor);
        }
    }
}

//  Decodes and selects the proper color using BC3/BC4/BC5 4-bit color encoding.
float TBOX::TBOXEmu::decode4BitComponent(uint32_t code, float color0, float color1, bool signedFormat)
{
    float output = 0.0f;

    //  Select between the two methods of encoding.
    if (color0 > color1)
    {
        //  Select encoding.
        switch (code)
        {
            case 0x00:

                //  Use color0.
                output = color0;

                break;

            case 0x01:

                //  Use color1.
                output = color1;

                break;

            case 0x02:

                //  Use (6 * color0 + 1 * color1) / 7.
                output = (6.0f * color0 + 1.0f * color1) / 7.0f;
                break;

            case 0x03:

                //  Use (5 * color0 + 2 * color1) / 7.
                output = (5.0f * color0 + 2.0f * color1) / 7.0f;
                break;

            case 0x04:

                //  Use (4 * color0 + 3 * color1) / 7.
                output = (4.0f * color0 + 3.0f * color1) / 7.0f;
                break;

            case 0x05:

                //  Use (3 * color0 + 4 * color1) / 7.
                output = (3.0f * color0 + 4.0f * color1) / 7.0f;
                break;

            case 0x06:

                //  Use (2 * color0 + 5 * color1) / 7.
                output = (2.0f * color0 + 5.0f * color1) / 7.0f;
                break;

            case 0x07:

                //  Use (1 * color0 + 6 * color1) / 7.
                output = (1.0f * color0 + 6.0f * color1) / 7.0f;
                break;

            default:
                break;
        }
    }
    else
    {
        //  Select encoding.
        switch (code)
        {
            case 0x00:

                //  Use color0.
                output = color0;

                break;

            case 0x01:

                //  Use color1.
                output = color1;

                break;

            case 0x02:

                //  Use (4 * color0 + 1 * color1) / 5.
                output = (4.0f * color0 + 1.0f * color1) / 5.0f;
                break;

            case 0x03:

                //  Use (3 * color0 + 2 * color1) / 5.
                output = (3.0f * color0 + 2.0f * color1) / 5.0f;
                break;

            case 0x04:

                //  Use (2 * color0 + 3 * color1) / 5.
                output = (2.0f * color0 + 3.0f * color1) / 5.0f;
                break;

            case 0x05:

                //  Use (1 * color0 + 4 * color1) / 5.
                output = (1.0f * color0 + 4.0f * color1) / 5.0f;
                break;

            case 0x06:

                //  Use minimum value.
                output = signedFormat ? -1.0f : 0.0f;
                break;

            case 0x07:

                //  Use maximum value.
                output = 1.0f;
                break;

            default:
                break;
        }
    }

    return output;
}

//  Decodes and selects the proper RGB color for BC1/BC2/BC3 2-bit color encoding without transparent color.
void TBOX::TBOXEmu::decode2BitRGB(uint32_t code, float RGB0[], float RGB1[], float output[])
{
    //  Select color for the texel and store.
    switch (code)
    {
        case 0x00:

            //  Use RGB0.
            output[0] = RGB0[0];
            output[1] = RGB0[1];
            output[2] = RGB0[2];

            break;

        case 0x01:

            //  Use RGB1.
            output[0] = RGB1[0];
            output[1] = RGB1[1];
            output[2] = RGB1[2];

            break;

        case 0x02:

            //  Use (2 * RGB0 + RGB1) / 3.
            output[0] = (2 * RGB0[0] + RGB1[0]) / 3;
            output[1] = (2 * RGB0[1] + RGB1[1]) / 3;
            output[2] = (2 * RGB0[2] + RGB1[2]) / 3;

            break;

        case 0x03:

            //  Use (RGB0 + 2 * RGB1) / 3.
            output[0] = (RGB0[0] + 2 * RGB1[0]) / 3;
            output[1] = (RGB0[1] + 2 * RGB1[1]) / 3;
            output[2] = (RGB0[2] + 2 * RGB1[2]) / 3;

            break;

        default:
            break;
    }
}

//  Decodes and selects the proper RGB color for BC1/BC2/BC3 2-bit color encoding with transparent color.
void TBOX::TBOXEmu::decode2BitRGBTransparent(uint32_t code, float RGB0[], float RGB1[], float output[])
{
    //  Select color for the texel and store.
    switch (code)
    {
        case 0x00:

            //  Use RGB0.
            output[0] = RGB0[0];
            output[1] = RGB0[1];
            output[2] = RGB0[2];

            break;

        case 0x01:

            //  Use RGB1.
            output[0] = RGB1[0];
            output[1] = RGB1[1];
            output[2] = RGB1[2];

            break;

        case 0x02:

            //  Use (RGB0 + RGB1) / 2.
            output[0] = (RGB0[0] + RGB1[0]) * 0.5f;
            output[1] = (RGB0[1] + RGB1[1]) * 0.5f;
            output[2] = (RGB0[2] + RGB1[2]) * 0.5f;

            break;

        case 0x03:

            //  Use BLACK.
            output[0] = 0.0f;
            output[1] = 0.0f;
            output[2] = 0.0f;

            break;

        default:
            break;
    }
}

uint32_t TBOX::TBOXEmu::convertTo_R8G8B8A8_UNORM(float decodedColor[])
{
    uint8_t color[4];

    static const float eps = 1.f / float(1<<16);

    color[0] = fpu::f32_to_un8(fpu::F2F32(decodedColor[0] + eps));
    color[1] = fpu::f32_to_un8(fpu::F2F32(decodedColor[1] + eps));
    color[2] = fpu::f32_to_un8(fpu::F2F32(decodedColor[2] + eps));
    color[3] = fpu::f32_to_un8(fpu::F2F32(decodedColor[3] + eps));

    return (color[0] | (color[1] << 8) | (color[2] << 16) | (color[3] << 24));
}

uint32_t TBOX::TBOXEmu::convertTo_R8G8B8A8_SNORM(float decodedColor[])
{
    uint8_t color[4];

    color[0] = fpu::f32_to_sn8(fpu::F2F32(decodedColor[0]));
    color[1] = fpu::f32_to_sn8(fpu::F2F32(decodedColor[1]));
    color[2] = fpu::f32_to_sn8(fpu::F2F32(decodedColor[2]));
    color[3] = fpu::f32_to_sn8(fpu::F2F32(decodedColor[3]));

    return (color[0] | (color[1] << 8) | (color[2] << 16) | (color[3] << 24));
}

bool TBOX::TBOXEmu::filterSupported(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_R8_UNORM:
        case FORMAT_R8G8B8A8_UNORM:
        case FORMAT_R8G8B8A8_SRGB:
        case FORMAT_B8G8R8A8_UNORM:
        case FORMAT_B8G8R8A8_SRGB:
        case FORMAT_R16_UNORM:
        case FORMAT_R16_SFLOAT:
        case FORMAT_R16G16_UNORM:
        case FORMAT_R16G16_SNORM:
        case FORMAT_R16G16_SFLOAT:
        case FORMAT_R16G16B16A16_SFLOAT:
        case FORMAT_D24_UNORM_S8_UINT:
        case FORMAT_R32_SFLOAT:
        case FORMAT_B10G11R11_UFLOAT_PACK32:
        case FORMAT_A2B10G10R10_UNORM_PACK32:
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_UNORM_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
        case FORMAT_BC4_UNORM_BLOCK:
        case FORMAT_BC4_SNORM_BLOCK:
        case FORMAT_BC5_UNORM_BLOCK:
        case FORMAT_BC5_SNORM_BLOCK:
        case FORMAT_BC6H_UFLOAT_BLOCK:
        case FORMAT_BC6H_SFLOAT_BLOCK:
        case FORMAT_BC7_UNORM_BLOCK:
            return true;
        default:
            return false;
    }
}

bool TBOX::TBOXEmu::comparisonSupported(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_R16_SFLOAT:
        case FORMAT_D24_UNORM_S8_UINT:
        case FORMAT_R32_SFLOAT:
            return true;
        default:
            return false;
    }
}

bool TBOX::TBOXEmu::isCompressedFormat(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_UNORM_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
        case FORMAT_BC4_UNORM_BLOCK:
        case FORMAT_BC4_SNORM_BLOCK:
        case FORMAT_BC5_UNORM_BLOCK:
        case FORMAT_BC5_SNORM_BLOCK:
        case FORMAT_BC6H_UFLOAT_BLOCK:
        case FORMAT_BC6H_SFLOAT_BLOCK:
        case FORMAT_BC7_UNORM_BLOCK:
        case FORMAT_BC7_SRGB_BLOCK:
            return true;
        default:
            return false;
    }
}

TBOX::TBOXEmu::CompressedFormatInfo TBOX::TBOXEmu::getCompressedFormatInfo(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_UNORM_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC4_UNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC4_SNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_SNORM);
        case FORMAT_BC5_UNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC5_SNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_SNORM);
        case FORMAT_BC6H_UFLOAT_BLOCK:
        case FORMAT_BC6H_SFLOAT_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R32G32B32A32_SFLOAT);
        case FORMAT_BC7_UNORM_BLOCK:
        case FORMAT_BC7_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R32G32B32A32_SFLOAT);
        default:
            return CompressedFormatInfo();
    }
}

TBOX::TBOXEmu::CompressedFormatInfo TBOX::TBOXEmu::getCompressedFormatInfoL1(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_UNORM_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC4_UNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC4_SNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 8, FORMAT_R8G8B8A8_SNORM);
        case FORMAT_BC5_UNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_UNORM);
        case FORMAT_BC5_SNORM_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_SNORM);
        case FORMAT_BC6H_UFLOAT_BLOCK:
        case FORMAT_BC6H_SFLOAT_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R16G16B16A16_SFLOAT);
        case FORMAT_BC7_UNORM_BLOCK:
        case FORMAT_BC7_SRGB_BLOCK:
            return CompressedFormatInfo(2, 2, 16, FORMAT_R8G8B8A8_UNORM);
        default:
            return CompressedFormatInfo();
    }
}

bool TBOX::TBOXEmu::isSRGBFormat(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_R8G8B8A8_SRGB:
        case FORMAT_B8G8R8A8_SRGB:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
        case FORMAT_BC7_SRGB_BLOCK:
            return true;
        default:
            return false;
    }
}

bool TBOX::TBOXEmu::isFloat32Format(ImageFormat format)
{
    switch (format)
    {
        case FORMAT_R16_UNORM:
        case FORMAT_R16_SNORM:
        case FORMAT_R16G16_UNORM:
        case FORMAT_R16G16_SNORM:
        case FORMAT_R16G16B16_UNORM:
        case FORMAT_R16G16B16_SNORM:
        case FORMAT_R16G16B16A16_UNORM:
        case FORMAT_R16G16B16A16_SNORM:
        case FORMAT_R32_SFLOAT:
        case FORMAT_R32G32_SFLOAT:
        case FORMAT_R32G32B32_SFLOAT:
        case FORMAT_R32G32B32A32_SFLOAT:
        case FORMAT_X8_D24_UNORM_PACK32:
        case FORMAT_D32_SFLOAT:
        case FORMAT_D16_UNORM_S8_UINT:
        case FORMAT_D24_UNORM_S8_UINT:
        case FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default :
            return false;
    }
}


void TBOX::TBOXEmu::getCompressedTexel(ImageFormat format, uint32_t comprBlockI, uint32_t comprBlockJ, uint8_t data[])
{
    uint8_t compressedData[16];
    uint8_t decompressedData[16 * 4];

    for (int b = 0; b < 16; b++)
        compressedData[b] = data[b];

    switch (format)
    {
        case FORMAT_BC1_RGB_UNORM_BLOCK:
        case FORMAT_BC1_RGB_SRGB_BLOCK:
            {
                bool hasAlpha = false;
                decode_BC1(compressedData, decompressedData, hasAlpha);
                ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
                break;
            }
        case FORMAT_BC1_RGBA_UNORM_BLOCK:
        case FORMAT_BC1_RGBA_SRGB_BLOCK:
            {
                bool hasAlpha = true;
                decode_BC1(compressedData, decompressedData, hasAlpha);
                ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
                break;
            }
        case FORMAT_BC2_UNORM_BLOCK:
        case FORMAT_BC2_SRGB_BLOCK:
            decode_BC2(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC3_UNORM_BLOCK:
        case FORMAT_BC3_SRGB_BLOCK:
            decode_BC3(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC4_UNORM_BLOCK:
            decode_BC4_UNORM(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC4_SNORM_BLOCK:
            decode_BC4_SNORM(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC5_UNORM_BLOCK:
            decode_BC5_UNORM(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC5_SNORM_BLOCK:
            decode_BC5_SNORM(compressedData, decompressedData);
            ((uint32_t *)data)[0] = ((uint32_t *)decompressedData)[comprBlockJ * 4 + comprBlockI];
            break;
        case FORMAT_BC6H_UFLOAT_BLOCK:
            fetch_bptc_rgb_unsigned_float(compressedData, 0, comprBlockI, comprBlockJ, (float *)data);
            break;
        case FORMAT_BC6H_SFLOAT_BLOCK:
            fetch_bptc_rgb_signed_float(compressedData, 0, comprBlockI, comprBlockJ, (float *)data);
            break;
        case FORMAT_BC7_UNORM_BLOCK:
            fetch_bptc_rgba_unorm(compressedData, 0, comprBlockI, comprBlockJ, (float *)data);
            break;
        case FORMAT_BC7_SRGB_BLOCK:
            fetch_bptc_srgb_alpha_unorm(compressedData, 0, comprBlockI, comprBlockJ, (float *)data);
            break;
        default:
            ((uint32_t *)data)[0] = 0;
            break;
    }
}

/*
 * Copyright (C) 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file texcompress_bptc.c
 * GL_ARB_texture_compression_bptc support.
 */

const TBOX::TBOXEmu::bptc_unorm_mode TBOX::TBOXEmu::bptc_unorm_modes[] = {
    /* 0 */ {3, 4, false, false, 4, 0, true, false, 3, 0},
    /* 1 */ {2, 6, false, false, 6, 0, false, true, 3, 0},
    /* 2 */ {3, 6, false, false, 5, 0, false, false, 2, 0},
    /* 3 */ {2, 6, false, false, 7, 0, true, false, 2, 0},
    /* 4 */ {1, 0, true, true, 5, 6, false, false, 2, 3},
    /* 5 */ {1, 0, true, false, 7, 8, false, false, 2, 2},
    /* 6 */ {1, 0, false, false, 7, 7, true, false, 4, 0},
    /* 7 */ {2, 6, false, false, 5, 5, true, false, 2, 0}};

const TBOX::TBOXEmu::bptc_float_mode TBOX::TBOXEmu::bptc_float_modes[] = {
    /* 00 */
    {false,
     true,
     5,
     10,
     3,
     {5, 5, 5},
     {{2, 1, 4, 1, false},  {2, 2, 4, 1, false},  {3, 2, 4, 1, false}, {0, 0, 0, 10, false},
      {0, 1, 0, 10, false}, {0, 2, 0, 10, false}, {1, 0, 0, 5, false}, {3, 1, 4, 1, false},
      {2, 1, 0, 4, false},  {1, 1, 0, 5, false},  {3, 2, 0, 1, false}, {3, 1, 0, 4, false},
      {1, 2, 0, 5, false},  {3, 2, 1, 1, false},  {2, 2, 0, 4, false}, {2, 0, 0, 5, false},
      {3, 2, 2, 1, false},  {3, 0, 0, 5, false},  {3, 2, 3, 1, false}, {-1, 0, 0, 0, false}}},
    /* 01 */
    {false, true, 5, 7, 3, {6, 6, 6}, {{2, 1, 5, 1, false}, {3, 1, 4, 1, false}, {3, 1, 5, 1, false},
                                       {0, 0, 0, 7, false}, {3, 2, 0, 1, false}, {3, 2, 1, 1, false},
                                       {2, 2, 4, 1, false}, {0, 1, 0, 7, false}, {2, 2, 5, 1, false},
                                       {3, 2, 2, 1, false}, {2, 1, 4, 1, false}, {0, 2, 0, 7, false},
                                       {3, 2, 3, 1, false}, {3, 2, 5, 1, false}, {3, 2, 4, 1, false},
                                       {1, 0, 0, 6, false}, {2, 1, 0, 4, false}, {1, 1, 0, 6, false},
                                       {3, 1, 0, 4, false}, {1, 2, 0, 6, false}, {2, 2, 0, 4, false},
                                       {2, 0, 0, 6, false}, {3, 0, 0, 6, false}, {-1, 0, 0, 0, false}}},
    /* 00010 */
    {false,
     true,
     5,
     11,
     3,
     {5, 4, 4},
     {{0, 0, 0, 10, false},
      {0, 1, 0, 10, false},
      {0, 2, 0, 10, false},
      {1, 0, 0, 5, false},
      {0, 0, 10, 1, false},
      {2, 1, 0, 4, false},
      {1, 1, 0, 4, false},
      {0, 1, 10, 1, false},
      {3, 2, 0, 1, false},
      {3, 1, 0, 4, false},
      {1, 2, 0, 4, false},
      {0, 2, 10, 1, false},
      {3, 2, 1, 1, false},
      {2, 2, 0, 4, false},
      {2, 0, 0, 5, false},
      {3, 2, 2, 1, false},
      {3, 0, 0, 5, false},
      {3, 2, 3, 1, false},
      {-1, 0, 0, 0, false}}},
    /* 00011 */
    {false,
     false,
     0,
     10,
     4,
     {10, 10, 10},
     {{0, 0, 0, 10, false},
      {0, 1, 0, 10, false},
      {0, 2, 0, 10, false},
      {1, 0, 0, 10, false},
      {1, 1, 0, 10, false},
      {1, 2, 0, 10, false},
      {-1, 0, 0, 0, false}}},
    /* 00110 */
    {false, true, 5, 11, 3, {4, 5, 4}, {{0, 0, 0, 10, false}, {0, 1, 0, 10, false}, {0, 2, 0, 10, false},
                                        {1, 0, 0, 4, false},  {0, 0, 10, 1, false}, {3, 1, 4, 1, false},
                                        {2, 1, 0, 4, false},  {1, 1, 0, 5, false},  {0, 1, 10, 1, false},
                                        {3, 1, 0, 4, false},  {1, 2, 0, 4, false},  {0, 2, 10, 1, false},
                                        {3, 2, 1, 1, false},  {2, 2, 0, 4, false},  {2, 0, 0, 4, false},
                                        {3, 2, 0, 1, false},  {3, 2, 2, 1, false},  {3, 0, 0, 4, false},
                                        {2, 1, 4, 1, false},  {3, 2, 3, 1, false},  {-1, 0, 0, 0, false}}},
    /* 00111 */
    {false,
     true,
     0,
     11,
     4,
     {9, 9, 9},
     {{0, 0, 0, 10, false},
      {0, 1, 0, 10, false},
      {0, 2, 0, 10, false},
      {1, 0, 0, 9, false},
      {0, 0, 10, 1, false},
      {1, 1, 0, 9, false},
      {0, 1, 10, 1, false},
      {1, 2, 0, 9, false},
      {0, 2, 10, 1, false},
      {-1, 0, 0, 0, false}}},
    /* 01010 */
    {false, true, 5, 11, 3, {4, 4, 5}, {{0, 0, 0, 10, false}, {0, 1, 0, 10, false}, {0, 2, 0, 10, false},
                                        {1, 0, 0, 4, false},  {0, 0, 10, 1, false}, {2, 2, 4, 1, false},
                                        {2, 1, 0, 4, false},  {1, 1, 0, 4, false},  {0, 1, 10, 1, false},
                                        {3, 2, 0, 1, false},  {3, 1, 0, 4, false},  {1, 2, 0, 5, false},
                                        {0, 2, 10, 1, false}, {2, 2, 0, 4, false},  {2, 0, 0, 4, false},
                                        {3, 2, 1, 1, false},  {3, 2, 2, 1, false},  {3, 0, 0, 4, false},
                                        {3, 2, 4, 1, false},  {3, 2, 3, 1, false},  {-1, 0, 0, 0, false}}},
    /* 01011 */
    {false,
     true,
     0,
     12,
     4,
     {8, 8, 8},
     {{0, 0, 0, 10, false},
      {0, 1, 0, 10, false},
      {0, 2, 0, 10, false},
      {1, 0, 0, 8, false},
      {0, 0, 10, 2, true},
      {1, 1, 0, 8, false},
      {0, 1, 10, 2, true},
      {1, 2, 0, 8, false},
      {0, 2, 10, 2, true},
      {-1, 0, 0, 0, false}}},
    /* 01110 */
    {false,
     true,
     5,
     9,
     3,
     {5, 5, 5},
     {{0, 0, 0, 9, false}, {2, 2, 4, 1, false}, {0, 1, 0, 9, false}, {2, 1, 4, 1, false}, {0, 2, 0, 9, false},
      {3, 2, 4, 1, false}, {1, 0, 0, 5, false}, {3, 1, 4, 1, false}, {2, 1, 0, 4, false}, {1, 1, 0, 5, false},
      {3, 2, 0, 1, false}, {3, 1, 0, 4, false}, {1, 2, 0, 5, false}, {3, 2, 1, 1, false}, {2, 2, 0, 4, false},
      {2, 0, 0, 5, false}, {3, 2, 2, 1, false}, {3, 0, 0, 5, false}, {3, 2, 3, 1, false}, {-1, 0, 0, 0, false}}},
    /* 01111 */
    {false,
     true,
     0,
     16,
     4,
     {4, 4, 4},
     {{0, 0, 0, 10, false},
      {0, 1, 0, 10, false},
      {0, 2, 0, 10, false},
      {1, 0, 0, 4, false},
      {0, 0, 10, 6, true},
      {1, 1, 0, 4, false},
      {0, 1, 10, 6, true},
      {1, 2, 0, 4, false},
      {0, 2, 10, 6, true},
      {-1, 0, 0, 0, false}}},
    /* 10010 */
    {false,
     true,
     5,
     8,
     3,
     {6, 5, 5},
     {{0, 0, 0, 8, false}, {3, 1, 4, 1, false}, {2, 2, 4, 1, false}, {0, 1, 0, 8, false}, {3, 2, 2, 1, false},
      {2, 1, 4, 1, false}, {0, 2, 0, 8, false}, {3, 2, 3, 1, false}, {3, 2, 4, 1, false}, {1, 0, 0, 6, false},
      {2, 1, 0, 4, false}, {1, 1, 0, 5, false}, {3, 2, 0, 1, false}, {3, 1, 0, 4, false}, {1, 2, 0, 5, false},
      {3, 2, 1, 1, false}, {2, 2, 0, 4, false}, {2, 0, 0, 6, false}, {3, 0, 0, 6, false}, {-1, 0, 0, 0, false}}},
    /* 10011 */
    {true /* reserved */, true, 0, 0, 0, {0, 0, 0}, {{0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {-1, 0, 0, 0, false}}},
    /* 10110 */
    {false, true, 5, 8, 3, {5, 6, 5}, {{0, 0, 0, 8, false}, {3, 2, 0, 1, false},
                                       {2, 2, 4, 1, false}, {0, 1, 0, 8, false},
                                       {2, 1, 5, 1, false}, {2, 1, 4, 1, false},
                                       {0, 2, 0, 8, false}, {3, 1, 5, 1, false},
                                       {3, 2, 4, 1, false}, {1, 0, 0, 5, false},
                                       {3, 1, 4, 1, false}, {2, 1, 0, 4, false},
                                       {1, 1, 0, 6, false}, {3, 1, 0, 4, false},
                                       {1, 2, 0, 5, false}, {3, 2, 1, 1, false},
                                       {2, 2, 0, 4, false}, {2, 0, 0, 5, false},
                                       {3, 2, 2, 1, false}, {3, 0, 0, 5, false},
                                       {3, 2, 3, 1, false}, {-1, 0, 0, 0, false}}},
    /* 10111 */
    {true /* reserved */, true, 0, 0, 0, {0, 0, 0}, {{0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {-1, 0, 0, 0, false}}},
     /* 11010 */
    {false, true, 5, 8, 3, {5, 5, 6}, {{0, 0, 0, 8, false}, {3, 2, 1, 1, false},
                                       {2, 2, 4, 1, false}, {0, 1, 0, 8, false},
                                       {2, 2, 5, 1, false}, {2, 1, 4, 1, false},
                                       {0, 2, 0, 8, false}, {3, 2, 5, 1, false},
                                       {3, 2, 4, 1, false}, {1, 0, 0, 5, false},
                                       {3, 1, 4, 1, false}, {2, 1, 0, 4, false},
                                       {1, 1, 0, 5, false}, {3, 2, 0, 1, false},
                                       {3, 1, 0, 4, false}, {1, 2, 0, 6, false},
                                       {2, 2, 0, 4, false}, {2, 0, 0, 5, false},
                                       {3, 2, 2, 1, false}, {3, 0, 0, 5, false},
                                       {3, 2, 3, 1, false}, {-1, 0, 0, 0, false}}},
    /* 11011 */
    {true /* reserved */, true, 0, 0, 0, {0, 0, 0}, {{0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {-1, 0, 0, 0, false}}},
     /* 11110 */
    {false, false, 5, 6, 3, {6, 6, 6}, {{0, 0, 0, 6, false}, {3, 1, 4, 1, false}, {3, 2, 0, 1, false},
                                        {3, 2, 1, 1, false}, {2, 2, 4, 1, false}, {0, 1, 0, 6, false},
                                        {2, 1, 5, 1, false}, {2, 2, 5, 1, false}, {3, 2, 2, 1, false},
                                        {2, 1, 4, 1, false}, {0, 2, 0, 6, false}, {3, 1, 5, 1, false},
                                        {3, 2, 3, 1, false}, {3, 2, 5, 1, false}, {3, 2, 4, 1, false},
                                        {1, 0, 0, 6, false}, {2, 1, 0, 4, false}, {1, 1, 0, 6, false},
                                        {3, 1, 0, 4, false}, {1, 2, 0, 6, false}, {2, 2, 0, 4, false},
                                        {2, 0, 0, 6, false}, {3, 0, 0, 6, false}, {-1, 0, 0, 0, false}}},
    /* 11111 */
    {true /* reserved */, true, 0, 0, 0, {0, 0, 0}, {{0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {0, 0, 0, 0, false},
                                                     {0, 0, 0, 0, false}, {-1, 0, 0, 0, false}}},
 };

/* This partition table is used when the mode has two subsets. Each
 * partition is represented by a 32-bit value which gives 2 bits per texel
 * within the block. The value of the two bits represents which subset to use
 * (0 or 1).
 */
const uint32_t TBOX::TBOXEmu::partition_table1[N_PARTITIONS] = {
    0x50505050U, 0x40404040U, 0x54545454U, 0x54505040U, 0x50404000U, 0x55545450U, 0x55545040U, 0x54504000U,
    0x50400000U, 0x55555450U, 0x55544000U, 0x54400000U, 0x55555440U, 0x55550000U, 0x55555500U, 0x55000000U,
    0x55150100U, 0x00004054U, 0x15010000U, 0x00405054U, 0x00004050U, 0x15050100U, 0x05010000U, 0x40505054U,
    0x00404050U, 0x05010100U, 0x14141414U, 0x05141450U, 0x01155440U, 0x00555500U, 0x15014054U, 0x05414150U,
    0x44444444U, 0x55005500U, 0x11441144U, 0x05055050U, 0x05500550U, 0x11114444U, 0x41144114U, 0x44111144U,
    0x15055054U, 0x01055040U, 0x05041050U, 0x05455150U, 0x14414114U, 0x50050550U, 0x41411414U, 0x00141400U,
    0x00041504U, 0x00105410U, 0x10541000U, 0x04150400U, 0x50410514U, 0x41051450U, 0x05415014U, 0x14054150U,
    0x41050514U, 0x41505014U, 0x40011554U, 0x54150140U, 0x50505500U, 0x00555050U, 0x15151010U, 0x54540404U,
};

/* This partition table is used when the mode has three subsets. In this case
 * the values can be 0, 1 or 2.
 */
const uint32_t TBOX::TBOXEmu::partition_table2[N_PARTITIONS] = {
    0xaa685050U, 0x6a5a5040U, 0x5a5a4200U, 0x5450a0a8U, 0xa5a50000U, 0xa0a05050U, 0x5555a0a0U, 0x5a5a5050U,
    0xaa550000U, 0xaa555500U, 0xaaaa5500U, 0x90909090U, 0x94949494U, 0xa4a4a4a4U, 0xa9a59450U, 0x2a0a4250U,
    0xa5945040U, 0x0a425054U, 0xa5a5a500U, 0x55a0a0a0U, 0xa8a85454U, 0x6a6a4040U, 0xa4a45000U, 0x1a1a0500U,
    0x0050a4a4U, 0xaaa59090U, 0x14696914U, 0x69691400U, 0xa08585a0U, 0xaa821414U, 0x50a4a450U, 0x6a5a0200U,
    0xa9a58000U, 0x5090a0a8U, 0xa8a09050U, 0x24242424U, 0x00aa5500U, 0x24924924U, 0x24499224U, 0x50a50a50U,
    0x500aa550U, 0xaaaa4444U, 0x66660000U, 0xa5a0a5a0U, 0x50a050a0U, 0x69286928U, 0x44aaaa44U, 0x66666600U,
    0xaa444444U, 0x54a854a8U, 0x95809580U, 0x96969600U, 0xa85454a8U, 0x80959580U, 0xaa141414U, 0x96960000U,
    0xaaaa1414U, 0xa05050a0U, 0xa0a5a5a0U, 0x96000000U, 0x40804080U, 0xa9a8a9a8U, 0xaaaaaa44U, 0x2a4a5254U};

const uint8_t TBOX::TBOXEmu::anchor_indices[][N_PARTITIONS] = {
    /* Anchor index values for the second subset of two-subset partitioning */
    {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf,
     0xf, 0x2, 0x8, 0x2, 0x2, 0x8, 0x8, 0xf, 0x2, 0x8, 0x2, 0x2, 0x8, 0x8, 0x2, 0x2,
     0xf, 0xf, 0x6, 0x8, 0x2, 0x8, 0xf, 0xf, 0x2, 0x8, 0x2, 0x2, 0x2, 0xf, 0xf, 0x6,
     0x6, 0x2, 0x6, 0x8, 0xf, 0xf, 0x2, 0x2, 0xf, 0xf, 0xf, 0xf, 0xf, 0x2, 0x2, 0xf},

    /* Anchor index values for the second subset of three-subset partitioning */
    {0x3, 0x3, 0xf, 0xf, 0x8, 0x3, 0xf, 0xf, 0x8, 0x8, 0x6, 0x6, 0x6, 0x5, 0x3, 0x3,
     0x3, 0x3, 0x8, 0xf, 0x3, 0x3, 0x6, 0xa, 0x5, 0x8, 0x8, 0x6, 0x8, 0x5, 0xf, 0xf,
     0x8, 0xf, 0x3, 0x5, 0x6, 0xa, 0x8, 0xf, 0xf, 0x3, 0xf, 0x5, 0xf, 0xf, 0xf, 0xf,
     0x3, 0xf, 0x5, 0x5, 0x5, 0x8, 0x5, 0xa, 0x5, 0xa, 0x8, 0xd, 0xf, 0xc, 0x3, 0x3},

    /* Anchor index values for the third subset of three-subset
     * partitioning
     */
    {0xf, 0x8, 0x8, 0x3, 0xf, 0xf, 0x3, 0x8, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x8,
     0xf, 0x8, 0xf, 0x3, 0xf, 0x8, 0xf, 0x8, 0x3, 0xf, 0x6, 0xa, 0xf, 0xf, 0xa, 0x8,
     0xf, 0x3, 0xf, 0xa, 0xa, 0x8, 0x9, 0xa, 0x6, 0xf, 0x8, 0xf, 0x3, 0x6, 0x6, 0x8,
     0xf, 0x3, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x3, 0xf, 0xf, 0x8}};

int TBOX::TBOXEmu::extract_bits(const uint8_t *block, int offset, int n_bits)
{
    int byte_index = offset / 8;
    int bit_index = offset % 8;
    int n_bits_in_byte = min(n_bits, 8 - bit_index);
    int result = 0;
    int bit = 0;

    while (true)
    {
        result |= ((block[byte_index] >> bit_index) & ((1 << n_bits_in_byte) - 1)) << bit;

        n_bits -= n_bits_in_byte;

        if (n_bits <= 0)
            return result;

        bit += n_bits_in_byte;
        byte_index++;
        bit_index = 0;
        n_bits_in_byte = min(n_bits, 8);
    }
}

uint8_t TBOX::TBOXEmu::expand_component(uint8_t byte, int n_bits)
{
    /* Expands a n-bit quantity into a byte by copying the most-significant
     * bits into the unused least-significant bits.
     */
    return byte << (8 - n_bits) | (byte >> (2 * n_bits - 8));
}

int TBOX::TBOXEmu::extract_unorm_endpoints(const bptc_unorm_mode *mode, const uint8_t *block, int bit_offset,
                                           uint8_t endpoints[][4])
{
    int component;
    int subset;
    int endpoint;
    int pbit;
    int n_components;

    /* Extract each color component */
    for (component = 0; component < 3; component++)
    {
        for (subset = 0; subset < mode->n_subsets; subset++)
        {
            for (endpoint = 0; endpoint < 2; endpoint++)
            {
                endpoints[subset * 2 + endpoint][component]
                    = extract_bits(block, bit_offset, mode->n_color_bits);
                bit_offset += mode->n_color_bits;
            }
        }
    }

    /* Extract the alpha values */
    if (mode->n_alpha_bits > 0)
    {
        for (subset = 0; subset < mode->n_subsets; subset++)
        {
            for (endpoint = 0; endpoint < 2; endpoint++)
            {
                endpoints[subset * 2 + endpoint][3] = extract_bits(block, bit_offset, mode->n_alpha_bits);
                bit_offset += mode->n_alpha_bits;
            }
        }

        n_components = 4;
    }
    else
    {
        for (subset = 0; subset < mode->n_subsets; subset++)
            for (endpoint = 0; endpoint < 2; endpoint++)
                endpoints[subset * 2 + endpoint][3] = 255;

        n_components = 3;
    }

    /* Add in the p-bits */
    if (mode->has_endpoint_pbits)
    {
        for (subset = 0; subset < mode->n_subsets; subset++)
        {
            for (endpoint = 0; endpoint < 2; endpoint++)
            {
                pbit = extract_bits(block, bit_offset, 1);
                bit_offset += 1;

                for (component = 0; component < n_components; component++)
                {
                    endpoints[subset * 2 + endpoint][component] <<= 1;
                    endpoints[subset * 2 + endpoint][component] |= pbit;
                }
            }
        }
    }
    else if (mode->has_shared_pbits)
    {
        for (subset = 0; subset < mode->n_subsets; subset++)
        {
            pbit = extract_bits(block, bit_offset, 1);
            bit_offset += 1;

            for (endpoint = 0; endpoint < 2; endpoint++)
            {
                for (component = 0; component < n_components; component++)
                {
                    endpoints[subset * 2 + endpoint][component] <<= 1;
                    endpoints[subset * 2 + endpoint][component] |= pbit;
                }
            }
        }
    }

    /* Expand the n-bit values to a byte */
    for (subset = 0; subset < mode->n_subsets; subset++)
    {
        for (endpoint = 0; endpoint < 2; endpoint++)
        {
            for (component = 0; component < 3; component++)
            {
                endpoints[subset * 2 + endpoint][component] = expand_component(
                    endpoints[subset * 2 + endpoint][component],
                    mode->n_color_bits + mode->has_endpoint_pbits + mode->has_shared_pbits);
            }

            if (mode->n_alpha_bits > 0)
            {
                endpoints[subset * 2 + endpoint][3] = expand_component(
                    endpoints[subset * 2 + endpoint][3],
                    mode->n_alpha_bits + mode->has_endpoint_pbits + mode->has_shared_pbits);
            }
        }
    }

    return bit_offset;
}

bool TBOX::TBOXEmu::is_anchor(int n_subsets, int partition_num, int texel)
{
    if (texel == 0)
        return true;

    switch (n_subsets)
    {
    case 1:
        return false;
    case 2:
        return anchor_indices[0][partition_num] == texel;
    case 3:
        return (anchor_indices[1][partition_num] == texel || anchor_indices[2][partition_num] == texel);
    default:
        return false;
    }
}

int TBOX::TBOXEmu::count_anchors_before_texel(int n_subsets, int partition_num, int texel)
{
    int count = 1;

    if (texel == 0)
        return 0;

    switch (n_subsets)
    {
    case 1:
        break;
    case 2:
        if (texel > anchor_indices[0][partition_num])
            count++;
        break;
    case 3:
        if (texel > anchor_indices[1][partition_num])
            count++;
        if (texel > anchor_indices[2][partition_num])
            count++;
        break;
    default:
        return 0;
    }

    return count;
}

int32_t TBOX::TBOXEmu::interpolate(int32_t a, int32_t b, int index, int index_bits)
{
    static const uint8_t weights2[] = {0, 21, 43, 64};
    static const uint8_t weights3[] = {0, 9, 18, 27, 37, 46, 55, 64};
    static const uint8_t weights4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};
    static const uint8_t *weights[] = {NULL, NULL, weights2, weights3, weights4};
    int weight;

    weight = weights[index_bits][index];

    return ((64 - weight) * a + weight * b + 32) >> 6;
}

void TBOX::TBOXEmu::apply_rotation(int rotation, uint8_t *result)
{
    uint8_t t;

    if (rotation == 0)
        return;

    rotation--;

    t = result[rotation];
    result[rotation] = result[3];
    result[3] = t;
}

void TBOX::TBOXEmu::fetch_rgba_unorm_from_block(const uint8_t *block, uint8_t *result, int texel)
{
    int mode_num = ffs(block[0]);
    const bptc_unorm_mode *mode;
    int bit_offset, secondary_bit_offset;
    int partition_num;
    int subset_num;
    int rotation;
    int index_selection;
    int index_bits;
    int indices[2];
    int index;
    int anchors_before_texel;
    bool anchor;
    uint8_t endpoints[3 * 2][4];
    uint32_t subsets;
    int component;

    if (mode_num == 0)
    {
        /* According to the spec this mode is reserved and shouldn't be used. */
        memset(result, 0, 3);
        result[3] = 0xff;
        return;
    }

    mode = bptc_unorm_modes + mode_num - 1;
    bit_offset = mode_num;

    partition_num = extract_bits(block, bit_offset, mode->n_partition_bits);
    bit_offset += mode->n_partition_bits;

    switch (mode->n_subsets)
    {
        case 1:
            subsets = 0;
            break;
        case 2:
            subsets = partition_table1[partition_num];
            break;
        case 3:
            subsets = partition_table2[partition_num];
            break;
        default:
            return;
    }

    if (mode->has_rotation_bits)
    {
        rotation = extract_bits(block, bit_offset, 2);
        bit_offset += 2;
    }
    else
    {
        rotation = 0;
    }

    if (mode->has_index_selection_bit)
    {
        index_selection = extract_bits(block, bit_offset, 1);
        bit_offset++;
    }
    else
    {
        index_selection = 0;
    }

    bit_offset = extract_unorm_endpoints(mode, block, bit_offset, endpoints);

    anchors_before_texel = count_anchors_before_texel(mode->n_subsets, partition_num, texel);

    /* Calculate the offset to the secondary index */
    secondary_bit_offset = (bit_offset + BLOCK_SIZE * BLOCK_SIZE * mode->n_index_bits - mode->n_subsets
                            + mode->n_secondary_index_bits * texel - anchors_before_texel);

    /* Calculate the offset to the primary index for this texel */
    bit_offset += mode->n_index_bits * texel - anchors_before_texel;

    subset_num = (subsets >> (texel * 2)) & 3;

    anchor = is_anchor(mode->n_subsets, partition_num, texel);

    index_bits = mode->n_index_bits;
    if (anchor)
        index_bits--;
    indices[0] = extract_bits(block, bit_offset, index_bits);

    if (mode->n_secondary_index_bits)
    {
        index_bits = mode->n_secondary_index_bits;
        if (anchor)
            index_bits--;
        indices[1] = extract_bits(block, secondary_bit_offset, index_bits);
    }

    index = indices[index_selection];
    index_bits = (index_selection ? mode->n_secondary_index_bits : mode->n_index_bits);

    for (component = 0; component < 3; component++)
        result[component] = interpolate(endpoints[subset_num * 2][component],
                                        endpoints[subset_num * 2 + 1][component], index, index_bits);

    /* Alpha uses the opposite index from the color components */
    if (mode->n_secondary_index_bits && !index_selection)
    {
        index = indices[1];
        index_bits = mode->n_secondary_index_bits;
    }
    else
    {
        index = indices[0];
        index_bits = mode->n_index_bits;
    }

    result[3]
        = interpolate(endpoints[subset_num * 2][3], endpoints[subset_num * 2 + 1][3], index, index_bits);

    apply_rotation(rotation, result);
}

void TBOX::TBOXEmu::fetch_bptc_rgba_unorm_bytes(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j,
                                                uint8_t *texel)
{
    const uint8_t *block;

    block = map + (((rowStride + 3) / 4) * (j / 4) + (i / 4)) * 16;

    fetch_rgba_unorm_from_block(block, texel, (i % 4) + (j % 4) * 4);
}

void TBOX::TBOXEmu::fetch_bptc_rgba_unorm(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel)
{
    uint8_t texel_bytes[4];

    fetch_bptc_rgba_unorm_bytes(map, rowStride, i, j, texel_bytes);

    texel[0] = fpu::FLT(fpu::un8_to_f32(texel_bytes[0]));
    texel[1] = fpu::FLT(fpu::un8_to_f32(texel_bytes[1]));
    texel[2] = fpu::FLT(fpu::un8_to_f32(texel_bytes[2]));
    texel[3] = fpu::FLT(fpu::un8_to_f32(texel_bytes[3]));
}

void TBOX::TBOXEmu::fetch_bptc_srgb_alpha_unorm(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel)
{
    uint8_t texel_bytes[4];

    fetch_bptc_rgba_unorm_bytes(map, rowStride, i, j, texel_bytes);

    texel[0] = fpu::FLT(fpu::un8_to_f32(texel_bytes[0]));
    texel[1] = fpu::FLT(fpu::un8_to_f32(texel_bytes[1]));
    texel[2] = fpu::FLT(fpu::un8_to_f32(texel_bytes[2]));
    texel[3] = fpu::FLT(fpu::un8_to_f32(texel_bytes[3]));
}

int32_t TBOX::TBOXEmu::sign_extend(int32_t value, int n_bits)
{
    if ((value & (1 << (n_bits - 1))))
    {
        value |= (~uint32_t(0)) << n_bits;
    }

    return value;
}

int TBOX::TBOXEmu::signed_unquantize(int value, int n_endpoint_bits)
{
    bool sign;

    if (n_endpoint_bits >= 16)
        return value;

    if (value == 0)
        return 0;

    sign = false;

    if (value < 0)
    {
        sign = true;
        value = -value;
    }

    if (value >= (1 << (n_endpoint_bits - 1)) - 1)
        value = 0x7fff;
    else
        value = ((value << 15) + 0x4000) >> (n_endpoint_bits - 1);

    if (sign)
        value = -value;

    return value;
}

int TBOX::TBOXEmu::unsigned_unquantize(int value, int n_endpoint_bits)
{
    if (n_endpoint_bits >= 15)
        return value;

    if (value == 0)
        return 0;

    if (value == (1 << n_endpoint_bits) - 1)
        return 0xffff;

    return ((value << 15) + 0x4000) >> (n_endpoint_bits - 1);
}

int TBOX::TBOXEmu::extract_float_endpoints(const bptc_float_mode *mode, const uint8_t *block, int bit_offset,
                                           int32_t endpoints[][3], bool is_signed)
{
    const bptc_float_bitfield *bitfield;
    int endpoint, component;
    int n_endpoints;
    int value;
    int i;

    if (mode->n_partition_bits)
        n_endpoints = 4;
    else
        n_endpoints = 2;

    memset(endpoints, 0, sizeof endpoints[0][0] * n_endpoints * 3);

    for (bitfield = mode->bitfields; bitfield->endpoint != -1; bitfield++)
    {
        value = extract_bits(block, bit_offset, bitfield->n_bits);
        bit_offset += bitfield->n_bits;

        if (bitfield->reverse)
        {
            for (i = 0; i < bitfield->n_bits; i++)
            {
                if (value & (1 << i))
                    endpoints[bitfield->endpoint][bitfield->component]
                        |= 1 << ((bitfield->n_bits - 1 - i) + bitfield->offset);
            }
        }
        else
        {
            endpoints[bitfield->endpoint][bitfield->component] |= value << bitfield->offset;
        }
    }

    if (mode->transformed_endpoints)
    {
        /* The endpoints are specified as signed offsets from e0 */
        for (endpoint = 1; endpoint < n_endpoints; endpoint++)
        {
            for (component = 0; component < 3; component++)
            {
                value = sign_extend(endpoints[endpoint][component], mode->n_delta_bits[component]);
                endpoints[endpoint][component]
                    = ((endpoints[0][component] + value) & ((1 << mode->n_endpoint_bits) - 1));
            }
        }
    }

    if (is_signed)
    {
        for (endpoint = 0; endpoint < n_endpoints; endpoint++)
        {
            for (component = 0; component < 3; component++)
            {
                value = sign_extend(endpoints[endpoint][component], mode->n_endpoint_bits);
                endpoints[endpoint][component] = signed_unquantize(value, mode->n_endpoint_bits);
            }
        }
    }
    else
    {
        for (endpoint = 0; endpoint < n_endpoints; endpoint++)
        {
            for (component = 0; component < 3; component++)
            {
                endpoints[endpoint][component]
                    = unsigned_unquantize(endpoints[endpoint][component], mode->n_endpoint_bits);
            }
        }
    }

    return bit_offset;
}

int32_t TBOX::TBOXEmu::finish_unsigned_unquantize(int32_t value) { return value * 31 / 64; }

int32_t TBOX::TBOXEmu::finish_signed_unquantize(int32_t value)
{
    if (value < 0)
        return (-value * 31 / 32) | 0x8000;
    else
        return value * 31 / 32;
}

void TBOX::TBOXEmu::fetch_rgb_float_from_block(const uint8_t *block, float *result, int texel, bool is_signed)
{
    int mode_num;
    const bptc_float_mode *mode;
    int bit_offset;
    int partition_num;
    int subset_num;
    int index_bits;
    int index;
    int anchors_before_texel;
    int32_t endpoints[2 * 2][3];
    uint32_t subsets;
    int n_subsets;
    int component;
    int32_t value;

    if (block[0] & 0x2)
    {
        mode_num = (((block[0] >> 1) & 0xe) | (block[0] & 1)) + 2;
        bit_offset = 5;
    }
    else
    {
        mode_num = block[0] & 3;
        bit_offset = 2;
    }

    mode = bptc_float_modes + mode_num;

    if (mode->reserved)
    {
        memset(result, 0, sizeof result[0] * 3);
        result[3] = 1.0f;
        return;
    }

    bit_offset = extract_float_endpoints(mode, block, bit_offset, endpoints, is_signed);

    if (mode->n_partition_bits)
    {
        partition_num = extract_bits(block, bit_offset, mode->n_partition_bits);
        bit_offset += mode->n_partition_bits;

        subsets = partition_table1[partition_num];
        n_subsets = 2;
    }
    else
    {
        partition_num = 0;
        subsets = 0;
        n_subsets = 1;
    }

    anchors_before_texel = count_anchors_before_texel(n_subsets, partition_num, texel);

    /* Calculate the offset to the primary index for this texel */
    bit_offset += mode->n_index_bits * texel - anchors_before_texel;

    subset_num = (subsets >> (texel * 2)) & 3;

    index_bits = mode->n_index_bits;
    if (is_anchor(n_subsets, partition_num, texel))
        index_bits--;
    index = extract_bits(block, bit_offset, index_bits);

    for (component = 0; component < 3; component++)
    {
        value = interpolate(endpoints[subset_num * 2][component], endpoints[subset_num * 2 + 1][component],
                            index, mode->n_index_bits);

        if (is_signed)
            value = finish_signed_unquantize(value);
        else
            value = finish_unsigned_unquantize(value);

        uint16_t value_fp16 = (uint16_t)(value & 0xFFFF);
        result[component] = fpu::FLT(fpu::f16_to_f32(fpu::F16(value_fp16)));
    }

    result[3] = 1.0f;
}

void TBOX::TBOXEmu::fetch_bptc_rgb_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel, bool is_signed)
{
    const uint8_t *block;

    block = map + (((rowStride + 3) / 4) * (j / 4) + (i / 4)) * 16;

    fetch_rgb_float_from_block(block, texel, (i % 4) + (j % 4) * 4, is_signed);
}

void TBOX::TBOXEmu::fetch_bptc_rgb_signed_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel)
{
    fetch_bptc_rgb_float(map, rowStride, i, j, texel, true);
}

void TBOX::TBOXEmu::fetch_bptc_rgb_unsigned_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel)
{
    fetch_bptc_rgb_float(map, rowStride, i, j, texel, false);
}

uint16_t TBOX::TBOXEmu::sharedexp_to_float16(uint32_t exponent, uint32_t mantissa)
{
    //
    // A shared exponent float point value encodes this float point value (always positive) :
    //
    // v = mantissa[8:0] * 2^(-9) * 2^(exponent[4:0] - 5'd15)
    //

    // Mantissa in shared exponent format has no hidden bit so if the mantissa
    // is 0 then the exponent can be ignored and the float16 encoding is directly 0.
    if (mantissa == 0)
        return 0;

    uint32_t normalized_exponent = exponent;

    // Convert mantissa from 9 bits to 11 bits to match float16 mantissa.
    // The hidden bit (10) is 0 initially.
    //
    // Shared exponent format allows to encode exponents beyond the range
    // of float16 exponents :  [16, -15] vs [15, -14].  As the hidden
    // bit is not present the maximum exponent of 16 is always effectively
    // converted to 15.  The case of exponent -15 is handled by shifting
    // the mantissa in bit right.
    //
    // If the shared exponent is not 0 (> -15):
    //
    // normalized_mantissa[10:0] = {1'b0, mantissa[8:0], 1'b0}
    //
    // If the shared exponent is 0 (= -15):
    //
    // normalized_mantissa[10:0] = {2'b0, mantissa[8:0]}
    //
    //
    uint32_t normalized_mantissa = (exponent == 0) ? mantissa : mantissa << 1;

    // Normalize the mantissa so that MSB bit is 1, adjust the exponent
    // and stop when reaching the denormalized exponent (0 in excess 15).
    while (((normalized_mantissa & 0x0400) == 0) && (normalized_exponent > 1))
    {
        normalized_exponent--;
        normalized_mantissa = normalized_mantissa << 1;
    }

    // In float16 the minimum exponent is -14 which corresponds with encoded
    // exponent value 1.  But if the hidden bit is 0 then it's encoded as 0
    // (denormalized value).
    if ((normalized_mantissa & 0x0400) == 0)
        normalized_exponent = 0;

    // Encode the float16 value :
    //   - Sign is positive.
    //   - Mantissa is 10 bits dropping hidden bit.
    uint16_t v_fp16 = ((normalized_exponent & 0x01F) << 10) | (normalized_mantissa & 0x3FF);

    return v_fp16;
}


