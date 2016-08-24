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

#ifndef SHILL_WIFI_SCAN_SESSION_H_
#define SHILL_WIFI_SCAN_SESSION_H_

#include <deque>
#include <set>
#include <vector>

#include <base/callback.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST
#include <metrics/timer.h>

#include "shill/net/byte_string.h"
#include "shill/net/netlink_manager.h"
#include "shill/wifi/wifi_provider.h"

namespace shill {

class EventDispatcher;
class Metrics;
class NetlinkManager;
class NetlinkMessage;
class Nl80211Message;

// |ScanSession| sends requests to the kernel to scan WiFi frequencies for
// access points.  The sequence for a single scan is as follows:
//
//   +-------------+                                                +--------+
//   | ScanSession |                                                | Kernel |
//   +---+---------+                                                +-----+--+
//       |--- NL80211_CMD_TRIGGER_SCAN ---------------------------------->|
//       |<-- NL80211_CMD_TRIGGER_SCAN (broadcast) -----------------------|
//       |<-- NL80211_CMD_NEW_SCAN_RESULTS (broadcast) -------------------|
//       |--- NL80211_CMD_GET_SCAN -------------------------------------->|
//       |<-- NL80211_CMD_NEW_SCAN_RESULTS (reply, unicast, NLM_F_MULTI) -|
//       |<-- NL80211_CMD_NEW_SCAN_RESULTS (reply, unicast, NLM_F_MULTI) -|
//       |                               ...                              |
//       |<-- NL80211_CMD_NEW_SCAN_RESULTS (reply, unicast, NLM_F_MULTI) -|
//       |                                                                |
//
// Scanning WiFi frequencies for access points takes a long time (on the order
// of 100ms per frequency and the kernel doesn't return the result until the
// answers are ready for all the frequencies in the batch).  Given this,
// scanning all frequencies in one batch takes a very long time.
//
// A ScanSession is used to distribute a scan across multiple requests (hoping
// that a successful connection will result from an early request thereby
// obviating the need for the remainder of the scan).  A ScanSession can be
// used as follows (note, this is shown as synchronous code for clarity
// but it really should be implemented as asynchronous code):
//
// ScanSession::FractionList scan_fractions;
// scan_fractions.push_back(<some value>);
// ...
// scan_fractions.push_back(<some value>);
// ScanSession scan_session(netlink_manager_, dispatcher(),
//                          frequencies_seen_ever, all_scan_frequencies_,
//                          interface_index(), scan_fractions,
//                          kMinScanFrequencies, kMaxScanFrequencies,
//                          on_scan_failed);
// while (scan_session.HasMoreFrequencies()) {
//   scan_session.InitiateScan();
//   // Wait for scan results.  In the current WiFi code, this means wait
//   // until |WiFi::ScanDone| is called.
// }

class ScanSession {
 public:
  typedef base::Closure OnScanFailed;
  typedef std::deque<float> FractionList;
  // Used as a fraction in |FractionList| to indicate that future scans in
  // this session should not be limited to a subset of the frequencies we've
  // already seen.
  static const float kAllFrequencies;

  // Sets up a new progressive scan session.  Uses |netlink_manager| to send
  // NL80211_CMD_TRIGGER_SCAN messages to the kernel (uses |dispatcher| to
  // reissue those commands if a send request returns EBUSY).  Multiple scans
  // for APs on wifi device |ifindex| are issued (one for each call to
  // |InitiateScan|) on wifi frequencies taken from the union of unique
  // frequencies in |previous_frequencies| and |available_frequencies| (most
  // commonly seen frequencies before less commonly seen ones followed by
  // never-before seen frequencies, the latter in an unspecified order).
  //
  // Each scan takes a greater percentile (described by the values in
  // |fractions|) of the previously seen frequencies (but no less than
  // |min_frequencies| and no more than |max_frequencies|).  After all
  // previously seen frequencies have been requested, each |InitiateScan|
  // scans the next |max_frequencies| until all |available_frequencies| have
  // been exhausted.
  //
  // If a scan request to the kernel returns an error, |on_scan_failed| is
  // called.  The caller can reissue the scan by calling |ReInitiateScan| or
  // abort the scan session by deleting the |ScanSession| object.
  ScanSession(NetlinkManager* netlink_manager,
              EventDispatcher* dispatcher,
              const WiFiProvider::FrequencyCountList& previous_frequencies,
              const std::set<uint16_t>& available_frequencies,
              uint32_t ifindex,
              const FractionList& fractions,
              size_t min_frequencies,
              size_t max_frequencies,
              OnScanFailed on_scan_failed,
              Metrics* metrics);

  virtual ~ScanSession();

  // Returns true if |ScanSession| contains unscanned frequencies.
  virtual bool HasMoreFrequencies() const;

