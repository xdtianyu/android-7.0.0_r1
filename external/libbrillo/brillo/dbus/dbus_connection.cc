// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_connection.h>

#include <sysexits.h>

#include <base/bind.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;

namespace brillo {

DBusConnection::DBusConnection() {
}

DBusConnection::~DBusConnection() {
  if (bus_)
    bus_->ShutdownAndBlock();
}

scoped_refptr<dbus::Bus> DBusConnection::Connect() {
  return ConnectWithTimeout(base::TimeDelta());
}

scoped_refptr<dbus::Bus> DBusConnection::ConnectWithTimeout(
    base::TimeDelta timeout) {
  if (bus_)
    return bus_;

  base::TimeTicks deadline = base::TimeTicks::Now() + timeout;

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;

  scoped_refptr<dbus::Bus> bus = new dbus::Bus(options);

  do {
    if (bus->Connect()) {
      bus_ = bus;
      return bus_;
    }
    LOG(WARNING) << "Failed to get system bus.";
    // Wait 1 second to prevent trashing the device while waiting for the
    // dbus-daemon to start.
    sleep(1);
  } while (base::TimeTicks::Now() < deadline);

  LOG(ERROR) << "Failed to get system bus after " << timeout.InSeconds()
             << " seconds.";
  return nullptr;
}

}  // namespace brillo
