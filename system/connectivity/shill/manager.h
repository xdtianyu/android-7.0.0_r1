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

#ifndef SHILL_MANAGER_H_
#define SHILL_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/cellular/modem_info.h"
#include "shill/crypto_util_proxy.h"
#include "shill/dhcp_properties.h"
#include "shill/device.h"
#include "shill/device_info.h"
#include "shill/event_dispatcher.h"
#include "shill/geolocation_info.h"
#include "shill/hook_table.h"
#include "shill/metrics.h"
#include "shill/net/ip_address.h"
#include "shill/power_manager.h"
#include "shill/profile.h"
#include "shill/property_store.h"
#include "shill/service.h"
#include "shill/upstart/upstart.h"
#include "shill/wimax/wimax_provider.h"

namespace shill {

class ControlInterface;
class DeviceClaimer;
class DefaultProfile;
class Error;
class EventDispatcher;
class IPAddressStore;
class ManagerAdaptorInterface;
class Resolver;
class StoreInterface;
class VPNProvider;

#if !defined(DISABLE_WIFI)
class WiFiProvider;
#if defined(__BRILLO__)
class RPCServiceWatcherInterface;
class WiFiDriverHal;
#endif  // __BRILLO__
#endif  // DISABLE_WIFI

#if !defined(DISABLE_WIRED_8021X)
class EthernetEapProvider;
#endif  // DISABLE_WIRED_8021X

class Manager : public base::SupportsWeakPtr<Manager> {
 public:
  typedef base::Callback<void(const ServiceRefPtr& service)> ServiceCallback;

  struct Properties {
   public:
    Properties()
        : offline_mode(false),
          portal_check_interval_seconds(0),
          arp_gateway(true),
          connection_id_salt(0),
          minimum_mtu(IPConfig::kUndefinedMTU) {}
    bool offline_mode;
    std::string check_portal_list;
    std::string country;
    int32_t portal_check_interval_seconds;
    std::string portal_url;
    std::string host_name;
    // Whether to ARP for the default gateway in the DHCP client after
    // acquiring a lease.
    bool arp_gateway;
    // Comma-separated list of technologies for which link-monitoring is
    // enabled.
    std::string link_monitor_technologies;
    // Comma-separated list of technologies for which auto-connect is disabled.
    std::string no_auto_connect_technologies;
    // Comma-separated list of technologies that should never be enabled.
    std::string prohibited_technologies;
    // Comma-separated list of DNS search paths to be ignored.
    std::string ignored_dns_search_paths;
    // Comma-separated list of DNS servers to prepend to resolver list.
    std::string prepend_dns_servers;
    // Salt value use for calculating network connection ID.
    int connection_id_salt;
    // The minimum MTU value that will be respected in DHCP responses.
    int minimum_mtu;
  };

  Manager(ControlInterface* control_interface,
          EventDispatcher* dispatcher,
          Metrics* metrics,
          const std::string& run_directory,
          const std::string& storage_directory,
          const std::string& user_storage_directory);
  virtual ~Manager();

  void RegisterAsync(const base::Callback<void(bool)>& completion_callback);

  virtual void SetBlacklistedDevices(
      const std::vector<std::string>& blacklisted_devices);
  virtual void SetWhitelistedDevices(
      const std::vector<std::string>& whitelisted_devices);

  // Returns true if |device_name| is either not in the blacklist, or in the
  // whitelist, depending on which list was supplied in startup settings.
  virtual bool DeviceManagementAllowed(const std::string& device_name);

  virtual void Start();
  virtual void Stop();
  bool running() const { return running_; }

  const ProfileRefPtr& ActiveProfile() const;
  bool IsActiveProfile(const ProfileRefPtr& profile) const;
  bool MoveServiceToProfile(const ServiceRefPtr& to_move,
                            const ProfileRefPtr& destination);
  ProfileRefPtr LookupProfileByRpcIdentifier(const std::string& profile_rpcid);

  // Called via RPC call on Service (|to_set|) to set the "Profile" property.
  virtual void SetProfileForService(const ServiceRefPtr& to_set,
                                    const std::string& profile,
                                    Error* error);

  virtual void RegisterDevice(const DeviceRefPtr& to_manage);
  virtual void DeregisterDevice(const DeviceRefPtr& to_forget);

  virtual bool HasService(const ServiceRefPtr& service);
  // Register a Service with the Manager. Manager may choose to
  // connect to it immediately.
  virtual void RegisterService(const ServiceRefPtr& to_manage);
  // Deregister a Service from the Manager. Caller is responsible
  // for disconnecting the Service before-hand.
  virtual void DeregisterService(const ServiceRefPtr& to_forget);
  virtual void UpdateService(const ServiceRefPtr& to_update);

