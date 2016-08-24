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

#ifndef WEBSERVER_LIBWEBSERV_DBUS_PROTOCOL_HANDLER_H_
#define WEBSERVER_LIBWEBSERV_DBUS_PROTOCOL_HANDLER_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <brillo/errors/error.h>
#include <brillo/secure_blob.h>
#include <brillo/streams/stream.h>
#include <dbus/object_path.h>

#include <libwebserv/export.h>
#include <libwebserv/protocol_handler.h>
#include <libwebserv/request_handler_interface.h>

namespace org {
namespace chromium {
namespace WebServer {

class ProtocolHandlerProxyInterface;

}  // namespace WebServer
}  // namespace chromium
}  // namespace org

namespace libwebserv {

class DBusServer;
class Request;

class LIBWEBSERV_PRIVATE DBusProtocolHandler : public ProtocolHandler {
 public:
  DBusProtocolHandler(const std::string& name, DBusServer* server);
  ~DBusProtocolHandler() override;

  bool IsConnected() const override;

  std::string GetName() const override;

  std::set<uint16_t> GetPorts() const override;

  std::set<std::string> GetProtocols() const override;

  brillo::Blob GetCertificateFingerprint() const override;

  int AddHandler(const std::string& url,
                 const std::string& method,
                 std::unique_ptr<RequestHandlerInterface> handler) override;

  int AddHandlerCallback(
      const std::string& url,
      const std::string& method,
      const base::Callback<RequestHandlerInterface::HandlerSignature>&
          handler_callback) override;

  bool RemoveHandler(int handler_id) override;

 private:
  friend class FileInfo;
  friend class DBusServer;
  friend class ResponseImpl;

  using ProtocolHandlerProxyInterface =
      org::chromium::WebServer::ProtocolHandlerProxyInterface;

  struct HandlerMapEntry {
    std::string url;
    std::string method;
    std::map<ProtocolHandlerProxyInterface*, std::string> remote_handler_ids;
    std::unique_ptr<RequestHandlerInterface> handler;
  };

  // Called by the DBusServer class when the D-Bus proxy object gets connected
  // to the web server daemon.
  void Connect(ProtocolHandlerProxyInterface* proxy);

  // Called by the DBusServer class when the D-Bus proxy object gets
  // disconnected from the web server daemon.
  void Disconnect(const dbus::ObjectPath& object_path);

  // Asynchronous callbacks to handle successful or failed request handler
  // registration over D-Bus.
  void AddHandlerSuccess(
      int handler_id,
      ProtocolHandlerProxyInterface* proxy,
      const std::string& remote_handler_id);
  void AddHandlerError(int handler_id, brillo::Error* error);

  // Called by DBusServer when an incoming request is dispatched.
  bool ProcessRequest(const std::string& protocol_handler_id,
                      const std::string& remote_handler_id,
                      const std::string& request_id,
                      std::unique_ptr<Request> request,
                      brillo::ErrorPtr* error);

  // Called by Response object to finish the request and send response data.
  void CompleteRequest(
      const std::string& request_id,
      int status_code,
      const std::multimap<std::string, std::string>& headers,
      brillo::StreamPtr data_stream);

  // Makes a call to the (remote) web server request handler over D-Bus to
  // obtain the file content of uploaded file (identified by |file_id|) during
  // request with |request_id|.
  void GetFileData(
      const std::string& request_id,
      int file_id,
      const base::Callback<void(brillo::StreamPtr)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback);

  // A helper method to obtain a corresponding protocol handler D-Bus proxy for
  // outstanding request with ID |request_id|.
  ProtocolHandlerProxyInterface* GetRequestProtocolHandlerProxy(
      const std::string& request_id) const;

  // Protocol Handler name.
  std::string name_;
  // Back reference to the server object.
  DBusServer* server_{nullptr};
  // Handler data map. The key is the client-facing request handler ID returned
  // by AddHandler() when registering the handler.
  std::map<int, HandlerMapEntry> request_handlers_;
  // The counter to generate new handler IDs.
  int last_handler_id_{0};
  // Map of remote handler IDs (GUID strings) to client-facing request handler
  // IDs (int) which are returned by AddHandler() and used as a key in
  // |request_handlers_|.
  std::map<std::string, int> remote_handler_id_map_;
  // Remote D-Bus proxies for the server protocol handler objects.
  // There could be multiple protocol handlers with the same name (to make
  // it possible to server the same requests on different ports, for example).
  std::map<dbus::ObjectPath, ProtocolHandlerProxyInterface*> proxies_;
  // A map of request ID to protocol handler ID. Used to locate the appropriate
  // protocol handler D-Bus proxy for given request.
  std::map<std::string, std::string> request_id_map_;

  base::WeakPtrFactory<DBusProtocolHandler> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(DBusProtocolHandler);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_DBUS_PROTOCOL_HANDLER_H_
