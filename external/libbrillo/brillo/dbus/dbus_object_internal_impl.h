// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides internal implementation details of dispatching D-Bus
// method calls to a D-Bus object methods by reading the expected parameter
// values from D-Bus message buffer then invoking a native C++ callback with
// those parameters passed in. If the callback returns a value, that value is
// sent back to the caller of D-Bus method via the response message.

// This is achieved by redirecting the parsing of parameter values from D-Bus
// message buffer to DBusParamReader helper class.
// DBusParamReader de-serializes the parameter values from the D-Bus message
// and calls the provided native C++ callback with those arguments.
// However it expects the callback with a simple signature like this:
//    void callback(Args...);
// The method handlers for DBusObject, on the other hand, have one of the
// following signatures:
//    void handler(Args...);
//    ReturnType handler(Args...);
//    bool handler(ErrorPtr* error, Args...);
//    void handler(std::unique_ptr<DBusMethodResponse<T1, T2,...>>, Args...);
//
// To make this all work, we craft a simple callback suitable for
// DBusParamReader using a lambda in DBusInvoker::Invoke() and redirect the call
// to the appropriate method handler using additional data captured by the
// lambda object.

#ifndef LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_INTERNAL_IMPL_H_
#define LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_INTERNAL_IMPL_H_

#include <memory>
#include <string>
#include <type_traits>

#include <brillo/dbus/data_serialization.h>
#include <brillo/dbus/dbus_method_response.h>
#include <brillo/dbus/dbus_param_reader.h>
#include <brillo/dbus/dbus_param_writer.h>
#include <brillo/dbus/utils.h>
#include <brillo/errors/error.h>
#include <dbus/message.h>

namespace brillo {
namespace dbus_utils {

// This is an abstract base class to allow dispatching a native C++ callback
// method when a corresponding D-Bus method is called.
class DBusInterfaceMethodHandlerInterface {
 public:
  virtual ~DBusInterfaceMethodHandlerInterface() = default;

