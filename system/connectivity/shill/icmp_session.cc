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

#include "shill/icmp_session.h"

#include <arpa/inet.h>
#include <netinet/ip.h>

#include <base/time/default_tick_clock.h>

#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

namespace {
const int kIPHeaderLengthUnitBytes = 4;
}

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static std::string ObjectID(IcmpSession* i) { return "(icmp_session)"; }
}

uint16_t IcmpSession::kNextUniqueEchoId = 0;
const int IcmpSession::kTotalNumEchoRequests = 3;
const int IcmpSession::kEchoRequestIntervalSeconds = 1;  // default for ping
// We should not need more than 1 second after the last request is sent to
// receive the final reply.
const size_t IcmpSession::kTimeoutSeconds =
    kEchoRequestIntervalSeconds * kTotalNumEchoRequests + 1;

IcmpSession::IcmpSession(EventDispatcher* dispatcher)
    : weak_ptr_factory_(this),
      dispatcher_(dispatcher),
      icmp_(new Icmp()),
      echo_id_(kNextUniqueEchoId),
      current_sequence_number_(0),
      tick_clock_(&default_tick_clock_),
      echo_reply_callback_(Bind(&IcmpSession::OnEchoReplyReceived,
                                weak_ptr_factory_.GetWeakPtr())) {
  // Each IcmpSession will have a unique echo ID to identify requests and reply
  // messages.
  ++kNextUniqueEchoId;
}

IcmpSession::~IcmpSession() {
  Stop();
}

