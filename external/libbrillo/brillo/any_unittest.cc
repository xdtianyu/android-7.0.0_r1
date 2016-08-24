// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <brillo/any.h>
#include <gtest/gtest.h>

using brillo::Any;

TEST(Any, Empty) {
  Any val;
  EXPECT_TRUE(val.IsEmpty());

  Any val2 = val;
  EXPECT_TRUE(val.IsEmpty());
  EXPECT_TRUE(val2.IsEmpty());

  Any val3 = std::move(val);
  EXPECT_TRUE(val.IsEmpty());
  EXPECT_TRUE(val3.IsEmpty());
}

TEST(Any, SimpleTypes) {
  Any val(20);
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_TRUE(val.IsTypeCompatible<int>());
  EXPECT_EQ(20, val.Get<int>());

  Any val2(3.1415926);
  EXPECT_FALSE(val2.IsEmpty());
  EXPECT_TRUE(val2.IsTypeCompatible<double>());
  EXPECT_FALSE(val2.IsTypeCompatible<int>());
  EXPECT_DOUBLE_EQ(3.1415926, val2.Get<double>());

  Any val3(std::string("blah"));
  EXPECT_TRUE(val3.IsTypeCompatible<std::string>());
  EXPECT_EQ("blah", val3.Get<std::string>());
}

TEST(Any, Clear) {
  Any val('x');
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_EQ('x', val.Get<char>());

  val.Clear();
  EXPECT_TRUE(val.IsEmpty());
}

TEST(Any, Assignments) {
  Any val(20);
  EXPECT_EQ(20, val.Get<int>());

  val = 3.1415926;
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_TRUE(val.IsTypeCompatible<double>());
  EXPECT_DOUBLE_EQ(3.1415926, val.Get<double>());

  val = std::string("blah");
  EXPECT_EQ("blah", val.Get<std::string>());

  Any val2;
  EXPECT_TRUE(val2.IsEmpty());
  val2 = val;
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_FALSE(val2.IsEmpty());
  EXPECT_EQ("blah", val.Get<std::string>());
  EXPECT_EQ("blah", val2.Get<std::string>());
  val.Clear();
  EXPECT_TRUE(val.IsEmpty());
  EXPECT_EQ("blah", val2.Get<std::string>());
  val2.Clear();
  EXPECT_TRUE(val2.IsEmpty());

  val = std::vector<int>{100, 20, 3};
  auto v = val.Get<std::vector<int>>();
  EXPECT_EQ(100, v[0]);
  EXPECT_EQ(20, v[1]);
  EXPECT_EQ(3, v[2]);

  val2 = std::move(val);
  EXPECT_TRUE(val.IsEmpty());
  EXPECT_TRUE(val2.IsTypeCompatible<std::vector<int>>());
  EXPECT_EQ(3, val2.Get<std::vector<int>>().size());

  val = val2;
  EXPECT_TRUE(val.IsTypeCompatible<std::vector<int>>());
  EXPECT_TRUE(val2.IsTypeCompatible<std::vector<int>>());
  EXPECT_EQ(3, val.Get<std::vector<int>>().size());
  EXPECT_EQ(3, val2.Get<std::vector<int>>().size());
}

TEST(Any, Enums) {
  enum class Dummy { foo, bar, baz };
  Any val(Dummy::bar);
  EXPECT_FALSE(val.IsEmpty());
  EXPECT_TRUE(val.IsConvertibleToInteger());
  EXPECT_EQ(Dummy::bar, val.Get<Dummy>());
  EXPECT_EQ(1, val.GetAsInteger());

  val = Dummy::baz;
  EXPECT_EQ(2, val.GetAsInteger());

  val = Dummy::foo;
  EXPECT_EQ(0, val.GetAsInteger());
}

