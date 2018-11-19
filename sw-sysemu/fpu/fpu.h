#ifndef _FPU_H
#define _FPU_H

#include "fpu_types.h"

namespace fpu {

    // ----- softfloat wrappers ----------------------------------------------

    void rm(uint_fast8_t);
    uint_fast8_t rm();

    void flags(uint_fast8_t);
    uint_fast8_t flags();

    // ----- RISC-V single-precision extension -------------------------------

    // f32_classify() result bitmask
    static constexpr uint16_t FCLASS_NEG_INFINITY  = 0x0001;
    static constexpr uint16_t FCLASS_NEG_NORMAL    = 0x0002;
    static constexpr uint16_t FCLASS_NEG_SUBNORMAL = 0x0004;
    static constexpr uint16_t FCLASS_NEG_ZERO      = 0x0008;
    static constexpr uint16_t FCLASS_POS_ZERO      = 0x0010;
    static constexpr uint16_t FCLASS_POS_SUBNORMAL = 0x0020;
    static constexpr uint16_t FCLASS_POS_NORMAL    = 0x0040;
    static constexpr uint16_t FCLASS_POS_INFINITY  = 0x0080;
    static constexpr uint16_t FCLASS_SNAN          = 0x0100;
    static constexpr uint16_t FCLASS_QNAN          = 0x0200;

    // fclass combined classes
    static constexpr uint16_t FCLASS_INFINITY  = (FCLASS_NEG_INFINITY  | FCLASS_POS_INFINITY );
    static constexpr uint16_t FCLASS_NORMAL    = (FCLASS_NEG_NORMAL    | FCLASS_POS_NORMAL   );
    static constexpr uint16_t FCLASS_SUBNORMAL = (FCLASS_NEG_SUBNORMAL | FCLASS_POS_SUBNORMAL);
    static constexpr uint16_t FCLASS_ZERO      = (FCLASS_NEG_ZERO      | FCLASS_POS_ZERO     );
    static constexpr uint16_t FCLASS_NAN       = (FCLASS_SNAN          | FCLASS_QNAN         );
    static constexpr uint16_t FCLASS_NEGATIVE  = (FCLASS_NEG_INFINITY | FCLASS_NEG_NORMAL | FCLASS_NEG_SUBNORMAL | FCLASS_NEG_ZERO);
    static constexpr uint16_t FCLASS_POSITIVE  = (FCLASS_POS_INFINITY | FCLASS_POS_NORMAL | FCLASS_POS_SUBNORMAL | FCLASS_POS_ZERO);

    float32_t f32_add(float32_t, float32_t);
    float32_t f32_sub(float32_t, float32_t);
    float32_t f32_mul(float32_t, float32_t);
    float32_t f32_div(float32_t, float32_t);
    float32_t f32_sqrt(float32_t);

    float32_t f32_copySign(float32_t, float32_t);
    float32_t f32_copySignNot(float32_t, float32_t);
    float32_t f32_copySignXor(float32_t, float32_t);

    // NB: IEEE 754-201x compatible
    float32_t f32_minNum(float32_t, float32_t);
    float32_t f32_maxNum(float32_t, float32_t);

    bool f32_eq(float32_t, float32_t);
    bool f32_lt(float32_t, float32_t);
    bool f32_le(float32_t, float32_t);

    int_fast32_t f32_to_i32(float32_t);
    uint_fast32_t f32_to_ui32(float32_t);
    int_fast64_t f32_to_i64(float32_t);
    uint_fast64_t f32_to_ui64(float32_t);

    uint_fast16_t f32_classify(float32_t);

    float32_t i32_to_f32(int32_t);
    float32_t ui32_to_f32(uint32_t);
    float32_t i64_to_f32(int64_t);
    float32_t ui64_to_f32(uint64_t);

    float32_t f32_mulAdd(float32_t, float32_t, float32_t);
    float32_t f32_mulSub(float32_t, float32_t, float32_t);
    float32_t f32_negMulAdd(float32_t, float32_t, float32_t);
    float32_t f32_negMulSub(float32_t, float32_t, float32_t);

    // ----- Esperanto single-precision extension ----------------------------

    // Graphics downconvert

    float16_t f32_to_f16(float32_t);
    float11_t f32_to_f11(float32_t);
    float10_t f32_to_f10(float32_t);

    uint_fast32_t f32_to_un24(float32_t);
    uint_fast16_t f32_to_un16(float32_t);
    uint_fast16_t f32_to_un10(float32_t);
    uint_fast8_t f32_to_un8(float32_t);
    uint_fast8_t f32_to_un2(float32_t);

    uint_fast16_t f32_to_sn16(float32_t);
    uint_fast8_t f32_to_sn8(float32_t);
    //uint_fast32_t f32_to_sn24(float32_t);
    //uint_fast16_t f32_to_sn10(float32_t);
    //uint_fast8_t f32_to_sn2(float32_t);

    // Graphics upconvert

    float32_t f16_to_f32(float16_t);
    float32_t f11_to_f32(float11_t);
    float32_t f10_to_f32(float10_t);

    float32_t un24_to_f32(uint32_t);
    float32_t un16_to_f32(uint16_t);
    float32_t un10_to_f32(uint16_t);
    float32_t un8_to_f32(uint8_t);
    float32_t un2_to_f32(uint8_t);

    float32_t sn24_to_f32(uint32_t);
    float32_t sn16_to_f32(uint16_t);
    float32_t sn10_to_f32(uint16_t);
    float32_t sn8_to_f32(uint8_t);
    float32_t sn2_to_f32(uint8_t);

    // Graphics additional

    float32_t f32_sin2pi(float32_t);
    float32_t f32_exp2(float32_t);
    float32_t f32_log2(float32_t);
    float32_t f32_frac(float32_t);

    float32_t f32_rcp(float32_t);
    float32_t f32_rsqrt(float32_t);

    float32_t f32_roundToInt(float32_t);

    float32_t f32_cubeFaceIdx(uint8_t, float32_t);
    float32_t f32_cubeFaceSignS(uint8_t a, float32_t b);
    float32_t f32_cubeFaceSignT(uint8_t a, float32_t b);

    float32_t fxp1516_to_f32(int32_t);
    int32_t f32_to_fxp1714(float32_t);

    int32_t fxp1714_rcpStep(int32_t, int32_t);

    // ----- Esperanto tensor extension --------------------------------------

    float32_t f32_tensorMulAddF16(float32_t,
                                  float16_t, float16_t,
                                  float16_t, float16_t);

} // namespace fpu

#endif // _FPU_H
