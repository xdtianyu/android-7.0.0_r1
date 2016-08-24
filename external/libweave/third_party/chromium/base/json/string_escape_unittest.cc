// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/json/string_escape.h"

#include <gtest/gtest.h>
#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"

namespace base {

TEST(JSONStringEscapeTest, EscapeUTF8) {
  const struct {
    const char* to_escape;
    const char* escaped;
  } cases[] = {
    {"\b\001aZ\"\\wee", "\\b\\u0001aZ\\\"\\\\wee"},
    {"a\b\f\n\r\t\v\1\\.\"z",
        "a\\b\\f\\n\\r\\t\\u000B\\u0001\\\\.\\\"z"},
    {"b\x0f\x7f\xf0\xff!",  // \xf0\xff is not a valid UTF-8 unit.
        "b\\u000F\x7F\xEF\xBF\xBD\xEF\xBF\xBD!"},
    {"c<>d", "c\\u003C>d"},
    {"Hello\xe2\x80\xa8world", "Hello\\u2028world"},
    {"\xe2\x80\xa9purple", "\\u2029purple"},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    const char* in_ptr = cases[i].to_escape;
    std::string in_str = in_ptr;

    std::string out;
    EscapeJSONString(in_ptr, false, &out);
    EXPECT_EQ(std::string(cases[i].escaped), out);
    EXPECT_TRUE(IsStringUTF8(out));

    out.erase();
    bool convert_ok = EscapeJSONString(in_str, false, &out);
    EXPECT_EQ(std::string(cases[i].escaped), out);
    EXPECT_TRUE(IsStringUTF8(out));

    if (convert_ok) {
      std::string fooout = GetQuotedJSONString(in_str);
      EXPECT_EQ("\"" + std::string(cases[i].escaped) + "\"", fooout);
      EXPECT_TRUE(IsStringUTF8(out));
    }
  }

  std::string in = cases[0].to_escape;
  std::string out;
  EscapeJSONString(in, false, &out);
  EXPECT_TRUE(IsStringUTF8(out));

  // test quoting
  std::string out_quoted;
  EscapeJSONString(in, true, &out_quoted);
  EXPECT_EQ(out.length() + 2, out_quoted.length());
  EXPECT_EQ(out_quoted.find(out), 1U);
  EXPECT_TRUE(IsStringUTF8(out_quoted));

  // now try with a NULL in the string
  std::string null_prepend = "test";
  null_prepend.push_back(0);
  in = null_prepend + in;
  std::string expected = "test\\u0000";
  expected += cases[0].escaped;
  out.clear();
  EscapeJSONString(in, false, &out);
  EXPECT_EQ(expected, out);
  EXPECT_TRUE(IsStringUTF8(out));
}

TEST(JSONStringEscapeTest, EscapeBytes) {
  const struct {
    const char* to_escape;
    const char* escaped;
  } cases[] = {
    {"b\x0f\x7f\xf0\xff!", "b\\u000F\\u007F\\u00F0\\u00FF!"},
    {"\xe5\xc4\x4f\x05\xb6\xfd", "\\u00E5\\u00C4O\\u0005\\u00B6\\u00FD"},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    std::string in = std::string(cases[i].to_escape);
    EXPECT_FALSE(IsStringUTF8(in));

    EXPECT_EQ(std::string(cases[i].escaped),
        EscapeBytesAsInvalidJSONString(in, false));
    EXPECT_EQ("\"" + std::string(cases[i].escaped) + "\"",
        EscapeBytesAsInvalidJSONString(in, true));
  }

  const char kEmbedNull[] = { '\xab', '\x39', '\0', '\x9f', '\xab' };
  std::string in(kEmbedNull, arraysize(kEmbedNull));
  EXPECT_FALSE(IsStringUTF8(in));
  EXPECT_EQ(std::string("\\u00AB9\\u0000\\u009F\\u00AB"),
            EscapeBytesAsInvalidJSONString(in, false));
}

}  // namespace base
