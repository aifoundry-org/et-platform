#include "agent.h"

#include "system.h"

namespace bemu {

uint64_t Agent::emu_cycle() const noexcept
{
    return chip ? chip->emu_cycle() : 0;
}

}
