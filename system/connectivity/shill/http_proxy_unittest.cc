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

#include "shill/http_proxy.h"

#include <netinet/in.h>

#include <memory>
#include <string>
#include <vector>

#include <base/strings/stringprintf.h>
#include <gtest/gtest.h>

#include "shill/mock_async_connection.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_dns_client.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/net/ip_address.h"
#include "shill/net/mock_sockets.h"

using base::StringPrintf;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::AnyNumber;
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
const char kBadHeaderMissingURL[] = "BLAH\r\n";
const char kBadHeaderMissingVersion[] = "BLAH http://hostname\r\n";
const char kBadHostnameLine[] = "GET HTTP/1.1 http://hostname\r\n";
const char kBasicGetHeader[] = "GET / HTTP/1.1\r\n";
const char kBasicGetHeaderWithURL[] =
    "GET http://www.chromium.org/ HTTP/1.1\r\n";
const char kBasicGetHeaderWithURLNoTrailingSlash[] =
    "GET http://www.chromium.org HTTP/1.1\r\n";
const char kConnectQuery[] =
    "CONNECT 10.10.10.10:443 HTTP/1.1\r\n"
    "Host: 10.10.10.10:443\r\n\r\n";
const char kQueryTemplate[] = "GET %s HTTP/%s\r\n%s"
    "User-Agent: Mozilla/5.0 (X11; CrOS i686 1299.0.2011) "
    "AppleWebKit/535.8 (KHTML, like Gecko) Chrome/17.0.936.0 Safari/535.8\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;"
    "q=0.9,*/*;q=0.8\r\n"
    "Accept-Encoding: gzip,deflate,sdch\r\n"
    "Accept-Language: en-US,en;q=0.8,ja;q=0.6\r\n"
    "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n"
    "Cookie: PREF=ID=xxxxxxxxxxxxxxxx:U=xxxxxxxxxxxxxxxx:FF=0:"
    "TM=1317340083:LM=1317390705:GM=1:S=_xxxxxxxxxxxxxxx; "
    "NID=52=xxxxxxxxxxxxxxxxxxxx-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_xxxxxxxxxxxxxxxxxxxxxxx; "
    "HSID=xxxxxxxxxxxx-xxxx; APISID=xxxxxxxxxxxxxxxx/xxxxxxxxxxxxxxxxx; "
    "SID=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_xxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx-xxxxxxxxxxxxxxx"
    "xxx_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx-xxxxxxxxxxxxxxxxxx"
    "_xxxxx-xxxxxxxxxxxxxxxxxxxxxxxxxx-xx-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxx\r\n\r\n";
const char kInterfaceName[] = "int0";
const char kDNSServer0[] = "8.8.8.8";
const char kDNSServer1[] = "8.8.4.4";
const char kServerAddress[] = "10.10.10.10";
const char* kDNSServers[] = { kDNSServer0, kDNSServer1 };
const int kProxyFD = 10203;
const int kServerFD = 10204;
const int kClientFD = 10205;
const int kServerPort = 40506;
const int kConnectPort = 443;
}  // namespace

MATCHER_P(IsIPAddress, address, "") {
  IPAddress ip_address(IPAddress::kFamilyIPv4);
  EXPECT_TRUE(ip_address.SetAddressFromString(address));
  return ip_address.Equals(arg);
}

MATCHER_P(CallbackEq, callback, "") {
  return arg.Equals(callback);
}

class HTTPProxyTest : public Test {
 public:
  HTTPProxyTest()
      : interface_name_(kInterfaceName),
        server_async_connection_(nullptr),
        dns_servers_(kDNSServers, kDNSServers + 2),
        dns_client_(nullptr),
        device_info_(
            new NiceMock<MockDeviceInfo>(&control_, nullptr, nullptr, nullptr)),
        connection_(new StrictMock<MockConnection>(device_info_.get())),
        proxy_(connection_) {}

