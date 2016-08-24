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

#ifndef SHILL_SHIMS_NETFILTER_QUEUE_PROCESSOR_H_
#define SHILL_SHIMS_NETFILTER_QUEUE_PROCESSOR_H_

#include <inttypes.h>
#include <sys/time.h>

#include <deque>
#include <memory>
#include <string>

#include <base/macros.h>

struct nfgenmsg;
struct nfq_data;
struct nfq_handle;
struct nfq_q_handle;

namespace shill {

namespace shims {

class NetfilterQueueProcessor {
 public:
  NetfilterQueueProcessor(int input_queue, int output_queue);
  virtual ~NetfilterQueueProcessor();

  // Run the main loop of the processor.
  void Run();

  // Initialize state and install the processor so it accepts messages
  // from the kernel.
  bool Start();

  // Uninitialize state.
  void Stop();

 private:
  friend class NetfilterQueueProcessorTest;

  class Packet {
   public:
    Packet();
    virtual ~Packet();

    // Inputs a netfilter data packet and reads meta-information (packet id)
    // and attempts to decode the payload as a UDP packet.  Returns true if
    // the meta-information is decoded, regardless of whether the payload
    // was decoded.
    bool ParseNetfilterData(struct nfq_data* netfilter_data);

    // Getters.
    int in_device() const { return in_device_; }
    int out_device() const { return out_device_; }
    bool is_udp() const { return is_udp_; }
    uint32_t packet_id() const { return packet_id_; }
    uint32_t source_ip() const { return source_ip_; }
    uint32_t destination_ip() const { return destination_ip_; }
    uint16_t source_port() const { return source_port_; }
    uint16_t destination_port() const { return destination_port_; }

   private:
    friend class NetfilterQueueProcessorTest;

    bool ParsePayloadUDPData(const unsigned char* payload, size_t payload_len);

    // Setter only used in unit tests.
    void SetValues(int in_device,
                   int out_device,
                   bool is_udp,
                   uint32_t packet_id,
                   uint32_t source_ip,
                   uint32_t destination_ip,
                   uint16_t source_port,
                   uint16_t destination_port);

    uint32_t packet_id_;
    int in_device_;
    int out_device_;
    bool is_udp_;
    uint32_t source_ip_;
    uint32_t destination_ip_;
    uint16_t source_port_;
    uint16_t destination_port_;

    DISALLOW_COPY_AND_ASSIGN(Packet);
  };

  struct ListenerEntry {
    ListenerEntry()
        : last_transmission(0),
          port(0),
          device_index(0),
          address(0),
          netmask(0),
          destination(0) {}
    ListenerEntry(time_t last_transmission_in,
                  uint16_t port_in,
                  int device_index_in,
                  uint32_t address_in,
                  uint32_t netmask_in,
                  uint32_t destination_in)
        : last_transmission(last_transmission_in),
          port(port_in),
          device_index(device_index_in),
          address(address_in),
          netmask(netmask_in),
          destination(destination_in) {}
    time_t last_transmission;
    uint16_t port;
    int device_index;
    uint32_t address;
    uint32_t netmask;
    uint32_t destination;
  };

  typedef std::shared_ptr<ListenerEntry> ListenerEntryPtr;

  // Called by the netlink_queue code when a packet arrives for the
  // input queue.
  static int InputQueueCallback(struct nfq_q_handle* queue_handle,
                                struct nfgenmsg* generic_message,
                                struct nfq_data* netfilter_data,
                                void* private_data);

  // Called by the netlink_queue code when a packet arrives for the
  // output queue.
  static int OutputQueueCallback(struct nfq_q_handle* queue_handle,
                                 struct nfgenmsg* generic_message,
                                 struct nfq_data* netfilter_data,
                                 void* private_data);

  // Return the netmask associated with |device_index|.
  static uint32_t GetNetmaskForDevice(int device_index);

  // Expire listener that are no longer valid |now|.
  void ExpireListeners(time_t now);

  // Find a listener entry with port |port|, device index |device_index|
  // and local address |address|.
  std::deque<ListenerEntryPtr>::iterator FindListener(
      uint16_t port, int device_index, uint32_t address);

  // Find a listener entry with port |port| and device index |device_index|
  // which transmitted to multicast destination |destination|.
  std::deque<ListenerEntryPtr>::iterator FindDestination(
      uint16_t port, int device_index, uint32_t destination);

  // Returns true if incoming packet |packet| should be allowed to pass.
  bool IsIncomingPacketAllowed(const Packet& packet, time_t now);

  // Log the transmission of an outgoing packet.
  void LogOutgoingPacket(const Packet& packet, time_t now);

  static std::string AddressAndPortToString(uint32_t ip, uint16_t port);

  // Size of the packet buffer passed to the netlink queue library.
  static const int kBufferSize;
  // The number of seconds after which we should forget about a listener.
  static const int kExpirationIntervalSeconds;
  // Number of bytes in a single unit of IP header length.
  static const int kIPHeaderLengthUnitBytes;
  // The maximum expected value for the "header length" element of the IP
  // header, in units of kIPHeaderLengthUnitBytes bytes.
  static const int kMaxIPHeaderLength;
  // The maximum number of listeners that we keep track of.
  static const size_t kMaxListenerEntries;
  // Number of bytes of the network payload we are interested in seeing.
  static const int kPayloadCopySize;

  // Input and output queue numbers.
  int input_queue_;
  int output_queue_;

  // Pointer to a netfilter queue library instance.  A bare pointer is
  // necessary since this must be freed via nfq_close().
  struct nfq_handle* nfq_handle_;

  // Input and output queue handles.  A bare pointer is necessary since
  // this must be freed via nfq_destroy_queue().
  struct nfq_q_handle* input_queue_handle_;
  struct nfq_q_handle* output_queue_handle_;

  // A list of records of listening sockets.
  std::deque<ListenerEntryPtr> listeners_;

  DISALLOW_COPY_AND_ASSIGN(NetfilterQueueProcessor);
};

}  // namespace shims

}  // namespace shill

#endif  // SHILL_SHIMS_NETFILTER_QUEUE_PROCESSOR_H_

