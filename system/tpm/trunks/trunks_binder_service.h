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

#ifndef TRUNKS_TRUNKS_BINDER_SERVICE_H_
#define TRUNKS_TRUNKS_BINDER_SERVICE_H_

#include <base/memory/weak_ptr.h>
#include <brillo/binder_watcher.h>
#include <brillo/daemons/daemon.h>

#include "android/trunks/BnTrunks.h"
#include "trunks/command_transceiver.h"

namespace trunks {

// TrunksBinderService registers for and handles all incoming binder calls for
// the trunksd system daemon.
//
// Example Usage:
//
// TrunksBinderService service;
// service.set_transceiver(&my_transceiver);
// service.Run();
class TrunksBinderService : public brillo::Daemon {
 public:
  TrunksBinderService() = default;
  ~TrunksBinderService() override = default;

  // The |transceiver| will be the target of all incoming TPM commands. This
  // class does not take ownership of |transceiver|.
  void set_transceiver(CommandTransceiver* transceiver) {
    transceiver_ = transceiver;
  }

 protected:
  int OnInit() override;

 private:
  friend class BinderServiceInternal;
  class BinderServiceInternal : public android::trunks::BnTrunks {
   public:
    BinderServiceInternal(TrunksBinderService* service);
    ~BinderServiceInternal() override = default;

    // ITrunks interface.
    android::binder::Status SendCommand(
        const std::vector<uint8_t>& command,
        const android::sp<android::trunks::ITrunksClient>& client) override;
    android::binder::Status SendCommandAndWait(
        const std::vector<uint8_t>& command,
        std::vector<uint8_t>* response) override;

   private:
    void OnResponse(const android::sp<android::trunks::ITrunksClient>& client,
                    const std::string& response);

    base::WeakPtr<BinderServiceInternal> GetWeakPtr() {
      return weak_factory_.GetWeakPtr();
    }

    TrunksBinderService* service_;

    // Declared last so weak pointers are invalidated first on destruction.
    base::WeakPtrFactory<BinderServiceInternal> weak_factory_{this};
  };

  CommandTransceiver* transceiver_ = nullptr;
  brillo::BinderWatcher watcher_;
  android::sp<BinderServiceInternal> binder_;

  DISALLOW_COPY_AND_ASSIGN(TrunksBinderService);
};

}  // namespace trunks


#endif  // TRUNKS_TRUNKS_BINDER_SERVICE_H_
