// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/indented_text.h"

#include <string>
#include <utility>
#include <vector>

#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/strings/string_utils.h>

using std::string;
using std::vector;

namespace chromeos_dbus_bindings {

IndentedText::IndentedText() : offset_(0) {}

void IndentedText::AddBlankLine() {
  AddLine("");
}

void IndentedText::AddBlock(const IndentedText& block) {
  AddBlockWithOffset(block, 0);
}

void IndentedText::AddBlockWithOffset(const IndentedText& block, size_t shift) {
  for (const auto& member : block.contents_) {
    AddLineWithOffset(member.first, member.second + shift);
  }
}

void IndentedText::AddLine(const std::string& line) {
  AddLineWithOffset(line, 0);
}

void IndentedText::AddLineWithOffset(const std::string& line, size_t shift) {
  contents_.emplace_back(line, shift + offset_);
}

void IndentedText::AddLineAndPushOffsetTo(const std::string& line,
                                          size_t occurrence,
                                          char c) {
  AddLine(line);
  size_t pos = 0;
  while (occurrence > 0) {
    pos = line.find(c, pos);
    CHECK(pos != string::npos);
    pos++;
    occurrence--;
  }
  PushOffset(pos);
}

void IndentedText::AddComments(const std::string& doc_string) {
  // Try to retain indentation in the comments. Find the first non-empty line
  // of the comment and find its whitespace indentation prefix.
  // For all subsequent lines, remove the same whitespace prefix as found
  // at the first line of the comment but keep any additional spaces to
  // maintain the comment layout.
  auto lines = brillo::string_utils::Split(doc_string, "\n", false, false);
  vector<string> lines_out;
  lines_out.reserve(lines.size());
  bool first_nonempty_found = false;
  std::string trim_prefix;
  for (string line : lines) {
    base::TrimWhitespaceASCII(line, base::TRIM_TRAILING, &line);
    if (!first_nonempty_found) {
      size_t pos = line.find_first_not_of(" \t");
      if (pos != std::string::npos) {
        first_nonempty_found = true;
        trim_prefix = line.substr(0, pos);
        lines_out.push_back(line.substr(pos));
      }
    } else {
      if (base::StartsWith(line, trim_prefix,
                           base::CompareCase::INSENSITIVE_ASCII)) {
        line = line.substr(trim_prefix.length());
      } else {
        base::TrimWhitespaceASCII(line, base::TRIM_LEADING, &line);
      }
      lines_out.push_back(line);
    }
  }

  // We already eliminated all empty lines at the beginning of the comment
  // block. Now remove the trailing empty lines.
  while (!lines_out.empty() && lines_out.back().empty())
    lines_out.pop_back();

  for (const string& line : lines_out) {
    const bool all_whitespace = (line.find_first_not_of(" \t") == string::npos);
    if (all_whitespace) {
      AddLine("//");
    } else {
      AddLine("// " + line);
    }
  }
}

string IndentedText::GetContents() const {
  string output;
  for (const string& line : GetLines()) {
    output.append(line);
    output.append("\n");
  }
  return output;
}

std::vector<std::string> IndentedText::GetLines() const {
  vector<string> result;
  for (const auto& member : contents_) {
    const string& line = member.first;
    size_t shift = line.empty() ? 0 : member.second;
    string indent(shift, ' ');
    result.push_back(indent + line);
  }
  return result;
}

void IndentedText::PushOffset(size_t shift) {
  offset_ += shift;
  offset_history_.push_back(shift);
}

void IndentedText::PopOffset() {
  CHECK(!offset_history_.empty());
  offset_ -= offset_history_.back();
  offset_history_.pop_back();
}

void IndentedText::Reset() {
  offset_ = 0;
  offset_history_.clear();
  contents_.clear();
}

}  // namespace chromeos_dbus_bindings
