// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/exported_property_set.h>

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/macros.h>
#include <brillo/dbus/dbus_object.h>
#include <brillo/dbus/dbus_object_test_helpers.h>
#include <brillo/errors/error_codes.h>
#include <dbus/message.h>
#include <dbus/property.h>
#include <dbus/object_path.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_exported_object.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::Unused;

namespace brillo {

namespace dbus_utils {

namespace {

const char kBoolPropName[] = "BoolProp";
const char kUint8PropName[] = "Uint8Prop";
const char kInt16PropName[] = "Int16Prop";
const char kUint16PropName[] = "Uint16Prop";
const char kInt32PropName[] = "Int32Prop";
const char kUint32PropName[] = "Uint32Prop";
const char kInt64PropName[] = "Int64Prop";
const char kUint64PropName[] = "Uint64Prop";
const char kDoublePropName[] = "DoubleProp";
const char kStringPropName[] = "StringProp";
const char kPathPropName[] = "PathProp";
const char kStringListPropName[] = "StringListProp";
const char kPathListPropName[] = "PathListProp";
const char kUint8ListPropName[] = "Uint8ListProp";

const char kTestInterface1[] = "org.chromium.TestInterface1";
const char kTestInterface2[] = "org.chromium.TestInterface2";
const char kTestInterface3[] = "org.chromium.TestInterface3";

const std::string kTestString("lies");
const dbus::ObjectPath kMethodsExportedOnPath(std::string("/export"));
const dbus::ObjectPath kTestObjectPathInit(std::string("/path_init"));
const dbus::ObjectPath kTestObjectPathUpdate(std::string("/path_update"));

}  // namespace

class ExportedPropertySetTest : public ::testing::Test {
 public:
  struct Properties {
   public:
    ExportedProperty<bool> bool_prop_;
    ExportedProperty<uint8_t> uint8_prop_;
    ExportedProperty<int16_t> int16_prop_;
    ExportedProperty<uint16_t> uint16_prop_;
    ExportedProperty<int32_t> int32_prop_;
    ExportedProperty<uint32_t> uint32_prop_;
    ExportedProperty<int64_t> int64_prop_;
    ExportedProperty<uint64_t> uint64_prop_;
    ExportedProperty<double> double_prop_;
    ExportedProperty<std::string> string_prop_;
    ExportedProperty<dbus::ObjectPath> path_prop_;
    ExportedProperty<std::vector<std::string>> stringlist_prop_;
    ExportedProperty<std::vector<dbus::ObjectPath>> pathlist_prop_;
    ExportedProperty<std::vector<uint8_t>> uint8list_prop_;

    Properties(scoped_refptr<dbus::Bus> bus, const dbus::ObjectPath& path)
        : dbus_object_(nullptr, bus, path) {
      // The empty string is not a valid value for an ObjectPath.
      path_prop_.SetValue(kTestObjectPathInit);
      DBusInterface* itf1 = dbus_object_.AddOrGetInterface(kTestInterface1);
      itf1->AddProperty(kBoolPropName, &bool_prop_);
      itf1->AddProperty(kUint8PropName, &uint8_prop_);
      itf1->AddProperty(kInt16PropName, &int16_prop_);
      // I chose this weird grouping because N=2 is about all the permutations
      // of GetAll that I want to anticipate.
      DBusInterface* itf2 = dbus_object_.AddOrGetInterface(kTestInterface2);
      itf2->AddProperty(kUint16PropName, &uint16_prop_);
      itf2->AddProperty(kInt32PropName, &int32_prop_);
      DBusInterface* itf3 = dbus_object_.AddOrGetInterface(kTestInterface3);
      itf3->AddProperty(kUint32PropName, &uint32_prop_);
      itf3->AddProperty(kInt64PropName, &int64_prop_);
      itf3->AddProperty(kUint64PropName, &uint64_prop_);
      itf3->AddProperty(kDoublePropName, &double_prop_);
      itf3->AddProperty(kStringPropName, &string_prop_);
      itf3->AddProperty(kPathPropName, &path_prop_);
      itf3->AddProperty(kStringListPropName, &stringlist_prop_);
      itf3->AddProperty(kPathListPropName, &pathlist_prop_);
      itf3->AddProperty(kUint8ListPropName, &uint8list_prop_);
      dbus_object_.RegisterAsync(
          AsyncEventSequencer::GetDefaultCompletionAction());
    }
    virtual ~Properties() {}

