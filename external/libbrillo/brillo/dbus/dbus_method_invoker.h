// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a way to call D-Bus methods on objects in remote processes
// as if they were native C++ function calls.

// CallMethodAndBlock (along with CallMethodAndBlockWithTimeout) lets you call
// a D-Bus method synchronously and pass all the required parameters as C++
// function arguments. CallMethodAndBlock relies on automatic C++ to D-Bus data
// serialization implemented in brillo/dbus/data_serialization.h.
// CallMethodAndBlock invokes the D-Bus method and returns the Response.

// The method call response should be parsed with ExtractMethodCallResults().
// The method takes an optional list of pointers to the expected return values
// of the D-Bus method.

// CallMethod and CallMethodWithTimeout are similar to CallMethodAndBlock but
// make the calls asynchronously. They take two callbacks: one for successful
// method invocation and the second is for error conditions.

// Here is an example of synchronous calls:
// Call "std::string MyInterface::MyMethod(int, double)" over D-Bus:

//  using brillo::dbus_utils::CallMethodAndBlock;
//  using brillo::dbus_utils::ExtractMethodCallResults;
//
//  brillo::ErrorPtr error;
//  auto resp = CallMethodAndBlock(obj,
//                                 "org.chromium.MyService.MyInterface",
//                                 "MyMethod",
//                                 &error,
//                                 2, 8.7);
//  std::string return_value;
//  if (resp && ExtractMethodCallResults(resp.get(), &error, &return_value)) {
//    // Use the |return_value|.
//  } else {
//    // An error occurred. Use |error| to get details.
//  }

// And here is how to call D-Bus methods asynchronously:
// Call "std::string MyInterface::MyMethod(int, double)" over D-Bus:

//  using brillo::dbus_utils::CallMethod;
//  using brillo::dbus_utils::ExtractMethodCallResults;
//
//  void OnSuccess(const std::string& return_value) {
//    // Use the |return_value|.
//  }
//
//  void OnError(brillo::Error* error) {
//    // An error occurred. Use |error| to get details.
//  }
//
//  brillo::dbus_utils::CallMethod(obj,
//                                   "org.chromium.MyService.MyInterface",
//                                   "MyMethod",
//                                   base::Bind(OnSuccess),
//                                   base::Bind(OnError),
//                                   2, 8.7);

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_METHOD_INVOKER_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_METHOD_INVOKER_H_

#include <memory>
#include <string>
#include <tuple>

#include <base/bind.h>
#include <brillo/dbus/dbus_param_reader.h>
#include <brillo/dbus/dbus_param_writer.h>
#include <brillo/dbus/utils.h>
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <brillo/brillo_export.h>
#include <dbus/file_descriptor.h>
#include <dbus/message.h>
#include <dbus/object_proxy.h>

