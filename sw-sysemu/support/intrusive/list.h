/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef BEMU_INTRUSIVE_LIST_H
#define BEMU_INTRUSIVE_LIST_H

#include "detail/member_pointer.h"
#include <iterator>

namespace bemu { namespace intrusive {


//==------------------------------------------------------------------------==//
///
/// List hook; must be declared as member variable of the list element class.
///
//==------------------------------------------------------------------------==//

struct List_hook {
    // Remove the hooked object from the containing list, even when the caller
    // does not know which list.
    void unlink() noexcept;

    // Insert `node` before `this`. Does not check for null pointers.
    void link(List_hook* node) noexcept;

    // Insert [first, last) before `this`. The behavior is undefined if `this`
    // is in the sequence [first, last]. Does not check for null pointers.
    void splice(List_hook* first, List_hook* last) noexcept;

    List_hook* m_prev = this;
    List_hook* m_next = this;
};


inline void
List_hook::unlink() noexcept
{
    m_next->m_prev = m_prev;
    m_prev->m_next = m_next;
    m_prev = m_next = this;
}


inline void
List_hook::link(List_hook* node) noexcept
{
    if (node != this) {
        // unlink `node` from its current list
        node->m_next->m_prev = node->m_prev;
        node->m_prev->m_next = node->m_next;

        // link `node` before `this`
        node->m_next = this;
        node->m_prev = m_prev;
        node->m_prev->m_next = node->m_next->m_prev = node;
    }
}


inline void
List_hook::splice(List_hook* first, List_hook* last) noexcept
{
    if (first != last) {
        last = last->m_prev;

        // unlink [first, last) from its current list
        last->m_next->m_prev = first->m_prev;
        first->m_prev->m_next = last->m_next;

        // link [first, last) before `this`
        first->m_prev = m_prev;
        first->m_prev->m_next = first;
        last->m_next = this;
        last->m_next->m_prev = last;
    }
}


//==------------------------------------------------------------------------==//
///
/// List iterator
///
//==------------------------------------------------------------------------==//

template<class T, List_hook T::* member>
struct List_iterator {
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;

    List_iterator(const List_iterator&) noexcept = default;

    explicit List_iterator(List_hook* ptr) noexcept
        : m_ptr(ptr)
    { }

    pointer
    operator->() const noexcept
    { return detail::pointer_from_member<T, List_hook, member>(m_ptr); }

    reference
    operator*() const noexcept
    { return *detail::pointer_from_member<T, List_hook, member>(m_ptr); }

    List_iterator&
    operator++() noexcept
    {
        m_ptr = m_ptr->m_next;
        return *this;
    }

    List_iterator
    operator++(int) noexcept
    {
        List_iterator tmp = *this;
        m_ptr = m_ptr->m_next;
        return tmp;
    }

    List_iterator&
    operator--() noexcept
    {
        m_ptr = m_ptr->m_prev;
        return *this;
    }

    List_iterator
    operator--(int) noexcept
    {
        List_iterator tmp = *this;
        m_ptr = m_ptr->m_prev;
        return tmp;
    }

    friend bool
    operator==(const List_iterator& lhs, const List_iterator& rhs) noexcept
    { return lhs.m_ptr == rhs.m_ptr; }

    friend bool
    operator!=(const List_iterator& lhs, const List_iterator& rhs) noexcept
    { return lhs.m_ptr != rhs.m_ptr; }

    List_hook*
    get() const noexcept
    { return const_cast<List_hook*>(m_ptr); }

    List_hook* m_ptr;
};


//==------------------------------------------------------------------------==//
///
/// List const_iterator
///
//==------------------------------------------------------------------------==//

template<class T, List_hook T::* member>
struct List_const_iterator {
    using difference_type     = std::ptrdiff_t;
    using iterator_category   = std::bidirectional_iterator_tag;
    using value_type          = T;
    using pointer             = const T*;
    using reference           = const T&;

    List_const_iterator(const List_const_iterator&) noexcept = default;

    List_const_iterator(const List_iterator<T, member>& other) noexcept
        : m_ptr(other.m_ptr)
    { }

    explicit List_const_iterator(const List_hook* ptr) noexcept
        : m_ptr(ptr)
    { }

    pointer
    operator->() const noexcept
    { return detail::pointer_from_member<T, const List_hook, member>(m_ptr); }

    reference
    operator*() const noexcept
    { return *detail::pointer_from_member<T, const List_hook, member>(m_ptr); }

    List_const_iterator&
    operator++() noexcept
    {
        m_ptr = m_ptr->m_next;
        return *this;
    }

    List_const_iterator
    operator++(int) noexcept
    {
        List_const_iterator tmp = *this;
        m_ptr = m_ptr->m_next;
        return tmp;
    }

    List_const_iterator&
    operator--() noexcept
    {
        m_ptr = m_ptr->m_prev;
        return *this;
    }

    List_const_iterator
    operator--(int) noexcept
    {
        List_const_iterator tmp = *this;
        m_ptr = m_ptr->m_prev;
        return tmp;
    }