    DBusObject dbus_object_;
  };

  void SetUp() override {
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    bus_ = new dbus::MockBus(options);
    // By default, don't worry about threading assertions.
    EXPECT_CALL(*bus_, AssertOnOriginThread()).Times(AnyNumber());
    EXPECT_CALL(*bus_, AssertOnDBusThread()).Times(AnyNumber());
    // Use a mock exported object.
    mock_exported_object_ =
        new dbus::MockExportedObject(bus_.get(), kMethodsExportedOnPath);
    EXPECT_CALL(*bus_, GetExportedObject(kMethodsExportedOnPath))
        .Times(1).WillOnce(Return(mock_exported_object_.get()));

    EXPECT_CALL(*mock_exported_object_,
                ExportMethod(dbus::kPropertiesInterface, _, _, _)).Times(3);
    p_.reset(new Properties(bus_, kMethodsExportedOnPath));
  }

  void TearDown() override {
    EXPECT_CALL(*mock_exported_object_, Unregister()).Times(1);
  }

  void AssertMethodReturnsError(dbus::MethodCall* method_call) {
    method_call->SetSerial(123);
    auto response = testing::CallMethod(p_->dbus_object_, method_call);
    ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  }

  std::unique_ptr<dbus::Response> GetPropertyOnInterface(
      const std::string& interface_name,
      const std::string& property_name) {
    dbus::MethodCall method_call(dbus::kPropertiesInterface,
                                 dbus::kPropertiesGet);
    method_call.SetSerial(123);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(interface_name);
    writer.AppendString(property_name);
    return testing::CallMethod(p_->dbus_object_, &method_call);
  }

  std::unique_ptr<dbus::Response> SetPropertyOnInterface(
      const std::string& interface_name,
      const std::string& property_name,
      const brillo::Any& value) {
    dbus::MethodCall method_call(dbus::kPropertiesInterface,
                                 dbus::kPropertiesSet);
    method_call.SetSerial(123);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(interface_name);
    writer.AppendString(property_name);
    dbus_utils::AppendValueToWriter(&writer, value);
    return testing::CallMethod(p_->dbus_object_, &method_call);
  }

  std::unique_ptr<dbus::Response> last_response_;
  scoped_refptr<dbus::MockBus> bus_;
  scoped_refptr<dbus::MockExportedObject> mock_exported_object_;
  std::unique_ptr<Properties> p_;
};

template<typename T>
class PropertyValidatorObserver {
 public:
  PropertyValidatorObserver()
      : validate_property_callback_(
            base::Bind(&PropertyValidatorObserver::ValidateProperty,
                       base::Unretained(this))) {}
  virtual ~PropertyValidatorObserver() {}

  MOCK_METHOD2_T(ValidateProperty,
                 bool(brillo::ErrorPtr* error, const T& value));

  const base::Callback<bool(brillo::ErrorPtr*, const T&)>&
  validate_property_callback() const {
    return validate_property_callback_;
  }

 private:
  base::Callback<bool(brillo::ErrorPtr*, const T&)>
      validate_property_callback_;

