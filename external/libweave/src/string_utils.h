// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_SRC_STRING_UTILS_H_
#define LIBWEAVE_SRC_STRING_UTILS_H_

#include <string>
#include <utility>
#include <vector>

namespace weave {

// Treats the string as a delimited list of substrings and returns the array
// of original elements of the list.
// |trim_whitespaces| causes each element to have all whitespaces trimmed off.
// |purge_empty_strings| specifies whether empty elements from the original
// string should be omitted.
std::vector<std::string> Split(const std::string& str,
                               const std::string& delimiter,
                               bool trim_whitespaces,
                               bool purge_empty_strings);

// Splits the string into two pieces at the first position of the specified
// delimiter.
std::pair<std::string, std::string> SplitAtFirst(const std::string& str,
                                                 const std::string& delimiter,
                                                 bool trim_whitespaces);

// Joins strings into a single string separated by |delimiter|.
template <class InputIterator>
std::string JoinRange(const std::string& delimiter,
                      InputIterator first,
                      InputIterator last) {
  std::string result;
  if (first == last)
    return result;
  result = *first;
  for (++first; first != last; ++first) {
    result += delimiter;
    result += *first;
  }
  return result;
}

template <class Container>
std::string Join(const std::string& delimiter, const Container& strings) {
  using std::begin;
  using std::end;
  return JoinRange(delimiter, begin(strings), end(strings));
}

inline std::string Join(const std::string& delimiter,
                        const std::string& str1,
                        const std::string& str2) {
  return str1 + delimiter + str2;
}

}  // namespace weave

#endif  // LIBWEAVE_SRC_STRING_UTILS_H_
