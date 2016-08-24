/*
 * Copyright (C) 2015, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AIDL_AIDL_H_
#define AIDL_AIDL_H_

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "aidl_language.h"
#include "io_delegate.h"
#include "options.h"
#include "type_namespace.h"

namespace android {
namespace aidl {

enum class AidlError {
  UNKOWN = std::numeric_limits<int32_t>::min(),
  BAD_PRE_PROCESSED_FILE,
  PARSE_ERROR,
  FOUND_PARCELABLE,
  BAD_PACKAGE,
  BAD_IMPORT,
  BAD_TYPE,
  BAD_METHOD_ID,
  GENERATION_ERROR,

  OK = 0,
};

int compile_aidl_to_cpp(const CppOptions& options,
                        const IoDelegate& io_delegate);
int compile_aidl_to_java(const JavaOptions& options,
                         const IoDelegate& io_delegate);
bool preprocess_aidl(const JavaOptions& options,
                     const IoDelegate& io_delegate);

namespace internals {

AidlError load_and_validate_aidl(
    const std::vector<std::string> preprocessed_files,
    const std::vector<std::string> import_paths,
    const std::string& input_file_name,
    const IoDelegate& io_delegate,
    TypeNamespace* types,
    std::unique_ptr<AidlInterface>* returned_interface,
    std::vector<std::unique_ptr<AidlImport>>* returned_imports);

bool parse_preprocessed_file(const IoDelegate& io_delegate,
                             const std::string& filename, TypeNamespace* types);

} // namespace internals

}  // namespace android
}  // namespace aidl

#endif  // AIDL_AIDL_H_
