// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/type_name_undecorate.h>

#include <brillo/variant_dictionary.h>
#include <gtest/gtest.h>

namespace brillo {

// Tests using tags from the __PRETTY_FUNCTION__ don't work when using RTTI
// to get the type name.
#ifndef USE_RTTI_FOR_TYPE_TAGS
TEST(TypeTags, GetTypeTag) {
  EXPECT_STREQ("const char *brillo::GetTypeTag() [T = int]", GetTypeTag<int>());
  EXPECT_STREQ("const char *brillo::GetTypeTag() [T = std::__1::map<std::__1::"
               "basic_string<char, std::__1::char_traits<char>, "
               "std::__1::allocator<char> >, brillo::Any, std::__1::less<"
               "std::__1::basic_string<char, std::__1::char_traits<char>, "
               "std::__1::allocator<char> > >, std::__1::allocator<std::__1::"
               "pair<const std::__1::basic_string<char, std::__1::char_traits"
               "<char>, std::__1::allocator<char> >, brillo::Any> > >]",
               GetTypeTag<VariantDictionary>());
  EXPECT_STREQ("const char *brillo::GetTypeTag() [T = int []]",
               GetTypeTag<int[]>());
}
#endif  // USE_RTTI_FOR_TYPE_TAGS

TEST(TypeDecoration, UndecorateTypeName) {
  EXPECT_EQ("int", UndecorateTypeName("i"));
  EXPECT_EQ("char const* brillo::GetTypeTag<unsigned long long>()",
            UndecorateTypeName("_ZN6brillo10GetTypeTagIyEEPKcv"));
  EXPECT_EQ("std::__1::to_string(int)",
            UndecorateTypeName("_ZNSt3__19to_stringEi"));
}

#ifndef USE_RTTI_FOR_TYPE_TAGS
TEST(TypeDecoration, GetUndecoratedTypeNameForTag) {
  EXPECT_EQ("int",
            GetUndecoratedTypeNameForTag(
                "const char *brillo::GetTypeTag() [T = int]"));
  EXPECT_EQ("int []",
            GetUndecoratedTypeNameForTag(
                "const char *brillo::GetTypeTag() [T = int []]"));
  EXPECT_EQ("foo::bar<int []>()",
            GetUndecoratedTypeNameForTag(
                "const char *brillo::GetTypeTag() [T = foo::bar<int []>()]"));
}

TEST(TypeDecoration, GetUndecoratedTypeName) {
  EXPECT_EQ("int", GetUndecoratedTypeName<int>());
  EXPECT_EQ("int *", GetUndecoratedTypeName<int*>());
  EXPECT_EQ("const int *", GetUndecoratedTypeName<const int*>());
  EXPECT_EQ("int []", GetUndecoratedTypeName<int[]>());
  EXPECT_EQ("bool", GetUndecoratedTypeName<bool>());
  EXPECT_EQ("char", GetUndecoratedTypeName<char>());
  EXPECT_EQ("float", GetUndecoratedTypeName<float>());
  EXPECT_EQ("double", GetUndecoratedTypeName<double>());
  EXPECT_EQ("long", GetUndecoratedTypeName<long>());
  EXPECT_EQ("std::__1::map<int, double, std::__1::less<int>, "
            "std::__1::allocator<std::__1::pair<const int, double> > >",
            (GetUndecoratedTypeName<std::map<int, double>>()));
}
#endif  // USE_RTTI_FOR_TYPE_TAGS

}  // namespace brillo
