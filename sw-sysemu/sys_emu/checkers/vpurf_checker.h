#pragma once

#include <array>
#include <cstdint>

#include "emu_defines.h"
#include "processor.h"
#include "state.h"

class Vpurf_checker : public bemu::Agent
{
public:
    Vpurf_checker(bemu::System* chip) : bemu::Agent(chip) {}
    std::string name() const { return "VPURF-Checker"; }

    void pc_update(const bemu::Hart& cpu);
    void freg_write(const bemu::Hart& cpu, uint8_t fd);
    void freg_intmv(const bemu::Hart& cpu, uint8_t fd);
    void freg_load(const bemu::Hart& cpu, uint8_t fd);
    void freg_read(const bemu::Hart& cpu, uint8_t fs);

private:
    enum class Access_type {
        read = 0,
        load = 1,
        intmv = 2,
        write = 3,
        fmv_x0 = 4,
    };

    struct Access {
        uint64_t cycle;
        uint64_t pc;
        bemu::Instruction insn;
        Access_type type;

        bool is_8_cycle_write() const
        {
            return type == Access_type::intmv || type == Access_type::write;
        }

        bool fmv_x0_is_workaround() const
        {
            return type == Access_type::load || type == Access_type::write;
        }

        void clear() { type = Access_type::read; }
    };

    using Vpurf_model = std::array<Access, bemu::NFREGS>;

    std::array<Vpurf_model, EMU_NUM_THREADS> m_models;
};
