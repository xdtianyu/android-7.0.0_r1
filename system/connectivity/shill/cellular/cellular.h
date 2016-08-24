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

#ifndef SHILL_CELLULAR_CELLULAR_H_
#define SHILL_CELLULAR_CELLULAR_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/cellular/mobile_operator_info.h"
#include "shill/cellular/modem_info.h"
#include "shill/cellular/modem_proxy_interface.h"
#include "shill/device.h"
#include "shill/event_dispatcher.h"
#include "shill/metrics.h"
#include "shill/refptr_types.h"
#include "shill/rpc_task.h"

namespace shill {

class CellularCapability;
class Error;
class ExternalTask;
class MobileOperatorInfo;
class PPPDeviceFactory;
class ProcessManager;

class Cellular : public Device, public RPCTaskDelegate {
 public:
  enum Type {
    kTypeGSM,
    kTypeCDMA,
    kTypeUniversal,  // ModemManager1
    kTypeUniversalCDMA,
    kTypeInvalid,
  };

  // The device states progress linearly from Disabled to Linked.
  enum State {
    // This is the initial state of the modem and indicates that the modem radio
    // is not turned on.
    kStateDisabled,
    // This state indicates that the modem radio is turned on, and it should be
    // possible to measure signal strength.
    kStateEnabled,
    // The modem has registered with a network and has signal quality
    // measurements. A cellular service object is created.
    kStateRegistered,
    // The modem has connected to a network.
    kStateConnected,
    // The network interface is UP.
    kStateLinked,
  };

  // This enum must be kept in sync with ModemManager's MMModemState enum.
  enum ModemState {
    kModemStateFailed = -1,
    kModemStateUnknown = 0,
    kModemStateInitializing = 1,
    kModemStateLocked = 2,
    kModemStateDisabled = 3,
    kModemStateDisabling = 4,
    kModemStateEnabling = 5,
    kModemStateEnabled = 6,
    kModemStateSearching = 7,
    kModemStateRegistered = 8,
    kModemStateDisconnecting = 9,
    kModemStateConnecting = 10,
    kModemStateConnected = 11,
  };

  // |path| is the ModemManager.Modem DBus object path (e.g.,
  // "/org/chromium/ModemManager/Gobi/0").
  // |service| is the modem mananager service name (e.g.,
  // /org/chromium/ModemManager or /org/freedesktop/ModemManager1).
  Cellular(ModemInfo* modem_info,
           const std::string& link_name,
           const std::string& address,
           int interface_index,
           Type type,
           const std::string& service,
           const std::string& path);
  ~Cellular() override;

  // Load configuration for the device from |storage|.
  bool Load(StoreInterface* storage) override;

  // Save configuration for the device to |storage|.
  bool Save(StoreInterface* storage) override;

  // Asynchronously connects the modem to the network. Populates |error| on
  // failure, leaves it unchanged otherwise.
  virtual void Connect(Error* error);

  // Asynchronously disconnects the modem from the network and populates
  // |error| on failure, leaves it unchanged otherwise.
  virtual void Disconnect(Error* error, const char* reason);

  // Asynchronously activates the modem. Returns an error on failure.
  void Activate(const std::string& carrier, Error* error,
                const ResultCallback& callback);

  // Performs the necessary steps to bring the service to the activated state,
  // once an online payment has been done.
  void CompleteActivation(Error* error);

  const CellularServiceRefPtr& service() const { return service_; }
  MobileOperatorInfo* home_provider_info() const {
    return home_provider_info_.get();
  }
  MobileOperatorInfo* serving_operator_info() const {
    return serving_operator_info_.get();
  }

  // Deregisters and destructs the current service and destroys the connection,
  // if any. This also eliminates the circular references between this device
  // and the associated service, allowing eventual device destruction.
  virtual void DestroyService();

  static std::string GetStateString(State state);
  static std::string GetModemStateString(ModemState modem_state);

  std::string CreateDefaultFriendlyServiceName();
  bool IsDefaultFriendlyServiceName(const std::string& service_name) const;

  // Update the home provider from the information in |operator_info|. This
  // information may be from the SIM / received OTA.
  void UpdateHomeProvider(const MobileOperatorInfo* operator_info);
  // Update the serving operator using information in |operator_info|.
  // Additionally, if |home_provider_info| is not nullptr, use it to come up
  // with a better name.
  void UpdateServingOperator(const MobileOperatorInfo* operator_info,
                             const MobileOperatorInfo* home_provider_info);

  State state() const { return state_; }

