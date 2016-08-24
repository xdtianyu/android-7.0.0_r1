// Copyright 2015 The Android Open Source Project
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

#include <arpa/inet.h>
#include <map>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/files/file_util.h>
#include <base/message_loop/message_loop.h>
#include <base/strings/stringprintf.h>
#include <brillo/bind_lambda.h>
#include <brillo/streams/file_stream.h>
#include <brillo/streams/tls_stream.h>

#include "buffet/socket_stream.h"
#include "buffet/weave_error_conversion.h"

namespace buffet {

using weave::provider::Network;

namespace {

std::string GetIPAddress(const sockaddr* sa) {
  std::string addr;
  char str[INET6_ADDRSTRLEN] = {};
  switch (sa->sa_family) {
    case AF_INET:
      if (inet_ntop(AF_INET,
                    &(reinterpret_cast<const sockaddr_in*>(sa)->sin_addr), str,
                    sizeof(str))) {
        addr = str;
      }
      break;

    case AF_INET6:
      if (inet_ntop(AF_INET6,
                    &(reinterpret_cast<const sockaddr_in6*>(sa)->sin6_addr),
                    str, sizeof(str))) {
        addr = str;
      }
      break;
  }
  if (addr.empty())
    addr = base::StringPrintf("<Unknown address family: %d>", sa->sa_family);
  return addr;
}

int ConnectSocket(const std::string& host, uint16_t port) {
  std::string service = std::to_string(port);
  addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM};
  addrinfo* result = nullptr;
  if (getaddrinfo(host.c_str(), service.c_str(), &hints, &result)) {
    PLOG(WARNING) << "Failed to resolve host name: " << host;
    return -1;
  }

  int socket_fd = -1;
  for (const addrinfo* info = result; info != nullptr; info = info->ai_next) {
    socket_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (socket_fd < 0)
      continue;

    std::string addr = GetIPAddress(info->ai_addr);
    LOG(INFO) << "Connecting to address: " << addr;
    if (connect(socket_fd, info->ai_addr, info->ai_addrlen) == 0)
      break;  // Success.

    PLOG(WARNING) << "Failed to connect to address: " << addr;
    close(socket_fd);
    socket_fd = -1;
  }

  freeaddrinfo(result);
  return socket_fd;
}

void OnSuccess(const Network::OpenSslSocketCallback& callback,
               brillo::StreamPtr tls_stream) {
  callback.Run(
      std::unique_ptr<weave::Stream>{new SocketStream{std::move(tls_stream)}},
      nullptr);
}

void OnError(const weave::DoneCallback& callback,
             const brillo::Error* brillo_error) {
  weave::ErrorPtr error;
  ConvertError(*brillo_error, &error);
  callback.Run(std::move(error));
}

}  // namespace

void SocketStream::Read(void* buffer,
                        size_t size_to_read,
                        const ReadCallback& callback) {
  brillo::ErrorPtr brillo_error;
  if (!ptr_->ReadAsync(
          buffer, size_to_read,
          base::Bind([](const ReadCallback& callback,
                        size_t size) { callback.Run(size, nullptr); },
                     callback),
          base::Bind(&OnError, base::Bind(callback, 0)), &brillo_error)) {
    weave::ErrorPtr error;
    ConvertError(*brillo_error, &error);
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, 0, base::Passed(&error)));
  }
}

void SocketStream::Write(const void* buffer,
                         size_t size_to_write,
                         const WriteCallback& callback) {
  brillo::ErrorPtr brillo_error;
  if (!ptr_->WriteAllAsync(buffer, size_to_write, base::Bind(callback, nullptr),
                           base::Bind(&OnError, callback), &brillo_error)) {
    weave::ErrorPtr error;
    ConvertError(*brillo_error, &error);
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)));
  }
}

void SocketStream::CancelPendingOperations() {
  ptr_->CancelPendingAsyncOperations();
}

std::unique_ptr<weave::Stream> SocketStream::ConnectBlocking(
    const std::string& host,
    uint16_t port) {
  int socket_fd = ConnectSocket(host, port);
  if (socket_fd <= 0)
    return nullptr;

  auto ptr_ = brillo::FileStream::FromFileDescriptor(socket_fd, true, nullptr);
  if (ptr_)
    return std::unique_ptr<Stream>{new SocketStream{std::move(ptr_)}};

  close(socket_fd);
  return nullptr;
}

void SocketStream::TlsConnect(std::unique_ptr<Stream> socket,
                              const std::string& host,
                              const Network::OpenSslSocketCallback& callback) {
  SocketStream* stream = static_cast<SocketStream*>(socket.get());
  brillo::TlsStream::Connect(
      std::move(stream->ptr_), host, base::Bind(&OnSuccess, callback),
      base::Bind(&OnError, base::Bind(callback, nullptr)));
}

}  // namespace buffet
