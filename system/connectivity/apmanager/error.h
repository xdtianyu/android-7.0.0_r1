//
// Copyright (C) 2015 The Android Open Source Project
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

#ifndef APMANAGER_ERROR_H_
#define APMANAGER_ERROR_H_

#include <memory>
#include <string>

#include <base/location.h>
#include <base/macros.h>

namespace brillo {
class Error;
using ErrorPtr = std::unique_ptr<Error>;
}  // namespace brillo

namespace apmanager {

class Error {
 public:
  enum Type {
    kSuccess = 0,  // No error.
    kOperationInProgress,
    kInternalError,
    kInvalidArguments,
    kInvalidConfiguration,
    kNumErrors
  };

  Error();
  ~Error();

  void Populate(Type type,
                const std::string& message,
                const tracked_objects::Location& location);

  void Reset();

  Type type() const { return type_; }
  const std::string& message() const { return message_; }

  bool IsSuccess() const { return type_ == kSuccess; }
  bool IsFailure() const { return !IsSuccess() && !IsOngoing(); }
  bool IsOngoing() const { return type_ == kOperationInProgress; }

  // Log an error message from |from_here|.  If |error| is non-NULL, also
  // populate it.
  static void PopulateAndLog(Error* error,
                             Type type,
                             const std::string& message,
                             const tracked_objects::Location& from_here);

  // TODO(zqiu): put this under a compiler flag (e.g. __DBUS__).
  // Sets the D-Bus error and returns true if Error represents failure.
  // Leaves error unchanged, and returns false otherwise.
  bool ToDBusError(brillo::ErrorPtr* error) const;

 private:
  friend class ErrorTest;

  Type type_;
  std::string message_;
  tracked_objects::Location location_;

  DISALLOW_COPY_AND_ASSIGN(Error);
};

}  // namespace apmanager

#endif  // APMANAGER_ERROR_H_
