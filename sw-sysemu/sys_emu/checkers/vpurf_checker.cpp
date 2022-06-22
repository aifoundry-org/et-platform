#include "vpurf_checker.h"

#include "emu_gio.h"
#include "system.h"

namespace
{
inline bool is_fmv_x0(uint32_t bits)
{
    static constexpr uint32_t mask = 0xfff07fff;
    static constexpr uint32_t enc = 0xe0000053;
    return (bits & mask) == enc;
}
} // namespace

void Vpurf_checker::freg_write(const bemu::Hart& cpu, uint8_t fd)
{
    const uint64_t cycle = cpu.chip->emu_cycle();
    const unsigned hid = bemu::hart_index(cpu);

    // TODO check for fclass.ps

    auto& access = m_models[hid][fd];
    access.cycle = cycle;
    access.pc = cpu.pc;
    access.insn = cpu.inst;
    access.type = Access_type::write;
}

void Vpurf_checker::freg_intmv(const bemu::Hart& cpu, uint8_t fd)
{
    const uint64_t cycle = cpu.chip->emu_cycle();
    const unsigned hid = bemu::hart_index(cpu);

    auto& access = m_models[hid][fd];
    access.cycle = cycle;
    access.pc = cpu.pc;
    access.insn = cpu.inst;
    access.type = Access_type::intmv;
}

void Vpurf_checker::freg_load(const bemu::Hart& cpu, uint8_t fd)
{
    const uint64_t cycle = cpu.chip->emu_cycle();
    const unsigned hid = bemu::hart_index(cpu);

    auto& access = m_models[hid][fd];
    access.cycle = cycle;
    access.pc = cpu.pc;
    access.insn = cpu.inst;
    access.type = Access_type::load;
}

void Vpurf_checker::freg_read(const bemu::Hart& cpu, uint8_t fs)
{
    const uint64_t cycle = cpu.chip->emu_cycle();
    const unsigned hid = bemu::hart_index(cpu);

    auto& access = m_models[hid][fs];

    const bool fmv_x0_is_workaround
        = access.type == Access_type::load || access.type == Access_type::write;

    const bool is_8_cycle_write = access.type == Access_type::write
                                  || access.type == Access_type::intmv;

    const uint64_t cycles_since_access = cycle - access.cycle;

    // No outstanding write to register
    if (access.type == Access_type::read) return;

    // Register fmv.x.w x0,fs for workarounds
    if (fmv_x0_is_workaround && is_fmv_x0(cpu.inst.bits)) {
        access.cycle = cycle;
        access.pc = cpu.pc;
        access.insn = cpu.inst;
        access.type = Access_type::fmv_x0;
        return;
    }

    // Handle B/C workarounds with NOPs
    if (is_8_cycle_write && cycles_since_access >= 8) {
        access.type = Access_type::read;
        return;
    }

    // Handle A/C workarounds with fmv.x.w x0,fs
    if (access.type == Access_type::fmv_x0) {
        if (cycles_since_access > 1) {
            access.type = Access_type::read;
            return;
        } else {
            LOG_AGENT(INFO, *this,
                      "[%s] 0x%" PRIx64 " (0x%08" PRIx32
                      ") fmv.x.w x0,f%d at cycle %" PRIu64,
                      cpu.name().c_str(), access.pc, access.insn.bits, fs,
                      access.cycle);
            LOG_AGENT(INFO, *this,
                      "[%s] 0x%" PRIx64 " (0x%08" PRIx32 ") reads f%d",
                      cpu.name().c_str(), cpu.pc, cpu.inst.bits, fs);
            LOG_AGENT(
                FTL, *this,
                "[%s] Missing NOP between VPURF workaround and actual read",
                cpu.name().c_str());
        }
    }

    // If we got here it means no workaround applies
    const char wa_type = [](Access_type acc) {
        switch (acc) {
        case Access_type::load:
            return 'A';
        case Access_type::intmv:
            return 'B';
        case Access_type::write:
            return 'C';
        default:
            return '?';
        }
    }(access.type);

    LOG_AGENT(
        INFO, *this,
        "[%s] 0x%" PRIx64 " (0x%08" PRIx32 ") writes f%d at cycle %" PRIu64,
        cpu.name().c_str(), access.pc, access.insn.bits, fs, access.cycle);
    LOG_AGENT(INFO, *this, "[%s] 0x%" PRIx64 " (0x%08" PRIx32 ") reads f%d",
              cpu.name().c_str(), cpu.pc, cpu.inst.bits, fs);
    LOG_AGENT(FTL, *this, "[%s] VPURF workaround required for type %c",
              cpu.name().c_str(), wa_type);
}
