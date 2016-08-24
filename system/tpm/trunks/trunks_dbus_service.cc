//
// Copyright (C) 2014 The Android Open Source Project
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
//

#include "trunks/trunks_dbus_service.h"

#include <base/bind.h>
#include <brillo/bind_lambda.h>

#include "trunks/dbus_interface.h"
#include "trunks/error_codes.h"
#include "trunks/interface.pb.h"

namespace trunks {

using brillo::dbus_utils::AsyncEventSequencer;
using brillo::dbus_utils::DBusMethodResponse;

TrunksDBusService::TrunksDBusService()
    : brillo::DBusServiceDaemon(trunks::kTrunksServiceName) {}

void TrunksDBusService::RegisterDBusObjectsAsync(
    AsyncEventSequencer* sequencer) {
  trunks_dbus_object_.reset(new brillo::dbus_utils::DBusObject(
      nullptr, bus_, dbus::ObjectPath(kTrunksServicePath)));
  brillo::dbus_utils::DBusInterface* dbus_interface =
      trunks_dbus_object_->AddOrGetInterface(kTrunksInterface);
  dbus_interface->AddMethodHandler(kSendCommand, base::Unretained(this),
                                   &TrunksDBusService::HandleSendCommand);
  trunks_dbus_object_->RegisterAsync(
      sequencer->GetHandler("Failed to register D-Bus object.", true));
}

void TrunksDBusService::HandleSendCommand(
    std::unique_ptr<DBusMethodResponse<const SendCommandResponse&>>
        response_sender,
    const SendCommandRequest& request) {
  // Convert |response_sender| to a shared_ptr so |transceiver_| can safely
  // copy the callback.
  using SharedResponsePointer =
      std::shared_ptr<DBusMethodResponse<const SendCommandResponse&>>;
  // A callback that constructs the response protobuf and sends it.
  auto callback = [](const SharedResponsePointer& response,
                     const std::string& response_from_tpm) {
    SendCommandResponse tpm_response_proto;
    tpm_response_proto.set_response(response_from_tpm);
    response->Return(tpm_response_proto);
  };
  if (!request.has_command() || request.command().empty()) {
    LOG(ERROR) << "TrunksDBusService: Invalid request.";
    callback(SharedResponsePointer(std::move(response_sender)),
             CreateErrorResponse(SAPI_RC_BAD_PARAMETER));
    return;
  }
  transceiver_->SendCommand(
      request.command(),
      base::Bind(callback, SharedResponsePointer(std::move(response_sender))));
}

}  // namespace trunks
