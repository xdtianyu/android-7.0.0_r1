// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/guid.h"

#include <stdint.h>

#include <limits>

#include <gtest/gtest.h>

#include "base/strings/string_util.h"
#include "build/build_config.h"

namespace base {

#if defined(OS_POSIX)

namespace {

template <typename Char>
inline bool IsHexDigit(Char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'F') ||
         (c >= 'a' && c <= 'f');
}

bool IsValidGUID(const std::string& guid) {
  const size_t kGUIDLength = 36U;
  if (guid.length() != kGUIDLength)
    return false;

  for (size_t i = 0; i < guid.length(); ++i) {
    char current = guid[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (current != '-')
        return false;
    } else {
      if (!IsHexDigit(current))
        return false;
    }
  }

  return true;
}

bool IsGUIDv4(const std::string& guid) {
  // The format of GUID version 4 must be xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx,
  // where y is one of [8, 9, A, B].
  return IsValidGUID(guid) && guid[14] == '4' &&
         (guid[19] == '8' || guid[19] == '9' || guid[19] == 'A' ||
          guid[19] == 'a' || guid[19] == 'B' || guid[19] == 'b');
}

}  // namespace

TEST(GUIDTest, GUIDGeneratesAllZeroes) {
  uint64_t bytes[] = {0, 0};
  std::string clientid = RandomDataToGUIDString(bytes);
  EXPECT_EQ("00000000-0000-0000-0000-000000000000", clientid);
}

TEST(GUIDTest, GUIDGeneratesCorrectly) {
  uint64_t bytes[] = {0x0123456789ABCDEFULL, 0xFEDCBA9876543210ULL};
  std::string clientid = RandomDataToGUIDString(bytes);
  EXPECT_EQ("01234567-89AB-CDEF-FEDC-BA9876543210", clientid);
}
#endif

TEST(GUIDTest, GUIDCorrectlyFormatted) {
  const int kIterations = 10;
  for (int it = 0; it < kIterations; ++it) {
    std::string guid = GenerateGUID();
    EXPECT_TRUE(IsValidGUID(guid));
  }
}

TEST(GUIDTest, GUIDBasicUniqueness) {
  const int kIterations = 10;
  for (int it = 0; it < kIterations; ++it) {
    std::string guid1 = GenerateGUID();
    std::string guid2 = GenerateGUID();
    EXPECT_EQ(36U, guid1.length());
    EXPECT_EQ(36U, guid2.length());
    EXPECT_NE(guid1, guid2);
#if defined(OS_POSIX)
    EXPECT_TRUE(IsGUIDv4(guid1));
    EXPECT_TRUE(IsGUIDv4(guid2));
#endif
  }
}

}  // namespace base