 protected:
  virtual void SetUp() {
    EXPECT_CALL(*connection_.get(), interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
    EXPECT_CALL(*connection_.get(), dns_servers())
        .WillRepeatedly(ReturnRef(dns_servers_));
  }
  virtual void TearDown() {
    if (proxy_.sockets_) {
      ExpectStop();
    }
    const int proxy_fds[] = {
      proxy_.client_socket_,
      proxy_.server_socket_,
      proxy_.proxy_socket_
    };
    for (const int fd : proxy_fds) {
      if (fd != -1) {
        EXPECT_CALL(sockets_, Close(fd));
      }
    }
  }
  string CreateRequest(const string& url, const string& http_version,
                       const string& extra_lines) {
    string append_lines(extra_lines);
    if (append_lines.size()) {
      append_lines.append("\r\n");
    }
    return StringPrintf(kQueryTemplate, url.c_str(), http_version.c_str(),
                        append_lines.c_str());
  }
  int InvokeGetSockName(int fd, struct sockaddr* addr_out,
                        socklen_t* sockaddr_size) {
    struct sockaddr_in addr;
    EXPECT_EQ(kProxyFD, fd);
    EXPECT_GE(sizeof(sockaddr_in), *sockaddr_size);
    addr.sin_addr.s_addr = 0;
    addr.sin_port = kServerPort;
    memcpy(addr_out, &addr, sizeof(addr));
    *sockaddr_size = sizeof(sockaddr_in);
    return 0;
  }
  void  InvokeSyncConnect(const IPAddress& /*address*/, int /*port*/) {
    proxy_.OnConnectCompletion(true, kServerFD);
  }
  size_t FindInRequest(const string& find_string) {
    const ByteString& request_data = GetClientData();
    string request_string(
        reinterpret_cast<const char*>(request_data.GetConstData()),
        request_data.GetLength());
    return request_string.find(find_string);
  }
  // Accessors
  const ByteString& GetClientData() {
    return proxy_.client_data_;
  }
  HTTPProxy* proxy() { return &proxy_; }
  HTTPProxy::State GetProxyState() {
    return proxy_.state_;
  }
  const ByteString& GetServerData() {
    return proxy_.server_data_;
  }
  MockSockets& sockets() { return sockets_; }
  MockEventDispatcher& dispatcher() { return dispatcher_; }


  // Expectations
  void ExpectClientReset() {
    EXPECT_EQ(-1, proxy_.client_socket_);
    EXPECT_TRUE(proxy_.client_version_.empty());
    EXPECT_EQ(HTTPProxy::kDefaultServerPort, proxy_.server_port_);
    EXPECT_EQ(-1, proxy_.server_socket_);
    EXPECT_TRUE(proxy_.idle_timeout_.IsCancelled());
    EXPECT_TRUE(proxy_.client_headers_.empty());
    EXPECT_TRUE(proxy_.server_hostname_.empty());
    EXPECT_TRUE(proxy_.client_data_.IsEmpty());
    EXPECT_TRUE(proxy_.server_data_.IsEmpty());
    EXPECT_FALSE(proxy_.read_client_handler_.get());
    EXPECT_FALSE(proxy_.write_client_handler_.get());
    EXPECT_FALSE(proxy_.read_server_handler_.get());
    EXPECT_FALSE(proxy_.write_server_handler_.get());
    EXPECT_FALSE(proxy_.is_route_requested_);
  }
  void ExpectReset() {
    EXPECT_FALSE(proxy_.accept_handler_.get());
    EXPECT_EQ(proxy_.connection_.get(), connection_.get());
    EXPECT_FALSE(proxy_.dispatcher_);
    EXPECT_FALSE(proxy_.dns_client_.get());
    EXPECT_EQ(-1, proxy_.proxy_port_);
    EXPECT_EQ(-1, proxy_.proxy_socket_);
    EXPECT_FALSE(proxy_.server_async_connection_.get());
    EXPECT_FALSE(proxy_.sockets_);
    EXPECT_EQ(HTTPProxy::kStateIdle, proxy_.state_);
    ExpectClientReset();
  }
  void ExpectStart() {
    EXPECT_CALL(sockets(), Socket(_, _, _))
        .WillOnce(Return(kProxyFD));
    EXPECT_CALL(sockets(), Bind(kProxyFD, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(sockets(), GetSockName(kProxyFD, _, _))
        .WillOnce(Invoke(this, &HTTPProxyTest::InvokeGetSockName));
    EXPECT_CALL(sockets(), SetNonBlocking(kProxyFD))
        .WillOnce(Return(0));
    EXPECT_CALL(sockets(), Listen(kProxyFD, _))
        .WillOnce(Return(0));
    EXPECT_CALL(dispatcher_,
                CreateReadyHandler(kProxyFD,
                                   IOHandler::kModeInput,
                                   CallbackEq(proxy_.accept_callback_)))
        .WillOnce(ReturnNew<IOHandler>());
  }
  void ExpectStop() {
     if (dns_client_) {
       EXPECT_CALL(*dns_client_, Stop())
           .Times(AtLeast(1));
     }
     if (server_async_connection_) {
       EXPECT_CALL(*server_async_connection_, Stop())
           .Times(AtLeast(1));
     }
     if (proxy_.is_route_requested_) {
       EXPECT_CALL(*connection_.get(), ReleaseRouting());
     }
  }
  void ExpectClientInput(int fd) {
    EXPECT_CALL(sockets(), Accept(kProxyFD, _, _))
        .WillOnce(Return(fd));
    EXPECT_CALL(sockets(), SetNonBlocking(fd))
        .WillOnce(Return(0));
    EXPECT_CALL(dispatcher(),
                CreateInputHandler(fd,
                                   CallbackEq(proxy_.read_client_callback_), _))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectTransactionTimeout();
    ExpectClientHeaderTimeout();
  }
  void ExpectTimeout(int timeout) {
    EXPECT_CALL(dispatcher_, PostDelayedTask(_, timeout * 1000));
  }
  void ExpectClientHeaderTimeout() {
    ExpectTimeout(HTTPProxy::kClientHeaderTimeoutSeconds);
  }
  void ExpectConnectTimeout() {
    ExpectTimeout(HTTPProxy::kConnectTimeoutSeconds);
  }
  void ExpectInputTimeout() {
    ExpectTimeout(HTTPProxy::kInputTimeoutSeconds);
  }
  void ExpectRepeatedInputTimeout() {
    EXPECT_CALL(dispatcher_,
                PostDelayedTask(_, HTTPProxy::kInputTimeoutSeconds * 1000))
        .Times(AnyNumber());
  }
  void ExpectTransactionTimeout() {
    ExpectTimeout(HTTPProxy::kTransactionTimeoutSeconds);
  }
  void ExpectInClientResponse(const string& response_data) {
    string server_data(reinterpret_cast<char*>(proxy_.server_data_.GetData()),
                       proxy_.server_data_.GetLength());
    EXPECT_NE(string::npos, server_data.find(response_data));
  }
  void ExpectClientError(int code, const string& error) {
    EXPECT_EQ(HTTPProxy::kStateFlushResponse, GetProxyState());
    string status_line = StringPrintf("HTTP/1.1 %d ERROR", code);
    ExpectInClientResponse(status_line);
    ExpectInClientResponse(error);
  }
  void ExpectClientInternalError() {
    ExpectClientError(500, HTTPProxy::kInternalErrorMsg);
  }
  void ExpectClientVersion(const string& version) {
    EXPECT_EQ(version, proxy_.client_version_);
  }
  void ExpectServerHostname(const string& hostname) {
    EXPECT_EQ(hostname, proxy_.server_hostname_);
  }
  void ExpectFirstLine(const string& line) {
    EXPECT_EQ(line, proxy_.client_headers_[0] + "\r\n");
  }
  void ExpectDNSRequest(const string& host, bool return_value) {
    EXPECT_CALL(*dns_client_, Start(StrEq(host), _))
        .WillOnce(Return(return_value));
  }
  void ExpectAsyncConnect(const string& address, int port,
                          bool return_value) {
    EXPECT_CALL(*server_async_connection_, Start(IsIPAddress(address), port))
        .WillOnce(Return(return_value));
  }
  void ExpectSyncConnect(const string& address, int port) {
    EXPECT_CALL(*server_async_connection_, Start(IsIPAddress(address), port))
        .WillOnce(DoAll(Invoke(this, &HTTPProxyTest::InvokeSyncConnect),
                        Return(true)));
  }
  void ExpectClientData() {
    EXPECT_CALL(dispatcher(),
                CreateReadyHandler(kClientFD,
                                   IOHandler::kModeOutput,
                                   CallbackEq(proxy_.write_client_callback_)))
        .WillOnce(ReturnNew<IOHandler>());
  }
  void ExpectClientResult() {
    ExpectClientData();
    ExpectInputTimeout();
  }
  void ExpectServerInput() {
    EXPECT_CALL(dispatcher(),
                CreateInputHandler(kServerFD,
                                   CallbackEq(proxy_.read_server_callback_), _))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectInputTimeout();
  }
  void ExpectServerOutput() {
    EXPECT_CALL(dispatcher(),
                CreateReadyHandler(kServerFD,
                                   IOHandler::kModeOutput,
                                   CallbackEq(proxy_.write_server_callback_)))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectInputTimeout();
  }
  void ExpectRepeatedServerOutput() {
    EXPECT_CALL(dispatcher(),
                CreateReadyHandler(kServerFD, IOHandler::kModeOutput,
                                   CallbackEq(proxy_.write_server_callback_)))
        .WillOnce(ReturnNew<IOHandler>());
    ExpectRepeatedInputTimeout();
  }
  void ExpectTunnelClose() {
    EXPECT_CALL(sockets(), Close(kClientFD))
        .WillOnce(Return(0));
    EXPECT_CALL(sockets(), Close(kServerFD))
        .WillOnce(Return(0));
    ExpectStop();
  }
  void ExpectRouteRequest() {
    EXPECT_CALL(*connection_.get(), RequestRouting());
  }
  void ExpectRouteRelease() {
    EXPECT_CALL(*connection_.get(), ReleaseRouting());
  }

  // Callers for various private routines in the proxy
  bool StartProxy() {
    bool ret = proxy_.Start(&dispatcher_, &sockets_);
    if (ret) {
      dns_client_ = new StrictMock<MockDNSClient>();
      // Passes ownership.
      proxy_.dns_client_.reset(dns_client_);
      server_async_connection_ = new StrictMock<MockAsyncConnection>();
      // Passes ownership.
      proxy_.server_async_connection_.reset(server_async_connection_);
    }
    return ret;
  }
  void AcceptClient(int fd) {
    proxy_.AcceptClient(fd);
  }
  void GetDNSResultFailure(const string& error_msg) {
    Error error(Error::kOperationFailed, error_msg);
    IPAddress address(IPAddress::kFamilyUnknown);
    proxy_.GetDNSResult(error, address);
  }
  void GetDNSResultSuccess(const IPAddress& address) {
    Error error;
    proxy_.GetDNSResult(error, address);
  }
  void OnConnectCompletion(bool result, int sockfd) {
    proxy_.OnConnectCompletion(result, sockfd);
  }
  void ReadFromClient(const string& data) {
    const unsigned char* ptr =
        reinterpret_cast<const unsigned char*>(data.c_str());
    vector<unsigned char> data_bytes(ptr, ptr + data.length());
    InputData proxy_data(data_bytes.data(), data_bytes.size());
    proxy_.ReadFromClient(&proxy_data);
  }
  void ReadFromServer(const string& data) {
    const unsigned char* ptr =
        reinterpret_cast<const unsigned char*>(data.c_str());
    vector<unsigned char> data_bytes(ptr, ptr + data.length());
    InputData proxy_data(data_bytes.data(), data_bytes.size());
    proxy_.ReadFromServer(&proxy_data);
  }
  void SendClientError(int code, const string& error) {
    proxy_.SendClientError(code, error);
    EXPECT_FALSE(proxy_.server_data_.IsEmpty());
  }
  void StopClient() {
    EXPECT_CALL(*dns_client_, Stop());
    EXPECT_CALL(*server_async_connection_, Stop());
    proxy_.StopClient();
  }
  void StopProxy() {
    ExpectStop();
    proxy_.Stop();
    server_async_connection_ = nullptr;
    dns_client_ = nullptr;
    ExpectReset();
  }
  void WriteToClient(int fd) {
    proxy_.WriteToClient(fd);
  }
  void WriteToServer(int fd) {
    proxy_.WriteToServer(fd);
  }

  void SetupClient() {
    ExpectStart();
    ASSERT_TRUE(StartProxy());
    ExpectClientInput(kClientFD);
    AcceptClient(kProxyFD);
    EXPECT_EQ(HTTPProxy::kStateReadClientHeader, GetProxyState());
  }
  void SetupConnectWithRequest(const string& url, const string& http_version,
                               const string& extra_lines) {
    ExpectDNSRequest("www.chromium.org", true);
    ExpectRouteRequest();
    ReadFromClient(CreateRequest(url, http_version, extra_lines));
    IPAddress addr(IPAddress::kFamilyIPv4);
    EXPECT_TRUE(addr.SetAddressFromString(kServerAddress));
    GetDNSResultSuccess(addr);
  }
  void SetupConnect() {
    SetupConnectWithRequest("/", "1.1", "Host: www.chromium.org:40506");
  }
  void SetupConnectAsync() {
    SetupClient();
    ExpectAsyncConnect(kServerAddress, kServerPort, true);
    ExpectConnectTimeout();
    SetupConnect();
  }
  void SetupConnectComplete() {
    SetupConnectAsync();
    ExpectServerOutput();
    OnConnectCompletion(true, kServerFD);
    EXPECT_EQ(HTTPProxy::kStateTunnelData, GetProxyState());
  }
  void CauseReadError() {
    proxy_.OnReadError(string());
  }

 private:
  const string interface_name_;
  // Owned by the HTTPProxy, but tracked here for EXPECT().
  StrictMock<MockAsyncConnection>* server_async_connection_;
  vector<string> dns_servers_;
  // Owned by the HTTPProxy, but tracked here for EXPECT().
  StrictMock<MockDNSClient>* dns_client_;
  MockEventDispatcher dispatcher_;
  MockControl control_;
  std::unique_ptr<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  StrictMock<MockSockets> sockets_;
  HTTPProxy proxy_;  // Destroy first, before anything it references.
};

TEST_F(HTTPProxyTest, StartFailSocket) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(StartProxy());
  ExpectReset();
}

TEST_F(HTTPProxyTest, StartFailBind) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kProxyFD));
  EXPECT_CALL(sockets(), Bind(kProxyFD, _, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Close(kProxyFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(StartProxy());
  ExpectReset();
}

TEST_F(HTTPProxyTest, StartFailGetSockName) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kProxyFD));
  EXPECT_CALL(sockets(), Bind(kProxyFD, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), GetSockName(kProxyFD, _, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Close(kProxyFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(StartProxy());
  ExpectReset();
}

TEST_F(HTTPProxyTest, StartFailSetNonBlocking) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kProxyFD));
  EXPECT_CALL(sockets(), Bind(kProxyFD, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), GetSockName(kProxyFD, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), SetNonBlocking(kProxyFD))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Close(kProxyFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(StartProxy());
  ExpectReset();
}

