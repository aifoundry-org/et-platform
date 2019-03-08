#ifndef ETTEE_UTILS_H
#define ETTEE_UTILS_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <cmath>
#include <vector>

#define STRINGIFY(s) STRINGIFY_INTERNAL(s)
#define STRINGIFY_INTERNAL(s) #s


#define PERROR_IF(fail_cond) \
do { if (fail_cond) { \
    fprintf(stderr, __FILE__ ":" STRINGIFY(__LINE__) ": %s: PERROR: %s\n", __PRETTY_FUNCTION__, strerror(errno)); \
    abort(); \
} } while (0)


[[noreturn]] inline void THROW(std::string msg) {
    fprintf(stderr, "There was an error: %s\n", msg.c_str());
    abort();
}

inline void THROW_IF(bool fail_cond, std::string msg) {
    if (fail_cond) {
        THROW(msg);
    }
}

namespace inner
{
template <typename T>
void join_helper(std::ostringstream &stream, T &&t)
{
    stream << std::forward<T>(t);
}

template <typename T, typename... Ts>
void join_helper(std::ostringstream &stream, T &&t, Ts &&... ts)
{
    stream << std::forward<T>(t);
    join_helper(stream, std::forward<Ts>(ts)...);
}
} // namespace inner

// Helper template to avoid lots of nasty string temporary munging.
template <typename... Ts>
std::string join(Ts &&... ts)
{
    std::ostringstream stream;
    inner::join_helper(stream, std::forward<Ts>(ts)...);
    return stream.str();
}


inline bool is_power_of_2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

inline size_t align_up(size_t x, size_t alignment)
{
    assert(is_power_of_2(alignment));
    return (x + alignment - 1) & ~(alignment - 1);
}

inline bool is_aligned(size_t x, size_t alignment)
{
    assert(is_power_of_2(alignment));
    return (x & (alignment - 1)) == 0;
}


template<typename T>
inline std::ptrdiff_t stl_count(const std::vector<T> &v, const T &item)
{
    return std::count(v.begin(), v.end(), item);
}

template<typename T>
inline void stl_remove(std::vector<T> &v, const T &item)
{
    v.erase(std::remove(v.begin(), v.end(), item), v.end());
}

template<typename T>
inline std::ptrdiff_t stl_count(const std::vector<std::unique_ptr<T>> &v, const T *ptr)
{
    return std::count_if(v.begin(), v.end(), [ptr](const std::unique_ptr<T> &item) { return item.get() == ptr; });
}

template<typename T>
inline void stl_remove(std::vector<std::unique_ptr<T>> &v, const T *ptr)
{
    v.erase(std::remove_if(v.begin(), v.end(), [ptr](const std::unique_ptr<T> &item) { return item.get() == ptr; }),
            v.end());
}


inline unsigned defaultGridDim1D(unsigned global_size) { return align_up(global_size, 32) / 32; }
inline unsigned defaultBlockDim1D() { return 32; }


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


#endif  // ETTEE_UTILS_H
