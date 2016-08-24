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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_CALLBACK_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_CALLBACK_H_

#include <base/callback.h>
#include <base/macros.h>
#include <libwebserv/export.h>
#include <libwebserv/request_handler_interface.h>

namespace libwebserv {

// A simple request handler that wraps a callback function.
// Essentially, it redirects the RequestHandlerInterface::HandleRequest calls
// to the provided callback.
class LIBWEBSERV_EXPORT RequestHandlerCallback final
    : public RequestHandlerInterface {
 public:
  explicit RequestHandlerCallback(
      const base::Callback<HandlerSignature>& callback);

  void HandleRequest(std::unique_ptr<Request> request,
                     std::unique_ptr<Response> response) override;

 private:
  base::Callback<HandlerSignature> callback_;
  DISALLOW_COPY_AND_ASSIGN(RequestHandlerCallback);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_HANDLER_CALLBACK_H_
