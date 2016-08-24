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

#include <string>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/stringprintf.h>

#include "shill/async_connection.h"
#include "shill/connection.h"
#include "shill/dns_client.h"
#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/http_url.h"
#include "shill/logging.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

using base::Bind;
using base::Callback;
using base::StringPrintf;
using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kHTTP;
static string ObjectID(Connection* c) { return c->interface_name(); }
}

const int HTTPRequest::kConnectTimeoutSeconds = 10;
const int HTTPRequest::kDNSTimeoutSeconds = 5;
const int HTTPRequest::kInputTimeoutSeconds = 10;

const char HTTPRequest::kHTTPRequestTemplate[] =
    "GET %s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "Connection: Close\r\n\r\n";

HTTPRequest::HTTPRequest(ConnectionRefPtr connection,
                         EventDispatcher* dispatcher,
                         Sockets* sockets)
    : connection_(connection),
      dispatcher_(dispatcher),
      sockets_(sockets),
      weak_ptr_factory_(this),
      connect_completion_callback_(
          Bind(&HTTPRequest::OnConnectCompletion,
               weak_ptr_factory_.GetWeakPtr())),
      dns_client_callback_(Bind(&HTTPRequest::GetDNSResult,
                                weak_ptr_factory_.GetWeakPtr())),
      read_server_callback_(Bind(&HTTPRequest::ReadFromServer,
                                 weak_ptr_factory_.GetWeakPtr())),
      write_server_callback_(Bind(&HTTPRequest::WriteToServer,
                                  weak_ptr_factory_.GetWeakPtr())),
      dns_client_(
          new DNSClient(connection->IsIPv6() ? IPAddress::kFamilyIPv6
                                             : IPAddress::kFamilyIPv4,
                        connection->interface_name(),
                        connection->dns_servers(),
                        kDNSTimeoutSeconds * 1000,
                        dispatcher,
                        dns_client_callback_)),
      server_async_connection_(
          new AsyncConnection(connection_->interface_name(),
                              dispatcher_, sockets,
                              connect_completion_callback_)),
      server_port_(-1),
      server_socket_(-1),
      timeout_result_(kResultUnknown),
      is_running_(false) { }

HTTPRequest::~HTTPRequest() {
  Stop();
}

HTTPRequest::Result HTTPRequest::Start(
    const HTTPURL& url,
    const Callback<void(const ByteString&)>& read_event_callback,
    const Callback<void(Result, const ByteString&)>& result_callback) {
  SLOG(connection_.get(), 3) << "In " << __func__;

  DCHECK(!is_running_);

  is_running_ = true;
  request_data_ = ByteString(StringPrintf(kHTTPRequestTemplate,
                                          url.path().c_str(),
                                          url.host().c_str(),
                                          url.port()), false);
  server_hostname_ = url.host();
  server_port_ = url.port();
  connection_->RequestRouting();

  IPAddress addr(IPAddress::kFamilyIPv4);
  if (connection_->IsIPv6()) {
    addr.set_family(IPAddress::kFamilyIPv6);
  }
  if (addr.SetAddressFromString(server_hostname_)) {
    if (!ConnectServer(addr, server_port_)) {
      LOG(ERROR) << "Connect to "
                 << server_hostname_
                 << " failed synchronously";
      return kResultConnectionFailure;
    }
  } else {
    SLOG(connection_.get(), 3) << "Looking up host: " << server_hostname_;
    Error error;
    if (!dns_client_->Start(server_hostname_, &error)) {
      LOG(ERROR) << "Failed to start DNS client: " << error.message();
      Stop();
      return kResultDNSFailure;
    }
  }

  // Only install callbacks after connection succeeds in starting.
  read_event_callback_ = read_event_callback;
  result_callback_ = result_callback;

  return kResultInProgress;
}

void HTTPRequest::Stop() {
  SLOG(connection_.get(), 3) << "In " << __func__ << "; running is "
                             << is_running_;

  if (!is_running_) {
    return;
  }

  // Clear IO handlers first so that closing the socket doesn't cause
  // events to fire.
  write_server_handler_.reset();
  read_server_handler_.reset();

  connection_->ReleaseRouting();
  dns_client_->Stop();
  is_running_ = false;
  result_callback_.Reset();
  read_event_callback_.Reset();
  request_data_.Clear();
  response_data_.Clear();
  server_async_connection_->Stop();
  server_hostname_.clear();
  server_port_ = -1;
  if (server_socket_ != -1) {
    sockets_->Close(server_socket_);
    server_socket_ = -1;
  }
  timeout_closure_.Cancel();
  timeout_result_ = kResultUnknown;
}

