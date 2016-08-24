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

#ifndef SHILL_CELLULAR_CELLULAR_CAPABILITY_H_
#define SHILL_CELLULAR_CELLULAR_CAPABILITY_H_

#include <string>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/callbacks.h"
#include "shill/cellular/cellular.h"
#include "shill/metrics.h"

namespace shill {

class Cellular;
class CellularBearer;
class Error;
class ModemInfo;

// Cellular devices instantiate subclasses of CellularCapability that
// handle the specific modem technologies and capabilities.
//
// The CellularCapability is directly subclassed by:
// *  CelllularCapabilityUniversal which handles all modems managed by
//    a modem manager using the the org.chromium.ModemManager1 DBUS
//    interface.
// *  CellularCapabilityClassic which handles all modems managed by a
//    modem manager using the older org.chromium.ModemManager DBUS
//    interface.  This class is further subclassed to represent CDMA
//    and GSM modems.
//
// Pictorially:
//
// CellularCapability
//       |
//       |-- CellularCapabilityUniversal
//       |            |
//       |            |-- CellularCapabilityUniversalCDMA
//       |
//       |-- CellularCapabilityClassic
//                    |
//                    |-- CellularCapabilityGSM
//                    |
//                    |-- CellularCapabilityCDMA
//
// TODO(armansito): Currently, 3GPP logic is handled by
// CellularCapabilityUniversal. Eventually CellularCapabilityUniversal will
// only serve as the abstract base class for ModemManager1 3GPP and CDMA
// capabilities.
class CellularCapability {
 public:
  static const int kTimeoutActivate;
  static const int kTimeoutConnect;
  static const int kTimeoutDefault;
  static const int kTimeoutDisconnect;
  static const int kTimeoutEnable;
  static const int kTimeoutRegister;
  static const int kTimeoutReset;
  static const int kTimeoutScan;

  static const char kModemPropertyIMSI[];
  static const char kModemPropertyState[];

  // |cellular| is the parent Cellular device.
  CellularCapability(Cellular* cellular,
                     ControlInterface* control_interface,
                     ModemInfo* modem_info);
  virtual ~CellularCapability();

  virtual std::string GetTypeString() const = 0;

  // Called when the modem manager has sent a property change notification
  // signal.
  virtual void OnPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties) = 0;

  // -------------------------------------------------------------------------
  // Modem management
  // -------------------------------------------------------------------------

  // StartModem attempts to put the modem in a state in which it is usable for
  // creating services and establishing connections (if network conditions
  // permit). It potentially consists of multiple non-blocking calls to the
  // modem-manager server. After each call, control is passed back up to the
  // main loop. Each time a reply to a non-blocking call is received, the
  // operation advances to the next step, until either an error occurs in one of
  // them, or all the steps have been completed, at which point StartModem() is
  // finished.
  virtual void StartModem(Error* error, const ResultCallback& callback) = 0;

  // StopModem disconnects and disables a modem asynchronously.  |callback| is
  // invoked when this completes and the result is passed to the callback.
  virtual void StopModem(Error* error, const ResultCallback& callback) = 0;

  // Resets the modem.
  //
  // The default implementation fails by returning kNotSupported via |error|.
  virtual void Reset(Error* error, const ResultCallback& callback);

  // Checks to see if all proxies have been initialized.
  virtual bool AreProxiesInitialized() const = 0;

  // -------------------------------------------------------------------------
  // Activation
  // -------------------------------------------------------------------------

  // Returns true if service activation is required.
  //
  // The default implementation returns false.
  virtual bool IsServiceActivationRequired() const;

  // Returns true if the modem is being activated.
  //
  // The default implementation returns false.
  virtual bool IsActivating() const;

  // Activates the modem.
  //
  // The default implementation fails by returning kNotSupported via |error|.
  virtual void Activate(const std::string& carrier,
                        Error* error, const ResultCallback& callback);

  // Initiates the necessary to steps to verify that the cellular service has
  // been activated. Once these steps have been completed, the service should
  // be marked as activated.
  //
  // The default implementation fails by returning kNotSupported via |error|.
  virtual void CompleteActivation(Error* error);

  // -------------------------------------------------------------------------
  // Network service and registration
  // -------------------------------------------------------------------------

  // Configures the modem to support the |carrier|.
  //
  // The default implementation fails by returning kNotSupported via |error|.
  virtual void SetCarrier(const std::string& carrier,
                          Error* error,
                          const ResultCallback& callback);

  // Asks the modem to scan for networks.
  //
  // The default implementation fails by returning kNotSupported via |error|.
  //
  // Subclasses should implement this by fetching scan results asynchronously.
  // When the results are ready, update the kFoundNetworksProperty and send a
  // property change notification.  Finally, callback must be invoked to inform
  // the caller that the scan has completed.
  //
  // Errors are not generally reported, but on error the kFoundNetworksProperty
  // should be cleared and a property change notification sent out.
  //
  // TODO(jglasgow): Refactor to reuse code by putting notification logic into
  // Cellular or CellularCapability.
  //
  // TODO(jglasgow): Implement real error handling.
  virtual void Scan(Error* error, const ResultStringmapsCallback& callback);

