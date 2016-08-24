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

#include "apmanager/dbus/shill_dbus_proxy.h"

#include <base/bind.h>
#include <brillo/errors/error.h>

#if !defined(__ANDROID__)
#include <chromeos/dbus/service_constants.h>
#else
#include <dbus/apmanager/dbus-constants.h>
#endif  // __ANDROID__

#include "apmanager/event_dispatcher.h"

using std::string;

namespace apmanager {

ShillDBusProxy::ShillDBusProxy(
    const scoped_refptr<dbus::Bus>& bus,
    const base::Closure& service_appeared_callback,
    const base::Closure& service_vanished_callback)
    : manager_proxy_(new org::chromium::flimflam::ManagerProxy(bus)),
      dispatcher_(EventDispatcher::GetInstance()),
      service_appeared_callback_(service_appeared_callback),
      service_vanished_callback_(service_vanished_callback),
      service_available_(false) {
  // Monitor service owner changes. This callback lives for the lifetime of
  // the ObjectProxy.
  manager_proxy_->GetObjectProxy()->SetNameOwnerChangedCallback(
      base::Bind(&ShillDBusProxy::OnServiceOwnerChanged,
                 weak_factory_.GetWeakPtr()));

  // One time callback when service becomes available.
  manager_proxy_->GetObjectProxy()->WaitForServiceToBeAvailable(
      base::Bind(&ShillDBusProxy::OnServiceAvailable,
                 weak_factory_.GetWeakPtr()));
}

ShillDBusProxy::~ShillDBusProxy() {}

bool ShillDBusProxy::ClaimInterface(const string& interface_name) {
  if (!service_available_) {
    LOG(ERROR) << "ClaimInterface failed: service not available";
    return false;
  }
  brillo::ErrorPtr error;
  if (!manager_proxy_->ClaimInterface(kServiceName, interface_name, &error)) {
    LOG(ERROR) << "Failed to claim interface from shill: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ShillDBusProxy::ReleaseInterface(const string& interface_name) {
  if (!service_available_) {
    LOG(ERROR) << "ReleaseInterface failed: service not available";
    return false;
  }
  brillo::ErrorPtr error;
  if (!manager_proxy_->ReleaseInterface(kServiceName, interface_name, &error)) {
    LOG(ERROR) << "Failed to release interface from shill: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

#if defined(__BRILLO__)
bool ShillDBusProxy::SetupApModeInterface(string* interface_name) {
  if (!service_available_) {
    LOG(ERROR) << "SetupApModeInterface failed: service not available";
    return false;
  }
  brillo::ErrorPtr error;
  if (!manager_proxy_->SetupApModeInterface(interface_name, &error)) {
    LOG(ERROR) << "Failed to setup AP mode interface from shill: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}

bool ShillDBusProxy::SetupStationModeInterface(string* interface_name) {
  if (!service_available_) {
    LOG(ERROR) << "SetupStationModeInterface failed: service not available";
    return false;
  }
  brillo::ErrorPtr error;
  if (!manager_proxy_->SetupStationModeInterface(interface_name, &error)) {
    LOG(ERROR) << "Failed to setup station mode interface from shill: "
               << error->GetCode() << " " << error->GetMessage();
    return false;
  }
  return true;
}
#endif  // __BRILLO__

void ShillDBusProxy::OnServiceAvailable(bool available) {
  LOG(INFO) << __func__ << ": " << available;
  // Nothing to be done if proxy service not available.
  // The callback might invoke calls to the ObjectProxy, so defer the callback
  // to event loop.
  if (available && !service_appeared_callback_.is_null()) {
    dispatcher_->PostTask(service_appeared_callback_);
  } else if (!available && !service_vanished_callback_.is_null()) {
    dispatcher_->PostTask(service_vanished_callback_);
  }
  service_available_ = available;
}

void ShillDBusProxy::OnServiceOwnerChanged(const string& old_owner,
                                           const string& new_owner) {
  LOG(INFO) << __func__ << " old: " << old_owner << " new: " << new_owner;
  if (new_owner.empty()) {
    OnServiceAvailable(false);
  } else {
    OnServiceAvailable(true);
  }
}

}  // namespace apmanager
