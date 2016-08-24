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

#ifndef TRUNKS_TRUNKS_DBUS_SERVICE_H_
#define TRUNKS_TRUNKS_DBUS_SERVICE_H_

#include <memory>
#include <string>

#include <base/memory/weak_ptr.h>
#include <brillo/daemons/dbus_daemon.h>
#include <brillo/dbus/dbus_method_response.h>
#include <brillo/dbus/dbus_object.h>

#include "trunks/command_transceiver.h"
#include "trunks/interface.pb.h"

namespace trunks {

// TrunksDBusService registers for and handles all incoming D-Bus messages for
// the trunksd system daemon.
//
// Example Usage:
//
// TrunksDBusService service;
// service.set_transceiver(&my_transceiver);
// service.Run();
class TrunksDBusService : public brillo::DBusServiceDaemon {
 public:
  TrunksDBusService();
  ~TrunksDBusService() override = default;

  // The |transceiver| will be the target of all incoming TPM commands. This
  // class does not take ownership of |transceiver|.
  void set_transceiver(CommandTransceiver* transceiver) {
    transceiver_ = transceiver;
  }

 protected:
  // Exports D-Bus methods.
  void RegisterDBusObjectsAsync(
      brillo::dbus_utils::AsyncEventSequencer* sequencer) override;

 private:
  // Handles calls to the 'SendCommand' method.
  void HandleSendCommand(
      std::unique_ptr<brillo::dbus_utils::DBusMethodResponse<
          const SendCommandResponse&>> response_sender,
      const SendCommandRequest& request);

  base::WeakPtr<TrunksDBusService> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  std::unique_ptr<brillo::dbus_utils::DBusObject> trunks_dbus_object_;
  CommandTransceiver* transceiver_ = nullptr;

  // Declared last so weak pointers are invalidated first on destruction.
  base::WeakPtrFactory<TrunksDBusService> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(TrunksDBusService);
};

}  // namespace trunks


#endif  // TRUNKS_TRUNKS_DBUS_SERVICE_H_
