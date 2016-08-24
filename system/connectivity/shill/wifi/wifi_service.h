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

#ifndef SHILL_WIFI_WIFI_SERVICE_H_
#define SHILL_WIFI_WIFI_SERVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "shill/event_dispatcher.h"
#include "shill/key_value_store.h"
#include "shill/refptr_types.h"
#include "shill/service.h"

namespace shill {

class CertificateFile;
class ControlInterface;
class EventDispatcher;
class Error;
class Manager;
class Metrics;
class WiFiProvider;

class WiFiService : public Service {
 public:
  // TODO(pstew): Storage constants shouldn't need to be public
  // crbug.com/208736
  static const char kStorageHiddenSSID[];
  static const char kStorageMode[];
  static const char kStoragePassphrase[];
  static const char kStorageSecurity[];
  static const char kStorageSecurityClass[];
  static const char kStorageSSID[];
  static const char kStoragePreferredDevice[];
  static const char kStorageRoamThreshold[];
  static const char kStorageRoamThresholdSet[];

  WiFiService(ControlInterface* control_interface,
              EventDispatcher* dispatcher,
              Metrics* metrics,
              Manager* manager,
              WiFiProvider* provider,
              const std::vector<uint8_t>& ssid,
              const std::string& mode,
              const std::string& security,
              bool hidden_ssid);
  ~WiFiService();

  // Inherited from Service.
  void Connect(Error* error, const char* reason) override;
  void Disconnect(Error* error, const char* reason) override;
  bool Is8021x() const override;

  virtual void AddEndpoint(const WiFiEndpointConstRefPtr& endpoint);
  virtual void RemoveEndpoint(const WiFiEndpointConstRefPtr& endpoint);
  virtual int GetEndpointCount() const { return endpoints_.size(); }

  // Called to update the identity of the currently connected endpoint.
  // To indicate that there is no currently connect endpoint, call with
  // |endpoint| set to nullptr.
  virtual void NotifyCurrentEndpoint(const WiFiEndpointConstRefPtr& endpoint);
  // Called to inform of changes in the properties of an endpoint.
  // (Not necessarily the currently connected endpoint.)
  virtual void NotifyEndpointUpdated(const WiFiEndpointConstRefPtr& endpoint);

  // wifi_<MAC>_<BSSID>_<mode_string>_<security_string>
  std::string GetStorageIdentifier() const override;
  static bool ParseStorageIdentifier(const std::string& storage_name,
                                     std::string* address,
                                     std::string* mode,
                                     std::string* security);

  // Iterate over |storage| looking for WiFi servces with "old-style"
  // properties that don't include explicit type/mode/security, and add
  // these properties.  Returns true if any entries were fixed.
  static bool FixupServiceEntries(StoreInterface* storage);

  // Validate |mode| against all valid and supported service modes.
  static bool IsValidMode(const std::string& mode);

  // Validate |method| against all valid and supported security methods.
  static bool IsValidSecurityMethod(const std::string& method);

  // Validate |security_class| against all valid and supported
  // security classes.
  static bool IsValidSecurityClass(const std::string& security_class);

  const std::string& mode() const { return mode_; }
  const std::string& key_management() const { return GetEAPKeyManagement(); }
  const std::vector<uint8_t>& ssid() const { return ssid_; }
  const std::string& bssid() const { return bssid_; }
  const std::vector<uint16_t>& frequency_list() const {
    return frequency_list_;
  }
  uint16_t physical_mode() const { return physical_mode_; }
  uint16_t frequency() const { return frequency_; }

  // WiFi services can load from profile entries other than their current
  // storage identifier.  Override the methods from the parent Service
  // class which pertain to whether this service may be loaded from |storage|.
  std::string GetLoadableStorageIdentifier(
      const StoreInterface& storage) const override;
  bool IsLoadableFrom(const StoreInterface& storage) const override;

  // Override Load and Save from parent Service class.  We will call
  // the parent method.
  bool Load(StoreInterface* storage) override;
  bool Save(StoreInterface* storage) override;
  bool Unload() override;

  // Override SetState from parent Service class.  We will call the
  // parent method.
  void SetState(ConnectState state) override;

