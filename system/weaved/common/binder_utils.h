// Copyright 2016 The Android Open Source Project
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

#ifndef COMMON_BINDER_UTILS_H_
#define COMMON_BINDER_UTILS_H_

#include <memory>
#include <string>

#include <base/values.h>
#include <binder/Status.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <brillo/errors/error.h>

namespace weave {
class Error;  // Forward declaration.
using ErrorPtr = std::unique_ptr<Error>;
}  // namespace weave

namespace weaved {
namespace binder_utils {

// Converts the result of weave API call into a binder Status object.
// If |success| is true, return binder::Status::ok(), otherwise the method
// constructs a service-specific failure status with an error message obtained
// from the |error| object.
android::binder::Status ToStatus(bool success, weave::ErrorPtr* error);

// Converts a binder status code to a Brillo error object. Returns true if the
// status was isOk(), otherwise returns false and provides error information
// in the |error| object.
bool StatusToError(android::binder::Status status, brillo::ErrorPtr* error);

// Converts binder's UTF16 string into a regular UTF8-encoded standard string.
inline std::string ToString(const android::String16& value) {
  return android::String8{value}.string();
}

// Converts regular UTF8-encoded standard string into a binder's UTF16 string.
inline android::String16 ToString16(const std::string& value) {
  return android::String16{value.c_str()};
}

// Serializes a dictionary to a string for transferring over binder.
android::String16 ToString16(const base::Value& value);

// De-serializes a dictionary from a binder string.
android::binder::Status ParseDictionary(
    const android::String16& json,
    std::unique_ptr<base::DictionaryValue>* dict);

}  // namespace binder_utils
}  // namespace weaved

#endif  // COMMON_BINDER_UTILS_H_
