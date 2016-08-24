// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_object.h>

#include <memory>

#include <base/bind.h>
#include <brillo/dbus/dbus_object_test_helpers.h>
#include <brillo/dbus/mock_exported_object_manager.h>
#include <dbus/message.h>
#include <dbus/property.h>
#include <dbus/object_path.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_exported_object.h>

using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::_;

namespace brillo {
namespace dbus_utils {

namespace {

const char kMethodsExportedOn[] = "/export";

const char kTestInterface1[] = "org.chromium.Test.MathInterface";
const char kTestMethod_Add[] = "Add";
const char kTestMethod_Negate[] = "Negate";
const char kTestMethod_Positive[] = "Positive";
const char kTestMethod_AddSubtract[] = "AddSubtract";

const char kTestInterface2[] = "org.chromium.Test.StringInterface";
const char kTestMethod_StrLen[] = "StrLen";
const char kTestMethod_CheckNonEmpty[] = "CheckNonEmpty";

const char kTestInterface3[] = "org.chromium.Test.NoOpInterface";
const char kTestMethod_NoOp[] = "NoOp";
const char kTestMethod_WithMessage[] = "TestWithMessage";
const char kTestMethod_WithMessageAsync[] = "TestWithMessageAsync";

struct Calc {
  int Add(int x, int y) { return x + y; }
  int Negate(int x) { return -x; }
  void Positive(std::unique_ptr<DBusMethodResponse<double>> response,
                double x) {
    if (x >= 0.0) {
      response->Return(x);
      return;
    }
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "test", "not_positive",
                 "Negative value passed in");
    response->ReplyWithError(error.get());
  }
  void AddSubtract(int x, int y, int* sum, int* diff) {
    *sum = x + y;
    *diff = x - y;
  }
};

int StrLen(const std::string& str) {
  return str.size();
}

bool CheckNonEmpty(ErrorPtr* error, const std::string& str) {
  if (!str.empty())
    return true;
  Error::AddTo(error, FROM_HERE, "test", "string_empty", "String is empty");
  return false;
}

void NoOp() {}

bool TestWithMessage(ErrorPtr* /* error */,
                     dbus::Message* message,
                     std::string* str) {
  *str = message->GetSender();
  return true;
}

void TestWithMessageAsync(
    std::unique_ptr<DBusMethodResponse<std::string>> response,
    dbus::Message* message) {
  response->Return(message->GetSender());
}

}  // namespace

class DBusObjectTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    bus_ = new dbus::MockBus(options);
    // By default, don't worry about threading assertions.
    EXPECT_CALL(*bus_, AssertOnOriginThread()).Times(AnyNumber());
    EXPECT_CALL(*bus_, AssertOnDBusThread()).Times(AnyNumber());
    // Use a mock exported object.
    const dbus::ObjectPath kMethodsExportedOnPath{
        std::string{kMethodsExportedOn}};
    mock_exported_object_ =
        new dbus::MockExportedObject(bus_.get(), kMethodsExportedOnPath);
    EXPECT_CALL(*bus_, GetExportedObject(kMethodsExportedOnPath))
        .Times(AnyNumber())
        .WillRepeatedly(Return(mock_exported_object_.get()));
    EXPECT_CALL(*mock_exported_object_, ExportMethod(_, _, _, _))
        .Times(AnyNumber());
    EXPECT_CALL(*mock_exported_object_, Unregister()).Times(1);

    dbus_object_ = std::unique_ptr<DBusObject>(
        new DBusObject(nullptr, bus_, kMethodsExportedOnPath));

