// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.

#ifndef BASE_STRINGS_STRING_UTIL_H_
#define BASE_STRINGS_STRING_UTIL_H_

#include <ctype.h>
#include <stdarg.h>   // va_list
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/strings/string_piece.h"  // For implicit conversions.
#include "build/build_config.h"

// On Android, bionic's stdio.h defines an snprintf macro when being built with
// clang. Undefine it here so it won't collide with base::snprintf().
#undef snprintf

namespace base {

// C standard-library functions that aren't cross-platform are provided as
// "base::...", and their prototypes are listed below. These functions are
// then implemented as inline calls to the platform-specific equivalents in the
// platform-specific headers.

// Wrapper for vsnprintf that always null-terminates and always returns the
// number of characters that would be in an untruncated formatted
// string, even when truncation occurs.
int vsnprintf(char* buffer, size_t size, const char* format, va_list arguments)
    PRINTF_FORMAT(3, 0);

// Some of these implementations need to be inlined.

// We separate the declaration from the implementation of this inline
// function just so the PRINTF_FORMAT works.
inline int snprintf(char* buffer,
                    size_t size,
                    _Printf_format_string_ const char* format,
                    ...) PRINTF_FORMAT(3, 4);
inline int snprintf(char* buffer,
                    size_t size,
                    _Printf_format_string_ const char* format,
                    ...) {
  va_list arguments;
  va_start(arguments, format);
  int result = vsnprintf(buffer, size, format, arguments);
  va_end(arguments);
  return result;
}

// BSD-style safe and consistent string copy functions.
// Copies |src| to |dst|, where |dst_size| is the total allocated size of |dst|.
// Copies at most |dst_size|-1 characters, and always NULL terminates |dst|, as
// long as |dst_size| is not 0.  Returns the length of |src| in characters.
// If the return value is >= dst_size, then the output was truncated.
// NOTE: All sizes are in number of characters, NOT in bytes.
size_t strlcpy(char* dst, const char* src, size_t dst_size);

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
inline char ToLowerASCII(char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// ASCII-specific toupper.  The standard library's toupper is locale sensitive,
// so we don't want to use it here.
inline char ToUpperASCII(char c) {
  return (c >= 'a' && c <= 'z') ? (c + ('A' - 'a')) : c;
}
// Converts the given string to it's ASCII-lowercase equivalent.
std::string ToLowerASCII(StringPiece str);
// Converts the given string to it's ASCII-uppercase equivalent.
std::string ToUpperASCII(StringPiece str);

// Functor for case-insensitive ASCII comparisons for STL algorithms like
// std::search.
//
// Note that a full Unicode version of this functor is not possible to write
// because case mappings might change the number of characters, depend on
// context (combining accents), and require handling UTF-16. If you need
// proper Unicode support, use base::i18n::ToLower/FoldCase and then just
// use a normal operator== on the result.
template<typename Char> struct CaseInsensitiveCompareASCII {
 public:
  bool operator()(Char x, Char y) const {
    return ToLowerASCII(x) == ToLowerASCII(y);
  }
};

// Like strcasecmp for case-insensitive ASCII characters only. Returns:
//   -1  (a < b)
//    0  (a == b)
//    1  (a > b)
// (unlike strcasecmp which can return values greater or less than 1/-1). For
// full Unicode support, use base::i18n::ToLower or base::i18h::FoldCase
// and then just call the normal string operators on the result.
int CompareCaseInsensitiveASCII(StringPiece a, StringPiece b);

// Equality for ASCII case-insensitive comparisons. For full Unicode support,
// use base::i18n::ToLower or base::i18h::FoldCase and then compare with either
// == or !=.
bool EqualsCaseInsensitiveASCII(StringPiece a, StringPiece b);

// Contains the set of characters representing whitespace in the corresponding
// encoding. Null-terminated. The ASCII versions are the whitespaces as defined
// by HTML5, and don't include control characters.
extern const char kWhitespaceASCII[];

// Replaces characters in |replace_chars| from anywhere in |input| with
// |replace_with|.  Each character in |replace_chars| will be replaced with
// the |replace_with| string.  Returns true if any characters were replaced.
// |replace_chars| must be null-terminated.
// NOTE: Safe to use the same variable for both |input| and |output|.
bool ReplaceChars(const std::string& input,
                  const StringPiece& replace_chars,
                  const std::string& replace_with,
                  std::string* output);

enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};

// Removes characters in |trim_chars| from the beginning and end of |input|.
// The 8-bit version only works on 8-bit characters, not UTF-8.
//
// It is safe to use the same variable for both |input| and |output| (this is
// the normal usage to trim in-place).
bool TrimString(const std::string& input,
                StringPiece trim_chars,
                std::string* output);

// StringPiece versions of the above. The returned pieces refer to the original
// buffer.
StringPiece TrimString(StringPiece input,
                       const StringPiece& trim_chars,
                       TrimPositions positions);

// Trims any whitespace from either end of the input string.
//
// The StringPiece versions return a substring referencing the input buffer.
// The ASCII versions look only for ASCII whitespace.
//
// The std::string versions return where whitespace was found.
// NOTE: Safe to use the same variable for both input and output.
TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output);

// Returns true if the specified string matches the criteria. How can a wide
// string be 8-bit or UTF8? It contains only characters that are < 256 (in the
// first case) or characters that use only 8-bits and whose 8-bit
// representation looks like a UTF-8 string (the second case).
//
// Note that IsStringUTF8 checks not only if the input is structurally
// valid but also if it doesn't contain any non-character codepoint
// (e.g. U+FFFE). It's done on purpose because all the existing callers want
// to have the maximum 'discriminating' power from other encodings. If
// there's a use case for just checking the structural validity, we have to
// add a new function for that.
//
// IsStringASCII assumes the input is likely all ASCII, and does not leave early
// if it is not the case.
bool IsStringUTF8(const StringPiece& str);
bool IsStringASCII(const StringPiece& str);

}  // namespace base

#if defined(OS_WIN)
#include "base/strings/string_util_win.h"
#elif defined(OS_POSIX)
#include "base/strings/string_util_posix.h"
#else
#error Define string operations appropriately for your platform
#endif

#endif  // BASE_STRINGS_STRING_UTIL_H_
