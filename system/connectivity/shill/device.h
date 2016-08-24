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

#ifndef SHILL_DEVICE_H_
#define SHILL_DEVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/adaptor_interfaces.h"
#include "shill/callbacks.h"
#include "shill/connection_diagnostics.h"
#include "shill/connection_tester.h"
#include "shill/connectivity_trial.h"
#include "shill/dns_server_tester.h"
#include "shill/event_dispatcher.h"
#include "shill/ipconfig.h"
#include "shill/net/ip_address.h"
#include "shill/net/shill_time.h"
#include "shill/portal_detector.h"
#include "shill/property_store.h"
#include "shill/refptr_types.h"
#include "shill/service.h"
#include "shill/technology.h"

namespace shill {

class ControlInterface;
class DHCPProvider;
class DeviceAdaptorInterface;
class Endpoint;
class Error;
class EventDispatcher;
class GeolocationInfo;
class LinkMonitor;
class Manager;
class Metrics;
class RTNLHandler;
class TrafficMonitor;

// Device superclass.  Individual network interfaces types will inherit from
// this class.
class Device : public base::RefCounted<Device> {
 public:
  // Progressively scanning for access points (APs) is done with multiple scans,
  // each containing a group of channels.  The scans are performed in order of
  // decreasing likelihood of connecting on one of the channels in a group
  // (the number of channels in a group is a matter for system tuning).  Fully
  // scanning for APs does a complete scan of all the channels in a single scan.
  // Progressive scanning is supported for wifi devices; technologies that
  // support scan but don't support progressive scan will always perform a full
  // scan, regardless of the requested scan type.
  enum ScanType { kProgressiveScan, kFullScan };

  // A constructor for the Device object
  Device(ControlInterface* control_interface,
         EventDispatcher* dispatcher,
         Metrics* metrics,
         Manager* manager,
         const std::string& link_name,
         const std::string& address,
         int interface_index,
         Technology::Identifier technology);

  // Initialize type-specific network interface properties.
  virtual void Initialize();

  // Enable or disable the device. This is a convenience method for
  // cases where we want to SetEnabledNonPersistent, but don't care
  // about the results.
  virtual void SetEnabled(bool enable);
  // Enable or disable the device. Unlike SetEnabledPersistent, it does not
  // save the setting in the profile.
  //
  // TODO(quiche): Replace both of the next two methods with calls to
  // SetEnabledChecked.
  virtual void SetEnabledNonPersistent(bool enable,
                                       Error* error,
                                       const ResultCallback& callback);
  // Enable or disable the device, and save the setting in the profile.
  // The setting is persisted before the enable or disable operation
  // starts, so that even if it fails, the user's intent is still recorded
  // for the next time shill restarts.
  virtual void SetEnabledPersistent(bool enable,
                                    Error* error,
                                    const ResultCallback& callback);
  // Enable or disable the Device, depending on |enable|.
  // Save the new setting to the profile, if |persist| is true.
  // Report synchronous errors using |error|, and asynchronous completion
  // with |callback|.
  virtual void SetEnabledChecked(bool enable,
                                 bool persist,
                                 Error* error,
                                 const ResultCallback& callback);
  // Similar to SetEnabledChecked, but without sanity checking, and
  // without saving the new value of |enable| to the profile. If you
  // are sane (i.e. not Cellular), you should use
  // SetEnabledChecked instead.
  virtual void SetEnabledUnchecked(bool enable,
                                   Error* error,
                                   const ResultCallback& callback);

  // Returns true if the underlying device reports that it is already enabled.
  // Used when the device is registered with the Manager, so that shill can
  // sync its state/ with the true state of the device. The default is to
  // report false.
  virtual bool IsUnderlyingDeviceEnabled() const;

  virtual void LinkEvent(unsigned flags, unsigned change);

  // The default implementation sets |error| to kNotSupported.
  virtual void Scan(ScanType scan_type, Error* error,
                    const std::string& reason);
  // The default implementation sets |error| to kNotSupported.
  virtual void SetSchedScan(bool enable, Error* error);
  virtual void RegisterOnNetwork(const std::string& network_id, Error* error,
                                 const ResultCallback& callback);
  virtual void RequirePIN(const std::string& pin, bool require,
                          Error* error, const ResultCallback& callback);
  virtual void EnterPIN(const std::string& pin,
                        Error* error, const ResultCallback& callback);
  virtual void UnblockPIN(const std::string& unblock_code,
                          const std::string& pin,
                          Error* error, const ResultCallback& callback);
  virtual void ChangePIN(const std::string& old_pin,
                         const std::string& new_pin,
                         Error* error, const ResultCallback& callback);
  virtual void Reset(Error* error, const ResultCallback& callback);

