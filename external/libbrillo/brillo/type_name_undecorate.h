// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_TYPE_NAME_UNDECORATE_H_
#define LIBBRILLO_BRILLO_TYPE_NAME_UNDECORATE_H_

#include <string>
#include <typeinfo>

#include <brillo/brillo_export.h>

#if !defined(USE_RTTI_FOR_TYPE_TAGS) && !defined(__clang__)
// When type information is used with RTTI disabled, we rely on
// __PRETTY_FUNCTION__ macro for type tags. Unfortunately gcc and clang produce
// different signatures for types that have optional template parameters, such
// as std::vector and std::map. The problem arises when inter-operating between
// libraries that are compiled with different compilers.
// Since most of Brillo is compiled with clang, we choose clang here exclusively
// and prevent this code from compiling with GCC to avoid hidden runtime errors.
#error TypeInfo/Any with RTTI disabled is supported on clang compiler only.
#endif

namespace brillo {

template<typename T>
const char* GetTypeTag() {
#if defined(USE_RTTI_FOR_TYPE_TAGS) && \
    (defined(__cpp_rtti) || defined(__GXX_RTTI))
  return typeid(T).name();
#else
  // __PRETTY_FUNCTION__ would include the type T signature and therefore each
  // instance of brillo::internal_details::GetTypeTag<T>() will have a different
  // tag string.
  return __PRETTY_FUNCTION__;
#endif
}

// Explicitly instantiate GetTypeTag<T>() for common types to minimize static
// data segment pollution.
extern template BRILLO_EXPORT const char* GetTypeTag<int8_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<uint8_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<int16_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<uint16_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<int32_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<uint32_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<int64_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<uint64_t>();
extern template BRILLO_EXPORT const char* GetTypeTag<bool>();
extern template BRILLO_EXPORT const char* GetTypeTag<double>();
extern template BRILLO_EXPORT const char* GetTypeTag<std::string>();

// Use brillo::UndecorateTypeName() to obtain human-readable type from
// the decorated/mangled type name returned by std::type_info::name().
BRILLO_EXPORT std::string UndecorateTypeName(const char* type_name);

// Returns undecorated type name for the given type tag. This will extract the
// actual type name from the type tag string.
BRILLO_EXPORT std::string GetUndecoratedTypeNameForTag(const char* type_tag);

// A template helper function that returns the undecorated type name for type T.
template<typename T>
inline std::string GetUndecoratedTypeName() {
  return GetUndecoratedTypeNameForTag(GetTypeTag<T>());
}

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_TYPE_NAME_UNDECORATE_H_
