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

#ifndef SHILL_WIFI_WIFI_H_
#define SHILL_WIFI_WIFI_H_

// A WiFi device represents a wireless network interface implemented as an IEEE
// 802.11 station.  An Access Point (AP) (or, more correctly, a Basic Service
// Set(BSS)) is represented by a WiFiEndpoint.  An AP provides a WiFiService,
// which is the same concept as Extended Service Set (ESS) in 802.11,
// identified by an SSID.  A WiFiService includes zero or more WiFiEndpoints
// that provide that service.
//
// A WiFi device interacts with a real device through WPA Supplicant.
// Wifi::Start() creates a connection to WPA Supplicant, represented by
// |supplicant_interface_proxy_|.  [1]
//
// A WiFi device becomes aware of WiFiEndpoints through BSSAdded signals from
// WPA Supplicant, which identifies them by a "path".  The WiFi object maintains
// an EndpointMap in |endpoint_by_rpcid_|, in which the key is the "path" and
// the value is a pointer to a WiFiEndpoint object.  When a WiFiEndpoint is
// added, it is associated with a WiFiService.
//
// The WiFi device connects to a WiFiService, not a WiFiEndpoint, through WPA
// Supplicant. It is the job of WPA Supplicant to select a BSS (aka
// WiFiEndpoint) to connect to.  The protocol for establishing a connection is
// as follows:
//
//  1.  The WiFi device sends AddNetwork to WPA Supplicant, which returns a
//  "network path" when done.
//
//  2.  The WiFi device sends SelectNetwork, indicating the network path
//  received in 1, to WPA Supplicant, which begins the process of associating
//  with an AP in the ESS.  At this point the WiFiService which is being
//  connected is called the |pending_service_|.
//
//  3.  During association to an EAP-TLS network, WPA Supplicant can send
//  multiple "Certification" events, which provide information about the
//  identity of the remote entity.
//
//  4.  When association is complete, WPA Supplicant sends a PropertiesChanged
//  signal to the WiFi device, indicating a change in the CurrentBSS.  The
//  WiFiService indicated by the new value of CurrentBSS is set as the
//  |current_service_|, and |pending_service_| is (normally) cleared.
//
// Some key things to notice are 1) WPA Supplicant does the work of selecting
// the AP (aka WiFiEndpoint) and it tells the WiFi device which AP it selected.
// 2) The process of connecting is asynchronous.  There is a |current_service_|
// to which the WiFi device is presently using and a |pending_service_| to which
// the WiFi device has initiated a connection.
//
// A WiFi device is notified that an AP has gone away via the BSSRemoved signal.
// When the last WiFiEndpoint of a WiFiService is removed, the WiFiService
// itself is deleted.
//
// TODO(gmorain): Add explanation of hidden SSIDs.
//
// WPA Supplicant's PropertiesChanged signal communicates changes in the state
// of WPA Supplicant's current service.  This state is stored in
// |supplicant_state_| and reflects WPA Supplicant's view of the state of the
// connection to an AP.  Changes in this state sometimes cause state changes in
// the WiFiService to which a WiFi device is connected.  For example, when WPA
// Supplicant signals the new state to be "completed", then the WiFiService
// state gets changed to "configuring".  State change notifications are not
// reliable because WPA Supplicant may coalesce state changes in quick
// succession so that only the last of the changes is signaled.
//
// Notes:
//
// 1.  Shill's definition of the interface is described in
// shill/dbus_proxies/supplicant-interface.xml, and the WPA Supplicant's
// description of the same interface is in
// third_party/wpa_supplicant/doc/dbus.doxygen.

#include <time.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST
#include <metrics/timer.h>

#include "shill/device.h"
#include "shill/event_dispatcher.h"
#include "shill/key_value_store.h"
#include "shill/net/netlink_manager.h"
#include "shill/power_manager.h"
#include "shill/refptr_types.h"
#include "shill/service.h"
#include "shill/supplicant/supplicant_event_delegate_interface.h"

