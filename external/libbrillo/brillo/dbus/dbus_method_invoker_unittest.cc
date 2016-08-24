// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_method_invoker.h>

#include <string>

#include <brillo/bind_lambda.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_object_proxy.h>
#include <dbus/scoped_dbus_error.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "brillo/dbus/test.pb.h"

using testing::AnyNumber;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::_;

using dbus::MessageReader;
using dbus::MessageWriter;
using dbus::Response;

namespace brillo {
namespace dbus_utils {

const char kTestPath[] = "/test/path";
const char kTestServiceName[] = "org.test.Object";
const char kTestInterface[] = "org.test.Object.TestInterface";
const char kTestMethod1[] = "TestMethod1";
const char kTestMethod2[] = "TestMethod2";
const char kTestMethod3[] = "TestMethod3";
const char kTestMethod4[] = "TestMethod4";

class DBusMethodInvokerTest : public testing::Test {
 public:
  void SetUp() override {
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    bus_ = new dbus::MockBus(options);
    // By default, don't worry about threading assertions.
    EXPECT_CALL(*bus_, AssertOnOriginThread()).Times(AnyNumber());
    EXPECT_CALL(*bus_, AssertOnDBusThread()).Times(AnyNumber());
    // Use a mock exported object.
    mock_object_proxy_ = new dbus::MockObjectProxy(
        bus_.get(), kTestServiceName, dbus::ObjectPath(kTestPath));
    EXPECT_CALL(*bus_,
                GetObjectProxy(kTestServiceName, dbus::ObjectPath(kTestPath)))
        .WillRepeatedly(Return(mock_object_proxy_.get()));
    int def_timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT;
    EXPECT_CALL(*mock_object_proxy_,
                MockCallMethodAndBlockWithErrorDetails(_, def_timeout_ms, _))
        .WillRepeatedly(Invoke(this, &DBusMethodInvokerTest::CreateResponse));
  }

  void TearDown() override { bus_ = nullptr; }

  Response* CreateResponse(dbus::MethodCall* method_call,
                           int /* timeout_ms */,
                           dbus::ScopedDBusError* dbus_error) {
    if (method_call->GetInterface() == kTestInterface) {
      if (method_call->GetMember() == kTestMethod1) {
        MessageReader reader(method_call);
        int v1, v2;
        // Input: two ints.
        // Output: sum of the ints converted to string.
        if (reader.PopInt32(&v1) && reader.PopInt32(&v2)) {
          auto response = Response::CreateEmpty();
          MessageWriter writer(response.get());
          writer.AppendString(std::to_string(v1 + v2));
          return response.release();
        }
      } else if (method_call->GetMember() == kTestMethod2) {
        method_call->SetSerial(123);
        dbus_set_error(dbus_error->get(), "org.MyError", "My error message");
        return nullptr;
      } else if (method_call->GetMember() == kTestMethod3) {
        MessageReader reader(method_call);
        dbus_utils_test::TestMessage msg;
        if (PopValueFromReader(&reader, &msg)) {
          auto response = Response::CreateEmpty();
          MessageWriter writer(response.get());
          AppendValueToWriter(&writer, msg);
          return response.release();
        }
      } else if (method_call->GetMember() == kTestMethod4) {
        method_call->SetSerial(123);
        MessageReader reader(method_call);
        dbus::FileDescriptor fd;
        if (reader.PopFileDescriptor(&fd)) {
          auto response = Response::CreateEmpty();
          MessageWriter writer(response.get());
          fd.CheckValidity();
          writer.AppendFileDescriptor(fd);
          return response.release();
        }
      }
    }

    LOG(ERROR) << "Unexpected method call: " << method_call->ToString();
    return nullptr;
  }

  std::string CallTestMethod(int v1, int v2) {
    std::unique_ptr<dbus::Response> response =
        brillo::dbus_utils::CallMethodAndBlock(mock_object_proxy_.get(),
                                               kTestInterface, kTestMethod1,
                                               nullptr, v1, v2);
    EXPECT_NE(nullptr, response.get());
    std::string result;
    using brillo::dbus_utils::ExtractMethodCallResults;
    EXPECT_TRUE(ExtractMethodCallResults(response.get(), nullptr, &result));
    return result;
  }

