// Copyright (c) 2011 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/data_encoding.h"

#include <algorithm>
#include <numeric>

#include <base/logging.h>
#include <gtest/gtest.h>

namespace weave {

TEST(data_encoding, UrlEncoding) {
  std::string test = "\"http://sample/path/0014.html \"";
  std::string encoded = UrlEncode(test.c_str());
  EXPECT_EQ("%22http%3A%2F%2Fsample%2Fpath%2F0014.html+%22", encoded);
  EXPECT_EQ(test, UrlDecode(encoded.c_str()));

  test = "\"http://sample/path/0014.html \"";
  encoded = UrlEncode(test.c_str(), false);
  EXPECT_EQ("%22http%3A%2F%2Fsample%2Fpath%2F0014.html%20%22", encoded);
  EXPECT_EQ(test, UrlDecode(encoded.c_str()));
}

TEST(data_encoding, WebParamsEncoding) {
  std::string encoded =
      WebParamsEncode({{"q", "test"}, {"path", "/usr/bin"}, {"#", "%"}});
  EXPECT_EQ("q=test&path=%2Fusr%2Fbin&%23=%25", encoded);

  auto params = WebParamsDecode(encoded);
  EXPECT_EQ(3u, params.size());
  EXPECT_EQ("q", params[0].first);
  EXPECT_EQ("test", params[0].second);
  EXPECT_EQ("path", params[1].first);
  EXPECT_EQ("/usr/bin", params[1].second);
  EXPECT_EQ("#", params[2].first);
  EXPECT_EQ("%", params[2].second);
}

TEST(data_encoding, Base64Encode) {
  const std::string text1 = "hello world";
  const std::string encoded1 = "aGVsbG8gd29ybGQ=";

  const std::string text2 =
      "Lorem ipsum dolor sit amet, facilisis erat nec aliquam, scelerisque "
      "molestie commodo. Viverra tincidunt integer erat ipsum, integer "
      "molestie, arcu in, sit mauris ac a sed sit etiam.";
  const std::string encoded2 =
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGZhY2lsaXNpcyBlcmF0IG5lYyBhbGlxdWF"
      "tLCBzY2VsZXJpc3F1ZSBtb2xlc3RpZSBjb21tb2RvLiBWaXZlcnJhIHRpbmNpZHVudCBpbn"
      "RlZ2VyIGVyYXQgaXBzdW0sIGludGVnZXIgbW9sZXN0aWUsIGFyY3UgaW4sIHNpdCBtYXVya"
      "XMgYWMgYSBzZWQgc2l0IGV0aWFtLg==";

  std::vector<uint8_t> data3(256);
  std::iota(data3.begin(), data3.end(), 0);  // Fills the buffer with 0x00-0xFF.
  const std::string encoded3 =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ"
      "1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaW"
      "prbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en"
      "6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU"
      "1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";

  EXPECT_EQ(encoded1, Base64Encode(text1));
  EXPECT_EQ(encoded2, Base64Encode(text2));
  EXPECT_EQ(encoded3, Base64Encode(data3));
}

TEST(data_encoding, Base64EncodeWrapLines) {
  const std::string text1 = "hello world";
  const std::string encoded1 = "aGVsbG8gd29ybGQ=\n";

  const std::string text2 =
      "Lorem ipsum dolor sit amet, facilisis erat nec aliquam, scelerisque "
      "molestie commodo. Viverra tincidunt integer erat ipsum, integer "
      "molestie, arcu in, sit mauris ac a sed sit etiam.";
  const std::string encoded2 =
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGZhY2lsaXNpcyBlcmF0IG5lYyBh\n"
      "bGlxdWFtLCBzY2VsZXJpc3F1ZSBtb2xlc3RpZSBjb21tb2RvLiBWaXZlcnJhIHRp\n"
      "bmNpZHVudCBpbnRlZ2VyIGVyYXQgaXBzdW0sIGludGVnZXIgbW9sZXN0aWUsIGFy\n"
      "Y3UgaW4sIHNpdCBtYXVyaXMgYWMgYSBzZWQgc2l0IGV0aWFtLg==\n";

  std::vector<uint8_t> data3(256);
  std::iota(data3.begin(), data3.end(), 0);  // Fills the buffer with 0x00-0xFF.
  const std::string encoded3 =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v\n"
      "MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f\n"
      "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P\n"
      "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/\n"
      "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v\n"
      "8PHy8/T19vf4+fr7/P3+/w==\n";

  EXPECT_EQ(encoded1, Base64EncodeWrapLines(text1));
  EXPECT_EQ(encoded2, Base64EncodeWrapLines(text2));
  EXPECT_EQ(encoded3, Base64EncodeWrapLines(data3));
}

TEST(data_encoding, Base64Decode) {
  const std::string encoded1 = "dGVzdCBzdHJpbmc=";

  const std::string encoded2 =
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGZhY2lsaXNpcyBlcmF0IG5lYyBh\n"
      "bGlxdWFtLCBzY2VsZXJpc3F1ZSBtb2xlc3RpZSBjb21tb2RvLiBWaXZlcnJhIHRp\r\n"
      "bmNpZHVudCBpbnRlZ2VyIGVyYXQgaXBzdW0sIGludGVnZXIgbW9sZXN0aWUsIGFy\r"
      "Y3UgaW4sIHNpdCBtYXVyaXMgYWMgYSBzZWQgc2l0IGV0aWFt\n"
      "Lg==\n\n\n";
  const std::string decoded2 =
      "Lorem ipsum dolor sit amet, facilisis erat nec aliquam, scelerisque "
      "molestie commodo. Viverra tincidunt integer erat ipsum, integer "
      "molestie, arcu in, sit mauris ac a sed sit etiam.";

  const std::string encoded3 =
      "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ"
      "1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaW"
      "prbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en"
      "6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU"
      "1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";
  std::vector<uint8_t> decoded3(256);
  std::iota(decoded3.begin(), decoded3.end(), 0);  // Fill with 0x00..0xFF.

  std::string decoded;
  EXPECT_TRUE(Base64Decode(encoded1, &decoded));
  EXPECT_EQ("test string", decoded);

  EXPECT_TRUE(Base64Decode(encoded2, &decoded));
  EXPECT_EQ(decoded2, decoded);

  std::vector<uint8_t> decoded_blob;
  EXPECT_TRUE(Base64Decode(encoded3, &decoded_blob));
  EXPECT_EQ(decoded3, decoded_blob);

  EXPECT_FALSE(Base64Decode("A", &decoded_blob));
  EXPECT_TRUE(decoded_blob.empty());

  EXPECT_TRUE(Base64Decode("/w==", &decoded_blob));
  EXPECT_EQ((std::vector<uint8_t>{0xFF}), decoded_blob);

  EXPECT_TRUE(Base64Decode("//8=", &decoded_blob));
  EXPECT_EQ((std::vector<uint8_t>{0xFF, 0xFF}), decoded_blob);

  EXPECT_FALSE(Base64Decode("AAECAwQFB,cI", &decoded_blob));
  EXPECT_TRUE(decoded_blob.empty());
}

}  // namespace weave
