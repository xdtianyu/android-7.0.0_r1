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

#include "code_writer.h"

#include <iostream>
#include <stdarg.h>

#include <android-base/stringprintf.h>

using std::cerr;
using std::endl;

namespace android {
namespace aidl {

namespace {

class StringCodeWriter : public CodeWriter {
 public:
  StringCodeWriter(std::string* output_buffer) : output_(output_buffer) {}
  virtual ~StringCodeWriter() = default;

  bool Write(const char* format, ...) override {
    va_list ap;
    va_start(ap, format);
    android::base::StringAppendV(output_, format, ap);
    va_end(ap);
    return true;
  }

  bool Close() override { return true; }

 private:
  std::string* output_;
};  // class StringCodeWriter

class FileCodeWriter : public CodeWriter {
 public:
  FileCodeWriter(FILE* output_file, bool close_on_destruction)
      : output_(output_file),
        close_on_destruction_(close_on_destruction) {}
  virtual ~FileCodeWriter() {
    if (close_on_destruction_ && output_ != nullptr) {
      fclose(output_);
    }
  }

  bool Write(const char* format, ...) override {
    bool success;
    va_list ap;
    va_start(ap, format);
    success = vfprintf(output_, format, ap) >= 0;
    va_end(ap);
    no_error_ = no_error_ && success;
    return success;
  }

  bool Close() override {
    if (output_ != nullptr) {
      no_error_ = fclose(output_) == 0 && no_error_;
      output_ = nullptr;
    }
    return no_error_;
  }

 private:
  bool no_error_ = true;
  FILE* output_;
  bool close_on_destruction_;
};  // class StringCodeWriter

}  // namespace

CodeWriterPtr GetFileWriter(const std::string& output_file) {
  CodeWriterPtr result;
  FILE* to = nullptr;
  bool close_on_destruction = true;
  if (output_file == "-") {
    to = stdout;
    close_on_destruction = false;
  } else {
    // open file in binary mode to ensure that the tool produces the
    // same output on all platforms !!
    to = fopen(output_file.c_str(), "wb");
  }

  if (to != nullptr) {
    result.reset(new FileCodeWriter(to, close_on_destruction));
  } else {
    cerr << "unable to open " << output_file << " for write" << endl;
  }

  return result;
}

CodeWriterPtr GetStringWriter(std::string* output_buffer) {
  return CodeWriterPtr(new StringCodeWriter(output_buffer));
}

}  // namespace aidl
}  // namespace android
