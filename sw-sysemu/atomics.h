/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_ATOMICS_H
#define BEMU_ATOMICS_H

#include <cstdint>
#include <algorithm>
#include <functional>

#include "softfloat/platform.h"
#include "softfloat/internals.h"
#include "softfloat/specialize.h"

namespace bemu {

template<typename T>
struct replace {
    using first_argument_type  = T;
    using second_argument_type = T;
    using result_type          = T;

    T operator() (const T& x __attribute__((unused)), const T& y) const {
        return y;
    }
};


template<typename T>
struct maximum {
    using first_argument_type  = T;
    using second_argument_type = T;
    using result_type          = T;

    T operator() (const T& x, const T& y) const {
        return std::max(x, y);
    }
};


template<typename T>
struct minimum {
    using first_argument_type  = T;
    using second_argument_type = T;
    using result_type          = T;

    T operator() (const T& x, const T& y) const {
        return std::min(x, y);
    }
};


struct f32_maximum {
    using first_argument_type  = uint32_t;
    using second_argument_type = uint32_t;
    using result_type          = uint32_t;

    uint32_t operator() (const uint32_t& x, const uint32_t& y) const {
        if (isNaNF32UI(x) || isNaNF32UI(y)) {
            if (softfloat_isSigNaNF32UI(x) || softfloat_isSigNaNF32UI(y)) {
                return defaultNaNF32UI;
            }
            return isNaNF32UI(x) ? (isNaNF32UI(y) ? defaultNaNF32UI : y) : x;
        }
        bool signX = signF32UI(x);
        bool signY = signF32UI(y);
        return (signX != signY)
                ? (signY ? x : y)
                : (((x != y) && (signY ^ (y < x))) ? x : y);
    }
};


struct f32_minimum {
    using first_argument_type  = uint32_t;
    using second_argument_type = uint32_t;
    using result_type          = uint32_t;

    uint32_t operator() (const uint32_t& x, const uint32_t& y) const {
        if (isNaNF32UI(x) || isNaNF32UI(y)) {
            if (softfloat_isSigNaNF32UI(x) || softfloat_isSigNaNF32UI(y)) {
                return defaultNaNF32UI;
            }
            return isNaNF32UI(x) ? (isNaNF32UI(y) ? defaultNaNF32UI : y) : x;
        }
        bool signX = signF32UI( x );
        bool signY = signF32UI( y );
        return (signX != signY)
                ? (signX ? x : y)
                : (((x != y) && (signX ^ (x < y))) ? x : y );
    }
};


} // namespace bemu

#endif // BEMU_ATOMICS_H
