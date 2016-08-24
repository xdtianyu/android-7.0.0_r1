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

#include "shill/dbus/chromeos_supplicant_process_proxy.h"

#include <string>

#include "shill/event_dispatcher.h"
#include "shill/logging.h"
#include "shill/supplicant/wpa_supplicant.h"

using std::string;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDBus;
static string ObjectID(const dbus::ObjectPath* p) { return p->value(); }
}

const char ChromeosSupplicantProcessProxy::kInterfaceName[] =
    "fi.w1.wpa_supplicant1";
const char ChromeosSupplicantProcessProxy::kPropertyDebugLevel[] =
    "DebugLevel";
const char ChromeosSupplicantProcessProxy::kPropertyDebugTimestamp[] =
    "DebugTimestamp";
const char ChromeosSupplicantProcessProxy::kPropertyDebugShowKeys[] =
    "DebugShowKeys";
const char ChromeosSupplicantProcessProxy::kPropertyInterfaces[] =
    "Interfaces";
const char ChromeosSupplicantProcessProxy::kPropertyEapMethods[] =
    "EapMethods";

ChromeosSupplicantProcessProxy::PropertySet::PropertySet(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const PropertyChangedCallback& callback)
    : dbus::PropertySet(object_proxy, interface_name, callback) {
  RegisterProperty(kPropertyDebugLevel, &debug_level);
  RegisterProperty(kPropertyDebugTimestamp, &debug_timestamp);
  RegisterProperty(kPropertyDebugShowKeys, &debug_show_keys);
  RegisterProperty(kPropertyInterfaces, &interfaces);
  RegisterProperty(kPropertyEapMethods, &eap_methods);
}

ChromeosSupplicantProcessProxy::ChromeosSupplicantProcessProxy(
    EventDispatcher* dispatcher,
    const scoped_refptr<dbus::Bus>& bus,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback)
    : supplicant_proxy_(
        new fi::w1::wpa_supplicant1Proxy(
            bus,
            WPASupplicant::kDBusAddr,
            dbus::ObjectPath(WPASupplicant::kDBusPath))),
      dispatcher_(dispatcher),
      service_appeared_callback_(service_appeared_callback),
      service_vanished_callback_(service_vanished_callback),
      service_available_(false) {
  // Register properties.
  properties_.reset(
      new PropertySet(
          supplicant_proxy_->GetObjectProxy(),
          kInterfaceName,
          base::Bind(&ChromeosSupplicantProcessProxy::OnPropertyChanged,
                     weak_factory_.GetWeakPtr())));

  // Register signal handlers.
  dbus::ObjectProxy::OnConnectedCallback on_connected_callback =
      base::Bind(&ChromeosSupplicantProcessProxy::OnSignalConnected,
                 weak_factory_.GetWeakPtr());
  supplicant_proxy_->RegisterInterfaceAddedSignalHandler(
      base::Bind(&ChromeosSupplicantProcessProxy::InterfaceAdded,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  supplicant_proxy_->RegisterInterfaceRemovedSignalHandler(
      base::Bind(&ChromeosSupplicantProcessProxy::InterfaceRemoved,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);
  supplicant_proxy_->RegisterPropertiesChangedSignalHandler(
      base::Bind(&ChromeosSupplicantProcessProxy::PropertiesChanged,
                 weak_factory_.GetWeakPtr()),
      on_connected_callback);

  // Connect property signals and initialize cached values. Based on
  // recommendations from src/dbus/property.h.
  properties_->ConnectSignals();
  properties_->GetAll();

  // Monitor service owner changes. This callback lives for the lifetime of
  // the ObjectProxy.
  supplicant_proxy_->GetObjectProxy()->SetNameOwnerChangedCallback(
      base::Bind(&ChromeosSupplicantProcessProxy::OnServiceOwnerChanged,
                 weak_factory_.GetWeakPtr()));

  // One time callback when service becomes available.
  supplicant_proxy_->GetObjectProxy()->WaitForServiceToBeAvailable(
      base::Bind(&ChromeosSupplicantProcessProxy::OnServiceAvailable,
                 weak_factory_.GetWeakPtr()));
}

ChromeosSupplicantProcessProxy::~ChromeosSupplicantProcessProxy() {}

bool ChromeosSupplicantProcessProxy::CreateInterface(
    const KeyValueStore& args, string* rpc_identifier) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }
  brillo::VariantDictionary dict;
  KeyValueStore::ConvertToVariantDictionary(args, &dict);
  dbus::ObjectPath path;
  brillo::ErrorPtr error;
  if (!supplicant_proxy_->CreateInterface(dict, &path, &error)) {
    // Interface might already been created by wpasupplicant.
    LOG(ERROR) << "Failed to create interface: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  *rpc_identifier = path.value();
  return true;
}

bool ChromeosSupplicantProcessProxy::RemoveInterface(
    const string& rpc_identifier) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__ << ": "
                                               << rpc_identifier;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }

  brillo::ErrorPtr error;
  if (!supplicant_proxy_->RemoveInterface(dbus::ObjectPath(rpc_identifier),
                                          &error)) {
    LOG(FATAL) << "Failed to remove interface " << rpc_identifier << ": "
               << error->GetCode() << " " << error->GetMessage();
    return false;  // Make the compiler happy.
  }
  return true;
}

