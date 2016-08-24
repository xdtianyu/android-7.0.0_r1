// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_H_

#include <string>
#include <typeinfo>

#include <base/bind.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/dbus/dbus_param_writer.h>
#include <dbus/message.h>

namespace brillo {
namespace dbus_utils {

class DBusObject;

// Base class for D-Bus signal proxy classes.
// Used mostly to store the polymorphic DBusSignal<...> in a single map
// container inside DBusInterface object.
class BRILLO_EXPORT DBusSignalBase {
 public:
  DBusSignalBase(DBusObject* dbus_object,
                 const std::string& interface_name,
                 const std::string& signal_name);
  virtual ~DBusSignalBase() = default;

 protected:
  bool SendSignal(dbus::Signal* signal) const;

  std::string interface_name_;
  std::string signal_name_;

 private:
  DBusObject* dbus_object_;

  DISALLOW_COPY_AND_ASSIGN(DBusSignalBase);
};

// DBusSignal<...> is a concrete signal proxy class that knows about the
// exact number of signal arguments and their types.
template<typename... Args>
class DBusSignal : public DBusSignalBase {
 public:
  // Expose the custom constructor from DBusSignalBase.
  using DBusSignalBase::DBusSignalBase;
  ~DBusSignal() override = default;

  // DBusSignal<...>::Send(...) dispatches the signal with the given arguments.
  bool Send(const Args&... args) const {
    dbus::Signal signal(interface_name_, signal_name_);
    dbus::MessageWriter signal_writer(&signal);
    DBusParamWriter::Append(&signal_writer, args...);
    return SendSignal(&signal);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DBusSignal);
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_H_