  virtual void SetCarrier(const std::string& carrier,
                          Error* error, const ResultCallback& callback);

  // Returns true if IPv6 is allowed and should be enabled when the device
  // tries to acquire an IP configuration. The default implementation allows
  // IPv6, which can be overridden by a derived class.
  virtual bool IsIPv6Allowed() const;

  virtual void DisableIPv6();
  virtual void EnableIPv6();
  virtual void EnableIPv6Privacy();

  // Returns true if the selected service on the device (if any) is connected.
  // Returns false if there is no selected service, or if the selected service
  // is not connected.
  bool IsConnected() const;

  // Called by Device so that subclasses can run hooks on the selected service
  // getting an IP.  Subclasses should call up to the parent first.
  virtual void OnConnected();

  // Called by the Connection so that the Device can update the service sorting
  // after one connection is bound to another.
  virtual void OnConnectionUpdated();

  // Returns true if the selected service on the device (if any) is connected
  // and matches the passed-in argument |service|.  Returns false if there is
  // no connected service, or if it does not match |service|.
  virtual bool IsConnectedToService(const ServiceRefPtr& service) const;

  // Returns true if the DHCP parameters provided indicate that we are tethered
  // to a mobile device.
  virtual bool IsConnectedViaTether() const;

  // Restart the portal detection process on a connected device.  This is
  // useful if the properties on the connected service have changed in a
  // way that may affect the decision to run portal detection at all.
  // Returns true if portal detection was started.
  virtual bool RestartPortalDetection();

  // Called by the manager to start a single connectivity test.  This is used to
  // log connection state triggered by a user feedback log request.
  virtual bool StartConnectivityTest();

  // Get receive and transmit byte counters.
  virtual uint64_t GetReceiveByteCount();
  virtual uint64_t GetTransmitByteCount();

  // Perform a TDLS |operation| on the underlying device, with respect
  // to a given |peer|.  The string returned is empty for any operation
  // other than kTDLSOperationStatus, which returns the state of the
  // TDLS link with |peer|.  This method is only valid for WiFi devices,
  // but needs to be declared here since it is part of the Device RPC
  // API.
  virtual std::string PerformTDLSOperation(const std::string& operation,
                                           const std::string& peer,
                                           Error* error);

  // Reset the persisted byte counters associated with the device.
  void ResetByteCounters();

  // Requests that portal detection be done, if this device has the default
  // connection.  Returns true if portal detection was started.
  virtual bool RequestPortalDetection();

  std::string GetRpcIdentifier() const;
  std::string GetStorageIdentifier() const;

  // Returns a list of Geolocation objects. Each object is multiple
  // key-value pairs representing one entity that can be used for
  // Geolocation.
  virtual std::vector<GeolocationInfo> GetGeolocationObjects() const;

  // Enable or disable this interface to receive packets even if it is not
  // the default connection.  This is useful in limited situations such as
  // during portal detection.
  virtual void SetLooseRouting(bool is_loose_routing);

  // Enable or disable same-net multi-home support for this interface.  When
  // enabled, ARP filtering is enabled in order to avoid the "ARP Flux"
  // effect where peers may end up with inaccurate IP address mappings due to
  // the default Linux ARP transmit / reply behavior.  See
  // http://linux-ip.net/html/ether-arp.html for more details on this effect.
  virtual void SetIsMultiHomed(bool is_multi_homed);

  const std::string& address() const { return hardware_address_; }
  const std::string& link_name() const { return link_name_; }
  int interface_index() const { return interface_index_; }
  virtual const ConnectionRefPtr& connection() const { return connection_; }
  bool enabled() const { return enabled_; }
  bool enabled_persistent() const { return enabled_persistent_; }
  virtual Technology::Identifier technology() const { return technology_; }
  std::string GetTechnologyString(Error* error);

  virtual const IPConfigRefPtr& ipconfig() const { return ipconfig_; }
  virtual const IPConfigRefPtr& ip6config() const { return ip6config_; }
  virtual const IPConfigRefPtr& dhcpv6_config() const { return dhcpv6_config_; }
  void set_ipconfig(const IPConfigRefPtr& config) { ipconfig_ = config; }

  const std::string& FriendlyName() const;

  // Returns a string that is guaranteed to uniquely identify this Device
  // instance.
  const std::string& UniqueName() const;

  PropertyStore* mutable_store() { return &store_; }
  const PropertyStore& store() const { return store_; }
  RTNLHandler* rtnl_handler() { return rtnl_handler_; }
  bool running() const { return running_; }

  EventDispatcher* dispatcher() const { return dispatcher_; }

  // Load configuration for the device from |storage|.  This may include
  // instantiating non-visible services for which configuration has been
  // stored.
  virtual bool Load(StoreInterface* storage);

