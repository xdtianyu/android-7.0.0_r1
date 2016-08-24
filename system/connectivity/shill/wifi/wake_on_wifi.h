//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_WIFI_WAKE_ON_WIFI_H_
#define SHILL_WIFI_WAKE_ON_WIFI_H_

#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base/cancelable_callback.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <components/timers/alarm_timer_chromeos.h>

#include "shill/callbacks.h"
#include "shill/ip_address_store.h"
#include "shill/net/event_history.h"
#include "shill/net/ip_address.h"
#include "shill/net/netlink_manager.h"
#include "shill/refptr_types.h"
#include "shill/wifi/wifi.h"

namespace shill {

class ByteString;
class Error;
class EventDispatcher;
class GetWakeOnPacketConnMessage;
class Metrics;
class Nl80211Message;
class PropertyStore;
class SetWakeOnPacketConnMessage;

// |WakeOnWiFi| performs all wake on WiFi related tasks and logic (e.g.
// suspend/dark resume/resume logic, NIC wowlan programming via nl80211), and
// stores the state necessary to perform these actions.
//
// Shill implements two wake on WiFi features:
//   1) Dark connect: this feature allows the CrOS device to maintain WiFi
//      connectivity while suspended, and to wake from suspend in a low-power
//      state (dark resume) to maintain or re-establish WiFi connectivity.
//   2) Packet: this feature allows the CrOS device to wake from suspend upon
//      receiving network packets from any whitelisted hosts.
// Either or both of these features can be enabled/disabled by assigning the
// appropriate value to |wake_on_wifi_features_enabled_|.
//
// Note that wake on WiFi features are different from wake on WiFi triggers. The
// former refers to shill's suspend/resume/dark resume handling logic, whereas
// the latter refers to the NIC's  ability to wake the CPU on certain network
// events (e.g. disconnects). In order for shill's wake on WiFi features to
// work, the platform must be compiled with wake on WiFi support (i.e.
// DISABLE_WAKE_ON_WIFI not set), and its NIC must support the triggers required
// for the features to work (see WakeOnWiFi::WakeOnWiFiPacketEnabledAndSupported
// and WakeOnWiFi::WakeOnWiFiDarkConnectEnabledAndSupported for more details).
//
// The logic shill uses before, during (i.e. during dark resume), and after
// suspend when both wake on WiFi features are enabled are described below:
//
// OnBeforeSuspend
// ================
// This function is run when Manager announces an upcoming system suspend.
//
//         +--------------+
//         |          Yes |   +----------------+
// +-------+--------+     +-->|Renew DHCP Lease|
// |  Connected &   |         +------+---------+
// |holding expiring|                |
// |  DHCP lease?   |                v
// +------+---------+         +--------------------+
//        |               +-> |BeforeSuspendActions|
//        |           No  |   +--------------------+
//        +---------------+
//
// OnDarkResume
// =============
// This function is run when Manager announces that the system has entered
// dark resume and that there is an upcoming system suspend.
//
// +-------------+      +------------+     Unsupported     +----------+
// |  Too many   +----->|Wake reason?+-------------------->|Connected?|
// |dark resumes?|  No  +-+----------+                     +-+-----+--+
// +------+------+        |       |                          |     |
//        | Yes           |       | Disconnect/           No |     | Yes
//        v               |       |    SSID                  |     |
// +-------------------+  |       v                          |     |
// |  Disable Wake on  |  |     +------------+               |     v
// |  WiFi, start wake |  |     |  Initiate  |<--------------+    +--------+
// |  to scan timer &  |  |     |passive scan|                    |Get DHCP|
// |  report readiness |  |     +-+----------+           +------->| Lease  |
// +-------------------+  |       | ScanDone         Yes |        +--+---+-+
//    +-------------------+       v                      |           |   |
//    | Pattern                 +-------------+      +---------+     |   |
//    |                    No   | Any services| Yes  |Connected|     |   |
//    |    +--------------------+available for+----->| to AP?  |     |   |
//    |    |                    | autoconnect?|      +---+-----+     |   |
//    |    |                    +-------------+          |           |   |
//    |    |                                             |No         |   |
//    v    v                                             |           |   |
// +--------------------+       +-------+                |           |   |
// |BeforeSuspendActions|<------+Timeout|<---------------+       No  |   |
// +--------------------+       +-------+<---------------------------+   |
//         ^                                                             |
//         |                   +-------------------+                     |
//         +-------------------+ OnIPConfigUpdated/|             Yes     |
//                             |OnIPv6ConfigUpdated|<--------------------+
//                             +-------------------+
//
// BeforeSuspendActions
// =====================
// This function is run immediately before the system reports suspend readiness
// to Manager. This is the common "exit path" taken by OnBeforeSuspend and
// OnDarkResume before suspending.
//
// +----------------------+
// |Packet feature enabled|   Yes   +------------------------+
// |    and supported?    +-------->|Set Wake on Pattern flag|
// +-----+----------------+         +------------+-----------+
//       |                                       |
//    No |        +------------------------------+
//       |        |
// +-----v--------v-------+        No
// | Dark connect feature +---------------------------------+
// |enabled and supported?|                                 |
// +--+-------------------+                                 |
//    |                                                     |
//    |Yes    Yes   +----------------------------+          |        +---------+
//    |     +-----> |Set Wake on Disconnect flag,+--+    +--v----+   |Report   |
//    |     |       |Start Lease Renewal Timer*  |  |    |Program|   |Suspend  |
//    |     |       +----------------------------+  +--> |  NIC  |   |Readiness|
// +--v-----+-+                                     |    +-+---+-+   +--+------+
// |Connected?|                                     |      |   ^        ^
// +--------+-+                                     |      |   |Failed  |
//          |                                       |      ^   |        |Success
//          |       +----------------------------+  |  +---+---+---+    |
//          +-----> |Set Wake on SSID flag,      +--+  |  Verify   +----+
//            No    |Start Wake To Scan Timer**  |     |Programming|
//                  +----------------------------+     +-----------+
//
// *  if necessary (as indicated by caller of BeforeSuspendActions).
// ** if we need to whitelist more SSIDs than our NIC supports.
//
// OnAfterResume
// ==============
// This is run after Manager announces that the system has fully resumed from
// suspend.
//
// Wake on WiFi is disabled on the NIC if it was enabled before suspend or
// dark resume, and both the wake to scan timer and DHCP lease renewal timers
// are stopped.

class WakeOnWiFi {
 public:
  typedef base::Callback<void(const WiFi::FreqSet&)> InitiateScanCallback;
  // Callback used to report the wake reason for the current dark resume to
  // powerd.
  typedef base::Callback<void(const std::string&)> RecordWakeReasonCallback;

