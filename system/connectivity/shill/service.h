//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_SERVICE_H_
#define SHILL_SERVICE_H_

#include <time.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/cancelable_callback.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/accessor_interface.h"
#include "shill/adaptor_interfaces.h"
#include "shill/callbacks.h"
#include "shill/dhcp_properties.h"
#include "shill/net/event_history.h"
#include "shill/net/shill_time.h"
#include "shill/property_store.h"
#include "shill/refptr_types.h"
#include "shill/static_ip_parameters.h"
#include "shill/technology.h"

namespace chromeos_metrics {
class Timer;
}

namespace shill {

class ControlInterface;
class DhcpProperties;
class DiagnosticsReporter;
class Endpoint;
class Error;
class EventDispatcher;
class HTTPProxy;
class KeyValueStore;
class Manager;
class Metrics;
class MockManager;
class ServiceAdaptorInterface;
class ServiceMockAdaptor;
class ServicePropertyChangeNotifier;
class Sockets;
class StoreInterface;

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
class EapCredentials;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

// A Service is a uniquely named entity, which the system can
// connect in order to begin sending and receiving network traffic.
// All Services are bound to an Entry, which represents the persistable
// state of the Service.  If the Entry is populated at the time of Service
// creation, that information is used to prime the Service.  If not, the Entry
// becomes populated over time.
class Service : public base::RefCounted<Service> {
 public:
  static const char kCheckPortalAuto[];
  static const char kCheckPortalFalse[];
  static const char kCheckPortalTrue[];

  static const char kErrorDetailsNone[];

  // TODO(pstew): Storage constants shouldn't need to be public
  // crbug.com/208736
  static const char kStorageAutoConnect[];
  static const char kStorageCheckPortal[];
  static const char kStorageDNSAutoFallback[];
  static const char kStorageError[];
  static const char kStorageFavorite[];
  static const char kStorageGUID[];
  static const char kStorageHasEverConnected[];
  static const char kStorageName[];
  static const char kStoragePriority[];
  static const char kStoragePriorityWithinTechnology[];
  static const char kStorageProxyConfig[];
  static const char kStorageSaveCredentials[];
  static const char kStorageType[];
  static const char kStorageUIData[];
  static const char kStorageConnectionId[];
  static const char kStorageLinkMonitorDisabled[];
  static const char kStorageManagedCredentials[];

  static const uint8_t kStrengthMax;
  static const uint8_t kStrengthMin;

  enum ConnectFailure {
    kFailureUnknown,
    kFailureAAA,
    kFailureActivation,
    kFailureBadPassphrase,
    kFailureBadWEPKey,
    kFailureConnect,
    kFailureDHCP,
    kFailureDNSLookup,
    kFailureEAPAuthentication,
    kFailureEAPLocalTLS,
    kFailureEAPRemoteTLS,
    kFailureHTTPGet,
    kFailureIPSecCertAuth,
    kFailureIPSecPSKAuth,
    kFailureInternal,
    kFailureNeedEVDO,
    kFailureNeedHomeNetwork,
    kFailureOTASP,
    kFailureOutOfRange,
    kFailurePPPAuth,
    kFailurePinMissing,
    kFailureMax
  };
  enum ConnectState {
    kStateUnknown,
    kStateIdle,
    kStateAssociating,
    kStateConfiguring,
    kStateConnected,
    kStatePortal,
    kStateFailure,
    kStateOnline
  };
  enum CryptoAlgorithm {
    kCryptoNone,
    kCryptoRc4,
    kCryptoAes
  };

  enum UpdateCredentialsReason{
    kReasonCredentialsLoaded,
    kReasonPropertyUpdate
  };

  static const int kPriorityNone;

  // A constructor for the Service object
  Service(ControlInterface* control_interface,
          EventDispatcher* dispatcher,
          Metrics* metrics,
          Manager* manager,
          Technology::Identifier technology);

