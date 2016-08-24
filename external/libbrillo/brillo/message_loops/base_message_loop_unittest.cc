// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/message_loops/base_message_loop.h>

#include <gtest/gtest.h>

#include <brillo/message_loops/message_loop.h>

namespace brillo {

class BaseMessageLoopTest : public ::testing::Test {};

TEST(BaseMessageLoopTest, ParseBinderMinor) {
  EXPECT_EQ(57, BaseMessageLoop::ParseBinderMinor(
      "227 mcelog\n 58 sw_sync\n 59 ashmem\n 57 binder\n239 uhid\n"));
  EXPECT_EQ(123, BaseMessageLoop::ParseBinderMinor("123 binder\n"));

  EXPECT_EQ(BaseMessageLoop::kInvalidMinor,
            BaseMessageLoop::ParseBinderMinor("227 foo\n239 bar\n"));
}

}  // namespace brillo
