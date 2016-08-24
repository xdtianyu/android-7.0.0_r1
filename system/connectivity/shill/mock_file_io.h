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

#ifndef SHILL_MOCK_FILE_IO_H_
#define SHILL_MOCK_FILE_IO_H_

#include "shill/file_io.h"

#include <gmock/gmock.h>

namespace shill {

class MockFileIO : public FileIO {
 public:
  MockFileIO() {}
  ~MockFileIO() override {};
  MOCK_METHOD3(Write, ssize_t(int fd, const void* buf, size_t count));
  MOCK_METHOD3(Read, ssize_t(int fd, void* buf, size_t count));
  MOCK_METHOD1(Close, int(int fd));
  MOCK_METHOD1(SetFdNonBlocking, int(int fd));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockFileIO);
};

}  // namespace shill

#endif  // SHILL_MOCK_FILE_IO_H_
