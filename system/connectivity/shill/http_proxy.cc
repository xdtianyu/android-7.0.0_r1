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

#include <errno.h>
#include <netinet/in.h>
#include <linux/if.h>  // NOLINT - Needs definitions from netinet/in.h
#include <stdio.h>
#include <time.h>

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "shill/async_connection.h"
#include "shill/connection.h"
#include "shill/dns_client.h"
#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

using base::Bind;
using base::StringPrintf;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kHTTPProxy;
static string ObjectID(Connection* c) {
  return c->interface_name();
}
}

const int HTTPProxy::kClientHeaderTimeoutSeconds = 1;
const int HTTPProxy::kConnectTimeoutSeconds = 10;
const int HTTPProxy::kDNSTimeoutSeconds = 5;
const int HTTPProxy::kDefaultServerPort = 80;
const int HTTPProxy::kInputTimeoutSeconds = 30;
const size_t HTTPProxy::kMaxClientQueue = 10;
const size_t HTTPProxy::kMaxHeaderCount = 128;
const size_t HTTPProxy::kMaxHeaderSize = 2048;
const int HTTPProxy::kTransactionTimeoutSeconds = 600;

const char HTTPProxy::kHTTPMethodConnect[] = "connect";
const char HTTPProxy::kHTTPMethodTerminator[] = " ";
const char HTTPProxy::kHTTPURLDelimiters[] = " /#?";
const char HTTPProxy::kHTTPURLPrefix[] = "http://";
const char HTTPProxy::kHTTPVersionPrefix[] = " HTTP/1";
const char HTTPProxy::kInternalErrorMsg[] = "Proxy Failed: Internal Error";

HTTPProxy::HTTPProxy(ConnectionRefPtr connection)
    : state_(kStateIdle),
      connection_(connection),
      weak_ptr_factory_(this),
      accept_callback_(Bind(&HTTPProxy::AcceptClient,
                            weak_ptr_factory_.GetWeakPtr())),
      connect_completion_callback_(Bind(&HTTPProxy::OnConnectCompletion,
                                        weak_ptr_factory_.GetWeakPtr())),
      dns_client_callback_(Bind(&HTTPProxy::GetDNSResult,
                                weak_ptr_factory_.GetWeakPtr())),
      read_client_callback_(Bind(&HTTPProxy::ReadFromClient,
                                 weak_ptr_factory_.GetWeakPtr())),
      read_server_callback_(Bind(&HTTPProxy::ReadFromServer,
                                 weak_ptr_factory_.GetWeakPtr())),
      write_client_callback_(Bind(&HTTPProxy::WriteToClient,
                                  weak_ptr_factory_.GetWeakPtr())),
      write_server_callback_(Bind(&HTTPProxy::WriteToServer,
                                  weak_ptr_factory_.GetWeakPtr())),
      dispatcher_(nullptr),
      proxy_port_(-1),
      proxy_socket_(-1),
      sockets_(nullptr),
      client_socket_(-1),
      server_port_(kDefaultServerPort),
      server_socket_(-1),
      is_route_requested_(false) { }

HTTPProxy::~HTTPProxy() {
  Stop();
}

