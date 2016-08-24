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

#ifndef SHILL_CELLULAR_CELLULAR_CAPABILITY_GSM_H_
#define SHILL_CELLULAR_CELLULAR_CAPABILITY_GSM_H_

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/accessor_interface.h"
#include "shill/cellular/cellular.h"
#include "shill/cellular/cellular_capability.h"
#include "shill/cellular/cellular_capability_classic.h"
#include "shill/cellular/modem_gsm_card_proxy_interface.h"
#include "shill/cellular/modem_gsm_network_proxy_interface.h"

struct mobile_provider;

namespace shill {

class ModemInfo;

class CellularCapabilityGSM : public CellularCapabilityClassic {
 public:
  CellularCapabilityGSM(Cellular* cellular,
                        ControlInterface* control_interface,
                        ModemInfo* modem_info);
  ~CellularCapabilityGSM() override;

  // Inherited from CellularCapability.
  std::string GetTypeString() const override;
  void OnPropertiesChanged(
      const std::string& interface,
      const KeyValueStore& changed_properties,
      const std::vector<std::string>& invalidated_properties) override;
  void StartModem(Error* error, const ResultCallback& callback) override;
  bool AreProxiesInitialized() const override;
  void Scan(Error* error, const ResultStringmapsCallback& callback) override;
  void RegisterOnNetwork(const std::string& network_id,
                         Error* error,
                         const ResultCallback& callback) override;
  bool IsRegistered() const override;
  void SetUnregistered(bool searching) override;
  void OnServiceCreated() override;
  std::string GetNetworkTechnologyString() const override;
  std::string GetRoamingStateString() const override;
  bool AllowRoaming() override;
  void GetSignalQuality() override;
  void SetupConnectProperties(KeyValueStore* properties) override;
  void Connect(const KeyValueStore& properties,
               Error* error,
               const ResultCallback& callback) override;
  void RequirePIN(const std::string& pin,
                  bool require,
                  Error* error,
                  const ResultCallback& callback) override;
  void EnterPIN(const std::string& pin,
                Error* error,
                const ResultCallback& callback) override;
  void UnblockPIN(const std::string& unblock_code,
                  const std::string& pin,
                  Error* error,
                  const ResultCallback& callback) override;
  void ChangePIN(const std::string& old_pin,
                 const std::string& new_pin,
                 Error* error,
                 const ResultCallback& callback) override;

  // Inherited from CellularCapabilityClassic.
  void GetRegistrationState() override;
  // The following six methods are only ever called as callbacks (from the main
  // loop), which is why they don't take an Error* argument.
  void GetProperties(const ResultCallback& callback) override;

  virtual void GetIMEI(const ResultCallback& callback);
  virtual void GetIMSI(const ResultCallback& callback);
  virtual void GetSPN(const ResultCallback& callback);
  virtual void GetMSISDN(const ResultCallback& callback);
  virtual void Register(const ResultCallback& callback);

 protected:
  // Inherited from CellularCapabilityClassic.
  void InitProxies() override;
  void ReleaseProxies() override;

  // Initializes properties, such as IMSI, which are required before the device
  // is enabled.
  virtual void InitProperties();

 private:
  friend class CellularTest;
  friend class CellularCapabilityGSMTest;
  friend class CellularCapabilityTest;
  FRIEND_TEST(CellularCapabilityGSMTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityGSMTest, CreateDeviceFromProperties);
  FRIEND_TEST(CellularCapabilityGSMTest, GetIMEI);
  FRIEND_TEST(CellularCapabilityGSMTest, GetIMSI);
  FRIEND_TEST(CellularCapabilityGSMTest, GetIMSIFails);
  FRIEND_TEST(CellularCapabilityGSMTest, GetMSISDN);
  FRIEND_TEST(CellularCapabilityGSMTest, GetSPN);
  FRIEND_TEST(CellularCapabilityGSMTest, RequirePIN);
  FRIEND_TEST(CellularCapabilityGSMTest, EnterPIN);
  FRIEND_TEST(CellularCapabilityGSMTest, UnblockPIN);
  FRIEND_TEST(CellularCapabilityGSMTest, ChangePIN);
  FRIEND_TEST(CellularCapabilityGSMTest, ParseScanResult);
  FRIEND_TEST(CellularCapabilityGSMTest, ParseScanResultProviderLookup);
  FRIEND_TEST(CellularCapabilityGSMTest, RegisterOnNetwork);
  FRIEND_TEST(CellularCapabilityGSMTest, SetAccessTechnology);
  FRIEND_TEST(CellularCapabilityGSMTest, GetRegistrationState);
  FRIEND_TEST(CellularCapabilityGSMTest, OnPropertiesChanged);
  FRIEND_TEST(CellularCapabilityTest, AllowRoaming);
  FRIEND_TEST(CellularCapabilityTest, TryApns);
  FRIEND_TEST(CellularTest, ScanAsynchronousFailure);
  FRIEND_TEST(CellularTest, ScanImmediateFailure);
  FRIEND_TEST(CellularTest, ScanSuccess);
  FRIEND_TEST(CellularTest, StartGSMRegister);
  FRIEND_TEST(ModemTest, CreateDeviceFromProperties);

