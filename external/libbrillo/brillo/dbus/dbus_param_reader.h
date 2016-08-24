// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides generic method to parse function call arguments from
// D-Bus message buffer and subsequently invokes a provided native C++ callback
// with the parameter values passed as the callback arguments.

// This functionality is achieved by parsing method arguments one by one,
// left to right from the C++ callback's type signature, and moving the parsed
// arguments to the back to the next call to DBusInvoke::Invoke's arguments as
// const refs.  Each iteration has one fewer template specialization arguments,
// until there is only the return type remaining and we fall through to either
// the void or the non-void final specialization.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_READER_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_READER_H_

#include <type_traits>

#include <brillo/dbus/data_serialization.h>
#include <brillo/dbus/utils.h>
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <dbus/message.h>

namespace brillo {
namespace dbus_utils {

// A generic DBusParamReader stub class which allows us to specialize on
// a variable list of expected function parameters later on.
// This struct in itself is not used. But its concrete template specializations
// defined below are.
// |allow_out_params| controls whether DBusParamReader allows the parameter
// list to contain OUT parameters (pointers).
template<bool allow_out_params, typename...>
struct DBusParamReader;

// A generic specialization of DBusParamReader to handle variable function
// parameters. This specialization pops one parameter off the D-Bus message
// buffer and calls other specializations of DBusParamReader with fewer
// parameters to pop the remaining parameters.
//  CurrentParam  - the type of the current method parameter we are processing.
//  RestOfParams  - the types of remaining parameters to be processed.
template<bool allow_out_params, typename CurrentParam, typename... RestOfParams>
struct DBusParamReader<allow_out_params, CurrentParam, RestOfParams...> {
  // DBusParamReader::Invoke() is a member function that actually extracts the
  // current parameter from the message buffer.
  //  handler     - the C++ callback functor to be called when all the
  //                parameters are processed.
  //  method_call - D-Bus method call object we are processing.
  //  reader      - D-Bus message reader to pop the current argument value from.
  //  args...     - the callback parameters processed so far.
  template<typename CallbackType, typename... Args>
  static bool Invoke(const CallbackType& handler,
                     dbus::MessageReader* reader,
                     ErrorPtr* error,
                     const Args&... args) {
    return InvokeHelper<CurrentParam, CallbackType, Args...>(
        handler, reader, error, static_cast<const Args&>(args)...);
  }

  //
  // There are two specializations of this function:
  //  1. For the case where ParamType is a value type (D-Bus IN parameter).
  //  2. For the case where ParamType is a pointer (D-Bus OUT parameter).
  // In the second case, the parameter is not popped off the message reader,
  // since we do not expect the client to provide any data for it.
  // However after the final handler is called, the values for the OUT
  // parameters should be sent back in the method call response message.

  // Overload 1: ParamType is not a pointer.
  template<typename ParamType, typename CallbackType, typename... Args>
  static typename std::enable_if<!std::is_pointer<ParamType>::value, bool>::type
  InvokeHelper(const CallbackType& handler,
               dbus::MessageReader* reader,
               ErrorPtr* error,
               const Args&... args) {
    if (!reader->HasMoreData()) {
      Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                   DBUS_ERROR_INVALID_ARGS,
                   "Too few parameters in a method call");
      return false;
    }
    // ParamType could be a reference type (e.g. 'const std::string&').
    // Here we need a value type so we can create an object of this type and
    // pop the value off the message buffer into. Using std::decay<> to get
    // the value type. If ParamType is already a value type, ParamValueType will
    // be the same as ParamType.
    using ParamValueType = typename std::decay<ParamType>::type;
    // The variable to hold the value of the current parameter we reading from
    // the message buffer.
    ParamValueType current_param;
    if (!DBusType<ParamValueType>::Read(reader, &current_param)) {
      Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                   DBUS_ERROR_INVALID_ARGS,
                   "Method parameter type mismatch");
      return false;
    }
    // Call DBusParamReader::Invoke() to process the rest of parameters.
    // Note that this is not a recursive call because it is calling a different
    // method of a different class. We exclude the current parameter type
    // (ParamType) from DBusParamReader<> template parameter list and forward
    // all the parameters to the arguments of Invoke() and append the current
    // parameter to the end of the parameter list. We pass it as a const
    // reference to allow to use move-only types such as std::unique_ptr<> and
    // to eliminate unnecessarily copying data.
    return DBusParamReader<allow_out_params, RestOfParams...>::Invoke(
        handler, reader, error,
        static_cast<const Args&>(args)...,
        static_cast<const ParamValueType&>(current_param));
  }

  // Overload 2: ParamType is a pointer.
  template<typename ParamType, typename CallbackType, typename... Args>
  static typename std::enable_if<allow_out_params &&
                                 std::is_pointer<ParamType>::value, bool>::type
      InvokeHelper(const CallbackType& handler,
                   dbus::MessageReader* reader,
                   ErrorPtr* error,
                   const Args&... args) {
    // ParamType is a pointer. This is expected to be an output parameter.
    // Create storage for it and the handler will provide a value for it.
    using ParamValueType = typename std::remove_pointer<ParamType>::type;
    // The variable to hold the value of the current parameter we are passing
    // to the handler.
    ParamValueType current_param{};  // Default-initialize the value.
    // Call DBusParamReader::Invoke() to process the rest of parameters.
    // Note that this is not a recursive call because it is calling a different
    // method of a different class. We exclude the current parameter type
    // (ParamType) from DBusParamReader<> template parameter list and forward
    // all the parameters to the arguments of Invoke() and append the current
    // parameter to the end of the parameter list.
    return DBusParamReader<allow_out_params, RestOfParams...>::Invoke(
        handler, reader, error,
        static_cast<const Args&>(args)...,
        &current_param);
  }
};  // struct DBusParamReader<ParamType, RestOfParams...>

// The final specialization of DBusParamReader<> used when no more parameters
// are expected in the message buffer. Actually dispatches the call to the
// handler with all the accumulated arguments.
template<bool allow_out_params>
struct DBusParamReader<allow_out_params> {
  template<typename CallbackType, typename... Args>
  static bool Invoke(const CallbackType& handler,
                     dbus::MessageReader* reader,
                     ErrorPtr* error,
                     const Args&... args) {
    if (reader->HasMoreData()) {
      Error::AddTo(error, FROM_HERE, errors::dbus::kDomain,
                   DBUS_ERROR_INVALID_ARGS,
                   "Too many parameters in a method call");
      return false;
    }
    handler(args...);
    return true;
  }
};  // struct DBusParamReader<>

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_PARAM_READER_H_