TEST_F(HTTPProxyTest, StartFailListen) {
  EXPECT_CALL(sockets(), Socket(_, _, _))
      .WillOnce(Return(kProxyFD));
  EXPECT_CALL(sockets(), Bind(kProxyFD, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), GetSockName(kProxyFD, _, _))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), SetNonBlocking(kProxyFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Listen(kProxyFD, _))
      .WillOnce(Return(-1));
  EXPECT_CALL(sockets(), Close(kProxyFD))
      .WillOnce(Return(0));
  EXPECT_FALSE(StartProxy());
  ExpectReset();
}

TEST_F(HTTPProxyTest, StartSuccess) {
  ExpectStart();
  EXPECT_TRUE(StartProxy());
}

TEST_F(HTTPProxyTest, SendClientError) {
  SetupClient();
  ExpectClientResult();
  SendClientError(500, "This is an error");
  ExpectClientError(500, "This is an error");

  // We succeed in sending all but one byte of the client response.
  int buf_len = GetServerData().GetLength();
  EXPECT_CALL(sockets(), Send(kClientFD, _, buf_len, 0))
      .WillOnce(Return(buf_len - 1));
  ExpectInputTimeout();
  WriteToClient(kClientFD);
  EXPECT_EQ(1, GetServerData().GetLength());
  EXPECT_EQ(HTTPProxy::kStateFlushResponse, GetProxyState());

  // When we are able to send the last byte, we close the connection.
  EXPECT_CALL(sockets(), Send(kClientFD, _, 1, 0))
      .WillOnce(Return(1));
  EXPECT_CALL(sockets(), Close(kClientFD))
      .WillOnce(Return(0));
  ExpectStop();
  WriteToClient(kClientFD);
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, ReadMissingURL) {
  SetupClient();
  ExpectClientResult();
  ReadFromClient(kBadHeaderMissingURL);
  ExpectClientError(501, "Server could not parse HTTP method");
}

