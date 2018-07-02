#ifndef _TXS_H
#define _TXS_H

#include "emu_defines.h"
#include "tbox_emu.h"

//#define TEXTURE_CACHE

extern "C" void init_txs(uint64_t imgTableAddr);
extern "C" void texsndh(xreg src1, xreg src2);
extern "C" void texsnds(freg src1);
extern "C" void texsndt(freg src1);
extern "C" void texsndr(freg src1);
extern "C" void texrcv(freg dst, const uint32_t imm);

#ifdef CHECKER
extern "C" void checker_sample_quad(uint32_t thread, uint64_t basePtr, TBOXEmu::SampleRequest currentRequest, fdata input[], fdata output[]);
#endif

extern "C" void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[]);

#endif // _TXS_H
