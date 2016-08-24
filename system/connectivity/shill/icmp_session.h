//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef SHILL_ICMP_SESSION_H_
#define SHILL_ICMP_SESSION_H_

#if defined(__ANDROID__)
#include <linux/icmp.h>
#else
#include <netinet/ip_icmp.h>
#endif  // __ANDROID__

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/time/default_tick_clock.h>
#include <base/time/tick_clock.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/icmp.h"
#include "shill/net/io_handler.h"

namespace shill {

class EventDispatcher;
class IPAddress;

// The IcmpSession class encapsulates the task of performing a stateful exchange
// of echo requests and echo replies between this host and another (i.e. ping).
// The Icmp class is used to perform the sending of echo requests. Each
// IcmpSession object only allows one ICMP session to be running at one time.
// Multiple ICMP sessions can be run concurrently by creating multiple
// IcmpSession objects.
class IcmpSession {
 public:
  // The result of an ICMP session is a vector of time deltas representing how
  // long it took to receive a echo reply for each sent echo request. The vector
  // is sorted in the order that the echo requests were sent. Zero time deltas
  // represent echo requests that we did not receive a corresponding reply for.
  using IcmpSessionResult = std::vector<base::TimeDelta>;
  using IcmpSessionResultCallback =
      base::Callback<void(const IcmpSessionResult&)>;

  explicit IcmpSession(EventDispatcher* dispatcher);

  // We always call IcmpSession::Stop in the destructor to clean up, in case an
  // ICMP session is still in progress.
  virtual ~IcmpSession();

  // Starts an ICMP session, sending |kNumEchoRequestsToSend| echo requests to
  // |destination|, |kEchoRequestIntervalSeconds| apart. |result_callback| will
  // be called a) after all echo requests are sent and all echo replies are
  // received, or b) after |kTimeoutSeconds| have passed. |result_callback| will
  // only be invoked once on the first occurrence of either of these events.
  virtual bool Start(const IPAddress& destination,
                     const IcmpSessionResultCallback& result_callback);

  // Stops the current ICMP session by closing the ICMP socket and resetting
  // callbacks. Does nothing if a ICMP session is not started.
  virtual void Stop();

  bool IsStarted() { return icmp_->IsStarted(); }

  // Utility function that returns false iff |result| indicates that no echo
  // replies were received to any ICMP echo request that was sent during the
  // ICMP session that generated |result|.
  static bool AnyRepliesReceived(const IcmpSessionResult& result);

  // Utility function that returns the packet loss rate for the ICMP session
  // that generated |result| is greater than |percentage_threshold| percent.
  // The percentage packet loss determined by this function will be rounded
  // down to the closest integer percentage value. |percentage_threshold| is
  // expected to be a non-negative integer value.
  static bool IsPacketLossPercentageGreaterThan(const IcmpSessionResult& result,
                                                int percentage_threshold);

 private:
  using SentRecvTimePair = std::pair<base::TimeTicks, base::TimeTicks>;

  friend class IcmpSessionTest;

  FRIEND_TEST(IcmpSessionTest, Constructor);  // for |echo_id_|

  static uint16_t kNextUniqueEchoId;  // unique across IcmpSession objects
  static const int kTotalNumEchoRequests;
  static const int kEchoRequestIntervalSeconds;
  static const size_t kTimeoutSeconds;

  // Sends a single echo request to |destination|. This function will call
  // itself repeatedly via the event loop every |kEchoRequestIntervalSeconds|
  // until |kNumEchoRequestToSend| echo requests are sent or the timeout is
  // reached.
  void TransmitEchoRequestTask(const IPAddress& destination);

  // Called when an ICMP packet is received.
  void OnEchoReplyReceived(InputData* data);

  // Helper function that generates the result of the current ICMP session.
  IcmpSessionResult GenerateIcmpResult();

  // Called when the input handler |echo_reply_handler_| encounters an error.
  void OnEchoReplyError(const std::string& error_msg);

  // Calls |result_callback_| with the results collected so far, then stops the
  // IcmpSession. This function is called when the ICMP session successfully
  // completes, or when it times out. Does nothing if an ICMP session is not
  // started.
  void ReportResultAndStopSession();

  base::WeakPtrFactory<IcmpSession> weak_ptr_factory_;
  EventDispatcher* dispatcher_;
  std::unique_ptr<Icmp> icmp_;
  const uint16_t echo_id_;  // unique ID for this object's echo request/replies
  uint16_t current_sequence_number_;
  std::map<uint16_t, SentRecvTimePair> seq_num_to_sent_recv_time_;
  std::set<uint16_t> received_echo_reply_seq_numbers_;
  // Allow for an injectable tick clock for testing.
  base::TickClock* tick_clock_;
  base::DefaultTickClock default_tick_clock_;
  base::CancelableClosure timeout_callback_;
  IcmpSessionResultCallback result_callback_;
  IOHandler::InputCallback echo_reply_callback_;
  std::unique_ptr<IOHandler> echo_reply_handler_;

  DISALLOW_COPY_AND_ASSIGN(IcmpSession);
};

}  // namespace shill

#endif  // SHILL_ICMP_SESSION_H_