TEST_F(HTTPProxyTest, ReadMissingVersion) {
  SetupClient();
  ExpectClientResult();
  ReadFromClient(kBadHeaderMissingVersion);
  ExpectClientError(501, "Server only accepts HTTP/1.x requests");
}

TEST_F(HTTPProxyTest, ReadBadHostname) {
  SetupClient();
  ExpectClientResult();
  ReadFromClient(kBadHostnameLine);
  ExpectClientInternalError();
}

TEST_F(HTTPProxyTest, GoodFirstLineWithoutURL) {
  SetupClient();
  ExpectClientHeaderTimeout();
  ReadFromClient(kBasicGetHeader);
  ExpectClientVersion("1.1");
  ExpectServerHostname("");
  ExpectFirstLine(kBasicGetHeader);
}

TEST_F(HTTPProxyTest, GoodFirstLineWithURL) {
  SetupClient();
  ExpectClientHeaderTimeout();
  ReadFromClient(kBasicGetHeaderWithURL);
  ExpectClientVersion("1.1");
  ExpectServerHostname("www.chromium.org");
  ExpectFirstLine(kBasicGetHeader);
}

TEST_F(HTTPProxyTest, GoodFirstLineWithURLNoSlash) {
  SetupClient();
  ExpectClientHeaderTimeout();
  ReadFromClient(kBasicGetHeaderWithURLNoTrailingSlash);
  ExpectClientVersion("1.1");
  ExpectServerHostname("www.chromium.org");
  ExpectFirstLine(kBasicGetHeader);
}

