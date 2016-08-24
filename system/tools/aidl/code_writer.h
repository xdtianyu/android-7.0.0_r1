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

#ifndef AIDL_CODE_WRITER_H_
#define AIDL_CODE_WRITER_H_

#include <memory>
#include <string>

#include <stdio.h>

#include <android-base/macros.h>

namespace android {
namespace aidl {

class CodeWriter {
 public:
  // Write a formatted string to this writer in the usual printf sense.
  // Returns false on error.
  virtual bool Write(const char* format, ...) = 0;
  virtual bool Close() = 0;
  virtual ~CodeWriter() = default;
};  // class CodeWriter

using CodeWriterPtr = std::unique_ptr<CodeWriter>;

// Get a CodeWriter that writes to |output_file|.
CodeWriterPtr GetFileWriter(const std::string& output_file);

// Get a CodeWriter that writes to a string buffer.
// Caller retains ownership of the buffer.
// The buffer must outlive the CodeWriter.
CodeWriterPtr GetStringWriter(std::string* output_buffer);

}  // namespace aidl
}  // namespace android

#endif // AIDL_CODE_WRITER_H_
