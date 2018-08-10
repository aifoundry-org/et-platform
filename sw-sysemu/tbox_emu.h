#ifndef _TBOX_EMU_H
#define _TBOX_EMU_H

#include "emu_defines.h"

class TBOXEmu
{
private:

    static const uint32_t TEXTURE_CACHE_BANKS = 2;
    static const uint32_t TEXTURE_CACHE_LINES_PER_BANK = 32;
    static const uint32_t TEXTURE_CACHE_QWORDS_PER_LINE = 8;

    static const uint32_t MAX_L2_REQUESTS = 1024;
    static const uint32_t L2_REQUEST_QWORDS = 8;

    static const uint32_t IMAGE_INFO_CACHE_SIZE = 16;

    typedef union
    {
        struct
        {
            uint16_t must_be_zero_lo   : 3,
                   mantissa_variable : 7,
                   exponent_variable : 4,
                   must_be_zero_hi   : 2;
        } encode;
        uint16_t value;
    } EncodeSRGBFP16;

    static const EncodeSRGBFP16 SRGB2LINEAR_TABLE[256];

    static const uint32_t BYTES_PER_TEXEL_IN_MEMORY[];
    static const uint32_t BYTES_PER_TEXEL_IN_L1[];

public :

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
                     packets   :  2;  // Compiler
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
    } SampleRequest;

    typedef union
    {
        uint64_t data[4];
        struct
        {
            uint64_t address;
            uint64_t type         :  3,
                     format       :  9,
                     width        : 16,
                     height       : 16,
                     depth        : 12,
                     reserved0    :  8;
            uint64_t arraybase    : 12,
                     arraycount   : 12,
                     basemip      :  5,
                     mipcount     :  5,
                     swizzler     :  3,
                     swizzleg     :  3,
                     swizzleb     :  3,
                     swizzlea     :  3,
                     reserved1    : 18;
            uint64_t rowpitch     : 10,
                     mippitchl0   :  5,
                     mippitchl1   :  5,
                     elementpitch : 30,
                     tiled        :  1,
                     packedlayout :  1,
                     packedmip    :  4,
                     packedlevel  :  4,
                     reserved2    :  4;
        } info;
    } ImageInfo;

    typedef struct
    {
        uint32_t thread_mask;
        uint64_t address;                     // Aligned to request size
        uint64_t data[L2_REQUEST_QWORDS];
        bool     ready;
        bool     free;
    } L2Request;

    void set_request_header(uint32_t thread, uint64_t src1, uint64_t src2);
    void set_request_coordinates(uint32_t thread, uint32_t index, fdata coord);
    fdata get_request_results(uint32_t thread, uint32_t index);
    void set_request_pending(uint32_t thread, bool v);
    bool check_request_pending(uint32_t thread);
    void set_image_table_address(uint64_t addr);
    uint64_t get_image_table_address();
    void print_sample_request(uint32_t thread) const;

    void texture_cache_initialize();
    void image_info_cache_initialize();

    void sample_quad(uint32_t thread, bool fake_sampler, bool output_result);
    bool get_image_info(uint32_t thread, ImageInfo &currentImage);
    bool get_image_info(SampleRequest request, ImageInfo &currentImage);
    void sample_quad(SampleRequest currentRequest, fdata input[], fdata output[], bool fake_sampler);
    void sample_quad(SampleRequest currentRequest, ImageInfo currentImage, fdata input[], fdata output[], bool output_result);
    void decompress_texture_cache_line_data(ImageInfo currentImage, uint32_t startTexel,
                                            uint64_t inData[TEXTURE_CACHE_QWORDS_PER_LINE], uint64_t outData[TEXTURE_CACHE_QWORDS_PER_LINE]);

    //L2Request* get_l2_request_queue() const;
    uint32_t get_num_new_l2_requests(uint32_t thread) const;
    uint32_t get_num_pending_l2_requests(uint32_t thread) const;
    void clear_l2_requests(uint32_t thread);
    void reset_l2_requests_counters(uint32_t thread);

    static void print_sample_request(SampleRequest req);
    static void print_image_info(ImageInfo in);

    static uint64_t compute_mip_offset(uint32_t mip_pitch_l0, uint32_t mip_pitch_l1, uint32_t row_pitch, uint32_t rows, uint32_t mip_level);

    TBOXEmu();