TEST_F(HTTPProxyTest, NoHostInRequest) {
  SetupClient();
  ExpectClientResult();
  ReadFromClient(CreateRequest("/", "1.1", ""));
  ExpectClientError(400, "I don't know what host you want me to connect to");
}

TEST_F(HTTPProxyTest, TooManyColonsInHost) {
  SetupClient();
  ExpectClientResult();
  ReadFromClient(CreateRequest("/", "1.1", "Host: www.chromium.org:80:40506"));
  ExpectClientError(400, "Too many colons in hostname");
}

TEST_F(HTTPProxyTest, ClientReadError) {
  SetupClient();
  EXPECT_CALL(sockets(), Close(kClientFD))
      .WillOnce(Return(0));
  ExpectStop();
  CauseReadError();
  ExpectClientReset();
}

TEST_F(HTTPProxyTest, DNSRequestFailure) {
  SetupClient();
  ExpectRouteRequest();
  ExpectDNSRequest("www.chromium.org", false);
  ExpectClientResult();
  ReadFromClient(CreateRequest("/", "1.1", "Host: www.chromium.org:40506"));
  ExpectClientError(502, "Could not resolve hostname");
}

TEST_F(HTTPProxyTest, DNSRequestDelayedFailure) {
  SetupClient();
  ExpectRouteRequest();
  ExpectDNSRequest("www.chromium.org", true);
  ReadFromClient(CreateRequest("/", "1.1", "Host: www.chromium.org:40506"));
  ExpectClientResult();
  const std::string not_found_error(DNSClient::kErrorNotFound);
  GetDNSResultFailure(not_found_error);
  ExpectClientError(502, string("Could not resolve hostname: ") +
                    not_found_error);
}

