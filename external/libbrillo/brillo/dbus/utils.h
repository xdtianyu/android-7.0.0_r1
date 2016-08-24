// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_UTILS_H_
#define LIBBRILLO_BRILLO_DBUS_UTILS_H_

#include <memory>
#include <string>

#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <dbus/exported_object.h>
#include <dbus/message.h>
#include <dbus/scoped_dbus_error.h>

namespace brillo {
namespace dbus_utils {

// A helper function to create a D-Bus error response object as unique_ptr<>.
BRILLO_EXPORT std::unique_ptr<dbus::Response> CreateDBusErrorResponse(
    dbus::MethodCall* method_call,
    const std::string& error_name,
    const std::string& error_message);

// Create a D-Bus error response object from brillo::Error. If the last
// error in the error chain belongs to "dbus" error domain, its error code
// and message are directly translated to D-Bus error code and message.
// Any inner errors are formatted as "domain/code:message" string and appended
// to the D-Bus error message, delimited by semi-colons.
BRILLO_EXPORT std::unique_ptr<dbus::Response> GetDBusError(
    dbus::MethodCall* method_call,
    const brillo::Error* error);

// AddDBusError() is the opposite of GetDBusError(). It de-serializes the Error
// object received over D-Bus.
BRILLO_EXPORT void AddDBusError(brillo::ErrorPtr* error,
                                const std::string& dbus_error_name,
                                const std::string& dbus_error_message);

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_UTILS_H_
