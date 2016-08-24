// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "base/macros.h"
#include "base/strings/utf_string_conversion_utils.h"

using ::testing::ElementsAre;

namespace base {

TEST(StringUtilTest, IsStringUTF8) {
  EXPECT_TRUE(IsStringUTF8("abc"));
  EXPECT_TRUE(IsStringUTF8("\xc2\x81"));
  EXPECT_TRUE(IsStringUTF8("\xe1\x80\xbf"));
  EXPECT_TRUE(IsStringUTF8("\xf1\x80\xa0\xbf"));
  EXPECT_TRUE(IsStringUTF8("a\xc2\x81\xe1\x80\xbf\xf1\x80\xa0\xbf"));
  EXPECT_TRUE(IsStringUTF8("\xef\xbb\xbf" "abc"));  // UTF-8 BOM

  // surrogate code points
  EXPECT_FALSE(IsStringUTF8("\xed\xa0\x80\xed\xbf\xbf"));
  EXPECT_FALSE(IsStringUTF8("\xed\xa0\x8f"));
  EXPECT_FALSE(IsStringUTF8("\xed\xbf\xbf"));

  // overlong sequences
  EXPECT_FALSE(IsStringUTF8("\xc0\x80"));  // U+0000
  EXPECT_FALSE(IsStringUTF8("\xc1\x80\xc1\x81"));  // "AB"
  EXPECT_FALSE(IsStringUTF8("\xe0\x80\x80"));  // U+0000
  EXPECT_FALSE(IsStringUTF8("\xe0\x82\x80"));  // U+0080
  EXPECT_FALSE(IsStringUTF8("\xe0\x9f\xbf"));  // U+07ff
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\x80\x8D"));  // U+000D
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\x82\x91"));  // U+0091
  EXPECT_FALSE(IsStringUTF8("\xf0\x80\xa0\x80"));  // U+0800
  EXPECT_FALSE(IsStringUTF8("\xf0\x8f\xbb\xbf"));  // U+FEFF (BOM)
  EXPECT_FALSE(IsStringUTF8("\xf8\x80\x80\x80\xbf"));  // U+003F
  EXPECT_FALSE(IsStringUTF8("\xfc\x80\x80\x80\xa0\xa5"));  // U+00A5

  // Beyond U+10FFFF (the upper limit of Unicode codespace)
  EXPECT_FALSE(IsStringUTF8("\xf4\x90\x80\x80"));  // U+110000
  EXPECT_FALSE(IsStringUTF8("\xf8\xa0\xbf\x80\xbf"));  // 5 bytes
  EXPECT_FALSE(IsStringUTF8("\xfc\x9c\xbf\x80\xbf\x80"));  // 6 bytes

  // BOMs in UTF-16(BE|LE) and UTF-32(BE|LE)
  EXPECT_FALSE(IsStringUTF8("\xfe\xff"));
  EXPECT_FALSE(IsStringUTF8("\xff\xfe"));
  EXPECT_FALSE(IsStringUTF8(std::string("\x00\x00\xfe\xff", 4)));
  EXPECT_FALSE(IsStringUTF8("\xff\xfe\x00\x00"));

  // Non-characters : U+xxFFF[EF] where xx is 0x00 through 0x10 and <FDD0,FDEF>
  EXPECT_FALSE(IsStringUTF8("\xef\xbf\xbe"));  // U+FFFE)
  EXPECT_FALSE(IsStringUTF8("\xf0\x8f\xbf\xbe"));  // U+1FFFE
  EXPECT_FALSE(IsStringUTF8("\xf3\xbf\xbf\xbf"));  // U+10FFFF
  EXPECT_FALSE(IsStringUTF8("\xef\xb7\x90"));  // U+FDD0
  EXPECT_FALSE(IsStringUTF8("\xef\xb7\xaf"));  // U+FDEF
  // Strings in legacy encodings. We can certainly make up strings
  // in a legacy encoding that are valid in UTF-8, but in real data,
  // most of them are invalid as UTF-8.
  EXPECT_FALSE(IsStringUTF8("caf\xe9"));  // cafe with U+00E9 in ISO-8859-1
  EXPECT_FALSE(IsStringUTF8("\xb0\xa1\xb0\xa2"));  // U+AC00, U+AC001 in EUC-KR
  EXPECT_FALSE(IsStringUTF8("\xa7\x41\xa6\x6e"));  // U+4F60 U+597D in Big5
  // "abc" with U+201[CD] in windows-125[0-8]
  EXPECT_FALSE(IsStringUTF8("\x93" "abc\x94"));
  // U+0639 U+064E U+0644 U+064E in ISO-8859-6
  EXPECT_FALSE(IsStringUTF8("\xd9\xee\xe4\xee"));
  // U+03B3 U+03B5 U+03B9 U+03AC in ISO-8859-7
  EXPECT_FALSE(IsStringUTF8("\xe3\xe5\xe9\xdC"));

  // Check that we support Embedded Nulls. The first uses the canonical UTF-8
  // representation, and the second uses a 2-byte sequence. The second version
  // is invalid UTF-8 since UTF-8 states that the shortest encoding for a
  // given codepoint must be used.
  static const char kEmbeddedNull[] = "embedded\0null";
  EXPECT_TRUE(IsStringUTF8(
      std::string(kEmbeddedNull, sizeof(kEmbeddedNull))));
  EXPECT_FALSE(IsStringUTF8("embedded\xc0\x80U+0000"));
}

TEST(StringUtilTest, IsStringASCII) {
  static char char_ascii[] =
      "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
  static std::wstring wchar_ascii(
      L"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF");

  // Test a variety of the fragment start positions and lengths in order to make
  // sure that bit masking in IsStringASCII works correctly.
  // Also, test that a non-ASCII character will be detected regardless of its
  // position inside the string.
  {
    const size_t string_length = arraysize(char_ascii) - 1;
    for (size_t offset = 0; offset < 8; ++offset) {
      for (size_t len = 0, max_len = string_length - offset; len < max_len;
           ++len) {
        EXPECT_TRUE(IsStringASCII(StringPiece(char_ascii + offset, len)));
        for (size_t char_pos = offset; char_pos < len; ++char_pos) {
          char_ascii[char_pos] |= '\x80';
          EXPECT_FALSE(IsStringASCII(StringPiece(char_ascii + offset, len)));
          char_ascii[char_pos] &= ~'\x80';
        }
      }
    }
  }
}

TEST(StringUtilTest, ReplaceChars) {
  struct TestData {
    const char* input;
    const char* replace_chars;
    const char* replace_with;
    const char* output;
    bool result;
  } cases[] = {
    { "", "", "", "", false },
    { "test", "", "", "test", false },
    { "test", "", "!", "test", false },
    { "test", "z", "!", "test", false },
    { "test", "e", "!", "t!st", true },
    { "test", "e", "!?", "t!?st", true },
    { "test", "ez", "!", "t!st", true },
    { "test", "zed", "!?", "t!?st", true },
    { "test", "t", "!?", "!?es!?", true },
    { "test", "et", "!>", "!>!>s!>", true },
    { "test", "zest", "!", "!!!!", true },
    { "test", "szt", "!", "!e!!", true },
    { "test", "t", "test", "testestest", true },
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    std::string output;
    bool result = ReplaceChars(cases[i].input,
                               cases[i].replace_chars,
                               cases[i].replace_with,
                               &output);
    EXPECT_EQ(cases[i].result, result);
    EXPECT_EQ(cases[i].output, output);
  }
}

}  // namespace base
