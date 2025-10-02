/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef FPU_CASTS_H
#define FPU_CASTS_H

#include <type_traits>
#include "fpu_types.h"
#include "softfloat/internals.h"

union ui16_f11 { uint16_t ui; float11_t f; };
union ui16_f10 { uint16_t ui; float10_t f; };

namespace fpu {

namespace detail {

template<typename T>
inline uint32_t UI32(std::true_type, const T& x) {
    return uint32_t(x);
}

template<typename T>
inline uint32_t UI32(std::false_type, const T&) {
    static_assert(sizeof(T) != sizeof(T),
                  "fpu::UI32() only accepts integral types");
    return 0;
}

template<typename T>
inline float32_t F32(std::true_type, const T& x) {
    ui32_f32 uZ;
    uZ.ui = uint32_t(x);
    return uZ.f;
}

template<typename T>
inline float32_t F32(std::false_type, const T&) {
    static_assert(sizeof(T) != sizeof(T),
                  "fpu::F32() only accepts integral types");
    return float32_t {};
}

template<typename T>
inline float16_t F16(std::true_type, const T& x) {
    ui16_f16 uZ;
    uZ.ui = uint16_t(x);
    return uZ.f;
}

template<typename T>
inline float16_t F16(std::false_type, const T&) {
    static_assert(sizeof(T) != sizeof(T),
                  "fpu::F16() only accepts integral types");
    return float16_t {};
}

template<typename T>
inline float11_t F11(std::true_type, const T& x) {
    ui16_f11 uZ;
    uZ.ui = uint16_t(x);
    return uZ.f;
}

template<typename T>
inline float11_t F11(std::false_type, const T&) {
    static_assert(sizeof(T) != sizeof(T),
                  "fpu::F11() only accepts integral types");
    return float11_t {};
}

template<typename T>
inline float10_t F10(std::true_type, const T& x) {
    ui16_f10 uZ;
    uZ.ui = uint16_t(x);
    return uZ.f;
}

template<typename T>
inline float10_t F10(std::false_type, const T&) {
    static_assert(sizeof(T) != sizeof(T),
                  "fpu::F10() only accepts integral types");
    return float10_t {};
}

} // namespace detail

inline float FLT(uint32_t x) {
    union { uint32_t ui; float flt; } uZ;
    uZ.ui = x;
    return uZ.flt;
}

inline float FLT(float32_t x) {
    union { float32_t f; float flt; } uZ;
    uZ.f = x;
    return uZ.flt;
}

inline float32_t F2F32(float x) {
    union { float32_t f; float flt; } uZ;
    uZ.flt = x;
    return uZ.f;
}

inline uint32_t F2UI32(float x) {
    union { uint32_t ui; float flt; } uZ;
    uZ.flt = x;
    return uZ.ui;
}

template<typename T>
inline float32_t F32(const T& x) {
    return detail::F32(std::is_integral<T>{}, x);
}

template<typename T>
inline float16_t F16(const T& x) {
    return detail::F16(std::is_integral<T>{}, x);
}

template<typename T>
inline float11_t F11(const T& x) {
    return detail::F11(std::is_integral<T>{}, x);
}

template<typename T>
inline float10_t F10(const T& x) {
    return detail::F10(std::is_integral<T>{}, x);
}

inline uint32_t UI32(float10_t x) {
    ui16_f10 uZ;
    uZ.f = x;
    return uint32_t(uZ.ui);
}

inline uint32_t UI32(float11_t x) {
    ui16_f11 uZ;
    uZ.f = x;
    return uint32_t(uZ.ui);
}

inline uint32_t UI32(float16_t x) {
    ui16_f16 uZ;
    uZ.f = x;
    return uint32_t(uZ.ui);
}

inline uint32_t UI32(float32_t x) {
    ui32_f32 uZ;
    uZ.f = x;
    return uZ.ui;
}

template<typename T>
inline uint32_t UI32(const T& x) {
    return detail::UI32(std::is_integral<T>{}, x);
}

inline uint16_t UI16(float16_t x) {
    ui16_f16 uZ;
    uZ.f = x;
    return uZ.ui;
}

inline uint16_t UI16(float11_t x) {
    ui16_f11 uZ;
    uZ.f = x;
    return uZ.ui;
}

inline uint16_t UI16(float10_t x) {
    ui16_f10 uZ;
    uZ.f = x;
    return uZ.ui;
}

} // namespace fpu

#endif // FPU_CASTS_H
