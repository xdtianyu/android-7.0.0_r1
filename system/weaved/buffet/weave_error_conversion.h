// Copyright 2015 The Android Open Source Project
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

#ifndef BUFFET_WEAVE_ERROR_CONVERSION_H_
#define BUFFET_WEAVE_ERROR_CONVERSION_H_

#include <memory>
#include <string>

#include <brillo/errors/error.h>
#include <weave/error.h>

namespace buffet {

inline void ConvertError(const weave::Error& source,
                         std::unique_ptr<brillo::Error>* destination) {
  const weave::Error* inner_error = source.GetInnerError();
  if (inner_error)
    ConvertError(*inner_error, destination);

  const auto& location = source.GetLocation();
  brillo::Error::AddTo(
      destination,
      tracked_objects::Location{
          location.function_name.c_str(), location.file_name.c_str(),
          location.line_number, tracked_objects::GetProgramCounter()},
      "weave", source.GetCode(), source.GetMessage());
}

inline void ConvertError(const brillo::Error& source,
                         std::unique_ptr<weave::Error>* destination) {
  const brillo::Error* inner_error = source.GetInnerError();
  if (inner_error)
    ConvertError(*inner_error, destination);

  const auto& location = source.GetLocation();
  weave::Error::AddTo(
      destination,
      tracked_objects::Location{
          location.function_name.c_str(), location.file_name.c_str(),
          location.line_number, tracked_objects::GetProgramCounter()},
      source.GetCode(), source.GetMessage());
}

}  // namespace buffet

#endif  // BUFFET_WEAVE_ERROR_CONVERSION_H_
