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

#ifndef SHILL_FILE_READER_H_
#define SHILL_FILE_READER_H_

#include <string>

#include <base/macros.h>
#include <base/files/file_path.h>
#include <base/files/scoped_file.h>

namespace shill {

// A helper class for reading a file line-by-line, which is expected to
// be a substitute for std::getline() as the Google C++ style guide disallows
// the use of stream.
class FileReader {
 public:
  FileReader();
  ~FileReader();

  // Closes the file.
  void Close();

  // Opens the file of a given path. Returns true on success.
  bool Open(const base::FilePath& file_path);

  // Reads a line, terminated by either LF or EOF, from the file into
  // a given string, with LF excluded. Returns false if no more line
  // can be read from the file.
  bool ReadLine(std::string* line);

 private:
  // The file to read.
  base::ScopedFILE file_;

  DISALLOW_COPY_AND_ASSIGN(FileReader);
};

}  // namespace shill

#endif  // SHILL_FILE_READER_H_
