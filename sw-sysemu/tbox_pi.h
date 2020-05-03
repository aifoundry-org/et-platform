/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _TBOX_PO_H_
#define _TBOX_PO_H_

#include <cstdint>

namespace bemu {
namespace TBOX {


    typedef enum
    {
        SAMPLE_OP_SAMPLE       =  0,
        SAMPLE_OP_SAMPLE_L     =  1,
        SAMPLE_OP_SAMPLE_C     =  2,
        SAMPLE_OP_SAMPLE_C_L   =  3,
        SAMPLE_OP_GATHER4      =  4,
        SAMPLE_OP_GATHER4_C    =  5,
        SAMPLE_OP_GATHER4_PO   =  6,
        SAMPLE_OP_GATHER4_PO_C =  7,
        SAMPLE_OP_LD           =  8,
        MAX_SAMPLE_OP          = 15
    } SampleOperation;
    
    typedef enum
    {
        FILTER_TYPE_NEAREST = 0,
        FILTER_TYPE_LINEAR  = 1,
        MAX_FILTER_TYPE     = 1
    } FilterType;
    
    typedef enum
    {
        ADDRESS_MODE_REPEAT               = 0,
        ADDRESS_MODE_MIRRORED_REPEAT      = 1,
        ADDRESS_MODE_CLAMP_TO_EDGE        = 2,
        ADDRESS_MODE_CLAMP_TO_BORDER      = 3,
        ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
        MAX_ADDRESS_MODE                  = 7
    } AddressMode;
    
    typedef enum
    {
        COMPARE_OP_NEVER            = 0,
        COMPARE_OP_LESS             = 1,
        COMPARE_OP_EQUAL            = 2,
        COMPARE_OP_LESS_OR_EQUAL    = 3,
        COMPARE_OP_GREATER          = 4,
        COMPARE_OP_NOT_EQUAL        = 5,
        COMPARE_OP_GREATER_OR_EQUAL = 6,
        COMPARE_OP_ALWAYS           = 7,
        MAX_COMPARE_OPERATION       = 7
    } CompareOperation;
    
    typedef enum
    {
        COMPONENT_SWIZZLE_NONE     = 0,
        COMPONENT_SWIZZLE_IDENTITY = 1,
        COMPONENT_SWIZZLE_ZERO     = 2,
        COMPONENT_SWIZZLE_ONE      = 3,
        COMPONENT_SWIZZLE_R        = 4,
        COMPONENT_SWIZZLE_G        = 5,
        COMPONENT_SWIZZLE_B        = 6,
        COMPONENT_SWIZZLE_A        = 7,
        MAX_COMPONENT_SWIZZLE      = 8
    } ComponentSwizzle;
    
    typedef enum
    {
        BORDER_COLOR_TRANSPARENT_BLACK     = 0,
        BORDER_COLOR_OPAQUE_BLACK          = 1,
        BORDER_COLOR_OPAQUE_WHITE          = 2,
        BORDER_COLOR_FROM_IMAGE_DESCRIPTOR = 3,
        MAX_BORDER_COLOR                   = 4,
    } BorderColor;
    
    typedef enum
    {
        IMAGE_TYPE_1D         = 0,
        IMAGE_TYPE_2D         = 1,
        IMAGE_TYPE_CUBE       = 2,
        IMAGE_TYPE_3D         = 3,
        IMAGE_TYPE_1D_ARRAY   = 4,
        IMAGE_TYPE_2D_ARRAY   = 5,
        IMAGE_TYPE_CUBE_ARRAY = 6,
        MAX_IMAGE_TYPE        = 7
    } ImageType;
    
