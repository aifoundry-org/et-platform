/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "txs.h"
#include "emu.h"
#include "emu_memop.h"
#include "tbox_emu.h"
#include "rbox.h"
#include "log.h"
#include "fpu/fpu.h"
#include "emu_gio.h"
#include "emu_casts.h"

static TBOX::TBOXEmu tbox_emulator;

void init_txs(uint64_t imgTableAddr)
{
    LOG(DEBUG, "Setting Image Table Address = %016" PRIx64, imgTableAddr);
 
    tbox_emulator.set_image_table_address(imgTableAddr);

    for (uint32_t t = 0; t < EMU_NUM_THREADS; t++)
        tbox_emulator.set_request_pending(t, false);

    tbox_emulator.texture_cache_initialize();
}

uint32_t tbox_id_from_thread(uint32_t current_thread)
{
    uint32_t neigh_id = current_thread / EMU_THREADS_PER_NEIGH;

    if ((EMU_TBOXES_PER_SHIRE == 4) ||
        (EMU_TBOXES_PER_SHIRE == 2) ||
        (EMU_TBOXES_PER_SHIRE == 1))
    {
        if (neigh_id > EMU_TBOXES_PER_SHIRE)
        {
            LOG(FTL, "Neighborhood %d has no TBOX configured", neigh_id);
            return 0;
        }
        else
        {
            return neigh_id;
        }
    }
    else
    {
        LOG(FTL, "Unsupported number of TBOXes per Shire %d", EMU_TBOXES_PER_SHIRE);
        return 0;
    }
}


static char coord_name[5]="strq";

/*
    Adds a sample_request in TBOX and execute it
*/
void new_sample_request(uint32_t current_thread, uint32_t port_id, uint32_t number_packets, uint64_t base_address)
{
    uint32_t shire_id = current_thread / EMU_THREADS_PER_SHIRE;
    uint32_t tbox_id = tbox_id_from_thread(current_thread);

    LOG(DEBUG, "\tSample Request for TBOX %d Packets = %u, Port_id = %u, Hart_id = %u, Port Base Address = %" PRIx64,
        tbox_id, number_packets, port_id, current_thread, base_address);

    uint64_t val[12];
    
    /* Get data from port and send it to TBOX */
    for(unsigned i=0; i<number_packets*2; i++)
    {
        val[i] = vmemread64(base_address);
        base_address+=8; // 8 bytes
    }    
    
    GET_TBOX(shire_id, tbox_id).set_request_pending(current_thread, true);

    // Set header
    TBOX::SampleRequest header;
    memcpy(&header, val, sizeof(TBOX::SampleRequest));
    GET_TBOX(shire_id, tbox_id).set_request_header(current_thread, header);
    LOG(DEBUG, "\tSample request header %016" PRIx64 " %016" PRIx64, header.data[0], header.data[1]);

    // Parse header and send coordinates
    for(unsigned char i = 0; i < header.info.packets; i++)
    {
        fdata coordinates;
        memcpy(&coordinates, &(val[((i+1)<<2)]), sizeof(fdata));
        tbox[shire_id][tbox_id].set_request_coordinates(current_thread, i, coordinates);
        LOG(DEBUG, "\t Set *%c* texture coordinates", coord_name[i]);
        for(uint32_t c = 0; c < VL_TBOX; c++)
        {
            LOG(DEBUG, "\t[%d] 0x%08x (%f)", c, coordinates.u[c], cast_uint32_to_float(coordinates.u[c]));
        }
    }

    GET_TBOX(shire_id, tbox_id).set_request_pending(current_thread, false);

    /* Compute request */
    GET_TBOX(shire_id, tbox_id).sample_quad(current_thread, true); // Performs Sample Request

    /* Get result */
    fdata data[4]; // Space for 4 channels

    unsigned num_channels = GET_TBOX(shire_id, tbox_id).get_request_results(current_thread, data);
    
    for (uint32_t channel = 0; channel < num_channels; channel++)
    {
        LOG(DEBUG, "\t[Channel %d] 0x%08x 0x%08x 0x%08x 0x%08x <-", channel, data[channel].u[0], data[channel].u[1], data[channel].u[2], data[channel].u[3]);
        /* Put result in port */
        write_msg_port_data_from_tbox(current_thread, port_id, shire_id * EMU_TBOXES_PER_SHIRE + tbox_id, &(data[channel].u[0]), 1);
    }
}

void checker_sample_quad(uint32_t thread __attribute__((unused)), uint64_t basePtr,
                         TBOX::SampleRequest currentRequest_, fdata input[], fdata output[])
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

void decompress_texture_cache_line_data(TBOX::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[])
{
    tbox_emulator.decompress_texture_cache_line_data(currentImage, startTexel, inData, outData);
}