  // AutoConnect MAY choose to ignore the connection request in some
  // cases. For example, if the corresponding Device only supports one
  // concurrent connection, and another Service is already connected
  // or connecting.
  //
  // AutoConnect MAY issue RPCs immediately. So AutoConnect MUST NOT
  // be called from a D-Bus signal handler context.
  virtual void AutoConnect();
  // Queue up a connection attempt. Derived classes SHOULD call the
  // base class implementation before beginning a connect. The base
  // class will log the connection attempt, and update base-class
  // state.
  virtual void Connect(Error* error, const char* reason);
  // Disconnect this service.  Override this method to add your service specific
  // disconnect logic, but call the super class's Disconnect() first.
  virtual void Disconnect(Error* error, const char* reason);
  // Disconnects this service via Disconnect().  Marks the service as having
  // failed with |failure|.  Do not override this method.
  virtual void DisconnectWithFailure(ConnectFailure failure,
                                     Error* error,
                                     const char* reason);
  // Disconnects this service via Disconnect(). The service will not be eligible
  // for auto-connect until a subsequent call to Connect, or Load.  Do not
  // override this method.
  virtual void UserInitiatedDisconnect(Error* error);
  // Connect to this service via Connect(). This function indicates that the
  // connection attempt is user-initiated.
  virtual void UserInitiatedConnect(Error* error);

  // The default implementation returns the error kInvalidArguments.
  virtual void ActivateCellularModem(const std::string& carrier,
                                     Error* error,
                                     const ResultCallback& callback);
  // The default implementation returns the error kNotSupported.
  virtual void CompleteCellularActivation(Error* error);

  virtual bool IsActive(Error* error);

  // Returns whether services of this type should be auto-connect by default.
  virtual bool IsAutoConnectByDefault() const { return false; }

  virtual ConnectState state() const { return state_; }
  // Updates the state of the Service and alerts the manager.  Also
  // clears |failure_| if the new state isn't a failure.
  virtual void SetState(ConnectState state);
  std::string GetStateString() const;

  // Set portal detection failure phase and status (reason). This function
  // is called when portal detection failed for the Service.
  virtual void SetPortalDetectionFailure(const std::string& phase,
                                         const std::string& status);

  // State utility functions
  static bool IsConnectedState(ConnectState state);
  static bool IsConnectingState(ConnectState state);

  virtual bool IsConnected() const;
  virtual bool IsConnecting() const;
  virtual bool IsFailed() const {
    // We sometimes lie about the failure state, to keep Chrome happy
    // (see comment in WiFi::HandleDisconnect). Hence, we check both
    // state and |failed_time_|.
    return state() == kStateFailure || failed_time_ > 0;
  }

  virtual bool IsInFailState() const {
    return state() == kStateFailure;
  }

  virtual bool IsOnline() const {
    return state() == kStateOnline;
  }

  // Returns true if the connection for |this| depends on service |b|.
  virtual bool IsDependentOn(const ServiceRefPtr& b) const;

  virtual bool IsPortalled() const {
    return state() == kStatePortal;
  }

  // Return true if service is allowed to automatically switch to fallback
  // DNS server.
  virtual bool is_dns_auto_fallback_allowed() const {
    return is_dns_auto_fallback_allowed_;
  }

  virtual bool link_monitor_disabled() const { return link_monitor_disabled_; }

  virtual ConnectFailure failure() const { return failure_; }
  // Sets the |previous_error_| property based on the current |failure_|, and
  // sets a serial number for this failure.
  virtual void SaveFailure();
  // Records the failure mode and time. Sets the Service state to "Failure".
  virtual void SetFailure(ConnectFailure failure);
  // Records the failure mode and time. Sets the Service state to "Idle".
  // Avoids showing a failure mole in the UI.
  virtual void SetFailureSilent(ConnectFailure failure);

  // Returns a string that is guaranteed to uniquely identify this Service
  // instance.
  const std::string& unique_name() const { return unique_name_; }

  virtual std::string GetRpcIdentifier() const;

  // Returns the unique persistent storage identifier for the service.
  virtual std::string GetStorageIdentifier() const = 0;

  // Returns the identifier within |storage| from which configuration for
  // this service can be loaded.  Returns an empty string if no entry in
  // |storage| can be used.
  virtual std::string GetLoadableStorageIdentifier(
      const StoreInterface& storage) const;

  // Returns whether the service configuration can be loaded from |storage|.
  virtual bool IsLoadableFrom(const StoreInterface& storage) const;

  // Returns true if the service uses 802.1x for key management.
  virtual bool Is8021x() const { return false; }

  // Loads the service from persistent |storage|. Returns true on success.
  virtual bool Load(StoreInterface* storage);

  // Indicate to service that it is no longer persisted to storage.  It
  // should purge any stored profile state (e.g., credentials).  Returns
  // true to indicate that this service should also be unregistered from
  // the manager, false otherwise.
  virtual bool Unload();