    typedef enum
    {
        FORMAT_UNDEFINED                  = 0,
        FORMAT_R4G4_UNORM_PACK8           = 1,
        FORMAT_R4G4B4A4_UNORM_PACK16      = 2,
        FORMAT_B4G4R4A4_UNORM_PACK16      = 3,
        FORMAT_R5G6B5_UNORM_PACK16        = 4,
        FORMAT_B5G6R5_UNORM_PACK16        = 5,
        FORMAT_R5G5B5A1_UNORM_PACK16      = 6,
        FORMAT_B5G5R5A1_UNORM_PACK16      = 7,
        FORMAT_A1R5G5B5_UNORM_PACK16      = 8,
        FORMAT_R8_UNORM                   = 9,
        FORMAT_R8_SNORM                   = 10,
        FORMAT_R8_USCALED                 = 11,
        FORMAT_R8_SSCALED                 = 12,
        FORMAT_R8_UINT                    = 13,
        FORMAT_R8_SINT                    = 14,
        FORMAT_R8_SRGB                    = 15,
        FORMAT_R8G8_UNORM                 = 16,
        FORMAT_R8G8_SNORM                 = 17,
        FORMAT_R8G8_USCALED               = 18,
        FORMAT_R8G8_SSCALED               = 19,
        FORMAT_R8G8_UINT                  = 20,
        FORMAT_R8G8_SINT                  = 21,
        FORMAT_R8G8_SRGB                  = 22,
        FORMAT_R8G8B8_UNORM               = 23,
        FORMAT_R8G8B8_SNORM               = 24,
        FORMAT_R8G8B8_USCALED             = 25,
        FORMAT_R8G8B8_SSCALED             = 26,
        FORMAT_R8G8B8_UINT                = 27,
        FORMAT_R8G8B8_SINT                = 28,
        FORMAT_R8G8B8_SRGB                = 29,
        FORMAT_B8G8R8_UNORM               = 30,
        FORMAT_B8G8R8_SNORM               = 31,
        FORMAT_B8G8R8_USCALED             = 32,
        FORMAT_B8G8R8_SSCALED             = 33,
        FORMAT_B8G8R8_UINT                = 34,
        FORMAT_B8G8R8_SINT                = 35,
        FORMAT_B8G8R8_SRGB                = 36,
        FORMAT_R8G8B8A8_UNORM             = 37,
        FORMAT_R8G8B8A8_SNORM             = 38,
        FORMAT_R8G8B8A8_USCALED           = 39,
        FORMAT_R8G8B8A8_SSCALED           = 40,
        FORMAT_R8G8B8A8_UINT              = 41,
        FORMAT_R8G8B8A8_SINT              = 42,
        FORMAT_R8G8B8A8_SRGB              = 43,
        FORMAT_B8G8R8A8_UNORM             = 44,
        FORMAT_B8G8R8A8_SNORM             = 45,
        FORMAT_B8G8R8A8_USCALED           = 46,
        FORMAT_B8G8R8A8_SSCALED           = 47,
        FORMAT_B8G8R8A8_UINT              = 48,
        FORMAT_B8G8R8A8_SINT              = 49,
        FORMAT_B8G8R8A8_SRGB              = 50,
        FORMAT_A8B8G8R8_UNORM_PACK32      = 51,
        FORMAT_A8B8G8R8_SNORM_PACK32      = 52,
        FORMAT_A8B8G8R8_USCALED_PACK32    = 53,
        FORMAT_A8B8G8R8_SSCALED_PACK32    = 54,
        FORMAT_A8B8G8R8_UINT_PACK32       = 55,
        FORMAT_A8B8G8R8_SINT_PACK32       = 56,
        FORMAT_A8B8G8R8_SRGB_PACK32       = 57,
        FORMAT_A2R10G10B10_UNORM_PACK32   = 58,
        FORMAT_A2R10G10B10_SNORM_PACK32   = 59,
        FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
        FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
        FORMAT_A2R10G10B10_UINT_PACK32    = 62,
        FORMAT_A2R10G10B10_SINT_PACK32    = 63,
        FORMAT_A2B10G10R10_UNORM_PACK32   = 64,
        FORMAT_A2B10G10R10_SNORM_PACK32   = 65,
        FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
        FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
        FORMAT_A2B10G10R10_UINT_PACK32    = 68,
        FORMAT_A2B10G10R10_SINT_PACK32    = 69,
        FORMAT_R16_UNORM                  = 70,
        FORMAT_R16_SNORM                  = 71,
        FORMAT_R16_USCALED                = 72,
        FORMAT_R16_SSCALED                = 73,
        FORMAT_R16_UINT                   = 74,
        FORMAT_R16_SINT                   = 75,
        FORMAT_R16_SFLOAT                 = 76,
        FORMAT_R16G16_UNORM               = 77,
        FORMAT_R16G16_SNORM               = 78,
        FORMAT_R16G16_USCALED             = 79,
        FORMAT_R16G16_SSCALED             = 80,
        FORMAT_R16G16_UINT                = 81,
        FORMAT_R16G16_SINT                = 82,
        FORMAT_R16G16_SFLOAT              = 83,
        FORMAT_R16G16B16_UNORM            = 84,
        FORMAT_R16G16B16_SNORM            = 85,
        FORMAT_R16G16B16_USCALED          = 86,
        FORMAT_R16G16B16_SSCALED          = 87,
        FORMAT_R16G16B16_UINT             = 88,
        FORMAT_R16G16B16_SINT             = 89,
        FORMAT_R16G16B16_SFLOAT           = 90,
        FORMAT_R16G16B16A16_UNORM         = 91,
        FORMAT_R16G16B16A16_SNORM         = 92,
        FORMAT_R16G16B16A16_USCALED       = 93,
        FORMAT_R16G16B16A16_SSCALED       = 94,
        FORMAT_R16G16B16A16_UINT          = 95,
        FORMAT_R16G16B16A16_SINT          = 96,
        FORMAT_R16G16B16A16_SFLOAT        = 97,
        FORMAT_R32_UINT                   = 98,
        FORMAT_R32_SINT                   = 99,
        FORMAT_R32_SFLOAT                 = 100,
        FORMAT_R32G32_UINT                = 101,
        FORMAT_R32G32_SINT                = 102,
        FORMAT_R32G32_SFLOAT              = 103,
        FORMAT_R32G32B32_UINT             = 104,
        FORMAT_R32G32B32_SINT             = 105,
        FORMAT_R32G32B32_SFLOAT           = 106,
        FORMAT_R32G32B32A32_UINT          = 107,
        FORMAT_R32G32B32A32_SINT          = 108,
        FORMAT_R32G32B32A32_SFLOAT        = 109,
        FORMAT_R64_UINT                   = 110,
        FORMAT_R64_SINT                   = 111,
        FORMAT_R64_SFLOAT                 = 112,
        FORMAT_R64G64_UINT                = 113,
        FORMAT_R64G64_SINT                = 114,
        FORMAT_R64G64_SFLOAT              = 115,
        FORMAT_R64G64B64_UINT             = 116,
        FORMAT_R64G64B64_SINT             = 117,
        FORMAT_R64G64B64_SFLOAT           = 118,
        FORMAT_R64G64B64A64_UINT          = 119,
        FORMAT_R64G64B64A64_SINT          = 120,
        FORMAT_R64G64B64A64_SFLOAT        = 121,
        FORMAT_B10G11R11_UFLOAT_PACK32    = 122,
        FORMAT_E5B9G9R9_UFLOAT_PACK32     = 123,
        FORMAT_D16_UNORM                  = 124,
        FORMAT_X8_D24_UNORM_PACK32        = 125,
        FORMAT_D32_SFLOAT                 = 126,
        FORMAT_S8_UINT                    = 127,
        FORMAT_D16_UNORM_S8_UINT          = 128,
        FORMAT_D24_UNORM_S8_UINT          = 129,
        FORMAT_D32_SFLOAT_S8_UINT         = 130,
        FORMAT_BC1_RGB_UNORM_BLOCK        = 131,
        FORMAT_BC1_RGB_SRGB_BLOCK         = 132,
        FORMAT_BC1_RGBA_UNORM_BLOCK       = 133,
        FORMAT_BC1_RGBA_SRGB_BLOCK        = 134,
        FORMAT_BC2_UNORM_BLOCK            = 135,
        FORMAT_BC2_SRGB_BLOCK             = 136,
        FORMAT_BC3_UNORM_BLOCK            = 137,
        FORMAT_BC3_SRGB_BLOCK             = 138,
        FORMAT_BC4_UNORM_BLOCK            = 139,
        FORMAT_BC4_SNORM_BLOCK            = 140,
        FORMAT_BC5_UNORM_BLOCK            = 141,
        FORMAT_BC5_SNORM_BLOCK            = 142,
        FORMAT_BC6H_UFLOAT_BLOCK          = 143,
        FORMAT_BC6H_SFLOAT_BLOCK          = 144,
        FORMAT_BC7_UNORM_BLOCK            = 145,
        FORMAT_BC7_SRGB_BLOCK             = 146,
        FORMAT_ETC2_R8G8B8_UNORM_BLOCK    = 147,
        FORMAT_ETC2_R8G8B8_SRGB_BLOCK     = 148,
        FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK  = 149,
        FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK   = 150,
        FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK  = 151,
        FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK   = 152,
        FORMAT_EAC_R11_UNORM_BLOCK        = 153,
        FORMAT_EAC_R11_SNORM_BLOCK        = 154,
        FORMAT_EAC_R11G11_UNORM_BLOCK     = 155,
        FORMAT_EAC_R11G11_SNORM_BLOCK     = 156,
        FORMAT_ASTC_4x4_UNORM_BLOCK       = 157,
        FORMAT_ASTC_4x4_SRGB_BLOCK        = 158,
        FORMAT_ASTC_5x4_UNORM_BLOCK       = 159,
        FORMAT_ASTC_5x4_SRGB_BLOCK        = 160,
        FORMAT_ASTC_5x5_UNORM_BLOCK       = 161,
        FORMAT_ASTC_5x5_SRGB_BLOCK        = 162,
        FORMAT_ASTC_6x5_UNORM_BLOCK       = 163,
        FORMAT_ASTC_6x5_SRGB_BLOCK        = 164,
        FORMAT_ASTC_6x6_UNORM_BLOCK       = 165,
        FORMAT_ASTC_6x6_SRGB_BLOCK        = 166,
        FORMAT_ASTC_8x5_UNORM_BLOCK       = 167,
        FORMAT_ASTC_8x5_SRGB_BLOCK        = 168,
        FORMAT_ASTC_8x6_UNORM_BLOCK       = 169,
        FORMAT_ASTC_8x6_SRGB_BLOCK        = 170,
        FORMAT_ASTC_8x8_UNORM_BLOCK       = 171,
        FORMAT_ASTC_8x8_SRGB_BLOCK        = 172,
        FORMAT_ASTC_10x5_UNORM_BLOCK      = 173,
        FORMAT_ASTC_10x5_SRGB_BLOCK       = 174,
        FORMAT_ASTC_10x6_UNORM_BLOCK      = 175,
        FORMAT_ASTC_10x6_SRGB_BLOCK       = 176,
        FORMAT_ASTC_10x8_UNORM_BLOCK      = 177,
        FORMAT_ASTC_10x8_SRGB_BLOCK       = 178,
        FORMAT_ASTC_10x10_UNORM_BLOCK     = 179,
        FORMAT_ASTC_10x10_SRGB_BLOCK      = 180,
        FORMAT_ASTC_12x10_UNORM_BLOCK     = 181,
        FORMAT_ASTC_12x10_SRGB_BLOCK      = 182,
        FORMAT_ASTC_12x12_UNORM_BLOCK     = 183,
        FORMAT_ASTC_12x12_SRGB_BLOCK      = 184,
        MAX_IMAGE_FORMAT                  = 511
    } ImageFormat;

    
    /*   OLD 128-bit header  
    typedef union
    {
        uint64_t data[2];
        struct
        {   // A means to be added to: sh_res_tex_txreq
            // B means to be added to: samp_txreq
            uint64_t operation :  4,
                     opmode    :  3,  // B only if sample_*c. Compiler must write it if gather4 w/o comparison. Comparison mode is in trace.txt, in CompareOperation here.
                     imageid   : 12,  // A
                     ioffset   :  4,  // Compiler
                     joffset   :  4,  // Compiler
                     koffset   :  4,  // Compiler
                     minfilter :  1,  // B. filter mode in trace.txt 1-linear 0-near
                     magfilter :  1,  // B. filter mode in trace.txt
                     mipfilter :  1,  // B. filter mode in trace.txt
                     aniso     :  1,  // B. filter mode in trace.txt. 1 if (anisotropic, anisotropic, anisotropic, normal)
                     addrmodeu :  3,  // B. addressing mode in trace.txt. AddressMode here
                     addrmodev :  3,  // B. addressing mode in trace.txt. AddressMode here
                     addrmodew :  3,  // B. addressing mode in trace.txt. AddressMode here
                     border    :  2,  // B. BorderColor. Set always to BORDER_COLOR_TRANSPARENT_BLACK (0)
                     swizzler  :  3,  // Compiler
                     swizzleg  :  3,  // Compiler
                     swizzleb  :  3,  // Compiler
                     swizzlea  :  3,  // Compiler
                     mask      :  4,  // Compiler
                     packets   :  2;  // Compiler. Number of dimensions
            union
            {
                uint16_t lod_array[4];
                struct
                {
                    uint64_t lod         : 16,
                             anisodeltau :  8,
                             anisodeltav :  8,
                             anisoratio  : 16,
                             unused      : 16;
                } lodaniso;
            } lodaniso;
        } info;
    } SampleRequest; */
    
