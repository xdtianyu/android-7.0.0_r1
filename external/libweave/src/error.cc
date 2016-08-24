// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <weave/error.h>

#include <base/logging.h>
#include <base/strings/stringprintf.h>

namespace weave {

namespace {
inline void LogError(const tracked_objects::Location& location,
                     const std::string& code,
                     const std::string& message) {
  // Use logging::LogMessage() directly instead of LOG(ERROR) to substitute
  // the current error location with the location passed in to the Error object.
  // This way the log will contain the actual location of the error, and not
  // as if it always comes from chromeos/errors/error.cc(22).
  logging::LogMessage(location.file_name(), location.line_number(),
                      logging::LOG_ERROR)
          .stream()
      << location.function_name() << "(...): "
      << "Code=" << code << ", Message=" << message;
}
}  // anonymous namespace

ErrorPtr Error::Create(const tracked_objects::Location& location,
                       const std::string& code,
                       const std::string& message) {
  return Create(location, code, message, ErrorPtr());
}

ErrorPtr Error::Create(const tracked_objects::Location& location,
                       const std::string& code,
                       const std::string& message,
                       ErrorPtr inner_error) {
  LogError(location, code, message);
  return ErrorPtr(new Error(location, code, message, std::move(inner_error)));
}

Error::AddToTypeProxy Error::AddTo(ErrorPtr* error,
                                   const tracked_objects::Location& location,
                                   const std::string& code,
                                   const std::string& message) {
  if (error) {
    *error = Create(location, code, message, std::move(*error));
  } else {
    // Create already logs the error, but if |error| is nullptr,
    // we still want to log the error...
    LogError(location, code, message);
  }
  return {};
}

Error::AddToTypeProxy Error::AddToPrintf(
    ErrorPtr* error,
    const tracked_objects::Location& location,
    const std::string& code,
    const char* format,
    ...) {
  va_list ap;
  va_start(ap, format);
  std::string message = base::StringPrintV(format, ap);
  va_end(ap);
  AddTo(error, location, code, message);
  return {};
}

ErrorPtr Error::Clone() const {
  ErrorPtr inner_error = inner_error_ ? inner_error_->Clone() : nullptr;
  return ErrorPtr(
      new Error(location_, code_, message_, std::move(inner_error)));
}

bool Error::HasError(const std::string& code) const {
  return FindError(this, code) != nullptr;
}

const Error* Error::GetFirstError() const {
  const Error* err = this;
  while (err->GetInnerError())
    err = err->GetInnerError();
  return err;
}

Error::Error(const tracked_objects::Location& location,
             const std::string& code,
             const std::string& message,
             ErrorPtr inner_error)
    : Error{tracked_objects::LocationSnapshot{location}, code, message,
            std::move(inner_error)} {}

Error::Error(const tracked_objects::LocationSnapshot& location,
             const std::string& code,
             const std::string& message,
             ErrorPtr inner_error)
    : code_(code),
      message_(message),
      location_(location),
      inner_error_(std::move(inner_error)) {}

const Error* Error::FindError(const Error* error_chain_start,
                              const std::string& code) {
  while (error_chain_start) {
    if (error_chain_start->GetCode() == code)
      break;
    error_chain_start = error_chain_start->GetInnerError();
  }
  return error_chain_start;
}

}  // namespace weave
