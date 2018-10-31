#ifndef _TXS_H
#define _TXS_H

#include "emu_defines.h"
#include "tbox_emu.h"

//#define TEXTURE_CACHE

extern void init_txs(uint64_t imgTableAddr);

extern void new_sample_request(unsigned port_id, unsigned number_packets, uint64_t base_address);

// FIXME: THESE INSTRUCTIONS ARE OBSOLETE
extern void texsndh(xreg src1, xreg src2, const char* comm = 0);
extern void texsnds(freg src1, const char* comm = 0);
extern void texsndt(freg src1, const char* comm = 0);
extern void texsndr(freg src1, const char* comm = 0);
extern void texrcv(freg dst, const uint32_t imm, const char* comm = 0);

extern void checker_sample_quad(uint32_t thread, uint64_t basePtr, TBOXEmu::SampleRequest currentRequest, fdata input[], fdata output[]);
extern void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[]);

#endif // _TXS_H
