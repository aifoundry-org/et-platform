/* -*- Mode:C++; c-basic-offset: 4; -*- */

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "txs.h"
#include "emu.h"
#include "log.h"
#include "ipc.h"
#include "fpu.h"
#include "emu_gio.h"
#include "emu_casts.h"

using emu::gprintf;
using emu::gsprintf;
using emu::gfprintf;

extern int fake_sampler;
extern char dis[];

static TBOXEmu tbox_emulator;

void init_txs(uint64_t imgTableAddr)
{
    LOG(DEBUG, "Setting Image Table Address = %016llx", imgTableAddr);
 
    tbox_emulator.set_image_table_address(imgTableAddr);

    for (uint32_t t = 0; t < EMU_NUM_THREADS; t++)
        tbox_emulator.set_request_pending(t, false);

    tbox_emulator.texture_cache_initialize();
}

void texsndh(xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndh x%d, x%d%s%s", src1, src2, (comm?" # ":""), (comm?comm:"")););
    LOG(DEBUG, "%s",dis);

    bool send_header = false;

    for (int e = 0; !send_header & (e < VL_TBOX); e++)
        send_header = (MREGS[0].b[e] == 1);

    if (send_header)
    {
        tbox_emulator.set_request_header(current_thread, XREGS[src1].x, XREGS[src2].x);
        LOG(DEBUG, "\tSample request %016llx %016llx", XREGS[src1].x, XREGS[src2].x);
        tbox_emulator.set_request_pending(current_thread, true);
    }

    IPC(ipc_texsnd(src1,src2,fnone,dis););
}

void texsnds(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsnds f%d%s%s", src1, (comm?" # ":""), (comm?comm:"")););
    LOG(DEBUG, "%s",dis);

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL_TBOX); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 0, FREGS[src1]);

        LOG(DEBUG, "\t Set *s* texture coordinates from f%d", src1);
        for(uint32_t c = 0; c < VL_TBOX; c++)
        {
            LOG(DEBUG, "\t[%d] 0x%08x (%f)", c, FREGS[src1].u[c], cast_uint32_to_float(FREGS[src1].u[c]));
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texsndt(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndt f%d%s%s", src1, (comm?" # ":""), (comm?comm:"")););
    LOG(DEBUG, "%s",dis);

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL_TBOX); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 1, FREGS[src1]);

        LOG(DEBUG, "\t Set *t* texture coordinates from f%d", src1);
        for(uint32_t c = 0; c < VL_TBOX; c++)
        {
            LOG(DEBUG, "\t[%d] 0x%08x (%f)", c, FREGS[src1].u[c], cast_uint32_to_float(FREGS[src1].u[c]));
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texsndr(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndr f%d%s%s", src1, (comm?" # ":""), (comm?comm:"")););
    LOG(DEBUG, "%s",dis);

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL_TBOX); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 2, FREGS[src1]);

        LOG(DEBUG, "\t Set *r* texture coordinates from f%d", src1);
        for(uint32_t c = 0; c < VL_TBOX; c++)
        {
            LOG(DEBUG, "\t[%d] 0x%08x (%f)", c, FREGS[src1].u[c], cast_uint32_to_float(FREGS[src1].u[c]));
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texrcv(freg dst, const uint32_t idx, const char* comm)
{
    DISASM(gsprintf(dis,"I: texrcv f%d, 0x%x%s%s", dst, idx, (comm?" # ":""), (comm?comm:"")););
    LOG(DEBUG, "%s",dis);

    bool sample = false;

    if (tbox_emulator.check_request_pending(current_thread))
    {
        for (int e = 0; !sample & (e < VL_TBOX); e++)
            sample = (MREGS[0].b[e] == 1); // if at least one frag alive then sample quad
        tbox_emulator.set_request_pending(current_thread, false);
    }

    if (sample)
        tbox_emulator.sample_quad(current_thread, fake_sampler, true);

    fdata data;

    data = tbox_emulator.get_request_results(current_thread, idx);

    for (uint32_t c = 0; c < VL_TBOX; c++)
    {
        // texrcv should pay attention to the mask!!!
        if ( MREGS[0].b[c] == 0 ) continue;

        FREGS[dst].u[c] = data.u[c];

        // Print as float16?
        iufval32 tmp;
        tmp.f = fpu::f16_to_f32(cast_uint16_to_float16(data.h[c * 2]));
        LOG(DEBUG, "\t[%d] 0x%04x (%g) FP16 <-", c, FREGS[dst].h[c*2], cast_uint32_to_float(tmp.u));
        LOG(DEBUG, "\t[%d] 0x%08x (%g) FP32 <-", c, FREGS[dst].u[c], cast_uint32_to_float(FREGS[dst].u[c]));
    }

    logfregchange(dst);
    IPC(ipc_texrcv(dst,dis););
}

void checker_sample_quad(uint32_t thread, uint64_t basePtr, TBOXEmu::SampleRequest currentRequest_, fdata input[], fdata output[])
{
    uint64_t base_copy = tbox_emulator.get_image_table_address();
    if ( base_copy != basePtr )
    {
        LOG(WARN, "WARNING!!! changing image table address from %llx to %llx by checker request.", base_copy, basePtr);
        tbox_emulator.set_image_table_address(basePtr);
    }

    tbox_emulator.sample_quad(currentRequest_, input, output, fake_sampler);

    tbox_emulator.set_image_table_address(base_copy);
}

void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[])
{
    tbox_emulator.decompress_texture_cache_line_data(currentImage, startTexel, inData, outData);
}
