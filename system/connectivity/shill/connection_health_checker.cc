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
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include <vector>

#include <base/bind.h>

#include "shill/async_connection.h"
#include "shill/connection.h"
#include "shill/dns_client.h"
#include "shill/dns_client_factory.h"
#include "shill/error.h"
#include "shill/http_url.h"
#include "shill/ip_address_store.h"
#include "shill/logging.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"
#include "shill/socket_info.h"
#include "shill/socket_info_reader.h"

using base::Bind;
using base::Unretained;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kConnection;
static string ObjectID(Connection* c) {
  return c->interface_name();
}
}

// static
const char* ConnectionHealthChecker::kDefaultRemoteIPPool[] = {
    "74.125.224.47",
    "74.125.224.79",
    "74.125.224.111",
    "74.125.224.143"
};
// static
const int ConnectionHealthChecker::kDNSTimeoutMilliseconds = 5000;
// static
const int ConnectionHealthChecker::kInvalidSocket = -1;
// static
const int ConnectionHealthChecker::kMaxFailedConnectionAttempts = 2;
// static
const int ConnectionHealthChecker::kMaxSentDataPollingAttempts = 2;
// static
const int ConnectionHealthChecker::kMinCongestedQueueAttempts = 2;
// static
const int ConnectionHealthChecker::kMinSuccessfulSendAttempts = 1;
// static
const int ConnectionHealthChecker::kNumDNSQueries = 5;
// static
const int ConnectionHealthChecker::kTCPStateUpdateWaitMilliseconds = 5000;
// static
const uint16_t ConnectionHealthChecker::kRemotePort = 80;

ConnectionHealthChecker::ConnectionHealthChecker(
    ConnectionRefPtr connection,
    EventDispatcher* dispatcher,
    IPAddressStore* remote_ips,
    const base::Callback<void(Result)>& result_callback)
    : connection_(connection),
      dispatcher_(dispatcher),
      remote_ips_(remote_ips),
      result_callback_(result_callback),
      socket_(new Sockets()),
      weak_ptr_factory_(this),
      connection_complete_callback_(
          Bind(&ConnectionHealthChecker::OnConnectionComplete,
               weak_ptr_factory_.GetWeakPtr())),
      tcp_connection_(new AsyncConnection(connection_->interface_name(),
                                          dispatcher_,
                                          socket_.get(),
                                          connection_complete_callback_)),
      report_result_(
          Bind(&ConnectionHealthChecker::ReportResult,
               weak_ptr_factory_.GetWeakPtr())),
      sock_fd_(kInvalidSocket),
      socket_info_reader_(new SocketInfoReader()),
      dns_client_factory_(DNSClientFactory::GetInstance()),
      dns_client_callback_(Bind(&ConnectionHealthChecker::GetDNSResult,
                                weak_ptr_factory_.GetWeakPtr())),
      health_check_in_progress_(false),
      num_connection_failures_(0),
      num_congested_queue_detected_(0),
      num_successful_sends_(0),
      tcp_state_update_wait_milliseconds_(kTCPStateUpdateWaitMilliseconds) {
  for (size_t i = 0; i < arraysize(kDefaultRemoteIPPool); ++i) {
    const char* ip_string = kDefaultRemoteIPPool[i];
    IPAddress ip(IPAddress::kFamilyIPv4);
    ip.SetAddressFromString(ip_string);
    remote_ips_->AddUnique(ip);
  }
}

ConnectionHealthChecker::~ConnectionHealthChecker() {
  Stop();
}

bool ConnectionHealthChecker::health_check_in_progress() const {
  return health_check_in_progress_;
}

void ConnectionHealthChecker::AddRemoteIP(IPAddress ip) {
  remote_ips_->AddUnique(ip);
}