TEST_F(HTTPProxyTest, TrailingClientData) {
  SetupClient();
  ExpectRouteRequest();
  ExpectDNSRequest("www.chromium.org", true);
  const string trailing_data("Trailing client data");
  ReadFromClient(CreateRequest("/", "1.1", "Host: www.chromium.org:40506") +
                 trailing_data);
  EXPECT_EQ(GetClientData().GetLength() - trailing_data.length(),
            FindInRequest(trailing_data));
  EXPECT_EQ(HTTPProxy::kStateLookupServer, GetProxyState());
}

TEST_F(HTTPProxyTest, LineContinuation) {
  SetupClient();
  ExpectRouteRequest();
  ExpectDNSRequest("www.chromium.org", true);
  string text_to_keep("X-Long-Header: this is one line\r\n"
                      "\tand this is another");
  ReadFromClient(CreateRequest("http://www.chromium.org/", "1.1",
                               text_to_keep));
  EXPECT_NE(string::npos, FindInRequest(text_to_keep));
}

// NB: This tests two different things:
//   1) That the system replaces the value for "Proxy-Connection" headers.
//   2) That when it replaces a header, it also removes the text in the line
//      continuation.
TEST_F(HTTPProxyTest, LineContinuationRemoval) {
  SetupClient();
  ExpectRouteRequest();
  ExpectDNSRequest("www.chromium.org", true);
  string text_to_remove("remove this text please");
  ReadFromClient(CreateRequest("http://www.chromium.org/", "1.1",
                               string("Proxy-Connection: stuff\r\n\t") +
                               text_to_remove));
  EXPECT_EQ(string::npos, FindInRequest(text_to_remove));
  EXPECT_NE(string::npos, FindInRequest("Proxy-Connection: close\r\n"));
}

