// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/type_name_undecorate.h>

#include <cstring>

#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#endif  // __GNUG__

namespace brillo {

std::string UndecorateTypeName(const char* type_name) {
#ifdef __GNUG__
  // Under g++ use abi::__cxa_demangle() to undecorate the type name.
  int status = 0;

  std::unique_ptr<char, decltype(&std::free)> res{
      abi::__cxa_demangle(type_name, nullptr, nullptr, &status),
      std::free
  };

  return (status == 0) ? res.get() : type_name;
#else
  // If not compiled with g++, do nothing...
  // E.g. MSVC's type_info::name() already contains undecorated name.
  return type_name;
#endif
}

std::string GetUndecoratedTypeNameForTag(const char* type_tag) {
#if defined(USE_RTTI_FOR_TYPE_TAGS) && \
    (defined(__cpp_rtti) || defined(__GXX_RTTI))
  return UndecorateTypeName(type_tag);
#else
  // The signature of type tag for, say, 'int' would be the following:
  //    const char *brillo::GetTypeTag() [T = int]
  // So we just need to extract the type name between '[T = ' and ']'.
  const char* token = " = ";
  const char* pos = std::strstr(type_tag, token);
  if (!pos)
    return type_tag;
  std::string name = pos + std::strlen(token);
  if (!name.empty() && name.back() == ']')
    name.pop_back();
  return name;
#endif
}

// Implementations of the explicitly instantiated GetTypeTag<T>() for common
// types.
template const char* GetTypeTag<int8_t>();
template const char* GetTypeTag<uint8_t>();
template const char* GetTypeTag<int16_t>();
template const char* GetTypeTag<uint16_t>();
template const char* GetTypeTag<int32_t>();
template const char* GetTypeTag<uint32_t>();
template const char* GetTypeTag<int64_t>();
template const char* GetTypeTag<uint64_t>();
template const char* GetTypeTag<bool>();
template const char* GetTypeTag<double>();
template const char* GetTypeTag<std::string>();

}  // namespace brillo
