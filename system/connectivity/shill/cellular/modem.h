//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_CELLULAR_MODEM_H_
#define SHILL_CELLULAR_MODEM_H_

#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <base/files/file_util.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/cellular/cellular.h"
#include "shill/cellular/dbus_objectmanager_proxy_interface.h"
#include "shill/cellular/modem_info.h"
#include "shill/dbus_properties_proxy_interface.h"
#include "shill/refptr_types.h"

namespace shill {

class ControlInterface;

// Handles an instance of ModemManager.Modem and an instance of a Cellular
// device.
class Modem {
 public:
  // ||path| is the ModemManager.Modem DBus object path (e.g.,
  // "/org/chromium/ModemManager/Gobi/0").
  Modem(const std::string& service,
        const std::string& path,
        ModemInfo* modem_info,
        ControlInterface* control_interface);
  virtual ~Modem();

  // Asynchronously initializes support for the modem.
  // If the |properties| are valid and the MAC address is present,
  // constructs and registers a Cellular device in |device_| based on
  // |properties|.
  virtual void CreateDeviceFromModemProperties(
      const InterfaceToProperties& properties);

  void OnDeviceInfoAvailable(const std::string& link_name);

  const std::string& service() const { return service_; }
  const std::string& path() const { return path_; }

  void set_type(Cellular::Type type) { type_ = type; }

 protected:
  static const char kPropertyLinkName[];
  static const char kPropertyIPMethod[];
  static const char kPropertyType[];

  virtual void Init();

  CellularRefPtr device() const { return device_; }

  virtual Cellular* ConstructCellular(const std::string& link_name,
                                      const std::string& device_name,
                                      int interface_index);
  virtual bool GetLinkName(const KeyValueStore& properties,
                           std::string* name) const = 0;
  // Returns the name of the DBUS Modem interface.
  virtual std::string GetModemInterface(void) const = 0;

 private:
  friend class ModemTest;
  friend class Modem1Test;
  FRIEND_TEST(Modem1Test, Init);
  FRIEND_TEST(Modem1Test, CreateDeviceMM1);
  FRIEND_TEST(ModemManager1Test, Connect);
  FRIEND_TEST(ModemManager1Test, AddRemoveInterfaces);
  FRIEND_TEST(ModemManagerClassicTest, Connect);
  FRIEND_TEST(ModemManagerClassicTest, StartStop);
  FRIEND_TEST(ModemManagerCoreTest, ShouldAddModem);
  FRIEND_TEST(ModemTest, CreateDeviceEarlyFailures);
  FRIEND_TEST(ModemTest, CreateDevicePPP);
  FRIEND_TEST(ModemTest, EarlyDeviceProperties);
  FRIEND_TEST(ModemTest, GetDeviceParams);
  FRIEND_TEST(ModemTest, Init);
  FRIEND_TEST(ModemTest, PendingDevicePropertiesAndCreate);

  // Constants associated with fake network devices for PPP dongles.
  // See |fake_dev_serial_|, below, for more info.
  static constexpr char kFakeDevNameFormat[] = "no_netdev_%zu";
  static const char kFakeDevAddress[];
  static const int kFakeDevInterfaceIndex;

  // Find the |mac_address| and |interface_index| for the kernel
  // network device with name |link_name|. Returns true iff both
  // |mac_address| and |interface_index| were found. Modifies
  // |interface_index| even on failure.
  virtual bool GetDeviceParams(std::string* mac_address, int* interface_index);

  virtual void OnPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties);
  virtual void OnModemManagerPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& properties);

  // A proxy to the org.freedesktop.DBusProperties interface used to obtain
  // ModemManager.Modem properties and watch for property changes
  std::unique_ptr<DBusPropertiesProxyInterface> dbus_properties_proxy_;

  InterfaceToProperties initial_properties_;

  const std::string service_;
  const std::string path_;

  CellularRefPtr device_;

  ModemInfo* modem_info_;
  std::string link_name_;
  Cellular::Type type_;
  bool pending_device_info_;
  RTNLHandler* rtnl_handler_;

  ControlInterface* control_interface_;

  // Serial number used to uniquify fake device names for Cellular
  // devices that don't have network devices. (Names must be unique
  // for D-Bus, and PPP dongles don't have network devices.)
  static size_t fake_dev_serial_;

  DISALLOW_COPY_AND_ASSIGN(Modem);
};

class ModemClassic : public Modem {
 public:
  ModemClassic(const std::string& service,
               const std::string& path,
               ModemInfo* modem_info,
               ControlInterface* control_interface);
  ~ModemClassic() override;

  // Gathers information and passes it to CreateDeviceFromModemProperties.
  void CreateDeviceClassic(const KeyValueStore& modem_properties);

 protected:
  bool GetLinkName(const KeyValueStore& modem_properties,
                   std::string* name) const override;
  std::string GetModemInterface(void) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ModemClassic);
};

class Modem1 : public Modem {
 public:
  Modem1(const std::string& service,
         const std::string& path,
         ModemInfo* modem_info,
         ControlInterface* control_interface);
  ~Modem1() override;

  // Gathers information and passes it to CreateDeviceFromModemProperties.
  void CreateDeviceMM1(const InterfaceToProperties& properties);

 protected:
  bool GetLinkName(const KeyValueStore& modem_properties,
                   std::string* name) const override;
  std::string GetModemInterface(void) const override;

 private:
  friend class Modem1Test;

  DISALLOW_COPY_AND_ASSIGN(Modem1);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_MODEM_H_