  virtual bool HasEndpoints() const { return !endpoints_.empty(); }
  bool IsVisible() const override;
  bool IsSecurityMatch(const std::string& security) const;

  // Used by WiFi objects to indicate that the credentials for this network
  // have been called into question.  This method returns true if given this
  // suspicion, if it is probable that indeed these credentials are likely
  // to be incorrect.  Credentials that have never been used before are
  // considered suspect by default, while those which have been used
  // successfully in the past must have this method called a number of times
  // since the last time ResetSuspectedCredentialsFailures() was called.
  virtual bool AddSuspectedCredentialFailure();
  virtual void ResetSuspectedCredentialFailures();

  bool hidden_ssid() const { return hidden_ssid_; }
  bool ieee80211w_required() const { return ieee80211w_required_; }

  void InitializeCustomMetrics() const;
  void SendPostReadyStateMetrics(
      int64_t time_resume_to_ready_milliseconds) const override;

  // Clear any cached credentials stored in wpa_supplicant related to |this|.
  // This will disconnect this service if it is currently connected.
  void ClearCachedCredentials();

  // Override from parent Service class to correctly update connectability
  // when the EAP credentials change for 802.1x networks.
  void OnEapCredentialsChanged(
      Service::UpdateCredentialsReason reason) override;

  // Called by WiFiService to reset state associated with prior success
  // of a connection with particular EAP credentials or a passphrase.
  void OnCredentialChange(Service::UpdateCredentialsReason reason);

  // Override from parent Service class to register hidden services once they
  // have been configured.
  void OnProfileConfigured() override;

  // Called by WiFiProvider to reset the WiFi device reference on shutdown.
  virtual void ResetWiFi();

  // Called by WiFi to retrieve configuration parameters for wpa_supplicant.
  virtual KeyValueStore GetSupplicantConfigurationParameters() const;

  // "wpa", "rsn" and "psk" are equivalent from a configuration perspective.
  // This function maps them all into "psk".
  static std::string ComputeSecurityClass(const std::string& security);

  bool IsAutoConnectable(const char** reason) const override;

  // Signal level in dBm.  If no current endpoint, returns
  // std::numeric_limits<int>::min().
  int16_t SignalLevel() const;

  void set_expecting_disconnect(bool val) { expecting_disconnect_ = val; }
  bool expecting_disconnect() const { return expecting_disconnect_; }

  uint16_t roam_threshold_db() { return roam_threshold_db_; }
  bool roam_threshold_db_set() { return roam_threshold_db_set_; }

 protected:
  void SetEAPKeyManagement(const std::string& key_management) override;
  std::string GetTethering(Error* error) const override;

