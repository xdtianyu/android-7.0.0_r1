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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_UTILS_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_UTILS_H_

#include <memory>
#include <vector>

#include <base/callback_forward.h>
#include <brillo/errors/error.h>
#include <libwebserv/export.h>

namespace libwebserv {

class Request;
class Response;

using GetRequestDataSuccessCallback =
    base::Callback<void(std::unique_ptr<Request> request,
                        std::unique_ptr<Response> response,
                        std::vector<uint8_t> data)>;

using GetRequestDataErrorCallback =
    base::Callback<void(std::unique_ptr<Request> request,
                        std::unique_ptr<Response> response,
                        const brillo::Error* error)>;

// Reads the request data from |request| asynchronously and returns the data
// by calling |success_callback|. If an error occurred, |error_callback| is
// invoked with the error information passed into |error| parameter.
// The function takes ownership of the request and response objects for the
// duration of operation and returns them back via the callbacks.
LIBWEBSERV_EXPORT void GetRequestData(
    std::unique_ptr<Request> request,
    std::unique_ptr<Response> response,
    const GetRequestDataSuccessCallback& success_callback,
    const GetRequestDataErrorCallback& error_callback);

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_UTILS_H_
