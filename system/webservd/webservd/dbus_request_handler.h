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

#ifndef WEBSERVER_WEBSERVD_DBUS_REQUEST_HANDLER_H_
#define WEBSERVER_WEBSERVD_DBUS_REQUEST_HANDLER_H_

#include <string>

#include <base/macros.h>
#include <dbus/object_path.h>

#include "libwebserv/dbus-proxies.h"
#include "webservd/request_handler_interface.h"

namespace webservd {

class Server;

// A D-Bus interface for a request handler.
class DBusRequestHandler final : public RequestHandlerInterface {
 public:
  using RequestHandlerProxy = org::chromium::WebServer::RequestHandlerProxy;
  DBusRequestHandler(Server* server,
                     RequestHandlerProxy* handler_proxy);

  // Called to process an incoming HTTP request this handler is subscribed
  // to handle.
  void HandleRequest(Request* request) override;

 private:
  Server* server_{nullptr};
  RequestHandlerProxy* handler_proxy_{nullptr};

  DISALLOW_COPY_AND_ASSIGN(DBusRequestHandler);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_DBUS_REQUEST_HANDLER_H_