  // Save configuration for the device to |storage|.
  virtual bool Save(StoreInterface* storage);

  void set_dhcp_provider(DHCPProvider* provider) { dhcp_provider_ = provider; }

  DeviceAdaptorInterface* adaptor() const { return adaptor_.get(); }

  // Suspend event handler. Called by Manager before the system
  // suspends. This handler, along with any other suspend handlers,
  // will have Manager::kTerminationActionsTimeoutMilliseconds to
  // execute before the system enters the suspend state. |callback|
  // must be invoked after all synchronous and/or asynchronous actions
  // this function performs complete. Code that needs to run on exit should use
  // Manager::AddTerminationAction, rather than OnBeforeSuspend.
  //
  // The default implementation invokes the |callback| immediately, since
  // there is nothing to be done in the general case.
  virtual void OnBeforeSuspend(const ResultCallback& callback);

  // Resume event handler. Called by Manager as the system resumes.
  // The base class implementation takes care of renewing a DHCP lease
  // (if necessary). Derived classes may implement any technology
  // specific requirements by overriding, but should include a call to
  // the base class implementation.
  virtual void OnAfterResume();

  // This method is invoked when the system resumes from suspend temporarily in
  // the "dark resume" state. The system will reenter suspend in
  // Manager::kTerminationActionsTimeoutMilliseconds. |callback| must be invoked
  // after all synchronous and/or asynchronous actions this function performs
  // and/or posts complete.
  //
  // The default implementation invokes the |callback| immediately, since
  // there is nothing to be done in the general case.
  virtual void OnDarkResume(const ResultCallback& callback);

  // Destroy the lease, if any, with this |name|.
  // Called by the service during Unload() as part of the cleanup sequence.
  virtual void DestroyIPConfigLease(const std::string& name);

  // Called by DeviceInfo when the kernel adds or removes a globally-scoped
  // IPv6 address from this interface.
  virtual void OnIPv6AddressChanged();

  // Called by DeviceInfo when the kernel receives a update for IPv6 DNS server
  // addresses from this interface.
  virtual void OnIPv6DnsServerAddressesChanged();

  // Called when link becomes unreliable (multiple link monitor failures
  // detected in short period of time).
  virtual void OnUnreliableLink();

  // Called when link becomes reliable (no link failures in a predefined period
  // of time).
  virtual void OnReliableLink();

  // Program a rule into the NIC to wake the system from suspend upon receiving
  // packets from |ip_endpoint|. |error| indicates the result of the
  // operation.
  virtual void AddWakeOnPacketConnection(const std::string& ip_endpoint,
                                         Error* error);
  // Removes a rule previously programmed into the NIC to wake the system from
  // suspend upon receiving packets from |ip_endpoint|. |error| indicates the
  // result of the operation.
  virtual void RemoveWakeOnPacketConnection(const std::string& ip_endpoint,
                                            Error* error);
  // Removes all wake-on-packet rules programmed into the NIC. |error| indicates
  // the result of the operation.
  virtual void RemoveAllWakeOnPacketConnections(Error* error);

  // Initiate renewal of existing DHCP lease.
  void RenewDHCPLease();

  // Resolve the |input| string into a MAC address for a peer local to this
  // device. This could be a trivial operation if the |input| is already a MAC
  // address, or could involve an ARP table lookup.  Returns true and populates
  // |output| if the resolution completes, otherwise returns false and
  // populates |error|.
  virtual bool ResolvePeerMacAddress(const std::string& input,
                                     std::string* output,
                                     Error* error);

  // Creates a byte vector from a colon-separated hardware address string.
  static std::vector<uint8_t> MakeHardwareAddressFromString(
      const std::string& address_string);

  // Creates a colon-separated hardware address string from a byte vector.
  static std::string MakeStringFromHardwareAddress(
      const std::vector<uint8_t>& address_data);

  // Request the WiFi device to roam to AP with |addr|.
  // This call will send Roam command to wpa_supplicant.
  virtual bool RequestRoam(const std::string& addr, Error* error);