  DISALLOW_COPY_AND_ASSIGN(PropertyValidatorObserver);
};

TEST_F(ExportedPropertySetTest, UpdateNotifications) {
  EXPECT_CALL(*mock_exported_object_, SendSignal(_)).Times(14);
  p_->bool_prop_.SetValue(true);
  p_->uint8_prop_.SetValue(1);
  p_->int16_prop_.SetValue(1);
  p_->uint16_prop_.SetValue(1);
  p_->int32_prop_.SetValue(1);
  p_->uint32_prop_.SetValue(1);
  p_->int64_prop_.SetValue(1);
  p_->uint64_prop_.SetValue(1);
  p_->double_prop_.SetValue(1.0);
  p_->string_prop_.SetValue(kTestString);
  p_->path_prop_.SetValue(kTestObjectPathUpdate);
  p_->stringlist_prop_.SetValue({kTestString});
  p_->pathlist_prop_.SetValue({kTestObjectPathUpdate});
  p_->uint8list_prop_.SetValue({1});
}

TEST_F(ExportedPropertySetTest, UpdateToSameValue) {
  EXPECT_CALL(*mock_exported_object_, SendSignal(_)).Times(1);
  p_->bool_prop_.SetValue(true);
  p_->bool_prop_.SetValue(true);
}

TEST_F(ExportedPropertySetTest, GetAllNoArgs) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGetAll);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetAllInvalidInterface) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGetAll);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("org.chromium.BadInterface");
  auto response = testing::CallMethod(p_->dbus_object_, &method_call);
  dbus::MessageReader response_reader(response.get());
  dbus::MessageReader dict_reader(nullptr);
  ASSERT_TRUE(response_reader.PopArray(&dict_reader));
  // The response should just be a an empty array, since there are no properties
  // on this interface.  The spec doesn't say much about error conditions here,
  // so I'm going to assume this is a valid implementation.
  ASSERT_FALSE(dict_reader.HasMoreData());
  ASSERT_FALSE(response_reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetAllExtraArgs) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGetAll);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface1);
  writer.AppendString(kTestInterface1);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetAllCorrectness) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGetAll);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface2);
  auto response = testing::CallMethod(p_->dbus_object_, &method_call);
  dbus::MessageReader response_reader(response.get());
  dbus::MessageReader dict_reader(nullptr);
  dbus::MessageReader entry_reader(nullptr);
  ASSERT_TRUE(response_reader.PopArray(&dict_reader));
  ASSERT_TRUE(dict_reader.PopDictEntry(&entry_reader));
  std::string property_name;
  ASSERT_TRUE(entry_reader.PopString(&property_name));
  uint16_t value16;
  int32_t value32;
  if (property_name.compare(kUint16PropName) == 0) {
    ASSERT_TRUE(entry_reader.PopVariantOfUint16(&value16));
    ASSERT_FALSE(entry_reader.HasMoreData());
    ASSERT_TRUE(dict_reader.PopDictEntry(&entry_reader));
    ASSERT_TRUE(entry_reader.PopString(&property_name));
    ASSERT_EQ(property_name.compare(kInt32PropName), 0);
    ASSERT_TRUE(entry_reader.PopVariantOfInt32(&value32));
  } else {
    ASSERT_EQ(property_name.compare(kInt32PropName), 0);
    ASSERT_TRUE(entry_reader.PopVariantOfInt32(&value32));
    ASSERT_FALSE(entry_reader.HasMoreData());
    ASSERT_TRUE(dict_reader.PopDictEntry(&entry_reader));
    ASSERT_TRUE(entry_reader.PopString(&property_name));
    ASSERT_EQ(property_name.compare(kUint16PropName), 0);
    ASSERT_TRUE(entry_reader.PopVariantOfUint16(&value16));
  }
  ASSERT_FALSE(entry_reader.HasMoreData());
  ASSERT_FALSE(dict_reader.HasMoreData());
  ASSERT_FALSE(response_reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetNoArgs) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetInvalidInterface) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("org.chromium.BadInterface");
  writer.AppendString(kInt16PropName);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetBadPropertyName) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface1);
  writer.AppendString("IAmNotAProperty");
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetPropIfMismatch) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface1);
  writer.AppendString(kStringPropName);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetNoPropertyName) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface1);
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetExtraArgs) {
  dbus::MethodCall method_call(dbus::kPropertiesInterface,
                               dbus::kPropertiesGet);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString(kTestInterface1);
  writer.AppendString(kBoolPropName);
  writer.AppendString("Extra param");
  AssertMethodReturnsError(&method_call);
}

