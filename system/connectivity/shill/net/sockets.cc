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

#include "shill/net/sockets.h"

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <base/logging.h>
#include <base/posix/eintr_wrapper.h>

namespace shill {

Sockets::Sockets() {}

Sockets::~Sockets() {}

// Some system calls can be interrupted and return EINTR, but will succeed on
// retry.  The HANDLE_EINTR macro retries a call if it returns EINTR.  For a
// list of system calls that can return EINTR, see 'man 7 signal' under the
// heading "Interruption of System Calls and Library Functions by Signal
// Handlers".

int Sockets::Accept(int sockfd,
                    struct sockaddr* addr,
                    socklen_t* addrlen) const {
  return HANDLE_EINTR(accept(sockfd, addr, addrlen));
}

int Sockets::AttachFilter(int sockfd, struct sock_fprog* pf) const {
  return setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, pf, sizeof(*pf));
}

int Sockets::Bind(int sockfd,
                  const struct sockaddr* addr,
                  socklen_t addrlen) const {
  return bind(sockfd, addr, addrlen);
}

int Sockets::BindToDevice(int sockfd, const std::string& device) const {
  char dev_name[IFNAMSIZ];
  CHECK_GT(sizeof(dev_name), device.length());
  memset(&dev_name, 0, sizeof(dev_name));
  snprintf(dev_name, sizeof(dev_name), "%s", device.c_str());
  return setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &dev_name,
                    sizeof(dev_name));
}

int Sockets::ReuseAddress(int sockfd) const {
  int value = 1;
  return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

int Sockets::AddMulticastMembership(int sockfd, in_addr_t addr) const {
  ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = addr;
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  return setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}

int Sockets::Close(int fd) const {
  return IGNORE_EINTR(close(fd));
}

int Sockets::Connect(int sockfd,
                     const struct sockaddr* addr,
                     socklen_t addrlen) const {
  return HANDLE_EINTR(connect(sockfd, addr, addrlen));
}

int Sockets::Error() const {
  return errno;
}

std::string Sockets::ErrorString() const {
  return std::string(strerror(Error()));
}

int Sockets::GetSockName(int sockfd,
                         struct sockaddr* addr,
                         socklen_t* addrlen) const {
  return getsockname(sockfd, addr, addrlen);
}


int Sockets::GetSocketError(int sockfd) const {
  int error;
  socklen_t optlen = sizeof(error);
  if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &optlen) == 0) {
    return error;
  }
  return -1;
}


int Sockets::Ioctl(int d, int request, void* argp) const {
  return HANDLE_EINTR(ioctl(d, request, argp));
}

int Sockets::Listen(int sockfd, int backlog) const {
  return listen(sockfd, backlog);
}

ssize_t Sockets::RecvFrom(int sockfd,
                          void* buf,
                          size_t len,
                          int flags,
                          struct sockaddr* src_addr,
                          socklen_t* addrlen) const {
  return HANDLE_EINTR(recvfrom(sockfd, buf, len, flags, src_addr, addrlen));
}

int Sockets::Select(int nfds,
                    fd_set* readfds,
                    fd_set* writefds,
                    fd_set* exceptfds,
                    struct timeval* timeout) const {
  return HANDLE_EINTR(select(nfds, readfds, writefds, exceptfds, timeout));
}

ssize_t Sockets::Send(int sockfd,
                      const void* buf,
                      size_t len,
                      int flags) const {
  return HANDLE_EINTR(send(sockfd, buf, len, flags));
}

ssize_t Sockets::SendTo(int sockfd,
                        const void* buf,
                        size_t len,
                        int flags,
                        const struct sockaddr* dest_addr,
                        socklen_t addrlen) const {
  return HANDLE_EINTR(sendto(sockfd, buf, len, flags, dest_addr, addrlen));
}

int Sockets::SetNonBlocking(int sockfd) const {
  return HANDLE_EINTR(
      fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK));
}

int Sockets::SetReceiveBuffer(int sockfd, int size) const {
  // Note: kernel will set buffer to 2*size to allow for struct skbuff overhead
  return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUFFORCE, &size, sizeof(size));
}

int Sockets::ShutDown(int sockfd, int how) const {
  return HANDLE_EINTR(shutdown(sockfd, how));
}

int Sockets::Socket(int domain, int type, int protocol) const {
  return socket(domain, type, protocol);
}

ScopedSocketCloser::ScopedSocketCloser(Sockets* sockets, int fd)
    : sockets_(sockets),
      fd_(fd) {}

ScopedSocketCloser::~ScopedSocketCloser() {
  sockets_->Close(fd_);
  fd_ = Sockets::kInvalidFileDescriptor;
}

int ScopedSocketCloser::Release() {
  int fd = fd_;
  fd_ = Sockets::kInvalidFileDescriptor;
  return fd;
}

}  // namespace shill
