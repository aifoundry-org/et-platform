#include "log.h"

#include "sys_emu.h"

void notify_freg_load(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&,
                      const bemu::freg_t&)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_load(cpu, fd);
    }
}

void notify_freg_intmv(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&,
                       const bemu::freg_t&)
{
    auto emu = cpu.chip->emu();
    if (emu->get_vpurf_check()) {
        emu->get_vpurf_checker().freg_intmv(cpu, fd);
    }
}

void notify_freg_write(const bemu::Hart& cpu, uint8_t fd, const bemu::mreg_t&,
                       const bemu::freg_t&)
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
