/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_LOAD_H
#define BEMU_LOAD_H

#include <cstddef>
#include "main_memory.h"

namespace bemu {


void load_elf(MainMemory& mem, const char* filename);

void load_raw(MainMemory& mem, const char* filename, unsigned long long addr);


} // bemu

#endif // BEMU_LOAD_H
