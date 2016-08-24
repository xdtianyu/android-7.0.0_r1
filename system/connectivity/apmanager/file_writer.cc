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

#include "apmanager/file_writer.h"

#include <base/files/file_util.h>

namespace apmanager {

namespace {

base::LazyInstance<FileWriter> g_file_writer
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

FileWriter::FileWriter() {}
FileWriter::~FileWriter() {}

FileWriter* FileWriter::GetInstance() {
  return g_file_writer.Pointer();
}

bool FileWriter::Write(const std::string& file_name,
                       const std::string& content) {
  if (base::WriteFile(base::FilePath(file_name),
                      content.c_str(),
                      content.size()) == -1) {
    return false;
  }
  return true;
}

}  // namespace apmanager
