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

#ifndef SHILL_ARP_CLIENT_H_
#define SHILL_ARP_CLIENT_H_

#include <memory>

#include <base/macros.h>

namespace shill {

class ArpPacket;
class ByteString;
class Sockets;
class ScopedSocketCloser;

// ArpClient task of creating ARP-capable sockets, as well as
// transmitting requests on and receiving responses from such
// sockets.
class ArpClient {
 public:
  explicit ArpClient(int interface_index);
  virtual ~ArpClient();

  // Create a socket for reception of ARP replies, and packet trasmission.
  // Returns true if successful, false otherwise.
  virtual bool StartReplyListener();

  // Create a socket for reception of ARP requests, and packet trasmission.
  // Returns true if successful, false otherwise.
  virtual bool StartRequestListener();

  // Destroy the client socket.
  virtual void Stop();

  // Receive an ARP request or reply and parse its contents into |packet|.
  // Also return the sender's MAC address (which may be different from the
  // MAC address in the ARP response) in |sender|.  Returns true on
  // succes, false otherwise.
  virtual bool ReceivePacket(ArpPacket* packet, ByteString* sender) const;

  // Send a formatted ARP request from |packet|.  Returns true on
  // success, false otherwise.
  virtual bool TransmitRequest(const ArpPacket& packet) const;

  virtual int socket() const { return socket_; }

  bool IsStarted() { return socket_closer_.get(); }

 private:
  friend class ArpClientTest;

  // Offset of the ARP OpCode within a captured ARP packet.
  static const size_t kArpOpOffset;

  // The largest packet we expect to receive as an ARP client.
  static const size_t kMaxArpPacketLength;

  // Start an ARP listener that listens for |arp_opcode| ARP packets.
  bool Start(uint16_t arp_opcode);
  bool CreateSocket(uint16_t arp_opcode);

  const int interface_index_;
  std::unique_ptr<Sockets> sockets_;
  std::unique_ptr<ScopedSocketCloser> socket_closer_;
  int socket_;

  DISALLOW_COPY_AND_ASSIGN(ArpClient);
};

}  // namespace shill

#endif  // SHILL_ARP_CLIENT_H_
