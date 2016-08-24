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

#include "shill/http_request.h"

#include <netinet/in.h>

#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/strings/stringprintf.h>
#include <gtest/gtest.h>

#include "shill/http_url.h"
#include "shill/mock_async_connection.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_dns_client.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using base::Unretained;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::ReturnNew;
using ::testing::ReturnRef;
using ::testing::SetArgumentPointee;
using ::testing::StrEq;
using ::testing::StrictMock;
using ::testing::Test;

namespace shill {

namespace {
const char kTextSiteName[] = "www.chromium.org";
const char kTextURL[] = "http://www.chromium.org/path/to/resource";
const char kNumericURL[] = "http://10.1.1.1";
const char kPath[] = "/path/to/resource";
const char kInterfaceName[] = "int0";
const char kDNSServer0[] = "8.8.8.8";
const char kDNSServer1[] = "8.8.4.4";
const char* kDNSServers[] = { kDNSServer0, kDNSServer1 };
const char kServerAddress[] = "10.1.1.1";
const int kServerFD = 10203;
const int kServerPort = 80;
}  // namespace

MATCHER_P(IsIPAddress, address, "") {
  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(address));
  return ip_address.Equals(arg);
}

MATCHER_P(ByteStringMatches, byte_string, "") {
  return byte_string.Equals(arg);
}

MATCHER_P(CallbackEq, callback, "") {
  return arg.Equals(callback);
}

class HTTPRequestTest : public Test {
 public:
  HTTPRequestTest()
      : interface_name_(kInterfaceName),
        server_async_connection_(new StrictMock<MockAsyncConnection>()),
        dns_servers_(kDNSServers, kDNSServers + 2),
        dns_client_(new StrictMock<MockDNSClient>()),
        device_info_(
            new NiceMock<MockDeviceInfo>(&control_, nullptr, nullptr, nullptr)),
        connection_(new StrictMock<MockConnection>(device_info_.get())) {}

 protected:
  class CallbackTarget {
   public:
    CallbackTarget()
        : read_event_callback_(
              Bind(&CallbackTarget::ReadEventCallTarget, Unretained(this))),
          result_callback_(
              Bind(&CallbackTarget::ResultCallTarget, Unretained(this))) {}

    MOCK_METHOD1(ReadEventCallTarget, void(const ByteString& response_data));
    MOCK_METHOD2(ResultCallTarget, void(HTTPRequest::Result result,
                                        const ByteString& response_data));
    const Callback<void(const ByteString&)>& read_event_callback() {
      return read_event_callback_;
    }
    const Callback<void(HTTPRequest::Result,
                        const ByteString&)>& result_callback() {
      return result_callback_;
    }

   private:
    Callback<void(const ByteString&)> read_event_callback_;
    Callback<void(HTTPRequest::Result, const ByteString&)> result_callback_;
  };

  virtual void SetUp() {
    EXPECT_CALL(*connection_.get(), IsIPv6())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*connection_.get(), interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
    EXPECT_CALL(*connection_.get(), dns_servers())
        .WillRepeatedly(ReturnRef(dns_servers_));

    request_.reset(new HTTPRequest(connection_, &dispatcher_, &sockets_));
    // Passes ownership.
    request_->dns_client_.reset(dns_client_);
    // Passes ownership.
    request_->server_async_connection_.reset(server_async_connection_);
  }
  virtual void TearDown() {
    if (request_->is_running_) {
      ExpectStop();

      // Subtle: Make sure the finalization of the request happens while our
      // expectations are still active.
      request_.reset();
    }
  }
  size_t FindInRequestData(const string& find_string) {
    string request_string(
        reinterpret_cast<char*>(request_->request_data_.GetData()),
        request_->request_data_.GetLength());
    return request_string.find(find_string);
  }
  // Accessors
  const ByteString& GetRequestData() {
    return request_->request_data_;
  }
  HTTPRequest* request() { return request_.get(); }
  MockSockets& sockets() { return sockets_; }

