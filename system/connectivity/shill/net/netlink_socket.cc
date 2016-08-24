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

#include "shill/net/netlink_socket.h"

#include <string>

#include <linux/if_packet.h>
#include <linux/netlink.h>
#include <sys/socket.h>

#include <base/logging.h>

#include "shill/net/netlink_message.h"
#include "shill/net/sockets.h"

// This is from a version of linux/socket.h that we don't have.
#define SOL_NETLINK 270

namespace shill {

// Keep this large enough to avoid overflows on IPv6 SNM routing update spikes
const int NetlinkSocket::kReceiveBufferSize = 512 * 1024;

NetlinkSocket::NetlinkSocket() : sequence_number_(0), file_descriptor_(-1) {}

NetlinkSocket::~NetlinkSocket() {
  if (sockets_ && (file_descriptor_ >= 0)) {
    sockets_->Close(file_descriptor_);
  }
}

bool NetlinkSocket::Init() {
  // Allows for a test to set |sockets_| before calling |Init|.
  if (sockets_) {
    LOG(INFO) << "|sockets_| already has a value -- this must be a test.";
  } else {
    sockets_.reset(new Sockets);
  }

  // The following is stolen directly from RTNLHandler.
  // TODO(wdg): refactor this and RTNLHandler together to use common code.
  // crbug.com/221940

  file_descriptor_ = sockets_->Socket(PF_NETLINK, SOCK_DGRAM, NETLINK_GENERIC);
  if (file_descriptor_ < 0) {
    LOG(ERROR) << "Failed to open netlink socket";
    return false;
  }

  if (sockets_->SetReceiveBuffer(file_descriptor_, kReceiveBufferSize)) {
    LOG(ERROR) << "Failed to increase receive buffer size";
  }

  struct sockaddr_nl addr;
  memset(&addr, 0, sizeof(addr));
  addr.nl_family = AF_NETLINK;

  if (sockets_->Bind(file_descriptor_,
                    reinterpret_cast<struct sockaddr*>(&addr),
                    sizeof(addr)) < 0) {
    sockets_->Close(file_descriptor_);
    file_descriptor_ = -1;
    LOG(ERROR) << "Netlink socket bind failed";
    return false;
  }
  VLOG(2) << "Netlink socket started";

  return true;
}

bool NetlinkSocket::RecvMessage(ByteString* message) {
  if (!message) {
    LOG(ERROR) << "Null |message|";
    return false;
  }

  // Determine the amount of data currently waiting.
  const size_t kDummyReadByteCount = 1;
  ByteString dummy_read(kDummyReadByteCount);
  ssize_t result;
  result = sockets_->RecvFrom(
      file_descriptor_,
      dummy_read.GetData(),
      dummy_read.GetLength(),
      MSG_TRUNC | MSG_PEEK,
      nullptr,
      nullptr);
  if (result < 0) {
    PLOG(ERROR) << "Socket recvfrom failed.";
    return false;
  }

  // Read the data that was waiting when we did our previous read.
  message->Resize(result);
  result = sockets_->RecvFrom(
      file_descriptor_,
      message->GetData(),
      message->GetLength(),
      0,
      nullptr,
      nullptr);
  if (result < 0) {
    PLOG(ERROR) << "Second socket recvfrom failed.";
    return false;
  }
  return true;
}

bool NetlinkSocket::SendMessage(const ByteString& out_msg) {
  ssize_t result = sockets_->Send(file_descriptor(), out_msg.GetConstData(),
                                  out_msg.GetLength(), 0);
  if (!result) {
    PLOG(ERROR) << "Send failed.";
    return false;
  }
  if (result != static_cast<ssize_t>(out_msg.GetLength())) {
    LOG(ERROR) << "Only sent " << result << " bytes out of "
               << out_msg.GetLength() << ".";
    return false;
  }

  return true;
}

bool NetlinkSocket::SubscribeToEvents(uint32_t group_id) {
  int err = setsockopt(file_descriptor_, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP,
                       &group_id, sizeof(group_id));
  if (err < 0) {
    PLOG(ERROR) << "setsockopt didn't work.";
    return false;
  }
  return true;
}

uint32_t NetlinkSocket::GetSequenceNumber() {
  if (++sequence_number_ == NetlinkMessage::kBroadcastSequenceNumber)
    ++sequence_number_;
  return sequence_number_;
}

}  // namespace shill.