bool HTTPProxy::Start(EventDispatcher* dispatcher,
                      Sockets* sockets) {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (sockets_) {
    // We are already running.
    return true;
  }

  proxy_socket_ = sockets->Socket(PF_INET, SOCK_STREAM, 0);
  if (proxy_socket_ < 0) {
    PLOG(ERROR) << "Failed to open proxy socket";
    return false;
  }

  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (sockets->Bind(proxy_socket_,
                    reinterpret_cast<struct sockaddr*>(&addr),
                    sizeof(addr)) < 0 ||
      sockets->GetSockName(proxy_socket_,
                           reinterpret_cast<struct sockaddr*>(&addr),
                           &addrlen) < 0 ||
      sockets->SetNonBlocking(proxy_socket_) < 0 ||
      sockets->Listen(proxy_socket_, kMaxClientQueue) < 0) {
    sockets->Close(proxy_socket_);
    proxy_socket_ = -1;
    PLOG(ERROR) << "HTTPProxy socket setup failed";
    return false;
  }

  accept_handler_.reset(
      dispatcher->CreateReadyHandler(proxy_socket_, IOHandler::kModeInput,
                                     accept_callback_));
  dispatcher_ = dispatcher;
  dns_client_.reset(new DNSClient(IPAddress::kFamilyIPv4,
                                  connection_->interface_name(),
                                  connection_->dns_servers(),
                                  kDNSTimeoutSeconds * 1000,
                                  dispatcher,
                                  dns_client_callback_));
  proxy_port_ = ntohs(addr.sin_port);
  server_async_connection_.reset(
      new AsyncConnection(connection_->interface_name(), dispatcher, sockets,
                          connect_completion_callback_));
  sockets_ = sockets;
  state_ = kStateWaitConnection;
  return true;
}

void HTTPProxy::Stop() {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (!sockets_) {
    return;
  }

  StopClient();

  accept_handler_.reset();
  dispatcher_ = nullptr;
  dns_client_.reset();
  proxy_port_ = -1;
  server_async_connection_.reset();
  sockets_->Close(proxy_socket_);
  proxy_socket_ = -1;
  sockets_ = nullptr;
  state_ = kStateIdle;
}