TEST_F(HTTPProxyTest, ConnectSynchronousFailure) {
  SetupClient();
  ExpectAsyncConnect(kServerAddress, kServerPort, false);
  ExpectClientResult();
  SetupConnect();
  ExpectClientError(500, "Could not create socket to connect to server");
}

TEST_F(HTTPProxyTest, ConnectAsyncConnectFailure) {
  SetupConnectAsync();
  ExpectClientResult();
  OnConnectCompletion(false, -1);
  ExpectClientError(500, "Socket connection delayed failure");
}

TEST_F(HTTPProxyTest, ConnectSynchronousSuccess) {
  SetupClient();
  ExpectSyncConnect(kServerAddress, 999);
  ExpectRepeatedServerOutput();
  SetupConnectWithRequest("/", "1.1", "Host: www.chromium.org:999");
  EXPECT_EQ(HTTPProxy::kStateTunnelData, GetProxyState());
}

TEST_F(HTTPProxyTest, ConnectIPAddresss) {
  SetupClient();
  ExpectSyncConnect(kServerAddress, 999);
  ExpectRepeatedServerOutput();
  ExpectRouteRequest();
  ReadFromClient(CreateRequest("/", "1.1",
                               StringPrintf("Host: %s:999", kServerAddress)));
  EXPECT_EQ(HTTPProxy::kStateTunnelData, GetProxyState());
}