  dbus_utils_test::TestMessage CallProtobufTestMethod(
      const dbus_utils_test::TestMessage& message) {
    std::unique_ptr<dbus::Response> response =
        brillo::dbus_utils::CallMethodAndBlock(mock_object_proxy_.get(),
                                               kTestInterface, kTestMethod3,
                                               nullptr, message);
    EXPECT_NE(nullptr, response.get());
    dbus_utils_test::TestMessage result;
    using brillo::dbus_utils::ExtractMethodCallResults;
    EXPECT_TRUE(ExtractMethodCallResults(response.get(), nullptr, &result));
    return result;
  }

  // Sends a file descriptor received over D-Bus back to the caller.
  dbus::FileDescriptor EchoFD(const dbus::FileDescriptor& fd_in) {
    std::unique_ptr<dbus::Response> response =
        brillo::dbus_utils::CallMethodAndBlock(mock_object_proxy_.get(),
                                               kTestInterface, kTestMethod4,
                                               nullptr, fd_in);
    EXPECT_NE(nullptr, response.get());
    dbus::FileDescriptor fd_out;
    using brillo::dbus_utils::ExtractMethodCallResults;
    EXPECT_TRUE(ExtractMethodCallResults(response.get(), nullptr, &fd_out));
    return fd_out;
  }

  scoped_refptr<dbus::MockBus> bus_;
  scoped_refptr<dbus::MockObjectProxy> mock_object_proxy_;
};

TEST_F(DBusMethodInvokerTest, TestSuccess) {
  EXPECT_EQ("4", CallTestMethod(2, 2));
  EXPECT_EQ("10", CallTestMethod(3, 7));
  EXPECT_EQ("-4", CallTestMethod(13, -17));
}

TEST_F(DBusMethodInvokerTest, TestFailure) {
  brillo::ErrorPtr error;
  std::unique_ptr<dbus::Response> response =
      brillo::dbus_utils::CallMethodAndBlock(
          mock_object_proxy_.get(), kTestInterface, kTestMethod2, &error);
  EXPECT_EQ(nullptr, response.get());
  EXPECT_EQ(brillo::errors::dbus::kDomain, error->GetDomain());
  EXPECT_EQ("org.MyError", error->GetCode());
  EXPECT_EQ("My error message", error->GetMessage());
}

TEST_F(DBusMethodInvokerTest, TestProtobuf) {
  dbus_utils_test::TestMessage test_message;
  test_message.set_foo(123);
  test_message.set_bar("bar");

  dbus_utils_test::TestMessage resp = CallProtobufTestMethod(test_message);

  EXPECT_EQ(123, resp.foo());
  EXPECT_EQ("bar", resp.bar());
}

TEST_F(DBusMethodInvokerTest, TestFileDescriptors) {
  // Passing a file descriptor over D-Bus would effectively duplicate the fd.
  // So the resulting file descriptor value would be different but it still
  // should be valid.
  dbus::FileDescriptor fd_stdin(0);
  fd_stdin.CheckValidity();
  EXPECT_NE(fd_stdin.value(), EchoFD(fd_stdin).value());
  dbus::FileDescriptor fd_stdout(1);
  fd_stdout.CheckValidity();
  EXPECT_NE(fd_stdout.value(), EchoFD(fd_stdout).value());
  dbus::FileDescriptor fd_stderr(2);
  fd_stderr.CheckValidity();
  EXPECT_NE(fd_stderr.value(), EchoFD(fd_stderr).value());
}

//////////////////////////////////////////////////////////////////////////////
// Asynchronous method invocation support

class AsyncDBusMethodInvokerTest : public testing::Test {
 public:
  void SetUp() override {
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    bus_ = new dbus::MockBus(options);
    // By default, don't worry about threading assertions.
    EXPECT_CALL(*bus_, AssertOnOriginThread()).Times(AnyNumber());
    EXPECT_CALL(*bus_, AssertOnDBusThread()).Times(AnyNumber());
    // Use a mock exported object.
    mock_object_proxy_ = new dbus::MockObjectProxy(
        bus_.get(), kTestServiceName, dbus::ObjectPath(kTestPath));
    EXPECT_CALL(*bus_,
                GetObjectProxy(kTestServiceName, dbus::ObjectPath(kTestPath)))
        .WillRepeatedly(Return(mock_object_proxy_.get()));
    int def_timeout_ms = dbus::ObjectProxy::TIMEOUT_USE_DEFAULT;
    EXPECT_CALL(*mock_object_proxy_,
                CallMethodWithErrorCallback(_, def_timeout_ms, _, _))
        .WillRepeatedly(Invoke(this, &AsyncDBusMethodInvokerTest::HandleCall));
  }