  // Persists |to_update| into an appropriate profile.
  virtual void UpdateDevice(const DeviceRefPtr& to_update);

#if !defined(DISABLE_WIFI)
  virtual void UpdateWiFiProvider();
#endif  // DISABLE_WIFI

  std::vector<DeviceRefPtr>
      FilterByTechnology(Technology::Identifier tech) const;

  ServiceRefPtr FindService(const std::string& name);
  RpcIdentifiers EnumerateAvailableServices(Error* error);

  // Return the complete list of services, including those that are not visible.
  RpcIdentifiers EnumerateCompleteServices(Error* error);

  // called via RPC (e.g., from ManagerDBusAdaptor)
  std::map<std::string, std::string> GetLoadableProfileEntriesForService(
      const ServiceConstRefPtr& service);
  ServiceRefPtr GetService(const KeyValueStore& args, Error* error);
  ServiceRefPtr ConfigureService(const KeyValueStore& args, Error* error);
  ServiceRefPtr ConfigureServiceForProfile(
      const std::string& profile_rpcid,
      const KeyValueStore& args,
      Error* error);
  ServiceRefPtr FindMatchingService(const KeyValueStore& args, Error* error);

  // Retrieve geolocation data from the Manager.
  const std::map<std::string, GeolocationInfos>
      &GetNetworksForGeolocation() const;

  // Called by Device when its geolocation data has been updated.
  virtual void OnDeviceGeolocationInfoUpdated(const DeviceRefPtr& device);

  void ConnectToBestServices(Error* error);

  // Method to create connectivity report for connected services.
  void CreateConnectivityReport(Error* error);

  // Request portal detection checks on each registered device until a portal
  // detection attempt starts on one of them.
  void RecheckPortal(Error* error);
  // Request portal detection be restarted on the device connected to
  // |service|.
  virtual void RecheckPortalOnService(const ServiceRefPtr& service);

  virtual void RequestScan(Device::ScanType scan_type,
                           const std::string& technology, Error* error);
  // Configure scheduled scan for wifi devices.
  virtual void SetSchedScan(bool enable, Error* error);
  std::string GetTechnologyOrder();
  virtual void SetTechnologyOrder(const std::string& order, Error* error);
  // Set up the profile list starting with a default profile along with
  // an (optional) list of startup profiles.
  void InitializeProfiles();
  // Create a profile.  This does not affect the profile stack.  Returns
  // the RPC path of the created profile in |path|.
  void CreateProfile(const std::string& name, std::string* path, Error* error);
  // Pushes existing profile with name |name| onto stack of managed profiles.
  // Returns the RPC path of the pushed profile in |path|.
  void PushProfile(const std::string& name, std::string* path, Error* error);
  // Insert an existing user profile with name |name| into the stack of
  // managed profiles.  Associate |user_hash| with this profile entry.
  // Returns the RPC path of the pushed profile in |path|.
  void InsertUserProfile(const std::string& name,
                         const std::string& user_hash,
                         std::string* path,
                         Error* error);
  // Pops profile named |name| off the top of the stack of managed profiles.
  void PopProfile(const std::string& name, Error* error);
  // Remove the active profile.
  void PopAnyProfile(Error* error);
  // Remove all user profiles from the stack of managed profiles leaving only
  // default profiles.
  void PopAllUserProfiles(Error* error);
  // Remove the underlying persistent storage for a profile.
  void RemoveProfile(const std::string& name, Error* error);
  // Give the ownership of the device with name |device_name| to claimer with
  // name |claimer_name|. This will cause shill to stop managing this device.
  virtual void ClaimDevice(const std::string& claimer_name,
                           const std::string& interface_name,
                           Error* error);
  // Claimer |claimer_name| release the ownership of the device with
  // |interface_name| back to shill. This method will set |claimer_removed|
  // to true iff Claimer |claimer_name| is not the default claimer and no
  // longer claims any devices.
  virtual void ReleaseDevice(const std::string& claimer_name,
                             const std::string& interface_name,
                             bool* claimer_removed,
                             Error* error);
#if !defined(DISABLE_WIFI) && defined(__BRILLO__)
  // Setup an AP mode interface using WiFi driver HAL.  The driver
  // may or may not teardown the station mode interface as a result
  // of this call.  This behavior will be driver specific.
  // Returns true and sets |interface_name| on success, false otherwise.
  virtual bool SetupApModeInterface(std::string* out_interface_name,
                                    Error* error);

  // Setup a station mode interface using WiFi driver HAL.  The driver
  // may or may not teardown the AP mode interface as a result of this
  // call.  This behavior will be driver specific.
  // Returns true and sets |interface_name| on success, false otherwise.
  virtual bool SetupStationModeInterface(std::string* out_interface_name,
                                         Error* error);

  virtual void OnApModeSetterVanished();
#endif  // !DISABLE_WIFI && __BRILLO__

