//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/error.h"

#include <base/files/file_path.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>

#include "shill/logging.h"

using std::string;

namespace shill {

// static
const Error::Info Error::kInfos[kNumErrors] = {
  { kErrorResultSuccess, "Success (no error)" },
  { kErrorResultFailure, "Operation failed (no other information)" },
  { kErrorResultAlreadyConnected, "Already connected" },
  { kErrorResultAlreadyExists, "Already exists" },
  { kErrorResultIncorrectPin, "Incorrect PIN" },
  { kErrorResultInProgress, "In progress" },
  { kErrorResultInternalError, "Internal error" },
  { kErrorResultInvalidApn, "Invalid APN" },
  { kErrorResultInvalidArguments, "Invalid arguments" },
  { kErrorResultInvalidNetworkName, "Invalid network name" },
  { kErrorResultInvalidPassphrase, "Invalid passphrase" },
  { kErrorResultInvalidProperty, "Invalid property" },
  { kErrorResultNoCarrier, "No carrier" },
  { kErrorResultNotConnected, "Not connected" },
  { kErrorResultNotFound, "Not found" },
  { kErrorResultNotImplemented, "Not implemented" },
  { kErrorResultNotOnHomeNetwork, "Not on home network" },
  { kErrorResultNotRegistered, "Not registered" },
  { kErrorResultNotSupported, "Not supported" },
  { kErrorResultOperationAborted, "Operation aborted" },
  { kErrorResultOperationInitiated, "Operation initiated" },
  { kErrorResultOperationTimeout, "Operation timeout" },
  { kErrorResultPassphraseRequired, "Passphrase required" },
  { kErrorResultPermissionDenied, "Permission denied" },
  { kErrorResultPinBlocked, "SIM PIN is blocked"},
  { kErrorResultPinRequired, "SIM PIN is required"},
  { kErrorResultWrongState, "Wrong state" }
};

Error::Error() {
  Reset();
}

Error::Error(Type type) {
  Populate(type);
}

Error::Error(Type type, const string& message) {
  Populate(type, message);
}

Error::~Error() {}

void Error::Populate(Type type) {
  Populate(type, GetDefaultMessage(type));
}

void Error::Populate(Type type, const string& message) {
  CHECK(type < kNumErrors) << "Error type out of range: " << type;
  type_ = type;
  message_ = message;
}

void Error::Populate(Type type,
                     const string& message,
                     const tracked_objects::Location& location) {
  CHECK(type < kNumErrors) << "Error type out of range: " << type;
  type_ = type;
  message_ = message;
  location_ = location;
}

void Error::Reset() {
  Populate(kSuccess);
}

void Error::CopyFrom(const Error& error) {
  Populate(error.type_, error.message_);
}

bool Error::ToChromeosError(brillo::ErrorPtr* error) const {
  if (IsFailure()) {
    brillo::Error::AddTo(error,
                         location_,
                         brillo::errors::dbus::kDomain,
                         kInfos[type_].dbus_result,
                         message_);
    return true;
  }
  return false;
}

// static
string Error::GetDBusResult(Type type) {
  CHECK(type < kNumErrors) << "Error type out of range: " << type;
  return kInfos[type].dbus_result;
}

// static
string Error::GetDefaultMessage(Type type) {
  CHECK(type < kNumErrors) << "Error type out of range: " << type;
  return kInfos[type].message;
}

// static
void Error::PopulateAndLog(const tracked_objects::Location& from_here,
                           Error* error, Type type, const string& message) {
  string file_name = base::FilePath(from_here.file_name()).BaseName().value();
  LOG(ERROR) << "[" << file_name << "("
             << from_here.line_number() << ")]: "<< message;
  if (error) {
    error->Populate(type, message, from_here);
  }
}

}  // namespace shill

std::ostream& operator<<(std::ostream& stream, const shill::Error& error) {
  stream << error.GetDBusResult(error.type()) << ": " << error.message();
  return stream;
}