    DBusInterface* itf1 = dbus_object_->AddOrGetInterface(kTestInterface1);
    itf1->AddSimpleMethodHandler(
        kTestMethod_Add, base::Unretained(&calc_), &Calc::Add);
    itf1->AddSimpleMethodHandler(
        kTestMethod_Negate, base::Unretained(&calc_), &Calc::Negate);
    itf1->AddMethodHandler(
        kTestMethod_Positive, base::Unretained(&calc_), &Calc::Positive);
    itf1->AddSimpleMethodHandler(
        kTestMethod_AddSubtract, base::Unretained(&calc_), &Calc::AddSubtract);
    DBusInterface* itf2 = dbus_object_->AddOrGetInterface(kTestInterface2);
    itf2->AddSimpleMethodHandler(kTestMethod_StrLen, StrLen);
    itf2->AddSimpleMethodHandlerWithError(kTestMethod_CheckNonEmpty,
                                          CheckNonEmpty);
    DBusInterface* itf3 = dbus_object_->AddOrGetInterface(kTestInterface3);
    base::Callback<void()> noop_callback = base::Bind(NoOp);
    itf3->AddSimpleMethodHandler(kTestMethod_NoOp, noop_callback);
    itf3->AddSimpleMethodHandlerWithErrorAndMessage(
        kTestMethod_WithMessage, base::Bind(&TestWithMessage));
    itf3->AddMethodHandlerWithMessage(kTestMethod_WithMessageAsync,
                                      base::Bind(&TestWithMessageAsync));

    dbus_object_->RegisterAsync(
        AsyncEventSequencer::GetDefaultCompletionAction());
  }

  void ExpectError(dbus::Response* response, const std::string& expected_code) {
    EXPECT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
    EXPECT_EQ(expected_code, response->GetErrorName());
  }

  scoped_refptr<dbus::MockBus> bus_;
  scoped_refptr<dbus::MockExportedObject> mock_exported_object_;
  std::unique_ptr<DBusObject> dbus_object_;
  Calc calc_;
};

TEST_F(DBusObjectTest, Add) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(2);
  writer.AppendInt32(3);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int result;
  ASSERT_TRUE(reader.PopInt32(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(5, result);
}

TEST_F(DBusObjectTest, Negate) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Negate);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(98765);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int result;
  ASSERT_TRUE(reader.PopInt32(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(-98765, result);
}

TEST_F(DBusObjectTest, PositiveSuccess) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Positive);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendDouble(17.5);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  double result;
  ASSERT_TRUE(reader.PopDouble(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_DOUBLE_EQ(17.5, result);
}

TEST_F(DBusObjectTest, PositiveFailure) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Positive);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendDouble(-23.2);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ExpectError(response.get(), DBUS_ERROR_FAILED);
}

TEST_F(DBusObjectTest, AddSubtract) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_AddSubtract);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(2);
  writer.AppendInt32(3);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int sum = 0, diff = 0;
  ASSERT_TRUE(reader.PopInt32(&sum));
  ASSERT_TRUE(reader.PopInt32(&diff));
  ASSERT_FALSE(reader.HasMoreData());
  EXPECT_EQ(5, sum);
  EXPECT_EQ(-1, diff);
}

TEST_F(DBusObjectTest, StrLen0) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_StrLen);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("");
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int result;
  ASSERT_TRUE(reader.PopInt32(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(0, result);
}

TEST_F(DBusObjectTest, StrLen4) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_StrLen);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("test");
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int result;
  ASSERT_TRUE(reader.PopInt32(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(4, result);
}

TEST_F(DBusObjectTest, CheckNonEmpty_Success) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_CheckNonEmpty);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("test");
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ASSERT_EQ(dbus::Message::MESSAGE_METHOD_RETURN, response->GetMessageType());
  dbus::MessageReader reader(response.get());
  EXPECT_FALSE(reader.HasMoreData());
}

TEST_F(DBusObjectTest, CheckNonEmpty_Failure) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_CheckNonEmpty);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendString("");
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  ErrorPtr error;
  ExtractMethodCallResults(response.get(), &error);
  ASSERT_NE(nullptr, error.get());
  EXPECT_EQ("test", error->GetDomain());
  EXPECT_EQ("string_empty", error->GetCode());
  EXPECT_EQ("String is empty", error->GetMessage());
}

TEST_F(DBusObjectTest, CheckNonEmpty_MissingParams) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_CheckNonEmpty);
  method_call.SetSerial(123);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ASSERT_EQ(dbus::Message::MESSAGE_ERROR, response->GetMessageType());
  dbus::MessageReader reader(response.get());
  std::string message;
  ASSERT_TRUE(reader.PopString(&message));
  EXPECT_EQ(DBUS_ERROR_INVALID_ARGS, response->GetErrorName());
  EXPECT_EQ("Too few parameters in a method call", message);
  EXPECT_FALSE(reader.HasMoreData());
}