  // Called by a service to remove its associated configuration.  If |service|
  // is associated with a non-ephemeral profile, this configuration entry
  // will be removed and the manager will search for another matching profile.
  // If the service ends up with no matching profile, it is unloaded (which
  // may also remove the service from the manager's list, e.g. WiFi services
  // that are not visible)..
  void RemoveService(const ServiceRefPtr& service);
  // Handle the event where a profile is about to remove a profile entry.
  // Any Services that are dependent on this storage identifier will need
  // to find new profiles.  Return true if any service has been moved to a new
  // profile.  Any such services will have had the profile group removed from
  // the profile.
  virtual bool HandleProfileEntryDeletion(const ProfileRefPtr& profile,
                                          const std::string& entry_name);
  // Find a registered service that contains a GUID property that
  // matches |guid|.
  virtual ServiceRefPtr GetServiceWithGUID(const std::string& guid,
                                           Error* error);
  // Find a service that is both the member of |profile| and has a
  // storage identifier that matches |entry_name|.  This function is
  // called by the Profile in order to return a profile entry's properties.
  virtual ServiceRefPtr GetServiceWithStorageIdentifier(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error);
  // Create a temporary service for an entry |entry_name| within |profile|.
  // Callers must not register this service with the Manager or connect it
  // since it was never added to the provider's service list.
  virtual ServiceRefPtr CreateTemporaryServiceFromProfile(
      const ProfileRefPtr& profile,
      const std::string& entry_name,
      Error* error);
  // Return a reference to the Service associated with the default connection.
  // If there is no such connection, this function returns a reference to NULL.
  virtual ServiceRefPtr GetDefaultService() const;

  // Set enabled state of all |technology_name| devices to |enabled_state|.
  // Persist the state to storage is |persist| is true.
  void SetEnabledStateForTechnology(const std::string& technology_name,
                                    bool enabled_state,
                                    bool persist,
                                    Error* error,
                                    const ResultCallback& callback);
  // Return whether a technology is marked as enabled for portal detection.
  virtual bool IsPortalDetectionEnabled(Technology::Identifier tech);
  // Set the start-up value for the portal detection list.  This list will
  // be used until a value set explicitly over the control API.  Until
  // then, we ignore but do not overwrite whatever value is stored in the
  // profile.
  virtual void SetStartupPortalList(const std::string& portal_list);

  // Returns true if profile |a| has been pushed on the Manager's
  // |profiles_| stack before profile |b|.
  virtual bool IsProfileBefore(const ProfileRefPtr& a,
                               const ProfileRefPtr& b) const;

  // Return whether a service belongs to the ephemeral profile.
  virtual bool IsServiceEphemeral(const ServiceConstRefPtr& service) const;

  // Return whether a Technology has any connected Services.
  virtual bool IsTechnologyConnected(Technology::Identifier technology) const;

  // Return whether a technology is enabled for link monitoring.
  virtual bool IsTechnologyLinkMonitorEnabled(
      Technology::Identifier technology) const;

  // Return whether the Wake on LAN feature is enabled.
  virtual bool IsWakeOnLanEnabled() const { return is_wake_on_lan_enabled_; }

  // Return whether a technology is disabled for auto-connect.
  virtual bool IsTechnologyAutoConnectDisabled(
      Technology::Identifier technology) const;

  // Report whether |technology| is prohibited from being enabled.
  virtual bool IsTechnologyProhibited(Technology::Identifier technology) const;

  // Called by Profile when a |storage| completes initialization.
  void OnProfileStorageInitialized(Profile* storage);

  // Return a Device with technology |technology| in the enabled state.
  virtual DeviceRefPtr GetEnabledDeviceWithTechnology(
      Technology::Identifier technology) const;

  // Return a Device with link_name |link_name| in the enabled state.
  virtual DeviceRefPtr GetEnabledDeviceByLinkName(
      const std::string& link_name) const;

  // Returns true if at least one connection exists, and false if there's no
  // connected service.
  virtual bool IsConnected() const;
  // Returns true if at least one connection exists that have Internet
  // connectivity, and false if there's no such service.
  virtual bool IsOnline() const;
  std::string CalculateState(Error* error);

  // Recalculate the |connected_state_| string and emit a singal if it has
  // changed.
  void RefreshConnectionState();

  virtual int GetPortalCheckInterval() const {
    return props_.portal_check_interval_seconds;
  }
  virtual const std::string& GetPortalCheckURL() const {
    return props_.portal_url;
  }

