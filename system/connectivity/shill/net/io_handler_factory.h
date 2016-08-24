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

#ifndef SHILL_NET_IO_HANDLER_FACTORY_H_
#define SHILL_NET_IO_HANDLER_FACTORY_H_

#include "shill/net/io_handler.h"
#include "shill/net/shill_export.h"

namespace shill {

class SHILL_EXPORT IOHandlerFactory {
 public:
  IOHandlerFactory();
  virtual ~IOHandlerFactory();

  virtual IOHandler* CreateIOInputHandler(
      int fd,
      const IOHandler::InputCallback& input_callback,
      const IOHandler::ErrorCallback& error_callback);

  virtual IOHandler* CreateIOReadyHandler(
      int fd,
      IOHandler::ReadyMode mode,
      const IOHandler::ReadyCallback& input_callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(IOHandlerFactory);
};

}  // namespace shill

#endif  // SHILL_NET_IO_HANDLER_FACTORY_H_
