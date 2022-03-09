/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#include "system.h"

#define DMACTIVE(x)         (((x) >>  0) & 1)
#define NDMRESET(x)         (((x) >>  1) & 1)
#define CLRRESETHALTREQ(x)  (((x) >>  2) & 1)
#define SETRESETHALTREQ(x)  (((x) >>  3) & 1)
#define HASEL(x)            (((x) >> 26) & 1)
#define ACKHAVERESET(x)     (((x) >> 28) & 1)
#define HARTRESET(x)        (((x) >> 29) & 1)
#define RESUMEREQ(x)        (((x) >> 30) & 1)
#define HALTREQ(x)          (((x) >> 31) & 1)
#define HARTSEL(x)          ((((x) << 4) & 0xffc00) | (((x) >> 16) & 0x3ff))

namespace bemu {


//==------------------------------------------------------------------------==//
//
// Minionshire debug module
//
//==------------------------------------------------------------------------==//

void System::write_dmctrl(uint32_t value)
{
    uint32_t newvalue = value & 0xF400000F;
    uint32_t oldvalue = dmctrl;
    dmctrl = newvalue;

    // dmactive going from 0 to 1. In this case we reset the DM and we ignore
    // all other bits written :-/
    if (DMACTIVE(oldvalue) == 0 && DMACTIVE(newvalue) == 1) {
        for (unsigned s = 0; s < EMU_NUM_COMPUTE_SHIRES; ++s) {
            debug_reset(s);
        }
        dmctrl = 0;
        return;
    }

    if (DMACTIVE(newvalue) == 0) {
        // The DM is inactive; ignore request
        return;
    }

    // ndmreset goes from 0 to 1: start resetting the system
    if (NDMRESET(oldvalue) == 0 && NDMRESET(newvalue) == 1) {
        for (unsigned s = 0; s < EMU_NUM_COMPUTE_SHIRES; ++s) {
            begin_warm_reset(s);
        }
        return;
    }

    // ndmreset goes from 0 to 1: stop resetting the system
    if (NDMRESET(oldvalue) == 1 && NDMRESET(newvalue) == 0) {
        for (unsigned s = 0; s < EMU_NUM_COMPUTE_SHIRES; ++s) {
            end_warm_reset(s);
        }
        return;
    }

    // On any given write, a debugger may only write 1 to at most one of the
    // following bits: resumereq, hartreset, ackhavereset, setresethaltreq,
    // and clrresethaltreq. The others must be written 0.

    if (RESUMEREQ(newvalue) == 1) {
        // haltreq has priority over resumereq
        if (HALTREQ(newvalue) == 0) {
            // FIXME: do something here
        }
    }
    else if (HARTRESET(newvalue) == 1) {
        // hartreset goes from 0 to 1: start resetting the selected harts
        if (HARTRESET(oldvalue) == 0) {
            // FIXME: do something here
        }
    }
    else if (ACKHAVERESET(newvalue) == 1) {
        // FIXME: do something here
    }
    else if (SETRESETHALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }
    else if (CLRRESETHALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }

    // hartreset goes from 1 to 0: stop resetting the selected harts
    if (HARTRESET(oldvalue) == 1 && HARTRESET(newvalue) == 0) {
        // FIXME: do something here
    }

    if (HALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }
}


uint32_t System::read_dmctrl() const
{
    return dmctrl & 0x2FFFFFF3;
}


uint32_t System::read_andortree2() const
{
    static_assert(EMU_NUM_COMPUTE_SHIRES <= 48, "Wrong number of compute shires");

    // pre-set the 'all*' fields to 1 and the 'any*' fields to 0
    uint16_t value = 0x2a8;

    for (unsigned shire = 0; shire < EMU_NUM_COMPUTE_SHIRES; ++shire) {
        uint32_t and_or_tree_1 = calculate_andortree1(shire);

        // set anyhalted0, anyhalted1, and anyhalted2 by OR-ing
        value |= ((and_or_tree_1 | (and_or_tree_1 >> 1)) & 1) << (shire / 16);

        // set the other 'any*' fields by OR-ing; also set bit 11 to
        // 'anyselected' (will be cleared later)
        value |= (and_or_tree_1 << 1) & 0xd50;

        // set the 'all*' fields by AND-ing
        if ((and_or_tree_1 & (1 << 10)) != 0) {
            value &= ((and_or_tree_1 << 1) & 0x2a8) | ~0x2a8;
        }
    }

    // if 'anyselected' is 0 then set the 'all*' fields to 0
    if ((value & (1 << 11)) == 0) {
        value &= ~0x2a8;
    }

    // clear 'anyselected' to calculate the final value
    return value & 0x7ff;
}


uint16_t System::calculate_andortree0(unsigned neigh) const
{
    uint16_t selected = selected_neigh_harts(neigh);

    if (!selected) {
        return 0;
    }

    uint32_t halted      = neigh_esrs[neigh].hastatus0 & selected;
    uint32_t running     = (neigh_esrs[neigh].hastatus0 >> 16) & selected;
    uint32_t resumeack   = (neigh_esrs[neigh].hastatus0 >> 32) & selected;
    uint32_t havereset   = (neigh_esrs[neigh].hastatus0 >> 48) & selected;
    uint32_t unavailable = ~(halted | running) & selected;

    uint16_t value = 0;
    value |= (!!halted)              << 0; // anyhalted
    value |= (halted == selected)    << 1; // allhalted
    value |= (!!running)             << 2; // anyrunning
    value |= (running == selected)   << 3; // allrunning
    value |= (!!resumeack)           << 4; // anyresumeack
    value |= (resumeack == selected) << 5; // allresumeack
    value |= (!!havereset)           << 6; // anyhavereset
    value |= (havereset == selected) << 7; // allhavereset
    value |= (!!unavailable)         << 8; // anyunavailable
    value |= (!!selected)            << 9; // anyselected
    return value;
}


uint16_t System::calculate_andortree1(unsigned shire) const
{
    static_assert(EMU_NEIGH_PER_SHIRE == 4, "Wrong number of neighborhoods");

    // pre-set the 'all*' fields to 1 and the 'any*' fields to 0
    uint16_t value = 0x154;

    for (unsigned n = 0; n < EMU_NEIGH_PER_SHIRE; ++n) {
        uint16_t and_or_tree_0 = calculate_andortree0(n + shire * EMU_NEIGH_PER_SHIRE);

        // set anyhalted0 and anyhalted1 by OR-ing
        value |= (and_or_tree_0 & 1) << (n / 2);

        // set the other 'any*' fields by OR-ing
        value |= (and_or_tree_0 << 1) & 0x6a8;

        // set the 'all*' fields by AND-ing
        if ((and_or_tree_0 & (1 << 9)) != 0) {
            value &= ((and_or_tree_0 << 1) & 0x154) | ~0x154;
        }
    }

    // if 'anyselected' is 0 then set the 'all*' fields to 0
    if ((value & (1 << 10)) == 0) {
        value &= ~0x154;
    }

    return value & 0x7ff;
}


//==------------------------------------------------------------------------==//
//
// Service processor debug module
//
//==------------------------------------------------------------------------==//

void System::write_spdmctrl(uint32_t value)
{
    uint32_t newvalue = value & 0xF000000E;
    uint32_t oldvalue = spdmctrl;
    spdmctrl = newvalue;

    // ndmreset goes from 0 to 1: start resetting the system
    if (NDMRESET(oldvalue) == 0 && NDMRESET(newvalue) == 1) {
        begin_warm_reset(EMU_IO_SHIRE_SP);
        return;
    }

    // ndmreset goes from 0 to 1: stop resetting the system
    if (NDMRESET(oldvalue) == 1 && NDMRESET(newvalue) == 0) {
        end_warm_reset(EMU_IO_SHIRE_SP);
        return;
    }

    // On any given write, a debugger may only write 1 to at most one of the
    // following bits: resumereq, hartreset, ackhavereset, setresethaltreq,
    // and clrresethaltreq. The others must be written 0.

    if (RESUMEREQ(newvalue) == 1) {
        // haltreq has priority over resumereq
        if (HALTREQ(newvalue) == 0) {
            // FIXME: do something here
        }
    }
    else if (HARTRESET(newvalue) == 1) {
        // hartreset goes from 0 to 1: start resetting the selected harts
        if (HARTRESET(oldvalue) == 0) {
            // FIXME: do something here
        }
    }
    else if (ACKHAVERESET(newvalue) == 1) {
        // FIXME: do something here
    }
    else if (SETRESETHALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }
    else if (CLRRESETHALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }

    // hartreset goes from 1 to 0: stop resetting the selected harts
    if (HARTRESET(oldvalue) == 1 && HARTRESET(newvalue) == 0) {
        // FIXME: do something here
    }

    if (HALTREQ(newvalue) == 1) {
        // FIXME: do something here
    }
}


uint32_t System::read_spdmctrl() const
{
    return spdmctrl & 0x2FFFFFF3;
}


void System::write_sphastatus(uint32_t value)
{
    // FIXME: Do something here
    (void)(value);
}


uint32_t System::read_sphastatus() const
{
    return sphastatus & 0x7f;
}


} // namespace bemu
