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

#include "shill/async_connection.h"

#include <base/bind.h>
#include <errno.h>
#include <netinet/in.h>

#include <string>

#include "shill/event_dispatcher.h"
#include "shill/net/ip_address.h"
#include "shill/net/sockets.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;

namespace shill {

AsyncConnection::AsyncConnection(const string& interface_name,
                                 EventDispatcher* dispatcher,
                                 Sockets* sockets,
                                 const Callback<void(bool, int)>& callback)
    : interface_name_(interface_name),
      dispatcher_(dispatcher),
      sockets_(sockets),
      callback_(callback),
      fd_(-1),
      connect_completion_callback_(
          Bind(&AsyncConnection::OnConnectCompletion, Unretained(this))) { }

AsyncConnection::~AsyncConnection() {
  Stop();
}

bool AsyncConnection::Start(const IPAddress& address, int port) {
  DCHECK_LT(fd_, 0);

  int family = PF_INET;
  if (address.family() == IPAddress::kFamilyIPv6) {
    family = PF_INET6;
  }
  fd_ = sockets_->Socket(family, SOCK_STREAM, 0);
  if (fd_ < 0 ||
      sockets_->SetNonBlocking(fd_) < 0) {
    error_ = sockets_->ErrorString();
    PLOG(ERROR) << "Async socket setup failed";
    Stop();
    return false;
  }

  if (!interface_name_.empty() &&
      sockets_->BindToDevice(fd_, interface_name_) < 0) {
    error_ = sockets_->ErrorString();
    PLOG(ERROR) << "Async socket failed to bind to device";
    Stop();
    return false;
  }

  int ret = ConnectTo(address, port);
  if (ret == 0) {
    callback_.Run(true, fd_);  // Passes ownership
    fd_ = -1;
    return true;
  }

  if (sockets_->Error() != EINPROGRESS) {
    error_ = sockets_->ErrorString();
    PLOG(ERROR) << "Async socket connection failed";
    Stop();
    return false;
  }

  connect_completion_handler_.reset(
      dispatcher_->CreateReadyHandler(fd_,
                                      IOHandler::kModeOutput,
                                      connect_completion_callback_));
  error_ = string();

  return true;
}

void AsyncConnection::Stop() {
  connect_completion_handler_.reset();
  if (fd_ >= 0) {
    sockets_->Close(fd_);
    fd_ = -1;
  }
}

void AsyncConnection::OnConnectCompletion(int fd) {
  CHECK_EQ(fd_, fd);
  bool success = false;
  int returned_fd = -1;

  if (sockets_->GetSocketError(fd_) != 0) {
    error_ = sockets_->ErrorString();
    PLOG(ERROR) << "Async GetSocketError returns failure";
  } else {
    returned_fd = fd_;
    fd_ = -1;
    success = true;
  }
  Stop();

  // Run the callback last, since it may end up freeing this instance.
  callback_.Run(success, returned_fd);  // Passes ownership
}

int AsyncConnection::ConnectTo(const IPAddress& address, int port) {
  struct sockaddr* sock_addr = nullptr;
  socklen_t addr_len = 0;
  struct sockaddr_in iaddr;
  struct sockaddr_in6 iaddr6;
  if (address.family() == IPAddress::kFamilyIPv4) {
    CHECK_EQ(sizeof(iaddr.sin_addr.s_addr), address.GetLength());

    memset(&iaddr, 0, sizeof(iaddr));
    iaddr.sin_family = AF_INET;
    memcpy(&iaddr.sin_addr.s_addr, address.address().GetConstData(),
           sizeof(iaddr.sin_addr.s_addr));
    iaddr.sin_port = htons(port);

    sock_addr = reinterpret_cast<struct sockaddr*>(&iaddr);
    addr_len = sizeof(iaddr);
  } else if (address.family() == IPAddress::kFamilyIPv6) {
    CHECK_EQ(sizeof(iaddr6.sin6_addr.s6_addr), address.GetLength());

    memset(&iaddr6, 0, sizeof(iaddr6));
    iaddr6.sin6_family = AF_INET6;
    memcpy(&iaddr6.sin6_addr.s6_addr, address.address().GetConstData(),
           sizeof(iaddr6.sin6_addr.s6_addr));
    iaddr6.sin6_port = htons(port);

    sock_addr = reinterpret_cast<struct sockaddr*>(&iaddr6);
    addr_len = sizeof(iaddr6);
  } else {
    NOTREACHED();
  }

  return sockets_->Connect(fd_, sock_addr, addr_len);
}

}  // namespace shill
