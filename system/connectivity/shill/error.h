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

#ifndef SHILL_ERROR_H_
#define SHILL_ERROR_H_

#include <memory>
#include <string>

#include <base/location.h>
#include <base/macros.h>

namespace DBus {
class Error;
}  // namespace DBus

namespace brillo {
class Error;
using ErrorPtr = std::unique_ptr<Error>;
}  // namespace brillo

namespace shill {

class Error {
 public:
  enum Type {
    kSuccess = 0,  // No error.
    kOperationFailed,  // failure, otherwise unspecified
    kAlreadyConnected,
    kAlreadyExists,
    kIncorrectPin,
    kInProgress,
    kInternalError,
    kInvalidApn,
    kInvalidArguments,
    kInvalidNetworkName,
    kInvalidPassphrase,
    kInvalidProperty,
    kNoCarrier,
    kNotConnected,
    kNotFound,
    kNotImplemented,
    kNotOnHomeNetwork,
    kNotRegistered,
    kNotSupported,
    kOperationAborted,
    kOperationInitiated,
    kOperationTimeout,
    kPassphraseRequired,
    kPermissionDenied,
    kPinBlocked,
    kPinRequired,
    kWrongState,
    kNumErrors
  };

  Error();  // Success by default.
  explicit Error(Type type);  // Uses the default message for |type|.
  Error(Type type, const std::string& message);
  ~Error();

  void Populate(Type type);  // Uses the default message for |type|.
  void Populate(Type type, const std::string& message);
  void Populate(Type type,
                const std::string& message,
                const tracked_objects::Location& location);

  void Reset();

  void CopyFrom(const Error& error);

  // Sets the Chromeos |error| and returns true if Error represents failure.
  // Leaves error unchanged, and returns false otherwise.
  bool ToChromeosError(brillo::ErrorPtr* error) const;

  Type type() const { return type_; }
  const std::string& message() const { return message_; }

  bool IsSuccess() const { return type_ == kSuccess; }
  bool IsFailure() const { return !IsSuccess() && !IsOngoing(); }
  bool IsOngoing() const { return type_ == kOperationInitiated; }

  static std::string GetDBusResult(Type type);
  static std::string GetDefaultMessage(Type type);

  // Log an error message from |from_here|.  If |error| is non-NULL, also
  // populate it.
  static void PopulateAndLog(const tracked_objects::Location& from_here,
                             Error* error, Type type,
                             const std::string& message);

 private:
  struct Info {
    const char* dbus_result;  // Error type name.
    const char* message;  // Default Error type message.
  };

  static const Info kInfos[kNumErrors];

  Type type_;
  std::string message_;
  tracked_objects::Location location_;

  DISALLOW_COPY_AND_ASSIGN(Error);
};

}  // namespace shill

// stream operator provided to facilitate logging
std::ostream& operator<<(std::ostream& stream, const shill::Error& error);

#endif  // SHILL_ERROR_H_
