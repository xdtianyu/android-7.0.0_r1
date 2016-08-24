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

#include "shill/vpn/openvpn_management_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/data_encoding.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/net/sockets.h"
#include "shill/vpn/openvpn_driver.h"

using base::Bind;
using base::IntToString;
using base::SplitString;
using base::StringPrintf;
using base::Unretained;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(OpenVPNManagementServer* o) {
  return o->GetServiceRpcIdentifier();
}
}

namespace {
const char kPasswordTagAuth[] = "Auth";
}  // namespace

const char OpenVPNManagementServer::kStateReconnecting[] = "RECONNECTING";
const char OpenVPNManagementServer::kStateResolve[] = "RESOLVE";

OpenVPNManagementServer::OpenVPNManagementServer(OpenVPNDriver* driver)
    : driver_(driver),
      sockets_(nullptr),
      socket_(-1),
      dispatcher_(nullptr),
      connected_socket_(-1),
      hold_waiting_(false),
      hold_release_(false) {}

OpenVPNManagementServer::~OpenVPNManagementServer() {
  OpenVPNManagementServer::Stop();
}

bool OpenVPNManagementServer::Start(EventDispatcher* dispatcher,
                                    Sockets* sockets,
                                    vector<vector<string>>* options) {
  SLOG(this, 2) << __func__;
  if (IsStarted()) {
    return true;
  }

  int socket = sockets->Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket < 0) {
    PLOG(ERROR) << "Unable to create management server socket.";
    return false;
  }

  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (sockets->Bind(
          socket, reinterpret_cast<struct sockaddr*>(&addr), addrlen) < 0 ||
      sockets->Listen(socket, 1) < 0 ||
      sockets->GetSockName(
          socket, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) < 0) {
    PLOG(ERROR) << "Socket setup failed.";
    sockets->Close(socket);
    return false;
  }

  SLOG(this, 2) << "Listening socket: " << socket;
  sockets_ = sockets;
  socket_ = socket;
  ready_handler_.reset(
      dispatcher->CreateReadyHandler(
          socket, IOHandler::kModeInput,
          Bind(&OpenVPNManagementServer::OnReady, Unretained(this))));
  dispatcher_ = dispatcher;

  // Append openvpn management API options.
  driver_->AppendOption("management", inet_ntoa(addr.sin_addr),
                        IntToString(ntohs(addr.sin_port)), options);
  driver_->AppendOption("management-client", options);
  driver_->AppendOption("management-hold", options);
  hold_release_ = false;
  hold_waiting_ = false;

  driver_->AppendOption("management-query-passwords", options);
  if (driver_->AppendValueOption(kOpenVPNStaticChallengeProperty,
                                 "static-challenge",
                                 options)) {
    options->back().push_back("1");  // Force echo.
  }
  return true;
}

void OpenVPNManagementServer::Stop() {
  SLOG(this, 2) << __func__;
  if (!IsStarted()) {
    return;
  }
  state_.clear();
  input_handler_.reset();
  if (connected_socket_ >= 0) {
    sockets_->Close(connected_socket_);
    connected_socket_ = -1;
  }
  dispatcher_ = nullptr;
  ready_handler_.reset();
  if (socket_ >= 0) {
    sockets_->Close(socket_);
    socket_ = -1;
  }
  sockets_ = nullptr;
}

void OpenVPNManagementServer::ReleaseHold() {
  SLOG(this, 2) << __func__;
  hold_release_ = true;
  if (!hold_waiting_) {
    return;
  }
  LOG(INFO) << "Releasing hold.";
  hold_waiting_ = false;
  SendHoldRelease();
}

void OpenVPNManagementServer::Hold() {
  SLOG(this, 2) << __func__;
  hold_release_ = false;
}

void OpenVPNManagementServer::Restart() {
  LOG(INFO) << "Restart.";
  SendSignal("SIGUSR1");
}

std::string OpenVPNManagementServer::GetServiceRpcIdentifier() {
  return driver_->GetServiceRpcIdentifier();
}

void OpenVPNManagementServer::OnReady(int fd) {
  SLOG(this, 2) << __func__ << "(" << fd << ")";
  connected_socket_ = sockets_->Accept(fd, nullptr, nullptr);
  if (connected_socket_ < 0) {
    PLOG(ERROR) << "Connected socket accept failed.";
    return;
  }
  ready_handler_.reset();
  input_handler_.reset(dispatcher_->CreateInputHandler(
      connected_socket_,
      Bind(&OpenVPNManagementServer::OnInput, Unretained(this)),
      Bind(&OpenVPNManagementServer::OnInputError, Unretained(this))));
  SendState("on");
}

