#ifndef bemu_tensor_error_h
#define bemu_tensor_error_h

#include <cstdint>
#include "emu_defines.h"
#include "emu_gio.h"
#include "processor.h"

namespace bemu {


inline void update_tensor_error(unsigned thread, uint16_t value)
{
    extern std::array<Hart,EMU_NUM_THREADS> cpu;

    cpu[thread].tensor_error |= value;
    if (value)
        LOG_OTHER(DEBUG, thread,
                  "\ttensor_error = 0x%04" PRIx16 " (0x%04" PRIx16 ")",
                  cpu[thread].tensor_error, value);
}


inline void update_tensor_error(uint16_t value)
{
    extern unsigned current_thread;

    update_tensor_error(current_thread, value);
}


} // namespace bemu

#endif // bemu_tensor_error_h
