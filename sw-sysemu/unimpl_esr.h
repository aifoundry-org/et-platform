/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_UNIMPL_ESR_H
#define BEMU_UNIMPL_ESR_H

namespace bemu {


// Exception for unimplemented ESR
struct unimpl_esr {
    uint64_t addr;

    constexpr unimpl_esr() : addr(0) {}
    constexpr unimpl_esr(uint64_t addr) : addr(addr) {}
    constexpr unimpl_esr(const unimpl_esr&) = default;
};


} // namespace bemu

#endif // BEMU_MEMORY_ESR_H
