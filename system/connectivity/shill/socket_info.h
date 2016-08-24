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

#ifndef SHILL_SOCKET_INFO_H_
#define SHILL_SOCKET_INFO_H_

#include <base/macros.h>

#include "shill/net/ip_address.h"

namespace shill {

class SocketInfo {
 public:
  // These connection states (except kConnectionStateUnknown and
  // kConnectionStateMax) are equivalent to and should be kept in sync with
  // those defined in kernel/inlude/net/tcp_states.h
  enum ConnectionState {
    kConnectionStateUnknown = -1,
    kConnectionStateEstablished = 1,
    kConnectionStateSynSent,
    kConnectionStateSynRecv,
    kConnectionStateFinWait1,
    kConnectionStateFinWait2,
    kConnectionStateTimeWait,
    kConnectionStateClose,
    kConnectionStateCloseWait,
    kConnectionStateLastAck,
    kConnectionStateListen,
    kConnectionStateClosing,
    kConnectionStateMax,
  };

  // These timer states (except kTimerStateUnknown and kTimerStateMax) are
  // equivalent to and should be kept in sync with those specified in
  // kernel/Documentation/networking/proc_net_tcp.txt
  enum TimerState {
    kTimerStateUnknown = -1,
    kTimerStateNoTimerPending = 0,
    kTimerStateRetransmitTimerPending,
    kTimerStateAnotherTimerPending,
    kTimerStateInTimeWaitState,
    kTimerStateZeroWindowProbeTimerPending,
    kTimerStateMax,
  };

  SocketInfo();
  SocketInfo(ConnectionState connection_state,
             const IPAddress& local_ip_address,
             uint16_t local_port,
             const IPAddress& remote_ip_address,
             uint16_t remote_port,
             uint64_t transmit_queue_value,
             uint64_t receive_queue_value,
             TimerState timer_state);
  SocketInfo(const SocketInfo& socket_info);
  ~SocketInfo();

  SocketInfo& operator=(const SocketInfo& socket_info);

  // Returns true if this socket info and |socket_info| refer to the same
  // socket, i.e. both have the same local address, local port, remote address,
  // and remote port.
  bool IsSameSocketAs(const SocketInfo& socket_info) const;

  ConnectionState connection_state() const { return connection_state_; }
  void set_connection_state(ConnectionState connection_state) {
    connection_state_ = connection_state;
  }

  const IPAddress& local_ip_address() const { return local_ip_address_; }
  void set_local_ip_address(const IPAddress& local_ip_address) {
    local_ip_address_ = local_ip_address;
  }

  uint16_t local_port() const { return local_port_; }
  void set_local_port(uint16_t local_port) { local_port_ = local_port; }

  const IPAddress& remote_ip_address() const { return remote_ip_address_; }
  void set_remote_ip_address(const IPAddress& remote_ip_address) {
    remote_ip_address_ = remote_ip_address;
  }

  uint16_t remote_port() const { return remote_port_; }
  void set_remote_port(uint16_t remote_port) { remote_port_ = remote_port; }

  uint64_t transmit_queue_value() const { return transmit_queue_value_; }
  void set_transmit_queue_value(uint64_t transmit_queue_value) {
    transmit_queue_value_ = transmit_queue_value;
  }

  uint64_t receive_queue_value() const { return receive_queue_value_; }
  void set_receive_queue_value(uint64_t receive_queue_value) {
    receive_queue_value_ = receive_queue_value;
  }

  TimerState timer_state() const { return timer_state_; }
  void set_timer_state(TimerState timer_state) { timer_state_ = timer_state; }

 private:
  ConnectionState connection_state_;
  IPAddress local_ip_address_;
  uint16_t local_port_;
  IPAddress remote_ip_address_;
  uint16_t remote_port_;
  uint64_t transmit_queue_value_;
  uint64_t receive_queue_value_;
  TimerState timer_state_;

  // No DISALLOW_COPY_AND_ASSIGN(SocketInfo) as SocketInfo needs to be kept in
  // STL containers.
};

}  // namespace shill

#endif  // SHILL_SOCKET_INFO_H_