  // Attempt to remove the service. On failure, no changes in state will occur.
  virtual void Remove(Error* error);

  // Saves the service to persistent |storage|. Returns true on success.
  virtual bool Save(StoreInterface* storage);

  // Applies all the properties in |args| to this service object's mutable
  // store, except for those in parameters_ignored_for_configure_.
  // Returns an error in |error| if one or more parameter set attempts
  // fails, but will only return the first error.
  virtual void Configure(const KeyValueStore& args, Error* error);

  // Iterate over all the properties in |args| and test for an identical
  // value in this service object's store.  Returns false if one or more
  // keys in |args| do not exist or have different values, true otherwise.
  virtual bool DoPropertiesMatch(const KeyValueStore& args) const;

  // Returns whether portal detection is explicitly disabled on this service
  // via a property set on it.
  virtual bool IsPortalDetectionDisabled() const;

  // Returns whether portal detection is set to follow the default setting
  // of this service's technology via a property set on it.
  virtual bool IsPortalDetectionAuto() const;

  // Returns true if the service is persisted to a non-ephemeral profile.
  virtual bool IsRemembered() const;

  // Returns true if the service RPC identifier should be part of the
  // manager's advertised services list, false otherwise.
  virtual bool IsVisible() const { return true; }

  // Returns true if there is a proxy configuration set on this service.
  virtual bool HasProxyConfig() const { return !proxy_config_.empty(); }

  // Returns whether this service has had recent connection issues.
  virtual bool HasRecentConnectionIssues();

  // If the AutoConnect property has not already been marked as saved, set
  // its value to true and mark it saved.
  virtual void EnableAndRetainAutoConnect();

  // Set the connection for this service.  If the connection is non-NULL, create
  // an HTTP Proxy that will utilize this service's connection to serve
  // requests.
  virtual void SetConnection(const ConnectionRefPtr& connection);
  virtual const ConnectionRefPtr& connection() const { return connection_; }

  // Emit service's IP config change event to chrome.
  virtual void NotifyIPConfigChanges();

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  // Examines the EAP credentials for the service and returns true if a
  // connection attempt can be made.
  virtual bool Is8021xConnectable() const;

  // Add an EAP certification id |name| at position |depth| in the stack.
  // Returns true if entry was added, false otherwise.
  virtual bool AddEAPCertification(const std::string& name, size_t depth);
  // Clear all EAP certification elements.
  virtual void ClearEAPCertification();
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  // Returns true if this service contains a IP address in its static IP
  // parameters, false otherwise.
  virtual bool HasStaticIPAddress() const;

  // Returns true if this service contains nameservers in its static IP
  // parameters, false otherwise.
  virtual bool HasStaticNameServers() const;

  // The inherited class that needs to send metrics after the service has
  // transitioned to the ready state should override this method.
  // |time_resume_to_ready_milliseconds| holds the elapsed time from when
  // the system was resumed until when the service transitioned to the
  // connected state.  This value is non-zero for the first service transition
  // to the connected state after a resume.
  virtual void SendPostReadyStateMetrics(
      int64_t /*time_resume_to_ready_milliseconds*/) const {}

  bool auto_connect() const { return auto_connect_; }
  void SetAutoConnect(bool connect);

  bool connectable() const { return connectable_; }
  // Sets the connectable property of the service, and broadcast the
  // new value. Does not update the manager.
  // TODO(petkov): Remove this method in favor of SetConnectableFull.
  void SetConnectable(bool connectable);
  // Sets the connectable property of the service, broadcasts the new
  // value, and alerts the manager if necessary.
  void SetConnectableFull(bool connectable);

  virtual bool explicitly_disconnected() const {
    return explicitly_disconnected_;
  }

  // Return RPC identifier for device that's internal to this service, which is
  // not registered with the manager.
  virtual std::string GetInnerDeviceRpcIdentifier() const { return ""; }

  bool retain_auto_connect() const { return retain_auto_connect_; }
  // Setter is deliberately omitted; use EnableAndRetainAutoConnect.

  void set_friendly_name(const std::string& n) { friendly_name_ = n; }
  const std::string& friendly_name() const { return friendly_name_; }
  // Sets the kNameProperty and broadcasts the change.
  void SetFriendlyName(const std::string& friendly_name);

