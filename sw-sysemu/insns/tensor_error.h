#ifndef bemu_tensor_error_h
#define bemu_tensor_error_h

#include <cstdint>
#include "emu_gio.h"
#include "processor.h"

namespace bemu {


inline void update_tensor_error(Hart& cpu, uint16_t value)
{
    cpu.tensor_error |= value;
    if (value)
        LOG_HART(DEBUG, cpu, "\ttensor_error = 0x%04" PRIx16 " (0x%04" PRIx16 ")",
                 cpu.tensor_error, value);
}


} // namespace bemu

#endif // bemu_tensor_error_h
