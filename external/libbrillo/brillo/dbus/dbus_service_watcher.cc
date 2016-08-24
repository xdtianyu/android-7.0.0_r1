// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_service_watcher.h>

#include <base/bind.h>

namespace brillo {
namespace dbus_utils {

DBusServiceWatcher::DBusServiceWatcher(
    scoped_refptr<dbus::Bus> bus,
    const std::string& connection_name,
    const base::Closure& on_connection_vanish)
    : bus_{bus},
      connection_name_{connection_name},
      on_connection_vanish_{on_connection_vanish} {
  monitoring_callback_ = base::Bind(
      &DBusServiceWatcher::OnServiceOwnerChange, weak_factory_.GetWeakPtr());
  // Register to listen, and then request the current owner;
  bus_->ListenForServiceOwnerChange(connection_name_, monitoring_callback_);
  bus_->GetServiceOwner(connection_name_, monitoring_callback_);
}

DBusServiceWatcher::~DBusServiceWatcher() {
  bus_->UnlistenForServiceOwnerChange(
      connection_name_, monitoring_callback_);
}

void DBusServiceWatcher::OnServiceOwnerChange(
    const std::string& service_owner) {
  if (service_owner.empty()) {
    on_connection_vanish_.Run();
  }
}

}  // namespace dbus_utils
}  // namespace brillo
