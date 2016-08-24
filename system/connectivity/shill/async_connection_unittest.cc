//
// Copyright (C) 2011 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/async_connection.h"

#include <netinet/in.h>

#include <vector>

#include <base/bind.h>
#include <gtest/gtest.h>

#include "shill/mock_event_dispatcher.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNew;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::Test;

namespace shill {

namespace {
const char kInterfaceName[] = "int0";
const char kIPv4Address[] = "10.11.12.13";
const char kIPv6Address[] = "2001:db8::1";
const int kConnectPort = 10203;
const int kErrorNumber = 30405;
const int kSocketFD = 60708;
}  // namespace

class AsyncConnectionTest : public Test {
 public:
  AsyncConnectionTest()
      : async_connection_(
            new AsyncConnection(kInterfaceName, &dispatcher_, &sockets_,
                                callback_target_.callback())),
        ipv4_address_(IPAddress::kFamilyIPv4),
        ipv6_address_(IPAddress::kFamilyIPv6) { }

  virtual void SetUp() {
    EXPECT_TRUE(ipv4_address_.SetAddressFromString(kIPv4Address));
    EXPECT_TRUE(ipv6_address_.SetAddressFromString(kIPv6Address));
  }
  virtual void TearDown() {
    if (async_connection_.get() && async_connection_->fd_ >= 0) {
      EXPECT_CALL(sockets(), Close(kSocketFD))
          .WillOnce(Return(0));
    }
  }
  void InvokeFreeConnection(bool /*success*/, int /*fd*/) {
    async_connection_.reset();
  }

 protected:
  class ConnectCallbackTarget {
   public:
    ConnectCallbackTarget()
        : callback_(Bind(&ConnectCallbackTarget::CallTarget,
                         Unretained(this))) {}

    MOCK_METHOD2(CallTarget, void(bool success, int fd));
    const Callback<void(bool, int)>& callback() { return callback_; }

   private:
    Callback<void(bool, int)> callback_;
  };

  void ExpectReset() {
    EXPECT_STREQ(kInterfaceName, async_connection_->interface_name_.c_str());
    EXPECT_EQ(&dispatcher_, async_connection_->dispatcher_);
    EXPECT_EQ(&sockets_, async_connection_->sockets_);
    EXPECT_TRUE(callback_target_.callback().
                Equals(async_connection_->callback_));
    EXPECT_EQ(-1, async_connection_->fd_);
    EXPECT_FALSE(async_connection_->connect_completion_callback_.is_null());
    EXPECT_FALSE(async_connection_->connect_completion_handler_.get());
  }

  void StartConnection() {
    EXPECT_CALL(sockets_, Socket(_, _, _))
        .WillOnce(Return(kSocketFD));
    EXPECT_CALL(sockets_, SetNonBlocking(kSocketFD))
        .WillOnce(Return(0));
    EXPECT_CALL(sockets_, BindToDevice(kSocketFD, StrEq(kInterfaceName)))
        .WillOnce(Return(0));
    EXPECT_CALL(sockets(), Connect(kSocketFD, _, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(sockets_, Error())
        .WillOnce(Return(EINPROGRESS));
    EXPECT_CALL(dispatcher(),
                CreateReadyHandler(kSocketFD, IOHandler::kModeOutput, _))
        .WillOnce(ReturnNew<IOHandler>());
    EXPECT_TRUE(async_connection().Start(ipv4_address_, kConnectPort));
  }

  void OnConnectCompletion(int fd) {
    async_connection_->OnConnectCompletion(fd);
  }
  AsyncConnection& async_connection() { return *async_connection_.get(); }
  StrictMock<MockSockets>& sockets() { return sockets_; }
  MockEventDispatcher& dispatcher() { return dispatcher_; }
  const IPAddress& ipv4_address() { return ipv4_address_; }
  const IPAddress& ipv6_address() { return ipv6_address_; }
  int fd() { return async_connection_->fd_; }
  void set_fd(int fd) { async_connection_->fd_ = fd; }
  StrictMock<ConnectCallbackTarget>& callback_target() {
    return callback_target_;
  }

 private:
  MockEventDispatcher dispatcher_;
  StrictMock<MockSockets> sockets_;
  StrictMock<ConnectCallbackTarget> callback_target_;
  std::unique_ptr<AsyncConnection> async_connection_;
  IPAddress ipv4_address_;
  IPAddress ipv6_address_;
};

TEST_F(AsyncConnectionTest, InitState) {
  ExpectReset();
  EXPECT_EQ(string(), async_connection().error());
}

TEST_F(AsyncConnectionTest, StartSocketFailure) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(kErrorNumber));
  EXPECT_FALSE(async_connection().Start(ipv4_address(), kConnectPort));
  ExpectReset();
  EXPECT_STREQ(strerror(kErrorNumber), async_connection().error().c_str());
}

TEST_F(AsyncConnectionTest, StartNonBlockingFailure) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(kErrorNumber));
  EXPECT_CALL(sockets(), Close(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(async_connection().Start(ipv4_address(), kConnectPort));
  ExpectReset();
  EXPECT_STREQ(strerror(kErrorNumber), async_connection().error().c_str());
}

TEST_F(AsyncConnectionTest, StartBindToDeviceFailure) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(kErrorNumber));
  EXPECT_CALL(sockets(), Close(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(async_connection().Start(ipv4_address(), kConnectPort));
  ExpectReset();
  EXPECT_STREQ(strerror(kErrorNumber), async_connection().error().c_str());
}

TEST_F(AsyncConnectionTest, SynchronousFailure) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Connect(kSocketFD, _, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Error())
      .Times(2)
      .WillRepeatedly(Return(0));
  EXPECT_CALL(sockets(), Close(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(async_connection().Start(ipv4_address(), kConnectPort));
  ExpectReset();
}

MATCHER_P2(IsSocketAddress, address, port, "") {
  const struct sockaddr_in* arg_saddr =
      reinterpret_cast<const struct sockaddr_in*>(arg);
  IPAddress arg_addr(IPAddress::kFamilyIPv4,
                     ByteString(reinterpret_cast<const unsigned char*>(
                         &arg_saddr->sin_addr.s_addr),
                                sizeof(arg_saddr->sin_addr.s_addr)));
  return address.Equals(arg_addr) && arg_saddr->sin_port == htons(port);
}

MATCHER_P2(IsSocketIpv6Address, ipv6_address, port, "") {
  const struct sockaddr_in6* arg_saddr =
      reinterpret_cast<const struct sockaddr_in6*>(arg);
  IPAddress arg_addr(IPAddress::kFamilyIPv6,
                     ByteString(reinterpret_cast<const unsigned char*>(
                         &arg_saddr->sin6_addr.s6_addr),
                                sizeof(arg_saddr->sin6_addr.s6_addr)));
  return ipv6_address.Equals(arg_addr) && arg_saddr->sin6_port == htons(port);
}

TEST_F(AsyncConnectionTest, SynchronousStart) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Connect(kSocketFD,
                                  IsSocketAddress(ipv4_address(), kConnectPort),
                                  sizeof(struct sockaddr_in)))
      .WillOnce(Return(-1));
  EXPECT_CALL(dispatcher(),
              CreateReadyHandler(kSocketFD, IOHandler::kModeOutput, _))
        .WillOnce(ReturnNew<IOHandler>());
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(EINPROGRESS));
  EXPECT_TRUE(async_connection().Start(ipv4_address(), kConnectPort));
  EXPECT_EQ(kSocketFD, fd());
}

