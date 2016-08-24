//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/shims/netfilter_queue_processor.h"

#include <arpa/inet.h>
#include <errno.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/ip.h>
#include <linux/netfilter.h>    /* for NF_ACCEPT */
#include <linux/types.h>
#include <linux/udp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <deque>

#include <base/files/scoped_file.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>

using std::deque;

namespace shill {

namespace shims {

// static
const int NetfilterQueueProcessor::kBufferSize = 4096;
const int NetfilterQueueProcessor::kExpirationIntervalSeconds = 5;
const int NetfilterQueueProcessor::kIPHeaderLengthUnitBytes = 4;
const int NetfilterQueueProcessor::kMaxIPHeaderLength =
    16;  // ihl is a 4-bit field.
const size_t NetfilterQueueProcessor::kMaxListenerEntries = 32;
const int NetfilterQueueProcessor::kPayloadCopySize = 0xffff;

NetfilterQueueProcessor::Packet::Packet()
    : packet_id_(0),
      in_device_(0),
      out_device_(0),
      is_udp_(false),
      source_ip_(INADDR_ANY),
      destination_ip_(INADDR_ANY),
      source_port_(0),
      destination_port_(0) {}

NetfilterQueueProcessor::Packet::~Packet() {}

std::string NetfilterQueueProcessor::AddressAndPortToString(uint32_t ip,
                                                            uint16_t port) {
  struct in_addr addr;
  addr.s_addr = htonl(ip);
  return base::StringPrintf("%s:%d", inet_ntoa(addr), port);
}

bool NetfilterQueueProcessor::Packet::ParseNetfilterData(
    struct nfq_data* netfilter_data) {
  struct nfqnl_msg_packet_hdr* packet_header =
      nfq_get_msg_packet_hdr(netfilter_data);
  if (!packet_header) {
    return false;
  }
  packet_id_ = ntohl(packet_header->packet_id);
  in_device_ = nfq_get_indev(netfilter_data);
  out_device_ = nfq_get_outdev(netfilter_data);

  unsigned char* payload;
  int payload_len = nfq_get_payload(netfilter_data, &payload);
  if (payload_len >= 0) {
    is_udp_ = ParsePayloadUDPData(payload, payload_len);
  }

