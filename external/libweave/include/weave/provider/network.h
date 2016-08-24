// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_NETWORK_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_NETWORK_H_

#include <string>

#include <base/callback.h>
#include <weave/error.h>
#include <weave/stream.h>

namespace weave {
namespace provider {

// Interface with methods to detect network connectivity and opening network
// connections.
class Network {
 public:
  enum class State {
    kOffline = 0,
    kError,
    kConnecting,
    kOnline,
  };

  // Callback type for AddConnectionChangedCallback.
  using ConnectionChangedCallback = base::Closure;

  // Callback type for OpenSslSocket.
  using OpenSslSocketCallback =
      base::Callback<void(std::unique_ptr<Stream> stream, ErrorPtr error)>;

  // Subscribes to notification about changes in network connectivity. Changes
  // may include but not limited: interface up or down, new IP was assigned,
  // cable is disconnected.
  virtual void AddConnectionChangedCallback(
      const ConnectionChangedCallback& callback) = 0;

  // Returns current Internet connectivity state
  virtual State GetConnectionState() const = 0;

  // Opens bidirectional sockets and returns attached stream.
  virtual void OpenSslSocket(const std::string& host,
                             uint16_t port,
                             const OpenSslSocketCallback& callback) = 0;

 protected:
  virtual ~Network() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_NETWORK_H_
