/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_LAZY_ARRAY_H
#define BEMU_LAZY_ARRAY_H

#include <cstddef>
#include <array>
#include <memory>

namespace bemu {


template<typename Tp, size_t N>
struct lazy_array {
    using array_type            = std::array<Tp,N>;
    using array_pointer         = std::unique_ptr<array_type>;

    using value_type            = typename array_type::value_type;
    using reference             = typename array_type::reference;
    using const_reference       = typename array_type::const_reference;
    using pointer               = typename array_type::pointer;
    using const_pointer         = typename array_type::const_pointer;
    using iterator              = typename array_type::iterator;
    using const_iterator        = typename array_type::const_iterator;
    using reverse_iterator      = typename array_type::reverse_iterator;
    using const_reverse_iterator = typename array_type::const_reverse_iterator;
    using size_type             = typename array_type::size_type;
    using difference_type       = typename array_type::difference_type;

    void allocate() {
        p.reset(new array_type);
    }

    // Iterators

    iterator begin() noexcept { return p->begin(); }
    const_iterator begin() const noexcept { return p->begin(); }

    iterator end() noexcept { return p->end(); }
    const_iterator end() const noexcept { return p->end(); }

    iterator rbegin() noexcept { return p->rbegin(); }
    const_iterator rbegin() const noexcept { return p->rbegin(); }

    iterator rend() noexcept { return p->rend(); }
    const_iterator rend() const noexcept { return p->rend(); }

    const_iterator cbegin() const noexcept { return p->cbegin(); }
    const_iterator cend() const noexcept { return p->cend(); }

    const_iterator crbegin() const noexcept { return p->crbegin(); }
    const_iterator crend() const noexcept { return p->crend(); }

    // Capacity

    constexpr size_type size() const noexcept { return p ? N : 0; }
    constexpr size_type max_size() const noexcept { return N; }
    constexpr bool empty() const noexcept { return !p; }

    // Element access

    reference operator[](size_type n) { return (*p)[n]; }
    constexpr const_reference operator[](size_type n) const { return (*p)[n]; }

    reference at(size_type n) { return p->at(n); }
    const_reference at(size_type n) const { return p->at(n); }

    reference front() { return p->front(); }
    constexpr const_reference front() const { return p->front(); }

    reference back() { return p->back(); }
    constexpr const_reference back() const { return p->back(); }

    pointer data() noexcept { return p ? p->data() : nullptr; }
    const_pointer data() const noexcept { return p ? p->data() : nullptr; }

    // Operations

    void fill(const_reference val) { p->fill(val); }

    void fill_pattern(const_pointer val, size_t size) {
      for ( size_t i = 0 ; i < N ; i++)
        (*p)[i] = val[i % size];
    }

    void swap(lazy_array& other) noexcept { p->swap(other); }

    // Members

    std::unique_ptr<array_type> p{};
};


} // namespace bemu

#endif // BEMU_LAZY_ARRAY_H
