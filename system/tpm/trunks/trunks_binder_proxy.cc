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

#include "trunks/trunks_binder_proxy.h"

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <binderwrapper/binder_wrapper.h>
#include <utils/Errors.h>

#include "android/trunks/BnTrunksClient.h"
#include "android/trunks/BpTrunks.h"
#include "trunks/binder_interface.h"
#include "trunks/error_codes.h"
#include "trunks/interface.pb.h"

namespace {

// Implements ITrunksClient and forwards response data to a ResponseCallback.
class ResponseObserver : public android::trunks::BnTrunksClient {
 public:
  ResponseObserver(const trunks::CommandTransceiver::ResponseCallback& callback)
      : callback_(callback) {}

  // ITrunksClient interface.
  android::binder::Status OnCommandResponse(
      const std::vector<uint8_t>& response_proto_data) override {
    trunks::SendCommandResponse response_proto;
    if (!response_proto.ParseFromArray(response_proto_data.data(),
                                       response_proto_data.size())) {
      LOG(ERROR) << "TrunksBinderProxy: Bad response data.";
      callback_.Run(
          trunks::CreateErrorResponse(trunks::SAPI_RC_MALFORMED_RESPONSE));
    }
    callback_.Run(response_proto.response());
    return android::binder::Status::ok();
  }

 private:
  trunks::CommandTransceiver::ResponseCallback callback_;
};

}  // namespace

namespace trunks {

bool TrunksBinderProxy::Init() {
  android::sp<android::IBinder> service_binder =
      android::BinderWrapper::GetOrCreateInstance()->GetService(
          kTrunksServiceName);
  if (!service_binder.get()) {
    LOG(ERROR) << "TrunksBinderProxy: Trunks service does not exist.";
    return false;
  }
  trunks_service_ = new android::trunks::BpTrunks(service_binder);
  return true;
}

void TrunksBinderProxy::SendCommand(const std::string& command,
                                    const ResponseCallback& callback) {
  SendCommandRequest command_proto;
  command_proto.set_command(command);
  std::vector<uint8_t> command_proto_data;
  command_proto_data.resize(command_proto.ByteSize());
  if (!command_proto.SerializeToArray(command_proto_data.data(),
                                      command_proto_data.size())) {
    LOG(ERROR) << "TrunksBinderProxy: Failed to serialize protobuf.";
    callback.Run(CreateErrorResponse(TRUNKS_RC_IPC_ERROR));
    return;
  }
  android::sp<ResponseObserver> observer(new ResponseObserver(callback));
  android::binder::Status status =
      trunks_service_->SendCommand(command_proto_data, observer);
  if (!status.isOk()) {
    LOG(ERROR) << "TrunksBinderProxy: Binder error: " << status.toString8();
    callback.Run(CreateErrorResponse(TRUNKS_RC_IPC_ERROR));
    return;
  }
}

std::string TrunksBinderProxy::SendCommandAndWait(const std::string& command) {
  SendCommandRequest command_proto;
  command_proto.set_command(command);
  std::vector<uint8_t> command_proto_data;
  command_proto_data.resize(command_proto.ByteSize());
  if (!command_proto.SerializeToArray(command_proto_data.data(),
                                      command_proto_data.size())) {
    LOG(ERROR) << "TrunksBinderProxy: Failed to serialize protobuf.";
    return CreateErrorResponse(TRUNKS_RC_IPC_ERROR);
  }
  std::vector<uint8_t> response_proto_data;
  android::binder::Status status = trunks_service_->SendCommandAndWait(
      command_proto_data, &response_proto_data);
  if (!status.isOk()) {
    LOG(ERROR) << "TrunksBinderProxy: Binder error: " << status.toString8();
    return CreateErrorResponse(TRUNKS_RC_IPC_ERROR);
  }
  trunks::SendCommandResponse response_proto;
  if (!response_proto.ParseFromArray(response_proto_data.data(),
                                     response_proto_data.size())) {
    LOG(ERROR) << "TrunksBinderProxy: Bad response data.";
    return trunks::CreateErrorResponse(trunks::SAPI_RC_MALFORMED_RESPONSE);
  }
  return response_proto.response();
}

}  // namespace trunks