TEST(Any, Integers) {
  Any val(14);
  EXPECT_TRUE(val.IsConvertibleToInteger());
  EXPECT_EQ(14, val.Get<int>());
  EXPECT_EQ(14, val.GetAsInteger());

  val = '\x40';
  EXPECT_TRUE(val.IsConvertibleToInteger());
  EXPECT_EQ(64, val.Get<char>());
  EXPECT_EQ(64, val.GetAsInteger());

  val = static_cast<uint16_t>(65535);
  EXPECT_TRUE(val.IsConvertibleToInteger());
  EXPECT_EQ(65535, val.Get<uint16_t>());
  EXPECT_EQ(65535, val.GetAsInteger());

  val = static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL);
  EXPECT_TRUE(val.IsConvertibleToInteger());
  EXPECT_EQ(0xFFFFFFFFFFFFFFFFULL, val.Get<uint64_t>());
  EXPECT_EQ(-1, val.GetAsInteger());

  val = "abc";
  EXPECT_FALSE(val.IsConvertibleToInteger());

  int a = 5;
  val = &a;
  EXPECT_FALSE(val.IsConvertibleToInteger());
}

TEST(Any, Pointers) {
  Any val("abc");  // const char*
  EXPECT_FALSE(val.IsTypeCompatible<char*>());
  EXPECT_TRUE(val.IsTypeCompatible<const char*>());
  EXPECT_FALSE(val.IsTypeCompatible<volatile char*>());
  EXPECT_TRUE(val.IsTypeCompatible<volatile const char*>());
  EXPECT_STREQ("abc", val.Get<const char*>());

  int a = 10;
  val = &a;
  EXPECT_TRUE(val.IsTypeCompatible<int*>());
  EXPECT_TRUE(val.IsTypeCompatible<const int*>());
  EXPECT_TRUE(val.IsTypeCompatible<volatile int*>());
  EXPECT_TRUE(val.IsTypeCompatible<volatile const int*>());
  EXPECT_EQ(10, *val.Get<const int*>());
  *val.Get<int*>() = 3;
  EXPECT_EQ(3, a);
}

TEST(Any, Arrays) {
  // The following test are here to validate the array-to-pointer decay rules.
  // Since Any does not store the contents of a C-style array, just a pointer
  // to the data, putting array data into Any could be dangerous.
  // Make sure the array's lifetime exceeds that of an Any containing the
  // pointer to the array data.
  // If you want to store the array with data, use corresponding value types
  // such as std::vector or a struct containing C-style array as a member.

  int int_array[] = {1, 2, 3};  // int*
  Any val = int_array;
  EXPECT_TRUE(val.IsTypeCompatible<int*>());
  EXPECT_TRUE(val.IsTypeCompatible<const int*>());
  EXPECT_TRUE(val.IsTypeCompatible<int[]>());
  EXPECT_TRUE(val.IsTypeCompatible<const int[]>());
  EXPECT_EQ(3, val.Get<int*>()[2]);

  const int const_int_array[] = {10, 20, 30};  // const int*
  val = const_int_array;
  EXPECT_FALSE(val.IsTypeCompatible<int*>());
  EXPECT_TRUE(val.IsTypeCompatible<const int*>());
  EXPECT_FALSE(val.IsTypeCompatible<int[]>());
  EXPECT_TRUE(val.IsTypeCompatible<const int[]>());
  EXPECT_EQ(30, val.Get<const int*>()[2]);
}

TEST(Any, References) {
  // Passing references to object via Any might be error-prone or the
  // semantics could be unfamiliar to other developers. In many cases,
  // using pointers instead of references are more conventional and easier
  // to understand. Even though the cases of passing references are quite
  // explicit on both storing and retrieving ends, you might want to
  // use pointers instead anyway.

  int a = 5;
  Any val(std::ref(a));  // int&
  EXPECT_EQ(5, val.Get<std::reference_wrapper<int>>().get());
  val.Get<std::reference_wrapper<int>>().get() = 7;
  EXPECT_EQ(7, val.Get<std::reference_wrapper<int>>().get());
  EXPECT_EQ(7, a);

  Any val2(std::cref(a));  // const int&
  EXPECT_EQ(7, val2.Get<std::reference_wrapper<const int>>().get());

  a = 10;
  EXPECT_EQ(10, val.Get<std::reference_wrapper<int>>().get());
  EXPECT_EQ(10, val2.Get<std::reference_wrapper<const int>>().get());
}

