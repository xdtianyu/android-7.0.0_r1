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

#ifndef SHILL_NET_IO_INPUT_HANDLER_H_
#define SHILL_NET_IO_INPUT_HANDLER_H_

#include <base/message_loop/message_loop.h>

#include "shill/net/io_handler.h"

namespace shill {

// Monitor file descriptor for reading.
class IOInputHandler : public IOHandler,
                       public base::MessageLoopForIO::Watcher {
 public:
  IOInputHandler(int fd,
                 const InputCallback& input_callback,
                 const ErrorCallback& error_callback);
  ~IOInputHandler();

  void Start() override;
  void Stop() override;

 private:
  // base::MessageLoopForIO::Watcher methods.
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  int fd_;
  base::MessageLoopForIO::FileDescriptorWatcher fd_watcher_;
  InputCallback input_callback_;
  ErrorCallback error_callback_;
};

}  // namespace shill

#endif  // SHILL_NET_IO_INPUT_HANDLER_H_
