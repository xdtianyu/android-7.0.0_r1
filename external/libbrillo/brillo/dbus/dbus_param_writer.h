// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DBusParamWriter::Append(writer, ...) provides functionality opposite
// to that of DBusParamReader. It writes each of the arguments to D-Bus message
// writer and return true if successful.

// DBusParamWriter::AppendDBusOutParams(writer, ...) is similar to Append()
// but is used to send out the D-Bus OUT (pointer type) parameters in a D-Bus
// method response message. This method skips any non-pointer parameters and
// only appends the data for arguments that are pointers.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_WRITER_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_WRITER_H_

#include <brillo/dbus/data_serialization.h>
#include <dbus/message.h>

namespace brillo {
namespace dbus_utils {

class DBusParamWriter final {
 public:
  // Generic writer method that takes 1 or more arguments. It recursively calls
  // itself (each time with one fewer arguments) until no more is left.
  template<typename ParamType, typename... RestOfParams>
  static void Append(dbus::MessageWriter* writer,
                     const ParamType& param,
                     const RestOfParams&... rest) {
    // Append the current |param| to D-Bus, then call Append() with one
    // fewer arguments, until none is left and stand-alone version of
    // Append(dbus::MessageWriter*) is called to end the iteration.
    DBusType<ParamType>::Write(writer, param);
    Append(writer, rest...);
  }

  // The final overload of DBusParamWriter::Append() used when no more
  // parameters are remaining to be written.
  // Does nothing and finishes meta-recursion.
  static void Append(dbus::MessageWriter* /*writer*/) {}

  // Generic writer method that takes 1 or more arguments. It recursively calls
  // itself (each time with one fewer arguments) until no more is left.
  // Handles non-pointer parameter by just skipping over it.
  template<typename ParamType, typename... RestOfParams>
  static void AppendDBusOutParams(dbus::MessageWriter* writer,
                                  const ParamType& /* param */,
                                  const RestOfParams&... rest) {
    // Skip the current |param| and call Append() with one fewer arguments,
    // until none is left and stand-alone version of
    // AppendDBusOutParams(dbus::MessageWriter*) is called to end the iteration.
    AppendDBusOutParams(writer, rest...);
  }

  // Generic writer method that takes 1 or more arguments. It recursively calls
  // itself (each time with one fewer arguments) until no more is left.
  // Handles only a parameter of pointer type and writes the data pointed to
  // to the output message buffer.
  template<typename ParamType, typename... RestOfParams>
  static void AppendDBusOutParams(dbus::MessageWriter* writer,
                                  ParamType* param,
                                  const RestOfParams&... rest) {
    // Append the current |param| to D-Bus, then call Append() with one
    // fewer arguments, until none is left and stand-alone version of
    // Append(dbus::MessageWriter*) is called to end the iteration.
    DBusType<ParamType>::Write(writer, *param);
    AppendDBusOutParams(writer, rest...);
  }

  // The final overload of DBusParamWriter::AppendDBusOutParams() used when no
  // more parameters are remaining to be written.
  // Does nothing and finishes meta-recursion.
  static void AppendDBusOutParams(dbus::MessageWriter* /*writer*/) {}
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_WRITER_H_