  return true;
}

bool NetfilterQueueProcessor::Packet::ParsePayloadUDPData(
    const unsigned char* payload, size_t payload_len) {
  struct iphdr ip;

  if (payload_len <= sizeof(ip)) {
    return false;
  }

  memcpy(&ip, payload, sizeof(ip));

  size_t iphdr_len = ip.ihl * kIPHeaderLengthUnitBytes;
  if (iphdr_len < sizeof(ip) ||
      ip.version != IPVERSION ||
      ip.protocol != IPPROTO_UDP) {
    return false;
  }

  struct udphdr udp;
  if (payload_len < iphdr_len + sizeof(udp)) {
    return false;
  }

  memcpy(&udp, payload + iphdr_len, sizeof(udp));

  source_ip_ = ntohl(ip.saddr);
  destination_ip_ = ntohl(ip.daddr);
  source_port_ = ntohs(udp.source);
  destination_port_ = ntohs(udp.dest);

  return true;
}

void NetfilterQueueProcessor::Packet::SetValues(int in_device,
                                                int out_device,
                                                bool is_udp,
                                                uint32_t packet_id,
                                                uint32_t source_ip,
                                                uint32_t destination_ip,
                                                uint16_t source_port,
                                                uint16_t destination_port) {
  in_device_ = in_device;
  out_device_ = out_device;
  is_udp_ = is_udp;
  packet_id_ = packet_id;
  source_ip_ = source_ip;
  destination_ip_ = destination_ip;
  source_port_ = source_port;
  destination_port_ = destination_port;
}

NetfilterQueueProcessor::NetfilterQueueProcessor(
    int input_queue, int output_queue)
    : input_queue_(input_queue),
      output_queue_(output_queue),
      nfq_handle_(NULL),
      input_queue_handle_(NULL),
      output_queue_handle_(NULL)  {
  VLOG(2) << "Created netfilter queue processor.";
}

NetfilterQueueProcessor::~NetfilterQueueProcessor() {
  Stop();
}

void NetfilterQueueProcessor::Run() {
  LOG(INFO) << "Netfilter queue processor running.";
  CHECK(nfq_handle_);

  int file_handle = nfq_fd(nfq_handle_);
  char buffer[kBufferSize] __attribute__((aligned));

  for (;;) {
    int receive_count = recv(file_handle, buffer, sizeof(buffer), 0);
    if (receive_count <= 0) {
      if (receive_count < 0 && errno == ENOBUFS) {
        LOG(WARNING) << "Packets dropped in the queue.";
        continue;
      }
      LOG(ERROR) << "Receive failed; exiting";
      break;
    }

    nfq_handle_packet(nfq_handle_, buffer, receive_count);
  }
}

bool NetfilterQueueProcessor::Start() {
  VLOG(2) << "Netfilter queue processor starting.";
  if (!nfq_handle_) {
    nfq_handle_ = nfq_open();
    if (!nfq_handle_) {
      LOG(ERROR) << "nfq_open() returned an error";
      return false;
    }
  }

  if (nfq_unbind_pf(nfq_handle_, AF_INET) < 0) {
    LOG(ERROR) << "nfq_unbind_pf() returned an error";
    return false;
  }

  if (nfq_bind_pf(nfq_handle_, AF_INET) < 0) {
    LOG(ERROR) << "nfq_bind_pf() returned an error";
    return false;
  }

  input_queue_handle_ = nfq_create_queue(
      nfq_handle_, input_queue_,
      &NetfilterQueueProcessor::InputQueueCallback, this);
  if (!input_queue_handle_) {
    LOG(ERROR) << "nfq_create_queue() failed for input queue " << input_queue_;
    return false;
  }

  if (nfq_set_mode(input_queue_handle_, NFQNL_COPY_PACKET,
                   kPayloadCopySize) < 0) {
    LOG(ERROR) << "nfq_set_mode() failed: can't set input queue packet_copy.";
    return false;
  }

  output_queue_handle_ = nfq_create_queue(
      nfq_handle_, output_queue_,
      &NetfilterQueueProcessor::OutputQueueCallback, this);
  if (!output_queue_handle_) {
    LOG(ERROR) << "nfq_create_queue() failed for output queue "
               << output_queue_;
    return false;
  }

  if (nfq_set_mode(output_queue_handle_, NFQNL_COPY_PACKET,
                   kPayloadCopySize) < 0) {
    LOG(ERROR) << "nfq_set_mode() failed: can't set output queue packet_copy.";
    return false;
  }

  return true;
}

void NetfilterQueueProcessor::Stop() {
  if (input_queue_handle_) {
    nfq_destroy_queue(input_queue_handle_);
    input_queue_handle_ = NULL;
  }

  if (output_queue_handle_) {
    nfq_destroy_queue(output_queue_handle_);
    output_queue_handle_ = NULL;
  }

  if (nfq_handle_) {
    nfq_close(nfq_handle_);
    nfq_handle_ = NULL;
  }
}

// static
int NetfilterQueueProcessor::InputQueueCallback(
    struct nfq_q_handle* queue_handle,
    struct nfgenmsg* generic_message,
    struct nfq_data* netfilter_data,
    void* private_data) {
  Packet packet;
  if (!packet.ParseNetfilterData(netfilter_data)) {
    LOG(FATAL) << "Unable to parse netfilter data.";
  }

  NetfilterQueueProcessor* processor =
      reinterpret_cast<NetfilterQueueProcessor*>(private_data);
  uint32_t verdict;
  time_t now = time(NULL);
  if (processor->IsIncomingPacketAllowed(packet, now)) {
    verdict = NF_ACCEPT;
  } else {
    verdict = NF_DROP;
  }
  return nfq_set_verdict(queue_handle, packet.packet_id(), verdict, 0, NULL);
}

// static
int NetfilterQueueProcessor::OutputQueueCallback(
    struct nfq_q_handle* queue_handle,
    struct nfgenmsg* generic_message,
    struct nfq_data* netfilter_data,
    void* private_data) {
  Packet packet;
  if (!packet.ParseNetfilterData(netfilter_data)) {
    LOG(FATAL) << "Unable to get parse netfilter data.";
  }

  NetfilterQueueProcessor* processor =
      reinterpret_cast<NetfilterQueueProcessor*>(private_data);
  time_t now = time(NULL);
  processor->LogOutgoingPacket(packet, now);
  return nfq_set_verdict(queue_handle, packet.packet_id(), NF_ACCEPT, 0, NULL);
}

// static
uint32_t NetfilterQueueProcessor::GetNetmaskForDevice(int device_index) {
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  if (if_indextoname(device_index, ifr.ifr_name) != ifr.ifr_name) {
    return INADDR_NONE;
  }

  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    return INADDR_NONE;
  }

  base::ScopedFD scoped_fd(socket_fd);

  if (ioctl(socket_fd, SIOCGIFNETMASK, &ifr) != 0) {
    return INADDR_NONE;
  }

  struct sockaddr_in* netmask_addr =
      reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_netmask);
  return ntohl(netmask_addr->sin_addr.s_addr);
}