  const std::string& guid() const { return guid_; }
  bool SetGuid(const std::string& guid, Error* error);

  bool has_ever_connected() const { return has_ever_connected_; }
  // Sets the has_ever_connected_ property of the service
  // and broadcasts the new value
  void SetHasEverConnected(bool has_ever_connected);

  int32_t priority() const { return priority_; }
  bool SetPriority(const int32_t& priority, Error* error);
  int32_t priority_within_technology() const {
      return priority_within_technology_;
  }
  bool SetPriorityWithinTechnology(const int32_t& priority, Error* error);

  size_t crypto_algorithm() const { return crypto_algorithm_; }
  bool key_rotation() const { return key_rotation_; }
  bool endpoint_auth() const { return endpoint_auth_; }

  void SetStrength(uint8_t strength);

  // uint8_t streams out as a char. Coerce to a larger type, so that
  // it prints as a number.
  uint16_t strength() const { return strength_; }

  virtual Technology::Identifier technology() const { return technology_; }
  std::string GetTechnologyString() const;

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  virtual const EapCredentials* eap() const { return eap_.get(); }
  void SetEapCredentials(EapCredentials* eap);
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  bool save_credentials() const { return save_credentials_; }
  void set_save_credentials(bool save) { save_credentials_ = save; }

  const std::string& error() const { return error_; }
  void set_error(const std::string& error) { error_ = error; }

  const std::string& error_details() const { return error_details_; }
  void SetErrorDetails(const std::string& details);

  static const char* ConnectFailureToString(const ConnectFailure& state);
  static const char* ConnectStateToString(const ConnectState& state);

  // Compare two services.  Returns true if Service |a| should be displayed
  // above |b|.  If |compare_connectivity_state| is true, the connectivity
  // state of the service (service->state()) is used as the most significant
  // criteria for comparsion, otherwise the service state is ignored.  Use
  // |tech_order| to rank services if more decisive criteria do not yield a
  // difference.  |reason| is populated with the exact criteria used for the
  // ultimate comparison.
  static bool Compare(Manager* manager,
                      ServiceRefPtr a,
                      ServiceRefPtr b,
                      bool compare_connectivity_state,
                      const std::vector<Technology::Identifier>& tech_order,
                      const char** reason);

  // These are defined in service.cc so that we don't have to include profile.h
  // TODO(cmasone): right now, these are here only so that we can get the
  // profile name as a property.  Can we store just the name, and then handle
  // setting the profile for this service via |manager_|?
  const ProfileRefPtr& profile() const;

  // Sets the profile property of this service. Broadcasts the new value if it's
  // not nullptr. If the new value is nullptr, the service will either be set to
  // another profile afterwards or it will not be visible and not monitored
  // anymore.
  void SetProfile(const ProfileRefPtr& p);

  // This is called from tests and shouldn't be called otherwise. Use SetProfile
  // instead.
  void set_profile(const ProfileRefPtr& p);

  // Notification that occurs when a service now has profile data saved
  // on its behalf.  Some service types like WiFi can choose to register
  // themselves at this point.
  virtual void OnProfileConfigured() {}

  // Notification that occurs when a single property has been changed via
  // the RPC adaptor.
  virtual void OnPropertyChanged(const std::string& property);

  // Notification that occurs when an EAP credential property has been
  // changed.  Some service subclasses can choose to respond to this
  // event.
  virtual void OnEapCredentialsChanged(UpdateCredentialsReason reason) {}

  // Called by the manager once after a resume.
  virtual void OnAfterResume();

  // Called by the manager once when entering dark resume.
  virtual void OnDarkResume();

  // Called by the manager to clear remembered state of being explicitly
  // disconnected.
  virtual void ClearExplicitlyDisconnected();

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  EapCredentials* mutable_eap() { return eap_.get(); }
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  const DhcpProperties& dhcp_properties() const {
    return *dhcp_properties_;
  }

  PropertyStore* mutable_store() { return &store_; }
  const PropertyStore& store() const { return store_; }
  StaticIPParameters* mutable_static_ip_parameters() {
    return &static_ip_parameters_;
  }
  const StaticIPParameters& static_ip_parameters() const {
    return static_ip_parameters_;
  }

  // Retrieves |key| from |id| in |storage| to |value|.  If this key does
  // not exist, assign |default_value| to |value|.
  static void LoadString(StoreInterface* storage,
                         const std::string& id,
                         const std::string& key,
                         const std::string& default_value,
                         std::string* value);