  // Expectations
  void ExpectReset() {
    EXPECT_EQ(connection_.get(), request_->connection_.get());
    EXPECT_EQ(&dispatcher_, request_->dispatcher_);
    EXPECT_EQ(&sockets_, request_->sockets_);
    EXPECT_TRUE(request_->result_callback_.is_null());
    EXPECT_TRUE(request_->read_event_callback_.is_null());
    EXPECT_FALSE(request_->connect_completion_callback_.is_null());
    EXPECT_FALSE(request_->dns_client_callback_.is_null());
    EXPECT_FALSE(request_->read_server_callback_.is_null());
    EXPECT_FALSE(request_->write_server_callback_.is_null());
    EXPECT_FALSE(request_->read_server_handler_.get());
    EXPECT_FALSE(request_->write_server_handler_.get());
    EXPECT_EQ(dns_client_, request_->dns_client_.get());
    EXPECT_EQ(server_async_connection_,
              request_->server_async_connection_.get());
    EXPECT_TRUE(request_->server_hostname_.empty());
    EXPECT_EQ(-1, request_->server_port_);
    EXPECT_EQ(-1, request_->server_socket_);
    EXPECT_EQ(HTTPRequest::kResultUnknown, request_->timeout_result_);
    EXPECT_TRUE(request_->request_data_.IsEmpty());
    EXPECT_TRUE(request_->response_data_.IsEmpty());
    EXPECT_FALSE(request_->is_running_);
  }
  void ExpectStop() {
    if (request_->server_socket_ != -1) {
      EXPECT_CALL(sockets(), Close(kServerFD))
          .WillOnce(Return(0));
    }
    EXPECT_CALL(*dns_client_, Stop())
        .Times(AtLeast(1));
    EXPECT_CALL(*server_async_connection_, Stop())
        .Times(AtLeast(1));
    EXPECT_CALL(*connection_.get(), ReleaseRouting());
  }
  void ExpectSetTimeout(int timeout) {
    EXPECT_CALL(dispatcher_, PostDelayedTask(_, timeout * 1000));
  }
  void ExpectSetConnectTimeout() {
    ExpectSetTimeout(HTTPRequest::kConnectTimeoutSeconds);
  }
  void ExpectSetInputTimeout() {
    ExpectSetTimeout(HTTPRequest::kInputTimeoutSeconds);
  }
  void ExpectInResponse(const string& expected_response_data) {
    string response_string(
        reinterpret_cast<char*>(request_->response_data_.GetData()),
        request_->response_data_.GetLength());
    EXPECT_NE(string::npos, response_string.find(expected_response_data));
  }
  void ExpectDNSRequest(const string& host, bool return_value) {
    EXPECT_CALL(*dns_client_, Start(StrEq(host), _))
        .WillOnce(Return(return_value));
  }
  void ExpectAsyncConnect(const string& address, int port,
                          bool return_value) {
    EXPECT_CALL(*server_async_connection_, Start(IsIPAddress(address), port))
        .WillOnce(Return(return_value));
    if (return_value) {
      ExpectSetConnectTimeout();
    }
  }
  void  InvokeSyncConnect(const IPAddress& /*address*/, int /*port*/) {
    CallConnectCompletion(true, kServerFD);
  }
  void CallConnectCompletion(bool success, int fd) {
    request_->OnConnectCompletion(success, fd);
  }
  void ExpectSyncConnect(const string& address, int port) {
    EXPECT_CALL(*server_async_connection_, Start(IsIPAddress(address), port))
        .WillOnce(DoAll(Invoke(this, &HTTPRequestTest::InvokeSyncConnect),
                        Return(true)));
  }
  void ExpectConnectFailure() {
    EXPECT_CALL(*server_async_connection_, Start(_, _))
        .WillOnce(Return(false));
  }
  void ExpectMonitorServerInput() {
    EXPECT_CALL(dispatcher_,
                CreateInputHandler(kServerFD,
                                   CallbackEq(request_->read_server_callback_),
                                   _))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectSetInputTimeout();
  }
  void ExpectMonitorServerOutput() {
    EXPECT_CALL(dispatcher_,
                CreateReadyHandler(
                    kServerFD, IOHandler::kModeOutput,
                    CallbackEq(request_->write_server_callback_)))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectSetInputTimeout();
  }
  void ExpectRouteRequest() {
    EXPECT_CALL(*connection_.get(), RequestRouting());
  }
  void ExpectRouteRelease() {
    EXPECT_CALL(*connection_.get(), ReleaseRouting());
  }
  void ExpectResultCallback(HTTPRequest::Result result) {
    EXPECT_CALL(target_, ResultCallTarget(result, _));
  }
  void InvokeResultVerify(HTTPRequest::Result result,
                          const ByteString& response_data) {
    EXPECT_EQ(HTTPRequest::kResultSuccess, result);
    EXPECT_TRUE(expected_response_.Equals(response_data));
  }
  void ExpectResultCallbackWithResponse(const string& response) {
    expected_response_ = ByteString(response, false);
    EXPECT_CALL(target_, ResultCallTarget(HTTPRequest::kResultSuccess, _))
        .WillOnce(Invoke(this, &HTTPRequestTest::InvokeResultVerify));
  }
  void ExpectReadEventCallback(const string& response) {
    ByteString response_data(response, false);
    EXPECT_CALL(target_, ReadEventCallTarget(ByteStringMatches(response_data)));
  }
  void GetDNSResultFailure(const string& error_msg) {
    Error error(Error::kOperationFailed, error_msg);
    IPAddress address(IPAddress::kFamilyUnknown);
    request_->GetDNSResult(error, address);
  }
  void GetDNSResultSuccess(const IPAddress& address) {
    Error error;
    request_->GetDNSResult(error, address);
  }
  void OnConnectCompletion(bool result, int sockfd) {
    request_->OnConnectCompletion(result, sockfd);
  }
  void ReadFromServer(const string& data) {
    const unsigned char* ptr =
        reinterpret_cast<const unsigned char*>(data.c_str());
    vector<unsigned char> data_writable(ptr, ptr + data.length());
    InputData server_data(data_writable.data(), data_writable.size());
    request_->ReadFromServer(&server_data);
  }
  void WriteToServer(int fd) {
    request_->WriteToServer(fd);
  }
  HTTPRequest::Result StartRequest(const string& url) {
    HTTPURL http_url;
    EXPECT_TRUE(http_url.ParseFromString(url));
    return request_->Start(http_url,
                           target_.read_event_callback(),
                           target_.result_callback());
  }
  void SetupConnectWithURL(const string& url, const string& expected_hostname) {
    ExpectRouteRequest();
    ExpectDNSRequest(expected_hostname, true);
    EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(url));
    IPAddress addr(IPAddress::kFamilyIPv4);
    EXPECT_TRUE(addr.SetAddressFromString(kServerAddress));
    GetDNSResultSuccess(addr);
  }
  void SetupConnect() {
    SetupConnectWithURL(kTextURL, kTextSiteName);
  }
  void SetupConnectAsync() {
    ExpectAsyncConnect(kServerAddress, kServerPort, true);
    SetupConnect();
  }
  void SetupConnectComplete() {
    SetupConnectAsync();
    ExpectMonitorServerOutput();
    OnConnectCompletion(true, kServerFD);
  }
  void CallTimeoutTask() {
    request_->TimeoutTask();
  }
  void CallServerErrorCallback() {
    request_->OnServerReadError(string());
  }

 private:
  const string interface_name_;
  // Owned by the HTTPRequest, but tracked here for EXPECT().
  StrictMock<MockAsyncConnection>* server_async_connection_;
  vector<string> dns_servers_;
  // Owned by the HTTPRequest, but tracked here for EXPECT().
  StrictMock<MockDNSClient>* dns_client_;
  StrictMock<MockEventDispatcher> dispatcher_;
  MockControl control_;
  std::unique_ptr<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  std::unique_ptr<HTTPRequest> request_;
  StrictMock<MockSockets> sockets_;
  StrictMock<CallbackTarget> target_;
  ByteString expected_response_;
};