  virtual DeviceInfo* device_info() { return &device_info_; }
#if !defined(DISABLE_CELLULAR)
  virtual ModemInfo* modem_info() { return &modem_info_; }
#endif  // DISABLE_CELLULAR
  PowerManager* power_manager() const { return power_manager_.get(); }
#if !defined(DISABLE_WIRED_8021X)
  virtual EthernetEapProvider* ethernet_eap_provider() const {
    return ethernet_eap_provider_.get();
  }
#endif  // DISABLE_WIRED_8021X
  VPNProvider* vpn_provider() const { return vpn_provider_.get(); }
#if !defined(DISABLE_WIFI)
  WiFiProvider* wifi_provider() const { return wifi_provider_.get(); }
#endif  // DISABLE_WIFI
#if !defined(DISABLE_WIMAX)
  virtual WiMaxProvider* wimax_provider() { return wimax_provider_.get(); }
#endif  // DISABLE_WIMAX
  PropertyStore* mutable_store() { return &store_; }
  virtual const PropertyStore& store() const { return store_; }
  virtual const base::FilePath& run_path() const { return run_path_; }
  const base::FilePath& storage_path() const { return storage_path_; }
  IPAddressStore* health_checker_remote_ips() const {
    return health_checker_remote_ips_.get();
  }

  bool GetArpGateway() const { return props_.arp_gateway; }

  virtual int GetMinimumMTU() const { return props_.minimum_mtu; }
  virtual void SetMinimumMTU(const int mtu) { props_.minimum_mtu = mtu; }

  virtual void UpdateEnabledTechnologies();
  virtual void UpdateUninitializedTechnologies();

  const DhcpProperties& dhcp_properties() const {
    return *dhcp_properties_;
  }

  // Writes the service |to_update| to persistant storage.  If the service's is
  // ephemeral, it is moved to the current profile.
  void SaveServiceToProfile(const ServiceRefPtr& to_update);

  // Adds a closure to be executed when ChromeOS suspends or shill terminates.
  // |name| should be unique; otherwise, a previous closure by the same name
  // will be replaced.  |start| will be called when RunTerminationActions() is
  // called.  When an action completed, TerminationActionComplete() must be
  // called.
  void AddTerminationAction(const std::string& name,
                            const base::Closure& start);

  // Users call this function to report the completion of an action |name|.
  // This function should be called once for each action.
  void TerminationActionComplete(const std::string& name);

  // Removes the action associtated with |name|.
  void RemoveTerminationAction(const std::string& name);

  // Runs the termination actions and notifies the metrics framework
  // that the termination actions started running, only if any termination
  // actions have been registered. If all actions complete within
  // |kTerminationActionsTimeoutMilliseconds|, |done_callback| is called with a
  // value of Error::kSuccess. Otherwise, it is called with
  // Error::kOperationTimeout.
  //
  // Returns true, if termination actions were run.
  bool RunTerminationActionsAndNotifyMetrics(
      const ResultCallback& done_callback);

  // Registers a |callback| that's invoked whenever the default service
  // changes. Returns a unique tag that can be used to deregister the
  // callback. A tag equal to 0 is invalid.
  virtual int RegisterDefaultServiceCallback(const ServiceCallback& callback);
  virtual void DeregisterDefaultServiceCallback(int tag);

#if !defined(DISABLE_WIFI)
  // Verifies that the destination described by certificate is valid, and that
  // we're currently connected to that destination.  A full description of the
  // rules being enforced is in doc/manager-api.txt.  Returns true iff all
  // checks pass, false otherwise.  On false, error is filled with a
  // descriptive error code and message.
  //
  // |certificate| is a PEM encoded x509 certificate, |public_key| is a base64
  // encoded public half of an RSA key, |nonce| is a random string, and
  // |signed_data| is a base64 encoded string as described in
  // doc/manager-api.txt.
  void VerifyDestination(const std::string& certificate,
                         const std::string& public_key,
                         const std::string& nonce,
                         const std::string& signed_data,
                         const std::string& destination_udn,
                         const std::string& hotspot_ssid,
                         const std::string& hotspot_bssid,
                         const ResultBoolCallback& cb,
                         Error* error);

  // After verifying the destination, encrypt the string data with
  // |public_key|, the base64 encoded public half of an RSA key pair.  Returns
  // the base64 encoded result if successful, or an empty string on failure.
  // On failure, |error| will be filled with an appropriately descriptive
  // message and error code.
  void VerifyAndEncryptData(const std::string& certificate,
                            const std::string& public_key,
                            const std::string& nonce,
                            const std::string& signed_data,
                            const std::string& destination_udn,
                            const std::string& hotspot_ssid,
                            const std::string& hotspot_bssid,
                            const std::string& data,
                            const ResultStringCallback& cb,
                            Error* error);

  // After verifying the destination, encrypt the password for |network_path|
  // under |public_key|.  Similar to EncryptData above except that the
  // information being encrypted is implicitly the authentication credentials
  // of the given network.
  void VerifyAndEncryptCredentials(const std::string& certificate,
                                   const std::string& public_key,
                                   const std::string& nonce,
                                   const std::string& signed_data,
                                   const std::string& destination_udn,
                                   const std::string& hotspot_ssid,
                                   const std::string& hotspot_bssid,
                                   const std::string& network_path,
                                   const ResultStringCallback& cb,
                                   Error* error);
#endif  // DISABLE_WIFI