TEST_F(AsyncConnectionTest, SynchronousStartIpv6) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Connect(kSocketFD,
                                  IsSocketIpv6Address(ipv6_address(),
                                                      kConnectPort),
                                  sizeof(struct sockaddr_in6)))
      .WillOnce(Return(-1));
  EXPECT_CALL(dispatcher(),
              CreateReadyHandler(kSocketFD, IOHandler::kModeOutput, _))
        .WillOnce(ReturnNew<IOHandler>());
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(EINPROGRESS));
  EXPECT_TRUE(async_connection().Start(ipv6_address(), kConnectPort));
  EXPECT_EQ(kSocketFD, fd());
}

TEST_F(AsyncConnectionTest, AsynchronousFailure) {
  StartConnection();
  EXPECT_CALL(sockets(), GetSocketError(kSocketFD))
      .WillOnce(Return(1));
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(kErrorNumber));
  EXPECT_CALL(callback_target(), CallTarget(false, -1));
  EXPECT_CALL(sockets(), Close(kSocketFD))
      .WillOnce(Return(0));
  OnConnectCompletion(kSocketFD);
  ExpectReset();
  EXPECT_STREQ(strerror(kErrorNumber), async_connection().error().c_str());
}

TEST_F(AsyncConnectionTest, AsynchronousSuccess) {
  StartConnection();
  EXPECT_CALL(sockets(), GetSocketError(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(callback_target(), CallTarget(true, kSocketFD));
  OnConnectCompletion(kSocketFD);
  ExpectReset();
}

TEST_F(AsyncConnectionTest, SynchronousSuccess) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Connect(kSocketFD,
                                  IsSocketAddress(ipv4_address(), kConnectPort),
                                  sizeof(struct sockaddr_in)))
      .WillOnce(Return(0));
  EXPECT_CALL(callback_target(), CallTarget(true, kSocketFD));
  EXPECT_TRUE(async_connection().Start(ipv4_address(), kConnectPort));
  ExpectReset();
}

TEST_F(AsyncConnectionTest, SynchronousSuccessIpv6) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kSocketFD));
  EXPECT_CALL(sockets(), SetNonBlocking(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), BindToDevice(kSocketFD, StrEq(kInterfaceName)))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Connect(kSocketFD,
                                  IsSocketIpv6Address(ipv6_address(),
                                                      kConnectPort),
                                  sizeof(struct sockaddr_in6)))
      .WillOnce(Return(0));
  EXPECT_CALL(callback_target(), CallTarget(true, kSocketFD));
  EXPECT_TRUE(async_connection().Start(ipv6_address(), kConnectPort));
  ExpectReset();
}

TEST_F(AsyncConnectionTest, FreeOnSuccessCallback) {
  StartConnection();
  EXPECT_CALL(sockets(), GetSocketError(kSocketFD))
      .WillOnce(Return(0));
  EXPECT_CALL(callback_target(), CallTarget(true, kSocketFD))
      .WillOnce(Invoke(this, &AsyncConnectionTest::InvokeFreeConnection));
  OnConnectCompletion(kSocketFD);
}

TEST_F(AsyncConnectionTest, FreeOnFailureCallback) {
  StartConnection();
  EXPECT_CALL(sockets(), GetSocketError(kSocketFD))
      .WillOnce(Return(1));
  EXPECT_CALL(callback_target(), CallTarget(false, -1))
      .WillOnce(Invoke(this, &AsyncConnectionTest::InvokeFreeConnection));
  EXPECT_CALL(sockets(), Error())
      .WillOnce(Return(kErrorNumber));
  EXPECT_CALL(sockets(), Close(kSocketFD))
      .WillOnce(Return(0));
  OnConnectCompletion(kSocketFD);
}

}  // namespace shill
