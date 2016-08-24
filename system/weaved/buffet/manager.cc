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

#include "buffet/manager.h"

#include <map>
#include <set>
#include <string>

#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <base/message_loop/message_loop.h>
#include <base/time/time.h>
#include <binderwrapper/binder_wrapper.h>
#include <cutils/properties.h>
#include <brillo/bind_lambda.h>
#include <brillo/errors/error.h>
#include <brillo/http/http_transport.h>
#include <brillo/http/http_utils.h>
#include <brillo/key_value_store.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/mime_utils.h>
#include <dbus/bus.h>
#include <dbus/object_path.h>
#include <dbus/values_util.h>
#include <weave/enum_to_string.h>

#include "brillo/weaved_system_properties.h"
#include "buffet/bluetooth_client.h"
#include "buffet/buffet_config.h"
#include "buffet/http_transport_client.h"
#include "buffet/mdns_client.h"
#include "buffet/shill_client.h"
#include "buffet/weave_error_conversion.h"
#include "buffet/webserv_client.h"
#include "common/binder_utils.h"

using brillo::dbus_utils::AsyncEventSequencer;
using NotificationListener =
    android::weave::IWeaveServiceManagerNotificationListener;

namespace buffet {

namespace {

const char kErrorDomain[] = "buffet";
const char kFileReadError[] = "file_read_error";
const char kBaseComponent[] = "base";
const char kRebootCommand[] = "base.reboot";

bool LoadFile(const base::FilePath& file_path,
              std::string* data,
              brillo::ErrorPtr* error) {
  if (!base::ReadFileToString(file_path, data)) {
    brillo::errors::system::AddSystemError(error, FROM_HERE, errno);
    brillo::Error::AddToPrintf(error, FROM_HERE, kErrorDomain, kFileReadError,
                               "Failed to read file '%s'",
                               file_path.value().c_str());
    return false;
  }
  return true;
}

void LoadTraitDefinitions(const BuffetConfig::Options& options,
                          weave::Device* device) {
  // Load component-specific device trait definitions.
  base::FilePath dir{options.definitions.Append("traits")};
  LOG(INFO) << "Looking for trait definitions in " << dir.value();
  base::FileEnumerator enumerator(dir, false, base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.json"));
  std::vector<std::string> result;
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    LOG(INFO) << "Loading trait definition from " << path.value();
    std::string json;
    CHECK(LoadFile(path, &json, nullptr));
    device->AddTraitDefinitionsFromJson(json);
  }
}

void LoadCommandDefinitions(const BuffetConfig::Options& options,
                            weave::Device* device) {
  auto load_packages = [device](const base::FilePath& root,
                                const base::FilePath::StringType& pattern) {
    base::FilePath dir{root.Append("commands")};
    LOG(INFO) << "Looking for command schemas in " << dir.value();
    base::FileEnumerator enumerator(dir, false, base::FileEnumerator::FILES,
                                    pattern);
    for (base::FilePath path = enumerator.Next(); !path.empty();
         path = enumerator.Next()) {
      LOG(INFO) << "Loading command schema from " << path.value();
      std::string json;
      CHECK(LoadFile(path, &json, nullptr));
      device->AddCommandDefinitionsFromJson(json);
    }
  };
  load_packages(options.definitions, FILE_PATH_LITERAL("*.json"));
  if (!options.test_definitions.empty())
    load_packages(options.test_definitions, FILE_PATH_LITERAL("*test.json"));
}

void LoadStateDefinitions(const BuffetConfig::Options& options,
                          weave::Device* device) {
  // Load component-specific device state definitions.
  base::FilePath dir{options.definitions.Append("states")};
  LOG(INFO) << "Looking for state definitions in " << dir.value();
  base::FileEnumerator enumerator(dir, false, base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.schema.json"));
  std::vector<std::string> result;
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    LOG(INFO) << "Loading state definition from " << path.value();
    std::string json;
    CHECK(LoadFile(path, &json, nullptr));
    device->AddStateDefinitionsFromJson(json);
  }
}

void LoadStateDefaults(const BuffetConfig::Options& options,
                       weave::Device* device) {
  // Load component-specific device state defaults.
  base::FilePath dir{options.definitions.Append("states")};
  LOG(INFO) << "Looking for state defaults in " << dir.value();
  base::FileEnumerator enumerator(dir, false, base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.defaults.json"));
  std::vector<std::string> result;
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    LOG(INFO) << "Loading state defaults from " << path.value();
    std::string json;
    CHECK(LoadFile(path, &json, nullptr));
    CHECK(device->SetStatePropertiesFromJson(json, nullptr));
  }
}

// Updates the manager's state property if the new value is different from
// the current value. In this case also adds the appropriate notification ID
// to the array to record the state change for clients.
void UpdateValue(Manager* manager,
                 std::string Manager::* prop,
                 const std::string& new_value,
                 int notification,
                 std::vector<int>* notification_ids) {
  if (manager->*prop != new_value) {
    manager->*prop = new_value;
    notification_ids->push_back(notification);
  }
}

}  // anonymous namespace

class Manager::TaskRunner : public weave::provider::TaskRunner {
 public:
  void PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override {
    brillo::MessageLoop::current()->PostDelayedTask(from_here, task, delay);
  }
};

Manager::Manager(const Options& options,
                 const scoped_refptr<dbus::Bus>& bus)
    : options_{options}, bus_{bus} {}

Manager::~Manager() {
  android::BinderWrapper* binder_wrapper = android::BinderWrapper::Get();
  for (const auto& listener : notification_listeners_) {
    binder_wrapper->UnregisterForDeathNotifications(
        android::IInterface::asBinder(listener));
  }
  for (const auto& pair : services_) {
    binder_wrapper->UnregisterForDeathNotifications(
        android::IInterface::asBinder(pair.first));
  }
}

void Manager::Start(AsyncEventSequencer* sequencer) {
  power_manager_client_.Init();
  RestartWeave(sequencer);
}

void Manager::RestartWeave(AsyncEventSequencer* sequencer) {
  Stop();

  task_runner_.reset(new TaskRunner{});
  config_.reset(new BuffetConfig{options_.config_options});
  http_client_.reset(new HttpTransportClient);
  shill_client_.reset(new ShillClient{bus_,
                                      options_.device_whitelist,
                                      !options_.xmpp_enabled});
  weave::provider::HttpServer* http_server{nullptr};
#ifdef BUFFET_USE_WIFI_BOOTSTRAPPING
  if (!options_.disable_privet) {
    mdns_client_ = MdnsClient::CreateInstance();
    web_serv_client_.reset(new WebServClient{
        bus_, sequencer,
        base::Bind(&Manager::CreateDevice, weak_ptr_factory_.GetWeakPtr())});
    bluetooth_client_ = BluetoothClient::CreateInstance();
    http_server = web_serv_client_.get();

    if (options_.enable_ping) {
      auto ping_handler = base::Bind(
          [](std::unique_ptr<weave::provider::HttpServer::Request> request) {
            request->SendReply(brillo::http::status_code::Ok, "Hello, world!",
                               brillo::mime::text::kPlain);
          });
      http_server->AddHttpRequestHandler("/privet/ping", ping_handler);
      http_server->AddHttpsRequestHandler("/privet/ping", ping_handler);
    }
  }
#endif  // BUFFET_USE_WIFI_BOOTSTRAPPING

  if (!http_server)
    CreateDevice();
}

void Manager::CreateDevice() {
  if (device_)
    return;

  device_ = weave::Device::Create(config_.get(), task_runner_.get(),
                                  http_client_.get(), shill_client_.get(),
                                  mdns_client_.get(), web_serv_client_.get(),
                                  shill_client_.get(), bluetooth_client_.get());

  LoadTraitDefinitions(options_.config_options, device_.get());
  LoadCommandDefinitions(options_.config_options, device_.get());
  LoadStateDefinitions(options_.config_options, device_.get());
  LoadStateDefaults(options_.config_options, device_.get());

  device_->AddSettingsChangedCallback(
      base::Bind(&Manager::OnConfigChanged, weak_ptr_factory_.GetWeakPtr()));

  device_->AddTraitDefsChangedCallback(
      base::Bind(&Manager::OnTraitDefsChanged,
                 weak_ptr_factory_.GetWeakPtr()));
  device_->AddStateChangedCallback(
      base::Bind(&Manager::OnComponentTreeChanged,
                 weak_ptr_factory_.GetWeakPtr()));
  device_->AddComponentTreeChangedCallback(
      base::Bind(&Manager::OnComponentTreeChanged,
                 weak_ptr_factory_.GetWeakPtr()));

  device_->AddGcdStateChangedCallback(
      base::Bind(&Manager::OnGcdStateChanged, weak_ptr_factory_.GetWeakPtr()));

  device_->AddPairingChangedCallbacks(
      base::Bind(&Manager::OnPairingStart, weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&Manager::OnPairingEnd, weak_ptr_factory_.GetWeakPtr()));

  device_->AddCommandHandler(kBaseComponent, kRebootCommand,
                             base::Bind(&Manager::OnRebootDevice,
                                        weak_ptr_factory_.GetWeakPtr()));

  CreateServicesForClients();
}

void Manager::Stop() {
  device_.reset();
#ifdef BUFFET_USE_WIFI_BOOTSTRAPPING
  web_serv_client_.reset();
  mdns_client_.reset();
#endif  // BUFFET_USE_WIFI_BOOTSTRAPPING
  shill_client_.reset();
  http_client_.reset();
  config_.reset();
  task_runner_.reset();
}

void Manager::OnTraitDefsChanged() {
  NotifyServiceManagerChange({NotificationListener::TRAITS});
}

void Manager::OnComponentTreeChanged() {
  NotifyServiceManagerChange({NotificationListener::COMPONENTS});
}

void Manager::OnGcdStateChanged(weave::GcdState state) {
  state_ = weave::EnumToString(state);
  NotifyServiceManagerChange({NotificationListener::STATE});
  property_set(weaved::system_properties::kState, state_.c_str());
}

void Manager::OnConfigChanged(const weave::Settings& settings) {
  std::vector<int> ids;
  UpdateValue(this, &Manager::cloud_id_, settings.cloud_id,
              NotificationListener::CLOUD_ID, &ids);
  UpdateValue(this, &Manager::device_id_, settings.device_id,
              NotificationListener::DEVICE_ID, &ids);
  UpdateValue(this, &Manager::device_name_, settings.name,
              NotificationListener::DEVICE_NAME, &ids);
  UpdateValue(this, &Manager::device_description_, settings.description,
              NotificationListener::DEVICE_DESCRIPTION, &ids);
  UpdateValue(this, &Manager::device_location_, settings.location,
              NotificationListener::DEVICE_LOCATION, &ids);
  UpdateValue(this, &Manager::oem_name_, settings.oem_name,
              NotificationListener::OEM_NAME, &ids);
  UpdateValue(this, &Manager::model_id_, settings.model_id,
              NotificationListener::MODEL_ID, &ids);
  UpdateValue(this, &Manager::model_name_, settings.model_name,
              NotificationListener::MODEL_NAME, &ids);
  NotifyServiceManagerChange(ids);
}

void Manager::OnPairingStart(const std::string& session_id,
                             weave::PairingType pairing_type,
                             const std::vector<uint8_t>& code) {
  // For now, just overwrite the exposed PairInfo with the most recent pairing
  // attempt.
  std::vector<int> ids;
  UpdateValue(this, &Manager::pairing_session_id_, session_id,
              NotificationListener::PAIRING_SESSION_ID, &ids);
  UpdateValue(this, &Manager::pairing_mode_, EnumToString(pairing_type),
              NotificationListener::PAIRING_MODE, &ids);
  std::string pairing_code{code.begin(), code.end()};
  UpdateValue(this, &Manager::pairing_code_, pairing_code,
              NotificationListener::PAIRING_CODE, &ids);
  NotifyServiceManagerChange(ids);
}

void Manager::OnPairingEnd(const std::string& session_id) {
  if (pairing_session_id_ != session_id)
    return;
  std::vector<int> ids;
  UpdateValue(this, &Manager::pairing_session_id_, "",
              NotificationListener::PAIRING_SESSION_ID, &ids);
  UpdateValue(this, &Manager::pairing_mode_, "",
              NotificationListener::PAIRING_MODE, &ids);
  UpdateValue(this, &Manager::pairing_code_, "",
              NotificationListener::PAIRING_CODE, &ids);
  NotifyServiceManagerChange(ids);
}

void Manager::OnRebootDevice(const std::weak_ptr<weave::Command>& cmd) {
  auto command = cmd.lock();
  if (!command || !command->Complete({}, nullptr))
    return;

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&Manager::RebootDeviceNow, weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(2));
}

