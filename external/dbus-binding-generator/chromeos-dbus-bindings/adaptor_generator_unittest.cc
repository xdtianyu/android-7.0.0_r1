// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos-dbus-bindings/adaptor_generator.h"

#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "chromeos-dbus-bindings/interface.h"
#include "chromeos-dbus-bindings/test_utils.h"

using std::string;
using std::vector;
using testing::Test;

namespace chromeos_dbus_bindings {

namespace {

const char kDBusTypeArryOfObjects[] = "ao";
const char kDBusTypeBool[] = "b";
const char kDBusTypeInt32[] = "i";
const char kDBusTypeInt64[] = "x";
const char kDBusTypeString[] = "s";

const char kPropertyAccessReadOnly[] = "read";
const char kPropertyAccessReadWrite[] = "readwrite";

const char kInterfaceName[] = "org.chromium.Test";
const char kInterfaceName2[] = "org.chromium.Test2";

const char kExpectedContent[] = R"literal_string(
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <base/macros.h>
#include <dbus/object_path.h>
#include <brillo/any.h>
#include <brillo/dbus/dbus_object.h>
#include <brillo/dbus/exported_object_manager.h>
#include <brillo/variant_dictionary.h>

namespace org {
namespace chromium {

// Interface definition for org::chromium::Test.
class TestInterface {
 public:
  virtual ~TestInterface() = default;

  virtual bool Kaneda(
      brillo::ErrorPtr* error,
      dbus::Message* message,
      const std::string& in_iwata,
      const std::vector<dbus::ObjectPath>& in_clarke,
      std::string* out_3) = 0;
  virtual bool Tetsuo(
      brillo::ErrorPtr* error,
      int32_t in_1,
      int64_t* out_2) = 0;
  virtual bool Kei(
      brillo::ErrorPtr* error) = 0;
  virtual bool Kiyoko(
      brillo::ErrorPtr* error,
      int64_t* out_akira,
      std::string* out_2) = 0;
};

// Interface adaptor for org::chromium::Test.
class TestAdaptor {
 public:
  TestAdaptor(TestInterface* interface) : interface_(interface) {}

  void RegisterWithDBusObject(brillo::dbus_utils::DBusObject* object) {
    brillo::dbus_utils::DBusInterface* itf =
        object->AddOrGetInterface("org.chromium.Test");

    itf->AddSimpleMethodHandlerWithErrorAndMessage(
        "Kaneda",
        base::Unretained(interface_),
        &TestInterface::Kaneda);
    itf->AddSimpleMethodHandlerWithError(
        "Tetsuo",
        base::Unretained(interface_),
        &TestInterface::Tetsuo);
    itf->AddSimpleMethodHandlerWithError(
        "Kei",
        base::Unretained(interface_),
        &TestInterface::Kei);
    itf->AddSimpleMethodHandlerWithError(
        "Kiyoko",
        base::Unretained(interface_),
        &TestInterface::Kiyoko);

    signal_Update_ = itf->RegisterSignalOfType<SignalUpdateType>("Update");
    signal_Mapping_ = itf->RegisterSignalOfType<SignalMappingType>("Mapping");

    itf->AddProperty(CharacterNameName(), &character_name_);
    write_property_.SetAccessMode(
        brillo::dbus_utils::ExportedPropertyBase::Access::kReadWrite);
    write_property_.SetValidator(
        base::Bind(&TestAdaptor::ValidateWriteProperty,
                   base::Unretained(this)));
    itf->AddProperty(WritePropertyName(), &write_property_);
  }

  void SendUpdateSignal() {
    auto signal = signal_Update_.lock();
    if (signal)
      signal->Send();
  }
  void SendMappingSignal(
      const std::string& in_key,
      const std::vector<dbus::ObjectPath>& in_2) {
    auto signal = signal_Mapping_.lock();
    if (signal)
      signal->Send(in_key, in_2);
  }

  static const char* CharacterNameName() { return "CharacterName"; }
  std::string GetCharacterName() const {
    return character_name_.GetValue().Get<std::string>();
  }
  void SetCharacterName(const std::string& character_name) {
    character_name_.SetValue(character_name);
  }

  static const char* WritePropertyName() { return "WriteProperty"; }
  std::string GetWriteProperty() const {
    return write_property_.GetValue().Get<std::string>();
  }
  void SetWriteProperty(const std::string& write_property) {
    write_property_.SetValue(write_property);
  }
  virtual bool ValidateWriteProperty(
      brillo::ErrorPtr* /*error*/, const std::string& /*value*/) {
    return true;
  }

  static dbus::ObjectPath GetObjectPath() {
    return dbus::ObjectPath{"/org/chromium/Test"};
  }

 private:
  using SignalUpdateType = brillo::dbus_utils::DBusSignal<>;
  std::weak_ptr<SignalUpdateType> signal_Update_;

  using SignalMappingType = brillo::dbus_utils::DBusSignal<
      std::string /*key*/,
      std::vector<dbus::ObjectPath>>;
  std::weak_ptr<SignalMappingType> signal_Mapping_;

  brillo::dbus_utils::ExportedProperty<std::string> character_name_;
  brillo::dbus_utils::ExportedProperty<std::string> write_property_;

