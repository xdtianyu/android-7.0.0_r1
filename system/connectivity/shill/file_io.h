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

#ifndef SHILL_FILE_IO_H_
#define SHILL_FILE_IO_H_

#include <base/lazy_instance.h>

namespace shill {

// A POSIX file IO wrapper to allow mocking in unit tests.
class FileIO {
 public:
  virtual ~FileIO();

  // This is a singleton -- use FileIO::GetInstance()->Foo().
  static FileIO* GetInstance();

  virtual ssize_t Write(int fd, const void* buf, size_t count);
  virtual ssize_t Read(int fd, void* buf, size_t count);
  virtual int Close(int fd);
  virtual int SetFdNonBlocking(int fd);

 protected:
  FileIO();

 private:
  friend struct base::DefaultLazyInstanceTraits<FileIO>;

  DISALLOW_COPY_AND_ASSIGN(FileIO);
};

}  // namespace shill

#endif  // SHILL_FILE_IO_H_