  // Assigns |value| to |key| in |storage| if |value| is non-empty and |save| is
  // true. Otherwise, removes |key| from |storage|. If |crypted| is true, the
  // value is encrypted.
  static void SaveString(StoreInterface* storage,
                         const std::string& id,
                         const std::string& key,
                         const std::string& value,
                         bool crypted,
                         bool save);

  // Called via RPC to get a dict containing profile-to-entry_name mappings
  // of all the profile entires which contain configuration applicable to
  // this service.
  std::map<std::string, std::string> GetLoadableProfileEntries();

  void set_connection_id(int connection_id) { connection_id_ = connection_id; }
  int connection_id() const { return connection_id_; }

  void set_unreliable(bool unreliable) { unreliable_ = unreliable; }
  bool unreliable() const { return unreliable_; }

 protected:
  friend class base::RefCounted<Service>;

  static const char kAutoConnBusy[];

  virtual ~Service();

  // Returns true if a character is allowed to be in a service storage id.
  static bool LegalChar(char a) { return isalnum(a) || a == '_'; }

  // Returns true if a character is disallowed to be in a service storage id.
  static bool IllegalChar(char a) { return !LegalChar(a); }

  virtual std::string CalculateState(Error* error);
  std::string CalculateTechnology(Error* error);

  bool GetVisibleProperty(Error* error);

  // Returns whether this service is in a state conducive to auto-connect.
  // This should include any tests used for computing connectable(),
  // as well as other critera such as whether the device associated with
  // this service is busy with another connection.
  //
  // If the service is not auto-connectable, |*reason| will be set to
  // point to C-string explaining why the service is not auto-connectable.
  virtual bool IsAutoConnectable(const char** reason) const;

  // HelpRegisterDerived*: Expose a property over RPC, with the name |name|.
  //
  // Reads of the property will be handled by invoking |get|.
  // Writes to the property will be handled by invoking |set|.
  // Clearing the property will be handled by PropertyStore.
  void HelpRegisterDerivedBool(
      const std::string& name,
      bool(Service::*get)(Error* error),
      bool(Service::*set)(const bool& value, Error* error),
      void(Service::*clear)(Error* error));
  void HelpRegisterDerivedInt32(
      const std::string& name,
      int32_t(Service::*get)(Error* error),
      bool(Service::*set)(const int32_t& value, Error* error));
  void HelpRegisterDerivedString(
      const std::string& name,
      std::string(Service::*get)(Error* error),
      bool(Service::*set)(const std::string& value, Error* error));
  void HelpRegisterConstDerivedUint16(
      const std::string& name,
      uint16_t(Service::*get)(Error* error) const);
  void HelpRegisterConstDerivedRpcIdentifier(
      const std::string& name,
      std::string(Service::*get)(Error*) const);
  void HelpRegisterConstDerivedStrings(
      const std::string& name, Strings(Service::*get)(Error* error) const);
  void HelpRegisterConstDerivedString(
      const std::string& name, std::string(Service::*get)(Error* error) const);

  // HelpRegisterObservedDerived*: Expose an property over RPC, with the
  // name |name|, for which property changes are automatically generated.
  //
  void HelpRegisterObservedDerivedBool(
      const std::string& name,
      bool(Service::*get)(Error* error),
      bool(Service::*set)(const bool& value, Error* error),
      void(Service::*clear)(Error* error));
  ServiceAdaptorInterface* adaptor() const { return adaptor_.get(); }

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  void UnloadEapCredentials();
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  // Ignore |parameter| when performing a Configure() operation.
  void IgnoreParameterForConfigure(const std::string& parameter);

  // Update the service's string-based "Error" RPC property based on the
  // failure_ enum.
  void UpdateErrorProperty();

  // RPC setter for the the "AutoConnect" property. Updates the |manager_|.
  // (cf. SetAutoConnect, which does not update the manager.)
  virtual bool SetAutoConnectFull(const bool& connect, Error* error);

  // RPC clear method for the "AutoConnect" property.  Sets the AutoConnect
  // property back to its default value, and clears the retain_auto_connect_
  // property to allow the AutoConnect property to be enabled automatically.
  void ClearAutoConnect(Error* error);