  void set_modem_state(ModemState state) { modem_state_ = state; }
  ModemState modem_state() const { return modem_state_; }
  bool IsUnderlyingDeviceEnabled() const override;
  bool IsModemRegistered() const;
  static bool IsEnabledModemState(ModemState state);

  void HandleNewSignalQuality(uint32_t strength);

  // Processes a change in the modem registration state, possibly creating,
  // destroying or updating the CellularService.
  void HandleNewRegistrationState();

  virtual void OnPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties);

  // Inherited from Device.
  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;
  void Stop(Error* error, const EnabledStateChangedCallback& callback) override;
  void LinkEvent(unsigned int flags, unsigned int change) override;
  void Scan(ScanType /*scan_type*/, Error* error,
            const std::string& /*reason*/) override;
  void RegisterOnNetwork(const std::string& network_id,
                         Error* error,
                         const ResultCallback& callback) override;
  void RequirePIN(const std::string& pin, bool require,
                  Error* error, const ResultCallback& callback) override;
  void EnterPIN(const std::string& pin,
                Error* error, const ResultCallback& callback) override;
  void UnblockPIN(const std::string& unblock_code,
                  const std::string& pin,
                  Error* error, const ResultCallback& callback) override;
  void ChangePIN(const std::string& old_pin,
                 const std::string& new_pin,
                 Error* error, const ResultCallback& callback) override;
  void Reset(Error* error, const ResultCallback& callback) override;
  void SetCarrier(const std::string& carrier,
                  Error* error, const ResultCallback& callback) override;
  bool IsIPv6Allowed() const override;
  void DropConnection() override;
  void SetServiceState(Service::ConnectState state) override;
  void SetServiceFailure(Service::ConnectFailure failure_state) override;
  void SetServiceFailureSilent(Service::ConnectFailure failure_state) override;
  void OnBeforeSuspend(const ResultCallback& callback) override;
  void OnAfterResume() override;

  void StartModemCallback(const EnabledStateChangedCallback& callback,
                          const Error& error);
  void StopModemCallback(const EnabledStateChangedCallback& callback,
                         const Error& error);
  void OnDisabled();
  void OnEnabled();
  void OnConnecting();
  void OnConnected() override;
  void OnConnectFailed(const Error& error);
  void OnDisconnected();
  void OnDisconnectFailed();
  std::string GetTechnologyFamily(Error* error);
  void OnModemStateChanged(ModemState new_state);
  void OnScanReply(const Stringmaps& found_networks, const Error& error);

  // accessor to read the allow roaming property
  bool allow_roaming_property() const { return allow_roaming_; }
  // Is the underlying device in the process of activating?
  bool IsActivating() const;

  // Initiate PPP link. Called from capabilities.
  virtual void StartPPP(const std::string& serial_device);
  // Callback for |ppp_task_|.
  virtual void OnPPPDied(pid_t pid, int exit);
  // Implements RPCTaskDelegate, for |ppp_task_|.
  void GetLogin(std::string* user, std::string* password) override;
  void Notify(const std::string& reason,
              const std::map<std::string, std::string>& dict) override;

  // ///////////////////////////////////////////////////////////////////////////
  // DBus Properties exposed by the Device interface of shill.
  void RegisterProperties();

  // getters
  const std::string& dbus_service() const { return dbus_service_; }
  const std::string& dbus_path() const { return dbus_path_; }
  const Stringmap& home_provider() const { return home_provider_; }
  const std::string& carrier() const { return carrier_; }
  bool scanning_supported() const { return scanning_supported_; }
  const std::string& esn() const { return esn_; }
  const std::string& firmware_revision() const { return firmware_revision_; }
  const std::string& hardware_revision() const { return hardware_revision_; }
  const std::string& imei() const { return imei_; }
  const std::string& imsi() const { return imsi_; }
  const std::string& mdn() const { return mdn_; }
  const std::string& meid() const { return meid_; }
  const std::string& min() const { return min_; }
  const std::string& manufacturer() const { return manufacturer_; }
  const std::string& model_id() const { return model_id_; }
  const std::string& mm_plugin() const { return mm_plugin_; }
  bool scanning() const { return scanning_; }

  const std::string& selected_network() const { return selected_network_; }
  const Stringmaps& found_networks() const { return found_networks_; }
  bool provider_requires_roaming() const { return provider_requires_roaming_; }
  bool sim_present() const { return sim_present_; }
  const Stringmaps& apn_list() const { return apn_list_; }
  const std::string& sim_identifier() const { return sim_identifier_; }

  const Strings& supported_carriers() const { return supported_carriers_; }
  uint16_t prl_version() const { return prl_version_; }

