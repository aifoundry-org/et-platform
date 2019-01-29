/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "txs.h"
#include "emu.h"
#include "emu_memop.h"
#include "log.h"
#include "fpu/fpu.h"
#include "emu_gio.h"
#include "emu_casts.h"

static TBOXEmu tbox_emulator;

void init_txs(uint64_t imgTableAddr)
{
    LOG(DEBUG, "Setting Image Table Address = %016" PRIx64, imgTableAddr);
 
    tbox_emulator.set_image_table_address(imgTableAddr);

    for (uint32_t t = 0; t < EMU_NUM_THREADS; t++)
        tbox_emulator.set_request_pending(t, false);

    tbox_emulator.texture_cache_initialize();
}

static char coord_name[5]="strq";

/*
    Adds a sample_request in TBOX and execute it
*/
void new_sample_request(unsigned port_id, unsigned number_packets, uint64_t base_address)
{

    LOG(DEBUG, "\tSample Request. Packets = %u, Port_id = %u, Hart_id = %u, Port Base Address = %" PRIx64, number_packets, port_id, current_thread, base_address);

    uint64_t val[12];
    
    /* Get data from port and send it to TBOX */
    for(unsigned i=0; i<number_packets*2; i++)
    {
        val[i] = vmemread64(base_address);
        base_address+=8; // 8 bytes
    }    
    
    tbox_emulator.set_request_pending(current_thread, true);

    // Set header
    TBOXEmu::SampleRequest header;
    memcpy(&header, val, sizeof(TBOXEmu::SampleRequest));
    tbox_emulator.set_request_header(current_thread, header);
    LOG(DEBUG, "\tSample request header %016" PRIx64 " %016" PRIx64, header.data[0], header.data[1]);

    // Parse header and send coordinates
    for(unsigned char i = 0; i < header.info.packets; i++)
    {
        fdata coordinates;
        memcpy(&coordinates, &(val[((i+1)<<2)]), sizeof(fdata));
        tbox_emulator.set_request_coordinates(current_thread, i, coordinates);
        LOG(DEBUG, "\t Set *%c* texture coordinates", coord_name[i]);
        for(uint32_t c = 0; c < VL_TBOX; c++)
        {
            LOG(DEBUG, "\t[%d] 0x%08x (%f)", c, coordinates.u[c], cast_uint32_to_float(coordinates.u[c]));
        }
    }

    tbox_emulator.set_request_pending(current_thread, false);

    /* Compute request */
    tbox_emulator.sample_quad(current_thread, true); // Performs Sample Request

    /* Get result */
    fdata data[4]; // Space for 4 channels

    unsigned num_channels = tbox_emulator.get_request_results(current_thread, data);
    
    for (uint32_t channel = 0; channel < num_channels; channel++)
    {
        LOG(DEBUG, "\t[Channel %d] 0x%08x 0x%08x 0x%08x 0x%08x <-", channel, data[channel].u[0], data[channel].u[1], data[channel].u[2], data[channel].u[3]);
        /* Put result in port */
        write_msg_port_data_from_tbox(current_thread, port_id, current_thread / EMU_TBOXES_PER_SHIRE, &(data[channel].u[0]), 1);
    }    
}

void checker_sample_quad(uint32_t thread __attribute__((unused)), uint64_t basePtr,
                         TBOXEmu::SampleRequest currentRequest_, fdata input[], fdata output[])
{
    uint64_t base_copy = tbox_emulator.get_image_table_address();
    if ( base_copy != basePtr )
    {
        LOG(WARN, "WARNING!!! changing image table address from %" PRIx64 " to %" PRIx64 " by checker request.", base_copy, basePtr);
        tbox_emulator.set_image_table_address(basePtr);
    }

    tbox_emulator.sample_quad(currentRequest_, input, output);

    tbox_emulator.set_image_table_address(base_copy);
}

void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[])
{
    tbox_emulator.decompress_texture_cache_line_data(currentImage, startTexel, inData, outData);
}
