// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/notification/xmpp_channel.h"

#include <string>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <weave/provider/network.h>
#include <weave/provider/task_runner.h>

#include "src/backoff_entry.h"
#include "src/data_encoding.h"
#include "src/notification/notification_delegate.h"
#include "src/notification/notification_parser.h"
#include "src/notification/xml_node.h"
#include "src/privet/openssl_utils.h"
#include "src/string_utils.h"
#include "src/utils.h"

namespace weave {

namespace {

std::string BuildXmppStartStreamCommand() {
  return "<stream:stream to='clouddevices.gserviceaccount.com' "
         "xmlns:stream='http://etherx.jabber.org/streams' "
         "xml:lang='*' version='1.0' xmlns='jabber:client'>";
}

std::string BuildXmppAuthenticateCommand(const std::string& account,
                                         const std::string& token) {
  std::vector<uint8_t> credentials;
  credentials.push_back(0);
  credentials.insert(credentials.end(), account.begin(), account.end());
  credentials.push_back(0);
  credentials.insert(credentials.end(), token.begin(), token.end());
  std::string msg =
      "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' "
      "mechanism='X-OAUTH2' auth:service='oauth2' "
      "auth:allow-non-google-login='true' "
      "auth:client-uses-full-bind-result='true' "
      "xmlns:auth='http://www.google.com/talk/protocol/auth'>" +
      Base64Encode(credentials) + "</auth>";
  return msg;
}

// Backoff policy.
// Note: In order to ensure a minimum of 20 seconds between server errors,
// we have a 30s +- 10s (33%) jitter initial backoff.
const BackoffEntry::Policy kDefaultBackoffPolicy = {
    // Number of initial errors (in sequence) to ignore before applying
    // exponential back-off rules.
    0,

    // Initial delay for exponential back-off in ms.
    30 * 1000,  // 30 seconds.

    // Factor by which the waiting time will be multiplied.
    2,

    // Fuzzing percentage. ex: 10% will spread requests randomly
    // between 90%-100% of the calculated time.
    0.33,  // 33%.

    // Maximum amount of time we are willing to delay our request in ms.
    10 * 60 * 1000,  // 10 minutes.

    // Time to keep an entry from being discarded even when it
    // has no significant state, -1 to never discard.
    -1,

    // Don't use initial delay unless the last request was an error.
    false,
};

// Used for keeping connection alive.
const int kRegularPingIntervalSeconds = 60;
const int kRegularPingTimeoutSeconds = 30;

// Used for diagnostic when connectivity changed.
const int kAgressivePingIntervalSeconds = 5;
const int kAgressivePingTimeoutSeconds = 10;

const int kConnectingTimeoutAfterNetChangeSeconds = 30;

}  // namespace

XmppChannel::XmppChannel(const std::string& account,
                         const std::string& access_token,
                         const std::string& xmpp_endpoint,
                         provider::TaskRunner* task_runner,
                         provider::Network* network)
    : account_{account},
      access_token_{access_token},
      xmpp_endpoint_{xmpp_endpoint},
      network_{network},
      backoff_entry_{&kDefaultBackoffPolicy},
      task_runner_{task_runner},
      iq_stanza_handler_{new IqStanzaHandler{this, task_runner}} {
  read_socket_data_.resize(4096);
  if (network) {
    network->AddConnectionChangedCallback(base::Bind(
        &XmppChannel::OnConnectivityChanged, weak_ptr_factory_.GetWeakPtr()));
  }
}

void XmppChannel::OnMessageRead(size_t size, ErrorPtr error) {
  read_pending_ = false;
  if (error)
    return Restart();
  std::string msg(read_socket_data_.data(), size);
  VLOG(2) << "Received XMPP packet: '" << msg << "'";

  if (!size)
    return Restart();

  stream_parser_.ParseData(msg);
  WaitForMessage();
}

void XmppChannel::OnStreamStart(const std::string& node_name,
                                std::map<std::string, std::string> attributes) {
  VLOG(2) << "XMPP stream start: " << node_name;
}

void XmppChannel::OnStreamEnd(const std::string& node_name) {
  VLOG(2) << "XMPP stream ended: " << node_name;
  Stop();
  if (IsConnected()) {
    // If we had a fully-established connection, restart it now.
    // However, if the connection has never been established yet (e.g.
    // authorization failed), do not restart right now. Wait till we get
    // new credentials.
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::Bind(&XmppChannel::Restart, task_ptr_factory_.GetWeakPtr()), {});
  } else if (delegate_) {
    delegate_->OnPermanentFailure();
  }
}