TEST_F(ExportedPropertySetTest, GetWorksWithBool) {
  auto response = GetPropertyOnInterface(kTestInterface1, kBoolPropName);
  dbus::MessageReader reader(response.get());
  bool value;
  ASSERT_TRUE(reader.PopVariantOfBool(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithUint8) {
  auto response = GetPropertyOnInterface(kTestInterface1, kUint8PropName);
  dbus::MessageReader reader(response.get());
  uint8_t value;
  ASSERT_TRUE(reader.PopVariantOfByte(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithInt16) {
  auto response = GetPropertyOnInterface(kTestInterface1, kInt16PropName);
  dbus::MessageReader reader(response.get());
  int16_t value;
  ASSERT_TRUE(reader.PopVariantOfInt16(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithUint16) {
  auto response = GetPropertyOnInterface(kTestInterface2, kUint16PropName);
  dbus::MessageReader reader(response.get());
  uint16_t value;
  ASSERT_TRUE(reader.PopVariantOfUint16(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithInt32) {
  auto response = GetPropertyOnInterface(kTestInterface2, kInt32PropName);
  dbus::MessageReader reader(response.get());
  int32_t value;
  ASSERT_TRUE(reader.PopVariantOfInt32(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithUint32) {
  auto response = GetPropertyOnInterface(kTestInterface3, kUint32PropName);
  dbus::MessageReader reader(response.get());
  uint32_t value;
  ASSERT_TRUE(reader.PopVariantOfUint32(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithInt64) {
  auto response = GetPropertyOnInterface(kTestInterface3, kInt64PropName);
  dbus::MessageReader reader(response.get());
  int64_t value;
  ASSERT_TRUE(reader.PopVariantOfInt64(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithUint64) {
  auto response = GetPropertyOnInterface(kTestInterface3, kUint64PropName);
  dbus::MessageReader reader(response.get());
  uint64_t value;
  ASSERT_TRUE(reader.PopVariantOfUint64(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithDouble) {
  auto response = GetPropertyOnInterface(kTestInterface3, kDoublePropName);
  dbus::MessageReader reader(response.get());
  double value;
  ASSERT_TRUE(reader.PopVariantOfDouble(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithString) {
  auto response = GetPropertyOnInterface(kTestInterface3, kStringPropName);
  dbus::MessageReader reader(response.get());
  std::string value;
  ASSERT_TRUE(reader.PopVariantOfString(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithPath) {
  auto response = GetPropertyOnInterface(kTestInterface3, kPathPropName);
  dbus::MessageReader reader(response.get());
  dbus::ObjectPath value;
  ASSERT_TRUE(reader.PopVariantOfObjectPath(&value));
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithStringList) {
  auto response = GetPropertyOnInterface(kTestInterface3, kStringListPropName);
  dbus::MessageReader reader(response.get());
  dbus::MessageReader variant_reader(nullptr);
  std::vector<std::string> value;
  ASSERT_TRUE(reader.PopVariant(&variant_reader));
  ASSERT_TRUE(variant_reader.PopArrayOfStrings(&value));
  ASSERT_FALSE(variant_reader.HasMoreData());
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithPathList) {
  auto response = GetPropertyOnInterface(kTestInterface3, kPathListPropName);
  dbus::MessageReader reader(response.get());
  dbus::MessageReader variant_reader(nullptr);
  std::vector<dbus::ObjectPath> value;
  ASSERT_TRUE(reader.PopVariant(&variant_reader));
  ASSERT_TRUE(variant_reader.PopArrayOfObjectPaths(&value));
  ASSERT_FALSE(variant_reader.HasMoreData());
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, GetWorksWithUint8List) {
  auto response = GetPropertyOnInterface(kTestInterface3, kPathListPropName);
  dbus::MessageReader reader(response.get());
  dbus::MessageReader variant_reader(nullptr);
  const uint8_t* buffer;
  size_t buffer_len;
  ASSERT_TRUE(reader.PopVariant(&variant_reader));
  // |buffer| remains under the control of the MessageReader.
  ASSERT_TRUE(variant_reader.PopArrayOfBytes(&buffer, &buffer_len));
  ASSERT_FALSE(variant_reader.HasMoreData());
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(ExportedPropertySetTest, SetInvalidInterface) {
  auto response = SetPropertyOnInterface(
      "BadInterfaceName", kStringPropName, brillo::Any(kTestString));
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(DBUS_ERROR_UNKNOWN_INTERFACE, response->GetErrorName());
}

TEST_F(ExportedPropertySetTest, SetBadPropertyName) {
  auto response = SetPropertyOnInterface(
      kTestInterface3, "IAmNotAProperty", brillo::Any(kTestString));
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(DBUS_ERROR_UNKNOWN_PROPERTY, response->GetErrorName());
}

TEST_F(ExportedPropertySetTest, SetFailsWithReadOnlyProperty) {
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(kTestString));
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(DBUS_ERROR_PROPERTY_READ_ONLY, response->GetErrorName());
}

TEST_F(ExportedPropertySetTest, SetFailsWithMismatchedValueType) {
  p_->string_prop_.SetAccessMode(ExportedPropertyBase::Access::kReadWrite);
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(true));
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(DBUS_ERROR_INVALID_ARGS, response->GetErrorName());
}

namespace {

bool SetInvalidProperty(brillo::ErrorPtr* error, Unused) {
  brillo::Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                       DBUS_ERROR_INVALID_ARGS, "Invalid value");
  return false;
}

}  // namespace

TEST_F(ExportedPropertySetTest, SetFailsWithValidator) {
  PropertyValidatorObserver<std::string> property_validator;
  p_->string_prop_.SetAccessMode(ExportedPropertyBase::Access::kReadWrite);
  p_->string_prop_.SetValidator(
      property_validator.validate_property_callback());

  brillo::ErrorPtr error = brillo::Error::Create(
      FROM_HERE, errors::dbus::kDomain, DBUS_ERROR_INVALID_ARGS, "");
  EXPECT_CALL(property_validator, ValidateProperty(_, kTestString))
      .WillOnce(Invoke(SetInvalidProperty));
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(kTestString));
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(DBUS_ERROR_INVALID_ARGS, response->GetErrorName());
}

TEST_F(ExportedPropertySetTest, SetWorksWithValidator) {
  PropertyValidatorObserver<std::string> property_validator;
  p_->string_prop_.SetAccessMode(ExportedPropertyBase::Access::kReadWrite);
  p_->string_prop_.SetValidator(
      property_validator.validate_property_callback());

  EXPECT_CALL(property_validator, ValidateProperty(_, kTestString))
      .WillOnce(Return(true));
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(kTestString));
  ASSERT_NE(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(kTestString, p_->string_prop_.value());
}

TEST_F(ExportedPropertySetTest, SetWorksWithSameValue) {
  PropertyValidatorObserver<std::string> property_validator;
  p_->string_prop_.SetAccessMode(ExportedPropertyBase::Access::kReadWrite);
  p_->string_prop_.SetValidator(
      property_validator.validate_property_callback());
  EXPECT_CALL(*mock_exported_object_, SendSignal(_)).Times(1);
  p_->string_prop_.SetValue(kTestString);

  // No need to validate the value if it is the same as the current one.
  EXPECT_CALL(property_validator, ValidateProperty(_, _)).Times(0);
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(kTestString));
  ASSERT_NE(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(kTestString, p_->string_prop_.value());
}

TEST_F(ExportedPropertySetTest, SetWorksWithoutValidator) {
  p_->string_prop_.SetAccessMode(ExportedPropertyBase::Access::kReadWrite);
  auto response = SetPropertyOnInterface(
      kTestInterface3, kStringPropName, brillo::Any(kTestString));
  ASSERT_NE(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ASSERT_EQ(kTestString, p_->string_prop_.value());
}

namespace {

void VerifySignal(dbus::Signal* signal) {
  ASSERT_NE(signal, nullptr);
  std::string interface_name;
  std::string property_name;
  uint8_t value;
  dbus::MessageReader reader(signal);
  dbus::MessageReader array_reader(signal);
  dbus::MessageReader dict_reader(signal);
  ASSERT_TRUE(reader.PopString(&interface_name));
  ASSERT_TRUE(reader.PopArray(&array_reader));
  ASSERT_TRUE(array_reader.PopDictEntry(&dict_reader));
  ASSERT_TRUE(dict_reader.PopString(&property_name));
  ASSERT_TRUE(dict_reader.PopVariantOfByte(&value));
  ASSERT_FALSE(dict_reader.HasMoreData());
  ASSERT_FALSE(array_reader.HasMoreData());
  ASSERT_TRUE(reader.HasMoreData());
  // Read the (empty) list of invalidated property names.
  ASSERT_TRUE(reader.PopArray(&array_reader));
  ASSERT_FALSE(array_reader.HasMoreData());
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(value, 57);
  ASSERT_EQ(property_name, std::string(kUint8PropName));
  ASSERT_EQ(interface_name, std::string(kTestInterface1));
}

}  // namespace

TEST_F(ExportedPropertySetTest, SignalsAreParsable) {
  EXPECT_CALL(*mock_exported_object_, SendSignal(_))
      .Times(1).WillOnce(Invoke(&VerifySignal));
  p_->uint8_prop_.SetValue(57);
}

}  // namespace dbus_utils

}  // namespace brillo