namespace shill {

class Error;
class GeolocationInfo;
class Mac80211Monitor;
class Metrics;
class NetlinkManager;
class NetlinkMessage;
class Nl80211Message;
class ScanSession;
class SupplicantEAPStateHandler;
class SupplicantInterfaceProxyInterface;
class SupplicantProcessProxyInterface;
class TDLSManager;
class WakeOnWiFi;
class WiFiProvider;
class WiFiService;

// WiFi class. Specialization of Device for WiFi.
class WiFi : public Device, public SupplicantEventDelegateInterface {
 public:
  typedef std::set<uint32_t> FreqSet;

  WiFi(ControlInterface* control_interface,
       EventDispatcher* dispatcher,
       Metrics* metrics,
       Manager* manager,
       const std::string& link,
       const std::string& address,
       int interface_index);
  ~WiFi() override;

  void Start(Error* error,
             const EnabledStateChangedCallback& callback) override;
  void Stop(Error* error, const EnabledStateChangedCallback& callback) override;
  void Scan(ScanType scan_type, Error* error,
            const std::string& reason) override;
  void SetSchedScan(bool enable, Error* error) override;
  // Callback for system suspend.
  void OnBeforeSuspend(const ResultCallback& callback) override;
  // Callback for dark resume.
  void OnDarkResume(const ResultCallback& callback) override;
  // Callback for system resume. If this WiFi device is idle, a scan
  // is initiated. Additionally, the base class implementation is
  // invoked unconditionally.
  void OnAfterResume() override;
  // Callback for when a service is configured with an IP.
  void OnConnected() override;
  // Callback for when a service fails to configure with an IP.
  void OnIPConfigFailure() override;

  // Calls corresponding functions of |wake_on_wifi_|. Refer to wake_on_wifi.h
  // for documentation.
  void AddWakeOnPacketConnection(const std::string& ip_endpoint,
                                 Error* error) override;
  void RemoveWakeOnPacketConnection(const std::string& ip_endpoint,
                                    Error* error) override;
  void RemoveAllWakeOnPacketConnections(Error* error) override;

  // Implementation of SupplicantEventDelegateInterface.  These methods
  // are called by SupplicantInterfaceProxy, in response to events from
  // wpa_supplicant.
  void BSSAdded(const std::string& BSS,
                const KeyValueStore& properties) override;
  void BSSRemoved(const std::string& BSS) override;
  void Certification(const KeyValueStore& properties) override;
  void EAPEvent(
      const std::string& status, const std::string& parameter) override;
  void PropertiesChanged(const KeyValueStore& properties) override;
  void ScanDone(const bool& success) override;
  void TDLSDiscoverResponse(const std::string& peer_address) override;

  // Called by WiFiService.
  virtual void ConnectTo(WiFiService* service);

  // After checking |service| state is active, initiate
  // process of disconnecting.  Log and return if not active.
  virtual void DisconnectFromIfActive(WiFiService* service);

  // If |service| is connected, initiate the process of disconnecting it.
  // Otherwise, if it a pending or current service, discontinue the process
  // of connecting and return |service| to the idle state.
  virtual void DisconnectFrom(WiFiService* service);
  virtual bool IsIdle() const;
  // Clear any cached credentials wpa_supplicant may be holding for
  // |service|.  This has a side-effect of disconnecting the service
  // if it is connected.
  virtual void ClearCachedCredentials(const WiFiService* service);

  // Called by WiFiEndpoint.
  virtual void NotifyEndpointChanged(const WiFiEndpointConstRefPtr& endpoint);

  // Utility, used by WiFiService and WiFiEndpoint.
  // Replace non-ASCII characters with '?'. Return true if one or more
  // characters were changed.
  static bool SanitizeSSID(std::string* ssid);

  // Formats |ssid| for logging purposes, to ease scrubbing.
  static std::string LogSSID(const std::string& ssid);

  // Called by Linkmonitor (overridden from Device superclass).
  void OnLinkMonitorFailure() override;

  // Called by Device when link becomes unreliable (overriden from Device
  // superclass).
  void OnUnreliableLink() override;

