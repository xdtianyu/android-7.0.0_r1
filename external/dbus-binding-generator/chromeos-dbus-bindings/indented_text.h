// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BINDINGS_INDENTED_TEXT_H_
#define CHROMEOS_DBUS_BINDINGS_INDENTED_TEXT_H_

#include <string>
#include <utility>
#include <vector>

#include <base/macros.h>

namespace chromeos_dbus_bindings {

class IndentedText {
 public:
  IndentedText();
  virtual ~IndentedText() = default;

  // Insert a blank line.
  void AddBlankLine();

  // Insert a block of indented text.
  void AddBlock(const IndentedText& block);
  void AddBlockWithOffset(const IndentedText& block, size_t shift);

  // Add a line at the current indentation.
  void AddLine(const std::string& line);
  void AddLineWithOffset(const std::string& line, size_t shift);
  // Adds a line and pushes an offset past the |nth_occurrence| of character |c|
  // in that line, effectively allowing to align following line to the position
  // following that character.
  void AddLineAndPushOffsetTo(const std::string& line,
                              size_t nth_occurrence,
                              char c);

  // Adds a block of comments.
  void AddComments(const std::string& doc_string);

  // Return a string representing the indented text.
  std::string GetContents() const;

  // Return a list of lines representing the intended indented text, not
  // including the \n.
  std::vector<std::string> GetLines() const;

  // Add or remove an offset to the current stack of indentation offsets.
  void PushOffset(size_t shift);
  void PopOffset();

  // Reset to initial state.
  void Reset();


 private:
  using IndentedLine = std::pair<std::string, size_t>;

  friend class IndentedTextTest;

  size_t offset_;
  std::vector<size_t> offset_history_;
  std::vector<IndentedLine> contents_;

  DISALLOW_COPY_AND_ASSIGN(IndentedText);
};

}  // namespace chromeos_dbus_bindings

#endif  // CHROMEOS_DBUS_BINDINGS_INDENTED_TEXT_H_