void Manager::RebootDeviceNow() {
  power_manager_client_.Reboot(android::RebootReason::DEFAULT);
}

android::binder::Status Manager::connect(
    const android::sp<android::weave::IWeaveClient>& client) {
  pending_clients_.push_back(client);
  if (device_)
    CreateServicesForClients();
  return android::binder::Status::ok();
}

android::binder::Status Manager::registerNotificationListener(
    const WeaveServiceManagerNotificationListener& listener) {
  notification_listeners_.insert(listener);
  android::BinderWrapper::Get()->RegisterForDeathNotifications(
      android::IInterface::asBinder(listener),
      base::Bind(&Manager::OnNotificationListenerDestroyed,
                 weak_ptr_factory_.GetWeakPtr(), listener));
  return android::binder::Status::ok();
}

android::binder::Status Manager::getCloudId(android::String16* id) {
  *id = weaved::binder_utils::ToString16(cloud_id_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getDeviceId(android::String16* id) {
  *id = weaved::binder_utils::ToString16(device_id_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getDeviceName(android::String16* name) {
  *name = weaved::binder_utils::ToString16(device_name_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getDeviceDescription(
    android::String16* description) {
  *description = weaved::binder_utils::ToString16(device_description_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getDeviceLocation(
    android::String16* location) {
  *location = weaved::binder_utils::ToString16(device_location_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getOemName(android::String16* name) {
  *name = weaved::binder_utils::ToString16(oem_name_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getModelName(android::String16* name) {
  *name = weaved::binder_utils::ToString16(model_name_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getModelId(android::String16* id) {
  *id = weaved::binder_utils::ToString16(model_id_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getPairingSessionId(android::String16* id) {
  *id = weaved::binder_utils::ToString16(pairing_session_id_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getPairingMode(android::String16* mode) {
  *mode = weaved::binder_utils::ToString16(pairing_mode_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getPairingCode(android::String16* code) {
  *code = weaved::binder_utils::ToString16(pairing_code_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getState(android::String16* state) {
  *state = weaved::binder_utils::ToString16(state_);
  return android::binder::Status::ok();
}

android::binder::Status Manager::getTraits(android::String16* traits) {
  *traits = weaved::binder_utils::ToString16(device_->GetTraits());
  return android::binder::Status::ok();
}

android::binder::Status Manager::getComponents(android::String16* components) {
  *components = weaved::binder_utils::ToString16(device_->GetComponents());
  return android::binder::Status::ok();
}

void Manager::CreateServicesForClients() {
  CHECK(device_);
  // For safety, iterate over a copy of |pending_clients_| and clear the
  // original vector before performing the iterations.
  std::vector<android::sp<android::weave::IWeaveClient>> pending_clients_copy;
  std::swap(pending_clients_copy, pending_clients_);
  for (const auto& client : pending_clients_copy) {
    android::sp<BinderWeaveService> service =
        new BinderWeaveService{device_.get(), client};
    services_.emplace(client, service);
    client->onServiceConnected(service);
    android::BinderWrapper::Get()->RegisterForDeathNotifications(
        android::IInterface::asBinder(client),
        base::Bind(&Manager::OnClientDisconnected,
                   weak_ptr_factory_.GetWeakPtr(),
                   client));
  }
}

void Manager::OnClientDisconnected(
    const android::sp<android::weave::IWeaveClient>& client) {
  services_.erase(client);
}

void Manager::OnNotificationListenerDestroyed(
    const WeaveServiceManagerNotificationListener& notification_listener) {
  notification_listeners_.erase(notification_listener);
}

void Manager::NotifyServiceManagerChange(
    const std::vector<int>& notification_ids) {
  if (notification_ids.empty())
    return;
  for (const auto& listener : notification_listeners_)
    listener->notifyServiceManagerChange(notification_ids);
}

}  // namespace buffet