  bool IsCurrentService(const WiFiServiceRefPtr service) {
    return service.get() == current_service_.get();
  }

  // Overridden from Device superclass
  std::vector<GeolocationInfo> GetGeolocationObjects() const override;

  // Overridden from Device superclass
  bool ShouldUseArpGateway() const override;

  // Called by a WiFiService when it disassociates itself from this Device.
  virtual void DisassociateFromService(const WiFiServiceRefPtr& service);

  // Called by a WiFiService when it unloads to destroy its lease file.
  virtual void DestroyServiceLease(const WiFiService& service);

  // Perform TDLS |operation| on |peer|.
  std::string PerformTDLSOperation(const std::string& operation,
                                   const std::string& peer,
                                   Error* error) override;

  // Overridden from Device superclass.
  bool IsTrafficMonitorEnabled() const override;

  // Remove all networks from WPA supplicant.
  // Passed as a callback to |wake_on_wifi_| where it is used.
  void RemoveSupplicantNetworks();

  bool RequestRoam(const std::string& addr, Error* error) override;

 private:
  enum ScanMethod {
    kScanMethodNone,
    kScanMethodFull,
    kScanMethodProgressive,
    kScanMethodProgressiveErrorToFull,
    kScanMethodProgressiveFinishedToFull
  };
  enum ScanState {
    kScanIdle,
    kScanScanning,
    kScanBackgroundScanning,
    kScanTransitionToConnecting,
    kScanConnecting,
    kScanConnected,
    kScanFoundNothing
  };

  // Result from a BSSAdded or BSSRemoved event.
  struct ScanResult {
    ScanResult() : is_removal(false) {}
    ScanResult(const std::string& path_in,
               const KeyValueStore& properties_in,
               bool is_removal_in)
        : path(path_in), properties(properties_in), is_removal(is_removal_in) {}
    std::string path;
    KeyValueStore properties;
    bool is_removal;
  };

  struct PendingScanResults {
    PendingScanResults() : is_complete(false) {}
    explicit PendingScanResults(const base::Closure& process_results_callback)
        : is_complete(false), callback(process_results_callback) {}

    // List of pending scan results to process.
    std::vector<ScanResult> results;

    // If true, denotes that the scan is complete (ScanDone() was called).
    bool is_complete;

    // Cancelable closure used to process the scan results.
    base::CancelableClosure callback;
  };

