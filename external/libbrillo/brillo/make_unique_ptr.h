// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MAKE_UNIQUE_PTR_H_
#define LIBBRILLO_BRILLO_MAKE_UNIQUE_PTR_H_

#include <memory>

namespace brillo {

// A function to convert T* into unique_ptr<T>
// Doing e.g. make_unique_ptr(new FooBarBaz<type>(arg)) is a shorter notation
// for unique_ptr<FooBarBaz<type>>(new FooBarBaz<type>(arg))
// Basically the same as Chromium's make_scoped_ptr().
// Deliberately not named "make_unique" to avoid conflicting with the similar,
// but more complex and semantically different C++14 function.
template <typename T>
std::unique_ptr<T> make_unique_ptr(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MAKE_UNIQUE_PTR_H_
