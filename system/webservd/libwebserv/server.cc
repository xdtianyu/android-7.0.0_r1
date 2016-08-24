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

#include <libwebserv/server.h>

#if defined(WEBSERV_USE_DBUS)
#include "libwebserv/dbus_server.h"
#endif  // defined(WEBSERV_USE_DBUS)

#if defined(WEBSERV_USE_BINDER)
#include "libwebserv/binder_server.h"
#endif  // defined(WEBSERV_USE_BINDER)

using std::unique_ptr;

namespace libwebserv {

#if defined(WEBSERV_USE_DBUS)
unique_ptr<Server> Server::ConnectToServerViaDBus(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& service_name,
    const brillo::dbus_utils::AsyncEventSequencer::CompletionAction& cb,
    const base::Closure& on_server_online,
    const base::Closure& on_server_offline) {
  DBusServer* server = new DBusServer;
  unique_ptr<Server> ret(server);
  server->Connect(bus, service_name, cb, on_server_online, on_server_offline);
  return ret;
}
#endif  // defined(WEBSERV_USE_DBUS)

#if defined(WEBSERV_USE_BINDER)
std::unique_ptr<Server> ConnectToServerViaBinder(
    brillo::MessageLoop* message_loop,
    const base::Closure& on_server_online,
    const base::Closure& on_server_offline) {
  return unique_ptr<Server>(new BinderServer(
      message_loop, on_server_online, on_server_offline,
      android::BinderWrapper::GetOrCreateInstance()));
}
#endif  // defined(WEBSERV_USE_BINDER)

}  // namespace libwebserv