  // Property accessors reserved for subclasses
  EventDispatcher* dispatcher() const { return dispatcher_; }
  ControlInterface* control_interface() const { return control_interface_; }
#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  const std::string& GetEAPKeyManagement() const;
  virtual void SetEAPKeyManagement(const std::string& key_management);
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  Manager* manager() const { return manager_; }
  Metrics* metrics() const { return metrics_; }

  // Save the serivce's auto_connect value, without affecting its auto_connect
  // property itself. (cf. EnableAndRetainAutoConnect)
  void RetainAutoConnect();

  // Inform base class of the security properties for the service.
  //
  // NB: When adding a call to this function from a subclass, please check
  // that the semantics of SecurityLevel() are appropriate for the subclass.
  void SetSecurity(CryptoAlgorithm crypt, bool rotation, bool endpoint_auth);

  // Return whether this service is suspected or confirmed to be
  // provided by a mobile device, which is likely to be using a
  // metered backhaul for internet connectivity.
  virtual std::string GetTethering(Error* error) const;

  // Emit property change notifications for all observed properties.
  void NotifyPropertyChanges();

 private:
  friend class ActivePassiveOutOfCreditsDetectorTest;
  friend class EthernetEapServiceTest;
  friend class EthernetServiceTest;
  friend class MetricsTest;
  friend class ManagerTest;
  friend class ServiceAdaptorInterface;
  friend class ServiceTest;
  friend class SubscriptionStateOutOfCreditsDetectorTest;
  friend class VPNProviderTest;
  friend class VPNServiceTest;
  friend class WiFiServiceTest;
  friend class WiMaxProviderTest;
  friend class WiMaxServiceTest;
  friend void TestCommonPropertyChanges(ServiceRefPtr, ServiceMockAdaptor*);
  friend void TestCustomSetterNoopChange(ServiceRefPtr, MockManager*);
  friend void TestNamePropertyChange(ServiceRefPtr, ServiceMockAdaptor*);
  FRIEND_TEST(AllMockServiceTest, AutoConnectWithFailures);
  FRIEND_TEST(CellularCapabilityGSMTest, SetStorageIdentifier);
  FRIEND_TEST(CellularServiceTest, IsAutoConnectable);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithoutSelectedService);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithSelectedService);
  FRIEND_TEST(DeviceTest, IPConfigUpdatedFailureWithStatic);
  FRIEND_TEST(ManagerTest, ConnectToBestServices);
  FRIEND_TEST(ServiceTest, AutoConnectLogging);
  FRIEND_TEST(ServiceTest, CalculateState);
  FRIEND_TEST(ServiceTest, CalculateTechnology);
  FRIEND_TEST(ServiceTest, Certification);
  FRIEND_TEST(ServiceTest, Compare);
  FRIEND_TEST(ServiceTest, ConfigureEapStringProperty);
  FRIEND_TEST(ServiceTest, ConfigureIgnoredProperty);
  FRIEND_TEST(ServiceTest, Constructor);
  FRIEND_TEST(ServiceTest, CustomSetterNoopChange);
  FRIEND_TEST(ServiceTest, GetIPConfigRpcIdentifier);
  FRIEND_TEST(ServiceTest, GetProperties);
  FRIEND_TEST(ServiceTest, GetTethering);
  FRIEND_TEST(ServiceTest, IsAutoConnectable);
  FRIEND_TEST(ServiceTest, IsDependentOn);
  FRIEND_TEST(ServiceTest, Load);
  FRIEND_TEST(ServiceTest, LoadAutoConnect);
  FRIEND_TEST(ServiceTest, PortalDetectionFailure);
  FRIEND_TEST(ServiceTest, RecheckPortal);
  FRIEND_TEST(ServiceTest, Save);
  FRIEND_TEST(ServiceTest, SaveString);
  FRIEND_TEST(ServiceTest, SaveStringCrypted);
  FRIEND_TEST(ServiceTest, SaveStringDontSave);
  FRIEND_TEST(ServiceTest, SaveStringEmpty);
  FRIEND_TEST(ServiceTest, SecurityLevel);
  FRIEND_TEST(ServiceTest, SetCheckPortal);
  FRIEND_TEST(ServiceTest, SetConnectableFull);
  FRIEND_TEST(ServiceTest, SetFriendlyName);
  FRIEND_TEST(ServiceTest, SetProperty);
  FRIEND_TEST(ServiceTest, State);
  FRIEND_TEST(ServiceTest, StateResetAfterFailure);
  FRIEND_TEST(ServiceTest, UniqueAttributes);
  FRIEND_TEST(ServiceTest, Unload);
  FRIEND_TEST(ServiceTest, UserInitiatedConnectionResult);
  FRIEND_TEST(WiFiServiceTest, SetPassphraseResetHasEverConnected);
  FRIEND_TEST(WiFiServiceTest, SuspectedCredentialFailure);
  FRIEND_TEST(WiFiServiceTest, SetPassphraseRemovesCachedCredentials);
  FRIEND_TEST(WiFiServiceTest, LoadPassphraseClearCredentials);
  FRIEND_TEST(WiFiTimerTest, ReconnectTimer);
  FRIEND_TEST(WiFiMainTest, EAPEvent);  // For eap_.
  FRIEND_TEST(WiMaxServiceTest, ChangeCredResetHasEverConnected);
  FRIEND_TEST(EthernetEapServiceTest, OnEapCredentialsChanged);

  static const char kAutoConnConnected[];
  static const char kAutoConnConnecting[];
  static const char kAutoConnExplicitDisconnect[];
  static const char kAutoConnNotConnectable[];
  static const char kAutoConnOffline[];
  static const char kAutoConnTechnologyNotConnectable[];
  static const char kAutoConnThrottled[];