 protected:
  friend class base::RefCounted<Device>;
  friend class DeviceHealthCheckerTest;
  FRIEND_TEST(CellularServiceTest, IsAutoConnectable);
  FRIEND_TEST(CellularTest, EnableTrafficMonitor);
  FRIEND_TEST(CellularTest, ModemStateChangeDisable);
  FRIEND_TEST(CellularTest, UseNoArpGateway);
  FRIEND_TEST(DeviceHealthCheckerTest, HealthCheckerPersistsAcrossDeviceReset);
  FRIEND_TEST(DeviceHealthCheckerTest, RequestConnectionHealthCheck);
  FRIEND_TEST(DeviceHealthCheckerTest, SetupHealthChecker);
  FRIEND_TEST(DevicePortalDetectionTest, RequestStartConnectivityTest);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithoutSelectedService);
  FRIEND_TEST(DeviceTest, AcquireIPConfigWithSelectedService);
  FRIEND_TEST(DeviceTest, AvailableIPConfigs);
  FRIEND_TEST(DeviceTest, DestroyIPConfig);
  FRIEND_TEST(DeviceTest, DestroyIPConfigNULL);
  FRIEND_TEST(DeviceTest, ConfigWithMinimumMTU);
  FRIEND_TEST(DeviceTest, EnableIPv6);
  FRIEND_TEST(DeviceTest, GetProperties);
  FRIEND_TEST(DeviceTest, IPConfigUpdatedFailureWithIPv6Config);
  FRIEND_TEST(DeviceTest, IPConfigUpdatedFailureWithIPv6Connection);
  FRIEND_TEST(DeviceTest, IsConnectedViaTether);
  FRIEND_TEST(DeviceTest, LinkMonitorFailure);
  FRIEND_TEST(DeviceTest, Load);
  FRIEND_TEST(DeviceTest, OnDHCPv6ConfigExpired);
  FRIEND_TEST(DeviceTest, OnDHCPv6ConfigFailed);
  FRIEND_TEST(DeviceTest, OnDHCPv6ConfigUpdated);
  FRIEND_TEST(DeviceTest, OnIPv6AddressChanged);
  FRIEND_TEST(DeviceTest, OnIPv6ConfigurationCompleted);
  FRIEND_TEST(DeviceTest, OnIPv6DnsServerAddressesChanged);
  FRIEND_TEST(DeviceTest,
              OnIPv6DnsServerAddressesChanged_LeaseExpirationUpdated);
  FRIEND_TEST(DeviceTest, PrependIPv4DNSServers);
  FRIEND_TEST(DeviceTest, PrependIPv6DNSServers);
  FRIEND_TEST(DeviceTest, Save);
  FRIEND_TEST(DeviceTest, SelectedService);
  FRIEND_TEST(DeviceTest, SetEnabledNonPersistent);
  FRIEND_TEST(DeviceTest, SetEnabledPersistent);
  FRIEND_TEST(DeviceTest, SetServiceConnectedState);
  FRIEND_TEST(DeviceTest, ShouldUseArpGateway);
  FRIEND_TEST(DeviceTest, Start);
  FRIEND_TEST(DeviceTest, StartTrafficMonitor);
  FRIEND_TEST(DeviceTest, Stop);
  FRIEND_TEST(DeviceTest, StopTrafficMonitor);
  FRIEND_TEST(ManagerTest, ConnectedTechnologies);
  FRIEND_TEST(ManagerTest, DefaultTechnology);
  FRIEND_TEST(ManagerTest, DeviceRegistrationAndStart);
  FRIEND_TEST(ManagerTest, GetEnabledDeviceWithTechnology);
  FRIEND_TEST(ManagerTest, SetEnabledStateForTechnology);
  FRIEND_TEST(ManagerTest, GetEnabledDeviceByLinkName);
  FRIEND_TEST(PPPDeviceTest, UpdateIPConfigFromPPP);
  FRIEND_TEST(WiFiMainTest, Connect);
  FRIEND_TEST(WiFiMainTest, UseArpGateway);
  FRIEND_TEST(WiMaxTest, ConnectTimeout);
  FRIEND_TEST(WiMaxTest, UseNoArpGateway);

  virtual ~Device();

  // Each device must implement this method to do the work needed to
  // enable the device to operate for establishing network connections.
  // The |error| argument, if not nullptr,
  // will refer to an Error that starts out with the value
  // Error::kOperationInitiated. This reflects the assumption that
  // enable (and disable) operations will usually be non-blocking,
  // and their completion will be indicated by means of an asynchronous
  // reply sometime later. There are two circumstances in which a
  // device's Start() method may overwrite |error|:
  //
  // 1. If an early failure is detected, such that the non-blocking
  //    part of the operation never takes place, then |error| should
  //    be set to the appropriate value corresponding to the type
  //    of failure. This is the "immediate failure" case.
  // 2. If the device is enabled without performing any non-blocking
  //    steps, then |error| should be Reset, i.e., its value set
  //    to Error::kSuccess. This is the "immediate success" case.
  //
  // In these two cases, because completion is immediate, |callback|
  // is not used. If neither of these two conditions holds, then |error|
  // should not be modified, and |callback| should be passed to the
  // method that will initiate the non-blocking operation.
  virtual void Start(Error* error,
                     const EnabledStateChangedCallback& callback) = 0;

