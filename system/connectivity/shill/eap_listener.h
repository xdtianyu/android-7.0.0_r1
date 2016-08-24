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

#ifndef SHILL_EAP_LISTENER_H_
#define SHILL_EAP_LISTENER_H_

#include <memory>

#include <base/callback.h>
#include <base/macros.h>

namespace shill {

class EventDispatcher;
class IOHandler;
class ScopedSocketCloser;
class Sockets;

// Listens for EAP packets on |interface_index| and invokes a
// callback when a request frame arrives.
class EapListener {
 public:
  typedef base::Callback<void()> EapRequestReceivedCallback;

  explicit EapListener(EventDispatcher* event_dispatcher,
                       int interface_index);
  virtual ~EapListener();

  // Create a socket for tranmission and reception.  Returns true
  // if successful, false otherwise.
  virtual bool Start();

  // Destroy the client socket.
  virtual void Stop();

  // Setter for |request_received_callback_|.
  virtual void set_request_received_callback(
      const EapRequestReceivedCallback& callback) {
    request_received_callback_ = callback;
  }

 private:
  friend class EapListenerTest;

  // The largest EAP packet we expect to receive.
  static const size_t kMaxEapPacketLength;

  // Creates |socket_|.  Returns true on succes, false on failure.
  bool CreateSocket();

  // Retrieves an EAP packet from |socket_|.  This is the callback method
  // configured on |receive_request_handler_|.
  void ReceiveRequest(int fd);

  // Event dispatcher to use for creating an input handler.
  EventDispatcher* dispatcher_;

  // The interface index fo the device to monitor.
  const int interface_index_;

  // Callback handle to invoke when an EAP request is received.
  EapRequestReceivedCallback request_received_callback_;

  // Sockets instance to perform socket calls on.
  std::unique_ptr<Sockets> sockets_;

  // Receive socket configured to receive PAE (Port Access Entity) packets.
  int socket_;

  // Scoped socket closer for the receive |socket_|.
  std::unique_ptr<ScopedSocketCloser> socket_closer_;

  // Input handler for |socket_|.  Calls ReceiveRequest().
  std::unique_ptr<IOHandler> receive_request_handler_;

  DISALLOW_COPY_AND_ASSIGN(EapListener);
};

}  // namespace shill

#endif  // SHILL_EAP_LISTENER_H_
