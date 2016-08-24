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

#ifndef WEBSERVER_WEBSERVD_REQUEST_HANDLER_INTERFACE_H_
#define WEBSERVER_WEBSERVD_REQUEST_HANDLER_INTERFACE_H_

#include <string>

#include <base/macros.h>

namespace webservd {

class Request;

// An abstract interface to dispatch of HTTP requests to remote handlers over
// implementation-specific IPC (e.g. D-Bus).
class RequestHandlerInterface {
 public:
  RequestHandlerInterface() = default;
  virtual ~RequestHandlerInterface() = default;

  virtual void HandleRequest(Request* request) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(RequestHandlerInterface);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_REQUEST_HANDLER_INTERFACE_H_
