// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/name_parser.h"

#include <map>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "chromeos-dbus-bindings/indented_text.h"

namespace chromeos_dbus_bindings {

TEST(NameParser, Parsing_Empty) {
  EXPECT_DEATH(NameParser{""}, "Empty name specified");
}

TEST(NameParser, Parsing_NoNamespaces) {
  NameParser parser{"foo"};
  EXPECT_EQ("foo", parser.type_name);
  EXPECT_TRUE(parser.namespaces.empty());
}

TEST(NameParser, Parsing_FullyQualified) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("FooBar", parser.type_name);
  EXPECT_THAT(parser.namespaces, testing::ElementsAre("foo", "bar"));
}

TEST(NameParser, MakeFullCppName) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("foo::bar::FooBar", parser.MakeFullCppName());
}

TEST(NameParser, MakeVariableName) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("foo_bar", parser.MakeVariableName());
}

TEST(NameParser, MakeVariableName_NoCapitals) {
  NameParser parser{"foo"};
  EXPECT_EQ("foo", parser.MakeVariableName());
}

TEST(NameParser, MakeVariableName_NoInitialCapital) {
  NameParser parser{"fooBarBaz"};
  EXPECT_EQ("foo_bar_baz", parser.MakeVariableName());
}

TEST(NameParser, MakeVariableName_AllCapitals) {
  NameParser parser{"UUID"};
  EXPECT_EQ("uuid", parser.MakeVariableName());
}

TEST(NameParser, MakeVariableName_MixedCapital) {
  NameParser parser{"FOObarBaz"};
  EXPECT_EQ("foobar_baz", parser.MakeVariableName());
}

TEST(NameParser, MakeInterfaceName) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("FooBarInterface", parser.MakeInterfaceName(false));
  EXPECT_EQ("foo::bar::FooBarInterface", parser.MakeInterfaceName(true));
}

TEST(NameParser, MakeProxyName) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("FooBarProxy", parser.MakeProxyName(false));
  EXPECT_EQ("foo::bar::FooBarProxy", parser.MakeProxyName(true));
}

TEST(NameParser, MakeAdaptorName) {
  NameParser parser{"foo.bar.FooBar"};
  EXPECT_EQ("FooBarAdaptor", parser.MakeAdaptorName(false));
  EXPECT_EQ("foo::bar::FooBarAdaptor", parser.MakeAdaptorName(true));
}

TEST(NameParser, AddOpenNamespaces) {
  std::string expected =
    "namespace foo {\n"
    "namespace bar {\n";
  NameParser parser{"foo.bar.FooBar"};
  IndentedText text;
  parser.AddOpenNamespaces(&text, false);
  EXPECT_EQ(expected, text.GetContents());
}

TEST(NameParser, AddOpenNamespaces_WithMainType) {
  std::string expected =
    "namespace foo {\n"
    "namespace bar {\n"
    "namespace FooBar {\n";
  NameParser parser{"foo.bar.FooBar"};
  IndentedText text;
  parser.AddOpenNamespaces(&text, true);
  EXPECT_EQ(expected, text.GetContents());
}

TEST(NameParser, AddCloseNamespaces) {
  std::string expected =
    "}  // namespace bar\n"
    "}  // namespace foo\n";
  NameParser parser{"foo.bar.FooBar"};
  IndentedText text;
  parser.AddCloseNamespaces(&text, false);
  EXPECT_EQ(expected, text.GetContents());
}

TEST(NameParser, AddCloseNamespaces_WithMainType) {
  std::string expected =
    "}  // namespace FooBar\n"
    "}  // namespace bar\n"
    "}  // namespace foo\n";
  NameParser parser{"foo.bar.FooBar"};
  IndentedText text;
  parser.AddCloseNamespaces(&text, true);
  EXPECT_EQ(expected, text.GetContents());
}

}  // namespace chromeos_dbus_bindings
