// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/daemons/dbus_daemon.h>

#include <sysexits.h>

#include <base/bind.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::ExportedObjectManager;

namespace brillo {

DBusDaemon::DBusDaemon() {
}

int DBusDaemon::OnInit() {
  int exit_code = Daemon::OnInit();
  if (exit_code != EX_OK)
    return exit_code;

  bus_ = dbus_connection_.Connect();
  CHECK(bus_);

  return exit_code;
}

DBusServiceDaemon::DBusServiceDaemon(const std::string& service_name)
    : service_name_(service_name) {
}

DBusServiceDaemon::DBusServiceDaemon(
    const std::string& service_name,
    const dbus::ObjectPath& object_manager_path)
    : service_name_(service_name), object_manager_path_(object_manager_path) {
}

DBusServiceDaemon::DBusServiceDaemon(const std::string& service_name,
                                     base::StringPiece object_manager_path)
    : DBusServiceDaemon(service_name,
                        dbus::ObjectPath(object_manager_path.as_string())) {
}

int DBusServiceDaemon::OnInit() {
  int exit_code = DBusDaemon::OnInit();
  if (exit_code != EX_OK)
    return exit_code;

  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  if (object_manager_path_.IsValid()) {
    object_manager_.reset(
        new ExportedObjectManager(bus_, object_manager_path_));
    object_manager_->RegisterAsync(
        sequencer->GetHandler("ObjectManager.RegisterAsync() failed.", true));
  }
  RegisterDBusObjectsAsync(sequencer.get());
  sequencer->OnAllTasksCompletedCall({
      base::Bind(&DBusServiceDaemon::TakeServiceOwnership,
                 base::Unretained(this))
  });
  return EX_OK;
}

void DBusServiceDaemon::RegisterDBusObjectsAsync(
    dbus_utils::AsyncEventSequencer* /* sequencer */) {
  // Do nothing here.
  // Overload this method to export custom D-Bus objects at daemon startup.
}

void DBusServiceDaemon::TakeServiceOwnership(bool success) {
  // Success should always be true since we've said that failures are fatal.
  CHECK(success) << "Init of one or more objects has failed.";
  CHECK(bus_->RequestOwnershipAndBlock(service_name_,
                                       dbus::Bus::REQUIRE_PRIMARY))
      << "Unable to take ownership of " << service_name_;
}

}  // namespace brillo
