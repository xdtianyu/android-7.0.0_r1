//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/connection_health_checker.h"

#include <arpa/inet.h>

#include <memory>
#include <string>
#include <vector>

#include <base/bind.h>
#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/scoped_vector.h>
#include <gtest/gtest.h>

#include "shill/mock_async_connection.h"
#include "shill/mock_connection.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_dns_client.h"
#include "shill/mock_dns_client_factory.h"
#include "shill/mock_ip_address_store.h"
#include "shill/mock_socket_info_reader.h"
#include "shill/net/mock_sockets.h"
#include "shill/test_event_dispatcher.h"

using base::Bind;
using base::Callback;
using base::Closure;
using base::Unretained;
using std::string;
using std::vector;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Gt;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::Sequence;
using ::testing::SetArgumentPointee;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::_;

namespace shill {

namespace {
const char kInterfaceName[] = "int0";
const char kIPAddress_8_8_8_8[] = "8.8.8.8";
const char kProxyIPAddressRemote[] = "74.125.224.84";
const char kProxyIPAddressLocal[] = "192.23.34.1";
const char kProxyIPv6AddressLocal[] = "::ffff:192.23.34.1";
const char kProxyURLRemote[] = "http://www.google.com";
const int kProxyFD = 100;
const int16_t kProxyPortLocal = 5540;
const int16_t kProxyPortRemote = 80;
}  // namespace

MATCHER_P(IsSameIPAddress, ip_addr, "") {
  return arg.Equals(ip_addr);
}

class ConnectionHealthCheckerTest : public Test {
 public:
  ConnectionHealthCheckerTest()
      : interface_name_(kInterfaceName),
        device_info_(&control_, &dispatcher_, nullptr, nullptr),
        connection_(new NiceMock<MockConnection>(&device_info_)),
        socket_(nullptr) {}

  // Invokes
  int GetSockName(int fd, struct sockaddr* addr_out, socklen_t* sockaddr_size) {
    struct sockaddr_in addr;
    EXPECT_EQ(kProxyFD, fd);
    EXPECT_LE(sizeof(sockaddr_in), *sockaddr_size);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, kProxyIPAddressLocal, &addr.sin_addr);
    addr.sin_port = htons(kProxyPortLocal);
    memcpy(addr_out, &addr, sizeof(addr));
    *sockaddr_size = sizeof(sockaddr_in);
    return 0;
  }

