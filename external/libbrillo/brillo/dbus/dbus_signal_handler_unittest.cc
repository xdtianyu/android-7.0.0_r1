// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_signal_handler.h>

#include <string>

#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_param_writer.h>
#include <dbus/mock_bus.h>
#include <dbus/mock_object_proxy.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::AnyNumber;
using testing::Return;
using testing::SaveArg;
using testing::_;

namespace brillo {
namespace dbus_utils {

const char kTestPath[] = "/test/path";
const char kTestServiceName[] = "org.test.Object";
const char kInterface[] = "org.test.Object.TestInterface";
const char kSignal[] = "TestSignal";

class DBusSignalHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    dbus::Bus::Options options;
    options.bus_type = dbus::Bus::SYSTEM;
    bus_ = new dbus::MockBus(options);
    // By default, don't worry about threading assertions.
    EXPECT_CALL(*bus_, AssertOnOriginThread()).Times(AnyNumber());
    EXPECT_CALL(*bus_, AssertOnDBusThread()).Times(AnyNumber());
    // Use a mock object proxy.
    mock_object_proxy_ = new dbus::MockObjectProxy(
        bus_.get(), kTestServiceName, dbus::ObjectPath(kTestPath));
    EXPECT_CALL(*bus_,
                GetObjectProxy(kTestServiceName, dbus::ObjectPath(kTestPath)))
        .WillRepeatedly(Return(mock_object_proxy_.get()));
  }

  void TearDown() override { bus_ = nullptr; }

 protected:
  template<typename SignalHandlerSink, typename... Args>
  void CallSignal(SignalHandlerSink* sink, Args... args) {
    dbus::ObjectProxy::SignalCallback signal_callback;
    EXPECT_CALL(*mock_object_proxy_, ConnectToSignal(kInterface, kSignal, _, _))
        .WillOnce(SaveArg<2>(&signal_callback));

    brillo::dbus_utils::ConnectToSignal(
        mock_object_proxy_.get(),
        kInterface,
        kSignal,
        base::Bind(&SignalHandlerSink::Handler, base::Unretained(sink)),
        {});

    dbus::Signal signal(kInterface, kSignal);
    dbus::MessageWriter writer(&signal);
    DBusParamWriter::Append(&writer, args...);
    signal_callback.Run(&signal);
  }

  scoped_refptr<dbus::MockBus> bus_;
  scoped_refptr<dbus::MockObjectProxy> mock_object_proxy_;
};

TEST_F(DBusSignalHandlerTest, ConnectToSignal) {
  EXPECT_CALL(*mock_object_proxy_, ConnectToSignal(kInterface, kSignal, _, _))
      .Times(1);

  brillo::dbus_utils::ConnectToSignal(
      mock_object_proxy_.get(), kInterface, kSignal, base::Closure{}, {});
}

TEST_F(DBusSignalHandlerTest, CallSignal_3Args) {
  class SignalHandlerSink {
   public:
    MOCK_METHOD3(Handler, void(int, int, double));
  } sink;

  EXPECT_CALL(sink, Handler(10, 20, 30.5)).Times(1);
  CallSignal(&sink, 10, 20, 30.5);
}

TEST_F(DBusSignalHandlerTest, CallSignal_2Args) {
  class SignalHandlerSink {
   public:
    // Take string both by reference and by value to make sure this works too.
    MOCK_METHOD2(Handler, void(const std::string&, std::string));
  } sink;

  EXPECT_CALL(sink, Handler(std::string{"foo"}, std::string{"bar"})).Times(1);
  CallSignal(&sink, std::string{"foo"}, std::string{"bar"});
}

TEST_F(DBusSignalHandlerTest, CallSignal_NoArgs) {
  class SignalHandlerSink {
   public:
    MOCK_METHOD0(Handler, void());
  } sink;

  EXPECT_CALL(sink, Handler()).Times(1);
  CallSignal(&sink);
}

TEST_F(DBusSignalHandlerTest, CallSignal_Error_TooManyArgs) {
  class SignalHandlerSink {
   public:
    MOCK_METHOD0(Handler, void());
  } sink;

  // Handler() expects no args, but we send an int.
  EXPECT_CALL(sink, Handler()).Times(0);
  CallSignal(&sink, 5);
}

TEST_F(DBusSignalHandlerTest, CallSignal_Error_TooFewArgs) {
  class SignalHandlerSink {
   public:
    MOCK_METHOD2(Handler, void(std::string, bool));
  } sink;

  // Handler() expects 2 args while we send it just one.
  EXPECT_CALL(sink, Handler(_, _)).Times(0);
  CallSignal(&sink, std::string{"bar"});
}

TEST_F(DBusSignalHandlerTest, CallSignal_Error_TypeMismatchArgs) {
  class SignalHandlerSink {
   public:
    MOCK_METHOD2(Handler, void(std::string, bool));
  } sink;

  // Handler() expects "sb" while we send it "ii".
  EXPECT_CALL(sink, Handler(_, _)).Times(0);
  CallSignal(&sink, 1, 2);
}

}  // namespace dbus_utils
}  // namespace brillo
