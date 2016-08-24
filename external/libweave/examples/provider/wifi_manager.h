// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_WIFI_MANAGER_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_WIFI_MANAGER_H_

#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <base/time/time.h>
#include <weave/provider/wifi.h>

namespace weave {

namespace provider {
class TaskRunner;
}

namespace examples {

class EventNetworkImpl;

// Basic weave::Wifi implementation.
// Production version of SSL socket needs secure server certificate check.
class WifiImpl : public provider::Wifi {
 public:
  WifiImpl(provider::TaskRunner* task_runner, EventNetworkImpl* network);
  ~WifiImpl();

  // Wifi implementation.
  void Connect(const std::string& ssid,
               const std::string& passphrase,
               const DoneCallback& callback) override;
  void StartAccessPoint(const std::string& ssid) override;
  void StopAccessPoint() override;
  bool IsWifi24Supported() const override {return true;};
  bool IsWifi50Supported() const override {return false;};

  static bool HasWifiCapability();

 private:
  void TryToConnect(const std::string& ssid,
                    const std::string& passphrase,
                    int pid,
                    base::Time until,
                    const DoneCallback& callback);
  bool hostapd_started_{false};
  provider::TaskRunner* task_runner_{nullptr};
  EventNetworkImpl* network_{nullptr};
  base::WeakPtrFactory<WifiImpl> weak_ptr_factory_{this};
  std::string iface_;
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_WIFI_MANAGER_H_
