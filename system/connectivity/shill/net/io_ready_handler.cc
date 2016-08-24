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

#include "shill/net/io_ready_handler.h"

#include <unistd.h>

#include <base/logging.h>

namespace shill {

IOReadyHandler::IOReadyHandler(int fd,
                               ReadyMode mode,
                               const ReadyCallback& ready_callback)
    : fd_(fd),
      ready_mode_(mode),
      ready_callback_(ready_callback) {}

IOReadyHandler::~IOReadyHandler() {
  Stop();
}

void IOReadyHandler::Start() {
  CHECK(ready_mode_ == kModeOutput || ready_mode_ == kModeInput);
  base::MessageLoopForIO::Mode mode;
  if (ready_mode_ == kModeOutput) {
    mode = base::MessageLoopForIO::WATCH_WRITE;
  } else {
    mode = base::MessageLoopForIO::WATCH_READ;
  }

  if (!base::MessageLoopForIO::current()->WatchFileDescriptor(
          fd_, true, mode, &fd_watcher_, this)) {
    LOG(ERROR) << "WatchFileDescriptor failed on read";
  }
}

void IOReadyHandler::Stop() {
  fd_watcher_.StopWatchingFileDescriptor();
}

void IOReadyHandler::OnFileCanReadWithoutBlocking(int fd) {
  CHECK_EQ(fd_, fd);
  CHECK_EQ(ready_mode_, kModeInput);

  ready_callback_.Run(fd_);
}

void IOReadyHandler::OnFileCanWriteWithoutBlocking(int fd) {
  CHECK_EQ(fd_, fd);
  CHECK_EQ(ready_mode_, kModeOutput);

  ready_callback_.Run(fd_);
}

}  // namespace shill