  int GetSockNameReturnsIPv6(int fd, struct sockaddr* addr_out,
                             socklen_t* sockaddr_size) {
    struct sockaddr_in6 addr;
    EXPECT_EQ(kProxyFD, fd);
    EXPECT_LE(sizeof(sockaddr_in6), *sockaddr_size);
    addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, kProxyIPv6AddressLocal, &addr.sin6_addr);
    addr.sin6_port = htons(kProxyPortLocal);
    memcpy(addr_out, &addr, sizeof(addr));
    *sockaddr_size = sizeof(sockaddr_in6);
    return 0;
  }

  void InvokeOnConnectionComplete(bool success, int sock_fd) {
    health_checker_->OnConnectionComplete(success, sock_fd);
  }

  void InvokeGetDNSResultFailure() {
    Error error(Error::kOperationFailed, "");
    IPAddress address(IPAddress::kFamilyUnknown);
    health_checker_->GetDNSResult(error, address);
  }

  void InvokeGetDNSResultSuccess(const IPAddress& address) {
    Error error;
    health_checker_->GetDNSResult(error, address);
  }

 protected:
  void SetUp() {
    EXPECT_CALL(*connection_.get(), interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
    ON_CALL(*connection_.get(), dns_servers())
        .WillByDefault(ReturnRef(dns_servers_));
    // ConnectionHealthChecker constructor should add some IPs
    EXPECT_CALL(remote_ips_, AddUnique(_)).Times(AtLeast(1));
    health_checker_.reset(
        new ConnectionHealthChecker(
             connection_,
             &dispatcher_,
             &remote_ips_,
             Bind(&ConnectionHealthCheckerTest::ResultCallbackTarget,
                  Unretained(this))));
    Mock::VerifyAndClearExpectations(&remote_ips_);
    socket_ = new StrictMock<MockSockets>();
    tcp_connection_ = new StrictMock<MockAsyncConnection>();
    socket_info_reader_ = new StrictMock<MockSocketInfoReader>();
    // Passes ownership for all of these.
    health_checker_->socket_.reset(socket_);
    health_checker_->tcp_connection_.reset(tcp_connection_);
    health_checker_->socket_info_reader_.reset(socket_info_reader_);
    health_checker_->dns_client_factory_ = MockDNSClientFactory::GetInstance();
  }

  void TearDown() {
    ExpectStop();
  }

  // Accessors for private data in ConnectionHealthChecker.
  const Sockets* socket() {
    return health_checker_->socket_.get();
  }
  const AsyncConnection* tcp_connection() {
    return health_checker_->tcp_connection_.get();
  }
  ScopedVector<DNSClient>& dns_clients() {
    return health_checker_->dns_clients_;
  }
  int NumDNSQueries() {
    return ConnectionHealthChecker::kNumDNSQueries;
  }
  int MaxFailedConnectionAttempts() {
    return ConnectionHealthChecker::kMaxFailedConnectionAttempts;
  }
  int MaxSentDataPollingAttempts() {
    return ConnectionHealthChecker::kMaxSentDataPollingAttempts;
  }
  int MinCongestedQueueAttempts() {
    return ConnectionHealthChecker::kMinCongestedQueueAttempts;
  }
  int MinSuccessfulSendAttempts() {
    return ConnectionHealthChecker::kMinSuccessfulSendAttempts;
  }
  void SetTCPStateUpdateWaitMilliseconds(int new_wait) {
    health_checker_->tcp_state_update_wait_milliseconds_ = new_wait;
  }

  // Mock Callbacks
  MOCK_METHOD1(ResultCallbackTarget,
               void(ConnectionHealthChecker::Result result));

  // Helper methods
  IPAddress StringToIPv4Address(const string& address_string) {
    IPAddress ip_address(IPAddress::kFamilyIPv4);
    EXPECT_TRUE(ip_address.SetAddressFromString(address_string));
    return ip_address;
  }
  // Naming: CreateSocketInfo
  //         + (Proxy/Other) : TCP connection for proxy socket / some other
  //         socket.
  //         + arg1: Pass in any SocketInfo::ConnectionState you want.
  //         + arg2: Pass in any value of transmit_queue_value you want.
  SocketInfo CreateSocketInfoOther() {
    return SocketInfo(
        SocketInfo::kConnectionStateUnknown,
        StringToIPv4Address(kIPAddress_8_8_8_8),
        0,
        StringToIPv4Address(kProxyIPAddressRemote),
        kProxyPortRemote,
        0,
        0,
        SocketInfo::kTimerStateUnknown);
  }
  SocketInfo CreateSocketInfoProxy(SocketInfo::ConnectionState state) {
    return SocketInfo(
        state,
        StringToIPv4Address(kProxyIPAddressLocal),
        kProxyPortLocal,
        StringToIPv4Address(kProxyIPAddressRemote),
        kProxyPortRemote,
        0,
        0,
        SocketInfo::kTimerStateUnknown);
  }
  SocketInfo CreateSocketInfoProxy(SocketInfo::ConnectionState state,
                                   SocketInfo::TimerState timer_state,
                                   uint64_t transmit_queue_value) {
    return SocketInfo(
        state,
        StringToIPv4Address(kProxyIPAddressLocal),
        kProxyPortLocal,
        StringToIPv4Address(kProxyIPAddressRemote),
        kProxyPortRemote,
        transmit_queue_value,
        0,
        timer_state);
  }


  // Expectations
  void ExpectReset() {
    EXPECT_EQ(connection_.get(), health_checker_->connection_.get());
    EXPECT_EQ(&dispatcher_, health_checker_->dispatcher_);
    EXPECT_EQ(socket_, health_checker_->socket_.get());
    EXPECT_FALSE(socket_ == nullptr);
    EXPECT_EQ(socket_info_reader_, health_checker_->socket_info_reader_.get());
    EXPECT_FALSE(socket_info_reader_ == nullptr);
    EXPECT_FALSE(health_checker_->connection_complete_callback_.is_null());
    EXPECT_EQ(tcp_connection_, health_checker_->tcp_connection_.get());
    EXPECT_FALSE(tcp_connection_ == nullptr);
    EXPECT_FALSE(health_checker_->health_check_in_progress_);
  }

  // Setup ConnectionHealthChecker::GetSocketInfo to return sock_info.
  // This only works if GetSocketInfo is called with kProxyFD.
  // If no matching sock_info is provided (Does not belong to proxy socket),
  // GetSocketInfo will (correctly) return false.
  void ExpectGetSocketInfoReturns(SocketInfo sock_info) {
    vector<SocketInfo> info_list;
    info_list.push_back(sock_info);
    EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
        .InSequence(seq_)
        .WillOnce(Invoke(this,
                         &ConnectionHealthCheckerTest::GetSockName));
    EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
        .InSequence(seq_)
        .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                        Return(true)));
  }
  void ExpectSuccessfulStart() {
    EXPECT_CALL(remote_ips_, Empty()).WillRepeatedly(Return(false));
    EXPECT_CALL(remote_ips_, GetRandomIP())
        .WillRepeatedly(Return(StringToIPv4Address(kProxyIPAddressRemote)));
    EXPECT_CALL(
        *tcp_connection_,
        Start(IsSameIPAddress(StringToIPv4Address(kProxyIPAddressRemote)),
              kProxyPortRemote))
        .InSequence(seq_)
        .WillOnce(Return(true));
  }
  void ExpectRetry() {
    EXPECT_CALL(*socket_, Close(kProxyFD))
        .InSequence(seq_);
    EXPECT_CALL(
        *tcp_connection_,
        Start(IsSameIPAddress(StringToIPv4Address(kProxyIPAddressRemote)),
              kProxyPortRemote))
        .InSequence(seq_)
        .WillOnce(Return(true));
  }
  void ExpectStop() {
    if (tcp_connection_)
      EXPECT_CALL(*tcp_connection_, Stop())
          .InSequence(seq_);
  }
  void ExpectCleanUp() {
    EXPECT_CALL(*socket_, Close(kProxyFD))
        .InSequence(seq_);
    EXPECT_CALL(*tcp_connection_, Stop())
        .InSequence(seq_);
  }

  void VerifyAndClearAllExpectations() {
    Mock::VerifyAndClearExpectations(this);
    Mock::VerifyAndClearExpectations(tcp_connection_);
    Mock::VerifyAndClearExpectations(socket_);
    Mock::VerifyAndClearExpectations(socket_info_reader_);
  }

  // Needed for other mocks, but not for the tests directly.
  const string interface_name_;
  NiceMock<MockControl> control_;
  NiceMock<MockDeviceInfo> device_info_;
  vector<string> dns_servers_;

  scoped_refptr<NiceMock<MockConnection>> connection_;
  EventDispatcherForTest dispatcher_;
  MockIPAddressStore remote_ips_;
  StrictMock<MockSockets>* socket_;
  StrictMock<MockSocketInfoReader>* socket_info_reader_;
  StrictMock<MockAsyncConnection>* tcp_connection_;
  // Expectations in the Expect* functions are put in this sequence.
  // This allows us to chain calls to Expect* functions.
  Sequence seq_;

  std::unique_ptr<ConnectionHealthChecker> health_checker_;
};