  // Each device must implement this method to do the work needed to
  // disable the device, i.e., clear any running state, and make the
  // device no longer capable of establishing network connections.
  // The discussion for Start() regarding the use of |error| and
  // |callback| apply to Stop() as well.
  virtual void Stop(Error* error,
                    const EnabledStateChangedCallback& callback) = 0;

  // The EnabledStateChangedCallback that gets passed to the device's
  // Start() and Stop() methods is bound to this method. |callback|
  // is the callback that was passed to SetEnabled().
  void OnEnabledStateChanged(const ResultCallback& callback,
                             const Error& error);

  // Drops the currently selected service along with its IP configuration and
  // connection, if any.
  virtual void DropConnection();

  // If there's an IP configuration in |ipconfig_|, releases the IP address and
  // destroys the configuration instance.
  void DestroyIPConfig();

  // Creates a new DHCP IP configuration instance, stores it in |ipconfig_| and
  // requests a new IP configuration.  Saves the DHCP lease to the generic
  // lease filename based on the interface name.  Registers a callback to
  // IPConfigUpdatedCallback on IP configuration changes. Returns true if the IP
  // request was successfully sent.
  bool AcquireIPConfig();

  // Creates a new DHCP IP configuration instance, stores it in |ipconfig_| and
  // requests a new IP configuration.  Saves the DHCP lease to a filename
  // based on the passed-in |lease_name|.  Registers a callback to
  // IPConfigUpdatedCallback on IP configuration changes. Returns true if the IP
  // request was successfully sent.
  bool AcquireIPConfigWithLeaseName(const std::string& lease_name);

#ifndef DISABLE_DHCPV6
  // Creates a new DHCPv6 configuration instances, stores it in
  // |dhcpv6_config_| and requests a new configuration.  Saves the DHCPv6
  // lease to a filename based on the passed-in |lease_name|.
  // The acquired configurations will not be used to setup a connection
  // for the device.
  bool AcquireIPv6ConfigWithLeaseName(const std::string& lease_name);
#endif

  // Assigns the IP configuration |properties| to |ipconfig_|.
  void AssignIPConfig(const IPConfig::Properties& properties);

  // Callback invoked on successful IP configuration updates.
  virtual void OnIPConfigUpdated(const IPConfigRefPtr& ipconfig,
                                 bool new_lease_acquired);

  // Called when IPv6 configuration changes.
  virtual void OnIPv6ConfigUpdated();

  // Callback invoked on IP configuration failures.
  void OnIPConfigFailed(const IPConfigRefPtr& ipconfig);

  // Callback invoked when "Refresh" is invoked on an IPConfig.  This usually
  // signals a change in static IP parameters.
  void OnIPConfigRefreshed(const IPConfigRefPtr& ipconfig);

  // Callback invoked when an IPConfig restarts due to lease expiry.  This
  // is advisory, since an "Updated" or "Failed" signal is guaranteed to
  // follow.
  void OnIPConfigExpired(const IPConfigRefPtr& ipconfig);

  // Called by Device so that subclasses can run hooks on the selected service
  // failing to get an IP.  The default implementation disconnects the selected
  // service with Service::kFailureDHCP.
  virtual void OnIPConfigFailure();

  // Callback invoked on successful DHCPv6 configuration updates.
  void OnDHCPv6ConfigUpdated(const IPConfigRefPtr& ipconfig,
                             bool new_lease_acquired);

  // Callback invoked on DHCPv6 configuration failures.
  void OnDHCPv6ConfigFailed(const IPConfigRefPtr& ipconfig);

  // Callback invoked when an DHCPv6Config restarts due to lease expiry.  This
  // is advisory, since an "Updated" or "Failed" signal is guaranteed to
  // follow.
  void OnDHCPv6ConfigExpired(const IPConfigRefPtr& ipconfig);

  // Maintain connection state (Routes, IP Addresses and DNS) in the OS.
  void CreateConnection();

  // Remove connection state
  void DestroyConnection();

  // Selects a service to be "current" -- i.e. link-state or configuration
  // events that happen to the device are attributed to this service.
  void SelectService(const ServiceRefPtr& service);

  // Set the state of the |selected_service_|.
  virtual void SetServiceState(Service::ConnectState state);

  // Set the failure of the selected service (implicitly sets the state to
  // "failure").
  virtual void SetServiceFailure(Service::ConnectFailure failure_state);

  // Records the failure mode and time of the selected service, and
  // sets the Service state of the selected service to "Idle".
  // Avoids showing a failure mole in the UI.
  virtual void SetServiceFailureSilent(Service::ConnectFailure failure_state);

  // Called by the Portal Detector whenever a trial completes.  Device
  // subclasses that choose unique mappings from portal results to connected
  // states can override this method in order to do so.
  virtual void PortalDetectorCallback(const PortalDetector::Result& result);

