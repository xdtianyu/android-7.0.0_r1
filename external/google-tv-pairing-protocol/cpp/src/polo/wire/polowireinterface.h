// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef POLO_WIRE_POLOWIREINTERFACE_H_
#define POLO_WIRE_POLOWIREINTERFACE_H_

#include <stddef.h>
#include <vector>

#include "polo/wire/polowirelistener.h"
#include "polo/util/macros.h"

namespace polo {
namespace wire {

// An interface for sending and receiving raw data for a Polo pairing session.
class PoloWireInterface {
 public:
  PoloWireInterface() {}
  virtual ~PoloWireInterface() {}

  // Sets the listener that will receive incoming data and error notifications.
  // @param listener the listener to set for this interface
  void set_listener(PoloWireListener* listener) { listener_ = listener; }

  // Sends data over the interface.
  // @param data the bytes to send
  virtual void Send(const std::vector<uint8_t>& data) = 0;

  // Receives the given number of bytes from the interface asynchronously. The
  // listener will be notified when the data is received.
  // @param num_bytes the number of bytes to receive
  virtual void Receive(size_t num_bytes) = 0;

 protected:
  // Gets the listener that will receive incoming data and error notifications.
  PoloWireListener* listener() const { return listener_; }

 private:
  PoloWireListener* listener_;

  DISALLOW_COPY_AND_ASSIGN(PoloWireInterface);
};

}  // namespace wire
}  // namespace polo

#endif  // POLO_WIRE_POLOWIREINTERFACE_H_