  // Calculate connection identifier, which is hash of salt value, gateway IP
  // address, and gateway MAC address.
  int CalcConnectionId(std::string gateway_ip, std::string gateway_mac);

  // Report the number of services associated with given connection
  // |connection_id|.
  void ReportServicesOnSameNetwork(int connection_id);

  // Running in passive mode, manager will not manage any devices (all devices
  // are blacklisted) by default. Remote application can specify devices for
  // shill to manage through ReleaseInterface/ClaimInterface DBus API using
  // default claimer (with "" as claimer_name).
  virtual void SetPassiveMode();

  // Decides whether Ethernet-like devices are treated as unknown devices
  // if they do not indicate a driver name.
  virtual void SetIgnoreUnknownEthernet(bool ignore);
  virtual bool ignore_unknown_ethernet() const {
    return ignore_unknown_ethernet_;
  }

  // Set the list of prepended DNS servers to |prepend_dns_servers|.
  virtual void SetPrependDNSServers(const std::string& prepend_dns_servers);

  // Accept hostname from DHCP server for devices matching |hostname_from|.
  virtual void SetAcceptHostnameFrom(const std::string& hostname_from);
  virtual bool ShouldAcceptHostnameFrom(const std::string& device_name) const;

  // Set DHCPv6 enabled device list.
  virtual void SetDHCPv6EnabledDevices(
      const std::vector<std::string>& device_list);

  // Return true if DHCPv6 is enabled for the given device with |device_name|.
  virtual bool IsDHCPv6EnabledForDevice(const std::string& device_name) const;

  // Filter the list of prepended DNS servers, copying only those that match
  // |family| into |dns_servers|.  |dns_servers| is cleared, regardless of
  // whether or not there are any addresses that match |family|.
  virtual std::vector<std::string> FilterPrependDNSServersByFamily(
      IPAddress::Family family) const;

  // Returns true iff |power_manager_| exists and is suspending (i.e.
  // power_manager->suspending() is true), false otherwise.
  virtual bool IsSuspending();

  void RecordDarkResumeWakeReason(const std::string& wake_reason);

  // Called when service's inner device changed.
  virtual void OnInnerDevicesChanged();

  void set_suppress_autoconnect(bool val) { suppress_autoconnect_ = val; }
  bool suppress_autoconnect() { return suppress_autoconnect_; }

  // Called when remote device claimer vanishes.
  virtual void OnDeviceClaimerVanished();

 private:
  friend class CellularTest;
  friend class DeviceInfoTest;
  friend class ManagerAdaptorInterface;
  friend class ManagerTest;
  friend class ModemInfoTest;
  friend class ModemManagerTest;
  friend class ServiceTest;
  friend class VPNServiceTest;
  friend class WiFiObjectTest;
  friend class WiMaxProviderTest;