  friend class WiFiObjectTest;  // access to supplicant_*_proxy_, link_up_
  friend class WiFiTimerTest;  // kNumFastScanAttempts, kFastScanIntervalSeconds
  friend class WiFiMainTest;  // ScanState, ScanMethod
  FRIEND_TEST(WiFiMainTest, AppendBgscan);
  FRIEND_TEST(WiFiMainTest, BackgroundScan);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, ConnectToServiceNotPending);  // ScanState
  FRIEND_TEST(WiFiMainTest, ConnectToWithError);  // ScanState
  FRIEND_TEST(WiFiMainTest, ConnectWhileNotScanning);  // ScanState
  FRIEND_TEST(WiFiMainTest, CurrentBSSChangedUpdateServiceEndpoint);
  FRIEND_TEST(WiFiMainTest, DisconnectReasonUpdated);
  FRIEND_TEST(WiFiMainTest, DisconnectReasonCleared);
  FRIEND_TEST(WiFiMainTest, FlushBSSOnResume);  // kMaxBSSResumeAgeSeconds
  FRIEND_TEST(WiFiMainTest, FullScanConnecting);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, FullScanConnectingToConnected);
  FRIEND_TEST(WiFiMainTest, FullScanDuringProgressive);  // ScanState
  FRIEND_TEST(WiFiMainTest, FullScanFindsNothing);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, InitialSupplicantState);  // kInterfaceStateUnknown
  FRIEND_TEST(WiFiMainTest, LinkMonitorFailure);  // set_link_monitor()
  FRIEND_TEST(WiFiMainTest, NoScansWhileConnecting_FullScan);  // ScanState
  FRIEND_TEST(WiFiMainTest, NoScansWhileConnecting);  // ScanState
  FRIEND_TEST(WiFiMainTest, PendingScanEvents);  // EndpointMap
  FRIEND_TEST(WiFiMainTest, ProgressiveScanConnectingToConnected);
  FRIEND_TEST(WiFiMainTest, ProgressiveScanConnectingToNotFound);
  FRIEND_TEST(WiFiMainTest, ProgressiveScanDuringFull);  // ScanState
  FRIEND_TEST(WiFiMainTest, ProgressiveScanError);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, ProgressiveScanFound);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, ProgressiveScanNotFound);  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, ScanRejected);  // ScanState
  FRIEND_TEST(WiFiMainTest, ScanResults);             // EndpointMap
  FRIEND_TEST(WiFiMainTest, ScanResultsWithUpdates);  // EndpointMap
  FRIEND_TEST(WiFiMainTest, ScanStateHandleDisconnect);  // ScanState
  FRIEND_TEST(WiFiMainTest, ScanStateNotScanningNoUma);  // ScanState
  FRIEND_TEST(WiFiMainTest, ScanStateUma);  // ScanState, ScanMethod
  FRIEND_TEST(WiFiMainTest, Stop);  // weak_ptr_factory_
  FRIEND_TEST(WiFiMainTest, TimeoutPendingServiceWithEndpoints);
  FRIEND_TEST(WiFiPropertyTest, BgscanMethodProperty);  // bgscan_method_
  FRIEND_TEST(WiFiTimerTest, FastRescan);  // kFastScanIntervalSeconds
  FRIEND_TEST(WiFiTimerTest, RequestStationInfo);  // kRequestStationInfoPeriod
  // kPostWakeConnectivityReportDelayMilliseconds
  FRIEND_TEST(WiFiTimerTest, ResumeDispatchesConnectivityReportTask);
  // kFastScanIntervalSeconds
  FRIEND_TEST(WiFiTimerTest, StartScanTimer_HaveFastScansRemaining);
  FRIEND_TEST(WiFiMainTest, ParseWiphyIndex_Success);  // kDefaultWiphyIndex
  // ScanMethod, ScanState
  FRIEND_TEST(WiFiMainTest, ResetScanStateWhenScanFailed);
  // kPostScanFailedDelayMilliseconds
  FRIEND_TEST(WiFiTimerTest, ScanDoneDispatchesTasks);
  // kMaxPassiveScanRetries, kMaxFreqsForPassiveScanRetries
  FRIEND_TEST(WiFiMainTest, InitiateScanInDarkResume_Idle);

  typedef std::map<const std::string, WiFiEndpointRefPtr> EndpointMap;
  typedef std::map<const WiFiService*, std::string> ReverseServiceMap;

  static const char* kDefaultBgscanMethod;
  static const uint16_t kBackgroundScanIntervalSeconds;
  static const uint16_t kDefaultBgscanShortIntervalSeconds;
  static const int32_t kDefaultBgscanSignalThresholdDbm;
  static const uint16_t kDefaultRoamThresholdDb;
  static const uint16_t kDefaultScanIntervalSeconds;
  static const time_t kMaxBSSResumeAgeSeconds;
  static const char kInterfaceStateUnknown[];
  // Delay between scans when supplicant finds "No suitable network".
  static const time_t kRescanIntervalSeconds;
  // Number of times to quickly attempt a scan after startup / disconnect.
  static const int kNumFastScanAttempts;
  static const int kFastScanIntervalSeconds;
  static const int kPendingTimeoutSeconds;
  static const int kReconnectTimeoutSeconds;
  static const int kRequestStationInfoPeriodSeconds;
  static const size_t kMinumumFrequenciesToScan;
  static const float kDefaultFractionPerScan;
  static const size_t kStuckQueueLengthThreshold;
  // Number of milliseconds to wait after waking from suspend to report the
  // connection status to metrics.
  static const int kPostWakeConnectivityReportDelayMilliseconds;
  // Used to instantiate |wiphy_index_| in WiFi. Assigned a large value so that
  // any attempts to match the default value of |wiphy_index_| against an actual
  // wiphy index reported in an NL80211 message will fail.
  static const uint32_t kDefaultWiphyIndex;
  // Number of milliseconds to wait after failing to launch a scan before
  // resetting the scan state to idle.
  static const int kPostScanFailedDelayMilliseconds;
  // Used to distinguish between a disconnect reason explicitly set by
  // supplicant and a default.
  static const int kDefaultDisconnectReason;

  void GetPhyInfo();
  void AppendBgscan(WiFiService* service,
                    KeyValueStore* service_params) const;
  std::string GetBgscanMethod(const int& argument, Error* error);
  uint16_t GetBgscanShortInterval(Error* /* error */) {
    return bgscan_short_interval_seconds_;
  }
  int32_t GetBgscanSignalThreshold(Error* /* error */) {
    return bgscan_signal_threshold_dbm_;
  }
  // These methods can't be 'const' because they are passed to
  // HelpRegisterDerivedUint16 which don't take const methods.
  uint16_t GetRoamThreshold(Error* /* error */) /*const*/ {
    return roam_threshold_db_;
  }
  uint16_t GetScanInterval(Error* /* error */) /*const*/ {
    return scan_interval_seconds_;
  }

  // RPC accessor for |link_statistics_|.
  KeyValueStore GetLinkStatistics(Error* error);

  bool GetScanPending(Error* /* error */);
  bool SetBgscanMethod(
      const int& argument, const std::string& method, Error* error);
  bool SetBgscanShortInterval(const uint16_t& seconds, Error* error);
  bool SetBgscanSignalThreshold(const int32_t& dbm, Error* error);
  bool SetRoamThreshold(const uint16_t& threshold, Error* /*error*/);
  bool SetScanInterval(const uint16_t& seconds, Error* error);
  void ClearBgscanMethod(const int& argument, Error* error);

  void CurrentBSSChanged(const std::string& new_bss);
  void DisconnectReasonChanged(const int32_t new_disconnect_reason);
  // Return the RPC identifier associated with the wpa_supplicant network
  // entry created for |service|.  If one does not exist, an empty string
  // is returned, and |error| is populated.
  std::string FindNetworkRpcidForService(const WiFiService* service,
                                         Error* error);
  void HandleDisconnect();
  // Update failure and state for disconnected service.
  // Set failure for disconnected service if disconnect is not user-initiated
  // and failure is not already set. Then set the state of the service back
  // to idle, so it can be used for future connections.
  void ServiceDisconnected(WiFiServiceRefPtr service);
  void HandleRoam(const std::string& new_bssid);
  void BSSAddedTask(const std::string& BSS,
                    const KeyValueStore& properties);
  void BSSRemovedTask(const std::string& BSS);
  void CertificationTask(const KeyValueStore& properties);
  void EAPEventTask(const std::string& status, const std::string& parameter);
  void PropertiesChangedTask(const KeyValueStore& properties);
  void ScanDoneTask();
  void ScanFailedTask();
  // UpdateScanStateAfterScanDone is spawned as a task from ScanDoneTask in
  // order to guarantee that it is run after the start of any connections that
  // result from a scan.  This works because supplicant sends all BSSAdded
  // signals to shill before it sends a ScanDone signal.  The code that
  // handles those signals launch tasks such that the tasks have the following
  // dependencies (an arrow from X->Y indicates X is guaranteed to run before
  // Y):
  //
  // [BSSAdded]-->[BssAddedTask]-->[SortServiceTask (calls ConnectTo)]
  //     |              |                 |
  //     V              V                 V
  // [ScanDone]-->[ScanDoneTask]-->[UpdateScanStateAfterScanDone]
  void UpdateScanStateAfterScanDone();
  void ScanTask();
  void StateChanged(const std::string& new_state);
  // Heuristic check if a connection failure was due to bad credentials.
  // Returns true and puts type of failure in |failure| if a credential
  // problem is detected.
  bool SuspectCredentials(WiFiServiceRefPtr service,
                          Service::ConnectFailure* failure) const;
  void HelpRegisterDerivedInt32(
      PropertyStore* store,
      const std::string& name,
      int32_t(WiFi::*get)(Error* error),
      bool(WiFi::*set)(const int32_t& value, Error* error));
  void HelpRegisterDerivedUint16(
      PropertyStore* store,
      const std::string& name,
      uint16_t(WiFi::*get)(Error* error),
      bool(WiFi::*set)(const uint16_t& value, Error* error));
  void HelpRegisterConstDerivedBool(
      PropertyStore* store,
      const std::string& name,
      bool(WiFi::*get)(Error* error));

  // Disable a network entry in wpa_supplicant, and catch any exception
  // that occurs.  Returns false if an exception occurred, true otherwise.
  bool DisableNetwork(const std::string& network);
  // Disable the wpa_supplicant network entry associated with |service|.
  // Any cached credentials stored in wpa_supplicant related to this
  // network entry will be preserved.  This will have the side-effect of
  // disconnecting this service if it is currently connected.  Returns
  // true if successful, otherwise returns false and populates |error|
  // with the reason for failure.
  virtual bool DisableNetworkForService(
      const WiFiService* service, Error* error);
  // Remove a network entry from wpa_supplicant, and catch any exception
  // that occurs.  Returns false if an exception occurred, true otherwise.
  bool RemoveNetwork(const std::string& network);
  // Remove the wpa_supplicant network entry associated with |service|.
  // Any cached credentials stored in wpa_supplicant related to this
  // network entry will be removed.  This will have the side-effect of
  // disconnecting this service if it is currently connected.  Returns
  // true if successful, otherwise returns false and populates |error|
  // with the reason for failure.
  virtual bool RemoveNetworkForService(
      const WiFiService* service, Error* error);
  // Update disable_ht40 setting in wpa_supplicant for the given service.
  void SetHT40EnableForService(const WiFiService* service, bool enable);
  // Perform the next in a series of progressive scans.
  void ProgressiveScanTask();
  // Task to configure scheduled scan in wpa_supplicant.
  void SetSchedScanTask(bool enable);
  // Recovers from failed progressive scan.
  void OnFailedProgressiveScan();
  // Restart fast scanning after disconnection.
  void RestartFastScanAttempts();
  // Schedules a scan attempt at time |scan_interval_seconds_| in the
  // future.  Cancels any currently pending scan timer.
  void StartScanTimer();
  // Cancels any currently pending scan timer.
  void StopScanTimer();
  // Initiates a scan, if idle. Reschedules the scan timer regardless.
  void ScanTimerHandler();
  // Abort any current scan (at the shill-level; let any request that's
  // already gone out finish).
  void AbortScan();
  // Abort any current scan and start a new scan of type |type| if shill is
  // currently idle.
  void InitiateScan(ScanType scan_type);
  // Suppresses manager auto-connects and flushes supplicant BSS cache, then
  // triggers the passive scan. Meant for use in dark resume where we want to
  // ensure that shill and supplicant do not use stale information to launch
  // connection attempts.
  void InitiateScanInDarkResume(const FreqSet& freqs);
  // If |freqs| contains at least one frequency channel a passive scan is
  // launched on all the frequencies in |freqs|. Otherwise, a passive scan is
  // launched on all channels.
  void TriggerPassiveScan(const FreqSet& freqs);
  // Starts a timer in order to limit the length of an attempt to
  // connect to a pending network.
  void StartPendingTimer();
  // Cancels any currently pending network timer.
  void StopPendingTimer();
  // Aborts a pending network that is taking too long to connect.
  void PendingTimeoutHandler();
  // Starts a timer in order to limit the length of an attempt to
  // reconnect to the current network.
  void StartReconnectTimer();
  // Stops any pending reconnect timer.
  void StopReconnectTimer();
  // Disconnects from the current service that is taking too long
  // to reconnect on its own.
  void ReconnectTimeoutHandler();
  // Sets the current pending service.  If the argument is non-NULL,
  // the Pending timer is started and the associated service is set
  // to "Associating", otherwise it is stopped.
  void SetPendingService(const WiFiServiceRefPtr& service);

  void OnSupplicantAppear();
  void OnSupplicantVanish();
  // Called by ScopeLogger when WiFi debug scope is enabled/disabled.
  void OnWiFiDebugScopeChanged(bool enabled);
  // Enable or disable debugging for the current connection attempt.
  void SetConnectionDebugging(bool enabled);
  // Enable high bitrates for the current network.  High rates are disabled
  // on the initial association and every reassociation afterward.
  void EnableHighBitrates();

  // Request and retrieve information about the currently connected station.
  void RequestStationInfo();
  void OnReceivedStationInfo(const Nl80211Message& nl80211_message);
  void StopRequestingStationInfo();

  void ConnectToSupplicant();

  void Restart();

  std::string GetServiceLeaseName(const WiFiService& service);

  // Netlink message handler for NL80211_CMD_NEW_WIPHY messages; copies
  // device's supported frequencies from that message into
  // |all_scan_frequencies_|.
  void OnNewWiphy(const Nl80211Message& nl80211_message);

  void OnTriggerPassiveScanResponse(const Nl80211Message& netlink_message);

  void SetScanState(ScanState new_state,
                    ScanMethod new_method,
                    const char* reason);
  void ReportScanResultToUma(ScanState state, ScanMethod method);
  static std::string ScanStateString(ScanState state, ScanMethod type);

  // In addition to calling the implementations of these functions in Device,
  // calls WakeOnWiFi::PrepareForWakeOnWiFiBeforeSuspend.
  void OnIPConfigUpdated(const IPConfigRefPtr& ipconfig,
                         bool new_lease_acquired) override;
  void OnIPv6ConfigUpdated() override;

  // Returns true iff the WiFi device is connected to the current service.
  bool IsConnectedToCurrentService();

  // Callback invoked to report whether this WiFi device is connected to
  // a service after waking from suspend. Wraps around a Call the function
  // with the same name in WakeOnWiFi.
  void ReportConnectedToServiceAfterWake();

  // Add a scan result to the list of pending scan results, and post a task
  // for handling these results if one is not already running.
  void AddPendingScanResult(const std::string& path,
                            const KeyValueStore& properties,
                            bool is_removal);

  // Callback invoked to handle pending scan results from AddPendingScanResult.
  void PendingScanResultsHandler();

  // Given a NL80211_CMD_NEW_WIPHY message |nl80211_message|, parses the
  // wiphy index of the NIC and sets |wiphy_index_| with the parsed index.
  // Returns true iff the wiphy index was parsed successfully, false otherwise.
  bool ParseWiphyIndex(const Nl80211Message& nl80211_message);

  // Callback invoked when the kernel broadcasts a notification that a scan has
  // started.
  virtual void OnScanStarted(const NetlinkMessage& netlink_message);

  // Helper function for setting supplicant_interface_proxy_ pointer.
  void SetSupplicantInterfaceProxy(
      SupplicantInterfaceProxyInterface* supplicant_interface_proxy);

  // Pointer to the provider object that maintains WiFiService objects.
  WiFiProvider* provider_;

  base::WeakPtrFactory<WiFi> weak_ptr_factory_;

  // Store cached copies of singletons for speed/ease of testing.
  Time* time_;

  bool supplicant_present_;

  std::unique_ptr<SupplicantProcessProxyInterface> supplicant_process_proxy_;
  std::unique_ptr<SupplicantInterfaceProxyInterface>
      supplicant_interface_proxy_;
  // wpa_supplicant's RPC path for this device/interface.
  std::string supplicant_interface_path_;
  // The rpcid used as the key is wpa_supplicant's D-Bus path for the
  // Endpoint (BSS, in supplicant parlance).
  EndpointMap endpoint_by_rpcid_;
  // Map from Services to the D-Bus path for the corresponding wpa_supplicant
  // Network.
  ReverseServiceMap rpcid_by_service_;
  // The Service we are presently connected to. May be nullptr is we're not
  // not connected to any Service.
  WiFiServiceRefPtr current_service_;
  // The Service we're attempting to connect to. May be nullptr if we're
  // not attempting to connect to a new Service. If non-NULL, should
  // be distinct from |current_service_|. (A service should not
  // simultaneously be both pending, and current.)
  WiFiServiceRefPtr pending_service_;
  std::string supplicant_state_;
  std::string supplicant_bss_;
  int32_t supplicant_disconnect_reason_;
  std::string phy_name_;
  // Indicates that we should flush supplicant's BSS cache after the
  // next scan completes.
  bool need_bss_flush_;
  struct timeval resumed_at_;
  // Executes when the (foreground) scan timer expires. Calls ScanTimerHandler.
  base::CancelableClosure scan_timer_callback_;
  // Executes when a pending service connect timer expires. Calls
  // PendingTimeoutHandler.
  base::CancelableClosure pending_timeout_callback_;
  // Executes when a reconnecting service timer expires. Calls
  // ReconnectTimeoutHandler.
  base::CancelableClosure reconnect_timeout_callback_;
  // Executes periodically while a service is connected, to update the
  // signal strength from the currently connected AP.
  base::CancelableClosure request_station_info_callback_;
  // Executes when WPA supplicant reports that a scan has failed via a ScanDone
  // signal.
  base::CancelableClosure scan_failed_callback_;
  // Number of remaining fast scans to be done during startup and disconnect.
  int fast_scans_remaining_;
  // Indicates that the current BSS has reached the completed state according
  // to supplicant.
  bool has_already_completed_;
  // Indicates that the current BSS for a connected service has changed, which
  // implies that a driver-based roam has been initiated.  If this roam
  // succeeds, we should renew our lease.
  bool is_roaming_in_progress_;
  // Indicates that we are debugging a problematic connection.
  bool is_debugging_connection_;
  // Tracks the process of an EAP negotiation.
  std::unique_ptr<SupplicantEAPStateHandler> eap_state_handler_;
  // Tracks mac80211 state, to diagnose problems such as queue stalls.
  std::unique_ptr<Mac80211Monitor> mac80211_monitor_;

  // Properties
  std::string bgscan_method_;
  uint16_t bgscan_short_interval_seconds_;
  int32_t bgscan_signal_threshold_dbm_;
  uint16_t roam_threshold_db_;
  uint16_t scan_interval_seconds_;

  bool progressive_scan_enabled_;
  std::string scan_configuration_;
  NetlinkManager* netlink_manager_;
  std::set<uint16_t> all_scan_frequencies_;
  std::unique_ptr<ScanSession> scan_session_;
  size_t min_frequencies_to_scan_;
  size_t max_frequencies_to_scan_;
  bool scan_all_frequencies_;

  // Holds the list of scan results waiting to be processed and a cancelable
  // closure for processing the pending tasks in PendingScanResultsHandler().
  std::unique_ptr<PendingScanResults> pending_scan_results_;

  // Fraction of previously seen scan frequencies to include in each
  // progressive scan batch (since the frequencies are sorted, the sum of the
  // fraction_per_scan_ over the scans in a session (* 100) is the percentile
  // of the frequencies that have been scanned).
  float fraction_per_scan_;

  ScanState scan_state_;
  ScanMethod scan_method_;
  chromeos_metrics::Timer scan_timer_;

  // Used to compute the number of bytes received since the link went up.
  uint64_t receive_byte_count_at_connect_;

  // Used to report the current state of our wireless link.
  KeyValueStore link_statistics_;

  // Wiphy interface index of this WiFi device.
  uint32_t wiphy_index_;

  std::unique_ptr<WakeOnWiFi> wake_on_wifi_;

  std::unique_ptr<TDLSManager> tdls_manager_;

  DISALLOW_COPY_AND_ASSIGN(WiFi);
};

}  // namespace shill

#endif  // SHILL_WIFI_WIFI_H_
