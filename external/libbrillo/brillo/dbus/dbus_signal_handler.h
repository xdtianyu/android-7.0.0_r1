// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_HANDLER_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_HANDLER_H_

#include <string>

#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_param_reader.h>
#include <dbus/message.h>
#include <dbus/object_proxy.h>

namespace brillo {
namespace dbus_utils {

// brillo::dbus_utils::ConnectToSignal() is a helper function similar to
// dbus::ObjectProxy::ConnectToSignal() but the |signal_callback| is an actual
// C++ signal handler with expected signal parameters as native method args.
//
// brillo::dbus_utils::ConnectToSignal() actually registers a stub signal
// handler with D-Bus which has a standard signature that matches
// dbus::ObjectProxy::SignalCallback.
//
// When a D-Bus signal is emitted, the stub handler is invoked, which unpacks
// the expected parameters from dbus::Signal message and then calls
// |signal_callback| with unpacked arguments.
//
// If the signal message doesn't contain correct number or types of arguments,
// an error message is logged to the system log and the signal is ignored
// (|signal_callback| is not invoked).
template<typename... Args>
void ConnectToSignal(
    dbus::ObjectProxy* object_proxy,
    const std::string& interface_name,
    const std::string& signal_name,
    base::Callback<void(Args...)> signal_callback,
    dbus::ObjectProxy::OnConnectedCallback on_connected_callback) {
  // DBusParamReader::Invoke() needs a functor object, not a base::Callback.
  // Wrap the callback with lambda so we can redirect the call.
  auto signal_callback_wrapper = [signal_callback](const Args&... args) {
    if (!signal_callback.is_null()) {
      signal_callback.Run(args...);
    }
  };

  // Raw signal handler stub method. When called, unpacks the signal arguments
  // from |signal| message buffer and redirects the call to
  // |signal_callback_wrapper| which, in turn, would call the user-provided
  // |signal_callback|.
  auto dbus_signal_callback = [signal_callback_wrapper](dbus::Signal* signal) {
    dbus::MessageReader reader(signal);
    DBusParamReader<false, Args...>::Invoke(
        signal_callback_wrapper, &reader, nullptr);
  };

  // Register our stub handler with D-Bus ObjectProxy.
  object_proxy->ConnectToSignal(interface_name,
                                signal_name,
                                base::Bind(dbus_signal_callback),
                                on_connected_callback);
}

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_SIGNAL_HANDLER_H_