  FRIEND_TEST(CellularCapabilityUniversalMainTest, TerminationAction);
  FRIEND_TEST(CellularCapabilityUniversalMainTest,
              TerminationActionRemovedByStopModem);
  FRIEND_TEST(CellularTest, LinkEventWontDestroyService);
  FRIEND_TEST(DefaultProfileTest, LoadManagerDefaultProperties);
  FRIEND_TEST(DefaultProfileTest, LoadManagerProperties);
  FRIEND_TEST(DefaultProfileTest, Save);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithoutSelectedService);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithSelectedService);
  FRIEND_TEST(DeviceTest, StartProhibited);
  FRIEND_TEST(ManagerTest, AvailableTechnologies);
  FRIEND_TEST(ManagerTest, ClaimBlacklistedDevice);
  FRIEND_TEST(ManagerTest, ClaimDeviceWhenClaimerNotVerified);
  FRIEND_TEST(ManagerTest, ClaimDeviceWithoutClaimer);
  FRIEND_TEST(ManagerTest, ConnectedTechnologies);
  FRIEND_TEST(ManagerTest, ConnectionStatusCheck);
  FRIEND_TEST(ManagerTest, ConnectToBestServices);
  FRIEND_TEST(ManagerTest, CreateConnectivityReport);
  FRIEND_TEST(ManagerTest, DefaultTechnology);
  FRIEND_TEST(ManagerTest, DetectMultiHomedDevices);
  FRIEND_TEST(ManagerTest, DeviceClaimerVanishedTask);
  FRIEND_TEST(ManagerTest, DevicePresenceStatusCheck);
  FRIEND_TEST(ManagerTest, DeviceRegistrationAndStart);
  FRIEND_TEST(ManagerTest, DisableTechnology);
  FRIEND_TEST(ManagerTest, EnableTechnology);
  FRIEND_TEST(ManagerTest, EnumerateProfiles);
  FRIEND_TEST(ManagerTest, EnumerateServiceInnerDevices);
  FRIEND_TEST(ManagerTest, HandleProfileEntryDeletionWithUnload);
  FRIEND_TEST(ManagerTest, InitializeProfilesInformsProviders);
  FRIEND_TEST(ManagerTest, InitializeProfilesHandlesDefaults);
  FRIEND_TEST(ManagerTest, IsDefaultProfile);
  FRIEND_TEST(ManagerTest, IsTechnologyAutoConnectDisabled);
  FRIEND_TEST(ManagerTest, IsTechnologyProhibited);
  FRIEND_TEST(ManagerTest, IsWifiIdle);
  FRIEND_TEST(ManagerTest, LinkMonitorEnabled);
  FRIEND_TEST(ManagerTest, MoveService);
  FRIEND_TEST(ManagerTest, NotifyDefaultServiceChanged);
  FRIEND_TEST(ManagerTest, OnApModeSetterVanished);
  FRIEND_TEST(ManagerTest, OnDeviceClaimerAppeared);
  FRIEND_TEST(ManagerTest, PopProfileWithUnload);
  FRIEND_TEST(ManagerTest, RegisterKnownService);
  FRIEND_TEST(ManagerTest, RegisterUnknownService);
  FRIEND_TEST(ManagerTest, ReleaseBlacklistedDevice);
  FRIEND_TEST(ManagerTest, ReleaseDevice);
  FRIEND_TEST(ManagerTest, RunTerminationActions);
  FRIEND_TEST(ManagerTest, ServiceRegistration);
  FRIEND_TEST(ManagerTest, SetupApModeInterface);
  FRIEND_TEST(ManagerTest, SetupStationModeInterface);
  FRIEND_TEST(ManagerTest, SortServicesWithConnection);
  FRIEND_TEST(ManagerTest, StartupPortalList);
  FRIEND_TEST(ServiceTest, IsAutoConnectable);

  struct DeviceClaim {
    DeviceClaim() {}
    DeviceClaim(const std::string& in_device_name,
                const ResultCallback& in_result_callback)
        : device_name(in_device_name),
          result_callback(in_result_callback) {}
    std::string device_name;
    ResultCallback result_callback;
  };

  static const char kErrorNoDevice[];
  static const char kErrorTypeRequired[];
  static const char kErrorUnsupportedServiceType[];

  // Technologies to probe for.
  static const char* kProbeTechnologies[];

  // Name of the default claimer.
  static const char kDefaultClaimerName[];

  // Timeout interval for probing various device status, and report them to
  // UMA stats.
  static const int kDeviceStatusCheckIntervalMilliseconds;
  // Time to wait for termination actions to complete.
  static const int kTerminationActionsTimeoutMilliseconds;

  void AutoConnect();
  std::vector<std::string> AvailableTechnologies(Error* error);
  std::vector<std::string> ConnectedTechnologies(Error* error);
  std::string DefaultTechnology(Error* error);
  std::vector<std::string> EnabledTechnologies(Error* error);
  std::vector<std::string> UninitializedTechnologies(Error* error);
  RpcIdentifiers EnumerateDevices(Error* error);
  RpcIdentifiers EnumerateProfiles(Error* error);
  RpcIdentifiers EnumerateWatchedServices(Error* error);
  std::string GetActiveProfileRpcIdentifier(Error* error);
  std::string GetCheckPortalList(Error* error);
  RpcIdentifier GetDefaultServiceRpcIdentifier(Error* error);
  std::string GetIgnoredDNSSearchPaths(Error* error);
  ServiceRefPtr GetServiceInner(const KeyValueStore& args, Error* error);
  bool SetCheckPortalList(const std::string& portal_list, Error* error);
  bool SetIgnoredDNSSearchPaths(const std::string& ignored_paths, Error* error);
  void EmitDefaultService();
  bool IsTechnologyInList(const std::string& technology_list,
                          Technology::Identifier tech) const;
  void EmitDeviceProperties();
#if !defined(DISABLE_WIFI)
  bool SetDisableWiFiVHT(const bool& disable_wifi_vht, Error* error);
  bool GetDisableWiFiVHT(Error* error);
