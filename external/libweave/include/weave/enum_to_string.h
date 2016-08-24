// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_ENUM_TO_STRING_H_
#define LIBWEAVE_INCLUDE_WEAVE_ENUM_TO_STRING_H_

#include <string>

#include <base/logging.h>

namespace weave {

// Helps to map enumeration to stings and back.
//
// Usage example:
// .h file:
// enum class MyEnum { kV1, kV2 };
//
// .cc file:
// template <>
// EnumToStringMap<MyEnum>::EnumToStringMap() : EnumToStringMap(kMap) { };
template <typename T>
class EnumToStringMap final {
  static_assert(std::is_enum<T>::value, "The type must be an enumeration");

 public:
  struct Map {
    const T id;
    const char* const name;
  };

  EnumToStringMap();

  const Map* begin() const { return begin_; }
  const Map* end() const { return end_; }

 private:
  template <size_t size>
  explicit EnumToStringMap(const Map (&map)[size])
      : begin_(map), end_(map + size) {}

  const Map* begin_;
  const Map* end_;
};

template <typename T>
std::string EnumToString(T id) {
  for (const auto& m : EnumToStringMap<T>()) {
    if (m.id == id) {
      CHECK(m.name);
      return m.name;
    }
  }
  NOTREACHED() << static_cast<int>(id);
  return std::string();
}

template <typename T>
bool StringToEnum(const std::string& name, T* id) {
  for (const auto& m : EnumToStringMap<T>()) {
    if (m.name && m.name == name) {
      *id = m.id;
      return true;
    }
  }
  return false;
}

}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_ENUM_TO_STRING_H_