TEST_F(ConnectionHealthCheckerTest, Constructor) {
  ExpectReset();
}

TEST_F(ConnectionHealthCheckerTest, SetConnection) {
  scoped_refptr<NiceMock<MockConnection>> new_connection =
      new NiceMock<MockConnection>(&device_info_);
  // If a health check was in progress when SetConnection is called, verify
  // that it restarts with the new connection.
  ExpectSuccessfulStart();
  health_checker_->Start();
  VerifyAndClearAllExpectations();

  EXPECT_CALL(remote_ips_, Empty()).WillRepeatedly(Return(true));
  EXPECT_CALL(*new_connection.get(), interface_name())
      .WillRepeatedly(ReturnRef(interface_name_));
  EXPECT_CALL(*this,
              ResultCallbackTarget(ConnectionHealthChecker::kResultUnknown));
  health_checker_->SetConnection(new_connection);
  EXPECT_NE(tcp_connection_, health_checker_->tcp_connection());
  EXPECT_EQ(new_connection.get(), health_checker_->connection());

  // health_checker_ has reset tcp_connection_ to a new object.
  // Since it owned tcp_connection_, the object has been destroyed.
  tcp_connection_ = nullptr;
}

TEST_F(ConnectionHealthCheckerTest, GarbageCollectDNSClients) {
  dns_clients().clear();
  health_checker_->GarbageCollectDNSClients();
  EXPECT_TRUE(dns_clients().empty());

  for (int i = 0; i < 3; ++i) {
    MockDNSClient* dns_client = new MockDNSClient();
    EXPECT_CALL(*dns_client, IsActive())
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    // Takes ownership.
    dns_clients().push_back(dns_client);
  }
  for (int i = 0; i < 2; ++i) {
    MockDNSClient* dns_client = new MockDNSClient();
    EXPECT_CALL(*dns_client, IsActive())
        .WillOnce(Return(false));
    // Takes ownership.
    dns_clients().push_back(dns_client);
  }

  EXPECT_EQ(5, dns_clients().size());
  health_checker_->GarbageCollectDNSClients();
  EXPECT_EQ(3, dns_clients().size());
  health_checker_->GarbageCollectDNSClients();
  EXPECT_EQ(3, dns_clients().size());
  health_checker_->GarbageCollectDNSClients();
  EXPECT_TRUE(dns_clients().empty());
}

