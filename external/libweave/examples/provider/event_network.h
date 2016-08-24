// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_EVENT_NETWORK_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_EVENT_NETWORK_H_

#include <weave/provider/network.h>

#include <base/memory/weak_ptr.h>

struct evdns_base;
struct bufferevent;

namespace weave {
namespace examples {

class EventTaskRunner;

class EventNetworkImpl : public weave::provider::Network {
  class Deleter {
   public:
    void operator()(evdns_base* dns_base);
    void operator()(bufferevent* bev);
  };

 public:
  explicit EventNetworkImpl(EventTaskRunner* task_runner_);
  void AddConnectionChangedCallback(
      const ConnectionChangedCallback& callback) override;
  State GetConnectionState() const override;
  void OpenSslSocket(const std::string& host,
                     uint16_t port,
                     const OpenSslSocketCallback& callback) override;

  void SetSimulateOffline(bool value) {
    simulate_offline_ = value;
    UpdateNetworkState();
  }

 private:
  void UpdateNetworkState();
  void UpdateNetworkStateCallback(provider::Network::State state);
  bool simulate_offline_{false};
  EventTaskRunner* task_runner_{nullptr};
  std::unique_ptr<evdns_base, Deleter> dns_base_;
  std::vector<ConnectionChangedCallback> callbacks_;
  provider::Network::State network_state_{provider::Network::State::kOffline};
  std::unique_ptr<bufferevent, Deleter> connectivity_probe_;

  base::WeakPtrFactory<EventNetworkImpl> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_EVENT_NETWORK_H_