private :

    static const char *toStrSampleOperation(SampleOperation op);
    static const char *toStrFilterType(FilterType type);
    static const char *toStrAddressMode(AddressMode am);
    static const char *toStrCompareOperation(CompareOperation cop);
    static const char *toStrComponentSwizzle(ComponentSwizzle swz);
    static const char *toStrBorderColor(BorderColor bc);
    static const char *toStrImageType(ImageType type);
    static const char *toStrImageFormat(ImageFormat fmt);

    float compare_texel(CompareOperation compop, float reference, float input);

    void wrap_texel_coord(uint32_t c[2], int32_t c_ul, uint32_t mip_dim, AddressMode addrmode);

    float apply_component_swizzle(ComponentSwizzle swizzle, float source, float red, float green, float blue, float alpha);

    uint64_t imageTableAddress;

    SampleRequest currentRequest[EMU_NUM_THREADS];

    bool request_pending[EMU_NUM_THREADS];

    fdata input[EMU_NUM_THREADS][5];

    fdata output[EMU_NUM_THREADS][4];

    uint32_t current_thread;

    uint32_t imageInfoCacheTags[IMAGE_INFO_CACHE_SIZE];
    ImageInfo imageInfoCache[IMAGE_INFO_CACHE_SIZE];
    bool imageInfoCacheValid[IMAGE_INFO_CACHE_SIZE];
    uint8_t imageInfoCacheLRU[IMAGE_INFO_CACHE_SIZE];

    uint64_t textureCacheTags[TEXTURE_CACHE_BANKS][TEXTURE_CACHE_LINES_PER_BANK];
    bool textureCacheValid[TEXTURE_CACHE_BANKS][TEXTURE_CACHE_LINES_PER_BANK];
    uint8_t textureCacheLRU[TEXTURE_CACHE_BANKS][TEXTURE_CACHE_LINES_PER_BANK];
    uint64_t textureCacheData[TEXTURE_CACHE_BANKS][TEXTURE_CACHE_LINES_PER_BANK][TEXTURE_CACHE_QWORDS_PER_LINE];

    L2Request l2_requests[MAX_L2_REQUESTS];
    uint32_t num_new_l2_requests[EMU_NUM_THREADS];
    uint32_t num_pending_l2_requests[EMU_NUM_THREADS];
    uint32_t num_created_l2_requests[EMU_NUM_THREADS];
    uint32_t num_total_l2_requests;

    bool   texture_cache_lookup(int32_t bank, uint64_t tag, uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE]);
    void   texture_cache_fill(int32_t bank, uint64_t tag, uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE]);
    uint32_t texture_cache_get_lru(uint32_t bank);
    void   texture_cache_update_lru(uint32_t bank, uint32_t access_way);

    bool   image_info_cache_lookup(uint32_t tag, ImageInfo &data);
    void   image_info_cache_fill(uint32_t tag, ImageInfo data);
    uint32_t image_info_cache_get_lru();
    void   image_info_cache_update_lru(uint32_t access_way);

    bool access_memory(uint64_t address, uint64_t &data);
    bool access_memory(uint64_t address, uint32_t &data);

    bool access_l2(uint64_t address, uint64_t &data);
    bool access_l2(uint64_t address, uint32_t &data);

    bool get_l2_data(uint64_t address, uint64_t &data);
    bool get_l2_data(uint64_t address, uint32_t &data);
    bool get_l2_data(uint64_t address, ImageInfo &data);

    void create_l2_request(uint64_t);

    void sample_bilinear(SampleRequest currentRequest, fdata s, fdata t, fdata r,
                         uint32_t req, ImageInfo currentImage, FilterType filter,
                         uint32_t slice, uint32_t sample_mip_level, float sample_mip_beta,
                         uint32_t aniso_sample, float aniso_weight, float aniso_deltas, float aniso_deltat,
                         float &red, float &green, float &blue, float &alpha, bool output_result);
    void sample_pixel(SampleRequest currentRequest, fdata input[], fdata output[],
                      uint32_t req, ImageInfo currentImage, FilterType filter, uint32_t mip_level,
                      uint32_t mip_beta, bool output_result);
    void read_texel(ImageInfo currentImage, uint32_t i, uint32_t j, uint32_t k, uint32_t l, uint32_t mip_level, float *texel);
    void read_texel(ImageInfo currentImage, uint32_t i, uint32_t j, uint64_t data[], float *texel, bool data_ready);
    void create_texture_cache_tags(SampleRequest currentRequest, ImageInfo currentImage, FilterType filter,
                                   uint32_t i[2], uint32_t j[2], uint32_t k, uint32_t l, uint32_t mip_level,
                                   uint32_t &num_banks, int32_t banks[4], uint64_t tags[4], uint64_t address[4][4]);
    uint64_t texel_virtual_address(ImageInfo currentImage, uint32_t i, uint32_t j, uint32_t k, uint32_t l, uint32_t mip_level);
    void read_bilinear_texels(ImageInfo currentImage, uint32_t i[2], uint32_t j[2], uint32_t k, uint32_t l, uint32_t mip_level,
                              float *texel_ul, float *texel_ur, float *texel_ll, float *texel_lr);
    bool read_texture_cache_line(ImageInfo currentImage, uint64_t address[4], uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE]);
    bool read_texture_cache_line_data(ImageInfo currentImage, uint64_t address[4], uint64_t data[TEXTURE_CACHE_QWORDS_PER_LINE]);
    bool read_image_info_cache_line(uint64_t address, ImageInfo &data);

    void compute_packed_mip_offset(ImageInfo currentImage, uint32_t bytesTexel, bool isCompressed,
                                   uint32_t tileWidthLog2, uint32_t tileHeightLog2, uint32_t mip_level, uint32_t mip_offset[]);
    uint64_t compute_tile_offset(uint32_t bytesTexel, uint32_t tile_i, uint32_t tile_j);

    struct CompressedFormatInfo
    {
        CompressedFormatInfo() : blockWidth(0), blockHeight(0), blockBytes(0), format(FORMAT_UNDEFINED) {};
        CompressedFormatInfo(uint32_t bw, uint32_t bh, uint32_t bb, ImageFormat fmt) : blockWidth(bw), blockHeight(bh), blockBytes(bb), format(fmt) {};

        uint32_t blockWidth;    // 2^n compressed block width
        uint32_t blockHeight;   // 2^n compressed block height
        uint32_t blockBytes;    // Compressed block size in bytes
        ImageFormat format;     // Decompressed format
    };

    void decode_BC1(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC2(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC3(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC4_UNORM(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC4_SNORM(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC5_UNORM(uint8_t *inBuffer, uint8_t *outBuffer);
    void decode_BC5_SNORM(uint8_t *inBuffer, uint8_t *outBuffer);

    void decode_BC4(uint8_t *inBuffer, uint8_t *outBuffer, bool signedFormat);
    void decode_BC5(uint8_t *inBuffer, uint8_t *outBuffer, bool signedFormat);

    uint32_t convertTo_R8G8B8A8_UNORM(float decodedColor[]);
    uint32_t convertTo_R8G8B8A8_SNORM(float decodedColor[]);

    void decode2BitRGB(uint32_t code, float RGB0[], float RGB1[], float output[]);
    void decode2BitRGBTransparent(uint32_t code, float RGB0[], float RGB1[], float output[]);
    float decode4BitComponent(uint32_t code, float color0, float color1, bool signedFormat);

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

    static const uint32_t BLOCK_SIZE = 4;
    static const uint32_t N_PARTITIONS = 64;
    static const uint32_t BLOCK_BYTES = 16;

    typedef struct
    {
       int  n_subsets;
       int  n_partition_bits;
       bool has_rotation_bits;
       bool has_index_selection_bit;
       int  n_color_bits;
       int  n_alpha_bits;
       bool has_endpoint_pbits;
       bool has_shared_pbits;
       int  n_index_bits;
       int  n_secondary_index_bits;
    } bptc_unorm_mode;

    typedef struct
    {
       int8_t  endpoint;
       uint8_t component;
       uint8_t offset;
       uint8_t n_bits;
       bool  reverse;
    } bptc_float_bitfield;

    typedef struct
    {
       bool                reserved;
       bool                transformed_endpoints;
       int                 n_partition_bits;
       int                 n_endpoint_bits;
       int                 n_index_bits;
       int                 n_delta_bits[3];
       bptc_float_bitfield bitfields[24];
    } bptc_float_mode;

    static const bptc_unorm_mode bptc_unorm_modes[];
    static const bptc_float_mode bptc_float_modes[];
    static const uint32_t partition_table1[N_PARTITIONS];
    static const uint32_t partition_table2[N_PARTITIONS];
    static const uint8_t anchor_indices[][N_PARTITIONS];

    void fetch_bptc_rgba_unorm_bytes(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, uint8_t *texel);
    void fetch_bptc_rgba_unorm(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel);
    void fetch_bptc_srgb_alpha_unorm(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel);
    void fetch_bptc_rgb_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel, bool is_signed);
    void fetch_bptc_rgb_signed_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel);
    void fetch_bptc_rgb_unsigned_float(const uint8_t *map, uint32_t rowStride, uint32_t i, uint32_t j, float *texel);

    int32_t finish_signed_unquantize(int32_t value);
    int32_t finish_unsigned_unquantize(int32_t value);
    int extract_float_endpoints(const bptc_float_mode *mode, const uint8_t *block, int bit_offset,
                                int32_t endpoints[][3], bool is_signed);
    int unsigned_unquantize(int value, int n_endpoint_bits);
    int signed_unquantize(int value, int n_endpoint_bits);
    int32_t sign_extend(int32_t value, int n_bits);
    void fetch_rgba_unorm_from_block(const uint8_t *block, uint8_t *result, int texel);
    void fetch_rgb_float_from_block(const uint8_t *block, float *result, int texel, bool is_signed);
    void apply_rotation(int rotation, uint8_t *result);
    int32_t interpolate(int32_t a, int32_t b, int index, int index_bits);
    int count_anchors_before_texel(int n_subsets, int partition_num, int texel);
    bool is_anchor(int n_subsets, int partition_num, int texel);
    int extract_unorm_endpoints(const bptc_unorm_mode *mode, const uint8_t *block, int bit_offset, uint8_t endpoints[][4]);
    uint8_t expand_component(uint8_t byte, int n_bits);
    int extract_bits(const uint8_t *block, int offset, int n_bits);

    uint16_t sharedexp_to_float16(uint32_t exponent, uint32_t mantissa);

    bool isCompressedFormat(ImageFormat format);
    CompressedFormatInfo getCompressedFormatInfo(ImageFormat format);
    CompressedFormatInfo getCompressedFormatInfoL1(ImageFormat format);

    void getCompressedTexel(ImageFormat format, uint32_t comprBlockI, uint32_t comprBlockJ, uint8_t data[]);

    bool isSRGBFormat(ImageFormat format);
    bool isFloat32Format(ImageFormat format);

    bool comparisonSupported(ImageFormat format);
    bool filterSupported(ImageFormat format);
};

#endif // _TBOX_EMU_H