void NetfilterQueueProcessor::ExpireListeners(time_t now) {
  time_t expiration_threshold = now - kExpirationIntervalSeconds;
  VLOG(2) << __func__ << " entered.";
  while (!listeners_.empty()) {
    const ListenerEntryPtr& last_listener = listeners_.back();
    if (last_listener->last_transmission >= expiration_threshold &&
        listeners_.size() <= kMaxListenerEntries) {
      break;
    }
    VLOG(2) << "Expired listener for "
            << AddressAndPortToString(last_listener->address,
                                      last_listener->port);
    listeners_.pop_back();
  }
}

deque<NetfilterQueueProcessor::ListenerEntryPtr>::iterator
    NetfilterQueueProcessor::FindListener(uint16_t port,
                                          int device_index,
                                          uint32_t address) {
  deque<ListenerEntryPtr>::iterator it;
  for (it = listeners_.begin(); it != listeners_.end(); ++it) {
    if ((*it)->port == port &&
        (*it)->device_index == device_index &&
        (*it)->address == address) {
      break;
    }
  }
  return it;
}

deque<NetfilterQueueProcessor::ListenerEntryPtr>::iterator
    NetfilterQueueProcessor::FindDestination(uint16_t port,
                                             int device_index,
                                             uint32_t destination) {
  deque<ListenerEntryPtr>::iterator it;
  for (it = listeners_.begin(); it != listeners_.end(); ++it) {
    if ((*it)->port == port &&
        (*it)->device_index == device_index &&
        (*it)->destination == destination) {
      break;
    }
  }
  return it;
}

bool NetfilterQueueProcessor::IsIncomingPacketAllowed(
    const Packet& packet, time_t now) {
  VLOG(2) << __func__ << " entered.";
  VLOG(3) << "Incoming packet is from "
          << AddressAndPortToString(packet.source_ip(),
                                    packet.source_port())
          << " and to "
          << AddressAndPortToString(packet.destination_ip(),
                                    packet.destination_port());
  if (!packet.is_udp()) {
    VLOG(2) << "Incoming packet is not udp.";
    return false;
  }

  ExpireListeners(now);

  uint16_t port = packet.destination_port();
  uint32_t address = packet.destination_ip();
  int device_index = packet.in_device();

  deque<ListenerEntryPtr>::iterator entry_ptr = listeners_.end();
  if (IN_MULTICAST(address)) {
    VLOG(2) << "Incoming packet is multicast.";
    entry_ptr = FindDestination(port, device_index, address);
  } else {
    entry_ptr = FindListener(port, device_index, address);
  }

  if (entry_ptr == listeners_.end()) {
    VLOG(2) << "Incoming does not match any listener.";
    return false;
  }

  uint32_t netmask = (*entry_ptr)->netmask;
  if ((packet.source_ip() & netmask) != ((*entry_ptr)->address & netmask)) {
    VLOG(2) << "Incoming packet is from a non-local address.";
    return false;
  }

  VLOG(3) << "Accepting packet.";
  return true;
}

void NetfilterQueueProcessor::LogOutgoingPacket(
    const Packet& packet, time_t now) {
  VLOG(2) << __func__ << " entered.";
  if (!packet.is_udp()) {
    VLOG(2) << "Outgoing packet is not udp.";
    return;
  }
  if (!IN_MULTICAST(packet.destination_ip())) {
    VLOG(2) << "Outgoing packet is not multicast.";
    return;
  }
  int device_index = packet.out_device();
  if (device_index == 0) {
    VLOG(2) << "Outgoing packet is not assigned a valid device.";
    return;
  }
  uint16_t port = packet.source_port();
  uint32_t address = packet.source_ip();
  uint32_t destination = 0;
  // Allow multicast replies if the destination port of the packet is the
  // same as the port the sender transmitted from;
  if (packet.source_port() == packet.destination_port()) {
    destination = packet.destination_ip();
  }
  deque<ListenerEntryPtr>::iterator entry_it =
      FindListener(port, device_index, address);
  if (entry_it != listeners_.end()) {
    if (entry_it != listeners_.begin()) {
      // Make this the newest entry.
      ListenerEntryPtr entry_ptr = *entry_it;
      listeners_.erase(entry_it);
      listeners_.push_front(entry_ptr);
      entry_it = listeners_.begin();
    }
    (*entry_it)->last_transmission = now;
  } else {
    uint32_t netmask = GetNetmaskForDevice(device_index);
    ListenerEntryPtr entry_ptr(
        new ListenerEntry(now, port, device_index,
                          address, netmask, destination));
    listeners_.push_front(entry_ptr);
    VLOG(2) << "Added listener for " << AddressAndPortToString(address, port)
            << " with destination "
            << AddressAndPortToString(destination, port);
  }

  // Perform expiration at the end, so that we don't end up expiring something
  // just to resurrect it again.
  ExpireListeners(now);
}

}  // namespace shims

}  // namespace shill