 private:
  friend class WiFiServiceSecurityTest;
  friend class WiFiServiceTest;  // SetPassphrase
  friend class WiFiServiceUpdateFromEndpointsTest;  // SignalToStrength
  FRIEND_TEST(MetricsTest, WiFiServicePostReady);
  FRIEND_TEST(MetricsTest, WiFiServicePostReadyAdHoc);
  FRIEND_TEST(MetricsTest, WiFiServicePostReadyEAP);
  FRIEND_TEST(WiFiMainTest, CurrentBSSChangedUpdateServiceEndpoint);
  FRIEND_TEST(WiFiMainTest, RoamThresholdProperty);
  FRIEND_TEST(WiFiProviderTest, OnEndpointAddedWithSecurity);  // security_
  FRIEND_TEST(WiFiServiceTest, AutoConnect);
  FRIEND_TEST(WiFiServiceTest, ClearWriteOnlyDerivedProperty);  // passphrase_
  FRIEND_TEST(WiFiServiceTest, ComputeCipher8021x);
  FRIEND_TEST(WiFiServiceTest, ConnectTask8021x);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskDynamicWEP);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskPSK);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskRSN);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskWEP);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskWPA);
  FRIEND_TEST(WiFiServiceTest, ConnectTaskWPA80211w);
  FRIEND_TEST(WiFiServiceTest, GetTethering);
  FRIEND_TEST(WiFiServiceTest, IsAutoConnectable);
  FRIEND_TEST(WiFiServiceTest, LoadHidden);
  FRIEND_TEST(WiFiServiceTest, SetPassphraseForNonPassphraseService);
  FRIEND_TEST(WiFiServiceTest, LoadAndUnloadPassphrase);
  FRIEND_TEST(WiFiServiceTest, LoadPassphraseClearCredentials);
  FRIEND_TEST(WiFiServiceTest, SecurityFromCurrentEndpoint);  // GetSecurity
  FRIEND_TEST(WiFiServiceTest, SetPassphraseResetHasEverConnected);
  FRIEND_TEST(WiFiServiceTest, SetPassphraseRemovesCachedCredentials);
  FRIEND_TEST(WiFiServiceTest, SignalToStrength);  // SignalToStrength
  FRIEND_TEST(WiFiServiceTest, SuspectedCredentialFailure);
  FRIEND_TEST(WiFiServiceTest, UpdateSecurity);  // SetEAPKeyManagement
  FRIEND_TEST(WiFiServiceTest, ConnectWithPreferredDevice);
  FRIEND_TEST(WiFiServiceTest, ConfigurePreferredDevice);
  FRIEND_TEST(WiFiServiceTest, LoadAndUnloadPreferredDevice);
  FRIEND_TEST(WiFiServiceTest, ChooseDevice);
  FRIEND_TEST(WiFiServiceUpdateFromEndpointsTest,
              AddEndpointWithPreferredDevice);
  FRIEND_TEST(WiFiServiceTest, SaveLoadRoamThreshold);

  static const char kAutoConnNoEndpoint[];
  static const char kAnyDeviceAddress[];
  static const int kSuspectedCredentialFailureThreshold;

  // Override the base clase implementation, because we need to allow
  // arguments that aren't base class methods.
  void HelpRegisterConstDerivedString(
      const std::string& name,
      std::string(WiFiService::*get)(Error* error));
  void HelpRegisterDerivedString(
      const std::string& name,
      std::string(WiFiService::*get)(Error* error),
      bool(WiFiService::*set)(const std::string& value, Error* error));
  void HelpRegisterWriteOnlyDerivedString(
      const std::string& name,
      bool(WiFiService::*set)(const std::string& value, Error* error),
      void(WiFiService::*clear)(Error* error),
      const std::string* default_value);
  void HelpRegisterDerivedUint16(
      const std::string& name,
      uint16_t(WiFiService::*get)(Error* error),
      bool(WiFiService::*set)(const uint16_t& value, Error* error),
      void(WiFiService::*clear)(Error* error));

  std::string GetDeviceRpcId(Error* error) const override;

  void ClearPassphrase(Error* error);
  void UpdateConnectable();
  void UpdateFromEndpoints();
  void UpdateSecurity();

  static CryptoAlgorithm ComputeCipher8021x(
      const std::set<WiFiEndpointConstRefPtr>& endpoints);
  static void ValidateWEPPassphrase(const std::string& passphrase,
                                    Error* error);
  static void ValidateWPAPassphrase(const std::string& passphrase,
                                    Error* error);
  static void ParseWEPPassphrase(const std::string& passphrase,
                                 int* key_index,
                                 std::vector<uint8_t>* password_bytes,
                                 Error* error);
  static bool CheckWEPIsHex(const std::string& passphrase, Error* error);
  static bool CheckWEPKeyIndex(const std::string& passphrase, Error* error);
  static bool CheckWEPPrefix(const std::string& passphrase, Error* error);

  // Maps a signal value, in dBm, to a "strength" value, from
  // |Service::kStrengthMin| to |Service:kStrengthMax|.
  static uint8_t SignalToStrength(int16_t signal_dbm);

  // Create a default group name for this WiFi service.
  std::string GetDefaultStorageIdentifier() const;

  // Return the security of this service.  If connected, the security
  // reported from the currently connected endpoint is returned.  Otherwise
  // the configured security for the service is returned.
  std::string GetSecurity(Error* error);

  // Return the security class of this service.  If connected, the
  // security class of the currently connected endpoint is returned.
  // Otherwise the configured security class for the service is
  // returned.
  //
  // See also: ComputeSecurityClass.
  std::string GetSecurityClass(Error* error);

  // Profile data for a WPA/RSN service can be stored under a number of
  // different security types.  These functions create different storage
  // property lists based on whether they are saved with their generic
  // "psk" name or if they use the (legacy) specific "wpa" or "rsn" names.
  KeyValueStore GetStorageProperties() const;

  // Called from DBus and during Load to validate and apply a passphrase for
  // this service.  If the passphrase is successfully changed, UpdateConnectable
  // and OnCredentialChange are both called and the method returns true.  This
  // method will return false if the passphrase cannot be set.  If the
  // passphrase is already set to the value of |passphrase|, this method will
  // return false.  If it is due to an error, |error| will be populated with the
  // appropriate information.
  bool SetPassphrase(const std::string& passphrase, Error* error);

  // Called by SetPassphrase and LoadPassphrase to perform the check on a
  // passphrase change.  |passphrase| is the new passphrase to be used for the
  // service.  If the new passphrase is not different from the existing
  // passphrase, SetPassphraseInternal will return false.  |reason| signals how
  // the SetPassphraseInternal method was triggered.  If the method was called
  // from Load, the has_ever_connected flag will not be reset.  If the method
  // was called from SetPassphrase, has_ever_connected will be set to false.
  bool SetPassphraseInternal(const std::string& passphrase,
                             Service::UpdateCredentialsReason reason);

  // Select a WiFi device (e.g, for connecting a hidden service with no
  // endpoints).
  WiFiRefPtr ChooseDevice();

  std::string GetPreferredDevice(Error* error);
  // Called from DBus and during load to apply the preferred device for this
  // service.
  bool SetPreferredDevice(const std::string& device_name, Error* error);

  void SetWiFi(const WiFiRefPtr& new_wifi);

  // This method can't be 'const' because it is passed to
  // HelpRegisterDerivedUint16, which doesn't take const methods.
  uint16_t GetRoamThreshold(Error* error) /*const*/;
  bool SetRoamThreshold(const uint16_t& threshold, Error* error);
  void ClearRoamThreshold(Error* error);

  // Properties
  std::string passphrase_;
  bool need_passphrase_;
  const std::string security_;
  // TODO(cmasone): see if the below can be pulled from the endpoint associated
  // with this service instead.
  const std::string mode_;
  std::string auth_mode_;
  bool hidden_ssid_;
  uint16_t frequency_;
  std::vector<uint16_t> frequency_list_;
  uint16_t physical_mode_;
  // Preferred device to use for connecting to this service.
  std::string preferred_device_;
  // The raw dBm signal strength from the associated endpoint.
  int16_t raw_signal_strength_;
  std::string hex_ssid_;
  std::string storage_identifier_;
  std::string bssid_;
  Stringmap vendor_information_;
  // The country code reported by the current endpoint.
  std::string country_code_;
  // If |security_| == kSecurity8021x, the crypto algorithm being used.
  // (Otherwise, crypto algorithm is implied by |security_|.)
  CryptoAlgorithm cipher_8021x_;

  // Track the number of consecutive times our current credentials have
  // been called into question.
  int suspected_credential_failures_;

  // Track whether or not we've warned about large signal values.
  // Used to avoid spamming the log.
  static bool logged_signal_warning;

  WiFiRefPtr wifi_;
  std::set<WiFiEndpointConstRefPtr> endpoints_;
  WiFiEndpointConstRefPtr current_endpoint_;
  const std::vector<uint8_t> ssid_;
  // Track whether IEEE 802.11w (Protected Management Frame) support is
  // mandated by one or more endpoints we have seen that provide this service.
  bool ieee80211w_required_;
  // Flag indicating if service disconnect is initiated by user for
  // connecting to other service.
  bool expecting_disconnect_;
  std::unique_ptr<CertificateFile> certificate_file_;
  uint16_t roam_threshold_db_;
  bool roam_threshold_db_set_;
  // Bare pointer is safe because WiFi service instances are owned by
  // the WiFiProvider and are guaranteed to be deallocated by the time
  // the WiFiProvider is.
  WiFiProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(WiFiService);
};

}  // namespace shill

#endif  // SHILL_WIFI_WIFI_SERVICE_H_