bool HTTPRequest::ConnectServer(const IPAddress& address, int port) {
  SLOG(connection_.get(), 3) << "In " << __func__;
  if (!server_async_connection_->Start(address, port)) {
    LOG(ERROR) << "Could not create socket to connect to server at "
               << address.ToString();
    SendStatus(kResultConnectionFailure);
    return false;
  }
  // Start a connection timeout only if we didn't synchronously connect.
  if (server_socket_ == -1) {
    StartIdleTimeout(kConnectTimeoutSeconds, kResultConnectionTimeout);
  }
  return true;
}

// DNSClient callback that fires when the DNS request completes.
void HTTPRequest::GetDNSResult(const Error& error, const IPAddress& address) {
  SLOG(connection_.get(), 3) << "In " << __func__;
  if (!error.IsSuccess()) {
    LOG(ERROR) << "Could not resolve hostname "
               << server_hostname_
               << ": "
               << error.message();
    if (error.message() == DNSClient::kErrorTimedOut) {
      SendStatus(kResultDNSTimeout);
    } else {
      SendStatus(kResultDNSFailure);
    }
    return;
  }
  ConnectServer(address, server_port_);
}

// AsyncConnection callback routine which fires when the asynchronous Connect()
// to the remote server completes (or fails).
void HTTPRequest::OnConnectCompletion(bool success, int fd) {
  SLOG(connection_.get(), 3) << "In " << __func__;
  if (!success) {
    LOG(ERROR) << "Socket connection delayed failure to "
               << server_hostname_
               << ": "
               << server_async_connection_->error();
    // |this| could be freed as a result of calling SendStatus().
    SendStatus(kResultConnectionFailure);
    return;
  }
  server_socket_ = fd;
  write_server_handler_.reset(
      dispatcher_->CreateReadyHandler(server_socket_,
                                      IOHandler::kModeOutput,
                                      write_server_callback_));
  StartIdleTimeout(kInputTimeoutSeconds, kResultRequestTimeout);
}

void HTTPRequest::OnServerReadError(const string& /*error_msg*/) {
  SendStatus(kResultResponseFailure);
}

// IOInputHandler callback which fires when data has been read from the
// server.
void HTTPRequest::ReadFromServer(InputData* data) {
  SLOG(connection_.get(), 3) << "In " << __func__ << " length " << data->len;
  if (data->len == 0) {
    SendStatus(kResultSuccess);
    return;
  }

  response_data_.Append(ByteString(data->buf, data->len));
  StartIdleTimeout(kInputTimeoutSeconds, kResultResponseTimeout);
  if (!read_event_callback_.is_null()) {
    read_event_callback_.Run(response_data_);
  }
}

void HTTPRequest::SendStatus(Result result) {
  // Save copies on the stack, since Stop() will remove them.
  Callback<void(Result, const ByteString&)> result_callback = result_callback_;
  const ByteString response_data(response_data_);
  Stop();

  // Call the callback last, since it may delete us and |this| may no longer
  // be valid.
  if (!result_callback.is_null()) {
    result_callback.Run(result, response_data);
  }
}

// Start a timeout for "the next event".
void HTTPRequest::StartIdleTimeout(int timeout_seconds, Result timeout_result) {
  timeout_result_ = timeout_result;
  timeout_closure_.Reset(
      Bind(&HTTPRequest::TimeoutTask, weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(timeout_closure_.callback(),
                               timeout_seconds * 1000);
}

void HTTPRequest::TimeoutTask() {
  LOG(ERROR) << "Connection with "
             << server_hostname_
             << " timed out";
  SendStatus(timeout_result_);
}

// Output ReadyHandler callback which fires when the server socket is
// ready for data to be sent to it.
void HTTPRequest::WriteToServer(int fd) {
  CHECK_EQ(server_socket_, fd);
  int ret = sockets_->Send(fd, request_data_.GetConstData(),
                           request_data_.GetLength(), 0);
  CHECK(ret < 0 || static_cast<size_t>(ret) <= request_data_.GetLength());

  SLOG(connection_.get(), 3) << "In " << __func__ << " wrote " << ret << " of "
                             << request_data_.GetLength();

  if (ret < 0) {
    LOG(ERROR) << "Client write failed to "
               << server_hostname_;
    SendStatus(kResultRequestFailure);
    return;
  }

  request_data_ = ByteString(request_data_.GetConstData() + ret,
                             request_data_.GetLength() - ret);

  if (request_data_.IsEmpty()) {
    write_server_handler_->Stop();
    read_server_handler_.reset(dispatcher_->CreateInputHandler(
        server_socket_,
        read_server_callback_,
        Bind(&HTTPRequest::OnServerReadError, weak_ptr_factory_.GetWeakPtr())));
    StartIdleTimeout(kInputTimeoutSeconds, kResultResponseTimeout);
  } else {
    StartIdleTimeout(kInputTimeoutSeconds, kResultRequestTimeout);
  }
}

}  // namespace shill