  // Adds an SSID to the list of things for which to scan.  Useful for hidden
  // SSIDs.
  virtual void AddSsid(const ByteString& ssid);

  // Start a wifi scan of the next set of frequencies (derived from the
  // constructor's parameters) after saving those frequencies for the potential
  // need to reinitiate a scan.
  virtual void InitiateScan();

  // Re-issues the previous scan (i.e., it uses the same frequency list as the
  // previous scan).  Other classes may use this when |on_scan_failed| is
  // called.  Called by |OnTriggerScanResponse| when the previous attempt to do
  // a scan fails.
  void ReInitiateScan();

 private:
  friend class ScanSessionTest;
  friend class WiFiObjectTest;  // OnTriggerScanResponse.
  FRIEND_TEST(ScanSessionTest, EBusy);
  FRIEND_TEST(ScanSessionTest, OnError);
  FRIEND_TEST(ScanSessionTest, OnTriggerScanResponse);

  // Milliseconds to wait before retrying a failed scan.
  static const uint64_t kScanRetryDelayMilliseconds;
  // Number of times to retry a failed scan before giving up and calling
  // |on_scan_failed_|.
  static const size_t kScanRetryCount;

  // Assists with sorting the |previous_frequencies| passed to the
  // constructor.
  static bool CompareFrequencyCount(const WiFiProvider::FrequencyCount& first,
                                    const WiFiProvider::FrequencyCount& second);

  // |GetScanFrequencies| gets the next set of WiFi scan frequencies.  Returns
  // at least |min_frequencies| (unless fewer frequencies remain from previous
  // calls) and no more than |max_frequencies|.  Inside these constraints,
  // |GetScanFrequencies| tries to return at least the number of frequencies
  // required to reach the connection fraction |scan_fraction| out of the total
  // number of previous connections.  For example, the first call requesting
  // 33.3% will return the minimum number frequencies that add up to _at least_
  // the 33.3rd percentile of frequencies to which we've successfully connected
  // in the past.  The next call of 33.3% returns the minimum number of
  // frequencies required so that the total of the frequencies returned are _at
  // least_ the 66.6th percentile of the frequencies to which we've successfully
  // connected.
  //
  // For example, say we've connected to 3 frequencies before:
  //  freq a,count=10; freq b,count=5; freq c,count=5.
  //
  //  GetScanFrequencies(.50,2,10) // Returns a & b (|a| reaches %ile but |b| is
  //                               // required to meet the minimum).
  //  GetScanFrequencies(.51,2,10) // Returns c & 9 frequencies from the list
  //                               // of frequencies to which we've never
  //                               // connected.
  virtual std::vector<uint16_t> GetScanFrequencies(float scan_fraction,
                                                   size_t min_frequencies,
                                                   size_t max_frequencies);

  // Does the real work of initiating a scan by sending an
  // NL80211_CMD_TRIGGER_SCAN message to the kernel and installing a handler for
  // any response (which only happens in the error case).
  void DoScan(const std::vector<uint16_t>& scan_frequencies);

  // Handles any unicast response to NL80211_CMD_TRIGGER_SCAN (which is,
  // likely, an error -- when things work, we get an
  // NL80211_CMD_NEW_SCAN_RESULTS broadcast message).
  void OnTriggerScanResponse(const Nl80211Message& message);
  void OnTriggerScanErrorResponse(NetlinkManager::AuxilliaryMessageType type,
                                  const NetlinkMessage* netlink_message);
  void ReportEbusyTime(int log_level);

  // Logs the results of the scan.
  void ReportResults(int log_level);

  base::WeakPtrFactory<ScanSession> weak_ptr_factory_;

  NetlinkManager* netlink_manager_;
  EventDispatcher* dispatcher_;

  // List of frequencies, sorted by the number of successful connections for
  // each frequency.
  WiFiProvider::FrequencyCountList frequency_list_;
  size_t total_connections_;
  size_t total_connects_provided_;
  float total_fraction_wanted_;
  std::vector<uint16_t> current_scan_frequencies_;
  uint32_t wifi_interface_index_;
  std::set<ByteString, bool(*)(const ByteString&, const ByteString&)> ssids_;
  FractionList fractions_;
  size_t min_frequencies_;
  size_t max_frequencies_;
  OnScanFailed on_scan_failed_;
  size_t scan_tries_left_;
  bool found_error_;

  // Statistics gathering.
  size_t original_frequency_count_;
  chromeos_metrics::Timer ebusy_timer_;
  Metrics* metrics_;

  DISALLOW_COPY_AND_ASSIGN(ScanSession);
};

}  // namespace shill.

#endif  // SHILL_WIFI_SCAN_SESSION_H_
