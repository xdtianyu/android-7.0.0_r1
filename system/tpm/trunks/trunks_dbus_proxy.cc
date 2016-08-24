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

#include "trunks/trunks_dbus_proxy.h"

#include <base/bind.h>
#include <brillo/bind_lambda.h>
#include <brillo/dbus/dbus_method_invoker.h>

#include "trunks/dbus_interface.h"
#include "trunks/error_codes.h"
#include "trunks/interface.pb.h"

namespace {

// Use a five minute timeout because some commands on some TPM hardware can take
// a very long time. If a few lengthy operations are already in the queue, a
// subsequent command needs to wait for all of them. Timeouts are always
// possible but under normal conditions 5 minutes seems to be plenty.
const int kDBusMaxTimeout = 5 * 60 * 1000;

}  // namespace

namespace trunks {

TrunksDBusProxy::TrunksDBusProxy() : weak_factory_(this) {}

TrunksDBusProxy::~TrunksDBusProxy() {
  if (bus_) {
    bus_->ShutdownAndBlock();
  }
}

bool TrunksDBusProxy::Init() {
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  bus_ = new dbus::Bus(options);
  object_proxy_ = bus_->GetObjectProxy(
      trunks::kTrunksServiceName,
      dbus::ObjectPath(trunks::kTrunksServicePath));
  origin_thread_id_ = base::PlatformThread::CurrentId();
  return (object_proxy_ != nullptr);
}

void TrunksDBusProxy::SendCommand(const std::string& command,
                              const ResponseCallback& callback) {
  if (origin_thread_id_ != base::PlatformThread::CurrentId()) {
    LOG(ERROR) << "Error TrunksDBusProxy cannot be shared by multiple threads.";
    callback.Run(CreateErrorResponse(TRUNKS_RC_IPC_ERROR));
  }
  SendCommandRequest tpm_command_proto;
  tpm_command_proto.set_command(command);
  auto on_success = [callback](const SendCommandResponse& response) {
    callback.Run(response.response());
  };
  auto on_error = [callback](brillo::Error* error) {
    SendCommandResponse response;
    response.set_response(CreateErrorResponse(SAPI_RC_NO_RESPONSE_RECEIVED));
    callback.Run(response.response());
  };
  brillo::dbus_utils::CallMethodWithTimeout(
      kDBusMaxTimeout,
      object_proxy_,
      trunks::kTrunksInterface,
      trunks::kSendCommand,
      base::Bind(on_success),
      base::Bind(on_error),
      tpm_command_proto);
}

std::string TrunksDBusProxy::SendCommandAndWait(const std::string& command) {
  if (origin_thread_id_ != base::PlatformThread::CurrentId()) {
    LOG(ERROR) << "Error TrunksDBusProxy cannot be shared by multiple threads.";
    return CreateErrorResponse(TRUNKS_RC_IPC_ERROR);
  }
  SendCommandRequest tpm_command_proto;
  tpm_command_proto.set_command(command);
  brillo::ErrorPtr error;
  std::unique_ptr<dbus::Response> dbus_response =
      brillo::dbus_utils::CallMethodAndBlockWithTimeout(
          kDBusMaxTimeout,
          object_proxy_,
          trunks::kTrunksInterface,
          trunks::kSendCommand,
          &error,
          tpm_command_proto);
  SendCommandResponse tpm_response_proto;
  if (dbus_response.get() && brillo::dbus_utils::ExtractMethodCallResults(
      dbus_response.get(), &error, &tpm_response_proto)) {
    return tpm_response_proto.response();
  } else {
    LOG(ERROR) << "TrunksProxy could not parse response: "
               << error->GetMessage();
    return CreateErrorResponse(SAPI_RC_MALFORMED_RESPONSE);
  }
}


}  // namespace trunks
