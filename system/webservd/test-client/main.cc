// Copyright 2016 The Android Open Source Project
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

#include <base/bind.h>
#include <base/macros.h>
#include <brillo/mime_utils.h>
#include <brillo/syslog_logging.h>
#include <libwebserv/protocol_handler.h>
#include <libwebserv/request_handler_interface.h>
#include <libwebserv/server.h>
#include <sysexits.h>

#if defined(WEBSERV_USE_DBUS)

#include <brillo/daemons/dbus_daemon.h>
#include <brillo/dbus/async_event_sequencer.h>

// If we're using DBus, pick a base class that does DBus related init.
using WebservTestClientBaseClass = brillo::DBusDaemon;
using brillo::dbus_utils::AsyncEventSequencer;

#elif defined(WEBSERV_USE_BINDER)

#include <brillo/daemons/daemon.h>
using WebservTestClientBaseClass = brillo::Daemon;

#else
#error "You must select one of Binder or DBus as an RPC mechanism."
#endif  // defined(WEBSERV_USE_DBUS)

using libwebserv::Server;
using libwebserv::ProtocolHandler;
using libwebserv::RequestHandlerInterface;
using libwebserv::Request;
using libwebserv::Response;

namespace {

void LogServerOnlineStatus(bool online) {
  LOG(INFO) << "Webserver is "
            << ((online) ? "online" : "offline");
}

class PingRequestHandler : public RequestHandlerInterface {
 public:
  static const char kMethods[];
  static const char kResponse[];
  static const char kUrl[];

  ~PingRequestHandler() override = default;
  void HandleRequest(std::unique_ptr<Request> /* request */,
                     std::unique_ptr<Response> response) override {
    response->ReplyWithText(200, kResponse, brillo::mime::text::kPlain);
  }
};  // class PingRequestHandler

const char PingRequestHandler::kMethods[] = "";  // all methods
const char PingRequestHandler::kResponse[] = "Still alive, still alive!\n";
const char PingRequestHandler::kUrl[] = "/webservd-test-client/ping";

class WebservTestClient : public WebservTestClientBaseClass {
 public:
  WebservTestClient() = default;
  ~WebservTestClient() override = default;

 protected:
  int OnInit() override {
    int exit_code = WebservTestClientBaseClass::OnInit();
    if (exit_code != EX_OK)
      return exit_code;

#ifdef WEBSERV_USE_DBUS
    webserver_ = Server::ConnectToServerViaDBus(
        bus_, bus_->GetConnectionName(),
        AsyncEventSequencer::GetDefaultCompletionAction(),
        base::Bind(&LogServerOnlineStatus, true /* online */),
        base::Bind(&LogServerOnlineStatus, false /* offline */));
#elif WEBSERV_USE_BINDER
    webserver_ = Server::ConnectToServerViaBinder(
        message_loop(),
        base::Bind(&LogServerOnlineStatus, true /* online */),
        base::Bind(&LogServerOnlineStatus, false /* offline */));
#endif  // WEBSERV_USE_DBUS || WEBSERV_USE_BINDER

    // Note that adding this handler is only local, and we won't receive
    // requests until the library does some async book keeping.
    ProtocolHandler* http_handler = webserver_->GetDefaultHttpHandler();
    http_handler->AddHandler(
        PingRequestHandler::kUrl,
        PingRequestHandler::kMethods,
        std::unique_ptr<RequestHandlerInterface>(new PingRequestHandler()));

    return exit_code;
  }

 private:
  std::unique_ptr<Server> webserver_;

  DISALLOW_COPY_AND_ASSIGN(WebservTestClient);
};  // class WebservTestClient

}  // namespace

int main(int /* argc */, char* /* argv */[]) {
  brillo::InitLog(brillo::kLogToSyslog | brillo::kLogHeader);
  WebservTestClient client;
  return client.Run();
}