#endif  // DISABLE_WIFI
  bool SetProhibitedTechnologies(const std::string& prohibited_technologies,
                                 Error* error);
  std::string GetProhibitedTechnologies(Error* error);
  void OnTechnologyProhibited(Technology::Identifier technology,
                              const Error& error);

  // For every device instance that is sharing the same connectivity with
  // another device, enable the multi-home flag.
  void DetectMultiHomedDevices();

  // Unload a service while iterating through |services_|.  Returns true if
  // service was erased (which means the caller loop should not increment
  // |service_iterator|), false otherwise (meaning the caller should
  // increment |service_iterator|).
  bool UnloadService(std::vector<ServiceRefPtr>::iterator* service_iterator);

  // Load Manager default properties from |profile|.
  void LoadProperties(const scoped_refptr<DefaultProfile>& profile);

  // Configure the device with profile data from all current profiles.
  void LoadDeviceFromProfiles(const DeviceRefPtr& device);

  void HelpRegisterConstDerivedRpcIdentifier(
      const std::string& name,
      RpcIdentifier(Manager::*get)(Error*));
  void HelpRegisterConstDerivedRpcIdentifiers(
      const std::string& name,
      RpcIdentifiers(Manager::*get)(Error*));
  void HelpRegisterDerivedString(
      const std::string& name,
      std::string(Manager::*get)(Error* error),
      bool(Manager::*set)(const std::string&, Error*));
  void HelpRegisterConstDerivedStrings(
      const std::string& name,
      Strings(Manager::*get)(Error*));
  void HelpRegisterDerivedBool(
      const std::string& name,
      bool(Manager::*get)(Error* error),
      bool(Manager::*set)(const bool& value, Error* error));

  bool HasProfile(const Profile::Identifier& ident);
  void PushProfileInternal(const Profile::Identifier& ident,
                           std::string* path,
                           Error* error);
  void PopProfileInternal();
  void OnProfilesChanged();

  void SortServices();
  void SortServicesTask();
  void DeviceStatusCheckTask();
  void ConnectionStatusCheck();
  void DevicePresenceStatusCheck();

  bool MatchProfileWithService(const ServiceRefPtr& service);

  // Sets the profile of |service| to |profile|, without notifying its
  // previous profile.  Configures a |service| with |args|, then saves
  // the resulting configuration to |profile|.  This method is useful
  // when copying a service configuration from one profile to another,
  // or writing a newly created service config to a specific profile.
  static void SetupServiceInProfile(ServiceRefPtr service,
                                    ProfileRefPtr profile,
                                    const KeyValueStore& args,
                                    Error* error);

  // For each technology present, connect to the "best" service available,
  // as determined by sorting all services independent of their current state.
  void ConnectToBestServicesTask();

  void NotifyDefaultServiceChanged(const ServiceRefPtr& service);

  // Runs the termination actions.  If all actions complete within
  // |kTerminationActionsTimeoutMilliseconds|, |done_callback| is called with a
  // value of Error::kSuccess.  Otherwise, it is called with
  // Error::kOperationTimeout.
  void RunTerminationActions(const ResultCallback& done_callback);

  // Called when the system is about to be suspended.  Each call will be
  // followed by a call to OnSuspendDone().
  void OnSuspendImminent();

  // Called when the system has completed a suspend attempt (possibly without
  // actually suspending, in the event of the user canceling the attempt).
  void OnSuspendDone();

  // Called when the system is entering a dark resume phase (and hence a dark
  // suspend is imminent).
  void OnDarkSuspendImminent();

  void OnSuspendActionsComplete(const Error& error);
  void OnDarkResumeActionsComplete(const Error& error);

#if !defined(DISABLE_WIFI)
  void VerifyToEncryptLink(std::string public_key, std::string data,
                           ResultStringCallback cb, const Error& error,
                           bool success);
#endif  // DISABLE_WIFI

  // Return true if wifi device is enabled with no existing connection (pending
  // or connected).
  bool IsWifiIdle();

  // For unit testing.
  void set_metrics(Metrics* metrics) { metrics_ = metrics; }
  void UpdateProviderMapping();

  // Used by tests to set a mock PowerManager.  Takes ownership of
  // power_manager.
  void set_power_manager(PowerManager* power_manager) {
    power_manager_.reset(power_manager);
  }

  DeviceRefPtr GetDeviceConnectedToService(ServiceRefPtr service);

  void DeregisterDeviceByLinkName(const std::string& link_name);

  // Returns the names of all of the devices that have been claimed by the
  // current DeviceClaimer.  Returns an empty vector if no DeviceClaimer is set.
  std::vector<std::string> ClaimedDevices(Error* error);

  EventDispatcher* dispatcher_;
  const base::FilePath run_path_;
  const base::FilePath storage_path_;
  const base::FilePath user_storage_path_;
  base::FilePath user_profile_list_path_;  // Changed in tests.
  std::unique_ptr<ManagerAdaptorInterface> adaptor_;
  DeviceInfo device_info_;
#if !defined(DISABLE_CELLULAR)
  ModemInfo modem_info_;
#endif  // DISABLE_CELLULAR
#if !defined(DISABLE_WIRED_8021X)
  std::unique_ptr<EthernetEapProvider> ethernet_eap_provider_;