    friend bool
    operator==(const List_const_iterator& lhs,
               const List_const_iterator& rhs) noexcept
    { return lhs.m_ptr == rhs.m_ptr; }

    friend bool
    operator!=(const List_const_iterator& lhs,
               const List_const_iterator& rhs) noexcept
    { return lhs.m_ptr != rhs.m_ptr; }

    List_hook*
    get() const noexcept
    { return const_cast<List_hook*>(m_ptr); }

    const List_hook* m_ptr;
};


//==------------------------------------------------------------------------==//
///
/// Intrusive list
///
//==------------------------------------------------------------------------==//

template<class T, List_hook T::* member>
class List {
public:
    using value_type              = T;
    using pointer                 = T*;
    using const_pointer           = const T*;
    using reference               = T&;
    using const_reference         = const T&;
    using iterator                = List_iterator<T, member>;
    using const_iterator          = List_const_iterator<T, member>;
    using const_reverse_iterator  = std::reverse_iterator<const_iterator>;
    using reverse_iterator        = std::reverse_iterator<iterator>;
    using size_type               = std::size_t;
    using difference_type         = std::ptrdiff_t;

    // Constructors

    List() noexcept = default;
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    // Iterators

    iterator
    begin() noexcept
    { return iterator(m_hook.m_next); }

    const_iterator
    begin() const noexcept
    { return const_iterator(m_hook.m_next); }

    const_iterator
    cbegin() const noexcept
    { return const_iterator(m_hook.m_next); }

    iterator
    end() noexcept
    { return iterator(&m_hook); }

    const_iterator
    end() const noexcept
    { return const_iterator(&m_hook); }

    const_iterator
    cend() const noexcept
    { return const_iterator(&m_hook); }

    reverse_iterator
    rbegin() noexcept
    { return reverse_iterator(end()); }

    const_reverse_iterator
    rbegin() const noexcept
    { return const_reverse_iterator(cend()); }

    const_reverse_iterator
    crbegin() const noexcept
    { return const_reverse_iterator(cend()); }

    reverse_iterator
    rend() noexcept
    { return reverse_iterator(begin()); }

    const_reverse_iterator
    rend() const noexcept
    { return const_reverse_iterator(cbegin()); }

    const_reverse_iterator
    crend() const noexcept
    { return const_reverse_iterator(cbegin()); }

    // Element access

    reference
    front() noexcept
    { return *begin(); }

    const_reference
    front() const noexcept
    { return *cbegin(); }

    reference
    back() noexcept
    { return *(--end()); }

    const_reference
    back() const noexcept
    { return *(--cend()); }

    // Add and remove elements

    void
    push_back(reference value) noexcept
    { insert(cend(), value); }

    void
    push_front(reference value) noexcept
    { insert(cbegin(), value); }

    void
    pop_back() noexcept
    { erase(--cend()); }

    void
    pop_front() noexcept
    { erase(cbegin()); }

    iterator
    insert(const_iterator pos, reference value) noexcept;

    iterator
    insert(const_iterator pos, iterator first, iterator last) noexcept;

    iterator
    erase(const_iterator pos) noexcept;

    iterator
    erase(const_iterator first, const_iterator last) noexcept;

    void
    clear() noexcept;

    void splice(const_iterator pos, List<T, member>& other) noexcept
    { pos.get()->splice(other.begin().get(), other.end().get()); }

    // Miscellaneous methods

    bool empty() const noexcept
    { return cbegin() == cend(); }

private:
    List_hook  m_hook;
};


template<class T, List_hook T::* member>
auto
List<T, member>::insert(const_iterator pos,
                        reference value) noexcept -> iterator
{
    pos.get()->link(detail::member_from_pointer<T, List_hook, member>(&value));
    return iterator(pos.get()->m_prev);
}


template<class T, List_hook T::* member>
auto
List<T, member>::insert(const_iterator pos,
                        iterator first,
                        iterator last) noexcept -> iterator
{
    if (first == last) {
        return iterator(pos.get());
    }
    while (first != last) {
        first = insert(pos, *first);
    }
    return first;
}


template<class T, List_hook T::* member>
auto
List<T, member>::erase(const_iterator pos) noexcept -> iterator
{
    if (pos == cend()) {
        return end();
    }
    iterator tmp(pos.get()->m_next);
    pos.get()->unlink();
    return tmp;
}


template<class T, List_hook T::* member>
auto
List<T, member>::erase(const_iterator first,
                       const_iterator last) noexcept -> iterator
{
    while (first != last) {
        first = erase(first);
    }
    return iterator(first.get());
}


template<class T, List_hook T::* member>
void
List<T, member>::clear() noexcept
{
    List_hook* node = &m_hook;
    do {
        List_hook* next_node = node->m_next;
        node->m_prev = node->m_next = node;
        node = next_node;
    } while (node != &m_hook);
}


}} // namespace bemu::intrusive

#endif // BEMU_INTRUSIVE_LIST_H
