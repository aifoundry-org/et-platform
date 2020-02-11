/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