  // SimLockStatus represents the fields in the Cellular.SIMLockStatus
  // DBUS property of the shill device.
  struct SimLockStatus {
   public:
    SimLockStatus() : enabled(false), retries_left(0) {}

    bool enabled;
    std::string lock_type;
    uint32_t retries_left;
  };

  static const char kNetworkPropertyAccessTechnology[];
  static const char kNetworkPropertyID[];
  static const char kNetworkPropertyLongName[];
  static const char kNetworkPropertyShortName[];
  static const char kNetworkPropertyStatus[];
  static const char kPhoneNumber[];
  static const char kPropertyAccessTechnology[];
  static const char kPropertyEnabledFacilityLocks[];
  static const char kPropertyUnlockRequired[];
  static const char kPropertyUnlockRetries[];

  // Calls to the proxy's GetIMSI() will be retried this many times.
  static const int kGetIMSIRetryLimit;

  // This much time will pass between retries of GetIMSI().
  static const int64_t kGetIMSIRetryDelayMilliseconds;

  void SetAccessTechnology(uint32_t access_technology);

  Stringmap ParseScanResult(const GSMScanResult& result);

  KeyValueStore SimLockStatusToProperty(Error* error);

  void SetupApnTryList();
  void FillConnectPropertyMap(KeyValueStore* properties);

  void HelpRegisterConstDerivedKeyValueStore(
      const std::string& name,
      KeyValueStore(CellularCapabilityGSM::*get)(Error* error));

  bool IsUnderlyingDeviceRegistered() const;

  // Signal callbacks
  void OnNetworkModeSignal(uint32_t mode);
  void OnRegistrationInfoSignal(uint32_t status,
                                const std::string& operator_code,
                                const std::string& operator_name);
  void OnSignalQualitySignal(uint32_t quality);

  // Method callbacks
  void OnGetRegistrationInfoReply(uint32_t status,
                                  const std::string& operator_code,
                                  const std::string& operator_name,
                                  const Error& error);
  void OnGetSignalQualityReply(uint32_t quality, const Error& error);
  void OnRegisterReply(const ResultCallback& callback,
                       const Error& error);
  void OnGetIMEIReply(const ResultCallback& callback,
                      const std::string& imei,
                      const Error& error);
  void OnGetIMSIReply(const ResultCallback& callback,
                      const std::string& imsi,
                      const Error& error);
  void OnGetSPNReply(const ResultCallback& callback,
                     const std::string& spn,
                     const Error& error);
  void OnGetMSISDNReply(const ResultCallback& callback,
                        const std::string& msisdn,
                        const Error& error);
  void OnScanReply(const ResultStringmapsCallback& callback,
                   const GSMScanResults& results,
                   const Error& error);
  void OnConnectReply(const ResultCallback& callback, const Error& error);

  std::unique_ptr<ModemGSMCardProxyInterface> card_proxy_;
  std::unique_ptr<ModemGSMNetworkProxyInterface> network_proxy_;
  base::WeakPtrFactory<CellularCapabilityGSM> weak_ptr_factory_;
  // Used to enrich information about the network operator in |ParseScanResult|.
  // TODO(pprabhu) Instead instantiate a local |MobileOperatorInfo| instance
  // once the context has been separated out. (crbug.com/363874)
  std::unique_ptr<MobileOperatorInfo> mobile_operator_info_;

  uint32_t registration_state_;
  uint32_t access_technology_;
  std::string spn_;
  mobile_provider* home_provider_info_;
  std::string desired_network_;

  // The number of times GetIMSI() has been retried.
  int get_imsi_retries_;

  // Amount of time to wait between retries of GetIMSI.  Defaults to
  // kGetIMSIRetryDelayMilliseconds, but can be altered by a unit test.
  int64_t get_imsi_retry_delay_milliseconds_;

  // Properties.
  std::deque<Stringmap> apn_try_list_;
  SimLockStatus sim_lock_status_;

  DISALLOW_COPY_AND_ASSIGN(CellularCapabilityGSM);
};

}  // namespace shill

#endif  // SHILL_CELLULAR_CELLULAR_CAPABILITY_GSM_H_
