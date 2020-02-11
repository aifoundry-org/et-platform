/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_LOAD_H
#define BEMU_LOAD_H

#include <cstddef>
#include "main_memory.h"

namespace bemu {


void load_elf(MainMemory& mem, const char* filename);

void load_raw(MainMemory& mem, const char* filename, unsigned long long addr);


} // bemu

#endif // BEMU_LOAD_H
