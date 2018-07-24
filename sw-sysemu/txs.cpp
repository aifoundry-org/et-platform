#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "txs.h"
#include "log.h"
#include "ipc.h"
#include "cvt.h"
#include "emu_gio.h"

using emu::gprintf;
using emu::gsprintf;
using emu::gfprintf;

extern int fake_sampler;
extern char dis[];

TBOXEmu tbox_emulator;

void init_txs(uint64_t imgTableAddr)
{
    DEBUG_EMU(gprintf("Setting Image Table Address = %016llx\n", imgTableAddr); )
 
    tbox_emulator.set_image_table_address(imgTableAddr);

    for (uint32_t t = 0; t < EMU_NUM_THREADS; t++)
        tbox_emulator.set_request_pending(t, false);

    tbox_emulator.texture_cache_initialize();
}

void texsndh(xreg src1, xreg src2, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndh x%d, x%d%s%s", src1, src2, (comm?" # ":""), (comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis););

    bool send_header = false;

    for (int e = 0; !send_header & (e < VL); e++)
        send_header = (MREGS[0].b[e] == 1);

    if (send_header)
    {
        tbox_emulator.set_request_header(current_thread, XREGS[src1].x, XREGS[src2].x);
        DEBUG_EMU(gprintf("\tSample request %016llx %016llx\n", XREGS[src1].x, XREGS[src2].x); )
        DEBUG_EMU(gprintf("\t"); tbox_emulator.print_sample_request(current_thread); )
        tbox_emulator.set_request_pending(current_thread, true);
    }

    IPC(ipc_texsnd(src1,src2,fnone,dis););
}

void texsnds(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsnds f%d%s%s", src1, (comm?" # ":""), (comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis););

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 0, FREGS[src1]);

        DEBUG_EMU( gprintf("\t Set *s* texture coordinates from f%d\n", src1); )
        for(uint32_t c = 0; c < VL; c++)
        {
            DEBUG_EMU( gprintf("\t[%d] 0x%08x (%f)\n", c, FREGS[src1].f[c], FREGS[src1].u[c]); )
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texsndt(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndt f%d%s%s", src1, (comm?" # ":""), (comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis););

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 1, FREGS[src1]);

        DEBUG_EMU( gprintf("\t Set *t* texture coordinates from f%d\n", src1); )
        for(uint32_t c = 0; c < VL; c++)
        {
            DEBUG_EMU( gprintf("\t[%d] 0x%08x (%f)\n", c, FREGS[src1].f[c], FREGS[src1].u[c]); )
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texsndr(freg src1, const char* comm)
{
    DISASM(gsprintf(dis,"I: texsndr f%d%s%s", src1, (comm?" # ":""), (comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis););

    bool send_coordinate = false;

    for (int e = 0; !send_coordinate & (e < VL); e++)
        send_coordinate = (MREGS[0].b[e] == 1);

    if (send_coordinate)
    {
        tbox_emulator.set_request_coordinates(current_thread, 2, FREGS[src1]);

        DEBUG_EMU( gprintf("\t Set *r* texture coordinates from f%d\n", src1); )
        for(uint32_t c = 0; c < VL; c++)
        {
            DEBUG_EMU( gprintf("\t[%d] 0x%08x (%f)\n", c, FREGS[src1].f[c], FREGS[src1].u[c]); )
        }
    }

    IPC(ipc_texsnd(xnone,xnone,src1,dis););
}

void texrcv(freg dst, const uint32_t idx, const char* comm)
{
    DISASM(gsprintf(dis,"I: texrcv f%d, 0x%x%s%s", dst, idx, (comm?" # ":""), (comm?comm:""));)
    DEBUG_EMU(gprintf("%s\n",dis););

    bool sample = false;

    if (tbox_emulator.check_request_pending(current_thread))
    {
        for (int e = 0; !sample & (e < VL); e++)
            sample = (MREGS[0].b[e] == 1); // if at least one frag alive then sample quad
        tbox_emulator.set_request_pending(current_thread, false);
    }

    if (sample)
        tbox_emulator.sample_quad(current_thread, fake_sampler, true);

    fdata data;

    data = tbox_emulator.get_request_results(current_thread, idx);

    for (uint32_t c = 0;  c < VL; c++)
    {
        // texrcv should pay attention to the mask!!!
        if ( MREGS[0].b[c] == 0 ) continue;

        FREGS[dst].f[c] = data.f[c];

        // Print as float16?
        float f32 = float16tofloat32(data.h[c * 2]);
        DEBUG_EMU(gprintf("\t[%d] 0x%04x (%f) FP16 <-\n", c, FREGS[dst].h[c*2], f32);)
        DEBUG_EMU(gprintf("\t[%d] 0x%08x (%f) FP32 <-\n", c, FREGS[dst].u[c], FREGS[dst].f[c]);)
    }

    logfregchange(dst);
    IPC(ipc_texrcv(dst,dis););
}

void checker_sample_quad(uint32_t thread, uint64_t basePtr, TBOXEmu::SampleRequest currentRequest_, fdata input[], fdata output[])
{
#ifdef CHECKER
    uint64_t base_copy = tbox_emulator.get_image_table_address();
    if ( base_copy != basePtr )
    {
        DEBUG_EMU(gprintf("WARNING!!! changing image table address from %llx to %llx by checker request.\n", base_copy, basePtr);)
        tbox_emulator.set_image_table_address(basePtr);
    }

    tbox_emulator.sample_quad(currentRequest_, input, output, fake_sampler);

    tbox_emulator.set_image_table_address(base_copy);
#else
    DEBUG_EMU(gprintf("ERROR!!! You need to compiler with CHECKER enabled to use checker_sample_quad().\n");)
    exit(-1);
#endif
}

void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[])
{
    tbox_emulator.decompress_texture_cache_line_data(currentImage, startTexel, inData, outData);
}
