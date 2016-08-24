//
// Copyright (C) 2014 The Android Open Source Project
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

#include "shill/net/io_input_handler.h"

#include <string>
#include <unistd.h>

#include <base/logging.h>
#include <base/strings/stringprintf.h>

namespace shill {

IOInputHandler::IOInputHandler(int fd,
                               const InputCallback& input_callback,
                               const ErrorCallback& error_callback)
    : fd_(fd),
      input_callback_(input_callback),
      error_callback_(error_callback) {}

IOInputHandler::~IOInputHandler() {
  Stop();
}

void IOInputHandler::Start() {
  if (!base::MessageLoopForIO::current()->WatchFileDescriptor(
          fd_, true, base::MessageLoopForIO::WATCH_READ,
          &fd_watcher_, this)) {
    LOG(ERROR) << "WatchFileDescriptor failed on read";
  }
}

void IOInputHandler::Stop() {
  fd_watcher_.StopWatchingFileDescriptor();
}

void IOInputHandler::OnFileCanReadWithoutBlocking(int fd) {
  CHECK_EQ(fd_, fd);

  unsigned char buf[IOHandler::kDataBufferSize];
  int len = read(fd, buf, sizeof(buf));
  if (len < 0) {
    std::string condition = base::StringPrintf(
        "File read error: %d", errno);
    LOG(ERROR) << condition;
    error_callback_.Run(condition);
  }

  InputData input_data(buf, len);
  input_callback_.Run(&input_data);
}

void IOInputHandler::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED() << "Not watching file descriptor for write";
}

}  // namespace shill
