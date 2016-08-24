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
//
// This code is derived from the 'iw' source code.  The copyright and license
// of that code is as follows:
//
// Copyright (c) 2007, 2008  Johannes Berg
// Copyright (c) 2007  Andy Lutomirski
// Copyright (c) 2007  Mike Kershaw
// Copyright (c) 2008-2009  Luis R. Rodriguez
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef SHILL_NET_NETLINK_SOCKET_H_
#define SHILL_NET_NETLINK_SOCKET_H_

#include <memory>

#include <base/bind.h>
#include <base/logging.h>
#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/net/shill_export.h"

namespace shill {

class Sockets;
class ByteString;

// Provides an abstraction to a netlink socket.  See
// http://www.infradead.org/~tgr/libnl/doc/core.html#core_netlink_fundamentals
// for documentation on how netlink sockets work (note that most of the rest of
// this document discusses libnl -- something not used by this code).
class SHILL_EXPORT NetlinkSocket {
 public:
  static const int kReceiveBufferSize;

  NetlinkSocket();
  virtual ~NetlinkSocket();

  // Non-trivial initialization.
  bool Init();

  // Returns the file descriptor used by the socket.
  virtual int file_descriptor() const { return file_descriptor_; }

  // Get the next message sequence number for this socket.
  // |GetSequenceNumber| won't return zero because that is the 'broadcast'
  // sequence number.
  virtual uint32_t GetSequenceNumber();

  // Reads data from the socket into |message| and returns true if successful.
  // The |message| parameter will be resized to hold the entirety of the read
  // message (and any data in |message| will be overwritten).
  virtual bool RecvMessage(ByteString* message);

  // Sends a message, returns true if successful.
  virtual bool SendMessage(const ByteString& message);

  // Subscribes to netlink broadcast events.
  virtual bool SubscribeToEvents(uint32_t group_id);

  virtual const Sockets* sockets() const { return sockets_.get(); }

 protected:
  uint32_t sequence_number_;

 private:
  friend class NetlinkManagerTest;
  friend class NetlinkSocketTest;
  FRIEND_TEST(NetlinkSocketTest, SequenceNumberTest);

  std::unique_ptr<Sockets> sockets_;
  int file_descriptor_;

  DISALLOW_COPY_AND_ASSIGN(NetlinkSocket);
};

}  // namespace shill

#endif  // SHILL_NET_NETLINK_SOCKET_H_