void OpenVPNManagementServer::OnInput(InputData* data) {
  SLOG(this, 2) << __func__ << "(" << data->len << ")";
  vector<string> messages = SplitString(
      string(reinterpret_cast<char*>(data->buf), data->len), "\n",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (vector<string>::const_iterator it = messages.begin();
       it != messages.end() && IsStarted(); ++it) {
    ProcessMessage(*it);
  }
}

void OpenVPNManagementServer::OnInputError(const std::string& error_msg) {
  LOG(ERROR) << error_msg;
  driver_->FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
}

void OpenVPNManagementServer::ProcessMessage(const string& message) {
  SLOG(this, 2) << __func__ << "(" << message << ")";
  if (message.empty()) {
    return;
  }
  if (!ProcessInfoMessage(message) &&
      !ProcessNeedPasswordMessage(message) &&
      !ProcessFailedPasswordMessage(message) &&
      !ProcessAuthTokenMessage(message) &&
      !ProcessStateMessage(message) &&
      !ProcessHoldMessage(message) &&
      !ProcessSuccessMessage(message)) {
    LOG(WARNING) << "Message ignored: " << message;
  }
}

bool OpenVPNManagementServer::ProcessInfoMessage(const string& message) {
  if (!base::StartsWith(message, ">INFO:", base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << message;
  return true;
}

bool OpenVPNManagementServer::ProcessNeedPasswordMessage(
    const string& message) {
  if (!base::StartsWith(message, ">PASSWORD:Need ",
                        base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << "Processing need-password message.";
  string tag = ParsePasswordTag(message);
  if (tag == kPasswordTagAuth) {
    if (message.find("SC:") != string::npos) {
      PerformStaticChallenge(tag);
    } else {
      PerformAuthentication(tag);
    }
  } else if (base::StartsWith(tag, "User-Specific TPM Token",
                              base::CompareCase::SENSITIVE)) {
    SupplyTPMToken(tag);
  } else {
    NOTIMPLEMENTED() << ": Unsupported need-password message: " << message;
    driver_->FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
  }
  return true;
}

// static
string OpenVPNManagementServer::ParseSubstring(const string& message,
                                               const string& start,
                                               const string& end) {
  SLOG(VPN, nullptr, 2) << __func__ << "(" << message
                        << ", " << start << ", " << end << ")";
  DCHECK(!start.empty() && !end.empty());
  size_t start_pos = message.find(start);
  if (start_pos == string::npos) {
    return string();
  }
  size_t end_pos = message.find(end, start_pos + start.size());
  if (end_pos == string::npos) {
    return string();
  }
  return message.substr(start_pos + start.size(),
                        end_pos - start_pos - start.size());
}

// static
string OpenVPNManagementServer::ParsePasswordTag(const string& message) {
  return ParseSubstring(message, "'", "'");
}

// static
string OpenVPNManagementServer::ParsePasswordFailedReason(
    const string& message) {
  return ParseSubstring(message, "['", "']");
}

void OpenVPNManagementServer::PerformStaticChallenge(const string& tag) {
  LOG(INFO) << "Perform static challenge: " << tag;
  string user = driver_->args()->LookupString(kOpenVPNUserProperty, "");
  string password = driver_->args()->LookupString(kOpenVPNPasswordProperty, "");
  string otp = driver_->args()->LookupString(kOpenVPNOTPProperty, "");
  string token = driver_->args()->LookupString(kOpenVPNTokenProperty, "");
  if (user.empty() || (token.empty() && (password.empty() || otp.empty()))) {
    NOTIMPLEMENTED() << ": Missing credentials:"
                     << (user.empty() ? " no-user" : "")
                     << (token.empty() ? " no-token" : "")
                     << (password.empty() ? " no-password" : "")
                     << (otp.empty() ? " no-otp" : "");
    driver_->FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
    return;
  }

  string password_encoded;
  if (!token.empty()) {
    password_encoded = token;
    // Don't reuse token.
    driver_->args()->RemoveString(kOpenVPNTokenProperty);
  } else {
    string b64_password(brillo::data_encoding::Base64Encode(password));
    string b64_otp(brillo::data_encoding::Base64Encode(otp));
    password_encoded = StringPrintf("SCRV1:%s:%s",
                                    b64_password.c_str(),
                                    b64_otp.c_str());
    // Don't reuse OTP.
    driver_->args()->RemoveString(kOpenVPNOTPProperty);
  }
  SendUsername(tag, user);
  SendPassword(tag, password_encoded);
}

void OpenVPNManagementServer::PerformAuthentication(const string& tag) {
  LOG(INFO) << "Perform authentication: " << tag;
  string user = driver_->args()->LookupString(kOpenVPNUserProperty, "");
  string password = driver_->args()->LookupString(kOpenVPNPasswordProperty, "");
  if (user.empty() || password.empty()) {
    NOTIMPLEMENTED() << ": Missing credentials:"
                     << (user.empty() ? " no-user" : "")
                     << (password.empty() ? " no-password" : "");
    driver_->FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
    return;
  }
  SendUsername(tag, user);
  SendPassword(tag, password);
}

void OpenVPNManagementServer::SupplyTPMToken(const string& tag) {
  SLOG(this, 2) << __func__ << "(" << tag << ")";
  string pin = driver_->args()->LookupString(kOpenVPNPinProperty, "");
  if (pin.empty()) {
    NOTIMPLEMENTED() << ": Missing PIN.";
    driver_->FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
    return;
  }
  SendPassword(tag, pin);
}

bool OpenVPNManagementServer::ProcessFailedPasswordMessage(
    const string& message) {
  if (!base::StartsWith(message, ">PASSWORD:Verification Failed:",
                        base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << message;
  string reason;
  if (ParsePasswordTag(message) == kPasswordTagAuth) {
    reason = ParsePasswordFailedReason(message);
  }
  driver_->FailService(Service::kFailureConnect, reason);
  return true;
}

bool OpenVPNManagementServer::ProcessAuthTokenMessage(const string& message) {
  if (!base::StartsWith(message, ">PASSWORD:Auth-Token:",
                        base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << "Auth-Token message ignored.";
  return true;
}

// >STATE:* message support. State messages are of the form:
//    >STATE:<date>,<state>,<detail>,<local-ip>,<remote-ip>
// where:
// <date> is the current time (since epoch) in seconds
// <state> is one of:
//    INITIAL, CONNECTING, WAIT, AUTH, GET_CONFIG, ASSIGN_IP, ADD_ROUTES,
//    CONNECTED, RECONNECTING, EXITING, RESOLVE, TCP_CONNECT
// <detail> is a free-form string giving details about the state change
// <local-ip> is a dotted-quad for the local IPv4 address (when available)
// <remote-ip> is a dotted-quad for the remote IPv4 address (when available)
bool OpenVPNManagementServer::ProcessStateMessage(const string& message) {
  if (!base::StartsWith(message, ">STATE:", base::CompareCase::SENSITIVE)) {
    return false;
  }
  vector<string> details = SplitString(message, ",", base::TRIM_WHITESPACE,
                                       base::SPLIT_WANT_ALL);
  if (details.size() > 1) {
    state_ = details[1];
    LOG(INFO) << "OpenVPN state: " << state_;
    if (state_ == kStateReconnecting) {
      OpenVPNDriver::ReconnectReason reason =
          OpenVPNDriver::kReconnectReasonUnknown;
      if (details.size() > 2 && details[2] == "tls-error") {
        reason = OpenVPNDriver::kReconnectReasonTLSError;
      }
      driver_->OnReconnecting(reason);
    }
  }

  return true;
}

bool OpenVPNManagementServer::ProcessHoldMessage(const string& message) {
  if (!base::StartsWith(message, ">HOLD:Waiting for hold release",
                        base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << "Client waiting for hold release.";
  hold_waiting_ = true;
  if (hold_release_) {
    ReleaseHold();
  }
  return true;
}

bool OpenVPNManagementServer::ProcessSuccessMessage(const string& message) {
  if (!base::StartsWith(message, "SUCCESS: ", base::CompareCase::SENSITIVE)) {
    return false;
  }
  LOG(INFO) << message;
  return true;
}

// static
string OpenVPNManagementServer::EscapeToQuote(const string& str) {
  string escaped;
  for (string::const_iterator it = str.begin(); it != str.end(); ++it) {
    if (*it == '\\' || *it == '"') {
      escaped += '\\';
    }
    escaped += *it;
  }
  return escaped;
}

void OpenVPNManagementServer::Send(const string& data) {
  SLOG(this, 2) << __func__;
  ssize_t len = sockets_->Send(connected_socket_, data.data(), data.size(), 0);
  PLOG_IF(ERROR, len < 0 || static_cast<size_t>(len) != data.size())
      << "Send failed.";
}

void OpenVPNManagementServer::SendState(const string& state) {
  SLOG(this, 2) << __func__ << "(" << state << ")";
  Send(StringPrintf("state %s\n", state.c_str()));
}

void OpenVPNManagementServer::SendUsername(const string& tag,
                                           const string& username) {
  SLOG(this, 2) << __func__;
  Send(StringPrintf("username \"%s\" %s\n", tag.c_str(), username.c_str()));
}

void OpenVPNManagementServer::SendPassword(const string& tag,
                                           const string& password) {
  SLOG(this, 2) << __func__;
  Send(StringPrintf("password \"%s\" \"%s\"\n",
                    tag.c_str(),
                    EscapeToQuote(password).c_str()));
}

void OpenVPNManagementServer::SendSignal(const string& signal) {
  SLOG(this, 2) << __func__ << "(" << signal << ")";
  Send(StringPrintf("signal %s\n", signal.c_str()));
}

void OpenVPNManagementServer::SendHoldRelease() {
  SLOG(this, 2) << __func__;
  Send("hold release\n");
}

}  // namespace shill