#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  static const size_t kEAPMaxCertificationElements;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X

  static const char kServiceSortAutoConnect[];
  static const char kServiceSortConnectable[];
  static const char kServiceSortHasEverConnected[];
  static const char kServiceSortIsConnected[];
  static const char kServiceSortDependency[];
  static const char kServiceSortIsConnecting[];
  static const char kServiceSortIsFailed[];
  static const char kServiceSortIsOnline[];
  static const char kServiceSortIsPortalled[];
  static const char kServiceSortPriority[];
  static const char kServiceSortPriorityWithinTechnology[];
  static const char kServiceSortSecurity[];
  static const char kServiceSortProfileOrder[];
  static const char kServiceSortEtc[];
  static const char kServiceSortSerialNumber[];
  static const char kServiceSortTechnology[];

  static const uint64_t kMaxAutoConnectCooldownTimeMilliseconds;
  static const uint64_t kMinAutoConnectCooldownTimeMilliseconds;
  static const uint64_t kAutoConnectCooldownBackoffFactor;

  static const int kDisconnectsMonitorSeconds;
  static const int kMisconnectsMonitorSeconds;
  static const int kReportDisconnectsThreshold;
  static const int kReportMisconnectsThreshold;
  static const int kMaxDisconnectEventHistory;
  static const int kMaxMisconnectEventHistory;

  bool GetAutoConnect(Error* error);

  std::string GetCheckPortal(Error* error);
  bool SetCheckPortal(const std::string& check_portal, Error* error);

  std::string GetGuid(Error* error);

  virtual std::string GetDeviceRpcId(Error* error) const = 0;

  std::string GetIPConfigRpcIdentifier(Error* error) const;

  std::string GetNameProperty(Error* error);
  // The base implementation asserts that |name| matches the current Name
  // property value.
  virtual bool SetNameProperty(const std::string& name, Error* error);

  int32_t GetPriority(Error* error);
  int32_t GetPriorityWithinTechnology(Error* error);

  std::string GetProfileRpcId(Error* error);
  bool SetProfileRpcId(const std::string& profile, Error* error);

  // Returns TCP port of service's HTTP proxy in host order.
  uint16_t GetHTTPProxyPort(Error* error) const;

  std::string GetProxyConfig(Error* error);
  bool SetProxyConfig(const std::string& proxy_config, Error* error);

  Strings GetDisconnectsProperty(Error* error) const;
  Strings GetMisconnectsProperty(Error* error) const;

  void ReEnableAutoConnectTask();
  // Disables autoconnect and posts a task to re-enable it after a cooldown.
  // Note that autoconnect could be disabled for other reasons as well.
  void ThrottleFutureAutoConnects();

  // Saves settings to profile, if we have one. Unlike
  // SaveServiceToProfile, SaveToProfile never assigns this service
  // into a profile.
  void SaveToProfile();

  // Qualify the conditions under which the most recent disconnect occurred.
  // Make note ot the fact that there was a problem connecting / staying
  // connected if the disconnection did not occur as a clear result of user
  // action.
  void NoteDisconnectEvent();

  // Utility function that returns true if a is different from b.  When they
  // are, "decision" is populated with the boolean value of "a > b".
  static bool DecideBetween(int a, int b, bool* decision);

  // Report the result of user-initiated connection attempt to UMA stats.
  // Currently only report stats for wifi service.
  void ReportUserInitiatedConnectionResult(ConnectState state);

  // Linearize security parameters (crypto algorithm, key rotation, endpoint
  // authentication) for comparison.
  uint16_t SecurityLevel();

  // WeakPtrFactory comes first, so that other fields can use it.
  base::WeakPtrFactory<Service> weak_ptr_factory_;

  ConnectState state_;
  ConnectState previous_state_;
  ConnectFailure failure_;
  bool auto_connect_;

  // Denotes whether the value of auto_connect_ property value should be
  // retained, i.e. only be allowed to change via explicit property changes
  // from the UI.
  bool retain_auto_connect_;

  std::string check_portal_;
  bool connectable_;
  std::string error_;
  std::string error_details_;
  std::string previous_error_;
  int32_t previous_error_serial_number_;
  bool explicitly_disconnected_;
  bool is_in_user_connect_;
  int32_t priority_;
  int32_t priority_within_technology_;
  uint8_t crypto_algorithm_;
  bool key_rotation_;
  bool endpoint_auth_;
  std::string portal_detection_failure_phase_;
  std::string portal_detection_failure_status_;

  uint8_t strength_;
  std::string proxy_config_;
  std::string ui_data_;
  std::string guid_;
  bool save_credentials_;
