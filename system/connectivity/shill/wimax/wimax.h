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

#ifndef SHILL_WIMAX_WIMAX_H_
#define SHILL_WIMAX_WIMAX_H_

#include <memory>
#include <set>
#include <string>

#include <base/cancelable_callback.h>
#include <base/memory/weak_ptr.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/device.h"

namespace shill {

class WiMaxDeviceProxyInterface;

class WiMax : public Device {
 public:
  WiMax(ControlInterface* control,
        EventDispatcher* dispatcher,
        Metrics* metrics,
        Manager* manager,
        const std::string& link_name,
        const std::string& address,
        int interface_index,
        const RpcIdentifier& path);

  ~WiMax() override;

  // Inherited from Device.
  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;
  void Stop(Error* error, const EnabledStateChangedCallback& callback) override;
  void Scan(ScanType /*scan_type*/, Error* error,
            const std::string& /*reason*/) override;

  virtual void ConnectTo(const WiMaxServiceRefPtr& service, Error* error);
  virtual void DisconnectFrom(const ServiceRefPtr& service, Error* error);

  // Signaled by |service| when stopped.
  virtual void OnServiceStopped(const WiMaxServiceRefPtr& service);

  // Signaled by WiMaxProvider when the RPC device disappears. The provider will
  // deregister and destroy the device after invoking this method.
  virtual void OnDeviceVanished();

  // Returns true if this device is not connecting or connected to a service.
  virtual bool IsIdle() const;

  const RpcIdentifier& path() const { return path_; }
  bool scanning() const { return scanning_; }
  const std::set<RpcIdentifier>& networks() const { return networks_; }

 private:
  friend class WiMaxTest;
  FRIEND_TEST(WiMaxProviderTest, OnNetworksChanged);
  FRIEND_TEST(WiMaxTest, ConnectTimeout);
  FRIEND_TEST(WiMaxTest, ConnectTo);
  FRIEND_TEST(WiMaxTest, DropService);
  FRIEND_TEST(WiMaxTest, IsIdle);
  FRIEND_TEST(WiMaxTest, OnConnectComplete);
  FRIEND_TEST(WiMaxTest, OnDeviceVanished);
  FRIEND_TEST(WiMaxTest, OnEnableComplete);
  FRIEND_TEST(WiMaxTest, OnNetworksChanged);
  FRIEND_TEST(WiMaxTest, OnServiceStopped);
  FRIEND_TEST(WiMaxTest, OnStatusChanged);
  FRIEND_TEST(WiMaxTest, StartStop);

  static const int kDefaultConnectTimeoutSeconds;
  static const int kDefaultRPCTimeoutSeconds;

  void OnScanNetworksComplete(const Error& error);
  void OnConnectComplete(const Error& error);
  void OnDisconnectComplete(const Error& error);
  void OnEnableComplete(const EnabledStateChangedCallback& callback,
                        const Error& error);
  void OnDisableComplete(const EnabledStateChangedCallback& callback,
                         const Error& error);

  void OnNetworksChanged(const RpcIdentifiers& networks);
  void OnStatusChanged(wimax_manager::DeviceStatus status);

  void DropService(Service::ConnectState state);

  // Initializes a callback that will invoke OnConnectTimeout. The timeout will
  // not be restarted if it's already scheduled.
  void StartConnectTimeout();
  // Cancels the connect timeout callback, if any, previously scheduled through
  // StartConnectTimeout.
  void StopConnectTimeout();
  // Returns true if a connect timeout is scheduled, false otherwise.
  bool IsConnectTimeoutStarted() const;
  // Called if a connect timeout scheduled through StartConnectTimeout
  // fires. Marks the callback as stopped and invokes DropService.
  void OnConnectTimeout();

  const RpcIdentifier path_;

  base::WeakPtrFactory<WiMax> weak_ptr_factory_;
  std::unique_ptr<WiMaxDeviceProxyInterface> proxy_;
  bool scanning_;
  WiMaxServiceRefPtr pending_service_;
  std::set<RpcIdentifier> networks_;
  wimax_manager::DeviceStatus status_;

  base::CancelableClosure connect_timeout_callback_;
  int connect_timeout_seconds_;

  DISALLOW_COPY_AND_ASSIGN(WiMax);
};

}  // namespace shill

#endif  // SHILL_WIMAX_WIMAX_H_
