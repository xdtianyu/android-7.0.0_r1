// Copyright 2016 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_CONNECTION_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_CONNECTION_H_

#include <base/memory/ref_counted.h>
#include <base/time/time.h>
#include <dbus/bus.h>

#include <brillo/brillo_export.h>

namespace brillo {

// DBusConnection adds D-Bus support to Daemon.
class BRILLO_EXPORT DBusConnection final {
 public:
  DBusConnection();
  ~DBusConnection();

  // Instantiates dbus::Bus and establishes a D-Bus connection. Returns a
  // reference to the connected bus, or an empty pointer in case of error.
  scoped_refptr<dbus::Bus> Connect();

  // Instantiates dbus::Bus and tries to establish a D-Bus connection for up to
  // |timeout|. If the connection can't be established after the timeout, fails
  // returning an empty pointer.
  scoped_refptr<dbus::Bus> ConnectWithTimeout(base::TimeDelta timeout);

 private:
  scoped_refptr<dbus::Bus> bus_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DBusConnection);
};

}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DAEMONS_DBUS_DAEMON_H_