    typedef union
    {
        uint64_t data[4];
        struct
        {
            // Image Instruction
            uint64_t operation :  4,    // Header[  3:  0], Image Sampling Operation : SAMPLE, SAMPLE_L, SAMPLE_C, SAMPLE_C_L, GATHER4, GATHER4_C, GATHER4_PO, GATHER4_PO_C
                     component :  2,    // Header[  5:  4], Component Selection (for GATHER4*) : Red, Green, Blue, Alpha
                     ioffset   :  4,    // Header[  9:  6], Texel Offset i (horizontal) : [-8, 7]
                     joffset   :  4,    // Header[ 13: 10], Texel Offset j (vertical)   : [-8, 7]
                     koffset   :  4,    // Header[ 17: 14], Texel Offset k (depth)      : [-8, 7]
                     packets   :  2,    // Header[ 18: 19], Sampler Request Packets (Only coordinate packets) : [1 to 3]
                     reserved0 : 12,    // Header[ 31: 20], Reserved
                     mask      :  8,    // Header[ 39: 32], Pixel Mask : {q1p3, q1p2, q1p1, q1p0, q0p3, q0p2, q0p1, q0p0}
                     reserved1 : 24;    // Header[ 63: 40], Reserved
            // Image State 
            uint32_t imageid   : 12,    // Header[ 75: 64], Image Descriptor Identifier 
                     reserved3 :  4,    // Header[ 79: 76], Reserved
                     borderid  : 12,    // Header[ 91: 80], Border Descriptor Identifier
                     reserved4 :  4;    // Header[ 95: 92], Reserved
    
            // Sampler State
            uint32_t minfilter :  1,    // Header[     96], Minification Filter : Nearest, Linear
                     magfilter :  1,    // Header[     97], Magnification Filter : Nearest, Linear
                     mipfilter :  1,    // Header[     98], Mipmap Filter : Nearest, Linear
                     aniso     :  1,    // Header[     99], Anisotropic Filter : Disabled, Enabled
                     compop    :  3,    // Header[102:100], Compare Operation (for SAMPLE_C*, GATHER4_C*) : Never, Less, Equal, Less_or_Equal, Greater, Not_Equal, Greater_or_Equal, Always
                     addrmodeu :  3,    // Header[105:103], Address Mode u (horizontal) : REPEAT, MIRRORED_REPEAT, CLAMP_TO_EDGE, CLAMP_TO_BORDER, MIRROR_CLAMP_TO_EDGE
                     addrmodev :  3,    // Header[108:106], Address Mode v (vertical)   : REPEAT, MIRRORED_REPEAT, CLAMP_TO_EDGE, CLAMP_TO_BORDER, MIRROR_CLAMP_TO_EDGE
                     addrmodew :  3,    // Header[111:109], Address Mode w (depth)      : REPEAT, MIRRORED_REPEAT, CLAMP_TO_EDGE, CLAMP_TO_BORDER, MIRROR_CLAMP_TO_EDGE
                     border    :  2,    // Header[113:112], Border Color Mode : Transparent_Black, Opaque_Black, Opaque_White, From_Image_Descriptor
                     swizzler  :  3,    // Header[116:114], Component Swizzle Red   : None, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzleg  :  3,    // Header[119:117], Component Swizzle Green : None, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzleb  :  3,    // Header[122:120], Component Swizzle Blue  : None, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzlea  :  3,    // Header[125:123], Component Swizzle Alpha : None, Identity, Zero, One, Red, Green, Blue, Alpha
                     reserved5 :  2;    // Header[127:126], Reserved
            union
            {
                uint16_t lod_array[2][4];       // Header[256:128], Per Pixel LOD : {q1p3, q1p2, q1p1, q1p0, q0p3, q0p2, q0p1, q0p0}
                struct
                {
                    uint16_t lod[2];            // Header[159:128], Per Quad LOD : {q1, q0}
                    uint32_t reserved0;         // Header[191:160], Reserved
                    uint8_t  anisodeltas[2];    // Header[199:192], Per Quad Anistropic Sample Delta u (horizontal): {q1, q0} [-1.0, 1.0]
                    uint8_t  anisodeltat[2];    // Header[215:208], Per Quad Anistropic Sample Delta v (vertical)  : {q1, q0} [-1.0, 1.0]
                    uint16_t anisoratio[2];     // Header[255:240], Per Quad Anisotropic Ratio : {q1, q0}
                } lodanisoq;
            } lodaniso;
        } info;
    } SampleRequest;
    
