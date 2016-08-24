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

#ifndef WEBSERVER_LIBWEBSERV_SERVER_H_
#define WEBSERVER_LIBWEBSERV_SERVER_H_

// In our own Android.mk, we set these flags for ourselves.  However, for
// libraries consuming libwebserv, they don't have any of that logic.  Leave us
// with DBus bindings until the Binder interface is ready.
#if !defined(WEBSERV_USE_DBUS) && !defined(WEBSERV_USE_BINDER)
#define WEBSERV_USE_DBUS
#endif

#include <memory>
#include <string>

#include <base/callback.h>
#include <base/macros.h>
#include <libwebserv/export.h>

#if defined(WEBSERV_USE_DBUS)
#include <base/memory/ref_counted.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <dbus/bus.h>
#endif  // defined(WEBSERV_USE_DBUS)

#if defined(WEBSERV_USE_BINDER)
#include <brillo/message_loops/message_loop.h>
#endif  // defined(WEBSERV_USE_BINDER)

namespace libwebserv {

class ProtocolHandler;

// Top-level wrapper class around HTTP server and provides an interface to
// the web server.
class LIBWEBSERV_EXPORT Server {
 public:
  Server() = default;
  virtual ~Server() = default;

#if defined(WEBSERV_USE_DBUS)
  // Establish a connection to the system webserver.
  //
  // |service_name| is the well known D-Bus name of the client's process, used
  // to expose a callback D-Bus object the web server calls back with incoming
  // requests.
  // |on_server_online| and |on_server_offline| will notify the caller when the
  // server comes up and down.
  //
  // Note that you can use the returned Server instance as if the webserver
  // process is actually running (ignoring webserver crashes and restarts).
  // All registered request handlers will simply be re-registered when the
  // webserver appears again.
  static std::unique_ptr<Server> ConnectToServerViaDBus(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name,
      const brillo::dbus_utils::AsyncEventSequencer::CompletionAction& cb,
      const base::Closure& on_server_online,
      const base::Closure& on_server_offline);
#endif  // defined(WEBSERV_USE_DBUS)

#if defined(WEBSERV_USE_BINDER)
  // Establish a connection to the system webserver.
  //
  // |on_server_online| and |on_server_offline| will notify the caller when the
  // server comes up and down.
  //
  // Note that you can use the returned Server instance as if the webserver
  // process is actually running (ignoring webserver crashes and restarts).
  // All registered request handlers will simply be re-registered when the
  // webserver appears again.
  static std::unique_ptr<Server> ConnectToServerViaBinder(
      brillo::MessageLoop* message_loop,
      const base::Closure& on_server_online,
      const base::Closure& on_server_offline);
#endif  // defined(WEBSERV_USE_BINDER)

  // A helper method that returns the default handler for "http".
  virtual ProtocolHandler* GetDefaultHttpHandler() = 0;

  // A helper method that returns the default handler for "https".
  virtual ProtocolHandler* GetDefaultHttpsHandler() = 0;

  // Returns an existing protocol handler by name.  If the handler with the
  // requested |name| does not exist, a new one will be created.
  //
  // The created handler is purely client side, and depends on the server
  // being configured to open a corresponding handler with the given name.
  // Because clients and the server come up asynchronously, we allow clients
  // to register anticipated handlers before server starts up.
  virtual ProtocolHandler* GetProtocolHandler(const std::string& name) = 0;

  // Returns true if |this| is connected to the web server daemon via IPC.
  virtual bool IsConnected() const = 0;

  // Set a user-callback to be invoked when a protocol handler is connect to the
  // server daemon.  Multiple calls to this method will overwrite previously set
  // callbacks.
  virtual void OnProtocolHandlerConnected(
      const base::Callback<void(ProtocolHandler*)>& callback) = 0;

  // Set a user-callback to be invoked when a protocol handler is disconnected
  // from the server daemon (e.g. on shutdown).  Multiple calls to this method
  // will overwrite previously set callbacks.
  virtual void OnProtocolHandlerDisconnected(
      const base::Callback<void(ProtocolHandler*)>& callback) = 0;

  // Returns the default request timeout used to process incoming requests.
  // The reply to an incoming request should be sent within this timeout or
  // else the web server will automatically abort the connection. If the timeout
  // is not set, the returned value will be base::TimeDelta::Max().
  virtual base::TimeDelta GetDefaultRequestTimeout() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Server);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_SERVER_H_
