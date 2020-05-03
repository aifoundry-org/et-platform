/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _TXS_H
#define _TXS_H

#include "state.h"
#include "tbox_pi.h"

namespace bemu {


//#define TEXTURE_CACHE
extern void init_txs(uint64_t imgTableAddr);

extern uint32_t tbox_id_from_thread(uint32_t thread);

extern void new_sample_request(uint32_t thread, uint32_t port_id, uint32_t number_packets, uint64_t base_address);

extern void checker_sample_quad(uint32_t thread, uint64_t basePtr, TBOX::SampleRequest currentRequest, freg_t input[], freg_t output[]);

extern void decompress_texture_cache_line_data(TBOX::ImageInfo currentImage, uint32_t startTexel, uint64_t inData[], uint64_t outData[]);


} // namespace bemu

#endif // _TXS_H
