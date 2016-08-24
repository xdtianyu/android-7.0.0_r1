//
// Copyright (C) 2013 The Android Open Source Project
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

#include "shill/file_reader.h"

#include <base/files/file_util.h>

using base::FilePath;
using std::string;

namespace shill {

FileReader::FileReader() {
}

FileReader::~FileReader() {
}

void FileReader::Close() {
  file_.reset();
}

bool FileReader::Open(const FilePath& file_path) {
  file_.reset(base::OpenFile(file_path, "rb"));
  return file_.get() != nullptr;
}

bool FileReader::ReadLine(string* line) {
  CHECK(line) << "Invalid argument";

  FILE* fp = file_.get();
  if (!fp)
    return false;

  line->clear();
  bool line_valid = false;
  int ch;
  while ((ch = fgetc(fp)) != EOF) {
    if (ch == '\n')
      return true;
    line->push_back(static_cast<char>(ch));
    line_valid = true;
  }
  return line_valid;
}

}  // namespace shill
