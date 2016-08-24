//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/dns_client.h"

#include <netdb.h>

#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/mock_ares.h"
#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/net/io_handler.h"
#include "shill/net/mock_time.h"
#include "shill/testing.h"

using base::Bind;
using base::Unretained;
using std::string;
using std::vector;
using testing::_;
using testing::DoAll;
using testing::Not;
using testing::Return;
using testing::ReturnArg;
using testing::ReturnNew;
using testing::Test;
using testing::SetArgumentPointee;
using testing::StrEq;
using testing::StrictMock;

namespace shill {

namespace {
const char kGoodName[] = "all-systems.mcast.net";
const char kResult[] = "224.0.0.1";
const char kGoodServer[] = "8.8.8.8";
const char kBadServer[] = "10.9xx8.7";
const char kNetworkInterface[] = "eth0";
char kReturnAddressList0[] = { static_cast<char>(224), 0, 0, 1 };
char* kReturnAddressList[] = { kReturnAddressList0, nullptr };
char kFakeAresChannelData = 0;
const ares_channel kAresChannel =
    reinterpret_cast<ares_channel>(&kFakeAresChannelData);
const int kAresFd = 10203;
const int kAresTimeoutMS = 2000;  // ARES transaction timeout
const int kAresWaitMS = 1000;     // Time period ARES asks caller to wait
}  // namespace

class DNSClientTest : public Test {
 public:
  DNSClientTest()
      : ares_result_(ARES_SUCCESS), address_result_(IPAddress::kFamilyUnknown) {
    time_val_.tv_sec = 0;
    time_val_.tv_usec = 0;
    ares_timeout_.tv_sec = kAresWaitMS / 1000;
    ares_timeout_.tv_usec = (kAresWaitMS % 1000) * 1000;
    hostent_.h_addrtype = IPAddress::kFamilyIPv4;
    hostent_.h_length = sizeof(kReturnAddressList0);
    hostent_.h_addr_list = kReturnAddressList;
  }

  virtual void SetUp() {
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(DoAll(SetArgumentPointee<0>(time_val_), Return(0)));
    SetInActive();
  }

  virtual void TearDown() {
    // We need to make sure the dns_client instance releases ares_
    // before the destructor for DNSClientTest deletes ares_.
    if (dns_client_.get()) {
      dns_client_->Stop();
    }
  }

  void AdvanceTime(int time_ms) {
    struct timeval adv_time = { time_ms/1000, (time_ms % 1000) * 1000 };
    timeradd(&time_val_, &adv_time, &time_val_);
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(DoAll(SetArgumentPointee<0>(time_val_), Return(0)));
  }

  void CallReplyCB() {
    dns_client_->ReceiveDNSReplyCB(dns_client_.get(), ares_result_, 0,
                                   &hostent_);
  }

  void CallDNSRead() {
    dns_client_->HandleDNSRead(kAresFd);
  }

  void CallDNSWrite() {
    dns_client_->HandleDNSWrite(kAresFd);
  }

  void CallTimeout() {
    dns_client_->HandleTimeout();
  }

  void CallCompletion() {
    dns_client_->HandleCompletion();
  }

  void CreateClient(const vector<string>& dns_servers, int timeout_ms) {
    dns_client_.reset(new DNSClient(IPAddress::kFamilyIPv4,
                                    kNetworkInterface,
                                    dns_servers,
                                    timeout_ms,
                                    &dispatcher_,
                                    callback_target_.callback()));
    dns_client_->ares_ = &ares_;
    dns_client_->time_ = &time_;
  }

  void SetActive() {
    // Returns that socket kAresFd is readable.
    EXPECT_CALL(ares_, GetSock(_, _, _))
        .WillRepeatedly(DoAll(SetArgumentPointee<1>(kAresFd), Return(1)));
    EXPECT_CALL(ares_, Timeout(_, _, _))
        .WillRepeatedly(
            DoAll(SetArgumentPointee<2>(ares_timeout_), ReturnArg<2>()));
  }

