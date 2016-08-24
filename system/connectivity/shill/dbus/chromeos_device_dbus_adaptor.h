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

#ifndef SHILL_DBUS_CHROMEOS_DEVICE_DBUS_ADAPTOR_H_
#define SHILL_DBUS_CHROMEOS_DEVICE_DBUS_ADAPTOR_H_

#include <string>
#include <vector>

#include <base/macros.h>

#include "dbus_bindings/org.chromium.flimflam.Device.h"
#include "shill/adaptor_interfaces.h"
#include "shill/dbus/chromeos_dbus_adaptor.h"

namespace shill {

class Device;

// There is a 1:1 mapping between Device and DeviceDBusAdaptor instances.
// Furthermore, the Device owns the DeviceDBusAdaptor and manages its lifetime,
// so we're OK with DeviceDBusAdaptor having a bare pointer to its owner device.
class ChromeosDeviceDBusAdaptor
    : public org::chromium::flimflam::DeviceAdaptor,
      public org::chromium::flimflam::DeviceInterface,
      public ChromeosDBusAdaptor,
      public DeviceAdaptorInterface {
 public:
  static const char kPath[];

  ChromeosDeviceDBusAdaptor(
      const scoped_refptr<dbus::Bus>& bus,
      Device* device);
  ~ChromeosDeviceDBusAdaptor() override;

  // Implementation of DeviceAdaptorInterface.
  const std::string& GetRpcIdentifier() override;
  void EmitBoolChanged(const std::string& name, bool value) override;
  void EmitUintChanged(const std::string& name, uint32_t value) override;
  void EmitUint16Changed(const std::string& name, uint16_t value) override;
  void EmitIntChanged(const std::string& name, int value) override;
  void EmitStringChanged(const std::string& name,
                         const std::string& value) override;
  void EmitStringmapChanged(const std::string& name,
                            const Stringmap& value) override;
  void EmitStringmapsChanged(const std::string& name,
                             const Stringmaps& value) override;
  void EmitStringsChanged(const std::string& name,
                          const Strings& value) override;
  void EmitKeyValueStoreChanged(const std::string& name,
                                const KeyValueStore& value) override;
  void EmitRpcIdentifierChanged(const std::string& name,
                                const std::string& value) override;
  void EmitRpcIdentifierArrayChanged(
      const std::string& name, const std::vector<std::string>& value) override;

  // Implementation of DeviceAdaptor.
  bool GetProperties(brillo::ErrorPtr* error,
                     brillo::VariantDictionary* out_properties) override;
  bool SetProperty(brillo::ErrorPtr* error,
                   const std::string& name,
                   const brillo::Any& value) override;
  bool ClearProperty(brillo::ErrorPtr* error,
                     const std::string& name) override;
  void Enable(DBusMethodResponsePtr<> response) override;
  void Disable(DBusMethodResponsePtr<> response) override;
  bool ProposeScan(brillo::ErrorPtr* error) override;
  bool AddIPConfig(brillo::ErrorPtr* error,
                   const std::string& method,
                   dbus::ObjectPath* out_path) override;
  void Register(DBusMethodResponsePtr<> response,
                const std::string& network_id) override;
  void RequirePin(DBusMethodResponsePtr<> response,
                  const std::string& pin,
                  bool require) override;
  void EnterPin(DBusMethodResponsePtr<> response,
                const std::string& pin) override;
  void UnblockPin(DBusMethodResponsePtr<> response,
                  const std::string& unblock_code,
                  const std::string& pin) override;
  void ChangePin(DBusMethodResponsePtr<> response,
                 const std::string& old_pin,
                 const std::string& new_pin) override;
  bool PerformTDLSOperation(brillo::ErrorPtr* error,
                            const std::string& operation,
                            const std::string& peer,
                            std::string* out_state) override;
  void Reset(DBusMethodResponsePtr<> response) override;
  bool ResetByteCounters(brillo::ErrorPtr* error) override;
  bool RequestRoam(brillo::ErrorPtr* error,
                   const std::string& addr) override;
  void SetCarrier(DBusMethodResponsePtr<> response,
                  const std::string& carrierr) override;
  bool AddWakeOnPacketConnection(brillo::ErrorPtr* error,
                                 const std::string& ip_endpoint) override;
  bool RemoveWakeOnPacketConnection(brillo::ErrorPtr* error,
                                    const std::string& ip_endpoint) override;
  bool RemoveAllWakeOnPacketConnections(brillo::ErrorPtr* error) override;

  Device* device() const { return device_; }

 private:
  Device* device_;

  DISALLOW_COPY_AND_ASSIGN(ChromeosDeviceDBusAdaptor);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_DEVICE_DBUS_ADAPTOR_H_