void XmppChannel::OnStanza(std::unique_ptr<XmlNode> stanza) {
  // Handle stanza asynchronously, since XmppChannel::OnStanza() is a callback
  // from expat XML parser and some stanza could cause the XMPP stream to be
  // reset and the parser to be re-initialized. We don't want to destroy the
  // parser while it is performing a callback invocation.
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&XmppChannel::HandleStanza, task_ptr_factory_.GetWeakPtr(),
                 base::Passed(std::move(stanza))),
      {});
}

void XmppChannel::HandleStanza(std::unique_ptr<XmlNode> stanza) {
  VLOG(2) << "XMPP stanza received: " << stanza->ToString();

  switch (state_) {
    case XmppState::kConnected:
      if (stanza->name() == "stream:features") {
        auto children = stanza->FindChildren("mechanisms/mechanism", false);
        for (const auto& child : children) {
          if (child->text() == "X-OAUTH2") {
            state_ = XmppState::kAuthenticationStarted;
            SendMessage(BuildXmppAuthenticateCommand(account_, access_token_));
            return;
          }
        }
      }
      break;
    case XmppState::kAuthenticationStarted:
      if (stanza->name() == "success") {
        state_ = XmppState::kStreamRestartedPostAuthentication;
        RestartXmppStream();
        return;
      } else if (stanza->name() == "failure") {
        if (stanza->FindFirstChild("not-authorized", false)) {
          state_ = XmppState::kAuthenticationFailed;
          return;
        }
      }
      break;
    case XmppState::kStreamRestartedPostAuthentication:
      if (stanza->name() == "stream:features" &&
          stanza->FindFirstChild("bind", false)) {
        state_ = XmppState::kBindSent;
        iq_stanza_handler_->SendRequest(
            "set", "", "", "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>",
            base::Bind(&XmppChannel::OnBindCompleted,
                       task_ptr_factory_.GetWeakPtr()),
            base::Bind(&XmppChannel::Restart, task_ptr_factory_.GetWeakPtr()));
        return;
      }
      break;
    default:
      if (stanza->name() == "message") {
        HandleMessageStanza(std::move(stanza));
        return;
      } else if (stanza->name() == "iq") {
        if (!iq_stanza_handler_->HandleIqStanza(std::move(stanza))) {
          LOG(ERROR) << "Failed to handle IQ stanza";
          CloseStream();
        }
        return;
      }
      LOG(INFO) << "Unexpected XMPP stanza ignored: " << stanza->ToString();
      return;
  }
  // Something bad happened. Close the stream and start over.
  LOG(ERROR) << "Error condition occurred handling stanza: "
             << stanza->ToString() << " in state: " << static_cast<int>(state_);
  CloseStream();
}

void XmppChannel::CloseStream() {
  SendMessage("</stream:stream>");
}

void XmppChannel::OnBindCompleted(std::unique_ptr<XmlNode> reply) {
  if (reply->GetAttributeOrEmpty("type") != "result") {
    CloseStream();
    return;
  }
  const XmlNode* jid_node = reply->FindFirstChild("bind/jid", false);
  if (!jid_node) {
    LOG(ERROR) << "XMPP Bind response is missing JID";
    CloseStream();
    return;
  }

  jid_ = jid_node->text();
  state_ = XmppState::kSessionStarted;
  iq_stanza_handler_->SendRequest(
      "set", "", "", "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>",
      base::Bind(&XmppChannel::OnSessionEstablished,
                 task_ptr_factory_.GetWeakPtr()),
      base::Bind(&XmppChannel::Restart, task_ptr_factory_.GetWeakPtr()));
}

