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

#ifndef BUFFET_MANAGER_H_
#define BUFFET_MANAGER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <base/values.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/errors/error.h>
#include <nativepower/power_manager_client.h>
#include <weave/device.h>

#include "android/weave/BnWeaveServiceManager.h"
#include "buffet/binder_weave_service.h"
#include "buffet/buffet_config.h"

namespace buffet {

class BluetoothClient;
class HttpTransportClient;
class MdnsClient;
class ShillClient;
class WebServClient;

// The Manager is responsible for global state of Buffet.  It exposes
// interfaces which affect the entire device such as device registration and
// device state.
class Manager final : public android::weave::BnWeaveServiceManager {
 public:
  struct Options {
    bool xmpp_enabled = true;
    bool disable_privet = false;
    bool enable_ping = false;
    std::set<std::string> device_whitelist;

    BuffetConfig::Options config_options;
  };

  Manager(const Options& options, const scoped_refptr<dbus::Bus>& bus);
  ~Manager() override;

  void Start(brillo::dbus_utils::AsyncEventSequencer* sequencer);
  void Stop();

 private:
  void RestartWeave(brillo::dbus_utils::AsyncEventSequencer* sequencer);
  void CreateDevice();

  // Binder methods for IWeaveServiceManager:
  using WeaveServiceManagerNotificationListener =
      android::sp<android::weave::IWeaveServiceManagerNotificationListener>;
  android::binder::Status connect(
      const android::sp<android::weave::IWeaveClient>& client) override;
  android::binder::Status registerNotificationListener(
      const WeaveServiceManagerNotificationListener& listener) override;
  android::binder::Status getDeviceId(android::String16* id) override;
  android::binder::Status getCloudId(android::String16* id) override;
  android::binder::Status getDeviceName(android::String16* name) override;
  android::binder::Status getDeviceDescription(
      android::String16* description) override;
  android::binder::Status getDeviceLocation(
      android::String16* location) override;
  android::binder::Status getOemName(android::String16* name) override;
  android::binder::Status getModelName(android::String16* name) override;
  android::binder::Status getModelId(android::String16* id) override;
  android::binder::Status getPairingSessionId(android::String16* id) override;
  android::binder::Status getPairingMode(android::String16* mode) override;
  android::binder::Status getPairingCode(android::String16* code) override;
  android::binder::Status getState(android::String16* state) override;
  android::binder::Status getTraits(android::String16* traits) override;
  android::binder::Status getComponents(android::String16* components) override;

  void OnTraitDefsChanged();
  void OnComponentTreeChanged();
  void OnGcdStateChanged(weave::GcdState state);
  void OnConfigChanged(const weave::Settings& settings);
  void OnPairingStart(const std::string& session_id,
                      weave::PairingType pairing_type,
                      const std::vector<uint8_t>& code);
  void OnPairingEnd(const std::string& session_id);

  void CreateServicesForClients();
  void OnClientDisconnected(
      const android::sp<android::weave::IWeaveClient>& client);
  void OnNotificationListenerDestroyed(
      const WeaveServiceManagerNotificationListener& notification_listener);
  void NotifyServiceManagerChange(const std::vector<int>& notification_ids);
  void OnRebootDevice(const std::weak_ptr<weave::Command>& cmd);
  void RebootDeviceNow();

  Options options_;
  scoped_refptr<dbus::Bus> bus_;

  class TaskRunner;
  std::unique_ptr<TaskRunner> task_runner_;
  std::unique_ptr<BluetoothClient> bluetooth_client_;
  std::unique_ptr<BuffetConfig> config_;
  std::unique_ptr<HttpTransportClient> http_client_;
  std::unique_ptr<ShillClient> shill_client_;
  std::unique_ptr<MdnsClient> mdns_client_;
  std::unique_ptr<WebServClient> web_serv_client_;
  std::unique_ptr<weave::Device> device_;

  std::vector<android::sp<android::weave::IWeaveClient>> pending_clients_;
  std::map<android::sp<android::weave::IWeaveClient>,
           android::sp<BinderWeaveService>> services_;
  std::set<WeaveServiceManagerNotificationListener> notification_listeners_;
  android::PowerManagerClient power_manager_client_;

  // State properties.
  std::string cloud_id_;
  std::string device_id_;
  std::string device_name_;
  std::string device_description_;
  std::string device_location_;
  std::string oem_name_;
  std::string model_name_;
  std::string model_id_;
  std::string pairing_session_id_;
  std::string pairing_mode_;
  std::string pairing_code_;
  std::string state_;

  base::WeakPtrFactory<Manager> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Manager);
};

}  // namespace buffet

#endif  // BUFFET_MANAGER_H_
