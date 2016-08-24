// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/url_utils.h>

#include <gtest/gtest.h>

namespace brillo {

TEST(UrlUtils, Combine) {
  EXPECT_EQ("http://sample.org/path",
            url::Combine("http://sample.org", "path"));
  EXPECT_EQ("http://sample.org/path",
            url::Combine("http://sample.org/", "path"));
  EXPECT_EQ("path1/path2", url::Combine("", "path1/path2"));
  EXPECT_EQ("path1/path2", url::Combine("path1", "path2"));
  EXPECT_EQ("http://sample.org", url::Combine("http://sample.org", ""));
  EXPECT_EQ("http://sample.org/path",
            url::Combine("http://sample.org/", "/path"));
  EXPECT_EQ("http://sample.org/path",
            url::Combine("http://sample.org", "//////path"));
  EXPECT_EQ("http://sample.org/", url::Combine("http://sample.org", "///"));
  EXPECT_EQ("http://sample.org/obj/path1/path2",
            url::Combine("http://sample.org/obj", "path1/path2"));
  EXPECT_EQ("http://sample.org/obj/path1/path2#tag",
            url::Combine("http://sample.org/obj#tag", "path1/path2"));
  EXPECT_EQ("http://sample.org/obj/path1/path2?k1=v1&k2=v2",
            url::Combine("http://sample.org/obj?k1=v1&k2=v2", "path1/path2"));
  EXPECT_EQ("http://sample.org/obj/path1/path2?k1=v1#k2=v2",
            url::Combine("http://sample.org/obj/?k1=v1#k2=v2", "path1/path2"));
  EXPECT_EQ("http://sample.org/obj/path1/path2#tag?",
            url::Combine("http://sample.org/obj#tag?", "path1/path2"));
  EXPECT_EQ("path1/path2", url::CombineMultiple("", {"path1", "path2"}));
  EXPECT_EQ("http://sample.org/obj/part1/part2",
            url::CombineMultiple("http://sample.org",
                                 {"obj", "", "/part1/", "part2"}));
}

TEST(UrlUtils, GetQueryString) {
  EXPECT_EQ("", url::GetQueryString("http://sample.org", false));
  EXPECT_EQ("", url::GetQueryString("http://sample.org", true));
  EXPECT_EQ("", url::GetQueryString("", false));
  EXPECT_EQ("", url::GetQueryString("", true));

  EXPECT_EQ("?q=v&b=2#tag?2",
            url::GetQueryString("http://s.com/?q=v&b=2#tag?2", false));
  EXPECT_EQ("?q=v&b=2",
            url::GetQueryString("http://s.com/?q=v&b=2#tag?2", true));

  EXPECT_EQ("#tag?a=2", url::GetQueryString("http://s.com/#tag?a=2", false));
  EXPECT_EQ("", url::GetQueryString("http://s.com/#tag?a=2", true));

  EXPECT_EQ("?a=2&b=2", url::GetQueryString("?a=2&b=2", false));
  EXPECT_EQ("?a=2&b=2", url::GetQueryString("?a=2&b=2", true));

  EXPECT_EQ("#s#?d#?f?#s?#d", url::GetQueryString("#s#?d#?f?#s?#d", false));
  EXPECT_EQ("", url::GetQueryString("#s#?d#?f?#s?#d", true));
}

TEST(UrlUtils, GetQueryStringParameters) {
  auto params = url::GetQueryStringParameters(
      "http://sample.org/path?k=v&&%3Dkey%3D=val%26&r#blah");

  EXPECT_EQ(3, params.size());
  EXPECT_EQ("k", params[0].first);
  EXPECT_EQ("v", params[0].second);
  EXPECT_EQ("=key=", params[1].first);
  EXPECT_EQ("val&", params[1].second);
  EXPECT_EQ("r", params[2].first);
  EXPECT_EQ("", params[2].second);
}

TEST(UrlUtils, GetQueryStringValue) {
  std::string url = "http://url?key1=val1&&key2=val2";
  EXPECT_EQ("val1", url::GetQueryStringValue(url, "key1"));
  EXPECT_EQ("val2", url::GetQueryStringValue(url, "key2"));
  EXPECT_EQ("", url::GetQueryStringValue(url, "key3"));

  auto params = url::GetQueryStringParameters(url);
  EXPECT_EQ("val1", url::GetQueryStringValue(params, "key1"));
  EXPECT_EQ("val2", url::GetQueryStringValue(params, "key2"));
  EXPECT_EQ("", url::GetQueryStringValue(params, "key3"));
}

TEST(UrlUtils, TrimOffQueryString) {
  std::string url = "http://url?key1=val1&key2=val2#fragment";
  std::string query = url::TrimOffQueryString(&url);
  EXPECT_EQ("http://url", url);
  EXPECT_EQ("?key1=val1&key2=val2#fragment", query);

  url = "http://url#fragment";
  query = url::TrimOffQueryString(&url);
  EXPECT_EQ("http://url", url);
  EXPECT_EQ("#fragment", query);

  url = "http://url";
  query = url::TrimOffQueryString(&url);
  EXPECT_EQ("http://url", url);
  EXPECT_EQ("", query);
}

TEST(UrlUtils, RemoveQueryString) {
  std::string url = "http://url?key1=val1&key2=val2#fragment";
  EXPECT_EQ("http://url", url::RemoveQueryString(url, true));
  EXPECT_EQ("http://url#fragment", url::RemoveQueryString(url, false));
}

TEST(UrlUtils, AppendQueryParam) {
  std::string url = "http://server.com/path";
  url = url::AppendQueryParam(url, "param", "value");
  EXPECT_EQ("http://server.com/path?param=value", url);
  url = url::AppendQueryParam(url, "param2", "v");
  EXPECT_EQ("http://server.com/path?param=value&param2=v", url);

  url = "http://server.com/path#fragment";
  url = url::AppendQueryParam(url, "param", "value");
  EXPECT_EQ("http://server.com/path?param=value#fragment", url);
  url = url::AppendQueryParam(url, "param2", "v");
  EXPECT_EQ("http://server.com/path?param=value&param2=v#fragment", url);

  url = url::AppendQueryParam("http://server.com/path?", "param", "value");
  EXPECT_EQ("http://server.com/path?param=value", url);
}

TEST(UrlUtils, AppendQueryParams) {
  std::string url = "http://server.com/path";
  url = url::AppendQueryParams(url, {});
  EXPECT_EQ("http://server.com/path", url);
  url = url::AppendQueryParams(url, {{"param", "value"}, {"q", "="}});
  EXPECT_EQ("http://server.com/path?param=value&q=%3D", url);
  url += "#fr?";
  url = url::AppendQueryParams(url, {{"p", "1"}, {"s&", "\n"}});
  EXPECT_EQ("http://server.com/path?param=value&q=%3D&p=1&s%26=%0A#fr?", url);
}

TEST(UrlUtils, HasQueryString) {
  EXPECT_FALSE(url::HasQueryString("http://server.com/path"));
  EXPECT_FALSE(url::HasQueryString("http://server.com/path#blah?v=1"));
  EXPECT_TRUE(url::HasQueryString("http://server.com/path?v=1#blah"));
  EXPECT_TRUE(url::HasQueryString("http://server.com/path?v=1"));
  EXPECT_FALSE(url::HasQueryString(""));
  EXPECT_TRUE(url::HasQueryString("?ss"));
}

}  // namespace brillo