  // Initiate portal detection, if enabled for this device type.
  bool StartPortalDetection();

  // Stop portal detection if it is running.
  void StopPortalDetection();

  // Initiate connection diagnostics with the |result| from a completed portal
  // detection attempt.
  virtual bool StartConnectionDiagnosticsAfterPortalDetection(
      const PortalDetector::Result& result);

  // Stop connection diagnostics if it is running.
  void StopConnectionDiagnostics();

  // Stop connectivity tester if it exists.
  void StopConnectivityTest();

  // Initiate link monitoring, if enabled for this device type.
  bool StartLinkMonitor();

  // Stop link monitoring if it is running.
  void StopLinkMonitor();

  // Respond to a LinkMonitor failure in a Device-specific manner.
  virtual void OnLinkMonitorFailure();

  // Respond to a LinkMonitor gateway's MAC address found/change event.
  virtual void OnLinkMonitorGatewayChange();

  // Returns true if traffic monitor is enabled on this device. The default
  // implementation will return false, which can be overridden by a derived
  // class.
  virtual bool IsTrafficMonitorEnabled() const;

  // Initiates traffic monitoring on the device if traffic monitor is enabled.
  void StartTrafficMonitor();

  // Stops traffic monitoring on the device if traffic monitor is enabled.
  void StopTrafficMonitor();

  // Start DNS test for the given servers. When retry_until_success is set,
  // callback will only be invoke when the test succeed or the test failed to
  // start (internal error). This function will return false if there is a test
  // that's already running, and true otherwise.
  virtual bool StartDNSTest(
      const std::vector<std::string>& dns_servers,
      const bool retry_until_success,
      const base::Callback<void(const DNSServerTester::Status)>& callback);
  // Stop DNS test if one is running.
  virtual void StopDNSTest();

  // Timer function for monitoring IPv6 DNS server's lifetime.
  void StartIPv6DNSServerTimer(uint32_t lifetime_seconds);
  void StopIPv6DNSServerTimer();

  // Stop all monitoring/testing activities on this device. Called when tearing
  // down or changing network connection on the device.
  void StopAllActivities();

  // Called by the Traffic Monitor when it detects a network problem. Device
  // subclasses that want to roam to a different network when encountering
  // network problems can override this method in order to do so. The parent
  // implementation handles the metric reporting of the network problem.
  virtual void OnEncounterNetworkProblem(int reason);

  // Set the state of the selected service, with checks to make sure
  // the service is already in a connected state before doing so.
  void SetServiceConnectedState(Service::ConnectState state);

  // Specifies whether an ARP gateway should be used for the
  // device technology.
  virtual bool ShouldUseArpGateway() const;

  // Indicates if the selected service is configured with a static IP address.
  bool IsUsingStaticIP() const;

  // Indicates if the selected service is configured with static nameservers.
  bool IsUsingStaticNameServers() const;

  const ServiceRefPtr& selected_service() const { return selected_service_; }

  void HelpRegisterConstDerivedString(
      const std::string& name,
      std::string(Device::*get)(Error*));

  void HelpRegisterConstDerivedRpcIdentifier(
      const std::string& name,
      RpcIdentifier(Device::*get)(Error*));

  void HelpRegisterConstDerivedRpcIdentifiers(
      const std::string& name,
      RpcIdentifiers(Device::*get)(Error*));

  void HelpRegisterConstDerivedUint64(
      const std::string& name,
      uint64_t(Device::*get)(Error*));

  // Called by the ConnectionTester whenever a connectivity test completes.
  virtual void ConnectionTesterCallback();

  // Property getters reserved for subclasses
  ControlInterface* control_interface() const { return control_interface_; }
  Metrics* metrics() const { return metrics_; }
  Manager* manager() const { return manager_; }
  const LinkMonitor* link_monitor() const { return link_monitor_.get(); }
  void set_link_monitor(LinkMonitor* link_monitor);
  // Use for unit test.
  void set_traffic_monitor(TrafficMonitor* traffic_monitor);

  // Calculates the time (in seconds) till a DHCP lease is due for renewal,
  // and stores this value in |result|. Returns false is there is no upcoming
  // DHCP lease renewal, true otherwise.
  bool TimeToNextDHCPLeaseRenewal(uint32_t* result);

 private:
  friend class CellularCapabilityTest;
  friend class CellularTest;
  friend class DeviceAdaptorInterface;
  friend class DeviceByteCountTest;
  friend class DevicePortalDetectionTest;
  friend class DeviceTest;
  friend class EthernetTest;
  friend class OpenVPNDriverTest;
  friend class TestDevice;
  friend class VirtualDeviceTest;
  friend class WiFiObjectTest;

