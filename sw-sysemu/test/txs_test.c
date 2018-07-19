#include <cstdio>
#include <txs.h>
#include <cvt.h>
#include <emu.h>

#define min(a, b) (((a) <= (b)) ? (a) : (b))
#define max(a, b) (((a) >= (b)) ? (a) : (b))

uint32_t ceil_log2(uint32_t in)
{
    uint32_t mask = 0x80000000;
    for (uint32_t b = 0; b < 32; b++)
        if (in & mask)
            return (31 - b) + (((in & ~mask) == 0) ? 0 : 1);
        else
            mask = mask >> 1;
}

void compute_image_mip_pitch(uint32_t width, uint32_t height, uint32_t mip_count, uint32_t texel_size, uint32_t &mip_pitch_l0, uint32_t &mip_pitch_l1)
{
    mip_pitch_l0 = ceil_log2(width * height * texel_size) - 6;
    mip_pitch_l1 = ceil_log2((width >> 1) * (height >> 1) * texel_size) - 6;
}

void create_mipmap(uint32_t *mipmap, uint32_t width, uint32_t height, uint32_t mip_count, bool tiled)
{
    // Mip Pitch in 64 byte blocks
    uint32_t mip_pitch_l0, mip_pitch_l1;
    compute_image_mip_pitch(width, height, mip_count, 4, mip_pitch_l0, mip_pitch_l1);

    printf("Mip Pitch L0 = %d  Mip Pitch L1 = %d\n", mip_pitch_l0, mip_pitch_l1);
    fflush(stdout);
    
    // Row Pitch in 64 byte blocks
    uint64_t row_pitch = tiled ? (width >> (4 - 2))
                             : (width >> (6 - 2));
    
    for (uint32_t l = 0; l < mip_count; l++)
    {
        uint32_t mip_offset = TBOXEmu::compute_mip_offset(mip_pitch_l0, mip_pitch_l1, row_pitch, height, l); 
        printf("Filling mip level %d mip_offset = %d\n", l, mip_offset);
        fflush(stdout);
        for (uint32_t y = 0; y < (height >> l); y++)
        {
            uint32_t row_offset;
            if (tiled)
                row_offset = (y >> 2) * max(1, row_pitch >> l);
            else
                row_offset = y * max(1, row_pitch >> l);
            //printf("Filling row %d row_offset = %d\n", y, row_offset);
            for(uint32_t x = 0; x < (width >> l); x++)
            {
                uint32_t texel_offset = (mip_offset >> 2) + row_offset * 16;
                if (tiled)
                    texel_offset += (x >> 2) * 16 + ((y & 0x03) << 2) + (x & 0x03);
                else
                    texel_offset += x;
                //printf("Filling texel (%d, %d) at offset = %d\n", x, y, texel_offset);
                fflush(stdout);
                mipmap[texel_offset] = (x & 0xFF) | ((y & 0xFF) << 8) | 0xFF800000;
            }
        }   
    }
    
    //for(uint32_t t = 0; t < 64; t++)
    //    printf("%08x ", mipmap[t]);
    //printf("\n");
}

int main()
{
    TBOXEmu::ImageInfo imageTable;
    TBOXEmu::SampleRequest sampleRequest;
    
    bool tiled = false;

    TBOXEmu::ImageInfo testImage;
    testImage.data[0] = 0;
    testImage.data[1] = 0x0000000ff00ff299;
    testImage.data[2] = 0x0000000020000000;
    testImage.data[3] = 0x01000000014c0040;
    
    TBOXEmu::print_image_info(testImage);
    
    // Tile and create mipmap.
    uint32_t mipmap[256 * 256 * 2];
    create_mipmap(mipmap, 256, 256, 9, tiled);
    
    imageTable.data[0] = 
    imageTable.data[1] = 
    imageTable.data[2] = 
    imageTable.data[3] = 0;
    
    imageTable.info.address = (uint64_t) mipmap;
    imageTable.info.type = TBOXEmu::IMAGE_TYPE_2D;
    imageTable.info.format = TBOXEmu::FORMAT_R8G8B8A8_UNORM;
    imageTable.info.width = 255;
    imageTable.info.height = 255;
    imageTable.info.mipcount = 9;
    imageTable.info.tiled = tiled;
    imageTable.info.rowpitch = tiled ? 64 : 16;

    uint32_t mip_pitch_l0, mip_pitch_l1;
    compute_image_mip_pitch(256, 256, 9, 4, mip_pitch_l0, mip_pitch_l1);
    imageTable.info.mippitchl0 = mip_pitch_l0;
    imageTable.info.mippitchl1 = mip_pitch_l1;
    
    imageTable.info.elementpitch = 0;
    
    sampleRequest.data[0] =
    sampleRequest.data[1] = 0;

    sampleRequest.info.operation = TBOXEmu::SAMPLE_OP_SAMPLE_L;
    sampleRequest.info.imageid = 0;
    sampleRequest.info.minfilter = TBOXEmu::FILTER_TYPE_LINEAR;
    sampleRequest.info.magfilter = TBOXEmu::FILTER_TYPE_LINEAR;
    sampleRequest.info.mipfilter = TBOXEmu::FILTER_TYPE_NEAREST;
    sampleRequest.info.aniso = 0;
    sampleRequest.info.addrmodeu = TBOXEmu::ADDRESS_MODE_CLAMP_TO_EDGE;
    sampleRequest.info.addrmodev = TBOXEmu::ADDRESS_MODE_CLAMP_TO_EDGE;
    sampleRequest.info.addrmodew = TBOXEmu::ADDRESS_MODE_CLAMP_TO_EDGE;
    sampleRequest.info.lodaniso.lod_array[0] = float32tofloat16(0.0);
    sampleRequest.info.lodaniso.lod_array[1] = float32tofloat16(4.0);
    sampleRequest.info.lodaniso.lod_array[2] = float32tofloat16(6.0);
    sampleRequest.info.lodaniso.lod_array[3] = float32tofloat16(8.0);
     
    TBOXEmu::print_sample_request(sampleRequest);

    float32 texcoords[] =
    {
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0
    };

    set_thread(0);  
    init_txs((uint64_t) &imageTable);
    init(x3, (uint64_t) sampleRequest.data[0]);
    init(x4, (uint64_t) sampleRequest.data[1]);
    init(x5, (uint64_t) texcoords);
    
    flw_ps(f1,  0, x5, "# Load texture coordinates");
    flw_ps(f2, 16, x5, "# Load texture coordinates");

    texsndh(x3, x4, "# Send sample request");
    texsnds(f1, "# Send coordinates");
    texsndt(f2, "# Send coordinates");
    texrcv(f8,  0, "# Receive result");
    texrcv(f9,  1, "# Receive result");
    texrcv(f10, 2, "# Receive result");
    texrcv(f11, 3, "# Receive result");
    
    float32 texcoords2[] =
    {
        0.50, 0.50, 0.50, 0.50,
        0.25, 0.25, 0.25, 0.25
    };
    
    init(x5, (uint64_t) texcoords2);

    flw_ps(f1,  0, x5, "# Load texture coordinates");
    flw_ps(f2, 16, x5, "# Load texture coordinates");


    texsndh(x3, x4, "# Send sample request");
    texsnds(f1, "# Send coordinates");
    texsndt(f2, "# Send coordinates");
    texrcv(f8,  0, "# Receive result");
    texrcv(f9,  1, "# Receive result");
    texrcv(f10, 2, "# Receive result");
    texrcv(f11, 3, "# Receive result");
}

