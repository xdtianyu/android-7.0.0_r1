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

#include "common/binder_utils.h"

#include <base/json/json_reader.h>
#include <base/json/json_writer.h>
#include <weave/error.h>

namespace weaved {
namespace binder_utils {

android::binder::Status ToStatus(bool success, weave::ErrorPtr* error) {
  if (success)
    return android::binder::Status::ok();
  return android::binder::Status::fromServiceSpecificError(
      1, android::String8{error->get()->GetMessage().c_str()});
}

bool StatusToError(android::binder::Status status, brillo::ErrorPtr* error) {
  if (status.isOk())
    return true;
  brillo::Error::AddTo(error, FROM_HERE, "binder",
                       std::to_string(status.exceptionCode()),
                       status.exceptionMessage().string());
  return false;
}

android::String16 ToString16(const base::Value& value) {
  std::string json;
  base::JSONWriter::Write(value, &json);
  return ToString16(json);
}

android::binder::Status ParseDictionary(
    const android::String16& json,
    std::unique_ptr<base::DictionaryValue>* dict) {
  int error = 0;
  std::string message;
  std::unique_ptr<base::Value> value{
      base::JSONReader::ReadAndReturnError(ToString(json), base::JSON_PARSE_RFC,
                                           &error, &message)
          .release()};
  base::DictionaryValue* dict_value = nullptr;
  if (!value || !value->GetAsDictionary(&dict_value)) {
    return android::binder::Status::fromServiceSpecificError(
        error, android::String8{message.c_str()});
  }
  dict->reset(dict_value);
  value.release();  // |dict| now owns the object.
  return android::binder::Status::ok();
}

}  // namespace binder_utils
}  // namespace weaved