// IOReadyHandler callback routine fired when a client connects to the
// proxy's socket.  We Accept() the client and start reading a request
// from it.
void HTTPProxy::AcceptClient(int fd) {
  SLOG(connection_.get(), 3) << "In " << __func__;

  int client_fd = sockets_->Accept(fd, nullptr, nullptr);
  if (client_fd < 0) {
    PLOG(ERROR) << "Client accept failed";
    return;
  }

  accept_handler_->Stop();

  client_socket_ = client_fd;

  sockets_->SetNonBlocking(client_socket_);
  read_client_handler_.reset(dispatcher_->CreateInputHandler(
      client_socket_,
      read_client_callback_,
      Bind(&HTTPProxy::OnReadError, weak_ptr_factory_.GetWeakPtr())));
  // Overall transaction timeout.
  transaction_timeout_.Reset(Bind(&HTTPProxy::StopClient,
                                  weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(transaction_timeout_.callback(),
                               kTransactionTimeoutSeconds * 1000);

  state_ = kStateReadClientHeader;
  StartIdleTimeout();
}

bool HTTPProxy::ConnectServer(const IPAddress& address, int port) {
  state_ = kStateConnectServer;
  if (!server_async_connection_->Start(address, port)) {
    SendClientError(500, "Could not create socket to connect to server");
    return false;
  }
  StartIdleTimeout();
  return true;
}

// DNSClient callback that fires when the DNS request completes.
void HTTPProxy::GetDNSResult(const Error& error, const IPAddress& address) {
  if (!error.IsSuccess()) {
    SendClientError(502, string("Could not resolve hostname: ") +
                    error.message());
    return;
  }
  ConnectServer(address, server_port_);
}

// IOReadyHandler callback routine which fires when the asynchronous Connect()
// to the remote server completes (or fails).
void HTTPProxy::OnConnectCompletion(bool success, int fd) {
  if (!success) {
    SendClientError(500, string("Socket connection delayed failure: ") +
                    server_async_connection_->error());
    return;
  }
  server_socket_ = fd;
  state_ = kStateTunnelData;

  // If this was a "CONNECT" request, notify the client that the connection
  // has been established by sending an "OK" response.
  if (base::LowerCaseEqualsASCII(client_method_, kHTTPMethodConnect)) {
    SetClientResponse(200, "OK", "", "");
    StartReceive();
  }

  StartTransmit();
}

void HTTPProxy::OnReadError(const string& error_msg) {
  StopClient();
}

// Read through the header lines from the client, modifying or adding
// lines as necessary.  Perform final determination of the hostname/port
// we should connect to and either start a DNS request or connect to a
// numeric address.
bool HTTPProxy::ParseClientRequest() {
  SLOG(connection_.get(), 3) << "In " << __func__;

  string host;
  bool found_via = false;
  bool found_connection = false;
  for (auto& header : client_headers_) {
    if (base::StartsWith(header, "Host:",
                         base::CompareCase::INSENSITIVE_ASCII)) {
      host = header.substr(5);
    } else if (base::StartsWith(header, "Via:",
                                base::CompareCase::INSENSITIVE_ASCII)) {
      found_via = true;
      header.append(StringPrintf(", %s shill-proxy", client_version_.c_str()));
    } else if (base::StartsWith(header, "Connection:",
                                base::CompareCase::INSENSITIVE_ASCII)) {
      found_connection = true;
      header.assign("Connection: close");
    } else if (base::StartsWith(header, "Proxy-Connection:",
                                base::CompareCase::INSENSITIVE_ASCII)) {
      header.assign("Proxy-Connection: close");
    }
  }

  if (!found_connection) {
    client_headers_.push_back("Connection: close");
  }
  if (!found_via) {
    client_headers_.push_back(
        StringPrintf("Via: %s shill-proxy", client_version_.c_str()));
  }

  // Assemble the request as it will be sent to the server.
  client_data_.Clear();
  if (!base::LowerCaseEqualsASCII(client_method_, kHTTPMethodConnect)) {
    for (const auto& header : client_headers_) {
      client_data_.Append(ByteString(header + "\r\n", false));
    }
    client_data_.Append(ByteString(string("\r\n"), false));
  }

  base::TrimWhitespaceASCII(host, base::TRIM_ALL, &host);
  if (host.empty()) {
    // Revert to using the hostname in the URL if no "Host:" header exists.
    host = server_hostname_;
  }

  if (host.empty()) {
    SendClientError(400, "I don't know what host you want me to connect to");
    return false;
  }

  server_port_ = 80;
  vector<string> host_parts = base::SplitString(
      host, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (host_parts.size() > 2) {
    SendClientError(400, "Too many colons in hostname");
    return false;
  } else if (host_parts.size() == 2) {
    server_hostname_ = host_parts[0];
    if (!base::StringToInt(host_parts[1], &server_port_)) {
      SendClientError(400, "Could not parse port number");
      return false;
    }
  } else {
    server_hostname_ = host;
  }

  connection_->RequestRouting();
  is_route_requested_ = true;

  IPAddress addr(IPAddress::kFamilyIPv4);
  if (addr.SetAddressFromString(server_hostname_)) {
    if (!ConnectServer(addr, server_port_)) {
      return false;
    }
  } else {
    SLOG(connection_.get(), 3) << "Looking up host: " << server_hostname_;
    Error error;
    if (!dns_client_->Start(server_hostname_, &error)) {
      SendClientError(502, "Could not resolve hostname: " + error.message());
      return false;
    }
    state_ = kStateLookupServer;
  }
  return true;
}

// Accept a new line into the client headers.  Returns false if a parse
// error occurs.
bool HTTPProxy::ProcessLastHeaderLine() {
  string* header = &client_headers_.back();
  base::TrimString(*header, "\r", header);

  if (header->empty()) {
    // Empty line terminates client headers.
    client_headers_.pop_back();
    if (!ParseClientRequest()) {
      return false;
    }
  }

  // Is this is the first header line?
  if (client_headers_.size() == 1) {
    if (!ReadClientHTTPMethod(header) ||
        !ReadClientHTTPVersion(header) ||
        !ReadClientHostname(header)) {
      return false;
    }
  }

  if (client_headers_.size() >= kMaxHeaderCount) {
    SendClientError(500, kInternalErrorMsg);
    return false;
  }

  return true;
}

// Split input from client into header lines, and consume parsed lines
// from InputData.  The passed in |data| is modified to indicate the
// characters consumed.
bool HTTPProxy::ReadClientHeaders(InputData* data) {
  unsigned char* ptr = data->buf;
  unsigned char* end = ptr + data->len;

  if (client_headers_.empty()) {
    client_headers_.push_back(string());
  }

  for (; ptr < end && state_ == kStateReadClientHeader; ++ptr) {
    if (*ptr == '\n') {
      if (!ProcessLastHeaderLine()) {
        return false;
      }

      // Start a new line.  New chararacters we receive will be appended there.
      client_headers_.push_back(string());
      continue;
    }

    string* header = &client_headers_.back();
    // Is the first character of the header line a space or tab character?
    if (header->empty() && (*ptr == ' ' || *ptr == '\t') &&
        client_headers_.size() > 1) {
      // Line Continuation: Add this character to the previous header line.
      // This way, all of the data (including newlines and line continuation
      // characters) related to a specific header will be contained within
      // a single element of |client_headers_|, and manipulation of headers
      // such as appending will be simpler.  This is accomplished by removing
      // the empty line we started, and instead appending the whitespace
      // and following characters to the previous line.
      client_headers_.pop_back();
      header = &client_headers_.back();
      header->append("\r\n");
    }

    if (header->length() >= kMaxHeaderSize) {
      SendClientError(500, kInternalErrorMsg);
      return false;
    }
    header->push_back(*ptr);
  }

  // Return the remaining data to the caller -- this could be POST data
  // or other non-header data sent with the client request.
  data->buf = ptr;
  data->len = end - ptr;

  return true;
}

// Finds the URL in the first line of an HTTP client header, and extracts
// and removes the hostname (and port) from the URL.  Returns false if a
// parse error occurs, and true otherwise (whether or not the hostname was
// found).
bool HTTPProxy::ReadClientHostname(string* header) {
  const string http_url_prefix(kHTTPURLPrefix);
  size_t url_idx = header->find(http_url_prefix);
  if (url_idx != string::npos) {
    size_t host_start = url_idx + http_url_prefix.length();
    size_t host_end =
      header->find_first_of(kHTTPURLDelimiters, host_start);
    if (host_end != string::npos) {
      server_hostname_ = header->substr(host_start,
                                        host_end - host_start);
      // Modify the URL passed upstream to remove "http://<hostname>".
      header->erase(url_idx, host_end - url_idx);
      if ((*header)[url_idx] != '/') {
        header->insert(url_idx, "/");
      }
    } else {
      LOG(ERROR) << "Could not find end of hostname in request.  Line was: "
                 << *header;
      SendClientError(500, kInternalErrorMsg);
      return false;
    }
  }
  return true;
}

bool HTTPProxy::ReadClientHTTPMethod(string* header) {
  size_t method_end = header->find(kHTTPMethodTerminator);
  if (method_end == string::npos || method_end == 0) {
    LOG(ERROR) << "Could not parse HTTP method.  Line was: " << *header;
    SendClientError(501, "Server could not parse HTTP method");
    return false;
  }
  client_method_ = header->substr(0, method_end);
  return true;
}

// Extract the HTTP version number from the first line of the client headers.
// Returns true if found.
bool HTTPProxy::ReadClientHTTPVersion(string* header) {
  const string http_version_prefix(kHTTPVersionPrefix);
  size_t http_ver_pos = header->find(http_version_prefix);
  if (http_ver_pos != string::npos) {
    client_version_ =
      header->substr(http_ver_pos + http_version_prefix.length() - 1);
  } else {
    SendClientError(501, "Server only accepts HTTP/1.x requests");
    return false;
  }
  return true;
}

// IOInputHandler callback that fires when data is read from the client.
// This could be header data, or perhaps POST data that follows the headers.
void HTTPProxy::ReadFromClient(InputData* data) {
  SLOG(connection_.get(), 3) << "In " << __func__ << " length " << data->len;

  if (data->len == 0) {
    // EOF from client.
    StopClient();
    return;
  }

  if (state_ == kStateReadClientHeader) {
    if (!ReadClientHeaders(data)) {
      return;
    }
    if (state_ == kStateReadClientHeader) {
      // Still consuming client headers; restart the input timer.
      StartIdleTimeout();
      return;
    }
  }

  // Check data->len again since ReadClientHeaders() may have consumed some
  // part of it.
  if (data->len != 0) {
    // The client sent some information after its headers.  Buffer the client
    // input and temporarily disable input events from the client.
    client_data_.Append(ByteString(data->buf, data->len));
    read_client_handler_->Stop();
    StartTransmit();
  }
}

// IOInputHandler callback which fires when data has been read from the
// server.
void HTTPProxy::ReadFromServer(InputData* data) {
  SLOG(connection_.get(), 3) << "In " << __func__ << " length " << data->len;
  if (data->len == 0) {
    // Server closed connection.
    if (server_data_.IsEmpty()) {
      StopClient();
      return;
    }
    state_ = kStateFlushResponse;
  } else {
    read_server_handler_->Stop();
  }

  server_data_.Append(ByteString(data->buf, data->len));

  StartTransmit();
}

// Return an HTTP error message back to the client.
void HTTPProxy::SendClientError(int code, const string& error) {
  SLOG(connection_.get(), 3) << "In " << __func__;
  LOG(ERROR) << "Sending error " << error;
  SetClientResponse(code, "ERROR", "text/plain", error);
  state_ = kStateFlushResponse;
  StartTransmit();
}

// Create an HTTP response message to be sent to the client.
void HTTPProxy::SetClientResponse(int code, const string& type,
                                  const string& content_type,
                                  const string& message) {
  string content_line;
  if (!message.empty() && !content_type.empty()) {
    content_line = StringPrintf("Content-Type: %s\r\n", content_type.c_str());
  }
  string response = StringPrintf("HTTP/1.1 %d %s\r\n"
                                 "%s\r\n"
                                 "%s", code, type.c_str(),
                                 content_line.c_str(),
                                 message.c_str());
  server_data_ = ByteString(response, false);
}

// Start a timeout for "the next event".  This timeout augments the overall
// transaction timeout to make sure there is some activity occurring at
// reasonable intervals.
void HTTPProxy::StartIdleTimeout() {
  int timeout_seconds = 0;
  switch (state_) {
    case kStateReadClientHeader:
      timeout_seconds = kClientHeaderTimeoutSeconds;
      break;
    case kStateConnectServer:
      timeout_seconds = kConnectTimeoutSeconds;
      break;
    case kStateLookupServer:
      // DNSClient has its own internal timeout, so we need not set one here.
      timeout_seconds = 0;
      break;
    default:
      timeout_seconds = kInputTimeoutSeconds;
      break;
  }
  idle_timeout_.Cancel();
  if (timeout_seconds != 0) {
    idle_timeout_.Reset(Bind(&HTTPProxy::StopClient,
                             weak_ptr_factory_.GetWeakPtr()));
    dispatcher_->PostDelayedTask(idle_timeout_.callback(),
                                 timeout_seconds * 1000);
  }
}

// Start the various input handlers.  Listen for new data only if we have
// completely written the last data we've received to the other end.
void HTTPProxy::StartReceive() {
  if (state_ == kStateTunnelData && client_data_.IsEmpty()) {
    read_client_handler_->Start();
  }
  if (server_data_.IsEmpty()) {
    if (state_ == kStateTunnelData) {
      if (read_server_handler_.get()) {
        read_server_handler_->Start();
      } else {
        read_server_handler_.reset(dispatcher_->CreateInputHandler(
            server_socket_,
            read_server_callback_,
            Bind(&HTTPProxy::OnReadError, weak_ptr_factory_.GetWeakPtr())));
      }
    } else if (state_ == kStateFlushResponse) {
      StopClient();
      return;
    }
  }
  StartIdleTimeout();
}

// Start the various output-ready handlers for the endpoints we have
// data waiting for.
void HTTPProxy::StartTransmit() {
  if (state_ == kStateTunnelData && !client_data_.IsEmpty()) {
    if (write_server_handler_.get()) {
      write_server_handler_->Start();
    } else {
      write_server_handler_.reset(
          dispatcher_->CreateReadyHandler(server_socket_,
                                          IOHandler::kModeOutput,
                                          write_server_callback_));
    }
  }
  if ((state_ == kStateFlushResponse || state_ == kStateTunnelData) &&
      !server_data_.IsEmpty()) {
    if (write_client_handler_.get()) {
      write_client_handler_->Start();
    } else {
      write_client_handler_.reset(
          dispatcher_->CreateReadyHandler(client_socket_,
                                          IOHandler::kModeOutput,
                                          write_client_callback_));
    }
  }
  StartIdleTimeout();
}

// End the transaction with the current client, restart the IOHandler
// which alerts us to new clients connecting.  This function is called
// during various error conditions and is a callback for all timeouts.
void HTTPProxy::StopClient() {
  SLOG(connection_.get(), 3) << "In " << __func__;

  if (is_route_requested_) {
    connection_->ReleaseRouting();
    is_route_requested_ = false;
  }
  write_client_handler_.reset();
  read_client_handler_.reset();
  if (client_socket_ != -1) {
    sockets_->Close(client_socket_);
    client_socket_ = -1;
  }
  client_headers_.clear();
  client_method_.clear();
  client_version_.clear();
  server_port_ = kDefaultServerPort;
  write_server_handler_.reset();
  read_server_handler_.reset();
  if (server_socket_ != -1) {
    sockets_->Close(server_socket_);
    server_socket_ = -1;
  }
  server_hostname_.clear();
  client_data_.Clear();
  server_data_.Clear();
  dns_client_->Stop();
  server_async_connection_->Stop();
  idle_timeout_.Cancel();
  transaction_timeout_.Cancel();
  accept_handler_->Start();
  state_ = kStateWaitConnection;
}

// Output ReadyHandler callback which fires when the client socket is
// ready for data to be sent to it.
void HTTPProxy::WriteToClient(int fd) {
  CHECK_EQ(client_socket_, fd);
  int ret = sockets_->Send(fd, server_data_.GetConstData(),
                           server_data_.GetLength(), 0);
  SLOG(connection_.get(), 3) << "In " << __func__ << " wrote " << ret << " of "
                             << server_data_.GetLength();
  if (ret < 0) {
    LOG(ERROR) << "Server write failed";
    StopClient();
    return;
  }

  server_data_ = ByteString(server_data_.GetConstData() + ret,
                            server_data_.GetLength() - ret);

  if (server_data_.IsEmpty()) {
    write_client_handler_->Stop();
  }

  StartReceive();
}

// Output ReadyHandler callback which fires when the server socket is
// ready for data to be sent to it.
void HTTPProxy::WriteToServer(int fd) {
  CHECK_EQ(server_socket_, fd);
  int ret = sockets_->Send(fd, client_data_.GetConstData(),
                           client_data_.GetLength(), 0);
  SLOG(connection_.get(), 3) << "In " << __func__ << " wrote " << ret << " of "
                             << client_data_.GetLength();

  if (ret < 0) {
    LOG(ERROR) << "Client write failed";
    StopClient();
    return;
  }

  client_data_ = ByteString(client_data_.GetConstData() + ret,
                            client_data_.GetLength() - ret);

  if (client_data_.IsEmpty()) {
    write_server_handler_->Stop();
  }

  StartReceive();
}

}  // namespace shill
