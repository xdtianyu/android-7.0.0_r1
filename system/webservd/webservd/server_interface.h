// Copyright 2015 The Android Open Source Project
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

#ifndef WEBSERVER_WEBSERVD_SERVER_INTERFACE_H_
#define WEBSERVER_WEBSERVD_SERVER_INTERFACE_H_

#include <base/macros.h>

#include "webservd/config.h"

namespace webservd {

class ProtocolHandler;
class TempFileManager;

// An abstract interface to expose Server object to IPC transport layer such as
// D-Bus.
class ServerInterface {
 public:
  ServerInterface() = default;

  // Called by ProtocolHandler to notify the server that a new protocol handler
  // appears online or goes offline.
  virtual void ProtocolHandlerStarted(ProtocolHandler* handler) = 0;
  virtual void ProtocolHandlerStopped(ProtocolHandler* handler) = 0;

  // Returns the server configuration data.
  virtual const Config& GetConfig() const = 0;

  // Returns the temp file manager used to track life-times of temporary files.
  // The returned pointer is still owned by the server, so it must not be
  // stored or deleted.
  virtual TempFileManager* GetTempFileManager() = 0;

 protected:
  // This interface should not be used to control the life-time of the class
  // that derives from this interface. This is especially important when a mock
  // server class is used. Since the life-time of the mock must be controlled
  // by the test itself, we can't let some business logic suddenly delete
  // the instance of this interface.
  // So, just declare the destructor as protected, so nobody can just call
  // delete on a pointer to ServerInterface.
  ~ServerInterface() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServerInterface);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_SERVER_INTERFACE_H_
