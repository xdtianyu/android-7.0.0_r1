//
// Copyright (C) 2015 The Android Open Source Project
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

#include "tpm_manager/server/dbus_service.h"

#include <memory>
#include <string>

#include <brillo/bind_lambda.h>
#include <dbus/bus.h>
#include <dbus/object_path.h>

#include "tpm_manager/common/tpm_manager_constants.h"
#include "tpm_manager/common/tpm_nvram_dbus_interface.h"
#include "tpm_manager/common/tpm_ownership_dbus_interface.h"

namespace tpm_manager {

DBusService::DBusService(const scoped_refptr<dbus::Bus>& bus,
                         TpmNvramInterface* nvram_service,
                         TpmOwnershipInterface* ownership_service)
    : dbus_object_(nullptr, bus, dbus::ObjectPath(kTpmManagerServicePath)),
      nvram_service_(nvram_service),
      ownership_service_(ownership_service) {}

void DBusService::Register(const CompletionAction& callback) {
  brillo::dbus_utils::DBusInterface* ownership_dbus_interface =
      dbus_object_.AddOrGetInterface(kTpmOwnershipInterface);

  ownership_dbus_interface->AddMethodHandler(
      kGetTpmStatus,
      base::Unretained(this),
      &DBusService::HandleOwnershipDBusMethod<
          GetTpmStatusRequest,
          GetTpmStatusReply,
          &TpmOwnershipInterface::GetTpmStatus>);

  ownership_dbus_interface->AddMethodHandler(
      kTakeOwnership,
      base::Unretained(this),
      &DBusService::HandleOwnershipDBusMethod<
          TakeOwnershipRequest,
          TakeOwnershipReply,
          &TpmOwnershipInterface::TakeOwnership>);

  ownership_dbus_interface->AddMethodHandler(
      kRemoveOwnerDependency,
      base::Unretained(this),
      &DBusService::HandleOwnershipDBusMethod<
          RemoveOwnerDependencyRequest,
          RemoveOwnerDependencyReply,
          &TpmOwnershipInterface::RemoveOwnerDependency>);

  brillo::dbus_utils::DBusInterface* nvram_dbus_interface =
      dbus_object_.AddOrGetInterface(kTpmNvramInterface);

  nvram_dbus_interface->AddMethodHandler(
      kDefineNvram,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          DefineNvramRequest,
          DefineNvramReply,
          &TpmNvramInterface::DefineNvram>);

  nvram_dbus_interface->AddMethodHandler(
      kDestroyNvram,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          DestroyNvramRequest,
          DestroyNvramReply,
          &TpmNvramInterface::DestroyNvram>);

  nvram_dbus_interface->AddMethodHandler(
      kWriteNvram,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          WriteNvramRequest,
          WriteNvramReply,
          &TpmNvramInterface::WriteNvram>);

  nvram_dbus_interface->AddMethodHandler(
      kReadNvram,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          ReadNvramRequest,
          ReadNvramReply,
          &TpmNvramInterface::ReadNvram>);

  nvram_dbus_interface->AddMethodHandler(
      kIsNvramDefined,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          IsNvramDefinedRequest,
          IsNvramDefinedReply,
          &TpmNvramInterface::IsNvramDefined>);

  nvram_dbus_interface->AddMethodHandler(
      kIsNvramLocked,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          IsNvramLockedRequest,
          IsNvramLockedReply,
          &TpmNvramInterface::IsNvramLocked>);

  nvram_dbus_interface->AddMethodHandler(
      kGetNvramSize,
      base::Unretained(this),
      &DBusService::HandleNvramDBusMethod<
          GetNvramSizeRequest,
          GetNvramSizeReply,
          &TpmNvramInterface::GetNvramSize>);

  dbus_object_.RegisterAsync(callback);
}

template<typename RequestProtobufType,
         typename ReplyProtobufType,
         DBusService::HandlerFunction<RequestProtobufType,
                                      ReplyProtobufType,
                                      TpmNvramInterface> func>
void DBusService::HandleNvramDBusMethod(
    std::unique_ptr<DBusMethodResponse<const ReplyProtobufType&>> response,
    const RequestProtobufType& request) {
  // Convert |response| to a shared_ptr so |nvram_service_| can safely copy the
  // callback.
  using SharedResponsePointer = std::shared_ptr<
      DBusMethodResponse<const ReplyProtobufType&>>;
  // A callback that sends off the reply protobuf.
  auto callback = [](const SharedResponsePointer& response,
                     const ReplyProtobufType& reply) {
    response->Return(reply);
  };
  (nvram_service_->*func)(
      request,
      base::Bind(callback, SharedResponsePointer(std::move(response))));
}

template<typename RequestProtobufType,
         typename ReplyProtobufType,
         DBusService::HandlerFunction<RequestProtobufType,
                                      ReplyProtobufType,
                                      TpmOwnershipInterface> func>
void DBusService::HandleOwnershipDBusMethod(
    std::unique_ptr<DBusMethodResponse<const ReplyProtobufType&>> response,
    const RequestProtobufType& request) {
  // Convert |response| to a shared_ptr so |ownership_service_| can safely
  // copy the callback.
  using SharedResponsePointer = std::shared_ptr<
      DBusMethodResponse<const ReplyProtobufType&>>;
  // A callback that sends off the reply protobuf.
  auto callback = [](const SharedResponsePointer& response,
                     const ReplyProtobufType& reply) {
    response->Return(reply);
  };
  (ownership_service_->*func)(
      request,
      base::Bind(callback, SharedResponsePointer(std::move(response))));
}

}  // namespace tpm_manager
