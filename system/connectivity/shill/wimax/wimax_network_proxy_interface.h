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

#ifndef SHILL_WIMAX_WIMAX_NETWORK_PROXY_INTERFACE_H_
#define SHILL_WIMAX_WIMAX_NETWORK_PROXY_INTERFACE_H_

#include <string>

#include <base/callback_forward.h>

#include "shill/accessor_interface.h"

namespace shill {

// Generally, a string representation of a Network's Identifier. We may group
// several different network identifiers into a single representative
// WiMaxNetworkId, if necessary.
typedef std::string WiMaxNetworkId;

class Error;

// These are the methods that a WiMaxManager.Network proxy must support. The
// interface is provided so that it can be mocked in tests.
class WiMaxNetworkProxyInterface {
 public:
  typedef base::Callback<void(int)> SignalStrengthChangedCallback;

  virtual ~WiMaxNetworkProxyInterface() {}

  virtual RpcIdentifier path() const = 0;

  virtual void set_signal_strength_changed_callback(
      const SignalStrengthChangedCallback& callback) = 0;

  // Properties.
  virtual uint32_t Identifier(Error* error) = 0;
  virtual std::string Name(Error* error) = 0;
  virtual int Type(Error* error) = 0;
  virtual int CINR(Error* error) = 0;
  virtual int RSSI(Error* error) = 0;
  virtual int SignalStrength(Error* error) = 0;
};

}  // namespace shill

#endif  // SHILL_WIMAX_WIMAX_NETWORK_PROXY_INTERFACE_H_
