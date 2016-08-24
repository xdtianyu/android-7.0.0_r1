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

#ifndef WEBSERVER_LIBWEBSERV_DBUS_SERVER_H_
#define WEBSERVER_LIBWEBSERV_DBUS_SERVER_H_

#include <map>
#include <memory>
#include <string>

#include <base/macros.h>
#include <dbus/bus.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/dbus_object.h>

#include "libwebserv/export.h"
#include "libwebserv/request_handler_interface.h"
#include "libwebserv/server.h"

namespace org {
namespace chromium {
namespace WebServer {

class ObjectManagerProxy;
class ProtocolHandlerProxyInterface;
class RequestHandlerAdaptor;
class ServerProxyInterface;

}  // namespace WebServer
}  // namespace chromium
}  // namespace org

namespace libwebserv {

class DBusProtocolHandler;

class LIBWEBSERV_PRIVATE DBusServer : public Server {
 public:
  DBusServer();
  ~DBusServer() override = default;

  // Establish a connection to the system webserver.
  // |service_name| is the well known D-Bus name of the client's process, used
  // to expose a callback D-Bus object the web server calls back with incoming
  // requests.
  // |on_server_online| and |on_server_offline| will notify the caller when the
  // server comes up and down.
  // Note that we can Connect() even before the webserver attaches to D-Bus,
  // and appropriate state will be built up when the webserver appears on D-Bus.
  void Connect(
      const scoped_refptr<dbus::Bus>& bus,
      const std::string& service_name,
      const brillo::dbus_utils::AsyncEventSequencer::CompletionAction& cb,
      const base::Closure& on_server_online,
      const base::Closure& on_server_offline);

  ProtocolHandler* GetDefaultHttpHandler() override;

  ProtocolHandler* GetDefaultHttpsHandler() override;
  ProtocolHandler* GetProtocolHandler(const std::string& name) override;

  bool IsConnected() const override;

  void OnProtocolHandlerConnected(
      const base::Callback<void(ProtocolHandler*)>& callback) override;

  void OnProtocolHandlerDisconnected(
      const base::Callback<void(ProtocolHandler*)>& callback) override;

  base::TimeDelta GetDefaultRequestTimeout() const override;

 private:
  friend class DBusProtocolHandler;
  class RequestHandler;

  // Handler invoked when a connection is established to web server daemon.
  void Online(org::chromium::WebServer::ServerProxyInterface* server);

  // Handler invoked when the web server daemon connection is dropped.
  void Offline(const dbus::ObjectPath& object_path);

  // Handler invoked when a new protocol handler D-Bus proxy object becomes
  // available.
  void ProtocolHandlerAdded(
      org::chromium::WebServer::ProtocolHandlerProxyInterface* handler);

  // Handler invoked when a protocol handler D-Bus proxy object disappears.
  void ProtocolHandlerRemoved(
      const dbus::ObjectPath& object_path);

  // Looks up a protocol handler by ID. If not found, returns nullptr.
  DBusProtocolHandler* GetProtocolHandlerByID(
      const std::string& id) const;

  // Like the public version, but returns our specific handler type.
  DBusProtocolHandler* GetProtocolHandlerImpl(const std::string& name);

  // Private implementation of D-Bus RequestHandlerInterface called by the web
  // server daemon whenever a new request is available to be processed.
  std::unique_ptr<RequestHandler> request_handler_;

  // D-Bus object to handler registration of RequestHandlerInterface.
  std::unique_ptr<brillo::dbus_utils::DBusObject> dbus_object_;

  // D-Bus object adaptor for RequestHandlerInterface.
  std::unique_ptr<org::chromium::WebServer::RequestHandlerAdaptor>
      dbus_adaptor_;

  // D-Bus object manager proxy that receives notification of web server
  // daemon's D-Bus object creation and destruction.
  std::unique_ptr<org::chromium::WebServer::ObjectManagerProxy> object_manager_;

  // A mapping of protocol handler name to the associated object.
  std::map<std::string, std::unique_ptr<DBusProtocolHandler>>
      protocol_handlers_names_;
  // A mapping of protocol handler IDs to the associated object.
  std::map<std::string, DBusProtocolHandler*> protocol_handlers_ids_;
  // A map between D-Bus object path of protocol handler and remote protocol
  // handler ID.
  std::map<dbus::ObjectPath, std::string> protocol_handler_id_map_;

  // User-specified callbacks for server and protocol handler life-time events.
  base::Closure on_server_online_;
  base::Closure on_server_offline_;
  base::Callback<void(ProtocolHandler*)> on_protocol_handler_connected_;
  base::Callback<void(ProtocolHandler*)> on_protocol_handler_disconnected_;

  // D-Bus proxy for the web server main object.
  org::chromium::WebServer::ServerProxyInterface* proxy_{nullptr};

  // D-Bus service name used by the daemon hosting this object.
  std::string service_name_;

  DISALLOW_COPY_AND_ASSIGN(DBusServer);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_DBUS_SERVER_H_
