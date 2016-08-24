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

#include "shill/wifi/scan_session.h"

#include <errno.h>

#include <limits>
#include <memory>
#include <set>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_event_dispatcher.h"
#include "shill/net/mock_netlink_manager.h"
#include "shill/net/netlink_manager.h"
#include "shill/net/netlink_message_matchers.h"
#include "shill/net/nl80211_message.h"

using std::set;
using std::vector;
using testing::_;
using testing::ContainerEq;
using testing::Test;

namespace shill {

static const uint16_t kExpectedFreq5640 = 5640;
static const uint16_t kExpectedFreq5600 = 5600;
static const uint16_t kExpectedFreq5580 = 5580;
static const uint16_t kExpectedFreq5560 = 5560;
static const uint16_t kExpectedFreq5620 = 5620;

static WiFiProvider::FrequencyCount kConnectedFrequencies[] = {
  WiFiProvider::FrequencyCount(kExpectedFreq5640, 40),  // 40th percentile.
  WiFiProvider::FrequencyCount(kExpectedFreq5600, 25),  // 65th percentile.
  WiFiProvider::FrequencyCount(kExpectedFreq5580, 20),  // 85th percentile.
  WiFiProvider::FrequencyCount(kExpectedFreq5560, 10),  // 95th percentile.
  WiFiProvider::FrequencyCount(kExpectedFreq5620, 5)    // 100th percentile.
};

static const uint16_t kExpectedFreq2432 = 2432;
static const uint16_t kExpectedFreq2427 = 2427;
static const uint16_t kExpectedFreq2422 = 2422;
static const uint16_t kExpectedFreq2417 = 2417;
static const uint16_t kExpectedFreq2412 = 2412;

static uint16_t kUnconnectedFrequencies[] = {
  kExpectedFreq2432,
  kExpectedFreq2427,
  kExpectedFreq2422,
  kExpectedFreq2417,
  kExpectedFreq2412
};

static const uint16_t kNl80211FamilyId = 0x13;

class ScanSessionTest : public Test {
 public:
  // Test set of "all the other frequencies this device can support" in
  // sorted order.
  ScanSessionTest() : weak_ptr_factory_(this) {
    WiFiProvider::FrequencyCountList default_connected_frequencies(
        kConnectedFrequencies,
        kConnectedFrequencies + arraysize(kConnectedFrequencies));

    set<uint16_t> default_unconnected_frequencies(
        kUnconnectedFrequencies,
        kUnconnectedFrequencies + arraysize(kUnconnectedFrequencies));

    BuildScanSession(default_connected_frequencies,
                     default_unconnected_frequencies);
  }

  void BuildScanSession(const WiFiProvider::FrequencyCountList
                            &connected_frequencies,
                        const std::set<uint16_t>& unconnected_frequencies) {
    const int kArbitraryMinimum = 1;
    const int kArbitraryMaximum = std::numeric_limits<int>::max();
    scan_session_.reset(new ScanSession(&netlink_manager_,
                                        &dispatcher_,
                                        connected_frequencies,
                                        unconnected_frequencies,
                                        0,
                                        ScanSession::FractionList(),
                                        kArbitraryMinimum,
                                        kArbitraryMaximum,
                                        Bind(&ScanSessionTest::OnScanError,
                                             weak_ptr_factory_.GetWeakPtr()),
                                        nullptr));
  }

  virtual std::vector<uint16_t> GetScanFrequencies(float scan_fraction,
                                                   size_t min_frequencies,
                                                   size_t max_frequencies) {
    return scan_session_->GetScanFrequencies(scan_fraction, min_frequencies,
                                             max_frequencies);
  }
  ScanSession* scan_session() { return scan_session_.get(); }

  void SetScanSize(size_t min_frequencies, size_t max_frequencies) {
    scan_session_->min_frequencies_ = min_frequencies;
    scan_session_->max_frequencies_ = max_frequencies;
  }

  size_t GetScanFrequencyCount() {
    return arraysize(kConnectedFrequencies) +
        arraysize(kUnconnectedFrequencies);
  }

 protected:
  MOCK_METHOD0(OnScanError, void());
  MockNetlinkManager* netlink_manager() { return &netlink_manager_; }
  MockEventDispatcher* dispatcher() { return &dispatcher_; }