  // setters
  void set_home_provider(const Stringmap& home_provider);
  void set_carrier(const std::string& carrier);
  void set_scanning_supported(bool scanning_supported);
  void set_esn(const std::string& esn);
  void set_firmware_revision(const std::string& firmware_revision);
  void set_hardware_revision(const std::string& hardware_revision);
  void set_imei(const std::string& imei);
  void set_imsi(const std::string& imsi);
  void set_mdn(const std::string& mdn);
  void set_meid(const std::string& meid);
  void set_min(const std::string& min);
  void set_manufacturer(const std::string& manufacturer);
  void set_model_id(const std::string& model_id);
  void set_mm_plugin(const std::string& mm_plugin);
  void set_scanning(bool scanning);

  void set_selected_network(const std::string& selected_network);
  void clear_found_networks();
  void set_found_networks(const Stringmaps& found_networks);
  void set_provider_requires_roaming(bool provider_requires_roaming);
  void set_sim_present(bool sim_present);
  void set_apn_list(const Stringmaps& apn_list);
  void set_sim_identifier(const std::string& sim_identifier);

  void set_supported_carriers(const Strings& supported_carriers);
  void set_prl_version(uint16_t prl_version);

  // Takes ownership.
  void set_home_provider_info(MobileOperatorInfo* home_provider_info);
  // Takes ownership.
  void set_serving_operator_info(MobileOperatorInfo* serving_operator_info);