TEST_F(ConnectionHealthCheckerTest, AddRemoteURL) {
  HTTPURL url;
  url.ParseFromString(kProxyURLRemote);
  string host = url.host();
  IPAddress remote_ip = StringToIPv4Address(kProxyIPAddressRemote);
  IPAddress remote_ip_2 = StringToIPv4Address(kIPAddress_8_8_8_8);

  MockDNSClientFactory* dns_client_factory
      = MockDNSClientFactory::GetInstance();
  vector<MockDNSClient*> dns_client_buffer;

  // All DNS queries fail.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    MockDNSClient* dns_client = new MockDNSClient();
    EXPECT_CALL(*dns_client, Start(host, _))
        .WillOnce(Return(false));
    dns_client_buffer.push_back(dns_client);
  }
  // Will pass ownership of dns_clients elements.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    EXPECT_CALL(*dns_client_factory, CreateDNSClient(_, _, _, _, _, _))
        .InSequence(seq_)
        .WillOnce(Return(dns_client_buffer[i]));
  }
  EXPECT_CALL(remote_ips_, AddUnique(_)).Times(0);
  health_checker_->AddRemoteURL(kProxyURLRemote);
  Mock::VerifyAndClearExpectations(dns_client_factory);
  Mock::VerifyAndClearExpectations(&remote_ips_);
  dns_client_buffer.clear();
  dns_clients().clear();

  // All but one DNS queries fail, 1 succeeds.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    MockDNSClient* dns_client = new MockDNSClient();
    EXPECT_CALL(*dns_client, Start(host, _))
        .WillOnce(Return(true));
    dns_client_buffer.push_back(dns_client);
  }
  // Will pass ownership of dns_clients elements.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    EXPECT_CALL(*dns_client_factory, CreateDNSClient(_, _, _, _, _, _))
        .InSequence(seq_)
        .WillOnce(Return(dns_client_buffer[i]));
  }
  EXPECT_CALL(remote_ips_, AddUnique(_));
  health_checker_->AddRemoteURL(kProxyURLRemote);
  for (int i = 0; i < NumDNSQueries() - 1; ++i) {
    InvokeGetDNSResultFailure();
  }
  InvokeGetDNSResultSuccess(remote_ip);
  Mock::VerifyAndClearExpectations(dns_client_factory);
  Mock::VerifyAndClearExpectations(&remote_ips_);
  dns_client_buffer.clear();
  dns_clients().clear();

  // Only 2 distinct IP addresses are returned.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    MockDNSClient* dns_client = new MockDNSClient();
    EXPECT_CALL(*dns_client, Start(host, _))
        .WillOnce(Return(true));
    dns_client_buffer.push_back(dns_client);
  }
  // Will pass ownership of dns_clients elements.
  for (int i = 0; i < NumDNSQueries(); ++i) {
    EXPECT_CALL(*dns_client_factory, CreateDNSClient(_, _, _, _, _, _))
        .InSequence(seq_)
        .WillOnce(Return(dns_client_buffer[i]));
  }
  EXPECT_CALL(remote_ips_, AddUnique(IsSameIPAddress(remote_ip))).Times(4);
  EXPECT_CALL(remote_ips_, AddUnique(IsSameIPAddress(remote_ip_2)));
  health_checker_->AddRemoteURL(kProxyURLRemote);
  for (int i = 0; i < NumDNSQueries() - 1; ++i) {
    InvokeGetDNSResultSuccess(remote_ip);
  }
  InvokeGetDNSResultSuccess(remote_ip_2);
  Mock::VerifyAndClearExpectations(dns_client_factory);
  Mock::VerifyAndClearExpectations(&remote_ips_);
  dns_client_buffer.clear();
  dns_clients().clear();
}