bool IcmpSession::Start(const IPAddress& destination,
                        const IcmpSessionResultCallback& result_callback) {
  if (!dispatcher_) {
    LOG(ERROR) << "Invalid dispatcher";
    return false;
  }
  if (IsStarted()) {
    LOG(WARNING) << "ICMP session already started";
    return false;
  }
  if (!icmp_->Start()) {
    return false;
  }
  echo_reply_handler_.reset(dispatcher_->CreateInputHandler(
      icmp_->socket(), echo_reply_callback_,
      Bind(&IcmpSession::OnEchoReplyError, weak_ptr_factory_.GetWeakPtr())));
  result_callback_ = result_callback;
  timeout_callback_.Reset(Bind(&IcmpSession::ReportResultAndStopSession,
                               weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(timeout_callback_.callback(),
                               kTimeoutSeconds * 1000);
  seq_num_to_sent_recv_time_.clear();
  received_echo_reply_seq_numbers_.clear();
  dispatcher_->PostTask(Bind(&IcmpSession::TransmitEchoRequestTask,
                             weak_ptr_factory_.GetWeakPtr(), destination));

  return true;
}

void IcmpSession::Stop() {
  if (!IsStarted()) {
    return;
  }
  timeout_callback_.Cancel();
  echo_reply_handler_.reset();
  icmp_->Stop();
}

// static
bool IcmpSession::AnyRepliesReceived(const IcmpSessionResult& result) {
  for (const base::TimeDelta& latency : result) {
    if (!latency.is_zero()) {
      return true;
    }
  }
  return false;
}

// static
bool IcmpSession::IsPacketLossPercentageGreaterThan(
    const IcmpSessionResult& result, int percentage_threshold) {
  if (percentage_threshold < 0) {
    LOG(ERROR) << __func__ << ": negative percentage threshold ("
               << percentage_threshold << ")";
    return false;
  }

  if (result.size() == 0) {
    return false;
  }

  int lost_packet_count = 0;
  for (const base::TimeDelta& latency : result) {
    if (latency.is_zero()) {
      ++lost_packet_count;
    }
  }
  int packet_loss_percentage = (lost_packet_count * 100) / result.size();
  return packet_loss_percentage > percentage_threshold;
}

void IcmpSession::TransmitEchoRequestTask(const IPAddress& destination) {
  if (!IsStarted()) {
    // This might happen when ping times out or is stopped between two calls
    // to IcmpSession::TransmitEchoRequestTask.
    return;
  }
  if (icmp_->TransmitEchoRequest(destination, echo_id_,
                                 current_sequence_number_)) {
    seq_num_to_sent_recv_time_.emplace(
        current_sequence_number_,
        std::make_pair(tick_clock_->NowTicks(), base::TimeTicks()));
  }
  ++current_sequence_number_;
  // If we fail to transmit the echo request, fall through instead of returning,
  // so we continue sending echo requests until |kTotalNumEchoRequests| echo
  // requests are sent.

  if (seq_num_to_sent_recv_time_.size() != kTotalNumEchoRequests) {
    dispatcher_->PostDelayedTask(
        Bind(&IcmpSession::TransmitEchoRequestTask,
             weak_ptr_factory_.GetWeakPtr(), destination),
        kEchoRequestIntervalSeconds * 1000);
  }
}

void IcmpSession::OnEchoReplyReceived(InputData* data) {
  ByteString message(data->buf, data->len);
  if (message.GetLength() < sizeof(struct iphdr) + sizeof(struct icmphdr)) {
    LOG(WARNING) << "Received ICMP packet is too short to contain ICMP header";
    return;
  }

  const struct iphdr* received_ip_header =
      reinterpret_cast<const struct iphdr*>(message.GetConstData());
  const struct icmphdr* received_icmp_header =
      reinterpret_cast<const struct icmphdr*>(message.GetConstData() +
                                              received_ip_header->ihl *
                                                  kIPHeaderLengthUnitBytes);
  // We might have received other types of ICMP traffic, so ensure that the
  // message is an echo reply before handling it.
  if (received_icmp_header->type != ICMP_ECHOREPLY) {
    return;
  }

  // Make sure the message is valid and matches a pending echo request.
  if (received_icmp_header->code != Icmp::kIcmpEchoCode) {
    LOG(WARNING) << "ICMP header code is invalid";
    return;
  }

  if (received_icmp_header->un.echo.id != echo_id_) {
    SLOG(this, 3) << "received message echo id ("
                  << received_icmp_header->un.echo.id
                  << ") does not match this ICMP session's echo id ("
                  << echo_id_ << ")";
    return;
  }

  uint16_t received_seq_num = received_icmp_header->un.echo.sequence;
  if (received_echo_reply_seq_numbers_.find(received_seq_num) !=
      received_echo_reply_seq_numbers_.end()) {
    // Echo reply for this message already handled previously.
    return;
  }

  const auto& seq_num_to_sent_recv_time_pair =
      seq_num_to_sent_recv_time_.find(received_seq_num);
  if (seq_num_to_sent_recv_time_pair == seq_num_to_sent_recv_time_.end()) {
    // Echo reply not meant for any sent echo requests.
    return;
  }

  // Record the time that the echo reply was received.
  seq_num_to_sent_recv_time_pair->second.second = tick_clock_->NowTicks();
  received_echo_reply_seq_numbers_.insert(received_seq_num);

  if (received_echo_reply_seq_numbers_.size() == kTotalNumEchoRequests) {
    // All requests sent and replies received, so report results and end the
    // ICMP session.
    ReportResultAndStopSession();
  }
}

std::vector<base::TimeDelta> IcmpSession::GenerateIcmpResult() {
  std::vector<base::TimeDelta> latencies;
  for (const auto& seq_num_to_sent_recv_time_pair :
       seq_num_to_sent_recv_time_) {
    const SentRecvTimePair& sent_recv_timestamp_pair =
        seq_num_to_sent_recv_time_pair.second;
    if (sent_recv_timestamp_pair.second.is_null()) {
      // Invalid latency if an echo response has not been received.
      latencies.push_back(base::TimeDelta());
    } else {
      latencies.push_back(sent_recv_timestamp_pair.second -
                          sent_recv_timestamp_pair.first);
    }
  }
  return latencies;
}

void IcmpSession::OnEchoReplyError(const std::string& error_msg) {
  LOG(ERROR) << __func__ << ": " << error_msg;
  // Do nothing when we encounter an IO error, so we can continue receiving
  // other pending echo replies.
}

void IcmpSession::ReportResultAndStopSession() {
  if (!IsStarted()) {
    LOG(WARNING) << "ICMP session not started";
    return;
  }
  Stop();
  // Invoke result callback after calling IcmpSession::Stop, since the callback
  // might delete this object. (Any subsequent call to IcmpSession::Stop leads
  // to a segfault since this function belongs to the deleted object.)
  if (!result_callback_.is_null()) {
    result_callback_.Run(GenerateIcmpResult());
  }
}

}  // namespace shill