TEST(Any, CustomTypes) {
  struct Person {
    std::string name;
    int age;
  };
  Any val(Person{"Jack", 40});
  Any val2 = val;
  EXPECT_EQ("Jack", val.Get<Person>().name);
  val.GetPtr<Person>()->name = "Joe";
  val.GetPtr<Person>()->age /= 2;
  EXPECT_EQ("Joe", val.Get<Person>().name);
  EXPECT_EQ(20, val.Get<Person>().age);
  EXPECT_EQ("Jack", val2.Get<Person>().name);
  EXPECT_EQ(40, val2.Get<Person>().age);
}

TEST(Any, Swap) {
  Any val(12);
  Any val2(2.7);
  EXPECT_EQ(12, val.Get<int>());
  EXPECT_EQ(2.7, val2.Get<double>());

  val.Swap(val2);
  EXPECT_EQ(2.7, val.Get<double>());
  EXPECT_EQ(12, val2.Get<int>());

  std::swap(val, val2);
  EXPECT_EQ(12, val.Get<int>());
  EXPECT_EQ(2.7, val2.Get<double>());
}

TEST(Any, TypeMismatch) {
  Any val(12);
  EXPECT_DEATH(val.Get<double>(),
               "Requesting value of type 'double' from variant containing "
               "'int'");

  val = std::string("123");
  EXPECT_DEATH(val.GetAsInteger(),
               "Unable to convert value of type 'std::.*' to integer");

  Any empty;
  EXPECT_DEATH(empty.GetAsInteger(), "Must not be called on an empty Any");
}

TEST(Any, TryGet) {
  Any val(12);
  Any empty;
  EXPECT_EQ("dummy", val.TryGet<std::string>("dummy"));
  EXPECT_EQ(12, val.TryGet<int>(17));
  EXPECT_EQ(17, empty.TryGet<int>(17));
}

TEST(Any, Compare_Int) {
  Any int1{12};
  Any int2{12};
  Any int3{20};
  EXPECT_EQ(int1, int2);
  EXPECT_NE(int2, int3);
}

TEST(Any, Compare_String) {
  Any str1{std::string{"foo"}};
  Any str2{std::string{"foo"}};
  Any str3{std::string{"bar"}};
  EXPECT_EQ(str1, str2);
  EXPECT_NE(str2, str3);
}

TEST(Any, Compare_Array) {
  Any vec1{std::vector<int>{1, 2}};
  Any vec2{std::vector<int>{1, 2}};
  Any vec3{std::vector<int>{1, 2, 3}};
  EXPECT_EQ(vec1, vec2);
  EXPECT_NE(vec2, vec3);
}

TEST(Any, Compare_Empty) {
  Any empty1;
  Any empty2;
  Any int1{1};
  EXPECT_EQ(empty1, empty2);
  EXPECT_NE(int1, empty1);
  EXPECT_NE(empty2, int1);
}

TEST(Any, Compare_NonComparable) {
  struct Person {
    std::string name;
    int age;
  };
  Any person1(Person{"Jack", 40});
  Any person2 = person1;
  Any person3(Person{"Jill", 20});
  EXPECT_NE(person1, person2);
  EXPECT_NE(person1, person3);
  EXPECT_NE(person2, person3);
}

TEST(Any, GetUndecoratedTypeName) {
  Any val;
  EXPECT_TRUE(val.GetUndecoratedTypeName().empty());

  val = 1;
  EXPECT_EQ(brillo::GetUndecoratedTypeName<int>(),
            val.GetUndecoratedTypeName());

  val = 3.1415926;
  EXPECT_EQ(brillo::GetUndecoratedTypeName<double>(),
            val.GetUndecoratedTypeName());

  val = std::string("blah");
  EXPECT_EQ(brillo::GetUndecoratedTypeName<std::string>(),
            val.GetUndecoratedTypeName());
}
