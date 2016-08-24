// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_SERVICE_WATCHER_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_SERVICE_WATCHER_H_

#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <brillo/brillo_export.h>
#include <dbus/bus.h>

namespace brillo {
namespace dbus_utils {

// DBusServiceWatcher just asks the bus to notify us when the owner of a remote
// DBus connection transitions to the empty string.  After registering a
// callback to be notified of name owner transitions, for the given
// |connection_name|, DBusServiceWatcher asks for the current owner.  If at any
// point an empty string is found for the connection name owner,
// DBusServiceWatcher will call back to notify of the connection vanishing.
//
// The chief value of this class is that it manages the lifetime of the
// registered callback in the Bus, because failure to remove callbacks will
// cause the Bus to crash the process on destruction.
class BRILLO_EXPORT DBusServiceWatcher {
 public:
  DBusServiceWatcher(scoped_refptr<dbus::Bus> bus,
                     const std::string& connection_name,
                     const base::Closure& on_connection_vanish);
  virtual ~DBusServiceWatcher();
  virtual std::string connection_name() const { return connection_name_; }

 private:
  void OnServiceOwnerChange(const std::string& service_owner);

  scoped_refptr<dbus::Bus> bus_;
  const std::string connection_name_;
  dbus::Bus::GetServiceOwnerCallback monitoring_callback_;
  base::Closure on_connection_vanish_;

  base::WeakPtrFactory<DBusServiceWatcher> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DBusServiceWatcher);
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_SERVICE_WATCHER_H_