TEST_F(HTTPRequestTest, Constructor) {
  ExpectReset();
}


TEST_F(HTTPRequestTest, FailConnectNumericSynchronous) {
  ExpectRouteRequest();
  ExpectConnectFailure();
  ExpectStop();
  EXPECT_EQ(HTTPRequest::kResultConnectionFailure, StartRequest(kNumericURL));
  ExpectReset();
}

TEST_F(HTTPRequestTest, FailConnectNumericAsynchronous) {
  ExpectRouteRequest();
  ExpectAsyncConnect(kServerAddress, HTTPURL::kDefaultHTTPPort, true);
  EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(kNumericURL));
  ExpectResultCallback(HTTPRequest::kResultConnectionFailure);
  ExpectStop();
  CallConnectCompletion(false, -1);
  ExpectReset();
}

TEST_F(HTTPRequestTest, FailConnectNumericTimeout) {
  ExpectRouteRequest();
  ExpectAsyncConnect(kServerAddress, HTTPURL::kDefaultHTTPPort, true);
  EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(kNumericURL));
  ExpectResultCallback(HTTPRequest::kResultConnectionTimeout);
  ExpectStop();
  CallTimeoutTask();
  ExpectReset();
}

TEST_F(HTTPRequestTest, SyncConnectNumeric) {
  ExpectRouteRequest();
  ExpectSyncConnect(kServerAddress, HTTPURL::kDefaultHTTPPort);
  ExpectMonitorServerOutput();
  EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(kNumericURL));
}