bool ChromeosSupplicantProcessProxy::GetInterface(
    const std::string& ifname, string* rpc_identifier) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__ << ": " << ifname;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }

  dbus::ObjectPath path;
  brillo::ErrorPtr error;
  if (!supplicant_proxy_->GetInterface(ifname, &path, &error)) {
    LOG(FATAL) << "Failed to get interface " << ifname << ": "
               << error->GetCode() << " " << error->GetMessage();
    return false;  // Make the compiler happy.
  }
  *rpc_identifier = path.value();
  return rpc_identifier;
}

bool ChromeosSupplicantProcessProxy::SetDebugLevel(const std::string& level) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__ << ": " << level;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }

  if (!properties_->debug_level.SetAndBlock(level)) {
    LOG(ERROR) << __func__ << " failed: " << level;
    return false;
  }
  return true;
}

bool ChromeosSupplicantProcessProxy::GetDebugLevel(string* level) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }
  if (!properties_->debug_level.GetAndBlock()) {
    LOG(ERROR) << "Failed to get DebugLevel";
    return false;
  }
  *level = properties_->debug_level.value();
  return true;
}

bool ChromeosSupplicantProcessProxy::ExpectDisconnect() {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
  if (!service_available_) {
    LOG(ERROR) << "Supplicant process not present";
    return false;
  }
  brillo::ErrorPtr error;
  supplicant_proxy_->ExpectDisconnect(&error);
  return true;
}

void ChromeosSupplicantProcessProxy::InterfaceAdded(
    const dbus::ObjectPath& /*path*/,
    const brillo::VariantDictionary& /*properties*/) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
}

void ChromeosSupplicantProcessProxy::InterfaceRemoved(
    const dbus::ObjectPath& /*path*/) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
}

void ChromeosSupplicantProcessProxy::PropertiesChanged(
    const brillo::VariantDictionary& /*properties*/) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__;
}

void ChromeosSupplicantProcessProxy::OnServiceAvailable(bool available) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__ << ": " << available;

  // The callback might invoke calls to the ObjectProxy, so defer the callback
  // to event loop.
  if (available && !service_appeared_callback_.is_null()) {
    dispatcher_->PostTask(service_appeared_callback_);
  } else if (!available && !service_vanished_callback_.is_null()) {
    dispatcher_->PostTask(service_vanished_callback_);
  }
  service_available_ = available;
}

void ChromeosSupplicantProcessProxy::OnServiceOwnerChanged(
    const string& old_owner, const string& new_owner) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__
      << "old: " << old_owner << " new: " << new_owner;
  if (new_owner.empty()) {
    OnServiceAvailable(false);
  } else {
    OnServiceAvailable(true);
  }
}

void ChromeosSupplicantProcessProxy::OnPropertyChanged(
    const std::string& property_name) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__ << ": "
      << property_name;
}

void ChromeosSupplicantProcessProxy::OnSignalConnected(
    const string& interface_name, const string& signal_name, bool success) {
  SLOG(&supplicant_proxy_->GetObjectPath(), 2) << __func__
      << "interface: " << interface_name << " signal: " << signal_name
      << "success: " << success;
  if (!success) {
    LOG(ERROR) << "Failed to connect signal " << signal_name
        << " to interface " << interface_name;
  }
}

}  // namespace shill
