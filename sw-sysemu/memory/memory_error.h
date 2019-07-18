/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_MEMORY_ERROR_H
#define BEMU_MEMORY_ERROR_H

namespace bemu {


// Basic class for signaling any type of memory error to the caller
struct memory_error {
    unsigned long long addr;

    constexpr memory_error() : addr(0) { }
    constexpr memory_error(unsigned long long x) : addr(x) { }
    constexpr memory_error(const memory_error&) = default;
};


} // namespace bemu

#endif // BEMU_MEMORY_ERROR_H