TEST_F(HTTPProxyTest, ConnectAsyncConnectSuccess) {
  SetupConnectComplete();
}

TEST_F(HTTPProxyTest, HTTPConnectMethod) {
  SetupClient();
  ExpectAsyncConnect(kServerAddress, kConnectPort, true);
  ExpectConnectTimeout();
  ExpectRouteRequest();
  ReadFromClient(kConnectQuery);
  ExpectRepeatedInputTimeout();
  ExpectClientData();
  OnConnectCompletion(true, kServerFD);
  ExpectInClientResponse("HTTP/1.1 200 OK\r\n\r\n");
}

TEST_F(HTTPProxyTest, TunnelData) {
  SetupConnectComplete();

  // The proxy is waiting for the server to be ready to accept data.
  EXPECT_CALL(sockets(), Send(kServerFD, _, _, 0))
      .WillOnce(Return(10));
  ExpectServerInput();
  WriteToServer(kServerFD);
  EXPECT_CALL(sockets(), Send(kServerFD, _, _, 0))
      .WillOnce(ReturnArg<2>());
  ExpectInputTimeout();
  WriteToServer(kServerFD);
  EXPECT_EQ(HTTPProxy::kStateTunnelData, GetProxyState());

  // Tunnel a reply back to the client.
  const string server_result("200 OK ... and so on");
  ExpectClientResult();
  ReadFromServer(server_result);
  EXPECT_EQ(server_result,
            string(reinterpret_cast<const char*>(
                GetServerData().GetConstData()),
                   GetServerData().GetLength()));

  // Allow part of the result string to be sent to the client.
  const int part = server_result.length() / 2;
  EXPECT_CALL(sockets(), Send(kClientFD, _, server_result.length(), 0))
      .WillOnce(Return(part));
  ExpectInputTimeout();
  WriteToClient(kClientFD);
  EXPECT_EQ(HTTPProxy::kStateTunnelData, GetProxyState());

  // The Server closes the connection while the client is still reading.
  ExpectInputTimeout();
  ReadFromServer("");
  EXPECT_EQ(HTTPProxy::kStateFlushResponse, GetProxyState());

  // When the last part of the response is written to the client, we close
  // all connections.
  EXPECT_CALL(sockets(), Send(kClientFD, _, server_result.length() - part, 0))
      .WillOnce(ReturnArg<2>());
  ExpectTunnelClose();
  WriteToClient(kClientFD);
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, TunnelDataFailWriteClient) {
  SetupConnectComplete();
  EXPECT_CALL(sockets(), Send(kClientFD, _, _, 0))
      .WillOnce(Return(-1));
  ExpectTunnelClose();
  WriteToClient(kClientFD);
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, TunnelDataFailWriteServer) {
  SetupConnectComplete();
  EXPECT_CALL(sockets(), Send(kServerFD, _, _, 0))
      .WillOnce(Return(-1));
  ExpectTunnelClose();
  WriteToServer(kServerFD);
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, TunnelDataFailReadServer) {
  SetupConnectComplete();
  EXPECT_CALL(sockets(), Send(kServerFD, _, _, 0))
      .WillOnce(Return(10));
  ExpectServerInput();
  WriteToServer(kServerFD);
  ExpectTunnelClose();
  CauseReadError();
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, TunnelDataFailClientClose) {
  SetupConnectComplete();
  ExpectTunnelClose();
  ReadFromClient("");
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, TunnelDataFailServerClose) {
  SetupConnectComplete();
  ExpectTunnelClose();
  ReadFromServer("");
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

TEST_F(HTTPProxyTest, StopClient) {
  SetupConnectComplete();
  EXPECT_CALL(sockets(), Close(kClientFD))
      .WillOnce(Return(0));
  EXPECT_CALL(sockets(), Close(kServerFD))
      .WillOnce(Return(0));
  ExpectRouteRelease();
  StopClient();
  ExpectClientReset();
  EXPECT_EQ(HTTPProxy::kStateWaitConnection, GetProxyState());
}

}  // namespace shill
