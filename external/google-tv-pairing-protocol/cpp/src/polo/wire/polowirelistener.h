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

#ifndef POLO_WIRE_POLOWIRELISTENER_H_
#define POLO_WIRE_POLOWIRELISTENER_H_

#include <stdint.h>
#include <vector>

namespace polo {
namespace wire {

// Listener that receives bytes from a wire interface.
class PoloWireListener {
 public:
  virtual ~PoloWireListener() {}

  // Handles bytes received over the interface.
  // @param data the array of bytes that was received
  virtual void OnBytesReceived(const std::vector<uint8_t>& data) = 0;

  // Handles a protocol error from the wire interface if there was an error
  // sending or receiving data. This should be treated as a fatal error and the
  // Polo session should be aborted.
  virtual void OnError() = 0;
};

}  // namespace wire
}  // namespace polo

#endif  // POLO_WIRE_POLOWIRELISTENER_H_
