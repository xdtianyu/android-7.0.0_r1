//
// Copyright (C) 2016 The Android Open Source Project
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

#include "trunks/trunks_binder_service.h"

#include <sysexits.h>

#include <base/bind.h>
#include <binderwrapper/binder_wrapper.h>

#include "trunks/binder_interface.h"
#include "trunks/command_transceiver.h"
#include "trunks/error_codes.h"
#include "trunks/interface.pb.h"

namespace {

// If |command| is a valid command protobuf, provides the |command_data| and
// returns true. Otherwise, returns false.
bool ParseCommandProto(const std::vector<uint8_t>& command,
                       std::string* command_data) {
  trunks::SendCommandRequest request_proto;
  if (!request_proto.ParseFromArray(command.data(), command.size()) ||
      !request_proto.has_command() || request_proto.command().empty()) {
    return false;
  }
  *command_data = request_proto.command();
  return true;
}

void CreateResponseProto(const std::string& data,
                         std::vector<uint8_t>* response) {
  trunks::SendCommandResponse response_proto;
  response_proto.set_response(data);
  response->resize(response_proto.ByteSize());
  CHECK(response_proto.SerializeToArray(response->data(), response->size()))
      << "TrunksBinderService: Failed to serialize protobuf.";
}

}  // namespace

namespace trunks {

int TrunksBinderService::OnInit() {
  android::BinderWrapper::Create();
  if (!watcher_.Init()) {
    LOG(ERROR) << "TrunksBinderService: BinderWatcher::Init failed.";
    return EX_UNAVAILABLE;
  }
  binder_ = new BinderServiceInternal(this);
  if (!android::BinderWrapper::Get()->RegisterService(
          kTrunksServiceName, android::IInterface::asBinder(binder_))) {
    LOG(ERROR) << "TrunksBinderService: RegisterService failed.";
    return EX_UNAVAILABLE;
  }
  LOG(INFO) << "Trunks: Binder service registered.";
  return brillo::Daemon::OnInit();
}

TrunksBinderService::BinderServiceInternal::BinderServiceInternal(
    TrunksBinderService* service)
    : service_(service) {}

android::binder::Status TrunksBinderService::BinderServiceInternal::SendCommand(
    const std::vector<uint8_t>& command,
    const android::sp<android::trunks::ITrunksClient>& client) {
  auto callback =
      base::Bind(&TrunksBinderService::BinderServiceInternal::OnResponse,
                 GetWeakPtr(), client);
  std::string command_data;
  if (!ParseCommandProto(command, &command_data)) {
    LOG(ERROR) << "TrunksBinderService: Bad command data.";
    callback.Run(CreateErrorResponse(SAPI_RC_BAD_PARAMETER));
    return android::binder::Status::ok();
  }
  service_->transceiver_->SendCommand(command_data, callback);
  return android::binder::Status::ok();
}

void TrunksBinderService::BinderServiceInternal::OnResponse(
    const android::sp<android::trunks::ITrunksClient>& client,
    const std::string& response) {
  std::vector<uint8_t> binder_response;
  CreateResponseProto(response, &binder_response);
  android::binder::Status status = client->OnCommandResponse(binder_response);
  if (!status.isOk()) {
    LOG(ERROR) << "TrunksBinderService: Failed to send response to client: "
               << status.toString8();
  }
}

android::binder::Status
TrunksBinderService::BinderServiceInternal::SendCommandAndWait(
    const std::vector<uint8_t>& command,
    std::vector<uint8_t>* response) {
  std::string command_data;
  if (!ParseCommandProto(command, &command_data)) {
    LOG(ERROR) << "TrunksBinderService: Bad command data.";
    CreateResponseProto(CreateErrorResponse(SAPI_RC_BAD_PARAMETER), response);
    return android::binder::Status::ok();
  }
  CreateResponseProto(service_->transceiver_->SendCommandAndWait(command_data),
                      response);
  return android::binder::Status::ok();
}

}  // namespace trunks
