// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_signal.h>

#include <brillo/dbus/dbus_object.h>

namespace brillo {
namespace dbus_utils {

DBusSignalBase::DBusSignalBase(DBusObject* dbus_object,
                               const std::string& interface_name,
                               const std::string& signal_name)
    : interface_name_(interface_name),
      signal_name_(signal_name),
      dbus_object_(dbus_object) {
}

bool DBusSignalBase::SendSignal(dbus::Signal* signal) const {
  // This sends the signal asynchronously.  However, the raw message inside
  // the signal object is ref-counted, so we're fine to pass a stack-allocated
  // Signal object here.
  return dbus_object_->SendSignal(signal);
}

}  // namespace dbus_utils
}  // namespace brillo