  // Types of triggers that we can program the NIC to wake the WiFi device.
  enum WakeOnWiFiTrigger {
    kWakeTriggerUnsupported = 0,  // Used for reporting, not programming NIC.
    kWakeTriggerPattern = 1,
    kWakeTriggerDisconnect = 2,
    kWakeTriggerSSID = 3
  };

  WakeOnWiFi(NetlinkManager* netlink_manager, EventDispatcher* dispatcher,
             Metrics* metrics,
             RecordWakeReasonCallback record_wake_reason_callback);
  virtual ~WakeOnWiFi();

  // Registers |store| with properties related to wake on WiFi.
  void InitPropertyStore(PropertyStore* store);

  // Starts |metrics_timer_| so that wake on WiFi related metrics are
  // periodically collected.
  void StartMetricsTimer();

  // Enable the NIC to wake on packets received from |ip_endpoint|.
  // Note: The actual programming of the NIC only happens before the system
  // suspends, in |OnBeforeSuspend|.
  void AddWakeOnPacketConnection(const std::string& ip_endpoint, Error* error);
  // Remove rule to wake on packets received from |ip_endpoint| from the NIC.
  // Note: The actual programming of the NIC only happens before the system
  // suspends, in |OnBeforeSuspend|.
  void RemoveWakeOnPacketConnection(const std::string& ip_endpoint,
                                    Error* error);
  // Remove all rules to wake on incoming packets from the NIC.
  // Note: The actual programming of the NIC only happens before the system
  // suspends, in |OnBeforeSuspend|.
  void RemoveAllWakeOnPacketConnections(Error* error);
  // Given a NL80211_CMD_NEW_WIPHY message |nl80211_message|, parses the
  // wake on WiFi capabilities of the NIC and set relevant members of this
  // WakeOnWiFi object to reflect the supported capbilities.
  virtual void ParseWakeOnWiFiCapabilities(
      const Nl80211Message& nl80211_message);
  // Callback invoked when the system reports its wakeup reason.
  //
  // Arguments:
  //  - |netlink_message|: wakeup report message (note: must manually check
  //    this message to make sure it is a wakeup report message).
  //
  // Note: Assumes only one wakeup reason is received. If more than one is
  // received, the only first one parsed will be handled.
  virtual void OnWakeupReasonReceived(const NetlinkMessage& netlink_message);
  // Performs pre-suspend actions relevant to wake on WiFi functionality.
  //
  // Arguments:
  //  - |is_connected|: whether the WiFi device is connected.
  //  - |ssid_whitelist|: list of SSIDs that the NIC will be programmed to wake
  //    the system on if the NIC is programmed to wake on SSID.
  //  - |done_callback|: callback to invoke when suspend  actions have
  //    completed.
  //  - |renew_dhcp_lease_callback|: callback to invoke to initiate DHCP lease
  //    renewal.
  //  - |remove_supplicant_networks_callback|: callback to invoke
  //    to remove all networks from WPA supplicant.
  //  - |have_dhcp_lease|: whether or not there is a DHCP lease to renew.
  //  - |time_to_next_lease_renewal|: number of seconds until next DHCP lease
  //    renewal is due.
  virtual void OnBeforeSuspend(
      bool is_connected,
      const std::vector<ByteString>& ssid_whitelist,
      const ResultCallback& done_callback,
      const base::Closure& renew_dhcp_lease_callback,
      const base::Closure& remove_supplicant_networks_callback,
      bool have_dhcp_lease,
      uint32_t time_to_next_lease_renewal);
  // Performs post-resume actions relevant to wake on wireless functionality.
  virtual void OnAfterResume();
  // Performs and post actions to be performed in dark resume.
  //
  // Arguments:
  //  - |is_connected|: whether the WiFi device is connected.
  //  - |ssid_whitelist|: list of SSIDs that the NIC will be programmed to wake
  //    the system on if the NIC is programmed to wake on SSID.
  //  - |done_callback|: callback to invoke when dark resume actions have
  //    completed.
  //  - |renew_dhcp_lease_callback|: callback to invoke to initiate DHCP lease
  //    renewal.
  //  - |initate_scan_callback|: callback to invoke to initiate a scan.
  //  - |remove_supplicant_networks_callback|: callback to invoke
  //    to remove all networks from WPA supplicant.
  virtual void OnDarkResume(
      bool is_connected,
      const std::vector<ByteString>& ssid_whitelist,
      const ResultCallback& done_callback,
      const base::Closure& renew_dhcp_lease_callback,
      const InitiateScanCallback& initiate_scan_callback,
      const base::Closure& remove_supplicant_networks_callback);
  // Called when we the current service is connected, and we have IP
  // reachability. Calls WakeOnWiFi::BeforeSuspendActions if we are in dark
  // resume to end the current dark resume. Otherwise, does nothing.
  virtual void OnConnectedAndReachable(bool start_lease_renewal_timer,
                                       uint32_t time_to_next_lease_renewal);
  // Callback invoked to report whether this WiFi device is connected to
  // a service after waking from suspend.
  virtual void ReportConnectedToServiceAfterWake(bool is_connected);
  // Called in WiFi::ScanDoneTask when there are no WiFi services available
  // for auto-connect after a scan. |initiate_scan_callback| is used for dark
  // resume scan retries.
  virtual void OnNoAutoConnectableServicesAfterScan(
      const std::vector<ByteString>& ssid_whitelist,
      const base::Closure& remove_supplicant_networks_callback,
      const InitiateScanCallback& initiate_scan_callback);
  // Called by WiFi when it is notified by the kernel that a scan has started.
  // If |is_active_scan| is true, the scan is an active scan. Otherwise, the
  // scan is a passive scan.
  virtual void OnScanStarted(bool is_active_scan);

