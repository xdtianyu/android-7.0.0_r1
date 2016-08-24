//
// Copyright (C) 2015 The Android Open Source Project
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

#include "shill/dbus/chromeos_wimax_manager_proxy.h"

#include <string>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/key_value_store.h"
#include "shill/logging.h"

using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(ChromeosWiMaxManagerProxy* w) {
  return "(wimax_manager_proxy)";
}
}

// static.
const char ChromeosWiMaxManagerProxy::kPropertyDevices[] = "Devices";

ChromeosWiMaxManagerProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyDevices, &devices);
}

ChromeosWiMaxManagerProxy::ChromeosWiMaxManagerProxy(
    EventDispatcher* dispatcher,
    const scoped_refptr<dbus::Bus>& bus,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback)
    : proxy_(
        new org::chromium::WiMaxManagerProxy(
            bus,
            wimax_manager::kWiMaxManagerServiceName,
            dbus::ObjectPath(wimax_manager::kWiMaxManagerServicePath))),
      dispatcher_(dispatcher),
      service_appeared_callback_(service_appeared_callback),
      service_vanished_callback_(service_vanished_callback),
      service_available_(false) {
  // Register signal handler.
  proxy_->RegisterDevicesChangedSignalHandler(
      base::Bind(&ChromeosWiMaxManagerProxy::DevicesChanged,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&ChromeosWiMaxManagerProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr()));

  // Register properties.
  properties_.reset(
      new PropertySet(
          proxy_->GetObjectProxy(),
          wimax_manager::kWiMaxManagerInterface,
          base::Bind(&ChromeosWiMaxManagerProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));
  properties_->ConnectSignals();
  properties_->GetAll();

  // Monitor service owner changes. This callback lives for the lifetime of
  // the ObjectProxy.
  proxy_->GetObjectProxy()->SetNameOwnerChangedCallback(
      base::Bind(&ChromeosWiMaxManagerProxy::OnServiceOwnerChanged,
                 weak_factory_.GetWeakPtr()));

  // One time callback when service becomes available.
  proxy_->GetObjectProxy()->WaitForServiceToBeAvailable(
      base::Bind(&ChromeosWiMaxManagerProxy::OnServiceAvailable,
                 weak_factory_.GetWeakPtr()));
}

ChromeosWiMaxManagerProxy::~ChromeosWiMaxManagerProxy() {}

void ChromeosWiMaxManagerProxy::set_devices_changed_callback(
    const DevicesChangedCallback& callback) {
  devices_changed_callback_ = callback;
}

RpcIdentifiers ChromeosWiMaxManagerProxy::Devices(Error* error) {
  SLOG(this, 2) << __func__;
  if (!service_available_) {
    Error::PopulateAndLog(FROM_HERE, error, Error::kOperationFailed,
                          "WiMax Manager process not present");
    return RpcIdentifiers();
  }

  if (!properties_->devices.GetAndBlock()) {
    LOG(ERROR) << "Failed to get Devices";
    return RpcIdentifiers();
  }
  RpcIdentifiers rpc_devices;
  KeyValueStore::ConvertPathsToRpcIdentifiers(properties_->devices.value(),
                                              &rpc_devices);
  return rpc_devices;
}

void ChromeosWiMaxManagerProxy::OnServiceAvailable(bool available) {
  SLOG(DBus, nullptr, 2) << __func__ << ": " << available;

  // The callback might invoke calls to the ObjectProxy, so defer the callback
  // to event loop.
  if (available && !service_appeared_callback_.is_null()) {
    dispatcher_->PostTask(service_appeared_callback_);
  } else if (!available && !service_vanished_callback_.is_null()) {
    dispatcher_->PostTask(service_vanished_callback_);
  }
  service_available_ = available;
}

void ChromeosWiMaxManagerProxy::OnServiceOwnerChanged(
    const string& old_owner, const string& new_owner) {
  SLOG(DBus, nullptr, 2) << __func__
      << "old: " << old_owner << " new: " << new_owner;
  if (new_owner.empty()) {
    OnServiceAvailable(false);
  } else {
    OnServiceAvailable(true);
  }
}

void ChromeosWiMaxManagerProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(DBus, nullptr, 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

void ChromeosWiMaxManagerProxy::OnPropertyChanged(
    const std::string& property_name) {
  SLOG(DBus, nullptr, 2) << __func__ << ": " << property_name;
}

void ChromeosWiMaxManagerProxy::DevicesChanged(
    const vector<dbus::ObjectPath>& devices) {
  SLOG(DBus, nullptr, 2) << __func__ << "(" << devices.size() << ")";
  if (devices_changed_callback_.is_null()) {
    return;
  }
  RpcIdentifiers rpc_devices;
  KeyValueStore::ConvertPathsToRpcIdentifiers(devices, &rpc_devices);
  devices_changed_callback_.Run(rpc_devices);
}

}  // namespace shill