  static const char kIPFlagTemplate[];
  static const char kIPFlagVersion4[];
  static const char kIPFlagVersion6[];
  static const char kIPFlagDisableIPv6[];
  static const char kIPFlagUseTempAddr[];
  static const char kIPFlagUseTempAddrUsedAndDefault[];
  static const char kIPFlagReversePathFilter[];
  static const char kIPFlagReversePathFilterEnabled[];
  static const char kIPFlagReversePathFilterLooseMode[];
  static const char kIPFlagArpAnnounce[];
  static const char kIPFlagArpAnnounceDefault[];
  static const char kIPFlagArpAnnounceBestLocal[];
  static const char kIPFlagArpIgnore[];
  static const char kIPFlagArpIgnoreDefault[];
  static const char kIPFlagArpIgnoreLocalOnly[];
  static const char kStoragePowered[];
  static const char kStorageReceiveByteCount[];
  static const char kStorageTransmitByteCount[];
  static const char kFallbackDnsTestHostname[];
  static const char* kFallbackDnsServers[];
  static const int kDNSTimeoutMilliseconds;

  // Maximum seconds between two link monitor failures to declare this link
  // (network) as unreliable.
  static const int kLinkUnreliableThresholdSeconds;

  static const size_t kHardwareAddressLength;

  // Configure static IP address parameters if the service provides them.
  void ConfigureStaticIPTask();

  // Right now, Devices reference IPConfigs directly when persisted to disk
  // It's not clear that this makes sense long-term, but that's how it is now.
  // This call generates a string in the right format for this persisting.
  // |suffix| is injected into the storage identifier used for the configs.
  std::string SerializeIPConfigs(const std::string& suffix);

  // Set an IP configuration flag on the device. |family| should be "ipv6" or
  // "ipv4". |flag| should be the name of the flag to be set and |value| is
  // what this flag should be set to. Overridden by unit tests to pretend
  // writing to procfs.
  virtual bool SetIPFlag(IPAddress::Family family,
                         const std::string& flag,
                         const std::string& value);

  // Request the removal of reverse-path filtering for this interface.
  // This will allow packets destined for this interface to be accepted,
  // even if this is not the default route for such a packet to arrive.
  void DisableReversePathFilter();

  // Request reverse-path filtering for this interface.
  void EnableReversePathFilter();

  // Disable ARP filtering on the device.  The interface will exhibit the
  // default Linux behavior -- incoming ARP requests are responded to by all
  // interfaces.  Outgoing ARP requests can contain any local address.
  void DisableArpFiltering();

  // Enable ARP filtering on the device.  Incoming ARP requests are responded
  // to only by the interface(s) owning the address.  Outgoing ARP requests
  // will contain the best local address for the target.
  void EnableArpFiltering();

  std::string GetSelectedServiceRpcIdentifier(Error* error);
  std::vector<std::string> AvailableIPConfigs(Error* error);

  // Get the LinkMonitor's average response time.
  uint64_t GetLinkMonitorResponseTime(Error* error);

  // Get receive and transmit byte counters. These methods simply wrap
  // GetReceiveByteCount and GetTransmitByteCount in order to be used by
  // HelpRegisterConstDerivedUint64.
  uint64_t GetReceiveByteCountProperty(Error* error);
  uint64_t GetTransmitByteCountProperty(Error* error);

  // Emit a property change signal for the "IPConfigs" property of this device.
  void UpdateIPConfigsProperty();

  // Called by DNS server tester when the fallback DNS servers test completes.
  void FallbackDNSResultCallback(const DNSServerTester::Status status);

  // Called by DNS server tester when the configured DNS servers test completes.
  void ConfigDNSResultCallback(const DNSServerTester::Status status);

  // Update DNS setting with the given DNS servers for the current connection.
  void SwitchDNSServers(const std::vector<std::string>& dns_servers);

  // Called when the lifetime for IPv6 DNS server expires.
  void IPv6DNSServerExpired();

  // Return true if given IP configuration contain both IP address and DNS
  // servers. Hence, ready to be used for network connection.
  bool IPConfigCompleted(const IPConfigRefPtr& ipconfig);

  // Setup network connection with given IP configuration, and start portal
  // detection on that connection.
  void SetupConnection(const IPConfigRefPtr& ipconfig);

  // Set the system hostname to |hostname| if this device is configured to
  // do so.  If |hostname| is too long, truncate this parameter to fit within
  // the maximum hostname size.
  bool SetHostname(const std::string& hostname);

  // Prepend the Manager's configured list of DNS servers into |ipconfig|
  // ensuring that only DNS servers of the same address family as |ipconfig| are
  // included in the final list.
  void PrependDNSServersIntoIPConfig(const IPConfigRefPtr& ipconfig);

