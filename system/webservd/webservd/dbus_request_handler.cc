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

#include "webservd/dbus_request_handler.h"

#include <tuple>
#include <vector>

#include <base/bind.h>
#include <brillo/http/http_request.h>
#include <brillo/mime_utils.h>

#include "libwebserv/dbus-proxies.h"
#include "webservd/request.h"
#include "webservd/server.h"

namespace webservd {

namespace {

void OnError(Request* request,
             bool debug,
             brillo::Error* error) {
  std::string error_msg{"Internal Server Error"};
  if (debug) {
    error_msg += "\r\n" + error->GetMessage();
  }
  request->Complete(brillo::http::status_code::InternalServerError, {},
                    brillo::mime::text::kPlain, error_msg);
}

}  // anonymous namespace

DBusRequestHandler::DBusRequestHandler(Server* server,
                                       RequestHandlerProxy* handler_proxy)
    : server_{server},
      handler_proxy_{handler_proxy} {
}

void DBusRequestHandler::HandleRequest(Request* request) {
  std::vector<std::tuple<std::string, std::string>> headers;
  for (const auto& pair : request->GetHeaders())
    headers.emplace_back(pair.first, pair.second);

  std::vector<std::tuple<int32_t, std::string, std::string, std::string,
                         std::string>> files;
  int32_t index = 0;
  for (const auto& file : request->GetFileInfo()) {
    files.emplace_back(index++, file->field_name, file->file_name,
                        file->content_type, file->transfer_encoding);
  }

  std::vector<std::tuple<bool, std::string, std::string>> params;
  for (const auto& pair : request->GetDataGet())
    params.emplace_back(false, pair.first, pair.second);

  for (const auto& pair : request->GetDataPost())
    params.emplace_back(true, pair.first, pair.second);

  auto error_callback = base::Bind(&OnError,
                                   base::Unretained(request),
                                   server_->GetConfig().use_debug);

  auto request_id = std::make_tuple(request->GetProtocolHandlerID(),
                                    request->GetRequestHandlerID(),
                                    request->GetID(),
                                    request->GetURL(),
                                    request->GetMethod());

  dbus::FileDescriptor body_data_pipe;
  body_data_pipe.PutValue(request->GetBodyDataFileDescriptor());
  body_data_pipe.CheckValidity();
  handler_proxy_->ProcessRequestAsync(
      request_id, headers, params, files, body_data_pipe,
      base::Bind(&base::DoNothing), error_callback);
}

}  // namespace webservd