void XmppChannel::OnSessionEstablished(std::unique_ptr<XmlNode> reply) {
  if (reply->GetAttributeOrEmpty("type") != "result") {
    CloseStream();
    return;
  }
  state_ = XmppState::kSubscribeStarted;
  std::string body =
      "<subscribe xmlns='google:push'>"
      "<item channel='cloud_devices' from=''/></subscribe>";
  iq_stanza_handler_->SendRequest(
      "set", "", account_, body,
      base::Bind(&XmppChannel::OnSubscribed, task_ptr_factory_.GetWeakPtr()),
      base::Bind(&XmppChannel::Restart, task_ptr_factory_.GetWeakPtr()));
}

void XmppChannel::OnSubscribed(std::unique_ptr<XmlNode> reply) {
  if (reply->GetAttributeOrEmpty("type") != "result") {
    CloseStream();
    return;
  }
  state_ = XmppState::kSubscribed;
  if (delegate_)
    delegate_->OnConnected(GetName());
}

void XmppChannel::HandleMessageStanza(std::unique_ptr<XmlNode> stanza) {
  const XmlNode* node = stanza->FindFirstChild("push:push/push:data", true);
  if (!node) {
    LOG(WARNING) << "XMPP message stanza is missing <push:data> element";
    return;
  }
  std::string data = node->text();
  std::string json_data;
  if (!Base64Decode(data, &json_data)) {
    LOG(WARNING) << "Failed to decode base64-encoded message payload: " << data;
    return;
  }

  VLOG(2) << "XMPP push notification data: " << json_data;
  auto json_dict = LoadJsonDict(json_data, nullptr);
  if (json_dict && delegate_)
    ParseNotificationJson(*json_dict, delegate_, GetName());
}

void XmppChannel::CreateSslSocket() {
  CHECK(!stream_);
  state_ = XmppState::kConnecting;
  LOG(INFO) << "Starting XMPP connection to: " << xmpp_endpoint_;

  std::pair<std::string, std::string> host_port =
      SplitAtFirst(xmpp_endpoint_, ":", true);
  CHECK(!host_port.first.empty());
  CHECK(!host_port.second.empty());
  uint32_t port = 0;
  CHECK(base::StringToUint(host_port.second, &port)) << xmpp_endpoint_;

  network_->OpenSslSocket(host_port.first, port,
                          base::Bind(&XmppChannel::OnSslSocketReady,
                                     task_ptr_factory_.GetWeakPtr()));
}

void XmppChannel::OnSslSocketReady(std::unique_ptr<Stream> stream,
                                   ErrorPtr error) {
  if (error) {
    LOG(ERROR) << "TLS handshake failed. Restarting XMPP connection";
    backoff_entry_.InformOfRequest(false);

    LOG(INFO) << "Delaying connection to XMPP server for "
              << backoff_entry_.GetTimeUntilRelease();
    return task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(&XmppChannel::CreateSslSocket,
                              task_ptr_factory_.GetWeakPtr()),
        backoff_entry_.GetTimeUntilRelease());
  }
  CHECK(XmppState::kConnecting == state_);
  backoff_entry_.InformOfRequest(true);
  stream_ = std::move(stream);
  state_ = XmppState::kConnected;
  RestartXmppStream();
  ScheduleRegularPing();
}

void XmppChannel::SendMessage(const std::string& message) {
  CHECK(stream_) << "No XMPP socket stream available";
  if (write_pending_) {
    queued_write_data_ += message;
    return;
  }
  write_socket_data_ = queued_write_data_ + message;
  queued_write_data_.clear();
  VLOG(2) << "Sending XMPP message: " << message;

  write_pending_ = true;
  stream_->Write(
      write_socket_data_.data(), write_socket_data_.size(),
      base::Bind(&XmppChannel::OnMessageSent, task_ptr_factory_.GetWeakPtr()));
}

void XmppChannel::OnMessageSent(ErrorPtr error) {
  write_pending_ = false;
  if (error)
    return Restart();
  if (queued_write_data_.empty()) {
    WaitForMessage();
  } else {
    SendMessage(std::string{});
  }
}

