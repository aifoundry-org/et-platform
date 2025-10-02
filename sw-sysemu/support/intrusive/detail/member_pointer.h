/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_INTRUSIVE_DETAIL_MEMBER_POINTER_H
#define BEMU_INTRUSIVE_DETAIL_MEMBER_POINTER_H

#include <cstddef>

namespace bemu { namespace intrusive { namespace detail {


template<class To, class From>
inline To
pointer_cast(From* ptr)
{
    return static_cast<To>(static_cast<void*>(ptr));
}


template<class To, class From>
inline const To
pointer_cast(const From* ptr)
{
    return static_cast<const To>(static_cast<const void*>(ptr));
}


template<class T, class Member>
inline std::ptrdiff_t
member_offset(const Member T::* member)
{
#ifdef _MSVC_VER
#error "Windows compiler not supported"
#elif defined(__GNUC__)
    const T* const obj = 0;
    return std::ptrdiff_t(pointer_cast<const char*>(&(obj->*member)) -
                          pointer_cast<const char*>(obj));
#else
    union {
        const Member T::* ptr;
        std::ptrdiff_t diff;
    } tmp;
    tmp.ptr = member;
    return tmp.diff - 1;
#endif
}


template<class T, class Member, Member T::* member>
inline T*
pointer_from_member(Member* ptr)
{
    char* const p = pointer_cast<char*>(ptr);
    return pointer_cast<T*>(p - member_offset(member));
}


template<class T, class Member, Member T::* member>
inline const T*
pointer_from_member(const Member* ptr)
{
    const char* const p = pointer_cast<const char*>(ptr);
    return pointer_cast<const T*>(p - member_offset(member));
}


template<class T, class Member, Member T::* member>
inline Member*
member_from_pointer(T* ptr)
{
    char* const p = pointer_cast<char*>(ptr);
    return pointer_cast<Member*>(p + member_offset(member));
}


template<class T, class Member, Member T::* member>
inline const Member*
member_from_pointer(const T* ptr)
{
    const char* const p = pointer_cast<const char*>(ptr);
    return pointer_cast<const Member*>(p + member_offset(member));
}


}}} // namespace bemu::intrusive::detail

#endif // BEMU_INTRUSIVE_DETAIL_MEMBER_POINTER_H
