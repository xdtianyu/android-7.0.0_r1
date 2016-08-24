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

#include "apmanager/error.h"

#include <base/files/file_path.h>
#include <base/logging.h>
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

using std::string;

namespace apmanager {

Error::Error() : type_(kSuccess) {}

Error::~Error() {}

void Error::Populate(Type type,
                     const string& message,
                     const tracked_objects::Location& location) {
  CHECK(type < kNumErrors) << "Error type out of range: " << type;
  type_ = type;
  message_ = message;
  location_ = location;
}

void Error::Reset() {
  type_ = kSuccess;
  message_ = "";
  location_ = tracked_objects::Location();
}

bool Error::ToDBusError(brillo::ErrorPtr* error) const {
  if (IsSuccess()) {
    return false;
  }

  string error_code = kErrorInternalError;
  if (type_ == kInvalidArguments) {
    error_code = kErrorInvalidArguments;
  } else if (type_ == kInvalidConfiguration) {
    error_code = kErrorInvalidConfiguration;
  }

  brillo::Error::AddTo(error,
                       location_,
                       brillo::errors::dbus::kDomain,
                       error_code,
                       message_);
  return true;
}

// static
void Error::PopulateAndLog(Error* error,
                           Type type,
                           const string& message,
                           const tracked_objects::Location& from_here) {
  string file_name = base::FilePath(from_here.file_name()).BaseName().value();
  LOG(ERROR) << "[" << file_name << "("
             << from_here.line_number() << ")]: "<< message;
  if (error) {
    error->Populate(type, message, from_here);
  }
}

}  // namespace apmanager