void XmppChannel::WaitForMessage() {
  if (read_pending_ || !stream_)
    return;

  read_pending_ = true;
  stream_->Read(
      read_socket_data_.data(), read_socket_data_.size(),
      base::Bind(&XmppChannel::OnMessageRead, task_ptr_factory_.GetWeakPtr()));
}

std::string XmppChannel::GetName() const {
  return "xmpp";
}

bool XmppChannel::IsConnected() const {
  return state_ == XmppState::kSubscribed;
}

void XmppChannel::AddChannelParameters(base::DictionaryValue* channel_json) {
  // No extra parameters needed for XMPP.
}

void XmppChannel::Restart() {
  LOG(INFO) << "Restarting XMPP";
  Stop();
  Start(delegate_);
}

void XmppChannel::Start(NotificationDelegate* delegate) {
  CHECK(state_ == XmppState::kNotStarted);
  delegate_ = delegate;

  CreateSslSocket();
}

void XmppChannel::Stop() {
  if (IsConnected() && delegate_)
    delegate_->OnDisconnected();

  task_ptr_factory_.InvalidateWeakPtrs();
  ping_ptr_factory_.InvalidateWeakPtrs();

  stream_.reset();
  state_ = XmppState::kNotStarted;
}

void XmppChannel::RestartXmppStream() {
  stream_parser_.Reset();
  stream_->CancelPendingOperations();
  read_pending_ = false;
  write_pending_ = false;
  SendMessage(BuildXmppStartStreamCommand());
}

void XmppChannel::SchedulePing(base::TimeDelta interval,
                               base::TimeDelta timeout) {
  VLOG(1) << "Next XMPP ping in " << interval << " with timeout " << timeout;
  ping_ptr_factory_.InvalidateWeakPtrs();
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&XmppChannel::PingServer,
                            ping_ptr_factory_.GetWeakPtr(), timeout),
      interval);
}

void XmppChannel::ScheduleRegularPing() {
  SchedulePing(base::TimeDelta::FromSeconds(kRegularPingIntervalSeconds),
               base::TimeDelta::FromSeconds(kRegularPingTimeoutSeconds));
}

void XmppChannel::ScheduleFastPing() {
  SchedulePing(base::TimeDelta::FromSeconds(kAgressivePingIntervalSeconds),
               base::TimeDelta::FromSeconds(kAgressivePingTimeoutSeconds));
}

void XmppChannel::PingServer(base::TimeDelta timeout) {
  VLOG(1) << "Sending XMPP ping";
  if (!IsConnected()) {
    LOG(WARNING) << "XMPP channel is not connected";
    Restart();
    return;
  }

  // Send an XMPP Ping request as defined in XEP-0199 extension:
  // http://xmpp.org/extensions/xep-0199.html
  iq_stanza_handler_->SendRequestWithCustomTimeout(
      "get", jid_, account_, "<ping xmlns='urn:xmpp:ping'/>", timeout,
      base::Bind(&XmppChannel::OnPingResponse, task_ptr_factory_.GetWeakPtr(),
                 base::Time::Now()),
      base::Bind(&XmppChannel::OnPingTimeout, task_ptr_factory_.GetWeakPtr(),
                 base::Time::Now()));
}

void XmppChannel::OnPingResponse(base::Time sent_time,
                                 std::unique_ptr<XmlNode> reply) {
  VLOG(1) << "XMPP response received after " << (base::Time::Now() - sent_time);
  // Ping response received from server. Everything seems to be in order.
  // Reschedule with default intervals.
  ScheduleRegularPing();
}

void XmppChannel::OnPingTimeout(base::Time sent_time) {
  LOG(WARNING) << "XMPP channel seems to be disconnected. Ping timed out after "
               << (base::Time::Now() - sent_time);
  Restart();
}

void XmppChannel::OnConnectivityChanged() {
  if (state_ == XmppState::kNotStarted)
    return;

  if (state_ == XmppState::kConnecting &&
      backoff_entry_.GetTimeUntilRelease() <
          base::TimeDelta::FromSeconds(
              kConnectingTimeoutAfterNetChangeSeconds)) {
    VLOG(1) << "Next reconnect in " << backoff_entry_.GetTimeUntilRelease();
    return;
  }

  ScheduleFastPing();
}

}  // namespace weave
