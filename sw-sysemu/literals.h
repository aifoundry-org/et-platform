/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------*/

#ifndef BEMU_LITERALS_H
#define BEMU_LITERALS_H

namespace bemu {
inline namespace literals {


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


} // namespace bemu::literals
} // namespace bemu

#endif // BEMU_LITERALS_H
