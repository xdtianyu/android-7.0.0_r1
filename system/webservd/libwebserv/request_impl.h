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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_IMPL_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_IMPL_H_

#include <string>

#include <base/files/file.h>
#include <base/macros.h>
#include <brillo/streams/stream.h>
#include <libwebserv/request.h>

namespace libwebserv {

class DBusProtocolHandler;

// Implementation of the Request interface.
class RequestImpl final : public Request {
 public:
  // Overrides from Request.
  brillo::StreamPtr GetDataStream() override;

 private:
  friend class DBusServer;

  LIBWEBSERV_PRIVATE RequestImpl(DBusProtocolHandler* handler,
                                 const std::string& url,
                                 const std::string& method);
  DBusProtocolHandler* handler_{nullptr};
  base::File raw_data_fd_;
  bool last_posted_data_was_file_{false};

  DISALLOW_COPY_AND_ASSIGN(RequestImpl);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_IMPL_H_
