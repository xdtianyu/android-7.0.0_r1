// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEMPLATE_UTIL_H_
#define BASE_TEMPLATE_UTIL_H_

#include <stddef.h>
#include <type_traits>

#include "build/build_config.h"

namespace base {

template <class T> struct is_non_const_reference : std::false_type {};
template <class T> struct is_non_const_reference<T&> : std::true_type {};
template <class T> struct is_non_const_reference<const T&> : std::false_type {};

namespace internal {

// Types YesType and NoType are guaranteed such that sizeof(YesType) <
// sizeof(NoType).
typedef char YesType;

struct NoType {
  YesType dummy[2];
};

}  // namespace internal

}  // namespace base

#endif  // BASE_TEMPLATE_UTIL_H_
