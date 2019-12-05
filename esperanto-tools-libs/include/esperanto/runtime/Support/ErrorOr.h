//===- llvm/Support/Error.h - Recoverable error handling --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

//******************************************************************************
// Copyright (C) 2019, Esperanto Technologies Inc.
// The copyright to the computer program(s) herein is the
// property of Esperanto Technologies, Inc. All Rights Reserved.
// The program(s) may be used and/or copied only with
// the written permission of Esperanto Technologies and
// in accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been supplied.
//------------------------------------------------------------------------------

//===----------------------------------------------------------------------===//
//
// This file defines an API used to report recoverable errors.
//
//===----------------------------------------------------------------------===//

#ifndef ET_RUNTIME_ERROR_OR_H
#define ET_RUNTIME_ERROR_OR_H

// @file

#include "esperanto/runtime/Support/RuntimeErrors.h"

#include <cassert>
#include <system_error>
#include <type_traits>
#include <utility>

namespace et_runtime {

/// @class ErrorOr
/// Represents either an error or a value T.
///
/// ErrorOr<T> is a pointer-like class that represents the result of an
/// operation. The result is either an error, or a value of type T. This is
/// designed to emulate the usage of returning a pointer where nullptr indicates
/// failure. However instead of just knowing that the operation failed, we also
/// have an error_code and optional user data that describes why it failed.
///
/// It is used like the following.
/// \code
///   ErrorOr<Buffer> getBuffer();
///
///   auto buffer = getBuffer();
///   if (error_code ec = buffer.getError())
///     return ec;
///   buffer->write("adena");
/// \endcode
///
///
/// Implicit conversion to bool returns true if there is a usable value. The
/// unary * and -> operators provide pointer like access to the value. Accessing
/// the value when there is an error has undefined behavior.
///
/// When T is a reference type the behavior is slightly different. The reference
/// is held in a std::reference_wrapper<std::remove_reference<T>::type>, and
/// there is special handling to make operator -> work as if T was not a
/// reference.
///
/// @tparam T:  T cannot be a rvalue reference.
template <class T> class ErrorOr {
  template <class OtherT> friend class ErrorOr;

  static const bool isRef = std::is_reference<T>::value;

  using wrap = std::reference_wrapper<typename std::remove_reference<T>::type>;

public:
  using storage_type = typename std::conditional<isRef, wrap, T>::type;

private:
  using reference = typename std::remove_reference<T>::type &;
  using const_reference = const typename std::remove_reference<T>::type &;
  using pointer = typename std::remove_reference<T>::type *;
  using const_pointer = const typename std::remove_reference<T>::type *;

public:
  ErrorOr(etrtError error) : HasError(true), error_type_(error) {}

  template <class OtherT>
  ErrorOr(OtherT &&Val,
          typename std::enable_if<std::is_convertible<OtherT, T>::value>::type
              * = nullptr)
      : HasError(false) {
    new (getStorage()) storage_type(std::forward<OtherT>(Val));
  }

  ErrorOr(const ErrorOr &Other) { copyConstruct(Other); }

  template <class OtherT>
  ErrorOr(const ErrorOr<OtherT> &Other,
          typename std::enable_if<std::is_convertible<OtherT, T>::value>::type
              * = nullptr) {
    copyConstruct(Other);
  }

  template <class OtherT>
  explicit ErrorOr(
      const ErrorOr<OtherT> &Other,
      typename std::enable_if<
          !std::is_convertible<OtherT, const T &>::value>::type * = nullptr) {
    copyConstruct(Other);
  }

  ErrorOr(ErrorOr &&Other) { moveConstruct(std::move(Other)); }

  template <class OtherT>
  ErrorOr(ErrorOr<OtherT> &&Other,
          typename std::enable_if<std::is_convertible<OtherT, T>::value>::type
              * = nullptr) {
    moveConstruct(std::move(Other));
  }

  // This might eventually need SFINAE but it's more complex than is_convertible
  // & I'm too lazy to write it right now.
  template <class OtherT>
  explicit ErrorOr(
      ErrorOr<OtherT> &&Other,
      typename std::enable_if<!std::is_convertible<OtherT, T>::value>::type * =
          nullptr) {
    moveConstruct(std::move(Other));
  }

  ErrorOr &operator=(const ErrorOr &Other) {
    copyAssign(Other);
    return *this;
  }

  ErrorOr &operator=(ErrorOr &&Other) {
    moveAssign(std::move(Other));
    return *this;
  }

  ~ErrorOr() {
    if (!HasError)
      getStorage()->~storage_type();
  }

  /// Return false if there is an error.
  explicit operator bool() const { return !HasError; }

  reference get() { return *getStorage(); }
  const_reference get() const { return const_cast<ErrorOr<T> *>(this)->get(); }

  etrtError getError() const { return HasError ? error_type_ : etrtSuccess; }

  pointer operator->() { return toPointer(getStorage()); }

  const_pointer operator->() const { return toPointer(getStorage()); }

  reference operator*() { return *getStorage(); }

  const_reference operator*() const { return *getStorage(); }

private:
  template <class OtherT> void copyConstruct(const ErrorOr<OtherT> &Other) {
    if (!Other.HasError) {
      // Get the other value.
      HasError = false;
      new (getStorage()) storage_type(*Other.getStorage());
    } else {
      // Get other's error.
      HasError = true;
      error_type_ = Other.getError();
    }
  }

  template <class T1>
  static bool compareThisIfSameType(const T1 &a, const T1 &b) {
    return &a == &b;
  }

  template <class T1, class T2>
  static bool compareThisIfSameType(const T1 &a, const T2 &b) {
    return false;
  }

  template <class OtherT> void copyAssign(const ErrorOr<OtherT> &Other) {
    if (compareThisIfSameType(*this, Other))
      return;

    this->~ErrorOr();
    new (this) ErrorOr(Other);
  }

  template <class OtherT> void moveConstruct(ErrorOr<OtherT> &&Other) {
    if (!Other.HasError) {
      // Get the other value.
      HasError = false;
      new (getStorage()) storage_type(std::move(*Other.getStorage()));
    } else {
      // Get other's error.
      HasError = true;
      error_type_ = Other.getError();
    }
  }

  template <class OtherT> void moveAssign(ErrorOr<OtherT> &&Other) {
    if (compareThisIfSameType(*this, Other))
      return;

    this->~ErrorOr();
    new (this) ErrorOr(std::move(Other));
  }

  pointer toPointer(pointer Val) { return Val; }

  const_pointer toPointer(const_pointer Val) const { return Val; }

  pointer toPointer(wrap *Val) { return &Val->get(); }

  const_pointer toPointer(const wrap *Val) const { return &Val->get(); }

  storage_type *getStorage() {
    assert(!HasError && "Cannot get value when an error exists!");
    return reinterpret_cast<storage_type *>(&TStorage);
  }

  const storage_type *getStorage() const {
    assert(!HasError && "Cannot get value when an error exists!");
    return reinterpret_cast<const storage_type *>(&TStorage);
  }

  bool HasError : 1;
  union {
    storage_type TStorage;
    etrtError error_type_;
  };
};

template <class T, class E>
typename std::enable_if<std::is_error_code_enum<E>::value ||
                            std::is_error_condition_enum<E>::value,
                        bool>::type
operator==(const ErrorOr<T> &Err, E Code) {
  return Err.getError() == Code;
}

} // namespace et_runtime

#endif // ET_RUNTIME_ERROR_OR_H
