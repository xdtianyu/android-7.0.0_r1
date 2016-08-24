// Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brillo/glib/object.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>

using brillo::glib::ScopedPtrArray;
using brillo::glib::ScopedError;
using brillo::glib::Retrieve;
using brillo::glib::Value;
using brillo::Resetter;

namespace {  // NOLINT

template <typename T>
void SetRetrieveTest(const T& x) {
  Value tmp(x);
  T result;
  EXPECT_TRUE(Retrieve(tmp, &result));
  EXPECT_EQ(result, x);
}

void ModifyValue(Value* x) {
  *x = 1.0 / 1231415926.0;   // An unlikely value
}

template <typename T, typename O>
void MutableRegularTestValue(const T& x, O modify) {
  Value tmp(x);
  Value y = tmp;  // copy-construction
  T result;
  EXPECT_TRUE(Retrieve(y, &result));
  EXPECT_EQ(result, x);
  modify(&y);
  LOG(INFO) << "Warning Expected.";
  EXPECT_TRUE(!(Retrieve(y, &result) && result == x));
  y = tmp;  // assignment
  EXPECT_TRUE(Retrieve(y, &result));
  EXPECT_EQ(result, x);
  modify(&y);
  LOG(INFO) << "Warning Expected.";
  EXPECT_TRUE(!(Retrieve(y, &result) && result == x));
}

void OutArgument(int** x) {
  *x = new int(10);  // NOLINT
}

}  // namespace

TEST(ResetterTest, All) {
  scoped_ptr<int> x;
  OutArgument(&Resetter(&x).lvalue());
  EXPECT_EQ(*x, 10);
}

TEST(RetrieveTest, Types) {
  SetRetrieveTest(std::string("Hello!"));
  SetRetrieveTest(static_cast<uint32_t>(10));
  SetRetrieveTest(10.5);
  SetRetrieveTest(true);
}

TEST(ValueTest, All) {
  Value x;  // default construction
  Value y = x;  // copy with default value
  x = y;  // assignment with default value
  Value z(1.5);
  x = z;  // assignment to default value
  MutableRegularTestValue(std::string("Hello!"), &ModifyValue);
}

TEST(ScopedErrorTest, All) {
  ScopedError a;  // default construction
  ScopedError b(::g_error_new(::g_quark_from_static_string("error"), -1,
                              ""));  // constructor
  ::GError* c = ::g_error_new(::g_quark_from_static_string("error"), -1,
                              "");
  ::GError* d = ::g_error_new(::g_quark_from_static_string("error"), -1,
                              "");
  a.reset(c);  // reset form 1
  (void)d;
}

TEST(ScopedPtrArrayTest, Construction) {
  const char item[] = "a string";
  char* a = static_cast<char*>(::g_malloc(sizeof(item)));
  std::strcpy(a, &item[0]);  // NOLINT

  ::GPtrArray* array = ::g_ptr_array_new();
  ::g_ptr_array_add(array, ::gpointer(a));

  ScopedPtrArray<const char*> x(array);
  EXPECT_EQ(x.size(), 1);
  EXPECT_EQ(x[0], a);  // indexing
}

TEST(ScopedPtrArrayTest, Reset) {
  const char item[] = "a string";
  char* a = static_cast<char*>(::g_malloc(sizeof(item)));
  std::strcpy(a, &item[0]);  // NOLINT

  ScopedPtrArray<const char*> x;  // default construction
  x.push_back(a);
  EXPECT_EQ(x.size(), 1);
  x.reset();
  EXPECT_EQ(x.size(), 0);

  char* b = static_cast<char*>(::g_malloc(sizeof(item)));
  std::strcpy(b, &item[0]);  // NOLINT

  ::GPtrArray* array = ::g_ptr_array_new();
  ::g_ptr_array_add(array, ::gpointer(b));

  x.reset(array);
  EXPECT_EQ(x.size(), 1);
}

TEST(ScopedPtrArrayTest, Iteration) {
  char* a[] = { static_cast<char*>(::g_malloc(1)),
      static_cast<char*>(::g_malloc(1)), static_cast<char*>(::g_malloc(1)) };

  ScopedPtrArray<const char*> x;
  std::copy(&a[0], &a[3], std::back_inserter(x));
  EXPECT_TRUE(std::equal(x.begin(), x.end(), &a[0]));
}