  // Returns true if the method has been handled synchronously (whether or not
  // a success or error response message had been sent).
  virtual void HandleMethod(dbus::MethodCall* method_call,
                            ResponseSender sender) = 0;
};

// This is a special implementation of DBusInterfaceMethodHandlerInterface for
// extremely simple synchronous method handlers that cannot possibly fail
// (that is, they do not send an error response).
// The handler is expected to take an arbitrary number of arguments of type
// |Args...| which can contain both inputs (passed in by value or constant
// reference) and outputs (passed in as pointers)...
// It may also return a single value of type R (or could be a void function if
// no return value is to be sent to the caller). If the handler has a return
// value, then it cannot have any output parameters in its parameter list.
// The signature of the callback handler is expected to be:
//    R(Args...)
template<typename R, typename... Args>
class SimpleDBusInterfaceMethodHandler
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit SimpleDBusInterfaceMethodHandler(
      const base::Callback<R(Args...)>& handler) : handler_(handler) {}

  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    DBusMethodResponse<R> method_response(method_call, sender);
    auto invoke_callback = [this, &method_response](const Args&... args) {
      method_response.Return(handler_.Run(args...));
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    // The handler is expected a return value, don't allow output parameters.
    if (!DBusParamReader<false, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a DBus method is dispatched.
  base::Callback<R(Args...)> handler_;
  DISALLOW_COPY_AND_ASSIGN(SimpleDBusInterfaceMethodHandler);
};

// Specialization of SimpleDBusInterfaceMethodHandlerInterface for
// R=void (methods with no return values).
template<typename... Args>
class SimpleDBusInterfaceMethodHandler<void, Args...>
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit SimpleDBusInterfaceMethodHandler(
      const base::Callback<void(Args...)>& handler) : handler_(handler) {}

  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    DBusMethodResponseBase method_response(method_call, sender);
    auto invoke_callback = [this, &method_response](const Args&... args) {
      handler_.Run(args...);
      auto response = method_response.CreateCustomResponse();
      dbus::MessageWriter writer(response.get());
      DBusParamWriter::AppendDBusOutParams(&writer, args...);
      method_response.SendRawResponse(std::move(response));
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    if (!DBusParamReader<true, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a DBus method is dispatched.
  base::Callback<void(Args...)> handler_;
  DISALLOW_COPY_AND_ASSIGN(SimpleDBusInterfaceMethodHandler);
};

// An implementation of DBusInterfaceMethodHandlerInterface for simple
// synchronous method handlers that may fail and return an error response
// message.
// The handler is expected to take an arbitrary number of arguments of type
// |Args...| which can contain both inputs (passed in by value or constant
// reference) and outputs (passed in as pointers)...
// In case of an error, the handler must return false and set the error details
// into the |error| object provided.
// The signature of the callback handler is expected to be:
//    bool(ErrorPtr*, Args...)
template<typename... Args>
class SimpleDBusInterfaceMethodHandlerWithError
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit SimpleDBusInterfaceMethodHandlerWithError(
      const base::Callback<bool(ErrorPtr*, Args...)>& handler)
      : handler_(handler) {}

  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    DBusMethodResponseBase method_response(method_call, sender);
    auto invoke_callback = [this, &method_response](const Args&... args) {
      ErrorPtr error;
      if (!handler_.Run(&error, args...)) {
        method_response.ReplyWithError(error.get());
      } else {
        auto response = method_response.CreateCustomResponse();
        dbus::MessageWriter writer(response.get());
        DBusParamWriter::AppendDBusOutParams(&writer, args...);
        method_response.SendRawResponse(std::move(response));
      }
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    if (!DBusParamReader<true, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a DBus method is dispatched.
  base::Callback<bool(ErrorPtr*, Args...)> handler_;
  DISALLOW_COPY_AND_ASSIGN(SimpleDBusInterfaceMethodHandlerWithError);
};

// An implementation of SimpleDBusInterfaceMethodHandlerWithErrorAndMessage
// which is almost identical to SimpleDBusInterfaceMethodHandlerWithError with
// the exception that the callback takes an additional parameter - raw D-Bus
// message used to invoke the method handler.
// The handler is expected to take an arbitrary number of arguments of type
// |Args...| which can contain both inputs (passed in by value or constant
// reference) and outputs (passed in as pointers)...
// In case of an error, the handler must return false and set the error details
// into the |error| object provided.
// The signature of the callback handler is expected to be:
//    bool(ErrorPtr*, dbus::Message*, Args...)
template<typename... Args>
class SimpleDBusInterfaceMethodHandlerWithErrorAndMessage
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit SimpleDBusInterfaceMethodHandlerWithErrorAndMessage(
      const base::Callback<bool(ErrorPtr*, dbus::Message*, Args...)>& handler)
      : handler_(handler) {}

  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    DBusMethodResponseBase method_response(method_call, sender);
    auto invoke_callback =
        [this, method_call, &method_response](const Args&... args) {
      ErrorPtr error;
      if (!handler_.Run(&error, method_call, args...)) {
        method_response.ReplyWithError(error.get());
      } else {
        auto response = method_response.CreateCustomResponse();
        dbus::MessageWriter writer(response.get());
        DBusParamWriter::AppendDBusOutParams(&writer, args...);
        method_response.SendRawResponse(std::move(response));
      }
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    if (!DBusParamReader<true, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a DBus method is dispatched.
  base::Callback<bool(ErrorPtr*, dbus::Message*, Args...)> handler_;
  DISALLOW_COPY_AND_ASSIGN(SimpleDBusInterfaceMethodHandlerWithErrorAndMessage);
};

// An implementation of DBusInterfaceMethodHandlerInterface for more generic
// (and possibly asynchronous) method handlers. The handler is expected
// to take an arbitrary number of input arguments of type |Args...| and send
// the method call response (including a possible error response) using
// the provided DBusMethodResponse object.
// The signature of the callback handler is expected to be:
//    void(std::unique_ptr<DBusMethodResponse<RetTypes...>, Args...)
template<typename Response, typename... Args>
class DBusInterfaceMethodHandler : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit DBusInterfaceMethodHandler(
      const base::Callback<void(std::unique_ptr<Response>, Args...)>& handler)
      : handler_(handler) {}

  // This method forwards the call to |handler_| after extracting the required
  // arguments from the DBus message buffer specified in |method_call|.
  // The output parameters of |handler_| (if any) are sent back to the called.
  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    auto invoke_callback = [this, method_call, &sender](const Args&... args) {
      std::unique_ptr<Response> response(new Response(method_call, sender));
      handler_.Run(std::move(response), args...);
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    if (!DBusParamReader<false, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      DBusMethodResponseBase method_response(method_call, sender);
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a D-Bus method is dispatched.
  base::Callback<void(std::unique_ptr<Response>, Args...)> handler_;

  DISALLOW_COPY_AND_ASSIGN(DBusInterfaceMethodHandler);
};

// An implementation of DBusInterfaceMethodHandlerWithMessage which is almost
// identical to AddSimpleMethodHandlerWithError with the exception that the
// callback takes an additional parameter - raw D-Bus message.
// The handler is expected to take an arbitrary number of input arguments of
// type |Args...| and send the method call response (including a possible error
// response) using the provided DBusMethodResponse object.
// The signature of the callback handler is expected to be:
//    void(std::unique_ptr<DBusMethodResponse<RetTypes...>, dbus::Message*,
//         Args...);
template<typename Response, typename... Args>
class DBusInterfaceMethodHandlerWithMessage
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  explicit DBusInterfaceMethodHandlerWithMessage(
      const base::Callback<void(std::unique_ptr<Response>, dbus::Message*,
                                Args...)>& handler)
      : handler_(handler) {}

  // This method forwards the call to |handler_| after extracting the required
  // arguments from the DBus message buffer specified in |method_call|.
  // The output parameters of |handler_| (if any) are sent back to the called.
  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    auto invoke_callback = [this, method_call, &sender](const Args&... args) {
      std::unique_ptr<Response> response(new Response(method_call, sender));
      handler_.Run(std::move(response), method_call, args...);
    };

    ErrorPtr param_reader_error;
    dbus::MessageReader reader(method_call);
    if (!DBusParamReader<false, Args...>::Invoke(
            invoke_callback, &reader, &param_reader_error)) {
      // Error parsing method arguments.
      DBusMethodResponseBase method_response(method_call, sender);
      method_response.ReplyWithError(param_reader_error.get());
    }
  }

 private:
  // C++ callback to be called when a D-Bus method is dispatched.
  base::Callback<void(std::unique_ptr<Response>,
                      dbus::Message*, Args...)> handler_;

  DISALLOW_COPY_AND_ASSIGN(DBusInterfaceMethodHandlerWithMessage);
};

// An implementation of DBusInterfaceMethodHandlerInterface that has custom
// processing of both input and output parameters. This class is used by
// DBusObject::AddRawMethodHandler and expects the callback to be of the
// following signature:
//    void(dbus::MethodCall*, ResponseSender)
// It will be up to the callback to parse the input parameters from the
// message buffer and construct the D-Bus Response object.
class RawDBusInterfaceMethodHandler
    : public DBusInterfaceMethodHandlerInterface {
 public:
  // A constructor that takes a |handler| to be called when HandleMethod()
  // virtual function is invoked.
  RawDBusInterfaceMethodHandler(
      const base::Callback<void(dbus::MethodCall*, ResponseSender)>& handler)
      : handler_(handler) {}

  void HandleMethod(dbus::MethodCall* method_call,
                    ResponseSender sender) override {
    handler_.Run(method_call, sender);
  }

 private:
  // C++ callback to be called when a D-Bus method is dispatched.
  base::Callback<void(dbus::MethodCall*, ResponseSender)> handler_;

  DISALLOW_COPY_AND_ASSIGN(RawDBusInterfaceMethodHandler);
};

}  // namespace dbus_utils
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_DBUS_DBUS_OBJECT_INTERNAL_IMPL_H_