    typedef union
    {
        uint64_t data[4];
        struct
        {
            uint64_t address;               // Image Descriptor[ 63:  0], Image Base Pointer
            uint64_t type         :  3,     // Image Descriptor[ 66: 64], Image Type : 1D, 2D, Cube, 1D Array, 2D Array, Cube Array
                     format       :  9,     // Image Descriptor[ 75: 67], Image Format
                     width        : 16,     // Image Descriptor[ 91: 76], Image Width  : [1, 64K]
                     height       : 16,     // Image Descriptor[107: 92], Image Height : [1, 64K]
                     depth        : 15,     // Image Descriptor[122:108], Image Depth (only for 3D Images) : [1, 32K]
                     mipcount     :  5;     // Image Descriptor[127:123], Mip Level Count : [1, 17]
                     
            uint64_t arraycount   : 12,     // Image Descriptor[139:128], Array Layer Count : [1, 4K]
                     arraybase    : 12,     // Image Descriptor[151:140], Base Array Layer  : [1, 4K]
                     basemip      :  5,     // Image Descriptor[156:152], Base Mip Level    : [0, 16]
                     swizzler     :  3,     // Image Descriptor[164:162], Component Swizzle Red   : Reserved, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzleg     :  3,     // Image Descriptor[167:165], Component Swizzle Green : Reserved, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzleb     :  3,     // Image Descriptor[170:168], Component Swizzle BLue  : Reserved, Identity, Zero, One, Red, Green, Blue, Alpha
                     swizzlea     :  3,     // Image Descriptor[173:171], Component Swizzle Alpha : Reserved, Identity, Zero, One, Red, Green, Blue, Alpha
                     rowpitch     : 14,     // Image Descriptor[188:175], Row Pitch : for Linear Layout [0, 16383], for Standard Tiled Layout [0, 1023]
                     reserved0    :  3;     // Image Descriptor[191:188], Reserved
    
            uint64_t mippitchl0   :  5,     // Image Descriptor[196:192], Mip Pitch L0 : for Linear Layout [1, 30], for Standard Tiled Layout [1, 20]
                     mippitchl1   :  5,     // Image Descriptor[201:197], Mip Pitch L1 : for Linear Layout [1, 29], for Standard Tiled Layout [1, 19]
                     mipscale8    :  5,     // Image Descriptor[206:202], Mip Count Scale by 8 (for 3D Images)
                     mipscale4    :  5,     // Image Descriptor[211:207], Mip Count Scale by 4 (for 2D, Cube and 3D Images)
                     elementpitch : 30,     // Image Descriptor[241:212], Element/Slice Pitch : for Linear Layout 2D Images [0, 1073741823]
                                            //                                                  for Standard Tiled Layout 2D Images [0, 1048575]
                                            //                                                  for Standard Tiled Layout 3D Images [0, 262143]
                     tiled        :  1,     // Image Descriptor[    242], Tiled : Linear Layout, Standard Tiled Layout
                     packedlayout :  1,     // Image Descriptor[    243], Packed Mip Layout : Vertical Packing, Horizontal Packing
                     packedmip    :  4,     // Image Descriptor[245:244], First Packed Mip : for Linear Layout [0, 10], for Standard Tiled Layout [0, 11]
                     packedlevel  :  4,     // Image Descriptor[247:246], First Packed Mip Level : for Linear Layout [0, 6], for Standard Tiled Layout [0, 7]
                     reserved1    :  8;     // Image Descriptor[255:248], Reserved
        } info;
    } ImageInfo;


} // namespace TBOX
} // namespace bemu

#endif