  void SetInActive() {
    EXPECT_CALL(ares_, GetSock(_, _, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(ares_, Timeout(_, _, _))
        .WillRepeatedly(ReturnArg<1>());
  }

  void SetupRequest(const string& name, const string& server) {
    vector<string> dns_servers;
    dns_servers.push_back(server);
    CreateClient(dns_servers, kAresTimeoutMS);
    // These expectations are fulfilled when dns_client_->Start() is called.
    EXPECT_CALL(ares_, InitOptions(_, _, _))
        .WillOnce(DoAll(SetArgumentPointee<0>(kAresChannel),
                        Return(ARES_SUCCESS)));
    EXPECT_CALL(ares_, SetServersCsv(_, _))
        .WillOnce(Return(ARES_SUCCESS));
    EXPECT_CALL(ares_, SetLocalDev(kAresChannel, StrEq(kNetworkInterface)))
        .Times(1);
    EXPECT_CALL(ares_, GetHostByName(kAresChannel, StrEq(name), _, _, _));
  }

  void StartValidRequest() {
    SetupRequest(kGoodName, kGoodServer);
    EXPECT_CALL(dispatcher_,
                CreateReadyHandler(kAresFd, IOHandler::kModeInput, _))
        .WillOnce(ReturnNew<IOHandler>());
    SetActive();
    EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
    Error error;
    ASSERT_TRUE(dns_client_->Start(kGoodName, &error));
    EXPECT_TRUE(error.IsSuccess());
    EXPECT_CALL(ares_, Destroy(kAresChannel));
  }

  void TestValidCompletion() {
    EXPECT_CALL(ares_, ProcessFd(kAresChannel, kAresFd, ARES_SOCKET_BAD))
        .WillOnce(InvokeWithoutArgs(this, &DNSClientTest::CallReplyCB));
    ExpectPostCompletionTask();
    CallDNSRead();

    // Make sure that the address value is correct as held in the DNSClient.
    ASSERT_TRUE(dns_client_->address_.IsValid());
    IPAddress ipaddr(dns_client_->address_.family());
    ASSERT_TRUE(ipaddr.SetAddressFromString(kResult));
    EXPECT_TRUE(ipaddr.Equals(dns_client_->address_));

    // Make sure the callback gets called with a success result, and save
    // the callback address argument in |address_result_|.
    EXPECT_CALL(callback_target_, CallTarget(IsSuccess(), _))
        .WillOnce(Invoke(this, &DNSClientTest::SaveCallbackArgs));
    CallCompletion();

    // Make sure the address was successfully passed to the callback.
    EXPECT_TRUE(ipaddr.Equals(address_result_));
    EXPECT_TRUE(dns_client_->address_.IsDefault());
  }

  void SaveCallbackArgs(const Error& error, const IPAddress& address)  {
    error_result_.CopyFrom(error);
    address_result_ = address;
  }

  void ExpectPostCompletionTask() {
    EXPECT_CALL(dispatcher_, PostTask(_));
  }

  void ExpectReset() {
    EXPECT_TRUE(dns_client_->address_.family() == IPAddress::kFamilyIPv4);
    EXPECT_TRUE(dns_client_->address_.IsDefault());
    EXPECT_FALSE(dns_client_->resolver_state_.get());
  }

 protected:
  class DNSCallbackTarget {
   public:
    DNSCallbackTarget()
        : callback_(Bind(&DNSCallbackTarget::CallTarget, Unretained(this))) {}

    MOCK_METHOD2(CallTarget, void(const Error& error,
                                  const IPAddress& address));
    const DNSClient::ClientCallback& callback() { return callback_; }

   private:
    DNSClient::ClientCallback callback_;
  };

  std::unique_ptr<DNSClient> dns_client_;
  StrictMock<MockEventDispatcher> dispatcher_;
  string queued_request_;
  StrictMock<DNSCallbackTarget> callback_target_;
  StrictMock<MockAres> ares_;
  StrictMock<MockTime> time_;
  struct timeval time_val_;
  struct timeval ares_timeout_;
  struct hostent hostent_;
  int ares_result_;
  Error error_result_;
  IPAddress address_result_;
};

class SentinelIOHandler : public IOHandler {
 public:
  MOCK_METHOD0(Die, void());
  virtual ~SentinelIOHandler() { Die(); }
};

TEST_F(DNSClientTest, Constructor) {
  vector<string> dns_servers;
  dns_servers.push_back(kGoodServer);
  CreateClient(dns_servers, kAresTimeoutMS);
  ExpectReset();
}

// Receive error because no DNS servers were specified.
TEST_F(DNSClientTest, NoServers) {
  CreateClient(vector<string>(), kAresTimeoutMS);
  Error error;
  EXPECT_FALSE(dns_client_->Start(kGoodName, &error));
  EXPECT_EQ(Error::kInvalidArguments, error.type());
}

// Setup error because SetServersCsv failed due to invalid DNS servers.
TEST_F(DNSClientTest, SetServersCsvInvalidServer) {
  vector<string> dns_servers;
  dns_servers.push_back(kBadServer);
  CreateClient(dns_servers, kAresTimeoutMS);
  EXPECT_CALL(ares_, InitOptions(_, _, _))
      .WillOnce(Return(ARES_SUCCESS));
  EXPECT_CALL(ares_, SetServersCsv(_, _))
      .WillOnce(Return(ARES_EBADSTR));
  Error error;
  EXPECT_FALSE(dns_client_->Start(kGoodName, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
}

// Setup error because InitOptions failed.
TEST_F(DNSClientTest, InitOptionsFailure) {
  vector<string> dns_servers;
  dns_servers.push_back(kGoodServer);
  CreateClient(dns_servers, kAresTimeoutMS);
  EXPECT_CALL(ares_, InitOptions(_, _, _))
      .WillOnce(Return(ARES_EBADFLAGS));
  Error error;
  EXPECT_FALSE(dns_client_->Start(kGoodName, &error));
  EXPECT_EQ(Error::kOperationFailed, error.type());
}

// Fail a second request because one is already in progress.
TEST_F(DNSClientTest, MultipleRequest) {
  StartValidRequest();
  Error error;
  ASSERT_FALSE(dns_client_->Start(kGoodName, &error));
  EXPECT_EQ(Error::kInProgress, error.type());
}

TEST_F(DNSClientTest, GoodRequest) {
  StartValidRequest();
  TestValidCompletion();
}

TEST_F(DNSClientTest, GoodRequestWithTimeout) {
  StartValidRequest();
  // Insert an intermediate HandleTimeout callback.
  AdvanceTime(kAresWaitMS);
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  CallTimeout();
  AdvanceTime(kAresWaitMS);
  TestValidCompletion();
}

TEST_F(DNSClientTest, GoodRequestWithDNSRead) {
  StartValidRequest();
  // Insert an intermediate HandleDNSRead callback.
  AdvanceTime(kAresWaitMS);
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, kAresFd, ARES_SOCKET_BAD));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  CallDNSRead();
  AdvanceTime(kAresWaitMS);
  TestValidCompletion();
}

TEST_F(DNSClientTest, GoodRequestWithDNSWrite) {
  StartValidRequest();
  // Insert an intermediate HandleDNSWrite callback.
  AdvanceTime(kAresWaitMS);
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, ARES_SOCKET_BAD, kAresFd));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  CallDNSWrite();
  AdvanceTime(kAresWaitMS);
  TestValidCompletion();
}

// Failure due to the timeout occurring during first call to RefreshHandles.
TEST_F(DNSClientTest, TimeoutFirstRefresh) {
  SetupRequest(kGoodName, kGoodServer);
  struct timeval init_time_val = time_val_;
  AdvanceTime(kAresTimeoutMS);
  EXPECT_CALL(time_, GetTimeMonotonic(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(init_time_val), Return(0)))
      .WillRepeatedly(DoAll(SetArgumentPointee<0>(time_val_), Return(0)));
  EXPECT_CALL(callback_target_, CallTarget(Not(IsSuccess()), _))
      .Times(0);
  EXPECT_CALL(ares_, Destroy(kAresChannel));
  Error error;
  // Expect the DNSClient to post a completion task.  However this task will
  // never run since the Stop() gets called before returning.  We confirm
  // that the task indeed gets canceled below in ExpectReset().
  ExpectPostCompletionTask();
  ASSERT_FALSE(dns_client_->Start(kGoodName, &error));
  EXPECT_EQ(Error::kOperationTimeout, error.type());
  EXPECT_EQ(string(DNSClient::kErrorTimedOut), error.message());
  ExpectReset();
}

// Failed request due to timeout within the dns_client.
TEST_F(DNSClientTest, TimeoutDispatcherEvent) {
  StartValidRequest();
  EXPECT_CALL(ares_, ProcessFd(kAresChannel,
                               ARES_SOCKET_BAD, ARES_SOCKET_BAD));
  AdvanceTime(kAresTimeoutMS);
  ExpectPostCompletionTask();
  CallTimeout();
  EXPECT_CALL(callback_target_, CallTarget(
      ErrorIs(Error::kOperationTimeout, DNSClient::kErrorTimedOut), _));
  CallCompletion();
}

// Failed request due to timeout reported by ARES.
TEST_F(DNSClientTest, TimeoutFromARES) {
  StartValidRequest();
  AdvanceTime(kAresWaitMS);
  ares_result_ = ARES_ETIMEOUT;
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, ARES_SOCKET_BAD, ARES_SOCKET_BAD))
        .WillOnce(InvokeWithoutArgs(this, &DNSClientTest::CallReplyCB));
  ExpectPostCompletionTask();
  CallTimeout();
  EXPECT_CALL(callback_target_, CallTarget(
      ErrorIs(Error::kOperationTimeout, DNSClient::kErrorTimedOut), _));
  CallCompletion();
}

