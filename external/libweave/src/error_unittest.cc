// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/error.h>

#include <gtest/gtest.h>

namespace weave {

namespace {

ErrorPtr GenerateNetworkError() {
  tracked_objects::Location loc("GenerateNetworkError", "error_unittest.cc", 15,
                                ::tracked_objects::GetProgramCounter());
  return Error::Create(loc, "not_found", "Resource not found");
}

ErrorPtr GenerateHttpError() {
  ErrorPtr inner = GenerateNetworkError();
  return Error::Create(FROM_HERE, "404", "Not found", std::move(inner));
}

}  // namespace

TEST(Error, Single) {
  ErrorPtr err = GenerateNetworkError();
  EXPECT_EQ("not_found", err->GetCode());
  EXPECT_EQ("Resource not found", err->GetMessage());
  EXPECT_EQ("GenerateNetworkError", err->GetLocation().function_name);
  EXPECT_EQ("error_unittest.cc", err->GetLocation().file_name);
  EXPECT_EQ(15, err->GetLocation().line_number);
  EXPECT_EQ(nullptr, err->GetInnerError());
  EXPECT_TRUE(err->HasError("not_found"));
  EXPECT_FALSE(err->HasError("404"));
  EXPECT_FALSE(err->HasError("bar"));
}

TEST(Error, Nested) {
  ErrorPtr err = GenerateHttpError();
  EXPECT_EQ("404", err->GetCode());
  EXPECT_EQ("Not found", err->GetMessage());
  EXPECT_NE(nullptr, err->GetInnerError());
  EXPECT_TRUE(err->HasError("not_found"));
  EXPECT_TRUE(err->HasError("404"));
  EXPECT_FALSE(err->HasError("bar"));
}

TEST(Error, Clone) {
  ErrorPtr err = GenerateHttpError();
  ErrorPtr clone = err->Clone();
  const Error* error1 = err.get();
  const Error* error2 = clone.get();
  while (error1 && error2) {
    EXPECT_NE(error1, error2);
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

}  // namespace weave
