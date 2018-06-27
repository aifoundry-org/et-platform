#ifndef _TXS_H
#define _TXS_H

#include "emu_defines.h"
#include "tbox_emu.h"

//#define TEXTURE_CACHE

extern "C" void init_txs(uint64 imgTableAddr);
extern "C" void texsndh(xreg src1, xreg src2);
extern "C" void texsnds(freg src1);
extern "C" void texsndt(freg src1);
extern "C" void texsndr(freg src1);
extern "C" void texrcv(freg dst, const uint32 imm);

#ifdef CHECKER
extern "C" void checker_sample_quad(uint32 thread, uint64 basePtr, TBOXEmu::SampleRequest currentRequest, fdata input[], fdata output[]);
#endif

extern "C" void decompress_texture_cache_line_data(TBOXEmu::ImageInfo currentImage, uint32 startTexel, uint64 inData[], uint64 outData[]);

#endif // _TXS_H