  // Mutate |servers| to include the Manager's prepended list of DNS servers for
  // |family|.  On return, it is guaranteed that there are no duplicate entries
  // in |servers|.
  void PrependDNSServers(const IPAddress::Family family,
                         std::vector<std::string>* servers);

  // Called by |connection_diagnostics| after diagnostics have finished.
  void ConnectionDiagnosticsCallback(
      const std::string& connection_issue,
      const std::vector<ConnectionDiagnostics::Event>& diagnostic_events);

  // |enabled_persistent_| is the value of the Powered property, as
  // read from the profile. If it is not found in the profile, it
  // defaults to true. |enabled_| reflects the real-time state of
  // the device, i.e., enabled or disabled. |enabled_pending_| reflects
  // the target state of the device while an enable or disable operation
  // is occurring.
  //
  // Some typical sequences for these state variables are shown below.
  //
  // Shill starts up, profile has been read:
  //  |enabled_persistent_|=true   |enabled_|=false   |enabled_pending_|=false
  //
  // Shill acts on the value of |enabled_persistent_|, calls SetEnabled(true):
  //  |enabled_persistent_|=true   |enabled_|=false   |enabled_pending_|=true
  //
  // SetEnabled completes successfully, device is enabled:
  //  |enabled_persistent_|=true   |enabled_|=true    |enabled_pending_|=true
  //
  // User presses "Disable" button, SetEnabled(false) is called:
  //  |enabled_persistent_|=false   |enabled_|=true    |enabled_pending_|=false
  //
  // SetEnabled completes successfully, device is disabled:
  //  |enabled_persistent_|=false   |enabled_|=false    |enabled_pending_|=false
  bool enabled_;
  bool enabled_persistent_;
  bool enabled_pending_;

  // Other properties
  bool reconnect_;
  const std::string hardware_address_;

  PropertyStore store_;

  const int interface_index_;
  bool running_;  // indicates whether the device is actually in operation
  const std::string link_name_;
  const std::string unique_id_;
  ControlInterface* control_interface_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;
  IPConfigRefPtr ipconfig_;
  IPConfigRefPtr ip6config_;
  IPConfigRefPtr dhcpv6_config_;
  ConnectionRefPtr connection_;
  base::WeakPtrFactory<Device> weak_ptr_factory_;
  std::unique_ptr<DeviceAdaptorInterface> adaptor_;
  std::unique_ptr<PortalDetector> portal_detector_;
  std::unique_ptr<LinkMonitor> link_monitor_;
  // Used for verifying whether DNS server is functional.
  std::unique_ptr<DNSServerTester> dns_server_tester_;
  base::Callback<void(const PortalDetector::Result&)>
      portal_detector_callback_;
  // Callback to invoke when IPv6 DNS servers lifetime expired.
  base::CancelableClosure ipv6_dns_server_expired_callback_;
  std::unique_ptr<TrafficMonitor> traffic_monitor_;
  // DNS servers obtained from ipconfig (either from DHCP or static config)
  // that are not working.
  std::vector<std::string> config_dns_servers_;
  Technology::Identifier technology_;
  // The number of portal detection attempts from Connected to Online state.
  // This includes all failure/timeout attempts and the final successful
  // attempt.
  int portal_attempts_to_online_;

  // Keep track of the offset between the interface-reported byte counters
  // and our persisted value.
  uint64_t receive_byte_offset_;
  uint64_t transmit_byte_offset_;

  // Maintain a reference to the connected / connecting service
  ServiceRefPtr selected_service_;

  // Cache singleton pointers for performance and test purposes.
  DHCPProvider* dhcp_provider_;
  RTNLHandler* rtnl_handler_;

  // Time when link monitor last failed.
  Time* time_;
  time_t last_link_monitor_failed_time_;
  // Callback to invoke when link becomes reliable again after it was previously
  // unreliable.
  base::CancelableClosure reliable_link_callback_;

  std::unique_ptr<ConnectionTester> connection_tester_;
  base::Callback<void()> connection_tester_callback_;

  // Track whether packets from non-optimal routes will be accepted by this
  // device.  This is referred to as "loose mode" (see RFC3704).
  bool is_loose_routing_;

  // Track the current same-net multi-home state.
  bool is_multi_homed_;

  // Remember which flag files were previously successfully written.
  std::set<std::string> written_flags_;

  std::unique_ptr<ConnectionDiagnostics> connection_diagnostics_;
  base::Callback<void(const std::string&,
                      const std::vector<ConnectionDiagnostics::Event>&)>
      connection_diagnostics_callback_;

  DISALLOW_COPY_AND_ASSIGN(Device);
};

}  // namespace shill

#endif  // SHILL_DEVICE_H_
