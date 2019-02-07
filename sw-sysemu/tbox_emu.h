#ifndef _TBOX_EMU_H
#define _TBOX_EMU_H

#include "emu_defines.h"

#include "tbox_pi.h"

namespace TBOX
{

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
    
        typedef struct
        {
            uint32_t thread_mask;
            uint64_t address;                     // Aligned to request size
            uint64_t data[L2_REQUEST_QWORDS];
            bool     ready;
            bool     free;
        } L2Request;
    
        void set_request_header(uint32_t thread, uint64_t src1, uint64_t src2); // Obsolete method
        void set_request_header(uint32_t thread, SampleRequest header);
    
        void set_request_coordinates(uint32_t thread, uint32_t index, fdata coord);
        fdata get_request_results(uint32_t thread, uint32_t index);
        unsigned get_request_results(uint32_t thread, fdata* data);
        void set_request_pending(uint32_t thread, bool v);
        bool check_request_pending(uint32_t thread);
        void set_image_table_address(uint64_t addr);
        uint64_t get_image_table_address();
        void print_sample_request(uint32_t thread) const;
    
        void texture_cache_initialize();
        void image_info_cache_initialize();
    
        void sample_quad(uint32_t thread, bool output_result);
        bool get_image_info(uint32_t thread, ImageInfo &currentImage);
        bool get_image_info(SampleRequest request, ImageInfo &currentImage);
        void sample_quad(SampleRequest currentRequest, fdata input[], fdata output[]);
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
    
        uint32_t request_hart;
    
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
                          uint32_t quad, uint32_t pixel,
                          ImageInfo currentImage, FilterType filter, uint32_t mip_level,
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
    
        static inline uint16_t cast_bytes_to_uint16(uint8_t *src);
        static inline uint32_t cast_bytes_to_uint24(uint8_t *src);
        static inline uint32_t cast_bytes_to_uint32(uint8_t *src);
        static inline uint64_t cast_bytes_to_uint64(uint8_t *src);
        static inline float cast_bytes_to_float(uint8_t *src);
        static inline void memcpy_uint16(uint8_t *dst, uint16_t src);
        static inline void memcpy_uint32(uint8_t *dst, uint32_t src);
        static inline void memcpy_uint64(uint8_t *dst, uint64_t src);
    
    };  // Class TBOXEmu

}   // Namespace TBOX

#endif // _TBOX_EMU_H