  void TearDown() override { bus_ = nullptr; }

  void HandleCall(dbus::MethodCall* method_call,
                  int /* timeout_ms */,
                  dbus::ObjectProxy::ResponseCallback success_callback,
                  dbus::ObjectProxy::ErrorCallback error_callback) {
    if (method_call->GetInterface() == kTestInterface) {
      if (method_call->GetMember() == kTestMethod1) {
        MessageReader reader(method_call);
        int v1, v2;
        // Input: two ints.
        // Output: sum of the ints converted to string.
        if (reader.PopInt32(&v1) && reader.PopInt32(&v2)) {
          auto response = Response::CreateEmpty();
          MessageWriter writer(response.get());
          writer.AppendString(std::to_string(v1 + v2));
          success_callback.Run(response.get());
        }
        return;
      } else if (method_call->GetMember() == kTestMethod2) {
        method_call->SetSerial(123);
        auto error_response = dbus::ErrorResponse::FromMethodCall(
            method_call, "org.MyError", "My error message");
        error_callback.Run(error_response.get());
        return;
      }
    }

    LOG(FATAL) << "Unexpected method call: " << method_call->ToString();
  }

  struct SuccessCallback {
    SuccessCallback(const std::string& in_result, int* in_counter)
        : result(in_result), counter(in_counter) {}

    explicit SuccessCallback(int* in_counter) : counter(in_counter) {}

    void operator()(const std::string& actual_result) {
      (*counter)++;
      EXPECT_EQ(result, actual_result);
    }
    std::string result;
    int* counter;
  };

  struct ErrorCallback {
    ErrorCallback(const std::string& in_domain,
                  const std::string& in_code,
                  const std::string& in_message,
                  int* in_counter)
        : domain(in_domain),
          code(in_code),
          message(in_message),
          counter(in_counter) {}

    explicit ErrorCallback(int* in_counter) : counter(in_counter) {}

    void operator()(brillo::Error* error) {
      (*counter)++;
      EXPECT_NE(nullptr, error);
      EXPECT_EQ(domain, error->GetDomain());
      EXPECT_EQ(code, error->GetCode());
      EXPECT_EQ(message, error->GetMessage());
    }

    std::string domain;
    std::string code;
    std::string message;
    int* counter;
  };

  scoped_refptr<dbus::MockBus> bus_;
  scoped_refptr<dbus::MockObjectProxy> mock_object_proxy_;
};

TEST_F(AsyncDBusMethodInvokerTest, TestSuccess) {
  int error_count = 0;
  int success_count = 0;
  brillo::dbus_utils::CallMethod(
      mock_object_proxy_.get(),
      kTestInterface,
      kTestMethod1,
      base::Bind(SuccessCallback{"4", &success_count}),
      base::Bind(ErrorCallback{&error_count}),
      2, 2);
  brillo::dbus_utils::CallMethod(
      mock_object_proxy_.get(),
      kTestInterface,
      kTestMethod1,
      base::Bind(SuccessCallback{"10", &success_count}),
      base::Bind(ErrorCallback{&error_count}),
      3, 7);
  brillo::dbus_utils::CallMethod(
      mock_object_proxy_.get(),
      kTestInterface,
      kTestMethod1,
      base::Bind(SuccessCallback{"-4", &success_count}),
      base::Bind(ErrorCallback{&error_count}),
      13, -17);
  EXPECT_EQ(0, error_count);
  EXPECT_EQ(3, success_count);
}

TEST_F(AsyncDBusMethodInvokerTest, TestFailure) {
  int error_count = 0;
  int success_count = 0;
  brillo::dbus_utils::CallMethod(
      mock_object_proxy_.get(),
      kTestInterface,
      kTestMethod2,
      base::Bind(SuccessCallback{&success_count}),
      base::Bind(ErrorCallback{brillo::errors::dbus::kDomain,
                               "org.MyError",
                               "My error message",
                               &error_count}),
      2, 2);
  EXPECT_EQ(1, error_count);
  EXPECT_EQ(0, success_count);
}

}  // namespace dbus_utils
}  // namespace brillo
