// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_method_invoker.h>

namespace brillo {
namespace dbus_utils {

void TranslateErrorResponse(const AsyncErrorCallback& callback,
                            dbus::ErrorResponse* resp) {
  if (!callback.is_null()) {
    ErrorPtr error;
    dbus::MessageReader reader(resp);
    std::string error_message;
    if (ExtractMessageParameters(&reader, &error, &error_message))
      AddDBusError(&error, resp->GetErrorName(), error_message);
    callback.Run(error.get());
  }
}

}  // namespace dbus_utils
}  // namespace brillo
