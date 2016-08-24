// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_param_reader.h>

#include <string>

#include <brillo/variant_dictionary.h>
#include <gtest/gtest.h>

using dbus::MessageReader;
using dbus::MessageWriter;
using dbus::Response;

namespace brillo {
namespace dbus_utils {

TEST(DBusParamReader, NoArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called]() { called = true; };
  EXPECT_TRUE(DBusParamReader<false>::Invoke(callback, &reader, nullptr));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, OneArg) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, 123);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](int param1) {
    EXPECT_EQ(123, param1);
    called = true;
  };
  EXPECT_TRUE(
      (DBusParamReader<false, int>::Invoke(callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, ManyArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  AppendValueToWriter(&writer, 1972);
  AppendValueToWriter(&writer,
                      VariantDictionary{{"key", std::string{"value"}}});
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool p1, int p2, const VariantDictionary& p3) {
    EXPECT_TRUE(p1);
    EXPECT_EQ(1972, p2);
    EXPECT_EQ(1, p3.size());
    EXPECT_EQ("value", p3.find("key")->second.Get<std::string>());
    called = true;
  };
  EXPECT_TRUE((DBusParamReader<false, bool, int, VariantDictionary>::Invoke(
      callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, TooManyArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  AppendValueToWriter(&writer, 1972);
  AppendValueToWriter(&writer,
                      VariantDictionary{{"key", std::string{"value"}}});
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool param1, int param2) {
    EXPECT_TRUE(param1);
    EXPECT_EQ(1972, param2);
    called = true;
  };
  ErrorPtr error;
  EXPECT_FALSE(
      (DBusParamReader<false, bool, int>::Invoke(callback, &reader, &error)));
  EXPECT_FALSE(called);
  EXPECT_EQ(errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, error->GetCode());
  EXPECT_EQ("Too many parameters in a method call", error->GetMessage());
}

TEST(DBusParamReader, TooFewArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool param1, int param2) {
    EXPECT_TRUE(param1);
    EXPECT_EQ(1972, param2);
    called = true;
  };
  ErrorPtr error;
  EXPECT_FALSE(
      (DBusParamReader<false, bool, int>::Invoke(callback, &reader, &error)));
  EXPECT_FALSE(called);
  EXPECT_EQ(errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, error->GetCode());
  EXPECT_EQ("Too few parameters in a method call", error->GetMessage());
}

TEST(DBusParamReader, TypeMismatch) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  AppendValueToWriter(&writer, 1972);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool param1, double param2) {
    EXPECT_TRUE(param1);
    EXPECT_DOUBLE_EQ(1972.0, param2);
    called = true;
  };
  ErrorPtr error;
  EXPECT_FALSE((
      DBusParamReader<false, bool, double>::Invoke(callback, &reader, &error)));
  EXPECT_FALSE(called);
  EXPECT_EQ(errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, error->GetCode());
  EXPECT_EQ("Method parameter type mismatch", error->GetMessage());
}

TEST(DBusParamReader, NoArgs_With_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](int* param1) {
    EXPECT_EQ(0, *param1);
    called = true;
  };
  EXPECT_TRUE(
      (DBusParamReader<true, int*>::Invoke(callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, OneArg_Before_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, 123);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](int param1, double* param2) {
    EXPECT_EQ(123, param1);
    EXPECT_DOUBLE_EQ(0.0, *param2);
    called = true;
  };
  EXPECT_TRUE((
      DBusParamReader<true, int, double*>::Invoke(callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, OneArg_After_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, 123);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](double* param1, int param2) {
    EXPECT_DOUBLE_EQ(0.0, *param1);
    EXPECT_EQ(123, param2);
    called = true;
  };
  EXPECT_TRUE((
      DBusParamReader<true, double*, int>::Invoke(callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, ManyArgs_With_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  AppendValueToWriter(&writer, 1972);
  AppendValueToWriter(&writer,
                      VariantDictionary{{"key", std::string{"value"}}});
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool p1,
                            std::string* p2,
                            int p3,
                            int* p4,
                            const VariantDictionary& p5,
                            bool* p6) {
    EXPECT_TRUE(p1);
    EXPECT_EQ("", *p2);
    EXPECT_EQ(1972, p3);
    EXPECT_EQ(0, *p4);
    EXPECT_EQ(1, p5.size());
    EXPECT_EQ("value", p5.find("key")->second.Get<std::string>());
    EXPECT_FALSE(*p6);
    called = true;
  };
  EXPECT_TRUE((DBusParamReader<true,
                               bool,
                               std::string*,
                               int,
                               int*,
                               VariantDictionary,
                               bool*>::Invoke(callback, &reader, nullptr)));
  EXPECT_TRUE(called);
}

TEST(DBusParamReader, TooManyArgs_With_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  AppendValueToWriter(&writer, 1972);
  AppendValueToWriter(&writer,
                      VariantDictionary{{"key", std::string{"value"}}});
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool param1, int param2, int* param3) {
    EXPECT_TRUE(param1);
    EXPECT_EQ(1972, param2);
    EXPECT_EQ(0, *param3);
    called = true;
  };
  ErrorPtr error;
  EXPECT_FALSE((DBusParamReader<true, bool, int, int*>::Invoke(
      callback, &reader, &error)));
  EXPECT_FALSE(called);
  EXPECT_EQ(errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, error->GetCode());
  EXPECT_EQ("Too many parameters in a method call", error->GetMessage());
}

TEST(DBusParamReader, TooFewArgs_With_OUT) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  AppendValueToWriter(&writer, true);
  MessageReader reader(message.get());
  bool called = false;
  auto callback = [&called](bool param1, int param2, int* param3) {
    EXPECT_TRUE(param1);
    EXPECT_EQ(1972, param2);
    EXPECT_EQ(0, *param3);
    called = true;
  };
  ErrorPtr error;
  EXPECT_FALSE((DBusParamReader<true, bool, int, int*>::Invoke(
      callback, &reader, &error)));
  EXPECT_FALSE(called);
  EXPECT_EQ(errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, error->GetCode());
  EXPECT_EQ("Too few parameters in a method call", error->GetMessage());
}

}  // namespace dbus_utils
}  // namespace brillo
