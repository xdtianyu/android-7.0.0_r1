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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_INTERFACE_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_INTERFACE_H_

#include <memory>

#include <libwebserv/request.h>
#include <libwebserv/response.h>

namespace libwebserv {

// The base interface for HTTP request handlers. When registering a handler,
// the RequestHandlerInterface is provided, and when a request comes in,
// RequestHandlerInterface::HandleRequest() is called to process the data and
// send response.
class RequestHandlerInterface {
 public:
  using HandlerSignature =
      void(std::unique_ptr<Request>, std::unique_ptr<Response>);
  virtual ~RequestHandlerInterface() = default;

  virtual void HandleRequest(std::unique_ptr<Request> request,
                             std::unique_ptr<Response> response) = 0;
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_INTERFACE_H_
