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

#ifndef TPM_MANAGER_CLIENT_TPM_NVRAM_DBUS_PROXY_H_
#define TPM_MANAGER_CLIENT_TPM_NVRAM_DBUS_PROXY_H_

#include "tpm_manager/common/tpm_nvram_interface.h"

#include <string>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <dbus/bus.h>
#include <dbus/object_proxy.h>

#include "tpm_manager/common/export.h"

namespace tpm_manager {

// An implementation of TpmNvramInterface that forwards requests to
// tpm_managerd over D-Bus.
// Usage:
// std::unique_ptr<TpmNvramInterface> tpm_manager = new TpmNvramDBusProxy();
// tpm_manager->DefineNvram(...);
class TPM_MANAGER_EXPORT TpmNvramDBusProxy : public TpmNvramInterface {
 public:
  TpmNvramDBusProxy() = default;
  virtual ~TpmNvramDBusProxy();

  // Performs initialization tasks. This method must be called before calling
  // any other method in this class. Returns true on success.
  bool Initialize();

  // TpmNvramInterface methods.
  void DefineNvram(const DefineNvramRequest& request,
                   const DefineNvramCallback& callback) override;
  void DestroyNvram(const DestroyNvramRequest& request,
                    const DestroyNvramCallback& callback) override;
  void WriteNvram(const WriteNvramRequest& request,
                  const WriteNvramCallback& callback) override;
  void ReadNvram(const ReadNvramRequest& request,
                 const ReadNvramCallback& callback) override;
  void IsNvramDefined(const IsNvramDefinedRequest& request,
                      const IsNvramDefinedCallback& callback) override;
  void IsNvramLocked(const IsNvramLockedRequest& request,
                     const IsNvramLockedCallback& callback) override;
  void GetNvramSize(const GetNvramSizeRequest& request,
                    const GetNvramSizeCallback& callback) override;

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
  DISALLOW_COPY_AND_ASSIGN(TpmNvramDBusProxy);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_CLIENT_TPM_NVRAM_DBUS_PROXY_H_
