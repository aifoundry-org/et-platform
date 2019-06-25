/* vim: set sw=4 et sta cin cino=\:0s,l1,g0,N-s,E-s,i0,+2s,(0,W2s : */

#ifndef BEMU_LITERALS_H
#define BEMU_LITERALS_H

namespace bemu {


//
// User-defined literals for memory sizes: X_KiB, X_MiB, X_GiB, X_TiB
//

constexpr inline unsigned long long operator"" _KiB(unsigned long long v)
{ return v << 10; }

constexpr inline unsigned long long operator"" _MiB(unsigned long long v)
{ return v << 20; }

constexpr inline unsigned long long operator"" _GiB(unsigned long long v)
{ return v << 30; }

constexpr inline unsigned long long operator"" _TiB(unsigned long long v)
{ return v << 40; }


} // namespace bemu

#endif // BEMU_LITERALS_H
