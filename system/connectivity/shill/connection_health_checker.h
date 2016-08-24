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

#ifndef SHILL_CONNECTION_HEALTH_CHECKER_H_
#define SHILL_CONNECTION_HEALTH_CHECKER_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/scoped_vector.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>

#include "shill/net/sockets.h"
#include "shill/refptr_types.h"
#include "shill/socket_info.h"

namespace shill {

class AsyncConnection;
class DNSClient;
class DNSClientFactory;
class Error;
class EventDispatcher;
class IPAddress;
class IPAddressStore;
class SocketInfoReader;

// The ConnectionHealthChecker class implements the facilities to test
// connectivity status on some connection asynchronously.
// In particular, the class can distinguish between three states of the
// connection:
//   -(1)- No connectivity (TCP connection can not be established)
//   -(2)- Partial connectivity (TCP connection can be established, but no data
//         transfer)
//   -(3)- Connectivity OK (TCP connection established, is healthy)
class ConnectionHealthChecker {
 public:
  enum Result {
    // There was some problem in the setup of ConnctionHealthChecker.
    // Could not attempt a tcp connection.
    kResultUnknown,
    // Failed to create TCP connection. Condition -(1)-.
    kResultConnectionFailure,
    // Failed to send data on TCP connection. Condition -(2)-.
    kResultCongestedTxQueue,
    // Condition -(3)-.
    kResultSuccess
  };

  ConnectionHealthChecker(ConnectionRefPtr connection,
                          EventDispatcher* dispatcher,
                          IPAddressStore* remote_ips,
                          const base::Callback<void(Result)>& result_callback);
  virtual ~ConnectionHealthChecker();

  // A new ConnectionHealthChecker is created with a default URL to attempt the
  // TCP connection with. Add a URL to try.
  virtual void AddRemoteURL(const std::string& url_string);

  // Name resolution can fail in conditions -(1)- and -(2)-. Add an IP address
  // to attempt the TCP connection with.
  virtual void AddRemoteIP(IPAddress ip);

  // Change the associated Connection on the Device.
  // This will restart any ongoing health check. Any ongoing DNS query will be
  // dropped (not restarted).
  virtual void SetConnection(ConnectionRefPtr connection);

  // Start a connection health check. The health check involves one or more
  // attempts at establishing and using a TCP connection. |result_callback_| is
  // called with the final result of the check. |result_callback_| will always
  // be called after a call to Start() unless Stop() is called in the meantime.
  // |result_callback_| may be called before Start() completes.
  //
  // Calling Start() while a health check is in progress is a no-op.
  virtual void Start();

  // Stop the current health check. No callback is called as a side effect of
  // this function.
  //
  // Calling Stop() on a Stop()ed health check is a no-op.
  virtual void Stop();

  static const char* ResultToString(Result result);

  // Accessors.
  const IPAddressStore* remote_ips() const { return remote_ips_; }
  virtual bool health_check_in_progress() const;

 protected:
  // For unit-tests.
  void set_dispatcher(EventDispatcher* dispatcher) {
    dispatcher_ = dispatcher;
  }
  void set_sock_fd(int sock_fd) { sock_fd_ = sock_fd; }
  int16_t num_connection_failures() const { return num_connection_failures_; }
  void set_num_connection_failures(int16_t val) {
    num_connection_failures_ = val;
  }
  int16_t num_tx_queue_polling_attempts() const {
    return num_tx_queue_polling_attempts_;
  }
  void set_num_tx_queue_polling_attempts(int16_t val) {
    num_tx_queue_polling_attempts_ = val;
  }
  int16_t num_congested_queue_detected() const {
    return num_congested_queue_detected_;
  }
  void set_num_congested_queue_detected(int16_t val) {
    num_congested_queue_detected_ = val;
  }
  int16_t num_successful_sends() const { return num_successful_sends_; }
  void set_num_successful_sends(int16_t val) {
    num_successful_sends_ = val;
  }
  void set_old_transmit_queue_value(uint64_t val) {
    old_transmit_queue_value_ = val;
  }
  Result health_check_result() const { return health_check_result_; }
  AsyncConnection* tcp_connection() const { return tcp_connection_.get(); }
  Connection* connection() const { return connection_.get(); }

