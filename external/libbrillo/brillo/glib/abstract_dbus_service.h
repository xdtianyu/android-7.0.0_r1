// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_GLIB_ABSTRACT_DBUS_SERVICE_H_
#define LIBBRILLO_BRILLO_GLIB_ABSTRACT_DBUS_SERVICE_H_

#include <brillo/brillo_export.h>
#include <brillo/glib/dbus.h>

namespace brillo {

// \precondition No functions in the dbus namespace can be called before
// ::g_type_init();

namespace dbus {
class BRILLO_EXPORT AbstractDbusService {
 public:
  virtual ~AbstractDbusService() {}

  // Setup the wrapped GObject and the GMainLoop
  virtual bool Initialize() = 0;
  virtual bool Reset() = 0;

  // Registers the GObject as a service with the system DBus
  // TODO(wad) make this testable by making BusConn and Proxy
  //           subclassing friendly.
  virtual bool Register(const brillo::dbus::BusConnection& conn);

  // Starts the run loop
  virtual bool Run();

  // Stops the run loop
  virtual bool Shutdown();

  // Used internally during registration to set the
  // proper service information.
  virtual const char* service_name() const = 0;
  virtual const char* service_path() const = 0;
  virtual const char* service_interface() const = 0;
  virtual GObject* service_object() const = 0;

 protected:
  virtual GMainLoop* main_loop() = 0;
};

}  // namespace dbus
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_GLIB_ABSTRACT_DBUS_SERVICE_H_
