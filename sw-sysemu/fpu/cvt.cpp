/* vim: set ts=8 sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#include <cmath>
#include "fpu_types.h"
#include "fpu_casts.h"
#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

namespace fpu {


float32_t un24_to_f32(uint32_t val)
{
    uint32_t maxrange = (1 << 24) - 1;
    float res = float(val & 0xFFFFFF) / float(maxrange);
    return fpu::F2F32(res);
}


float32_t un16_to_f32(uint16_t val)
{
    uint32_t maxrange = (1 << 16) - 1;
    float res = float(val & 0xFFFF) / float(maxrange);
    return fpu::F2F32(res);
}


float32_t un10_to_f32(uint16_t val)
{
    uint32_t maxrange = (1 << 10) - 1;
    float res = float(val & 0x3FF) / float(maxrange);
    return fpu::F2F32(res);
}


float32_t un8_to_f32(uint8_t val)
{
    uint32_t maxrange = (1 << 8 ) - 1;
    float res = float(val & 0xFF) / float(maxrange);
    return fpu::F2F32(res);
}


float32_t un2_to_f32(uint8_t val)
{
    uint32_t maxrange = (1 << 2) - 1;
    float res = float(val & 0x3) / float(maxrange);
    return fpu::F2F32(res);
}


#if 0
float32_t sn24_to_f32(uint32_t val)
{
    if (val == (1 << 23)) val = (1 << 23) + 1;
    int sign = ((val & 0x00800000) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x007fffff;
    uint32_t maxrange = (1 << 23) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}
#endif


float32_t sn16_to_f32(uint16_t val)
{
    if (val == (1 << 15)) val = (1 << 15) + 1;
    int sign = ((val & 0x00008000) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x00007fff;
    uint32_t maxrange = (1 << 15) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}


// Used by the TBOX
float32_t sn10_to_f32(uint16_t val)
{
    if (val == (1 << 9)) val = (1 << 9) + 1;
    int sign = ((val & 0x000200) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x0000001ff;
    uint32_t maxrange = (1 << 9) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}


float32_t sn8_to_f32(uint8_t val)
{
    if (val == (1 << 7)) val = (1 << 7) + 1;
    int sign = ((val & 0x000080) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x0000007f;
    uint32_t maxrange = (1 << 7) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}


// Used by the TBOX
float32_t sn2_to_f32(uint8_t val)
{
    if (val == (1 << 1)) val = (1 << 1) + 1;
    int sign = ((val & 0x00000002) == 0) ? 1 : -1;
    uint32_t value = ((sign < 0) ? (~val + 1) : val) & 0x00000001;
    uint32_t maxrange = (1 << 1) - 1;
    float res = float(sign) * (float(value) / float(maxrange));
    return fpu::F2F32(res);
}


uint32_t f32_to_un24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x00ffffff;
    }
    if (val_i >= 0x3f800000) {
        return 0x00ffffff;
    }
    if (val_i <= 0) {
        return 0x00000000;
    }
    uint32_t delta = (1 << 24) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint32_t(ratio + 0.5f);
}


uint16_t f32_to_un16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0xffff;
    }
    if (val_i >= 0x3f800000) {
        return 0xffff;
    }
    if (val_i <= 0) {
        return 0x0000;
    }
    uint32_t delta = (1 << 16) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint16_t(ratio + 0.5f);
}


uint16_t f32_to_un10(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x03ff;
    }
    if (val_i >= 0x3f800000) {
        return 0x03ff;
    }
    if (val_i <= 0) {
        return 0x0000;
    }
    uint32_t delta = (1 << 10) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint16_t(ratio + 0.5f);
}


uint8_t f32_to_un8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0xff;
    }
    if (val_i >= 0x3f800000) {
        return 0xff;
    }
    if (val_i <= 0) {
        return 0x00;
    }
    uint32_t delta = (1 << 8) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint8_t(ratio + 0.5f);
}


uint8_t f32_to_un2(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x03;
    }
    if (val_i >= 0x3f800000) {
        return 0x03;
    }
    if (val_i <= 0) {
        return 0x00;
    }
    uint32_t delta = (1 << 2) - 1;
    double ratio = double(fpu::FLT(val)) * double(delta);
    return uint8_t(ratio + 0.5f);
}


uint32_t f32_to_sn24(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x007fffff;
    }
    if (val_i >= 0x3f800000) {
        return 0x007fffff;
    }
    if (val.v >= 0xbf800000) {
        return 0x00800001;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 23) - 1));
    int32_t res = int32_t(int_val);
    return uint32_t(res & 0x00ffffff);
}


uint16_t f32_to_sn16(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x7fff;
    }
    if (val_i >= 0x3f800000) {
        return 0x7fff;
    }
    if (val.v >= 0xbf800000) {
        return 0x8001;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 15) - 1));
    int32_t res = int32_t(int_val);
    return uint16_t(res & 0x0000ffff);
}


uint8_t f32_to_sn8(float32_t val)
{
    int32_t val_i = int32_t(val.v);
    if (isNaNF32UI(val.v)) {
        return 0x7f;
    }
    if (val_i >= 0x3f800000) {
        return 0x7f;
    }
    if (val.v >= 0xbf800000) {
        return 0x81;
    }
    float int_val = round(fpu::FLT(val) * float((1 << 7) - 1));
    int32_t res = int32_t(int_val);
    return uint8_t(res & 0x000000ff);
}


} // namespace fpu
