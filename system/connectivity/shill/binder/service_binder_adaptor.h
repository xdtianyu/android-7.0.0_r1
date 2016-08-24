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

#ifndef SHILL_BINDER_SERVICE_BINDER_ADAPTOR_H_
#define SHILL_BINDER_SERVICE_BINDER_ADAPTOR_H_

#include <string>

#include <base/macros.h>
#include <utils/StrongPointer.h>

#include "android/system/connectivity/shill/BnService.h"
#include "shill/adaptor_interfaces.h"
#include "shill/binder/binder_adaptor.h"

namespace android {
namespace binder {
class Status;
}  // namespace binder
namespace system {
namespace connectivity {
namespace shill {
class IPropertyChangedCallback;
}  // namespace shill
}  // namespace connectivity
}  // namespace system
}  // namespace android

namespace shill {

class Service;

// Subclass of DBusAdaptor for Service objects
// There is a 1:1 mapping between Service and ServiceBinderAdaptor
// instances.  Furthermore, the Service owns the ServiceBinderAdaptor
// and manages its lifetime, so we're OK with ServiceBinderAdaptor
// having a bare pointer to its owner service.
class ServiceBinderAdaptor
    : public android::system::connectivity::shill::BnService,
      public BinderAdaptor,
      public ServiceAdaptorInterface {
 public:
  ServiceBinderAdaptor(Service* service, const std::string& id);
  ~ServiceBinderAdaptor() override;

  // Implementation of ServiceAdaptorInterface.
  const std::string& GetRpcIdentifier() override { return id(); }
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUint8Changed(const std::string& name, uint8_t value) override;
  void EmitUint16Changed(const std::string& name, uint16_t value) override;
  void EmitUint16sChanged(const std::string& name,
                          const Uint16s& value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitRpcIdentifierChanged(
      const std::string& name, const std::string& value) override;
  void EmitStringChanged(
      const std::string& name, const std::string& value) override;
  void EmitStringmapChanged(const std::string& name,
                            const Stringmap& value) override;

  // Implementation of BnService.
  android::binder::Status Connect();
  android::binder::Status GetState(int32_t* _aidl_return);
  android::binder::Status GetStrength(int8_t* _aidl_return);
  android::binder::Status GetError(int32_t* _aidl_return);
  android::binder::Status RegisterPropertyChangedSignalHandler(
      const android::sp<
          android::system::connectivity::shill::IPropertyChangedCallback>&
          callback);

  Service* service() const { return service_; }

 private:
  Service* service_;

  DISALLOW_COPY_AND_ASSIGN(ServiceBinderAdaptor);
};

}  // namespace shill

#endif  // SHILL_BINDER_SERVICE_BINDER_ADAPTOR_H_
