#include <emu.h>
#include <math.h>
#include "./trcp.h"

// FRCP   (input bit [31:0] x);      // ROM: ready!
// FRSQ   (input bit [31:0] x);      // ROM: ready!
// FSIN   (input bit [31:0] x);      // ROM: ready!
// FEXP   (input bit [31:0] x);      // ROM: ready!
// FLOG   (input bit [31:0] x);      // ROM: ready!

float32 ttrans_frcp(uint32 val) {

    uint32 x2   = ( val                  % (1 << (23-7))); // input bits [15: 0]
    uint32 idx  = ((val / (1 << (23-7))) % (1 <<     7 )); // input bits [22:16]
    uint8  exp  = ((val / (1 <<  23   )) % (1 <<     8 )); // input bits [30:23]
    bool   sign = ((val / (1 << (23+8)))                ); // input bit  [31]

    bool   or_mantissa  = (x2+idx > 0);
    bool   and_exponent = (exp > 253);

    uint64 c2 = trcp[idx][0];
    uint64 c1 = trcp[idx][1] * (1 << 17);
    uint64 c0 = trcp[idx][2] * (1 << 23);

    uint64 fma1 = (-c2 * x2 + c1 + (1 << 6)) / (1 << 7);
    uint64 fma2 = (-fma1 * x2 + c0 + (1 << 24)) / (1 << 25);

    uint32 output;

    // output sign
    output = sign * (1 << 31);
    
    // output exponent
    if(and_exponent)
    {
        if(or_mantissa)
            output += (0xff << 23);
    }
    else 
        output += (((254-exp-or_mantissa) & 0xff) << 23);

    // output mantissa
    if(and_exponent && or_mantissa)
        output += 0x20000;
    else if (or_mantissa)
        output += (uint32) fma2;

    return *((float32 *) &output);
}

