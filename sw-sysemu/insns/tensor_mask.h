#ifndef bemu_tensor_mask_h
#define bemu_tensor_mask_h

#include <cstdint>
#include "emu_defines.h"
#include "processor.h"

namespace bemu {


inline bool tmask_pass(int bit)
{
    extern std::array<Hart,EMU_NUM_THREADS> cpu;
    extern unsigned current_thread;

    // Returns the pass bit for a specific bit
    return (cpu[current_thread].tensor_mask >> bit) & 1;
}


} // namespace bemu

#endif // bemu_tensor_mask_h