 private:
  friend class ActivePassiveOutOfCreditsDetectorTest;
  friend class CellularTest;
  friend class CellularCapabilityTest;
  friend class CellularCapabilityCDMATest;
  friend class CellularCapabilityGSMTest;
  friend class CellularCapabilityUniversalTest;
  friend class CellularCapabilityUniversalCDMATest;
  friend class CellularServiceTest;
  friend class ModemTest;
  friend class SubscriptionStateOutOfCreditsDetectorTest;
  FRIEND_TEST(CellularCapabilityCDMATest, GetRegistrationState);
  FRIEND_TEST(CellularCapabilityGSMTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityTest, EnableModemFail);
  FRIEND_TEST(CellularCapabilityTest, EnableModemSucceed);
  FRIEND_TEST(CellularCapabilityTest, FinishEnable);
  FRIEND_TEST(CellularCapabilityTest, GetModemInfo);
  FRIEND_TEST(CellularCapabilityTest, GetModemStatus);
  FRIEND_TEST(CellularCapabilityUniversalCDMATest, OnCDMARegistrationChanged);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, Connect);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, IsServiceActivationRequired);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, StartModemAlreadyEnabled);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, StopModemConnected);
  FRIEND_TEST(CellularCapabilityUniversalMainTest,
              UpdatePendingActivationState);
  FRIEND_TEST(CellularCapabilityUniversalMainTest,
              UpdateRegistrationState);
  FRIEND_TEST(CellularCapabilityUniversalMainTest,
              UpdateRegistrationStateModemNotConnected);
  FRIEND_TEST(CellularCapabilityUniversalMainTest, UpdateScanningProperty);
  FRIEND_TEST(CellularCapabilityUniversalMainTest,
              UpdateServiceActivationState);
  FRIEND_TEST(CellularTest, ChangeServiceState);
  FRIEND_TEST(CellularTest, ChangeServiceStatePPP);
  FRIEND_TEST(CellularTest, CreateService);
  FRIEND_TEST(CellularTest, Connect);
  FRIEND_TEST(CellularTest, ConnectFailure);
  FRIEND_TEST(CellularTest, ConnectFailureNoService);
  FRIEND_TEST(CellularTest, ConnectSuccessNoService);
  FRIEND_TEST(CellularTest, CustomSetterNoopChange);
  FRIEND_TEST(CellularTest, DisableModem);
  FRIEND_TEST(CellularTest, Disconnect);
  FRIEND_TEST(CellularTest, DisconnectFailure);
  FRIEND_TEST(CellularTest, DisconnectWithCallback);
  FRIEND_TEST(CellularTest, DropConnection);
  FRIEND_TEST(CellularTest, DropConnectionPPP);
  FRIEND_TEST(CellularTest, EnableTrafficMonitor);
  FRIEND_TEST(CellularTest, EstablishLinkDHCP);
  FRIEND_TEST(CellularTest, EstablishLinkPPP);
  FRIEND_TEST(CellularTest, EstablishLinkStatic);
  FRIEND_TEST(CellularTest, FriendlyServiceName);
  FRIEND_TEST(CellularTest,
              HandleNewRegistrationStateForServiceRequiringActivation);
  FRIEND_TEST(CellularTest, HomeProviderServingOperator);
  FRIEND_TEST(CellularTest, LinkEventUpWithPPP);
  FRIEND_TEST(CellularTest, LinkEventUpWithoutPPP);
  FRIEND_TEST(CellularTest, LinkEventWontDestroyService);
  FRIEND_TEST(CellularTest, ModemStateChangeDisable);
  FRIEND_TEST(CellularTest, ModemStateChangeEnable);
  FRIEND_TEST(CellularTest, ModemStateChangeStaleConnected);
  FRIEND_TEST(CellularTest, ModemStateChangeValidConnected);
  FRIEND_TEST(CellularTest, Notify);
  FRIEND_TEST(CellularTest, OnAfterResumeDisableInProgressWantDisabled);
  FRIEND_TEST(CellularTest, OnAfterResumeDisableQueuedWantEnabled);
  FRIEND_TEST(CellularTest, OnAfterResumeDisabledWantDisabled);
  FRIEND_TEST(CellularTest, OnAfterResumeDisabledWantEnabled);
  FRIEND_TEST(CellularTest, OnAfterResumePowerDownInProgressWantEnabled);
  FRIEND_TEST(CellularTest, OnConnectionHealthCheckerResult);
  FRIEND_TEST(CellularTest, OnPPPDied);
  FRIEND_TEST(CellularTest, PPPConnectionFailedAfterAuth);
  FRIEND_TEST(CellularTest, PPPConnectionFailedBeforeAuth);
  FRIEND_TEST(CellularTest, PPPConnectionFailedDuringAuth);
  FRIEND_TEST(CellularTest, ScanAsynchronousFailure);
  FRIEND_TEST(CellularTest, ScanImmediateFailure);
  FRIEND_TEST(CellularTest, ScanSuccess);
  FRIEND_TEST(CellularTest, SetAllowRoaming);
  FRIEND_TEST(CellularTest, StartModemCallback);
  FRIEND_TEST(CellularTest, StartModemCallbackFail);
  FRIEND_TEST(CellularTest, StopModemCallback);
  FRIEND_TEST(CellularTest, StopModemCallbackFail);
  FRIEND_TEST(CellularTest, StopPPPOnDisconnect);
  FRIEND_TEST(CellularTest, StopPPPOnTermination);
  FRIEND_TEST(CellularTest, StorageIdentifier);
  FRIEND_TEST(CellularTest, StartConnected);
  FRIEND_TEST(CellularTest, StartCDMARegister);
  FRIEND_TEST(CellularTest, StartGSMRegister);
  FRIEND_TEST(CellularTest, StartLinked);
  FRIEND_TEST(CellularTest, StartPPP);
  FRIEND_TEST(CellularTest, StartPPPAfterEthernetUp);
  FRIEND_TEST(CellularTest, StartPPPAlreadyStarted);
  FRIEND_TEST(CellularTest, UpdateScanning);
  FRIEND_TEST(Modem1Test, CreateDeviceMM1);

  class MobileOperatorInfoObserver : public MobileOperatorInfo::Observer {
   public:
    // |cellular| must have lifespan longer than this object. In practice this
    // is enforced because |cellular| owns this object.
    explicit MobileOperatorInfoObserver(Cellular* cellular);
    ~MobileOperatorInfoObserver() override;

    void set_capability(CellularCapability* capability) {
      capability_ = capability;
    }

    // Inherited from MobileOperatorInfo::Observer
    void OnOperatorChanged() override;

   private:
    Cellular* const cellular_;
    // Owned by |Cellular|.
    CellularCapability* capability_;

    DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoObserver);
  };

  // Names of properties in storage
  static const char kAllowRoaming[];

  // the |kScanningProperty| exposed by Cellular device is sticky false. Every
  // time it is set to true, it must be reset to false after a time equal to
  // this constant.
  static const int64_t kDefaultScanningTimeoutMilliseconds;

  // Generic service name prefix, shown when the correct carrier name is
  // unknown.
  static const char kGenericServiceNamePrefix[];
  static unsigned int friendly_service_name_id_;

  void SetState(State state);

  // Invoked when the modem is connected to the cellular network to transition
  // to the network-connected state and bring the network interface up.
  void EstablishLink();

  void InitCapability(Type type);

  void CreateService();

  // HelpRegisterDerived*: Expose a property over RPC, with the name |name|.
  //
  // Reads of the property will be handled by invoking |get|.
  // Writes to the property will be handled by invoking |set|.
  // Clearing the property will be handled by PropertyStore.
  void HelpRegisterDerivedBool(
      const std::string& name,
      bool(Cellular::*get)(Error* error),
      bool(Cellular::*set)(const bool& value, Error* error));
  void HelpRegisterConstDerivedString(
      const std::string& name,
      std::string(Cellular::*get)(Error* error));

  void OnConnectReply(const Error& error);
  void OnDisconnectReply(const Error& error);

  // DBUS accessors to read/modify the allow roaming property
  bool GetAllowRoaming(Error* /*error*/) { return allow_roaming_; }
  bool SetAllowRoaming(const bool& value, Error* error);

  // When shill terminates or ChromeOS suspends, this function is called to
  // disconnect from the cellular network.
  void StartTermination();

  // This method is invoked upon the completion of StartTermination().
  void OnTerminationCompleted(const Error& error);

  // This function does the final cleanup once a disconnect request terminates.
  // Returns true, if the device state is successfully changed.
  bool DisconnectCleanup();

  // Executed after the asynchronous CellularCapability::StartModem
  // call from OnAfterResume completes.
  static void LogRestartModemResult(const Error& error);

  // Terminate the pppd process associated with this Device, and remove the
  // association between the PPPDevice and our CellularService. If this
  // Device is not using PPP, the method has no effect.
  void StopPPP();

  // Handlers for PPP events. Dispatched from Notify().
  void OnPPPAuthenticated();
  void OnPPPAuthenticating();
  void OnPPPConnected(const std::map<std::string, std::string>& params);
  void OnPPPDisconnected();

  void UpdateScanning();

  base::WeakPtrFactory<Cellular> weak_ptr_factory_;

  State state_;
  ModemState modem_state_;

  std::unique_ptr<CellularCapability> capability_;

  // Operator info objects. These objects receive updates as we receive
  // information about the network operators from the SIM or OTA. In turn, they
  // send out updates through their observer interfaces whenever the identity of
  // the network operator changes, or any other property of the operator
  // changes.
  std::unique_ptr<MobileOperatorInfo> home_provider_info_;
  std::unique_ptr<MobileOperatorInfo> serving_operator_info_;
  // Observer object to listen to updates from the operator info objects.
  std::unique_ptr<MobileOperatorInfoObserver> mobile_operator_info_observer_;

  // ///////////////////////////////////////////////////////////////////////////
  // All DBus Properties exposed by the Cellular device.
  // Properties common to GSM and CDMA modems.
  const std::string dbus_service_;  // org.*.ModemManager*
  const std::string dbus_path_;  // ModemManager.Modem
  Stringmap home_provider_;

  bool scanning_supported_;
  std::string carrier_;
  std::string esn_;
  std::string firmware_revision_;
  std::string hardware_revision_;
  std::string imei_;
  std::string imsi_;
  std::string manufacturer_;
  std::string mdn_;
  std::string meid_;
  std::string min_;
  std::string model_id_;
  std::string mm_plugin_;
  bool scanning_;

  // GSM only properties.
  // They are always exposed but are non empty only for GSM technology modems.
  std::string selected_network_;
  Stringmaps found_networks_;
  bool provider_requires_roaming_;
  uint16_t scan_interval_;
  bool sim_present_;
  Stringmaps apn_list_;
  std::string sim_identifier_;

  // CDMA only properties.
  uint16_t prl_version_;

  // This property is specific to Gobi modems.
  Strings supported_carriers_;
  // End of DBus properties.
  // ///////////////////////////////////////////////////////////////////////////

  ModemInfo* modem_info_;
  const Type type_;
  PPPDeviceFactory* ppp_device_factory_;

  ProcessManager* process_manager_;

  CellularServiceRefPtr service_;

  // User preference to allow or disallow roaming
  bool allow_roaming_;

  // Track whether a user initiated scan is in prgoress (initiated via ::Scan)
  bool proposed_scan_in_progress_;

  // Flag indicating that a disconnect has been explicitly requested.
  bool explicit_disconnect_;

  std::unique_ptr<ExternalTask> ppp_task_;
  PPPDeviceRefPtr ppp_device_;
  bool is_ppp_authenticating_;

  // Sometimes modems may be stuck in the SEARCHING state during the lack of
  // presence of a network. During this indefinite duration of time, keeping
  // the Device.Scanning property as |true| causes a bad user experience.
  // This callback sets it to |false| after a timeout period has passed.
  base::CancelableClosure scanning_timeout_callback_;
  int64_t scanning_timeout_milliseconds_;

  DISALLOW_COPY_AND_ASSIGN(Cellular);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_CELLULAR_H_
