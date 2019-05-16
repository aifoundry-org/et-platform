#ifndef ETTEE_UTILS_H
#define ETTEE_UTILS_H

#include "Support/HelperMacros.h"

#include <algorithm>
#include <cmath>

namespace quantization {
/// \returns the value \p in as clipped to the range of \p DestTy.
template <class SrcTy, class DestTy> DestTy clip(SrcTy in) {
  assert(sizeof(SrcTy) >= sizeof(DestTy) && "Invalid types");

  auto mx = std::numeric_limits<DestTy>::max();
  auto mn = std::numeric_limits<DestTy>::min();
  return std::max<SrcTy>(mn, std::min<SrcTy>(mx, in));
}

/// Converts floating point value to int8 based on the quantization
/// parameters.
inline int8_t quantize(float input, float scale, int32_t offset) {
  float result = input / scale + offset;
  return quantization::clip<int32_t, int8_t>(std::round(result));
}

/// Converts int8 quantized value back to floating point number based on
/// the quantization parameters.
inline float dequantize(int8_t input, float scale, int32_t offset) {
  return scale * (input - offset);
}
} // namespace quantization

#endif // ETTEE_UTILS_H
