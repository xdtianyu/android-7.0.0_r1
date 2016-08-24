// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_param_writer.h>

#include <string>

#include <brillo/any.h>
#include <gtest/gtest.h>

using dbus::MessageReader;
using dbus::MessageWriter;
using dbus::ObjectPath;
using dbus::Response;

namespace brillo {
namespace dbus_utils {

TEST(DBusParamWriter, Append_NoArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  DBusParamWriter::Append(&writer);
  EXPECT_EQ("", message->GetSignature());
}

TEST(DBusParamWriter, Append_OneArg) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  DBusParamWriter::Append(&writer, int32_t{2});
  EXPECT_EQ("i", message->GetSignature());
  DBusParamWriter::Append(&writer, std::string{"foo"});
  EXPECT_EQ("is", message->GetSignature());
  DBusParamWriter::Append(&writer, ObjectPath{"/o"});
  EXPECT_EQ("iso", message->GetSignature());

  int32_t int_value = 0;
  std::string string_value;
  ObjectPath path_value;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &string_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &path_value));

  EXPECT_EQ(2, int_value);
  EXPECT_EQ("foo", string_value);
  EXPECT_EQ(ObjectPath{"/o"}, path_value);
}

TEST(DBusParamWriter, Append_ManyArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  DBusParamWriter::Append(&writer, int32_t{9}, Any{7.5}, true);
  EXPECT_EQ("ivb", message->GetSignature());

  int32_t int_value = 0;
  Any variant_value;
  bool bool_value = false;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &variant_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &bool_value));

  EXPECT_EQ(9, int_value);
  EXPECT_DOUBLE_EQ(7.5, variant_value.Get<double>());
  EXPECT_TRUE(bool_value);
}

TEST(DBusParamWriter, AppendDBusOutParams_NoArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  DBusParamWriter::AppendDBusOutParams(&writer);
  EXPECT_EQ("", message->GetSignature());
}

TEST(DBusParamWriter, AppendDBusOutParams_OneArg) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  int32_t int_value_in{5};
  std::string string_value_in{"bar"};
  ObjectPath path_value_in{"/obj/path"};

  DBusParamWriter::AppendDBusOutParams(&writer, &int_value_in);
  EXPECT_EQ("i", message->GetSignature());
  DBusParamWriter::AppendDBusOutParams(&writer, &string_value_in);
  EXPECT_EQ("is", message->GetSignature());
  DBusParamWriter::AppendDBusOutParams(&writer, &path_value_in);
  EXPECT_EQ("iso", message->GetSignature());

  int32_t int_value = 0;
  std::string string_value;
  ObjectPath path_value;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &string_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &path_value));

  EXPECT_EQ(5, int_value);
  EXPECT_EQ("bar", string_value);
  EXPECT_EQ(ObjectPath{"/obj/path"}, path_value);
}

TEST(DBusParamWriter, AppendDBusOutParams_ManyArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  int32_t int_value_in{8};
  Any variant_value_in{8.5};
  bool bool_value_in{true};
  DBusParamWriter::AppendDBusOutParams(
      &writer, &int_value_in, &variant_value_in, &bool_value_in);
  EXPECT_EQ("ivb", message->GetSignature());

  int32_t int_value = 0;
  Any variant_value;
  bool bool_value = false;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &variant_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &bool_value));

  EXPECT_EQ(8, int_value);
  EXPECT_DOUBLE_EQ(8.5, variant_value.Get<double>());
  EXPECT_TRUE(bool_value);
}

TEST(DBusParamWriter, AppendDBusOutParams_Mixed_NoArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  DBusParamWriter::AppendDBusOutParams(&writer, 3, 5);
  EXPECT_EQ("", message->GetSignature());
}

TEST(DBusParamWriter, AppendDBusOutParams_Mixed_OneArg) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  int32_t int_value_in{5};
  std::string str_value_in{"bar"};
  ObjectPath path_value_in{"/obj"};

  DBusParamWriter::AppendDBusOutParams(&writer, 2, &int_value_in);
  EXPECT_EQ("i", message->GetSignature());
  DBusParamWriter::AppendDBusOutParams(&writer, &str_value_in, 0);
  EXPECT_EQ("is", message->GetSignature());
  DBusParamWriter::AppendDBusOutParams(&writer, 1, &path_value_in, 2);
  EXPECT_EQ("iso", message->GetSignature());

  int32_t int_value = 0;
  std::string string_value;
  ObjectPath path_value;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &string_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &path_value));

  EXPECT_EQ(5, int_value);
  EXPECT_EQ("bar", string_value);
  EXPECT_EQ(ObjectPath{"/obj"}, path_value);
}

TEST(DBusParamWriter, AppendDBusOutParams_Mixed_ManyArgs) {
  std::unique_ptr<Response> message(Response::CreateEmpty().release());
  MessageWriter writer(message.get());
  int32_t int_value_in{8};
  Any variant_value_in{7.5};
  bool bool_value_in{true};
  DBusParamWriter::AppendDBusOutParams(
      &writer, 0, &int_value_in, 1, &variant_value_in, 2, &bool_value_in, 3);
  EXPECT_EQ("ivb", message->GetSignature());

  int32_t int_value = 0;
  Any variant_value;
  bool bool_value = false;

  MessageReader reader(message.get());
  EXPECT_TRUE(PopValueFromReader(&reader, &int_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &variant_value));
  EXPECT_TRUE(PopValueFromReader(&reader, &bool_value));

  EXPECT_EQ(8, int_value);
  EXPECT_DOUBLE_EQ(7.5, variant_value.Get<double>());
  EXPECT_TRUE(bool_value);
}
}  // namespace dbus_utils
}  // namespace brillo
