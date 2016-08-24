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

#ifndef TRUNKS_TRUNKS_DBUS_PROXY_H_
#define TRUNKS_TRUNKS_DBUS_PROXY_H_

#include <string>

#include <base/memory/weak_ptr.h>
#include <base/threading/platform_thread.h>
#include <dbus/bus.h>
#include <dbus/object_proxy.h>

#include "trunks/command_transceiver.h"
#include "trunks/trunks_export.h"

namespace trunks {

// TrunksDBusProxy is a CommandTransceiver implementation that forwards all
// commands to the trunksd D-Bus daemon. See TrunksDBusService for details on
// how the commands are handled once they reach trunksd. A TrunksDBusProxy
// instance must be used in only one thread.
class TRUNKS_EXPORT TrunksDBusProxy: public CommandTransceiver {
 public:
  TrunksDBusProxy();
  ~TrunksDBusProxy() override;

  // Initializes the D-Bus client. Returns true on success.
  bool Init() override;

  // CommandTransceiver methods.
  void SendCommand(const std::string& command,
                   const ResponseCallback& callback) override;
  std::string SendCommandAndWait(const std::string& command) override;

 private:
  base::WeakPtr<TrunksDBusProxy> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  base::PlatformThreadId origin_thread_id_;
  scoped_refptr<dbus::Bus> bus_;
  dbus::ObjectProxy* object_proxy_;

  // Declared last so weak pointers are invalidated first on destruction.
  base::WeakPtrFactory<TrunksDBusProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrunksDBusProxy);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_DBUS_PROXY_H_
