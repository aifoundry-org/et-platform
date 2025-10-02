#include "log.h"

#include "sys_emu.h"

#ifdef SDK_RELEASE

void notify_pc_update(const bemu::Hart&, uint64_t) { }
void notify_freg_load(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) { }
void notify_freg_intmv(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) { }
void notify_freg_write(const bemu::Hart&, uint8_t, const bemu::mreg_t&, const bemu::freg_t&) { }
void notify_freg_read(const bemu::Hart&, uint8_t) { }

#else // !SDK_RELEASE

void notify_pc_update(const bemu::Hart& cpu, uint64_t)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().pc_update(cpu);
    }
}


void notify_freg_load(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&, const bemu::freg_t&)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_load(cpu, fd);
    }
}


void notify_freg_intmv(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&, const bemu::freg_t&)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_intmv(cpu, fd);
    }
}


void notify_freg_write(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&, const bemu::freg_t&)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_write(cpu, fd);
    }
}


void notify_freg_read(const bemu::Hart& cpu, uint8_t fs)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_read(cpu, fs);
    }
}

#endif // SDK_RELEASE