  MockEventDispatcher dispatcher_;
  MockNetlinkManager netlink_manager_;
  std::unique_ptr<ScanSession> scan_session_;
  base::WeakPtrFactory<ScanSessionTest> weak_ptr_factory_;
};

// Test that we can get a bunch of frequencies up to a specified fraction.
TEST_F(ScanSessionTest, Fraction) {
  vector<uint16_t> result;

  // Get the first 83% of the connected values.
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580};
    result = GetScanFrequencies(.83, 1, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // Get the next 4 values.
  {
    vector<uint16_t> expected{kExpectedFreq5560, kExpectedFreq5620,
      kExpectedFreq2412, kExpectedFreq2417};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 4);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the remaining list.
  {
    vector<uint16_t> expected{kExpectedFreq2422, kExpectedFreq2427,
      kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 20,
                                std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get a bunch of frequencies up to a specified fraction,
// followed by another group up to a specified fraction.
TEST_F(ScanSessionTest, TwoFractions) {
  vector<uint16_t> result;

  // Get the first 60% of the connected values.
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600};
    result = GetScanFrequencies(.60, 0, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // Get the next 32% of the connected values.
  {
    vector<uint16_t> expected{kExpectedFreq5580, kExpectedFreq5560};
    result = GetScanFrequencies(.32, 0, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the remaining list.
  {
    vector<uint16_t> expected{kExpectedFreq5620, kExpectedFreq2412,
      kExpectedFreq2417, kExpectedFreq2422, kExpectedFreq2427,
      kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies,
                                std::numeric_limits<size_t>::max(),
                                std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get a bunch of frequencies up to a minimum count, even
// when the requested fraction has already been reached.
TEST_F(ScanSessionTest, Min) {
  vector<uint16_t> result;

  // Get the first 3 previously seen values.
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580};
    result = GetScanFrequencies(.30, 3, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // Get the next value by requesting a minimum of 1.
  {
    vector<uint16_t> expected{kExpectedFreq5560};
    result = GetScanFrequencies(0.0, 1, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the remaining list.
  {
    vector<uint16_t> expected{kExpectedFreq5620, kExpectedFreq2412,
      kExpectedFreq2417, kExpectedFreq2422, kExpectedFreq2427,
      kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies,
                                std::numeric_limits<size_t>::max(),
                                std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get up to a specified maximum number of frequencies.
TEST_F(ScanSessionTest, Max) {
  vector<uint16_t> result;

  // Get the first 7 values (crosses seen/unseen boundary).
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580, kExpectedFreq5560, kExpectedFreq5620,
      kExpectedFreq2412, kExpectedFreq2417};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 7);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the remaining list.
  {
    vector<uint16_t> expected{kExpectedFreq2422, kExpectedFreq2427,
      kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 20,
                                std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get exactly the seen frequencies and exactly the unseen
// ones.
TEST_F(ScanSessionTest, Exact) {
  vector<uint16_t> result;

  // Get the first 5 values -- exactly on the seen/unseen border.
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580, kExpectedFreq5560, kExpectedFreq5620};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 5, 5);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the last 5.
  {
    vector<uint16_t> expected{kExpectedFreq2412, kExpectedFreq2417,
      kExpectedFreq2422, kExpectedFreq2427, kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies, 5, 5);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get everything in one read.
TEST_F(ScanSessionTest, AllOneRead) {
  vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
    kExpectedFreq5580, kExpectedFreq5560, kExpectedFreq5620,
    kExpectedFreq2412, kExpectedFreq2417, kExpectedFreq2422,
    kExpectedFreq2427, kExpectedFreq2432};
  vector<uint16_t> result;
  result = GetScanFrequencies(ScanSession::kAllFrequencies,
                              std::numeric_limits<size_t>::max(),
                              std::numeric_limits<size_t>::max());
  EXPECT_THAT(result, ContainerEq(expected));
  EXPECT_FALSE(scan_session()->HasMoreFrequencies());
}

// Test that we can get all the previously seen frequencies (and only the
// previously seen frequencies) via the requested fraction.
TEST_F(ScanSessionTest, EverythingConnected) {
  vector<uint16_t> result;

  // Get the first 100% of the connected values.
  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580, kExpectedFreq5560, kExpectedFreq5620};
    result = GetScanFrequencies(1.0, 0, std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  // And, get the remaining list.
  {
    vector<uint16_t> expected{kExpectedFreq2412, kExpectedFreq2417,
      kExpectedFreq2422, kExpectedFreq2427, kExpectedFreq2432};
    result = GetScanFrequencies(ScanSession::kAllFrequencies,
                                std::numeric_limits<size_t>::max(),
                                std::numeric_limits<size_t>::max());
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

TEST_F(ScanSessionTest, OnlyPreviouslySeen) {
  // Build a scan session with only previously connected frequencies.
  WiFiProvider::FrequencyCountList default_connected_frequencies(
      kConnectedFrequencies,
      kConnectedFrequencies + arraysize(kConnectedFrequencies));
  BuildScanSession(default_connected_frequencies, std::set<uint16_t>());

  // Get the first 100% of the connected values.
  vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
    kExpectedFreq5580, kExpectedFreq5560, kExpectedFreq5620};

  vector<uint16_t> result;
  result = GetScanFrequencies(ScanSession::kAllFrequencies, 1,
                              std::numeric_limits<size_t>::max());
  EXPECT_THAT(result, ContainerEq(expected));
  EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  result = GetScanFrequencies(ScanSession::kAllFrequencies,
                              std::numeric_limits<size_t>::max(),
                              std::numeric_limits<size_t>::max());
  EXPECT_TRUE(result.empty());
}

// Verify that max works inside the list of connected frequencies.
TEST_F(ScanSessionTest, MaxAppliesToConnected) {
  vector<uint16_t> result;

  {
    vector<uint16_t> expected{kExpectedFreq5640, kExpectedFreq5600,
      kExpectedFreq5580};

    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 3);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  {
    vector<uint16_t> expected{kExpectedFreq5560, kExpectedFreq5620,
      kExpectedFreq2412};

    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 3);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  {
    vector<uint16_t> expected{kExpectedFreq2417, kExpectedFreq2422,
      kExpectedFreq2427};

    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 3);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }

  {
    vector<uint16_t> expected{kExpectedFreq2432};

    result = GetScanFrequencies(ScanSession::kAllFrequencies, 1, 3);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

// Test that we can get each value individually.
TEST_F(ScanSessionTest, IndividualReads) {
  vector<uint16_t> result;
  static const float kArbitraryFraction = 0.83;

  {
    vector<uint16_t> expected{kExpectedFreq5640};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq5600};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq5580};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq5560};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq5620};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq2412};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq2417};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq2422};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq2427};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
  }
  {
    vector<uint16_t> expected{kExpectedFreq2432};
    result = GetScanFrequencies(kArbitraryFraction, 1, 1);
    EXPECT_THAT(result, ContainerEq(expected));
    EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  }
}

TEST_F(ScanSessionTest, OnTriggerScanResponse) {
  Nl80211Message::SetMessageType(kNl80211FamilyId);

  EXPECT_CALL(*netlink_manager(), SendNl80211Message(
      IsNl80211Command(kNl80211FamilyId, NL80211_CMD_TRIGGER_SCAN), _, _, _));
  scan_session()->InitiateScan();

  EXPECT_CALL(*this, OnScanError());
  NewScanResultsMessage not_supposed_to_get_this_message;
  scan_session()->OnTriggerScanResponse(not_supposed_to_get_this_message);
}

TEST_F(ScanSessionTest, ExhaustFrequencies) {
  // Set min & max scan frequency count to 1 so each scan will be of a single
  // frequency.
  SetScanSize(1, 1);

  // Perform all the progressive scans until the frequencies are exhausted.
  for (size_t i = 0; i < GetScanFrequencyCount(); ++i) {
    EXPECT_TRUE(scan_session()->HasMoreFrequencies());
    EXPECT_CALL(*netlink_manager(), SendNl80211Message(
        IsNl80211Command(kNl80211FamilyId, NL80211_CMD_TRIGGER_SCAN), _, _, _));
    scan_session()->InitiateScan();
  }

  EXPECT_FALSE(scan_session()->HasMoreFrequencies());
  EXPECT_CALL(*netlink_manager(), SendNl80211Message(
      IsNl80211Command(kNl80211FamilyId, NL80211_CMD_TRIGGER_SCAN), _, _, _))
      .Times(0);
  scan_session()->InitiateScan();
}

TEST_F(ScanSessionTest, OnError) {
  Nl80211Message::SetMessageType(kNl80211FamilyId);

  EXPECT_CALL(*netlink_manager(), SendNl80211Message(
      IsNl80211Command(kNl80211FamilyId, NL80211_CMD_TRIGGER_SCAN), _, _, _));
  scan_session()->InitiateScan();

  EXPECT_CALL(*this, OnScanError());
  ErrorAckMessage error_message(-EINTR);
  scan_session()->OnTriggerScanErrorResponse(NetlinkManager::kErrorFromKernel,
                                             &error_message);
}

TEST_F(ScanSessionTest, EBusy) {
  const size_t kSmallRetryNumber = 3;
  Nl80211Message::SetMessageType(kNl80211FamilyId);
  scan_session()->scan_tries_left_ = kSmallRetryNumber;

  EXPECT_CALL(*netlink_manager(), SendNl80211Message(
      IsNl80211Command(kNl80211FamilyId, NL80211_CMD_TRIGGER_SCAN), _, _, _));
  scan_session()->InitiateScan();

  ErrorAckMessage error_message(-EBUSY);
  for (size_t i = 0; i < kSmallRetryNumber; ++i) {
    EXPECT_CALL(*this, OnScanError()).Times(0);
    EXPECT_CALL(*dispatcher(), PostDelayedTask(_, _));
    scan_session()->OnTriggerScanErrorResponse(NetlinkManager::kErrorFromKernel,
                                               &error_message);
  }

  EXPECT_CALL(*this, OnScanError());
  scan_session()->OnTriggerScanErrorResponse(NetlinkManager::kErrorFromKernel,
                                             &error_message);
}

TEST_F(ScanSessionTest, ScanHidden) {
  scan_session_->AddSsid(ByteString("a", 1));
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(HasHiddenSSID(kNl80211FamilyId), _, _, _));
  scan_session()->InitiateScan();
}

TEST_F(ScanSessionTest, ScanNoHidden) {
  EXPECT_CALL(netlink_manager_,
              SendNl80211Message(HasNoHiddenSSID(kNl80211FamilyId), _, _, _));
  scan_session()->InitiateScan();
}

}  // namespace shill
