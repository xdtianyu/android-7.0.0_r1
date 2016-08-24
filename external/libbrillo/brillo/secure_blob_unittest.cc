// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Unit tests for SecureBlob.

#include "brillo/secure_blob.h"

#include <algorithm>
#include <iterator>
#include <numeric>

#include <base/logging.h>
#include <gtest/gtest.h>

namespace brillo {
using std::string;

class SecureBlobTest : public ::testing::Test {
 public:
  SecureBlobTest() {}
  virtual ~SecureBlobTest() {}

  static bool FindBlobInBlob(const brillo::Blob& haystack,
                             const brillo::Blob& needle) {
    auto pos = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end());
    return (pos != haystack.end());
  }

  static int FindBlobIndexInBlob(const brillo::Blob& haystack,
                                 const brillo::Blob& needle) {
    auto pos = std::search(
        haystack.begin(), haystack.end(), needle.begin(), needle.end());
    if (pos == haystack.end()) {
      return -1;
    }
    return std::distance(haystack.begin(), pos);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SecureBlobTest);
};

TEST_F(SecureBlobTest, AllocationSizeTest) {
  // Check that allocating a SecureBlob of a specified size works
  SecureBlob blob(32);

  EXPECT_EQ(32, blob.size());
}

TEST_F(SecureBlobTest, AllocationCopyTest) {
  // Check that allocating a SecureBlob with an iterator works
  unsigned char from_data[32];
  std::iota(std::begin(from_data), std::end(from_data), 0);

  SecureBlob blob(std::begin(from_data), std::end(from_data));

  EXPECT_EQ(sizeof(from_data), blob.size());

  for (unsigned int i = 0; i < sizeof(from_data); i++) {
    EXPECT_EQ(from_data[i], blob[i]);
  }
}

TEST_F(SecureBlobTest, IteratorConstructorTest) {
  // Check that allocating a SecureBlob with an iterator works
  brillo::Blob from_blob(32);
  for (unsigned int i = 0; i < from_blob.size(); i++) {
    from_blob[i] = i;
  }

  SecureBlob blob(from_blob.begin(), from_blob.end());

  EXPECT_EQ(from_blob.size(), blob.size());
  EXPECT_TRUE(SecureBlobTest::FindBlobInBlob(from_blob, blob));
}

TEST_F(SecureBlobTest, ResizeTest) {
  // Check that resizing a SecureBlob wipes the excess memory.  The test assumes
  // that resize() down by one will not re-allocate the memory, so the last byte
  // will still be part of the SecureBlob's allocation
  size_t length = 1024;
  SecureBlob blob(length);
  void* original_data = blob.data();
  for (size_t i = 0; i < length; i++) {
    blob[i] = i;
  }

  blob.resize(length - 1);

  EXPECT_EQ(original_data, blob.data());
  EXPECT_EQ(length - 1, blob.size());
  EXPECT_EQ(0, blob.data()[length - 1]);
}

TEST_F(SecureBlobTest, CombineTest) {
  SecureBlob blob1(32);
  SecureBlob blob2(32);
  std::iota(blob1.begin(), blob1.end(), 0);
  std::iota(blob2.begin(), blob2.end(), 32);
  SecureBlob combined_blob = SecureBlob::Combine(blob1, blob2);
  EXPECT_EQ(combined_blob.size(), (blob1.size() + blob2.size()));
  EXPECT_TRUE(SecureBlobTest::FindBlobInBlob(combined_blob, blob1));
  EXPECT_TRUE(SecureBlobTest::FindBlobInBlob(combined_blob, blob2));
  int blob1_index = SecureBlobTest::FindBlobIndexInBlob(combined_blob, blob1);
  int blob2_index = SecureBlobTest::FindBlobIndexInBlob(combined_blob, blob2);
  EXPECT_EQ(blob1_index, 0);
  EXPECT_EQ(blob2_index, 32);
}

TEST_F(SecureBlobTest, BlobToStringTest) {
  std::string test_string("Test String");
  SecureBlob blob = SecureBlob(test_string.begin(), test_string.end());
  EXPECT_EQ(blob.size(), test_string.length());
  std::string result_string = blob.to_string();
  EXPECT_EQ(test_string.compare(result_string), 0);
}

}  // namespace brillo