#endif  // DISABLE_WIRED_8021X
  std::unique_ptr<VPNProvider> vpn_provider_;
#if !defined(DISABLE_WIFI)
  std::unique_ptr<WiFiProvider> wifi_provider_;
#if defined(__BRILLO__)
  WiFiDriverHal* wifi_driver_hal_;
#endif  // __BRILLO__
#endif  // DISABLE_WIFI
#if !defined(DISABLE_WIMAX)
  std::unique_ptr<WiMaxProvider> wimax_provider_;
#endif  // DISABLE_WIMAX
  // Hold pointer to singleton Resolver instance for testing purposes.
  Resolver* resolver_;
  bool running_;
  // Used to facilitate unit tests which can't use RPC.
  bool connect_profiles_to_rpc_;
  std::vector<DeviceRefPtr> devices_;
  // We store Services in a vector, because we want to keep them sorted.
  // Services that are connected appear first in the vector.  See
  // Service::Compare() for details of the sorting criteria.
  std::vector<ServiceRefPtr> services_;
  // Map of technologies to Provider instances.  These pointers are owned
  // by the respective scoped_reptr objects that are held over the lifetime
  // of the Manager object.
  std::map<Technology::Identifier, ProviderInterface*> providers_;
  // List of startup profile names to push on the profile stack on startup.
  std::vector<ProfileRefPtr> profiles_;
  ProfileRefPtr ephemeral_profile_;
  ControlInterface* control_interface_;
  Metrics* metrics_;
  std::unique_ptr<PowerManager> power_manager_;
  std::unique_ptr<Upstart> upstart_;

  // The priority order of technologies
  std::vector<Technology::Identifier> technology_order_;

  // This is the last Service RPC Identifier for which we emitted a
  // "DefaultService" signal for.
  RpcIdentifier default_service_rpc_identifier_;

  // Manager can be optionally configured with a list of technologies to
  // do portal detection on at startup.  We need to keep track of that list
  // as well as a flag that tells us whether we should continue using it
  // instead of the configured portal list.
  std::string startup_portal_list_;
  bool use_startup_portal_list_;

  // Properties to be get/set via PropertyStore calls.
  Properties props_;
  PropertyStore store_;

  // Accept hostname supplied by the DHCP server from the specified devices.
  // eg. eth0 or eth*
  std::string accept_hostname_from_;

  base::CancelableClosure sort_services_task_;

  // Task for periodically checking various device status.
  base::CancelableClosure device_status_check_task_;

  // TODO(petkov): Currently this handles both terminate and suspend
  // actions. Rename all relevant identifiers to capture this.
  HookTable termination_actions_;

  // Is a suspend delay currently registered with the power manager?
  bool suspend_delay_registered_;

  // Whether Wake on LAN should be enabled for all Ethernet devices.
  bool is_wake_on_lan_enabled_;

  // Whether to ignore Ethernet-like devices that don't have an assigned driver.
  bool ignore_unknown_ethernet_;

  // Maps tags to callbacks for monitoring default service changes.
  std::map<int, ServiceCallback> default_service_callbacks_;
  int default_service_callback_tag_;

  // Delegate to handle destination verification operations for the manager.
  std::unique_ptr<CryptoUtilProxy> crypto_util_proxy_;

  // Stores IP addresses of some remote hosts that accept port 80 TCP
  // connections. ConnectionHealthChecker uses these IPs.
  // The store resides in Manager so that it persists across Device reset.
  std::unique_ptr<IPAddressStore> health_checker_remote_ips_;

  // Stores the most recent copy of geolocation information for each
  // technology type.
  std::map<std::string, GeolocationInfos> networks_for_geolocation_;

  // Stores the state of the highest ranked connected service.
  std::string connection_state_;

  // Stores the most recent state of all watched services.
  std::map<std::string, Service::ConnectState> watched_service_states_;

  // Device claimer is a remote application/service that claim/release devices
  // from/to shill. To reduce complexity, only allow one device claimer at a
  // time.
  std::unique_ptr<DeviceClaimer> device_claimer_;

  // When true, suppresses autoconnects in Manager::AutoConnect.
  bool suppress_autoconnect_;

  // Whether any of the services is in connected state or not.
  bool is_connected_state_;

  // List of blacklisted devices specified from command line.
  std::vector<std::string> blacklisted_devices_;

  // List of whitelisted devices specified from command line.
  std::vector<std::string> whitelisted_devices_;

  // List of DHCPv6 enabled devices.
  std::vector<std::string> dhcpv6_enabled_devices_;

  // DhcpProperties stored for the default profile.
  std::unique_ptr<DhcpProperties> dhcp_properties_;

  DISALLOW_COPY_AND_ASSIGN(Manager);
};

}  // namespace shill

#endif  // SHILL_MANAGER_H_
