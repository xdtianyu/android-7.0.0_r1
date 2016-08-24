// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/dbus/dbus_method_response.h>

#include <brillo/dbus/utils.h>

namespace brillo {
namespace dbus_utils {

DBusMethodResponseBase::DBusMethodResponseBase(dbus::MethodCall* method_call,
                                               ResponseSender sender)
    : sender_(sender), method_call_(method_call) {
}

DBusMethodResponseBase::~DBusMethodResponseBase() {
  if (method_call_) {
    // Response hasn't been sent by the handler. Abort the call.
    Abort();
  }
}

void DBusMethodResponseBase::ReplyWithError(const brillo::Error* error) {
  CheckCanSendResponse();
  auto response = GetDBusError(method_call_, error);
  SendRawResponse(std::move(response));
}

void DBusMethodResponseBase::ReplyWithError(
    const tracked_objects::Location& location,
    const std::string& error_domain,
    const std::string& error_code,
    const std::string& error_message) {
  ErrorPtr error;
  Error::AddTo(&error, location, error_domain, error_code, error_message);
  ReplyWithError(error.get());
}

void DBusMethodResponseBase::Abort() {
  SendRawResponse(std::unique_ptr<dbus::Response>());
}

void DBusMethodResponseBase::SendRawResponse(
    std::unique_ptr<dbus::Response> response) {
  CheckCanSendResponse();
  method_call_ = nullptr;  // Mark response as sent.
  sender_.Run(scoped_ptr<dbus::Response>{response.release()});
}

std::unique_ptr<dbus::Response>
DBusMethodResponseBase::CreateCustomResponse() const {
  return std::unique_ptr<dbus::Response>{
      dbus::Response::FromMethodCall(method_call_).release()};
}

bool DBusMethodResponseBase::IsResponseSent() const {
  return (method_call_ == nullptr);
}

void DBusMethodResponseBase::CheckCanSendResponse() const {
  CHECK(method_call_) << "Response already sent";
}

}  // namespace dbus_utils
}  // namespace brillo