TEST_F(DBusObjectTest, NoOp) {
  dbus::MethodCall method_call(kTestInterface3, kTestMethod_NoOp);
  method_call.SetSerial(123);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  ASSERT_FALSE(reader.HasMoreData());
}

TEST_F(DBusObjectTest, TestWithMessage) {
  const std::string sender{":1.2345"};
  dbus::MethodCall method_call(kTestInterface3, kTestMethod_WithMessage);
  method_call.SetSerial(123);
  method_call.SetSender(sender);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  std::string message;
  ASSERT_TRUE(reader.PopString(&message));
  ASSERT_FALSE(reader.HasMoreData());
  EXPECT_EQ(sender, message);
}

TEST_F(DBusObjectTest, TestWithMessageAsync) {
  const std::string sender{":6.7890"};
  dbus::MethodCall method_call(kTestInterface3, kTestMethod_WithMessageAsync);
  method_call.SetSerial(123);
  method_call.SetSender(sender);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  std::string message;
  ASSERT_TRUE(reader.PopString(&message));
  ASSERT_FALSE(reader.HasMoreData());
  EXPECT_EQ(sender, message);
}

TEST_F(DBusObjectTest, TooFewParams) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(2);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ExpectError(response.get(), DBUS_ERROR_INVALID_ARGS);
}

TEST_F(DBusObjectTest, TooManyParams) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(1);
  writer.AppendInt32(2);
  writer.AppendInt32(3);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ExpectError(response.get(), DBUS_ERROR_INVALID_ARGS);
}

TEST_F(DBusObjectTest, ParamTypeMismatch) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(1);
  writer.AppendBool(false);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ExpectError(response.get(), DBUS_ERROR_INVALID_ARGS);
}

TEST_F(DBusObjectTest, ParamAsVariant) {
  dbus::MethodCall method_call(kTestInterface1, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendVariantOfInt32(10);
  writer.AppendVariantOfInt32(3);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  dbus::MessageReader reader(response.get());
  int result;
  ASSERT_TRUE(reader.PopInt32(&result));
  ASSERT_FALSE(reader.HasMoreData());
  ASSERT_EQ(13, result);
}

TEST_F(DBusObjectTest, UnknownMethod) {
  dbus::MethodCall method_call(kTestInterface2, kTestMethod_Add);
  method_call.SetSerial(123);
  dbus::MessageWriter writer(&method_call);
  writer.AppendInt32(1);
  writer.AppendBool(false);
  auto response = testing::CallMethod(*dbus_object_, &method_call);
  ExpectError(response.get(), DBUS_ERROR_UNKNOWN_METHOD);
}

TEST_F(DBusObjectTest, ShouldReleaseOnlyClaimedInterfaces) {
  const dbus::ObjectPath kObjectManagerPath{std::string{"/"}};
  const dbus::ObjectPath kMethodsExportedOnPath{
      std::string{kMethodsExportedOn}};
  MockExportedObjectManager mock_object_manager{bus_, kObjectManagerPath};
  dbus_object_ = std::unique_ptr<DBusObject>(
      new DBusObject(&mock_object_manager, bus_, kMethodsExportedOnPath));
  EXPECT_CALL(mock_object_manager, ClaimInterface(_, _, _)).Times(0);
  EXPECT_CALL(mock_object_manager, ReleaseInterface(_, _)).Times(0);
  DBusInterface* itf1 = dbus_object_->AddOrGetInterface(kTestInterface1);
  itf1->AddSimpleMethodHandler(
      kTestMethod_Add, base::Unretained(&calc_), &Calc::Add);
  // When we tear down our DBusObject, it should release only interfaces it has
  // previously claimed.  This prevents a check failing inside the
  // ExportedObjectManager.  Since no interfaces have finished exporting
  // handlers, nothing should be released.
  dbus_object_.reset();
}

}  // namespace dbus_utils
}  // namespace brillo
