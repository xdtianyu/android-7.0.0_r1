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

#ifndef TPM_MANAGER_CLIENT_TPM_OWNERSHIP_DBUS_PROXY_H_
#define TPM_MANAGER_CLIENT_TPM_OWNERSHIP_DBUS_PROXY_H_

#include "tpm_manager/common/tpm_ownership_interface.h"

#include <string>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <dbus/bus.h>
#include <dbus/object_proxy.h>

#include "tpm_manager/common/export.h"

namespace tpm_manager {

// An implementation of TpmOwnershipInterface that forwards requests to
// tpm_managerd over D-Bus.
// Usage:
// std::unique_ptr<TpmOwnershipInterface> tpm_ = new TpmOwnershipDBusProxy();
// tpm_->GetTpmStatus(...);
class TPM_MANAGER_EXPORT TpmOwnershipDBusProxy : public TpmOwnershipInterface {
 public:
  TpmOwnershipDBusProxy() = default;
  virtual ~TpmOwnershipDBusProxy();

  // Performs initialization tasks. This method must be called before calling
  // any other method in this class. Returns true on success.
  bool Initialize();

  // TpmOwnershipInterface methods.
  void GetTpmStatus(const GetTpmStatusRequest& request,
                    const GetTpmStatusCallback& callback) override;
  void TakeOwnership(const TakeOwnershipRequest& request,
                     const TakeOwnershipCallback& callback) override;
  void RemoveOwnerDependency(
      const RemoveOwnerDependencyRequest& request,
      const RemoveOwnerDependencyCallback& callback) override;

  void set_object_proxy(dbus::ObjectProxy* object_proxy) {
    object_proxy_ = object_proxy;
  }

 private:
  // Template method to call a given |method_name| remotely via dbus.
  template<typename ReplyProtobufType,
           typename RequestProtobufType,
           typename CallbackType>
  void CallMethod(const std::string& method_name,
                  const RequestProtobufType& request,
                  const CallbackType& callback);

  scoped_refptr<dbus::Bus> bus_;
  dbus::ObjectProxy* object_proxy_;
  DISALLOW_COPY_AND_ASSIGN(TpmOwnershipDBusProxy);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_CLIENT_TPM_OWNERSHIP_DBUS_PROXY_H_