  bool in_dark_resume() { return in_dark_resume_; }

  virtual void OnWiphyIndexReceived(uint32_t index);

 private:
  friend class WakeOnWiFiTest;  // access to several members for tests
  friend class WiFiObjectTest;  // netlink_manager_
  // kWakeOnWiFiNotSupported.
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              WakeOnWiFiDisabled_SetWakeOnWiFiFeaturesEnabled);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              WakeOnWiFiDisabled_AddWakeOnPacketConnection_ReturnsError);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              WakeOnWiFiDisabled_RemoveWakeOnPacketConnection_ReturnsError);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              WakeOnWiFiDisabled_RemoveAllWakeOnPacketConnections_ReturnsError);
  // kMaxSetWakeOnPacketRetries.
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              RetrySetWakeOnPacketConnections_LessThanMaxRetries);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              RetrySetWakeOnPacketConnections_MaxAttemptsWithCallbackSet);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              RetrySetWakeOnPacketConnections_MaxAttemptsCallbackUnset);
  // kDarkResumeActionsTimeoutMilliseconds
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              OnBeforeSuspend_DHCPLeaseRenewal);
  // Dark resume wake reason strings (e.g. kWakeReasonStringDisconnect)
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher,
              OnWakeupReasonReceived_Disconnect);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_SSID);
  FRIEND_TEST(WakeOnWiFiTestWithMockDispatcher, OnWakeupReasonReceived_Pattern);
  // kMaxDarkResumesPerPeriodShort
  FRIEND_TEST(WakeOnWiFiTestWithDispatcher, OnBeforeSuspend_ClearsEventHistory);
  // kDarkResumeFrequencySamplingPeriodShortMinutes,
  // kMaxDarkResumesPerPeriodShort
  FRIEND_TEST(WakeOnWiFiTestWithDispatcher,
              OnDarkResume_NotConnected_MaxDarkResumes_ShortPeriod);
  // kDarkResumeFrequencySamplingPeriodLongMinutes,
  // kMaxDarkResumesPerPeriodLong,
  // kDarkResumeFrequencySamplingPeriodShortMinutes,
  // kMaxDarkResumesPerPeriodShort
  FRIEND_TEST(WakeOnWiFiTestWithDispatcher,
              OnDarkResume_NotConnected_MaxDarkResumes_LongPeriod);
  // kMaxFreqsForDarkResumeScanRetries, kMaxDarkResumeScanRetries
  FRIEND_TEST(WakeOnWiFiTestWithDispatcher, InitiateScanInDarkResume);

  static const char kWakeOnIPAddressPatternsNotSupported[];
  static const char kWakeOnWiFiNotSupported[];
  static const int kVerifyWakeOnWiFiSettingsDelayMilliseconds;
  static const int kMaxSetWakeOnPacketRetries;
  static const int kMetricsReportingFrequencySeconds;
  static const uint32_t kDefaultWakeToScanPeriodSeconds;
  static const uint32_t kDefaultNetDetectScanPeriodSeconds;
  static const uint32_t kImmediateDHCPLeaseRenewalThresholdSeconds;
  static const int kDarkResumeFrequencySamplingPeriodShortMinutes;
  static const int kDarkResumeFrequencySamplingPeriodLongMinutes;
  static const int kMaxDarkResumesPerPeriodShort;
  static const int kMaxDarkResumesPerPeriodLong;
  static int64_t DarkResumeActionsTimeoutMilliseconds;  // non-const for testing
  static const int kMaxFreqsForDarkResumeScanRetries;
  static const int kMaxDarkResumeScanRetries;
  // Dark resume wake reason names. These will be sent to powerd via
  // RecordDarkResumeWakeReason, to tell it the reason the system woke in the
  // current dark resume.
  static const char kWakeReasonStringPattern[];
  static const char kWakeReasonStringDisconnect[];
  static const char kWakeReasonStringSSID[];

  std::string GetWakeOnWiFiFeaturesEnabled(Error* error);
  bool SetWakeOnWiFiFeaturesEnabled(const std::string& enabled, Error* error);
  // Helper function to run and reset |suspend_actions_done_callback_|.
  void RunAndResetSuspendActionsDoneCallback(const Error& error);
  // Used for comparison of ByteString pairs in a set.
  static bool ByteStringPairIsLessThan(
      const std::pair<ByteString, ByteString>& lhs,
      const std::pair<ByteString, ByteString>& rhs);
  // Creates a mask which specifies which bytes in pattern of length
  // |pattern_len| to match against. Bits |offset| to |pattern_len| - 1 are set,
  // which bits 0 to bits 0 to |offset| - 1 are unset. This mask is saved in
  // |mask|.
  static void SetMask(ByteString* mask, uint32_t pattern_len, uint32_t offset);
  // Creates a pattern and mask for a NL80211 message that programs the NIC to
  // wake on packets originating from IP address |ip_addr|. The pattern and mask
  // are saved in |pattern| and |mask| respectively. Returns true iff the
  // pattern and mask are successfully created and written to |pattern| and
  // |mask| respectively.
  static bool CreateIPAddressPatternAndMask(const IPAddress& ip_addr,
                                            ByteString* pattern,
                                            ByteString* mask);
  static void CreateIPV4PatternAndMask(const IPAddress& ip_addr,
                                       ByteString* pattern, ByteString* mask);
  static void CreateIPV6PatternAndMask(const IPAddress& ip_addr,
                                       ByteString* pattern, ByteString* mask);
  // Creates and sets an attribute in a NL80211 message |msg| which indicates
  // the index of the wiphy interface to program. Returns true iff |msg| is
  // successfully configured.
  static bool ConfigureWiphyIndex(Nl80211Message* msg, int32_t index);
  // Creates and sets attributes in an SetWakeOnPacketConnMessage |msg| so that
  // the message will disable wake-on-packet functionality of the NIC with wiphy
  // index |wiphy_index|. Returns true iff |msg| is successfully configured.
  // NOTE: Assumes that |msg| has not been altered since construction.
  static bool ConfigureDisableWakeOnWiFiMessage(SetWakeOnPacketConnMessage* msg,
                                                uint32_t wiphy_index,
                                                Error* error);
  // Creates and sets attributes in a SetWakeOnPacketConnMessage |msg|
  // so that the message will program the NIC with wiphy index |wiphy_index|
  // with wake on wireless triggers in |trigs|. If |trigs| contains the
  // kWakeTriggerPattern trigger, the message is configured to program the NIC
  // to wake on packets from the IP addresses in |addrs|. If |trigs| contains
  // the kSSID trigger, the message is configured to program the NIC to wake on
  // the SSIDs in |ssid_whitelist|.
  // Returns true iff |msg| is successfully configured.
  // NOTE: Assumes that |msg| has not been altered since construction.
  static bool ConfigureSetWakeOnWiFiSettingsMessage(
      SetWakeOnPacketConnMessage* msg, const std::set<WakeOnWiFiTrigger>& trigs,
      const IPAddressStore& addrs, uint32_t wiphy_index,
      uint32_t net_detect_scan_period_seconds,
      const std::vector<ByteString>& ssid_whitelist,
      Error* error);
  // Helper function to ConfigureSetWakeOnWiFiSettingsMessage that creates a
  // single nested attribute inside the attribute list referenced by |patterns|
  // representing a wake-on-packet pattern matching rule with index |patnum|.
  // Returns true iff the attribute is successfully created and set.
  // NOTE: |patterns| is assumed to reference the nested attribute list
  // NL80211_WOWLAN_TRIG_PKT_PATTERN.
  // NOTE: |patnum| should be unique across multiple calls to this function to
  // prevent the formation of a erroneous nl80211 message or the overwriting of
  // pattern matching rules.
  static bool CreateSinglePattern(const IPAddress& ip_addr,
                                  AttributeListRefPtr patterns, uint8_t patnum,
                                  Error* error);
  // Creates and sets attributes in an GetWakeOnPacketConnMessage msg| so that
  // the message will request for wake-on-packet settings information from the
  // NIC with wiphy index |wiphy_index|. Returns true iff |msg| is successfully
  // configured.
  // NOTE: Assumes that |msg| has not been altered since construction.
  static bool ConfigureGetWakeOnWiFiSettingsMessage(
      GetWakeOnPacketConnMessage* msg, uint32_t wiphy_index, Error* error);
  // Given a NL80211_CMD_GET_WOWLAN response or NL80211_CMD_SET_WOWLAN request
  // |msg|, returns true iff the wake-on-wifi trigger settings in |msg| match
  // those in |trigs|. Performs the following checks for the following triggers:
  // - kWakeTriggerDisconnect: checks that the wake on disconnect flag is
  //   present and set.
  // - kIPAddress: checks that source IP addresses in |msg| match those reported
  //   in |addrs|.
  // - kSSID: checks that the SSIDs in |ssid_whitelist| and the scan interval
  //   |net_detect_scan_period_seconds| match those reported in |msg|.
  // Note: finding a trigger is in |msg| that is not expected based on the flags
  // in |trig| also counts as a mismatch.
  static bool WakeOnWiFiSettingsMatch(
      const Nl80211Message& msg, const std::set<WakeOnWiFiTrigger>& trigs,
      const IPAddressStore& addrs, uint32_t net_detect_scan_period_seconds,
      const std::vector<ByteString>& ssid_whitelist);
  // Handler for NL80211 message error responses from NIC wake on WiFi setting
  // programming attempts.
  void OnWakeOnWiFiSettingsErrorResponse(
      NetlinkManager::AuxilliaryMessageType type,
      const NetlinkMessage* raw_message);
  // Message handler for NL80211_CMD_SET_WOWLAN responses.
  static void OnSetWakeOnPacketConnectionResponse(
      const Nl80211Message& nl80211_message);
  // Request wake on WiFi settings for this WiFi device.
  void RequestWakeOnPacketSettings();
  // Verify that the wake on WiFi settings programmed into the NIC match
  // those recorded locally for this device in |wake_on_packet_connections_|,
  // |wake_on_wifi_triggers_|, and |wake_on_ssid_whitelist_|.
  void VerifyWakeOnWiFiSettings(const Nl80211Message& nl80211_message);
  // Sends an NL80211 message to program the NIC with wake on WiFi settings
  // configured in |wake_on_packet_connections_|, |wake_on_ssid_whitelist_|, and
  // |wake_on_wifi_triggers_|. If |wake_on_wifi_triggers_| is empty, calls
  // WakeOnWiFi::DisableWakeOnWiFi.
  void ApplyWakeOnWiFiSettings();
  // Helper function called by |ApplyWakeOnWiFiSettings| that sends an NL80211
  // message to program the NIC to disable wake on WiFi.
  void DisableWakeOnWiFi();
  // Calls |ApplyWakeOnWiFiSettings| and counts this call as
  // a retry. If |kMaxSetWakeOnPacketRetries| retries have already been
  // performed, resets counter and returns.
  void RetrySetWakeOnPacketConnections();
  // Utility functions to check which wake on WiFi features are currently
  // enabled based on the descriptor |wake_on_wifi_features_enabled_| and
  // are supported by the NIC.
  bool WakeOnWiFiPacketEnabledAndSupported();
  bool WakeOnWiFiDarkConnectEnabledAndSupported();
  // Called by metrics_timer_ to reports metrics.
  void ReportMetrics();
  // Actions executed before normal suspend and dark resume suspend.
  //
  // Arguments:
  //  - |is_connected|: whether the WiFi device is connected.
  //  - |start_lease_renewal_timer|: whether or not to start the DHCP lease
  //    renewal timer.
  //  - |time_to_next_lease_renewal|: number of seconds until next DHCP lease
  //    renewal is due.
  //  - |remove_supplicant_networks_callback|: callback to invoke
  //    to remove all networks from WPA supplicant.
  void BeforeSuspendActions(
      bool is_connected,
      bool start_lease_renewal_timer,
      uint32_t time_to_next_lease_renewal,
      const base::Closure& remove_supplicant_networks_callback);

  // Needed for |dhcp_lease_renewal_timer_| and |wake_to_scan_timer_| since
  // passing a empty base::Closure() causes a run-time DCHECK error when
  // SimpleAlarmTimer::Start or SimpleAlarmTimer::Reset are called.
  void OnTimerWakeDoNothing() {}

  // Parses an attribute list containing the SSID matches that caused the
  // system wake, along with the corresponding channels that these SSIDs were
  // detected in. Returns a set of unique frequencies that the reported
  // SSID matches occured in.
  //
  // Arguments:
  //  - |results_list|: Nested attribute list containing an array of nested
  //    attributes which contain the NL80211_ATTR_SSID or
  //    NL80211_ATTR_SCAN_FREQUENCIES attributes. This attribute list is assumed
  //    to have been extracted from a NL80211_CMD_SET_WOWLAN response message
  //    using the NL80211_WOWLAN_TRIG_NET_DETECT_RESULTS id.
  static WiFi::FreqSet ParseWakeOnSSIDResults(
      AttributeListConstRefPtr results_list);

  // Sets the |dark_resume_scan_retries_left_| counter if necessary, then runs
  // |initiate_scan_callback| with |freqs|.
  void InitiateScanInDarkResume(
      const InitiateScanCallback& initiate_scan_callback,
      const WiFi::FreqSet& freqs);

  // Pointers to objects owned by the WiFi object that created this object.
  EventDispatcher* dispatcher_;
  NetlinkManager* netlink_manager_;
  Metrics* metrics_;
  // Executes after the NIC's wake-on-packet settings are configured via
  // NL80211 messages to verify that the new configuration has taken effect.
  // Calls RequestWakeOnPacketSettings.
  base::CancelableClosure verify_wake_on_packet_settings_callback_;
  // Callback to be invoked after all suspend actions finish executing both
  // before regular suspend and before suspend in dark resume.
  ResultCallback suspend_actions_done_callback_;
  // Callback to report wake on WiFi related metrics.
  base::CancelableClosure report_metrics_callback_;
  // Number of retry attempts to program the NIC's wake-on-packet settings.
  int num_set_wake_on_packet_retries_;
  // Keeps track of triggers that the NIC will be programmed to wake from
  // while suspended.
  std::set<WakeOnWiFi::WakeOnWiFiTrigger> wake_on_wifi_triggers_;
  // Keeps track of what wake on wifi triggers this WiFi device supports.
  std::set<WakeOnWiFi::WakeOnWiFiTrigger> wake_on_wifi_triggers_supported_;
  // Max number of patterns this WiFi device can be programmed to wake on at one
  // time.
  size_t wake_on_wifi_max_patterns_;
  // Max number of SSIDs this WiFi device can be programmed to wake on at one
  // time.
  uint32_t wake_on_wifi_max_ssids_;
  // Keeps track of IP addresses whose packets this device will wake upon
  // receiving while the device is suspended. Only used if the NIC is programmed
  // to wake on IP address patterns.
  IPAddressStore wake_on_packet_connections_;
  // Keeps track of SSIDs that this device will wake on the appearance of while
  // the device is suspended. Only used if the NIC is programmed to wake on
  // SSIDs.
  std::vector<ByteString> wake_on_ssid_whitelist_;
  uint32_t wiphy_index_;
  bool wiphy_index_received_;
  // Describes the wake on WiFi features that are currently enabled.
  std::string wake_on_wifi_features_enabled_;
  // Timer that wakes the system to renew DHCP leases.
  timers::SimpleAlarmTimer dhcp_lease_renewal_timer_;
  // Timer that wakes the system to scan for networks.
  timers::SimpleAlarmTimer wake_to_scan_timer_;
  // Executes when the dark resume actions timer expires. Calls
  // ScanTimerHandler.
  base::CancelableClosure dark_resume_actions_timeout_callback_;
  // Whether shill is currently in dark resume.
  bool in_dark_resume_;
  // Period (in seconds) between instances where the system wakes from suspend
  // to scan for networks in dark resume.
  uint32_t wake_to_scan_period_seconds_;
  // Period (in seconds) between instances where the NIC performs Net Detect
  // scans while the system is suspended.
  uint32_t net_detect_scan_period_seconds_;
  // Timestamps of dark resume wakes that took place during the current
  // or most recent suspend.
  EventHistory dark_resume_history_;
  // Last wake reason reported by the kernel.
  WakeOnWiFiTrigger last_wake_reason_;
  // Whether or not to always start |wake_to_scan_timer_| before suspend.
  bool force_wake_to_scan_timer_;
  // Frequencies that the last wake on SSID matches reported by the kernel
  // occurred in.
  WiFi::FreqSet last_ssid_match_freqs_;
  // How many more times to retry the last dark resume scan that shill launched
  // if no auto-connectable services were found.
  int dark_resume_scan_retries_left_;

  // Callback invoked to report the wake reason for the current dark resume to
  // powerd.
  RecordWakeReasonCallback record_wake_reason_callback_;

  base::WeakPtrFactory<WakeOnWiFi> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WakeOnWiFi);
};

}  // namespace shill

#endif  // SHILL_WIFI_WAKE_ON_WIFI_H_
