// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_MAP_UTILS_H_
#define LIBBRILLO_BRILLO_MAP_UTILS_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

namespace brillo {

// Given an STL map, returns a set containing all keys from the map.
template<typename T>
inline std::set<typename T::key_type> GetMapKeys(const T& map) {
  std::set<typename T::key_type> keys;
  for (const auto& pair : map)
    keys.insert(keys.end(), pair.first);  // Map keys are already sorted.
  return keys;
}

// Given an STL map, returns a vector containing all keys from the map.
// The keys in the vector are sorted.
template<typename T>
inline std::vector<typename T::key_type> GetMapKeysAsVector(const T& map) {
  std::vector<typename T::key_type> keys;
  keys.reserve(map.size());
  for (const auto& pair : map)
    keys.push_back(pair.first);
  return keys;
}

// Given an STL map, returns a vector containing all values from the map.
template<typename T>
inline std::vector<typename T::mapped_type> GetMapValues(const T& map) {
  std::vector<typename T::mapped_type> values;
  values.reserve(map.size());
  for (const auto& pair : map)
    values.push_back(pair.second);
  return values;
}

// Given an STL map, returns a vector of key-value pairs from the map.
template<typename T>
inline std::vector<std::pair<typename T::key_type, typename T::mapped_type>>
MapToVector(const T& map) {
  std::vector<std::pair<typename T::key_type, typename T::mapped_type>> vector;
  vector.reserve(map.size());
  for (const auto& pair : map)
    vector.push_back(pair);
  return vector;
}

// Given an STL map, returns the value associated with a given key or a default
// value if the key is not present in the map.
template<typename T>
inline typename T::mapped_type GetOrDefault(
    const T& map,
    typename T::key_type key,
    const typename T::mapped_type& def) {
  typename T::const_iterator it = map.find(key);
  if (it == map.end())
    return def;
  return it->second;
}

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_MAP_UTILS_H_
