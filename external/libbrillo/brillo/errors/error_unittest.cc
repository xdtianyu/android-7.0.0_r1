// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/errors/error.h>

#include <gtest/gtest.h>

using brillo::Error;

namespace {

brillo::ErrorPtr GenerateNetworkError() {
  tracked_objects::Location loc("GenerateNetworkError",
                                "error_unittest.cc",
                                15,
                                ::tracked_objects::GetProgramCounter());
  return Error::Create(loc, "network", "not_found", "Resource not found");
}

brillo::ErrorPtr GenerateHttpError() {
  brillo::ErrorPtr inner = GenerateNetworkError();
  return Error::Create(FROM_HERE, "HTTP", "404", "Not found", std::move(inner));
}

}  // namespace

TEST(Error, Single) {
  brillo::ErrorPtr err = GenerateNetworkError();
  EXPECT_EQ("network", err->GetDomain());
  EXPECT_EQ("not_found", err->GetCode());
  EXPECT_EQ("Resource not found", err->GetMessage());
  EXPECT_EQ("GenerateNetworkError", err->GetLocation().function_name);
  EXPECT_EQ("error_unittest.cc", err->GetLocation().file_name);
  EXPECT_EQ(15, err->GetLocation().line_number);
  EXPECT_EQ(nullptr, err->GetInnerError());
  EXPECT_TRUE(err->HasDomain("network"));
  EXPECT_FALSE(err->HasDomain("HTTP"));
  EXPECT_FALSE(err->HasDomain("foo"));
  EXPECT_TRUE(err->HasError("network", "not_found"));
  EXPECT_FALSE(err->HasError("network", "404"));
  EXPECT_FALSE(err->HasError("HTTP", "404"));
  EXPECT_FALSE(err->HasError("HTTP", "not_found"));
  EXPECT_FALSE(err->HasError("foo", "bar"));
}

TEST(Error, Nested) {
  brillo::ErrorPtr err = GenerateHttpError();
  EXPECT_EQ("HTTP", err->GetDomain());
  EXPECT_EQ("404", err->GetCode());
  EXPECT_EQ("Not found", err->GetMessage());
  EXPECT_NE(nullptr, err->GetInnerError());
  EXPECT_EQ("network", err->GetInnerError()->GetDomain());
  EXPECT_TRUE(err->HasDomain("network"));
  EXPECT_TRUE(err->HasDomain("HTTP"));
  EXPECT_FALSE(err->HasDomain("foo"));
  EXPECT_TRUE(err->HasError("network", "not_found"));
  EXPECT_FALSE(err->HasError("network", "404"));
  EXPECT_TRUE(err->HasError("HTTP", "404"));
  EXPECT_FALSE(err->HasError("HTTP", "not_found"));
  EXPECT_FALSE(err->HasError("foo", "bar"));
}

TEST(Error, Clone) {
  brillo::ErrorPtr err = GenerateHttpError();
  brillo::ErrorPtr clone = err->Clone();
  const brillo::Error* error1 = err.get();
  const brillo::Error* error2 = clone.get();
  while (error1 && error2) {
    EXPECT_NE(error1, error2);
    EXPECT_EQ(error1->GetDomain(), error2->GetDomain());
    EXPECT_EQ(error1->GetCode(), error2->GetCode());
    EXPECT_EQ(error1->GetMessage(), error2->GetMessage());
    EXPECT_EQ(error1->GetLocation().function_name,
              error2->GetLocation().function_name);
    EXPECT_EQ(error1->GetLocation().file_name, error2->GetLocation().file_name);
    EXPECT_EQ(error1->GetLocation().line_number,
              error2->GetLocation().line_number);
    error1 = error1->GetInnerError();
    error2 = error2->GetInnerError();
  }
  EXPECT_EQ(error1, error2);
}