  // Registers on a network with |network_id|.
  virtual void RegisterOnNetwork(const std::string& network_id,
                                 Error* error,
                                 const ResultCallback& callback);

  // Returns true if the modem is registered on a network, which can be a home
  // or roaming network. It is possible that we cannot determine whether it is
  // a home or roaming network, but we still consider the modem is registered.
  virtual bool IsRegistered() const = 0;

  // If we are informed by means of something other than a signal indicating
  // a registration state change that the modem has unregistered from the
  // network, we need to update the network-type-specific capability object.
  virtual void SetUnregistered(bool searching) = 0;

  // Invoked by the parent Cellular device when a new service is created.
  virtual void OnServiceCreated() = 0;

  // Hook called by the Cellular device when either the Home Provider or the
  // Serving Operator changes. Default implementation calls other hooks declared
  // below. Overrides should chain up to this function.
  // Note: This may be called before |CellularService| is created.
  virtual void OnOperatorChanged();
  virtual void UpdateServiceOLP();

  // Returns an empty string if the network technology is unknown.
  virtual std::string GetNetworkTechnologyString() const = 0;

  virtual std::string GetRoamingStateString() const = 0;

  // Should this device allow roaming?
  // The decision to allow roaming or not is based on the home provider as well
  // as on the user modifiable "allow_roaming" property.
  virtual bool AllowRoaming() = 0;

  // Returns true if the cellular device should initiate passive traffic
  // monitoring to trigger active out-of-credit detection checks. The default
  // implementation returns false by default.
  virtual bool ShouldDetectOutOfCredit() const;

  // TODO(armansito): Remove this method once cromo is deprecated.
  virtual void GetSignalQuality() = 0;

  // -------------------------------------------------------------------------
  // Connection management
  // -------------------------------------------------------------------------

  // Fills |properties| with properties for establishing a connection, which
  // will be passed to Connect().
  virtual void SetupConnectProperties(KeyValueStore* properties) = 0;

  // Connects the modem to a network based on the connection properties
  // specified by |properties|.
  virtual void Connect(const KeyValueStore& properties,
                       Error* error,
                       const ResultCallback& callback) = 0;

  // Disconnects the modem from a network.
  virtual void Disconnect(Error* error, const ResultCallback& callback) = 0;

  // Called when a disconnect operation completes, successful or not.
  //
  // The default implementation does nothing.
  virtual void DisconnectCleanup();

  // Returns a pointer to the current active bearer object or nullptr if no
  // active bearer exists. The returned bearer object is managed by this
  // capability object. This implementation returns nullptr by default.
  virtual CellularBearer* GetActiveBearer() const;

  // -------------------------------------------------------------------------
  // SIM PIN management
  // -------------------------------------------------------------------------

  // The default implementation fails by returning kNotSupported via |error|.
  virtual void RequirePIN(const std::string& pin,
                          bool require,
                          Error* error,
                          const ResultCallback& callback);

  virtual void EnterPIN(const std::string& pin,
                        Error* error,
                        const ResultCallback& callback);

  virtual void UnblockPIN(const std::string& unblock_code,
                          const std::string& pin,
                          Error* error,
                          const ResultCallback& callback);

  virtual void ChangePIN(const std::string& old_pin,
                         const std::string& new_pin,
                         Error* error,
                         const ResultCallback& callback);

  // -------------------------------------------------------------------------

  Cellular* cellular() const { return cellular_; }
  ControlInterface* control_interface() const { return control_interface_; }
  ModemInfo* modem_info() const { return modem_info_; }

 protected:
  // Releases all proxies held by the object. This is most useful during unit
  // tests.
  virtual void ReleaseProxies() = 0;

  static void OnUnsupportedOperation(const char* operation, Error* error);

  // Accessor for subclasses to read the 'allow roaming' property.
  bool allow_roaming_property() const {
    return cellular_->allow_roaming_property();
  }

 private:
  friend class CellularCapabilityGSMTest;
  friend class CellularCapabilityTest;
  friend class CellularCapabilityUniversalTest;
  friend class CellularCapabilityUniversalCDMATest;
  friend class CellularTest;
  FRIEND_TEST(CellularCapabilityTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, UpdateActiveBearer);
  FRIEND_TEST(CellularTest, Connect);
  FRIEND_TEST(CellularTest, TearDown);

  Cellular* cellular_;
  ControlInterface* control_interface_;
  ModemInfo* modem_info_;

  DISALLOW_COPY_AND_ASSIGN(CellularCapability);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_CELLULAR_CAPABILITY_H_
