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

#ifndef TRUNKS_TRUNKS_BINDER_PROXY_H_
#define TRUNKS_TRUNKS_BINDER_PROXY_H_

#include <string>

#include "android/trunks/ITrunks.h"
#include "trunks/command_transceiver.h"
#include "trunks/trunks_export.h"

namespace trunks {

// TrunksBinderProxy is a CommandTransceiver implementation that forwards all
// commands to the trunksd binder daemon. See TrunksBinderService for details on
// how the commands are handled once they reach trunksd.
class TRUNKS_EXPORT TrunksBinderProxy : public CommandTransceiver {
 public:
  TrunksBinderProxy() = default;
  ~TrunksBinderProxy() override = default;

  // Initializes the client. Returns true on success.
  bool Init() override;

  // Asynchronous calls assume a message loop and binder watcher have already
  // been configured elsewhere.
  void SendCommand(const std::string& command,
                   const ResponseCallback& callback) override;
  std::string SendCommandAndWait(const std::string& command) override;

 private:
  android::sp<android::trunks::ITrunks> trunks_service_;

  DISALLOW_COPY_AND_ASSIGN(TrunksBinderProxy);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_BINDER_PROXY_H_