TEST_F(ConnectionHealthCheckerTest, GetSocketInfo) {
  SocketInfo sock_info;
  vector<SocketInfo> info_list;

  // GetSockName fails.
  EXPECT_CALL(*socket_, GetSockName(_, _, _))
      .WillOnce(Return(-1));
  EXPECT_FALSE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  Mock::VerifyAndClearExpectations(socket_);

  // GetSockName returns IPv6.
  EXPECT_CALL(*socket_, GetSockName(_, _, _))
      .WillOnce(
          Invoke(this,
                 &ConnectionHealthCheckerTest::GetSockNameReturnsIPv6));
  EXPECT_FALSE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  Mock::VerifyAndClearExpectations(socket_);

  // LoadTcpSocketInfo fails.
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
    .WillOnce(Return(false));
  EXPECT_FALSE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);

  // LoadTcpSocketInfo returns empty list.
  info_list.clear();
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                      Return(true)));
  EXPECT_FALSE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);

  // LoadTcpSocketInfo returns a list without our socket.
  info_list.clear();
  info_list.push_back(CreateSocketInfoOther());
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                      Return(true)));
  EXPECT_FALSE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);

  // LoadTcpSocketInfo returns a list with only our socket.
  info_list.clear();
  info_list.push_back(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown));
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                      Return(true)));
  EXPECT_TRUE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  EXPECT_TRUE(CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown)
                  .IsSameSocketAs(sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);

  // LoadTcpSocketInfo returns a list with two sockets, including ours.
  info_list.clear();
  info_list.push_back(CreateSocketInfoOther());
  info_list.push_back(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown));
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                      Return(true)));
  EXPECT_TRUE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  EXPECT_TRUE(CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown)
                  .IsSameSocketAs(sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);

  info_list.clear();
  info_list.push_back(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown));
  info_list.push_back(CreateSocketInfoOther());
  EXPECT_CALL(*socket_, GetSockName(kProxyFD, _, _))
      .WillOnce(Invoke(this, &ConnectionHealthCheckerTest::GetSockName));
  EXPECT_CALL(*socket_info_reader_, LoadTcpSocketInfo(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(info_list),
                      Return(true)));
  EXPECT_TRUE(health_checker_->GetSocketInfo(kProxyFD, &sock_info));
  EXPECT_TRUE(CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown)
                  .IsSameSocketAs(sock_info));
  Mock::VerifyAndClearExpectations(socket_);
  Mock::VerifyAndClearExpectations(socket_info_reader_);
}

