#ifndef _MAIN_MEMORY_REGION_IO_
#define _MAIN_MEMORY_REGION_IO_

#include "main_memory_region.h"
#ifdef SYS_EMU
#include "emu_defines.h"
#include "traps.h"
#include "../sys_emu/sys_emu.h"
#endif

#define IO_R_PU_TIMER_REG_MTIME    0x0012005000ULL
#define IO_R_PU_TIMER_REG_MTIMECMP 0x0012005008ULL

// Memory region used to implement the access to the IO region

struct main_memory_region_io : public main_memory_region
{
    // Constructors and destructors
    main_memory_region_io(uint64_t base, uint64_t size)
        : main_memory_region(base, size, false)
    {}

    ~main_memory_region_io() {}

    // read and write
    void write(uint64_t addr, size_t n, const void* source) override {
#ifdef SYS_EMU
        uint64_t value = *reinterpret_cast<const uint64_t*>(source);
        switch (addr) {
            case IO_R_PU_TIMER_REG_MTIME:
                if (n != 8) throw trap_bus_error(addr);
                sys_emu::get_pu_rvtimer().write_mtime(value);
                break;
            case IO_R_PU_TIMER_REG_MTIMECMP:
                if (n != 8) throw trap_bus_error(addr);
                sys_emu::get_pu_rvtimer().write_mtimecmp(value);
                break;
            default:
                throw trap_bus_error(addr);
        }
#else
        (void) addr;
        (void) n;
        (void) source;
#endif
    }

    void read(uint64_t addr, size_t n, void* result) override {
#ifdef SYS_EMU
        uint64_t* ptr = reinterpret_cast<uint64_t*>(result);
        switch (addr) {
            case IO_R_PU_TIMER_REG_MTIME:
                if (n != 8) throw trap_bus_error(addr);
                *ptr = sys_emu::get_pu_rvtimer().read_mtime();
                break;
            case IO_R_PU_TIMER_REG_MTIMECMP:
                if (n != 8) throw trap_bus_error(addr);
                *ptr = sys_emu::get_pu_rvtimer().read_mtimecmp();
                break;
            default:
                throw trap_bus_error(addr);
        }
#else
        (void) addr;
        (void) n;
        (void) result;
#endif
    }
};

#endif // _MAIN_MEMORY_REGION_IO_

