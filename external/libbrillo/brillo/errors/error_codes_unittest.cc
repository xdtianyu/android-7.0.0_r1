// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/errors/error_codes.h>

#include <gtest/gtest.h>

using brillo::errors::system::AddSystemError;

TEST(SystemErrorCodes, AddTo) {
  brillo::ErrorPtr error;

  AddSystemError(&error, FROM_HERE, ENOENT);
  EXPECT_EQ(brillo::errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("ENOENT", error->GetCode());
  EXPECT_EQ("No such file or directory", error->GetMessage());
  error.reset();

  AddSystemError(&error, FROM_HERE, EPROTO);
  EXPECT_EQ(brillo::errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("EPROTO", error->GetCode());
  EXPECT_EQ("Protocol error", error->GetMessage());
  error.reset();
}

TEST(SystemErrorCodes, AddTo_UnknownError) {
  brillo::ErrorPtr error;
  AddSystemError(&error, FROM_HERE, 10000);
  EXPECT_EQ(brillo::errors::system::kDomain, error->GetDomain());
  EXPECT_EQ("error_10000", error->GetCode());
  EXPECT_EQ("Unknown error 10000", error->GetMessage());
}
