// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/mime_utils.h>

#include <gtest/gtest.h>

namespace brillo {

TEST(MimeUtils, Combine) {
  std::string mime_string = mime::Combine(mime::types::kText, "xml");
  EXPECT_EQ(mime::text::kXml, mime_string);
  EXPECT_EQ(
      "application/json; charset=utf-8",
      mime::Combine(mime::types::kApplication, "json", {{"charset", "utf-8"}}));
}

TEST(MimeUtils, Split) {
  std::string s1, s2;
  EXPECT_TRUE(mime::Split(mime::image::kJpeg, &s1, &s2));
  EXPECT_EQ(mime::types::kImage, s1);
  EXPECT_EQ("jpeg", s2);

  mime::Parameters parameters;
  EXPECT_TRUE(
      mime::Split("application/json;charset=utf-8", &s1, &s2, &parameters));
  EXPECT_EQ(mime::types::kApplication, s1);
  EXPECT_EQ("json", s2);
  EXPECT_EQ(mime::application::kJson, mime::Combine(s1, s2));
  EXPECT_EQ(1, parameters.size());
  EXPECT_EQ(mime::parameters::kCharset, parameters.front().first);
  EXPECT_EQ("utf-8", parameters.front().second);
  EXPECT_EQ("application/json; charset=utf-8",
            mime::Combine(s1, s2, parameters));
}

TEST(MimeUtils, ExtractParts) {
  mime::Parameters parameters;

  EXPECT_EQ(mime::types::kText, mime::GetType(mime::text::kPlain));
  EXPECT_EQ("plain", mime::GetSubtype(mime::text::kPlain));

  parameters = mime::GetParameters("text/plain; charset=iso-8859-1;foo=bar");
  EXPECT_EQ(2, parameters.size());
  EXPECT_EQ(mime::parameters::kCharset, parameters[0].first);
  EXPECT_EQ("iso-8859-1", parameters[0].second);
  EXPECT_EQ("foo", parameters[1].first);
  EXPECT_EQ("bar", parameters[1].second);
}

TEST(MimeUtils, AppendRemoveParams) {
  std::string mime_string = mime::AppendParameter(
      mime::text::kXml, mime::parameters::kCharset, "utf-8");
  EXPECT_EQ("text/xml; charset=utf-8", mime_string);
  mime_string = mime::AppendParameter(mime_string, "foo", "bar");
  EXPECT_EQ("text/xml; charset=utf-8; foo=bar", mime_string);
  EXPECT_EQ("utf-8",
            mime::GetParameterValue(mime_string, mime::parameters::kCharset));
  EXPECT_EQ("bar", mime::GetParameterValue(mime_string, "foo"));
  EXPECT_EQ("", mime::GetParameterValue(mime_string, "baz"));
  mime_string = mime::RemoveParameters(mime_string);
  EXPECT_EQ(mime::text::kXml, mime_string);
}

}  // namespace brillo