void ConnectionHealthChecker::AddRemoteURL(const string& url_string) {
  GarbageCollectDNSClients();

  HTTPURL url;
  if (!url.ParseFromString(url_string)) {
    SLOG(connection_.get(), 2) << __func__ << ": Malformed url: "
                               << url_string << ".";
    return;
  }
  if (url.port() != kRemotePort) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Remote connections only supported "
                               << " to port 80, requested " << url.port()
                               << ".";
    return;
  }
  for (int i = 0; i < kNumDNSQueries; ++i) {
    Error error;
    DNSClient* dns_client =
      dns_client_factory_->CreateDNSClient(IPAddress::kFamilyIPv4,
                                           connection_->interface_name(),
                                           connection_->dns_servers(),
                                           kDNSTimeoutMilliseconds,
                                           dispatcher_,
                                           dns_client_callback_);
    dns_clients_.push_back(dns_client);
    if (!dns_clients_[i]->Start(url.host(), &error)) {
      SLOG(connection_.get(), 2) << __func__ << ": Failed to start DNS client "
                                 << "(query #" << i << "): "
                                 << error.message();
    }
  }
}

void ConnectionHealthChecker::Start() {
  if (health_check_in_progress_) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Health Check already in progress.";
    return;
  }
  if (!connection_.get()) {
    SLOG(connection_.get(), 2) << __func__ << ": Connection not ready yet.";
    result_callback_.Run(kResultUnknown);
    return;
  }

  health_check_in_progress_ = true;
  num_connection_failures_ = 0;
  num_congested_queue_detected_ = 0;
  num_successful_sends_ = 0;

  if (remote_ips_->Empty()) {
    // Nothing to try.
    Stop();
    SLOG(connection_.get(), 2) << __func__ << ": Not enough IPs.";
    result_callback_.Run(kResultUnknown);
    return;
  }

  // Initiate the first attempt.
  NextHealthCheckSample();
}

void ConnectionHealthChecker::Stop() {
  if (tcp_connection_.get() != nullptr)
    tcp_connection_->Stop();
  verify_sent_data_callback_.Cancel();
  ClearSocketDescriptor();
  health_check_in_progress_ = false;
  num_connection_failures_ = 0;
  num_congested_queue_detected_ = 0;
  num_successful_sends_ = 0;
  num_tx_queue_polling_attempts_ = 0;
}

void ConnectionHealthChecker::SetConnection(ConnectionRefPtr connection) {
  SLOG(connection_.get(), 3) << __func__;
  connection_ = connection;
  tcp_connection_.reset(new AsyncConnection(connection_->interface_name(),
                                            dispatcher_,
                                            socket_.get(),
                                            connection_complete_callback_));
  dns_clients_.clear();
  bool restart = health_check_in_progress();
  Stop();
  if (restart)
    Start();
}

const char* ConnectionHealthChecker::ResultToString(
    ConnectionHealthChecker::Result result) {
  switch (result) {
    case kResultUnknown:
      return "Unknown";
    case kResultConnectionFailure:
      return "ConnectionFailure";
    case kResultCongestedTxQueue:
      return "CongestedTxQueue";
    case kResultSuccess:
      return "Success";
    default:
      return "Invalid";
  }
}

void ConnectionHealthChecker::GetDNSResult(const Error& error,
                                           const IPAddress& ip) {
  if (!error.IsSuccess()) {
    SLOG(connection_.get(), 2) << __func__ << "DNSClient returned failure: "
                               << error.message();
    return;
  }
  remote_ips_->AddUnique(ip);
}

void ConnectionHealthChecker::GarbageCollectDNSClients() {
  ScopedVector<DNSClient> keep;
  ScopedVector<DNSClient> discard;
  for (size_t i = 0; i < dns_clients_.size(); ++i) {
    if (dns_clients_[i]->IsActive())
      keep.push_back(dns_clients_[i]);
    else
      discard.push_back(dns_clients_[i]);
  }
  dns_clients_.weak_clear();
  dns_clients_ = std::move(keep);
  discard.clear();
}