namespace brillo {
namespace dbus_utils {

// A helper method to dispatch a blocking D-Bus method call. Can specify
// zero or more method call arguments in |args| which will be sent over D-Bus.
// This method sends a D-Bus message and blocks for a time period specified
// in |timeout_ms| while waiting for a reply. The time out is in milliseconds or
// -1 (DBUS_TIMEOUT_USE_DEFAULT) for default, or DBUS_TIMEOUT_INFINITE for no
// timeout. If a timeout occurs, the response object contains an error object
// with DBUS_ERROR_NO_REPLY error code (those constants come from libdbus
// [dbus/dbus.h]).
// Returns a dbus::Response object on success. On failure, returns nullptr and
// fills in additional error details into the |error| object.
template<typename... Args>
inline std::unique_ptr<dbus::Response> CallMethodAndBlockWithTimeout(
    int timeout_ms,
    dbus::ObjectProxy* object,
    const std::string& interface_name,
    const std::string& method_name,
    ErrorPtr* error,
    const Args&... args) {
  dbus::MethodCall method_call(interface_name, method_name);
  // Add method arguments to the message buffer.
  dbus::MessageWriter writer(&method_call);
  DBusParamWriter::Append(&writer, args...);
  dbus::ScopedDBusError dbus_error;
  auto response = object->CallMethodAndBlockWithErrorDetails(
      &method_call, timeout_ms, &dbus_error);
  if (!response) {
    if (dbus_error.is_set()) {
      Error::AddTo(error,
                   FROM_HERE,
                   errors::dbus::kDomain,
                   dbus_error.name(),
                   dbus_error.message());
    } else {
      Error::AddToPrintf(error,
                         FROM_HERE,
                         errors::dbus::kDomain,
                         DBUS_ERROR_FAILED,
                         "Failed to call D-Bus method: %s.%s",
                         interface_name.c_str(),
                         method_name.c_str());
    }
  }
  return std::unique_ptr<dbus::Response>(response.release());
}

// Same as CallMethodAndBlockWithTimeout() but uses a default timeout value.
template<typename... Args>
inline std::unique_ptr<dbus::Response> CallMethodAndBlock(
    dbus::ObjectProxy* object,
    const std::string& interface_name,
    const std::string& method_name,
    ErrorPtr* error,
    const Args&... args) {
  return CallMethodAndBlockWithTimeout(dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       object,
                                       interface_name,
                                       method_name,
                                       error,
                                       args...);
}

namespace internal {
// In order to support non-copyable dbus::FileDescriptor, we have this
// internal::HackMove() helper function that does really nothing for normal
// types but uses Pass() for file descriptors so we can move them out from
// the temporaries created inside DBusParamReader<...>::Invoke().
// If only libchrome supported real rvalues so we can just do std::move() and
// be done with it.
template <typename T>
inline const T& HackMove(const T& val) {
  return val;
}

// Even though |val| here is passed as const&, the actual value is created
// inside DBusParamReader<...>::Invoke() and is temporary in nature, so it is
// safe to move the file descriptor out of |val|. That's why we are doing
// const_cast here. It is a bit hacky, but there is no negative side effects.
inline dbus::FileDescriptor HackMove(const dbus::FileDescriptor& val) {
  return std::move(const_cast<dbus::FileDescriptor&>(val));
}
}  // namespace internal

// Extracts the parameters of |ResultTypes...| types from the message reader
// and puts the values in the resulting |tuple|. Returns false on error and
// provides additional error details in |error| object.
template<typename... ResultTypes>
inline bool ExtractMessageParametersAsTuple(
    dbus::MessageReader* reader,
    ErrorPtr* error,
    std::tuple<ResultTypes...>* val_tuple) {
  auto callback = [val_tuple](const ResultTypes&... params) {
    *val_tuple = std::forward_as_tuple(internal::HackMove(params)...);
  };
  return DBusParamReader<false, ResultTypes...>::Invoke(
      callback, reader, error);
}
// Overload of ExtractMessageParametersAsTuple to handle reference types in
// tuples created with std::tie().
template<typename... ResultTypes>
inline bool ExtractMessageParametersAsTuple(
    dbus::MessageReader* reader,
    ErrorPtr* error,
    std::tuple<ResultTypes&...>* ref_tuple) {
  auto callback = [ref_tuple](const ResultTypes&... params) {
    *ref_tuple = std::forward_as_tuple(internal::HackMove(params)...);
  };
  return DBusParamReader<false, ResultTypes...>::Invoke(
      callback, reader, error);
}

// A helper method to extract a list of values from a message buffer.
// The function will return false and provide detailed error information on
// failure. It fails if the D-Bus message buffer (represented by the |reader|)
// contains too many, too few parameters or the parameters are of wrong types
// (signatures).
// The usage pattern is as follows:
//
//  int32_t data1;
//  std::string data2;
//  ErrorPtr error;
//  if (ExtractMessageParameters(reader, &error, &data1, &data2)) { ... }
//
// The above example extracts an Int32 and a String from D-Bus message buffer.
template<typename... ResultTypes>
inline bool ExtractMessageParameters(dbus::MessageReader* reader,
                                     ErrorPtr* error,
                                     ResultTypes*... results) {
  auto ref_tuple = std::tie(*results...);
  return ExtractMessageParametersAsTuple<ResultTypes...>(
      reader, error, &ref_tuple);
}

// Convenient helper method to extract return value(s) of a D-Bus method call.
// |results| must be zero or more pointers to data expected to be returned
// from the method called. If an error occurs, returns false and provides
// additional details in |error| object.
//
// It is OK to call this method even if the D-Bus method doesn't expect
// any return values. Just do not specify any output |results|. In this case,
// ExtractMethodCallResults() will verify that the method didn't return any
// data in the |message|.
template<typename... ResultTypes>
inline bool ExtractMethodCallResults(dbus::Message* message,
                                     ErrorPtr* error,
                                     ResultTypes*... results) {
  CHECK(message) << "Unable to extract parameters from a NULL message.";

  dbus::MessageReader reader(message);
  if (message->GetMessageType() == dbus::Message::MESSAGE_ERROR) {
    std::string error_message;
    if (ExtractMessageParameters(&reader, error, &error_message))
      AddDBusError(error, message->GetErrorName(), error_message);
    return false;
  }
  return ExtractMessageParameters(&reader, error, results...);
}

//////////////////////////////////////////////////////////////////////////////
// Asynchronous method invocation support

using AsyncErrorCallback = base::Callback<void(Error* error)>;

// A helper function that translates dbus::ErrorResponse response
// from D-Bus into brillo::Error* and invokes the |callback|.
void BRILLO_EXPORT TranslateErrorResponse(const AsyncErrorCallback& callback,
                                            dbus::ErrorResponse* resp);

// A helper function that translates dbus::Response from D-Bus into
// a list of C++ values passed as parameters to |success_callback|. If the
// response message doesn't have the correct number of parameters, or they
// are of wrong types, an error is sent to |error_callback|.
template<typename... OutArgs>
void TranslateSuccessResponse(
    const base::Callback<void(OutArgs...)>& success_callback,
    const AsyncErrorCallback& error_callback,
    dbus::Response* resp) {
  auto callback = [&success_callback](const OutArgs&... params) {
    if (!success_callback.is_null()) {
      success_callback.Run(params...);
    }
  };
  ErrorPtr error;
  dbus::MessageReader reader(resp);
  if (!DBusParamReader<false, OutArgs...>::Invoke(callback, &reader, &error) &&
      !error_callback.is_null()) {
    error_callback.Run(error.get());
  }
}

// A helper method to dispatch a non-blocking D-Bus method call. Can specify
// zero or more method call arguments in |params| which will be sent over D-Bus.
// This method sends a D-Bus message and returns immediately.
// When the remote method returns successfully, the success callback is
// invoked with the return value(s), if any.
// On error, the error callback is called. Note, the error callback can be
// called synchronously (before CallMethodWithTimeout returns) if there was
// a problem invoking a method (e.g. object or method doesn't exist).
// If the response is not received within |timeout_ms|, an error callback is
// called with DBUS_ERROR_NO_REPLY error code.
template<typename... InArgs, typename... OutArgs>
inline void CallMethodWithTimeout(
    int timeout_ms,
    dbus::ObjectProxy* object,
    const std::string& interface_name,
    const std::string& method_name,
    const base::Callback<void(OutArgs...)>& success_callback,
    const AsyncErrorCallback& error_callback,
    const InArgs&... params) {
  dbus::MethodCall method_call(interface_name, method_name);
  dbus::MessageWriter writer(&method_call);
  DBusParamWriter::Append(&writer, params...);

  dbus::ObjectProxy::ErrorCallback dbus_error_callback =
      base::Bind(&TranslateErrorResponse, error_callback);
  dbus::ObjectProxy::ResponseCallback dbus_success_callback = base::Bind(
      &TranslateSuccessResponse<OutArgs...>, success_callback, error_callback);

  object->CallMethodWithErrorCallback(
      &method_call, timeout_ms, dbus_success_callback, dbus_error_callback);
}

// Same as CallMethodWithTimeout() but uses a default timeout value.
template<typename... InArgs, typename... OutArgs>
inline void CallMethod(dbus::ObjectProxy* object,
                       const std::string& interface_name,
                       const std::string& method_name,
                       const base::Callback<void(OutArgs...)>& success_callback,
                       const AsyncErrorCallback& error_callback,
                       const InArgs&... params) {
  return CallMethodWithTimeout(dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                               object,
                               interface_name,
                               method_name,
                               success_callback,
                               error_callback,
                               params...);
}

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_METHOD_INVOKER_H_
