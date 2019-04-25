/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include "decode.h"
#include "emu_gio.h"
#include "insn.h"
#include "insn_func.h"
#include "log.h"
#include "utility.h"

// FIXME: Replace with "state.h"
#include "emu_defines.h"
extern uint64_t xregs[EMU_NUM_THREADS][NXREGS];
extern uint8_t csr_prv[EMU_NUM_THREADS];

//namespace bemu {


static inline uint_fast16_t bitmixb(uint_fast16_t sel, uint_fast16_t val0, uint_fast16_t val1)
{
    uint_fast16_t val = 0;
    for (unsigned pos = 0; pos < 16; ++pos) {
        if (sel & 1) {
            val |= ((val1 & 1) << pos);
            val1 >>= 1;
        } else {
            val |= ((val0 & 1) << pos);
            val0 >>= 1;
        }
        sel >>= 1;
    }
    return val;
}


void insn_packb(insn_t inst)
{
    DISASM_RD_RS1_RS2("packb");
    WRITE_RD( (RS1 & 0xff) | ((RS2 & 0xff) << 8) );
}


void insn_bitmixb(insn_t inst)
{
    DISASM_RD_RS1_RS2("bitmixb");
    WRITE_RD( bitmixb(uint16_t(RS1), uint16_t(RS2), uint16_t(RS2 >> 8)) );
}


//} // namespace bemu
