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

#include "webservd/dbus_protocol_handler.h"

#include <base/bind.h>
#include <brillo/dbus/async_event_sequencer.h>
#include <brillo/dbus/exported_object_manager.h>

#include "webservd/dbus_request_handler.h"
#include "webservd/protocol_handler.h"
#include "webservd/request.h"
#include "webservd/server.h"

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::DBusObject;
using brillo::dbus_utils::ExportedObjectManager;

namespace webservd {

DBusProtocolHandler::DBusProtocolHandler(
    ExportedObjectManager* object_manager,
    const dbus::ObjectPath& object_path,
    ProtocolHandler* protocol_handler,
    Server* server)
    : dbus_object_{new DBusObject{object_manager, object_manager->GetBus(),
                                  object_path}},
      protocol_handler_{protocol_handler},
      server_{server} {
  dbus_adaptor_.SetId(protocol_handler->GetID());
  dbus_adaptor_.SetName(protocol_handler->GetName());
  dbus_adaptor_.SetPort(protocol_handler->GetPort());
  dbus_adaptor_.SetProtocol(protocol_handler->GetProtocol());
  dbus_adaptor_.SetCertificateFingerprint(
      protocol_handler->GetCertificateFingerprint());
}

DBusProtocolHandler::~DBusProtocolHandler() {
  for (const auto& pair : dbus_service_data_) {
    server_->GetBus()->UnlistenForServiceOwnerChange(
        pair.first, pair.second.on_client_disconnected_callback);
  }
}

void DBusProtocolHandler::RegisterAsync(
    const AsyncEventSequencer::CompletionAction& completion_callback) {
  scoped_refptr<AsyncEventSequencer> sequencer(new AsyncEventSequencer());
  dbus_adaptor_.RegisterWithDBusObject(dbus_object_.get());
  dbus_object_->RegisterAsync(
      sequencer->GetHandler("Failed exporting ProtocolHandler.", true));
  sequencer->OnAllTasksCompletedCall({completion_callback});
}

ExportedObjectManager* DBusProtocolHandler::GetObjectManager() const {
  return dbus_object_->GetObjectManager().get();
}

bool DBusProtocolHandler::AddRequestHandler(
    brillo::ErrorPtr* /* error */,
    dbus::Message* message,
    const std::string& in_url,
    const std::string& in_method,
    const std::string& in_service_name,
    std::string* out_request_handler_id) {
  auto p = dbus_service_data_.find(in_service_name);
  if (p == dbus_service_data_.end()) {
    DBusServiceData dbus_service_data;
    dbus_service_data.owner = message->GetSender();
    dbus_service_data.handler_proxy.reset(
        new RequestHandlerProxy{server_->GetBus(), in_service_name});
    dbus_service_data.on_client_disconnected_callback =
        base::Bind(&DBusProtocolHandler::OnClientDisconnected,
                   weak_ptr_factory_.GetWeakPtr(),
                   in_service_name);
    server_->GetBus()->ListenForServiceOwnerChange(
        in_service_name, dbus_service_data.on_client_disconnected_callback);
    p = dbus_service_data_.emplace(in_service_name,
                                   std::move(dbus_service_data)).first;
  }
  std::unique_ptr<RequestHandlerInterface> handler{
    new DBusRequestHandler{server_, p->second.handler_proxy.get()}
  };
  std::string handler_id = protocol_handler_->AddRequestHandler(
      in_url, in_method, std::move(handler));
  p->second.handler_ids.insert(handler_id);
  handler_to_service_name_map_.emplace(handler_id, in_service_name);

  *out_request_handler_id = handler_id;
  return true;
}

bool DBusProtocolHandler::RemoveRequestHandler(
    brillo::ErrorPtr* error,
    const std::string& in_handler_id) {

  auto p = handler_to_service_name_map_.find(in_handler_id);
  if (p == handler_to_service_name_map_.end()) {
    brillo::Error::AddToPrintf(error,
                               FROM_HERE,
                               brillo::errors::dbus::kDomain,
                               DBUS_ERROR_FAILED,
                               "Handler with ID %s does not exist",
                               in_handler_id.c_str());
    return false;
  }
  std::string service_name = p->second;
  CHECK(protocol_handler_->RemoveRequestHandler(in_handler_id));
  DBusServiceData& dbus_service_data = dbus_service_data_[service_name];
  CHECK_EQ(1u, dbus_service_data.handler_ids.erase(in_handler_id));
  if (dbus_service_data.handler_ids.empty()) {
    server_->GetBus()->UnlistenForServiceOwnerChange(
        service_name, dbus_service_data.on_client_disconnected_callback);
    dbus_service_data_.erase(service_name);
  }
  return true;
}

void DBusProtocolHandler::OnClientDisconnected(
    const std::string& service_name,
    const std::string& service_owner) {
  // This method will be called when the client's D-Bus service owner has
  // changed which could be either the client exiting (|service_owner| is empty)
  // or is being replaced with another running instance.
  // In either case, we need to remove the old client's handlers since the
  // new client will register their own on start up anyway.
  // However pay attention to the case where the service owner is the same as
  // the sender, in which case we should not remove the handlers. This happens
  // if the handling process claims D-Bus service after it registers request
  // handlers with the web server.
  auto p = dbus_service_data_.find(service_name);
  if (p == dbus_service_data_.end() || p->second.owner == service_owner)
    return;

  for (const std::string& handler_id : p->second.handler_ids) {
    handler_to_service_name_map_.erase(handler_id);
    protocol_handler_->RemoveRequestHandler(handler_id);
  }
  server_->GetBus()->UnlistenForServiceOwnerChange(
      service_name, p->second.on_client_disconnected_callback);
  dbus_service_data_.erase(p);
}

bool DBusProtocolHandler::GetRequestFileData(
    brillo::ErrorPtr* error,
    const std::string& in_request_id,
    int32_t in_file_id,
    dbus::FileDescriptor* out_contents) {
  auto request = GetRequest(in_request_id, error);
  if (!request)
    return false;

  base::File file = request->GetFileData(in_file_id);
  if (file.IsValid()) {
    out_contents->PutValue(file.TakePlatformFile());
    out_contents->CheckValidity();
    return true;
  }

  brillo::Error::AddToPrintf(error,
                             FROM_HERE,
                             brillo::errors::dbus::kDomain,
                             DBUS_ERROR_FAILED,
                             "File with ID %d does not exist",
                             in_file_id);
  return false;
}

bool DBusProtocolHandler::CompleteRequest(
    brillo::ErrorPtr* error,
    const std::string& in_request_id,
    int32_t in_status_code,
    const std::vector<std::tuple<std::string, std::string>>& in_headers,
    int64_t in_data_size,
    dbus::FileDescriptor* out_response_stream) {
  auto request = GetRequest(in_request_id, error);
  if (!request)
    return false;

  base::File file = request->Complete(in_status_code, in_headers, in_data_size);
  if (file.IsValid()) {
    out_response_stream->PutValue(file.TakePlatformFile());
    out_response_stream->CheckValidity();
    return true;
  }
  brillo::Error::AddTo(error, FROM_HERE, brillo::errors::dbus::kDomain,
                       DBUS_ERROR_FAILED, "Response already received");
  return false;
}

Request* DBusProtocolHandler::GetRequest(const std::string& request_id,
                                         brillo::ErrorPtr* error) {
  Request* request = protocol_handler_->GetRequest(request_id);
  if (!request) {
    brillo::Error::AddToPrintf(error,
                               FROM_HERE,
                               brillo::errors::dbus::kDomain,
                               DBUS_ERROR_FAILED,
                               "Unknown request ID: %s",
                               request_id.c_str());
  }
  return request;
}

}  // namespace webservd
