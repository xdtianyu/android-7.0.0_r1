// Copyright 2015 The Android Open Source Project
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

#ifndef BUFFET_AP_MANAGER_CLIENT_H_
#define BUFFET_AP_MANAGER_CLIENT_H_

#include <memory>
#include <string>

#include <apmanager/dbus-proxies.h>
#include <base/callback.h>
#include <base/memory/ref_counted.h>

namespace buffet {

// Manages soft AP for wifi bootstrapping.
// Once created can handle multiple Start/Stop requests.
class ApManagerClient final {
 public:
  explicit ApManagerClient(const scoped_refptr<dbus::Bus>& bus);
  ~ApManagerClient();

  void Start(const std::string& ssid);
  void Stop();

  std::string GetSsid() const { return ssid_; }

 private:
  void RemoveService(const dbus::ObjectPath& object_path);

  void OnManagerAdded(
      org::chromium::apmanager::ManagerProxyInterface* manager_proxy);
  void OnServiceAdded(
      org::chromium::apmanager::ServiceProxyInterface* service_proxy);

  void OnSsidSet(bool success);

  void OnServiceRemoved(const dbus::ObjectPath& object_path);
  void OnManagerRemoved(const dbus::ObjectPath& object_path);

  scoped_refptr<dbus::Bus> bus_;

  std::unique_ptr<org::chromium::apmanager::ObjectManagerProxy>
      object_manager_proxy_;
  org::chromium::apmanager::ManagerProxyInterface* manager_proxy_{nullptr};

  dbus::ObjectPath service_path_;
  org::chromium::apmanager::ServiceProxyInterface* service_proxy_{nullptr};

  std::string ssid_;

  base::WeakPtrFactory<ApManagerClient> weak_ptr_factory_{this};
};

}  // namespace buffet

#endif  // BUFFET_AP_MANAGER_CLIENT_H_
