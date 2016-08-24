// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/xml_interface_parser.h"

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "chromeos-dbus-bindings/interface.h"

using std::string;
using std::vector;
using testing::Test;

namespace chromeos_dbus_bindings {

namespace {

const char kBadInterfaceFileContents0[] = "This has no resemblance to XML";
const char kBadInterfaceFileContents1[] = "<node>";
const char kGoodInterfaceFileContents[] = R"literal_string(
<node name="/org/chromium/Test">
  <interface name="fi.w1.wpa_supplicant1.Interface">
    <method name="Scan">
      <arg name="args" type="a{sv}" direction="in"/>
      <annotation name="org.chromium.DBus.Method.Kind" value="async"/>
      <annotation name="org.chromium.DBus.Method.IncludeDBusMessage"
                  value="true"/>
    </method>
    <method name="GetBlob">
      <arg name="name" type="s"/>
      <arg name="data" type="ay" direction="out"/>
      <annotation name="org.chromium.DBus.Method.Const" value="true"/>
    </method>
    <property name="Capabilities" type="a{sv}" access="read"/>
    <signal name="BSSRemoved">
      <arg name="BSS" type="o"/>
    </signal>
  </interface>
  <interface name="DummyInterface" />
  <node name="/"/>
  <node/>
</node>
)literal_string";
const char kInterfaceName[] = "fi.w1.wpa_supplicant1.Interface";
const char kScanMethod[] = "Scan";
const char kArgsArgument[] = "args";
const char kArrayStringVariantType[] = "a{sv}";
const char kGetBlobMethod[] = "GetBlob";
const char kNameArgument[] = "name";
const char kDataArgument[] = "data";
const char kStringType[] = "s";
const char kArrayByteType[] = "ay";
const char kBssRemovedSignal[] = "BSSRemoved";
const char kBssArgument[] = "BSS";
const char kObjectType[] = "o";
const char kCapabilitiesProperty[] = "Capabilities";
const char kReadAccess[] = "read";
}  // namespace

class XmlInterfaceParserTest : public Test {
 protected:
  XmlInterfaceParser parser_;
};

TEST_F(XmlInterfaceParserTest, BadInputFile) {
  EXPECT_FALSE(parser_.ParseXmlInterfaceFile(kBadInterfaceFileContents0, {}));
  EXPECT_FALSE(parser_.ParseXmlInterfaceFile(kBadInterfaceFileContents1, {}));
}

TEST_F(XmlInterfaceParserTest, GoodInputFile) {
  EXPECT_TRUE(parser_.ParseXmlInterfaceFile(kGoodInterfaceFileContents,
                                            {"DummyInterface"}));
  const vector<Interface>& interfaces = parser_.interfaces();
  ASSERT_EQ(1u, interfaces.size());
  const Interface& interface = interfaces.back();

  EXPECT_EQ(kInterfaceName, interface.name);
  EXPECT_EQ("/org/chromium/Test", interface.path);
  ASSERT_EQ(2u, interface.methods.size());
  ASSERT_EQ(1u, interface.signals.size());

  // <method name="Scan">
  EXPECT_EQ(kScanMethod, interface.methods[0].name);
  EXPECT_EQ(Interface::Method::Kind::kAsync, interface.methods[0].kind);
  EXPECT_FALSE(interface.methods[0].is_const);
  EXPECT_TRUE(interface.methods[0].include_dbus_message);
  ASSERT_EQ(1u, interface.methods[0].input_arguments.size());

  // <arg name="args" type="a{sv}" direction="in"/>
  EXPECT_EQ(kArgsArgument, interface.methods[0].input_arguments[0].name);
  EXPECT_EQ(kArrayStringVariantType,
            interface.methods[0].input_arguments[0].type);
  EXPECT_EQ(0u, interface.methods[0].output_arguments.size());

  // <method name="GetBlob">
  EXPECT_EQ(kGetBlobMethod, interface.methods[1].name);
  EXPECT_EQ(Interface::Method::Kind::kNormal, interface.methods[1].kind);
  EXPECT_TRUE(interface.methods[1].is_const);
  EXPECT_FALSE(interface.methods[1].include_dbus_message);
  EXPECT_EQ(1u, interface.methods[1].input_arguments.size());
  EXPECT_EQ(1u, interface.methods[1].output_arguments.size());

  // <arg name="name" type="s"/>  (direction="in" is implicit)
  EXPECT_EQ(kNameArgument, interface.methods[1].input_arguments[0].name);
  EXPECT_EQ(kStringType, interface.methods[1].input_arguments[0].type);

  // <arg name="data" type="ay" direction="out"/>
  EXPECT_EQ(kDataArgument, interface.methods[1].output_arguments[0].name);
  EXPECT_EQ(kArrayByteType, interface.methods[1].output_arguments[0].type);

  // <signal name="BSSRemoved">
  EXPECT_EQ(kBssRemovedSignal, interface.signals[0].name);
  EXPECT_EQ(1u, interface.signals[0].arguments.size());

  // <arg name="BSS" type="o"/>
  EXPECT_EQ(kBssArgument, interface.signals[0].arguments[0].name);
  EXPECT_EQ(kObjectType, interface.signals[0].arguments[0].type);

  // <property name="Capabilities" type="s" access="read"/>
  EXPECT_EQ(kCapabilitiesProperty, interface.properties[0].name);
  EXPECT_EQ(kArrayStringVariantType, interface.properties[0].type);
  EXPECT_EQ(kReadAccess, interface.properties[0].access);
}

}  // namespace chromeos_dbus_bindings