  TestInterface* interface_;  // Owned by container of this adapter.

  DISALLOW_COPY_AND_ASSIGN(TestAdaptor);
};

}  // namespace chromium
}  // namespace org

namespace org {
namespace chromium {

// Interface definition for org::chromium::Test2.
class Test2Interface {
 public:
  virtual ~Test2Interface() = default;

  virtual std::string Kaneda2(
      const std::string& in_iwata) const = 0;
  virtual void Tetsuo2(
      std::unique_ptr<brillo::dbus_utils::DBusMethodResponse<int64_t>> response,
      int32_t in_1) = 0;
  virtual void Kei2(
      std::unique_ptr<brillo::dbus_utils::DBusMethodResponse<bool>> response,
      dbus::Message* message) = 0;
};

// Interface adaptor for org::chromium::Test2.
class Test2Adaptor {
 public:
  Test2Adaptor(Test2Interface* interface) : interface_(interface) {}

  void RegisterWithDBusObject(brillo::dbus_utils::DBusObject* object) {
    brillo::dbus_utils::DBusInterface* itf =
        object->AddOrGetInterface("org.chromium.Test2");

    itf->AddSimpleMethodHandler(
        "Kaneda2",
        base::Unretained(interface_),
        &Test2Interface::Kaneda2);
    itf->AddMethodHandler(
        "Tetsuo2",
        base::Unretained(interface_),
        &Test2Interface::Tetsuo2);
    itf->AddMethodHandlerWithMessage(
        "Kei2",
        base::Unretained(interface_),
        &Test2Interface::Kei2);
  }

 private:
  Test2Interface* interface_;  // Owned by container of this adapter.

  DISALLOW_COPY_AND_ASSIGN(Test2Adaptor);
};

}  // namespace chromium
}  // namespace org
)literal_string";

}  // namespace
class AdaptorGeneratorTest : public Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  base::FilePath CreateInputFile(const string& contents) {
    base::FilePath path;
    EXPECT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.path(), &path));
    int written = base::WriteFile(path, contents.c_str(), contents.size());
    EXPECT_EQ(contents.size(), static_cast<size_t>(written));
    return path;
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(AdaptorGeneratorTest, GenerateAdaptors) {
  Interface interface;
  interface.name = kInterfaceName;
  interface.path = "/org/chromium/Test";
  interface.methods.emplace_back(
      "Kaneda",
      vector<Interface::Argument>{
          {"iwata", kDBusTypeString},
          {"clarke", kDBusTypeArryOfObjects}},
      vector<Interface::Argument>{{"", kDBusTypeString}});
  interface.methods.back().include_dbus_message = true;
  interface.methods.emplace_back(
      "Tetsuo",
      vector<Interface::Argument>{{"", kDBusTypeInt32}},
      vector<Interface::Argument>{{"", kDBusTypeInt64}});
  interface.methods.emplace_back("Kei");
  // Interface methods with more than one return argument should be ignored.
  interface.methods.emplace_back(
      "Kiyoko",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{
          {"akira", kDBusTypeInt64},
          {"", kDBusTypeString}});
  // Signals generate helper methods to send them.
  interface.signals.emplace_back(
      "Update",
      vector<Interface::Argument>{});
  interface.signals.emplace_back(
      "Mapping",
      vector<Interface::Argument>{
          {"key", kDBusTypeString},
          {"", kDBusTypeArryOfObjects}});
  interface.properties.emplace_back(
      "CharacterName",
      kDBusTypeString,
      kPropertyAccessReadOnly);
  interface.properties.emplace_back(
      "WriteProperty",
      kDBusTypeString,
      kPropertyAccessReadWrite);

  Interface interface2;
  interface2.name = kInterfaceName2;
  interface2.methods.emplace_back(
      "Kaneda2",
      vector<Interface::Argument>{{"iwata", kDBusTypeString}},
      vector<Interface::Argument>{{"", kDBusTypeString}});
  interface2.methods.back().is_const = true;
  interface2.methods.back().kind = Interface::Method::Kind::kSimple;
  interface2.methods.emplace_back(
      "Tetsuo2",
      vector<Interface::Argument>{{"", kDBusTypeInt32}},
      vector<Interface::Argument>{{"", kDBusTypeInt64}});
  interface2.methods.back().kind = Interface::Method::Kind::kAsync;
  interface2.methods.emplace_back(
      "Kei2",
      vector<Interface::Argument>{},
      vector<Interface::Argument>{{"", kDBusTypeBool}});
  interface2.methods.back().kind = Interface::Method::Kind::kAsync;
  interface2.methods.back().include_dbus_message = true;

  base::FilePath output_path = temp_dir_.path().Append("output.h");
  EXPECT_TRUE(AdaptorGenerator::GenerateAdaptors({interface, interface2},
                                                 output_path));
  string contents;
  EXPECT_TRUE(base::ReadFileToString(output_path, &contents));
  // The header guards contain the (temporary) filename, so we search for
  // the content we need within the string.
  test_utils::EXPECT_TEXT_CONTAINED(kExpectedContent, contents);
}

}  // namespace chromeos_dbus_bindings