TEST_F(HTTPRequestTest, FailDNSStart) {
  ExpectRouteRequest();
  ExpectDNSRequest(kTextSiteName, false);
  ExpectStop();
  EXPECT_EQ(HTTPRequest::kResultDNSFailure, StartRequest(kTextURL));
  ExpectReset();
}

TEST_F(HTTPRequestTest, FailDNSFailure) {
  ExpectRouteRequest();
  ExpectDNSRequest(kTextSiteName, true);
  EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(kTextURL));
  ExpectResultCallback(HTTPRequest::kResultDNSFailure);
  ExpectStop();
  GetDNSResultFailure(DNSClient::kErrorNoData);
  ExpectReset();
}

TEST_F(HTTPRequestTest, FailDNSTimeout) {
  ExpectRouteRequest();
  ExpectDNSRequest(kTextSiteName, true);
  EXPECT_EQ(HTTPRequest::kResultInProgress, StartRequest(kTextURL));
  ExpectResultCallback(HTTPRequest::kResultDNSTimeout);
  ExpectStop();
  const string error(DNSClient::kErrorTimedOut);
  GetDNSResultFailure(error);
  ExpectReset();
}

TEST_F(HTTPRequestTest, FailConnectText) {
  ExpectConnectFailure();
  ExpectResultCallback(HTTPRequest::kResultConnectionFailure);
  ExpectStop();
  SetupConnect();
  ExpectReset();
}

TEST_F(HTTPRequestTest, ConnectComplete) {
  SetupConnectComplete();
}

TEST_F(HTTPRequestTest, RequestTimeout) {
  SetupConnectComplete();
  ExpectResultCallback(HTTPRequest::kResultRequestTimeout);
  ExpectStop();
  CallTimeoutTask();
}

TEST_F(HTTPRequestTest, RequestData) {
  SetupConnectComplete();
  EXPECT_EQ(0, FindInRequestData(string("GET ") + kPath));
  EXPECT_NE(string::npos,
            FindInRequestData(string("\r\nHost: ") + kTextSiteName));
  ByteString request_data = GetRequestData();
  EXPECT_CALL(sockets(), Send(kServerFD, _, request_data.GetLength(), 0))
      .WillOnce(Return(request_data.GetLength() - 1));
  ExpectSetInputTimeout();
  WriteToServer(kServerFD);
  EXPECT_CALL(sockets(), Send(kServerFD, _, 1, 0))
      .WillOnce(Return(1));
  ExpectMonitorServerInput();
  WriteToServer(kServerFD);
}

TEST_F(HTTPRequestTest, ResponseTimeout) {
  SetupConnectComplete();
  ByteString request_data = GetRequestData();
  EXPECT_CALL(sockets(), Send(kServerFD, _, request_data.GetLength(), 0))
      .WillOnce(Return(request_data.GetLength()));
  ExpectMonitorServerInput();
  WriteToServer(kServerFD);
  ExpectResultCallback(HTTPRequest::kResultResponseTimeout);
  ExpectStop();
  CallTimeoutTask();
}

TEST_F(HTTPRequestTest, ResponseInputError) {
  SetupConnectComplete();
  ByteString request_data = GetRequestData();
  EXPECT_CALL(sockets(), Send(kServerFD, _, request_data.GetLength(), 0))
      .WillOnce(Return(request_data.GetLength()));
  ExpectMonitorServerInput();
  WriteToServer(kServerFD);
  ExpectResultCallback(HTTPRequest::kResultResponseFailure);
  ExpectStop();
  CallServerErrorCallback();
}

TEST_F(HTTPRequestTest, ResponseData) {
  SetupConnectComplete();
  const string response0("hello");
  ExpectReadEventCallback(response0);
  ExpectSetInputTimeout();
  ReadFromServer(response0);
  ExpectInResponse(response0);

  const string response1(" to you");
  ExpectReadEventCallback(response0 + response1);
  ExpectSetInputTimeout();
  ReadFromServer(response1);
  ExpectInResponse(response1);

  ExpectResultCallbackWithResponse(response0 + response1);
  ExpectStop();
  ReadFromServer("");
  ExpectReset();
}

}  // namespace shill