void ConnectionHealthChecker::NextHealthCheckSample() {
  // Finish conditions:
  if (num_connection_failures_ == kMaxFailedConnectionAttempts) {
    health_check_result_ = kResultConnectionFailure;
    dispatcher_->PostTask(report_result_);
    return;
  }
  if (num_congested_queue_detected_ == kMinCongestedQueueAttempts) {
    health_check_result_ = kResultCongestedTxQueue;
    dispatcher_->PostTask(report_result_);
    return;
  }
  if (num_successful_sends_ == kMinSuccessfulSendAttempts) {
    health_check_result_ = kResultSuccess;
    dispatcher_->PostTask(report_result_);
    return;
  }

  // Pick a random IP from the set of IPs.
  // This guards against
  //   (1) Repeated failed attempts for the same IP at start-up everytime.
  //   (2) All users attempting to connect to the same IP.
  IPAddress ip = remote_ips_->GetRandomIP();
  SLOG(connection_.get(), 3) << __func__ << ": Starting connection at "
                             << ip.ToString();
  if (!tcp_connection_->Start(ip, kRemotePort)) {
    SLOG(connection_.get(), 2) << __func__ << ": Connection attempt failed.";
    ++num_connection_failures_;
    NextHealthCheckSample();
  }
}

void ConnectionHealthChecker::OnConnectionComplete(bool success, int sock_fd) {
  if (!success) {
    SLOG(connection_.get(), 2) << __func__
                               << ": AsyncConnection connection attempt failed "
                               << "with error: "
                               << tcp_connection_->error();
    ++num_connection_failures_;
    NextHealthCheckSample();
    return;
  }

  SetSocketDescriptor(sock_fd);

  SocketInfo sock_info;
  if (!GetSocketInfo(sock_fd_, &sock_info) ||
      sock_info.connection_state() !=
          SocketInfo::kConnectionStateEstablished) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Connection originally not in established "
                                  "state.";
    // Count this as a failed connection attempt.
    ++num_connection_failures_;
    ClearSocketDescriptor();
    NextHealthCheckSample();
    return;
  }

  old_transmit_queue_value_ = sock_info.transmit_queue_value();
  num_tx_queue_polling_attempts_ = 0;

  // Send data on the connection and post a delayed task to check successful
  // transfer.
  char buf;
  if (socket_->Send(sock_fd_, &buf, sizeof(buf), 0) == -1) {
    SLOG(connection_.get(), 2) << __func__ << ": " << socket_->ErrorString();
    // Count this as a failed connection attempt.
    ++num_connection_failures_;
    ClearSocketDescriptor();
    NextHealthCheckSample();
    return;
  }

  verify_sent_data_callback_.Reset(
      Bind(&ConnectionHealthChecker::VerifySentData, Unretained(this)));
  dispatcher_->PostDelayedTask(verify_sent_data_callback_.callback(),
                               tcp_state_update_wait_milliseconds_);
}

void ConnectionHealthChecker::VerifySentData() {
  SocketInfo sock_info;
  bool sock_info_found = GetSocketInfo(sock_fd_, &sock_info);
  // Acceptable TCP connection states after sending the data:
  // kConnectionStateEstablished: No change in connection state since the send.
  // kConnectionStateCloseWait: The remote host recieved the sent data and
  //    requested connection close.
  if (!sock_info_found ||
      (sock_info.connection_state() !=
           SocketInfo::kConnectionStateEstablished &&
      sock_info.connection_state() !=
           SocketInfo::kConnectionStateCloseWait)) {
    SLOG(connection_.get(), 2)
        << __func__ << ": Connection not in acceptable state after send.";
    if (sock_info_found)
      SLOG(connection_.get(), 3) << "Found socket info but in state: "
                                 << sock_info.connection_state();
    ++num_connection_failures_;
  } else if (sock_info.transmit_queue_value() > old_transmit_queue_value_ &&
      sock_info.timer_state() ==
          SocketInfo::kTimerStateRetransmitTimerPending) {
    if (num_tx_queue_polling_attempts_ < kMaxSentDataPollingAttempts) {
      SLOG(connection_.get(), 2) << __func__
                                 << ": Polling again.";
      ++num_tx_queue_polling_attempts_;
      verify_sent_data_callback_.Reset(
          Bind(&ConnectionHealthChecker::VerifySentData, Unretained(this)));
      dispatcher_->PostDelayedTask(verify_sent_data_callback_.callback(),
                                   tcp_state_update_wait_milliseconds_);
      return;
    }
    SLOG(connection_.get(), 2) << __func__ << ": Sampled congested Tx-Queue";
    ++num_congested_queue_detected_;
  } else {
    SLOG(connection_.get(), 2) << __func__ << ": Sampled successful send.";
    ++num_successful_sends_;
  }
  ClearSocketDescriptor();
  NextHealthCheckSample();
}