// Failed request due to "host not found" reported by ARES.
TEST_F(DNSClientTest, HostNotFound) {
  StartValidRequest();
  AdvanceTime(kAresWaitMS);
  ares_result_ = ARES_ENOTFOUND;
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, kAresFd, ARES_SOCKET_BAD))
      .WillOnce(InvokeWithoutArgs(this, &DNSClientTest::CallReplyCB));
  ExpectPostCompletionTask();
  CallDNSRead();
  EXPECT_CALL(callback_target_, CallTarget(
      ErrorIs(Error::kOperationFailed, DNSClient::kErrorNotFound), _));
  CallCompletion();
}

// Make sure IOHandles are deallocated when GetSock() reports them gone.
TEST_F(DNSClientTest, IOHandleDeallocGetSock) {
  SetupRequest(kGoodName, kGoodServer);
  // This isn't any kind of scoped/ref pointer because we are tracking dealloc.
  SentinelIOHandler* io_handler = new SentinelIOHandler();
  EXPECT_CALL(dispatcher_,
              CreateReadyHandler(kAresFd, IOHandler::kModeInput, _))
      .WillOnce(Return(io_handler));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  SetActive();
  Error error;
  ASSERT_TRUE(dns_client_->Start(kGoodName, &error));
  AdvanceTime(kAresWaitMS);
  SetInActive();
  EXPECT_CALL(*io_handler, Die());
  EXPECT_CALL(ares_, ProcessFd(kAresChannel, kAresFd, ARES_SOCKET_BAD));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  CallDNSRead();
  EXPECT_CALL(ares_, Destroy(kAresChannel));
}

// Make sure IOHandles are deallocated when Stop() is called.
TEST_F(DNSClientTest, IOHandleDeallocStop) {
  SetupRequest(kGoodName, kGoodServer);
  // This isn't any kind of scoped/ref pointer because we are tracking dealloc.
  SentinelIOHandler* io_handler = new SentinelIOHandler();
  EXPECT_CALL(dispatcher_,
              CreateReadyHandler(kAresFd, IOHandler::kModeInput, _))
      .WillOnce(Return(io_handler));
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, kAresWaitMS));
  SetActive();
  Error error;
  ASSERT_TRUE(dns_client_->Start(kGoodName, &error));
  EXPECT_CALL(*io_handler, Die());
  EXPECT_CALL(ares_, Destroy(kAresChannel));
  dns_client_->Stop();
}

}  // namespace shill
