// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <algorithm>
#include <utility>

#include <base/strings/string_util.h>

#include "src/string_utils.h"

namespace weave {

namespace {

bool SplitAtFirst(const std::string& str,
                  const std::string& delimiter,
                  std::string* left_part,
                  std::string* right_part,
                  bool trim_whitespaces) {
  bool delimiter_found = false;
  std::string::size_type pos = str.find(delimiter);
  if (pos != std::string::npos) {
    *left_part = str.substr(0, pos);
    *right_part = str.substr(pos + delimiter.size());
    delimiter_found = true;
  } else {
    *left_part = str;
    right_part->clear();
  }

  if (trim_whitespaces) {
    base::TrimWhitespaceASCII(*left_part, base::TRIM_ALL, left_part);
    base::TrimWhitespaceASCII(*right_part, base::TRIM_ALL, right_part);
  }

  return delimiter_found;
}

}  // namespace

std::vector<std::string> Split(const std::string& str,
                               const std::string& delimiter,
                               bool trim_whitespaces,
                               bool purge_empty_strings) {
  std::vector<std::string> tokens;
  for (std::string::size_type i = 0;;) {
    const std::string::size_type pos =
        delimiter.empty() ? (i + 1) : str.find(delimiter, i);
    std::string tmp_str{str.substr(i, pos - i)};
    if (trim_whitespaces)
      base::TrimWhitespaceASCII(tmp_str, base::TRIM_ALL, &tmp_str);
    if (!tmp_str.empty() || !purge_empty_strings)
      tokens.emplace_back(std::move(tmp_str));
    if (pos >= str.size())
      break;
    i = pos + delimiter.size();
  }
  return tokens;
}

std::pair<std::string, std::string> SplitAtFirst(const std::string& str,
                                                 const std::string& delimiter,
                                                 bool trim_whitespaces) {
  std::pair<std::string, std::string> pair;
  SplitAtFirst(str, delimiter, &pair.first, &pair.second, trim_whitespaces);
  return pair;
}

}  // namespace weave