// TODO(pprabhu): Scrub IP address logging.
bool ConnectionHealthChecker::GetSocketInfo(int sock_fd,
                                            SocketInfo* sock_info) {
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  if (socket_->GetSockName(sock_fd,
                           reinterpret_cast<struct sockaddr*>(&addr),
                           &addrlen) != 0) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Failed to get address of created socket.";
    return false;
  }
  if (addr.ss_family != AF_INET) {
    SLOG(connection_.get(), 2) << __func__ << ": IPv6 socket address found.";
    return false;
  }

  CHECK_EQ(sizeof(struct sockaddr_in), addrlen);
  struct sockaddr_in* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
  uint16_t local_port = ntohs(addr_in->sin_port);
  char ipstr[INET_ADDRSTRLEN];
  const char* res = inet_ntop(AF_INET, &addr_in->sin_addr,
                              ipstr, sizeof(ipstr));
  if (res == nullptr) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Could not convert IP address to string.";
    return false;
  }

  IPAddress local_ip_address(IPAddress::kFamilyIPv4);
  CHECK(local_ip_address.SetAddressFromString(ipstr));
  SLOG(connection_.get(), 3) << "Local IP = " << local_ip_address.ToString()
                             << ":" << local_port;

  vector<SocketInfo> info_list;
  if (!socket_info_reader_->LoadTcpSocketInfo(&info_list)) {
    SLOG(connection_.get(), 2) << __func__
                               << ": Failed to load TCP socket info.";
    return false;
  }

  for (vector<SocketInfo>::const_iterator info_list_it = info_list.begin();
       info_list_it != info_list.end();
       ++info_list_it) {
    const SocketInfo& cur_sock_info = *info_list_it;

    SLOG(connection_.get(), 4)
        << "Testing against IP = "
        << cur_sock_info.local_ip_address().ToString()
        << ":" << cur_sock_info.local_port()
        << " (addresses equal:"
        << cur_sock_info.local_ip_address().Equals(local_ip_address)
        << ", ports equal:" << (cur_sock_info.local_port() == local_port)
        << ")";

    if (cur_sock_info.local_ip_address().Equals(local_ip_address) &&
        cur_sock_info.local_port() == local_port) {
      SLOG(connection_.get(), 3) << __func__
                                 << ": Found matching TCP socket info.";
      *sock_info = cur_sock_info;
      return true;
    }
  }

  SLOG(connection_.get(), 2) << __func__ << ": No matching TCP socket info.";
  return false;
}

void ConnectionHealthChecker::ReportResult() {
  SLOG(connection_.get(), 2) << __func__ << ": Result: "
                             << ResultToString(health_check_result_);
  Stop();
  result_callback_.Run(health_check_result_);
}

void ConnectionHealthChecker::SetSocketDescriptor(int sock_fd) {
  if (sock_fd_ != kInvalidSocket) {
    SLOG(connection_.get(), 4) << "Closing socket";
    socket_->Close(sock_fd_);
  }
  sock_fd_ = sock_fd;
}

void ConnectionHealthChecker::ClearSocketDescriptor() {
  SetSocketDescriptor(kInvalidSocket);
}

}  // namespace shill