#if !defined(DISABLE_WIFI) || !defined(DISABLE_WIRED_8021X)
  std::unique_ptr<EapCredentials> eap_;
#endif  // DISABLE_WIFI || DISABLE_WIRED_8021X
  std::unique_ptr<DhcpProperties> dhcp_properties_;
  Technology::Identifier technology_;
  // The time of the most recent failure. Value is 0 if the service is
  // not currently failed.
  time_t failed_time_;
  // Whether or not this service has ever reached kStateConnected.
  bool has_ever_connected_;

  EventHistory disconnects_;  // Connection drops.
  EventHistory misconnects_;  // Failures to connect.

  base::CancelableClosure reenable_auto_connect_task_;
  uint64_t auto_connect_cooldown_milliseconds_;

  ProfileRefPtr profile_;
  PropertyStore store_;
  std::set<std::string> parameters_ignored_for_configure_;

  EventDispatcher* dispatcher_;
  ControlInterface* control_interface_;
  unsigned int serial_number_;
  std::string unique_name_;  // MUST be unique amongst service instances

  // Service's friendly name is presented through the UI. By default it's the
  // same as |unique_name_| but normally Service subclasses override
  // it. WARNING: Don't log the friendly name at the default logging level due
  // to PII concerns.
  std::string friendly_name_;

  // List of subject names reported by remote entity during TLS setup.
  std::vector<std::string> remote_certification_;

  std::unique_ptr<ServiceAdaptorInterface> adaptor_;
  std::unique_ptr<ServicePropertyChangeNotifier> property_change_notifier_;
  std::unique_ptr<HTTPProxy> http_proxy_;
  ConnectionRefPtr connection_;
  StaticIPParameters static_ip_parameters_;
  Metrics* metrics_;
  Manager* manager_;
  std::unique_ptr<Sockets> sockets_;
  Time* time_;
  DiagnosticsReporter* diagnostics_reporter_;

  // The |serial_number_| for the next Service.
  static unsigned int next_serial_number_;

  // Network identifier indicating the network (gateway) the service is
  // connected to.
  int connection_id_;
  // When set to true, this service will automatically fallback to Google's DNS
  // servers if the portal detection failed due to DNS failure and Google's DNS
  // servers are working.
  bool is_dns_auto_fallback_allowed_;
  // When set to true, will not start link monitor when the connection to this
  // service is established.
  bool link_monitor_disabled_;
  // When set to true, the credentials for this service will be considered
  // valid, and will not require an initial connection to rank it highly for
  // auto-connect.
  bool managed_credentials_;
  // Flag indicating if this service is unreliable (experiencing multiple
  // link monitor failures in a short period of time).
  bool unreliable_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

}  // namespace shill

#endif  // SHILL_SERVICE_H_