TEST_F(ConnectionHealthCheckerTest, NextHealthCheckSample) {
  IPAddress ip = StringToIPv4Address(kProxyIPAddressRemote);
  ON_CALL(remote_ips_, GetRandomIP())
      .WillByDefault(Return(ip));

  health_checker_->set_num_connection_failures(MaxFailedConnectionAttempts());
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->NextHealthCheckSample();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  health_checker_->set_num_congested_queue_detected(
      MinCongestedQueueAttempts());
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultCongestedTxQueue));
  health_checker_->NextHealthCheckSample();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  health_checker_->set_num_successful_sends(MinSuccessfulSendAttempts());
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultSuccess));
  health_checker_->NextHealthCheckSample();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  EXPECT_CALL(*tcp_connection_, Start(_, _)).WillOnce(Return(true));
  health_checker_->NextHealthCheckSample();
  VerifyAndClearAllExpectations();

  // This test assumes that there are at least 2 connection attempts left
  // before ConnectionHealthChecker gives up.
  EXPECT_CALL(*tcp_connection_, Start(_, _))
    .WillOnce(Return(false))
    .WillOnce(Return(true));
  int16_t num_connection_failures = health_checker_->num_connection_failures();
  health_checker_->NextHealthCheckSample();
  EXPECT_EQ(num_connection_failures + 1,
            health_checker_->num_connection_failures());
}

TEST_F(ConnectionHealthCheckerTest, OnConnectionComplete) {
  // Test that num_connection_attempts is incremented on failure when
  // (1) Async Connection fails.
  health_checker_->set_num_connection_failures(
      MaxFailedConnectionAttempts() - 1);
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->OnConnectionComplete(false, -1);
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  // (2) The connection state is garbled up.
  health_checker_->set_num_connection_failures(
      MaxFailedConnectionAttempts() - 1);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->OnConnectionComplete(true, kProxyFD);
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  // (3) Send fails.
  health_checker_->set_num_connection_failures(
      MaxFailedConnectionAttempts() - 1);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateEstablished));
  EXPECT_CALL(*socket_, Send(kProxyFD, _, Gt(0), _)).WillOnce(Return(-1));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->OnConnectionComplete(true, kProxyFD);
  dispatcher_.DispatchPendingEvents();
}

TEST_F(ConnectionHealthCheckerTest, VerifySentData) {
  // (1) Test that num_connection_attempts is incremented when the connection
  // state is garbled up.
  health_checker_->set_num_connection_failures(
      MaxFailedConnectionAttempts() - 1);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateUnknown));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->set_sock_fd(kProxyFD);
  health_checker_->VerifySentData();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  // (2) Test that num_congested_queue_detected is incremented when all polling
  // attempts have expired.
  health_checker_->set_num_congested_queue_detected(
      MinCongestedQueueAttempts() - 1);
  health_checker_->set_num_tx_queue_polling_attempts(
      MaxSentDataPollingAttempts());
  health_checker_->set_old_transmit_queue_value(0);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateEstablished,
                            SocketInfo::kTimerStateRetransmitTimerPending,
                            1));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultCongestedTxQueue));
  health_checker_->set_sock_fd(kProxyFD);
  health_checker_->VerifySentData();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  // (3) Test that num_successful_sends is incremented if everything goes fine.
  health_checker_->set_num_successful_sends(MinSuccessfulSendAttempts() - 1);
  health_checker_->set_old_transmit_queue_value(0);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateEstablished,
                            SocketInfo::kTimerStateNoTimerPending,
                            0));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this,
      ResultCallbackTarget(ConnectionHealthChecker::kResultSuccess));
  health_checker_->set_sock_fd(kProxyFD);
  health_checker_->VerifySentData();
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();

  // (4) Test that VerifySentData correctly polls the tcpinfo twice.
  // We want to immediately dispatch posted tasks.
  SetTCPStateUpdateWaitMilliseconds(0);
  health_checker_->set_num_congested_queue_detected(
      MinCongestedQueueAttempts() - 1);
  health_checker_->set_num_tx_queue_polling_attempts(
      MaxSentDataPollingAttempts() - 1);
  health_checker_->set_old_transmit_queue_value(0);
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateEstablished,
                            SocketInfo::kTimerStateRetransmitTimerPending,
                            1));
  ExpectGetSocketInfoReturns(
      CreateSocketInfoProxy(SocketInfo::kConnectionStateEstablished,
                            SocketInfo::kTimerStateRetransmitTimerPending,
                            1));
  EXPECT_CALL(*socket_, Close(kProxyFD));
  ExpectStop();
  EXPECT_CALL(
      *this, ResultCallbackTarget(
          ConnectionHealthChecker::kResultCongestedTxQueue))
      .InSequence(seq_);
  health_checker_->set_sock_fd(kProxyFD);
  health_checker_->VerifySentData();
  dispatcher_.DispatchPendingEvents();
  dispatcher_.DispatchPendingEvents();
  // Force an extra dispatch to make sure that VerifySentData did not poll an
  // extra time. This dispatch should be a no-op.
  dispatcher_.DispatchPendingEvents();
  VerifyAndClearAllExpectations();
}

