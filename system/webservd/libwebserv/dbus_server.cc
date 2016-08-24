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

#include <libwebserv/dbus_server.h>

#include <tuple>
#include <vector>

#include <libwebserv/dbus_protocol_handler.h>
#include <libwebserv/request_impl.h>

#include "dbus_bindings/org.chromium.WebServer.RequestHandler.h"
#include "webservd/dbus-proxies.h"

namespace libwebserv {

class DBusServer::RequestHandler final
    : public org::chromium::WebServer::RequestHandlerInterface {
 public:
  explicit RequestHandler(DBusServer* server) : server_{server} {}
  bool ProcessRequest(
      brillo::ErrorPtr* error,
      const std::tuple<std::string, std::string, std::string, std::string,
                       std::string>& in_request_info,
      const std::vector<std::tuple<std::string, std::string>>& in_headers,
      const std::vector<std::tuple<bool, std::string, std::string>>& in_params,
      const std::vector<std::tuple<int32_t, std::string, std::string,
                                   std::string, std::string>>& in_files,
      const dbus::FileDescriptor& in_body) override;

 private:
  DBusServer* server_{nullptr};
  DISALLOW_COPY_AND_ASSIGN(RequestHandler);
};

bool DBusServer::RequestHandler::ProcessRequest(
    brillo::ErrorPtr* error,
    const std::tuple<std::string, std::string, std::string, std::string,
                     std::string>& in_request_info,
    const std::vector<std::tuple<std::string, std::string>>& in_headers,
    const std::vector<std::tuple<bool, std::string, std::string>>& in_params,
    const std::vector<std::tuple<int32_t, std::string, std::string, std::string,
                                 std::string>>& in_files,
    const dbus::FileDescriptor& in_body) {
  std::string protocol_handler_id = std::get<0>(in_request_info);
  std::string request_handler_id = std::get<1>(in_request_info);
  std::string request_id = std::get<2>(in_request_info);
  std::string url = std::get<3>(in_request_info);
  std::string method = std::get<4>(in_request_info);
  DBusProtocolHandler* protocol_handler =
      server_->GetProtocolHandlerByID(protocol_handler_id);
  if (!protocol_handler) {
    brillo::Error::AddToPrintf(error, FROM_HERE,
                               brillo::errors::dbus::kDomain,
                               DBUS_ERROR_FAILED,
                               "Unknown protocol handler '%s'",
                               protocol_handler_id.c_str());
    return false;
  }
  std::unique_ptr<RequestImpl> request{
    new RequestImpl{protocol_handler, url, method}};
  // Convert request data into format required by the Request object.
  for (const auto& tuple : in_params) {
    if (std::get<0>(tuple))
      request->post_data_.emplace(std::get<1>(tuple), std::get<2>(tuple));
    else
      request->get_data_.emplace(std::get<1>(tuple), std::get<2>(tuple));
  }

  for (const auto& tuple : in_headers)
    request->headers_.emplace(std::get<0>(tuple), std::get<1>(tuple));

  for (const auto& tuple : in_files) {
    request->file_info_.emplace(
        std::get<1>(tuple),  // field_name
        std::unique_ptr<FileInfo>{new FileInfo{
            protocol_handler,
            std::get<0>(tuple),     // file_id
            request_id,
            std::get<2>(tuple),     // file_name
            std::get<3>(tuple),     // content_type
            std::get<4>(tuple)}});  // transfer_encoding
  }

  request->raw_data_fd_ = base::File(dup(in_body.value()));
  CHECK(request->raw_data_fd_.IsValid());

  return protocol_handler->ProcessRequest(protocol_handler_id,
                                          request_handler_id,
                                          request_id,
                                          std::move(request),
                                          error);
}

DBusServer::DBusServer()
    : request_handler_{new RequestHandler{this}},
      dbus_adaptor_{new org::chromium::WebServer::RequestHandlerAdaptor{
          request_handler_.get()}} {}

void DBusServer::Connect(
    const scoped_refptr<dbus::Bus>& bus,
    const std::string& service_name,
    const brillo::dbus_utils::AsyncEventSequencer::CompletionAction& cb,
    const base::Closure& on_server_online,
    const base::Closure& on_server_offline) {
  service_name_ = service_name;
  dbus_object_.reset(new brillo::dbus_utils::DBusObject{
      nullptr, bus, dbus_adaptor_->GetObjectPath()});
  dbus_adaptor_->RegisterWithDBusObject(dbus_object_.get());
  dbus_object_->RegisterAsync(cb);
  on_server_online_ = on_server_online;
  on_server_offline_ = on_server_offline;
  object_manager_.reset(new org::chromium::WebServer::ObjectManagerProxy{bus});
  object_manager_->SetServerAddedCallback(
      base::Bind(&DBusServer::Online, base::Unretained(this)));
  object_manager_->SetServerRemovedCallback(
      base::Bind(&DBusServer::Offline, base::Unretained(this)));
  object_manager_->SetProtocolHandlerAddedCallback(
      base::Bind(&DBusServer::ProtocolHandlerAdded, base::Unretained(this)));
  object_manager_->SetProtocolHandlerRemovedCallback(
      base::Bind(&DBusServer::ProtocolHandlerRemoved, base::Unretained(this)));
}

void DBusServer::Online(
    org::chromium::WebServer::ServerProxyInterface* server) {
  VLOG(1) << "Web server is on-line.";
  proxy_ = server;
  if (!on_server_online_.is_null())
    on_server_online_.Run();
}

bool DBusServer::IsConnected() const {
  return proxy_ != nullptr;
}

void DBusServer::Offline(const dbus::ObjectPath& /* object_path */) {
  if (!on_server_offline_.is_null())
    on_server_offline_.Run();
  proxy_ = nullptr;
  VLOG(1) << "Web server is off-line.";
}

void DBusServer::ProtocolHandlerAdded(
    org::chromium::WebServer::ProtocolHandlerProxyInterface* handler) {
  VLOG(1) << "Server-side protocol handler with ID '" << handler->id()
          << "' is on-line (" << handler->name() << ")";

  protocol_handler_id_map_.emplace(handler->GetObjectPath(), handler->id());
  DBusProtocolHandler* registered_handler =
      GetProtocolHandlerImpl(handler->name());
  if (registered_handler) {
    protocol_handlers_ids_.emplace(handler->id(), registered_handler);
    registered_handler->Connect(handler);
    if (!on_protocol_handler_connected_.is_null())
      on_protocol_handler_connected_.Run(registered_handler);
  }
}

void DBusServer::ProtocolHandlerRemoved(const dbus::ObjectPath& object_path) {
  auto p = protocol_handler_id_map_.find(object_path);
  if (p == protocol_handler_id_map_.end())
    return;

  VLOG(1) << "Server-side protocol handler with ID '" << p->second
          << "' is off-line.";

  DBusProtocolHandler* registered_handler = GetProtocolHandlerByID(p->second);
  if (registered_handler) {
    if (!on_protocol_handler_disconnected_.is_null())
      on_protocol_handler_disconnected_.Run(registered_handler);
    registered_handler->Disconnect(object_path);
    protocol_handlers_ids_.erase(p->second);
  }

  protocol_handler_id_map_.erase(p);
}

ProtocolHandler* DBusServer::GetProtocolHandler(const std::string& name) {
  return GetProtocolHandlerImpl(name);
}

DBusProtocolHandler* DBusServer::GetProtocolHandlerImpl(
    const std::string& name) {
  auto p = protocol_handlers_names_.find(name);
  if (p == protocol_handlers_names_.end()) {
    VLOG(1) << "Creating a client-side instance of web server's protocol "
            << "handler with name '" << name << "'";
    p = protocol_handlers_names_.emplace(
        name,
        std::unique_ptr<DBusProtocolHandler>{
            new DBusProtocolHandler{name, this}}).first;
  }
  return p->second.get();
}

ProtocolHandler* DBusServer::GetDefaultHttpHandler() {
  return GetProtocolHandler(ProtocolHandler::kHttp);
}

ProtocolHandler* DBusServer::GetDefaultHttpsHandler() {
  return GetProtocolHandler(ProtocolHandler::kHttps);
}

DBusProtocolHandler* DBusServer::GetProtocolHandlerByID(
    const std::string& id) const {
  auto p = protocol_handlers_ids_.find(id);
  if (p == protocol_handlers_ids_.end()) {
    LOG(ERROR) << "Unable to locate protocol handler with ID '" << id << "'";
    return nullptr;
  }
  return p->second;
}

void DBusServer::OnProtocolHandlerConnected(
    const base::Callback<void(ProtocolHandler*)>& callback) {
  on_protocol_handler_connected_ = callback;
}

void DBusServer::OnProtocolHandlerDisconnected(
    const base::Callback<void(ProtocolHandler*)>& callback) {
  on_protocol_handler_disconnected_ = callback;
}

base::TimeDelta DBusServer::GetDefaultRequestTimeout() const {
  int timeout_seconds = proxy_ ? proxy_->default_request_timeout() : -1;
  if (timeout_seconds <= 0)
    return base::TimeDelta::Max();
  return base::TimeDelta::FromSeconds(timeout_seconds);
}

}  // namespace libwebserv
