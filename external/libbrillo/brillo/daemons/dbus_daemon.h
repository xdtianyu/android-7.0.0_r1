// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DAEMONS_DBUS_DAEMON_H_
#define LIBBRILLO_BRILLO_DAEMONS_DBUS_DAEMON_H_

#include <memory>
#include <string>

#include <base/strings/string_piece.h>
#include <base/memory/ref_counted.h>
#include <brillo/brillo_export.h>
#include <brillo/daemons/daemon.h>
#include <brillo/dbus/dbus_connection.h>
#include <brillo/dbus/exported_object_manager.h>
#include <dbus/bus.h>

namespace brillo {

namespace dbus_utils {
class AsyncEventSequencer;
}  // namespace dbus_utils

// DBusDaemon adds D-Bus support to Daemon.
// Derive your daemon from this class if you want D-Bus client services in your
// daemon (consuming other D-Bus objects). Currently uses a SYSTEM bus.
class BRILLO_EXPORT DBusDaemon : public Daemon {
 public:
  DBusDaemon();
  ~DBusDaemon() override = default;

 protected:
  // Calls the base OnInit() and then instantiates dbus::Bus and establishes
  // a D-Bus connection.
  int OnInit() override;

  // A reference to the |dbus_connection_| bus object often used by derived
  // classes.
  scoped_refptr<dbus::Bus> bus_;

 private:
  DBusConnection dbus_connection_;

  DISALLOW_COPY_AND_ASSIGN(DBusDaemon);
};

// DBusServiceDaemon adds D-Bus service support to DBusDaemon.
// Derive your daemon from this class if your daemon exposes D-Bus objects.
// Provides an ExportedObjectManager to announce your object/interface creation
// and destruction.
class BRILLO_EXPORT DBusServiceDaemon : public DBusDaemon {
 public:
  // Constructs the daemon.
  // |service_name| is the name of D-Bus service provided by the daemon.
  // |object_manager_path_| is a well-known D-Bus object path for
  // ExportedObjectManager object.
  // If |object_manager_path_| is not specified, then ExportedObjectManager is
  // not created and is not available as part of the D-Bus service.
  explicit DBusServiceDaemon(const std::string& service_name);
  DBusServiceDaemon(const std::string& service_name,
                    const dbus::ObjectPath& object_manager_path);
  DBusServiceDaemon(const std::string& service_name,
                    base::StringPiece object_manager_path);

 protected:
  // OnInit() overload exporting D-Bus objects. Exports the contained
  // ExportedObjectManager object and calls RegisterDBusObjectsAsync() to let
  // you provide additional D-Bus objects.
  int OnInit() override;

  // Overload this method to export your custom D-Bus objects at startup.
  // Objects exported in this way will finish exporting before we claim the
  // daemon's service name on DBus.
  virtual void RegisterDBusObjectsAsync(
      dbus_utils::AsyncEventSequencer* sequencer);

  std::string service_name_;
  dbus::ObjectPath object_manager_path_;
  std::unique_ptr<dbus_utils::ExportedObjectManager> object_manager_;

 private:
  // A callback that will be called when all the D-Bus objects/interfaces are
  // exported successfully and the daemon is ready to claim the D-Bus service
  // ownership.
  void TakeServiceOwnership(bool success);

  DISALLOW_COPY_AND_ASSIGN(DBusServiceDaemon);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DAEMONS_DBUS_DAEMON_H_