// Flow: Start() -> Start()
// Expectation: Only one AsyncConnection is setup
TEST_F(ConnectionHealthCheckerTest, StartStartSkipsSecond) {
  EXPECT_CALL(*tcp_connection_, Start(_, _))
      .WillOnce(Return(true));
  EXPECT_CALL(remote_ips_, Empty()).WillRepeatedly(Return(false));
  EXPECT_CALL(remote_ips_, GetRandomIP())
      .WillOnce(Return(StringToIPv4Address(kProxyIPAddressRemote)));
  health_checker_->Start();
  health_checker_->Start();
}

// Precondition: size(|remote_ips_|) > 0
// Flow: Start() -> Stop() before ConnectionComplete()
// Expectation: No call to |result_callback|
TEST_F(ConnectionHealthCheckerTest, StartStopNoCallback) {
  EXPECT_CALL(*tcp_connection_, Start(_, _))
      .WillOnce(Return(true));
  EXPECT_CALL(*tcp_connection_, Stop());
  EXPECT_CALL(*this, ResultCallbackTarget(_))
      .Times(0);
  EXPECT_CALL(remote_ips_, Empty()).WillRepeatedly(Return(false));
  EXPECT_CALL(remote_ips_, GetRandomIP())
      .WillOnce(Return(StringToIPv4Address(kProxyIPAddressRemote)));
  health_checker_->Start();
  health_checker_->Stop();
}

// Precondition: Empty remote_ips_
// Flow: Start()
// Expectation: call |result_callback| with kResultUnknown
TEST_F(ConnectionHealthCheckerTest, StartImmediateFailure) {
  EXPECT_CALL(remote_ips_, Empty()).WillOnce(Return(true));
  EXPECT_CALL(*tcp_connection_, Stop());
  EXPECT_CALL(*this, ResultCallbackTarget(
                           ConnectionHealthChecker::kResultUnknown));
  health_checker_->Start();
  Mock::VerifyAndClearExpectations(this);
  Mock::VerifyAndClearExpectations(&remote_ips_);
  Mock::VerifyAndClearExpectations(tcp_connection_);

  EXPECT_CALL(remote_ips_, Empty()).WillRepeatedly(Return(false));
  EXPECT_CALL(remote_ips_, GetRandomIP())
      .WillRepeatedly(Return(StringToIPv4Address(kProxyIPAddressRemote)));
  EXPECT_CALL(*tcp_connection_,
              Start(IsSameIPAddress(StringToIPv4Address(kProxyIPAddressRemote)),
                    kProxyPortRemote))
      .Times(MaxFailedConnectionAttempts())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*tcp_connection_, Stop());
  EXPECT_CALL(*this, ResultCallbackTarget(
                           ConnectionHealthChecker::kResultConnectionFailure));
  health_checker_->Start();
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(this);
  Mock::VerifyAndClearExpectations(tcp_connection_);
}

}  // namespace shill