 private:
  friend class ConnectionHealthCheckerTest;
  FRIEND_TEST(ConnectionHealthCheckerTest, GarbageCollectDNSClients);
  FRIEND_TEST(ConnectionHealthCheckerTest, GetSocketInfo);
  FRIEND_TEST(ConnectionHealthCheckerTest, NextHealthCheckSample);
  FRIEND_TEST(ConnectionHealthCheckerTest, OnConnectionComplete);
  FRIEND_TEST(ConnectionHealthCheckerTest, SetConnection);
  FRIEND_TEST(ConnectionHealthCheckerTest, VerifySentData);

  // List of static IPs for connection health check.
  static const char* kDefaultRemoteIPPool[];
  // Time to wait for DNS server.
  static const int kDNSTimeoutMilliseconds;
  static const int kInvalidSocket;
  // After |kMaxFailedConnectionAttempts| failed attempts to connect, give up
  // health check and return failure.
  static const int kMaxFailedConnectionAttempts;
  // After sending a small amount of data, attempt |kMaxSentDataPollingAttempts|
  // times to see if the data was sent successfully.
  static const int kMaxSentDataPollingAttempts;
  // After |kMinCongestedQueueAttempts| to send data indicate a congested tx
  // queue, finish health check and report a congested queue.
  static const int kMinCongestedQueueAttempts;
  // After sending data |kMinSuccessfulAttempts| times succesfully, finish
  // health check and report a healthy connection.
  static const int kMinSuccessfulSendAttempts;
  // Number of DNS queries to be spawned when a new remote URL is added.
  static const int kNumDNSQueries;
  static const uint16_t kRemotePort;
  // Time to wait before testing successful data transfer / disconnect after
  // request is made on the device.
  static const int kTCPStateUpdateWaitMilliseconds;

  // Callback for DnsClient
  void GetDNSResult(const Error& error, const IPAddress& ip);
  void GarbageCollectDNSClients();

  // Start a new AsyncConnection with callback set to OnConnectionComplete().
  void NextHealthCheckSample();
  void ReportResult();

  // Callback for AsyncConnection.
  // Observe the setup connection to test health state
  void OnConnectionComplete(bool success, int sock_fd);

  void VerifySentData();
  bool GetSocketInfo(int sock_fd, SocketInfo* sock_info);

  void SetSocketDescriptor(int sock_fd);
  void ClearSocketDescriptor();

  // The connection on which the health check is being run.
  ConnectionRefPtr connection_;
  EventDispatcher* dispatcher_;
  // Set of IPs to create TCP connection with for the health check.
  IPAddressStore* remote_ips_;
  base::Callback<void(Result)> result_callback_;

  std::unique_ptr<Sockets> socket_;
  base::WeakPtrFactory<ConnectionHealthChecker> weak_ptr_factory_;

  // Callback passed to |tcp_connection_| to report an established TCP
  // connection.
  const base::Callback<void(bool, int)> connection_complete_callback_;
  // Active TCP connection during health check.
  std::unique_ptr<AsyncConnection> tcp_connection_;
  const base::Callback<void(void)> report_result_;
  // Active socket for |tcp_connection_| during an active health check.
  int sock_fd_;
  // Interface to read TCP connection information from the system.
  std::unique_ptr<SocketInfoReader> socket_info_reader_;

  DNSClientFactory* dns_client_factory_;
  ScopedVector<DNSClient> dns_clients_;
  const base::Callback<void(const Error&, const IPAddress&)>
      dns_client_callback_;

  // Store the old value of the transmit queue to verify that data sent on the
  // connection is actually transmitted.
  uint64_t old_transmit_queue_value_;
  // Callback to post a delayed check on whether data sent on the TCP connection
  // was successfully transmitted.
  base::CancelableClosure verify_sent_data_callback_;

  bool health_check_in_progress_;
  // Number of connection failures in currently active health check.
  int16_t num_connection_failures_;
  // Number of times we have checked the tx-queue for the current send attempt.
  int16_t num_tx_queue_polling_attempts_;
  // Number of out of credit scenarios detected in current health check.
  int16_t num_congested_queue_detected_;
  // Number of successful send attempts currently active health check.
  int16_t num_successful_sends_;

  // Snooze time while polling for updated /proc/tcpinfo
  int tcp_state_update_wait_milliseconds_;

  // Temporarily store the result of health check so that |report_result_|
  // can report it.
  Result health_check_result_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionHealthChecker);
};

}  // namespace shill

#endif  // SHILL_CONNECTION_HEALTH_CHECKER_H_
