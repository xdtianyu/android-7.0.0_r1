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

#include "shill/portal_detector.h"

#include <memory>
#include <string>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/connectivity_trial.h"
#include "shill/mock_connection.h"
#include "shill/mock_connectivity_trial.h"
#include "shill/mock_control.h"
#include "shill/mock_device_info.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/net/mock_time.h"

using base::Bind;
using base::Callback;
using base::Unretained;
using std::string;
using std::vector;
using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::SetArgumentPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kBadURL[] = "badurl";
const char kInterfaceName[] = "int0";
const char kURL[] = "http://www.chromium.org";
const char kDNSServer0[] = "8.8.8.8";
const char kDNSServer1[] = "8.8.4.4";
const char* kDNSServers[] = { kDNSServer0, kDNSServer1 };
}  // namespace

MATCHER_P(IsResult, result, "") {
  return (result.trial_result.phase == arg.trial_result.phase &&
          result.trial_result.status == arg.trial_result.status &&
          result.final == arg.final);
}


class PortalDetectorTest : public Test {
 public:
  PortalDetectorTest()
      : device_info_(
            new NiceMock<MockDeviceInfo>(&control_, nullptr, nullptr, nullptr)),
        connection_(new StrictMock<MockConnection>(device_info_.get())),
        portal_detector_(
            new PortalDetector(connection_.get(), &dispatcher_,
                               callback_target_.result_callback())),
        connectivity_trial_(new StrictMock<MockConnectivityTrial>(
            connection_, PortalDetector::kRequestTimeoutSeconds)),
        interface_name_(kInterfaceName),
        dns_servers_(kDNSServers, kDNSServers + 2) {
    current_time_.tv_sec = current_time_.tv_usec = 0;
  }

  virtual void SetUp() {
    EXPECT_CALL(*connection_.get(), IsIPv6())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*connection_.get(), interface_name())
        .WillRepeatedly(ReturnRef(interface_name_));
    portal_detector_->time_ = &time_;
    EXPECT_CALL(time_, GetTimeMonotonic(_))
        .WillRepeatedly(Invoke(this, &PortalDetectorTest::GetTimeMonotonic));
    EXPECT_CALL(*connection_.get(), dns_servers())
        .WillRepeatedly(ReturnRef(dns_servers_));
    portal_detector_->connectivity_trial_
        .reset(connectivity_trial_);  // Passes ownership
    EXPECT_TRUE(portal_detector()->connectivity_trial_.get());
  }

  virtual void TearDown() {
    if (portal_detector()->connectivity_trial_.get()) {
      EXPECT_CALL(*connectivity_trial(), Stop());

      // Delete the portal detector while expectations still exist.
      portal_detector_.reset();
    }
  }

 protected:
  static const int kNumAttempts;

  class CallbackTarget {
   public:
    CallbackTarget()
        : result_callback_(Bind(&CallbackTarget::ResultCallback,
                                Unretained(this))) {
    }

    MOCK_METHOD1(ResultCallback, void(const PortalDetector::Result& result));
    Callback<void(const PortalDetector::Result&)>& result_callback() {
      return result_callback_;
    }

   private:
    Callback<void(const PortalDetector::Result&)> result_callback_;
  };

  bool StartPortalRequest(const string& url_string) {
    bool ret = portal_detector_->Start(url_string);
    return ret;
  }

  PortalDetector* portal_detector() { return portal_detector_.get(); }
  MockConnectivityTrial* connectivity_trial() { return connectivity_trial_; }
  MockEventDispatcher& dispatcher() { return dispatcher_; }
  CallbackTarget& callback_target() { return callback_target_; }

  void ExpectReset() {
    EXPECT_FALSE(portal_detector_->attempt_count_);
    EXPECT_FALSE(portal_detector_->failures_in_content_phase_);
    EXPECT_TRUE(callback_target_.result_callback().
                Equals(portal_detector_->portal_result_callback_));
  }

  void ExpectAttemptRetry(const PortalDetector::Result& result) {
    EXPECT_CALL(callback_target(),
                ResultCallback(IsResult(result)));
    EXPECT_CALL(*connectivity_trial(),
                Retry(PortalDetector::kMinTimeBetweenAttemptsSeconds * 1000));
  }

  void AdvanceTime(int milliseconds) {
    struct timeval tv = { milliseconds / 1000, (milliseconds % 1000) * 1000 };
    timeradd(&current_time_, &tv, &current_time_);
  }

  void StartAttempt() {
    EXPECT_CALL(*connectivity_trial(), Start(_, _)).WillOnce(Return(true));

    EXPECT_TRUE(StartPortalRequest(kURL));
  }

 private:
  int GetTimeMonotonic(struct timeval* tv) {
    *tv = current_time_;
    return 0;
  }

  StrictMock<MockEventDispatcher> dispatcher_;
  MockControl control_;
  std::unique_ptr<MockDeviceInfo> device_info_;
  scoped_refptr<MockConnection> connection_;
  CallbackTarget callback_target_;
  std::unique_ptr<PortalDetector> portal_detector_;
  MockConnectivityTrial* connectivity_trial_;
  StrictMock<MockTime> time_;
  struct timeval current_time_;
  const string interface_name_;
  vector<string> dns_servers_;
};

// static
const int PortalDetectorTest::kNumAttempts = 0;

TEST_F(PortalDetectorTest, Constructor) {
  ExpectReset();
}

TEST_F(PortalDetectorTest, InvalidURL) {
  EXPECT_CALL(*connectivity_trial(), Start(_, _)).WillOnce(Return(false));
  EXPECT_FALSE(portal_detector()->Start(kBadURL));
  ExpectReset();
}

TEST_F(PortalDetectorTest, StartAttemptFailed) {
  EXPECT_CALL(*connectivity_trial(), Start(kURL, 0)).WillOnce(Return(true));
  EXPECT_TRUE(StartPortalRequest(kURL));

  // Expect that the request will be started -- return failure.
  ConnectivityTrial::Result errorResult =
      ConnectivityTrial::GetPortalResultForRequestResult(
          HTTPRequest::kResultConnectionFailure);

  // Expect a non-final failure to be relayed to the caller.
  ExpectAttemptRetry(
      PortalDetector::Result(
          ConnectivityTrial::Result(
              ConnectivityTrial::kPhaseConnection,
              ConnectivityTrial::kStatusFailure),
          kNumAttempts,
          false));

  portal_detector()->CompleteAttempt(errorResult);
}

TEST_F(PortalDetectorTest, IsInProgress) {
  EXPECT_FALSE(portal_detector()->IsInProgress());
  // Starting the attempt immediately should result with IsInProgress returning
  // true
  EXPECT_CALL(*connectivity_trial(), Start(_, _))
      .Times(2).WillRepeatedly(Return(true));
  EXPECT_TRUE(StartPortalRequest(kURL));
  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(true));
  EXPECT_TRUE(portal_detector()->IsInProgress());

  // Starting the attempt with a delay should result with IsInProgress returning
  // false
  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(false));
  portal_detector()->StartAfterDelay(kURL, 2);
  EXPECT_FALSE(portal_detector()->IsInProgress());

  // Advance time, IsInProgress should now be true
  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(true));
  AdvanceTime(2000);
  EXPECT_TRUE(portal_detector()->IsInProgress());

  // Times beyond the start time before the attempt finishes should also return
  // true
  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(true));
  AdvanceTime(1000);
  EXPECT_TRUE(portal_detector()->IsInProgress());
}

TEST_F(PortalDetectorTest, AdjustStartDelayImmediate) {
  EXPECT_CALL(*connectivity_trial(), Start(kURL, 0)).WillOnce(Return(true));
  EXPECT_TRUE(StartPortalRequest(kURL));

  // A second attempt should be delayed by kMinTimeBetweenAttemptsSeconds.
  EXPECT_TRUE(portal_detector()->AdjustStartDelay(0)
              == PortalDetector::kMinTimeBetweenAttemptsSeconds);
}

TEST_F(PortalDetectorTest, AdjustStartDelayAfterDelay) {
  const int kDelaySeconds = 123;
  // The first attempt should be delayed by kDelaySeconds.
  EXPECT_CALL(*connectivity_trial(), Start(kURL, kDelaySeconds * 1000))
      .WillOnce(Return(true));

  portal_detector()->StartAfterDelay(kURL, kDelaySeconds);

  AdvanceTime(kDelaySeconds * 1000);

  // A second attempt should be delayed by kMinTimeBetweenAttemptsSeconds.
  EXPECT_TRUE(portal_detector()->AdjustStartDelay(0)
              == PortalDetector::kMinTimeBetweenAttemptsSeconds);
}

TEST_F(PortalDetectorTest, AttemptCount) {
  EXPECT_FALSE(portal_detector()->IsInProgress());
  // Expect the PortalDetector to immediately post a task for the each attempt.
  EXPECT_CALL(*connectivity_trial(), Start(_, _))
      .Times(2).WillRepeatedly(Return(true));
  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(false));
  portal_detector()->StartAfterDelay(kURL, 2);

  EXPECT_FALSE(portal_detector()->IsInProgress());

  // Expect that the request will be started -- return failure.
  EXPECT_CALL(*connectivity_trial(), Retry(0))
      .Times(PortalDetector::kMaxRequestAttempts - 1);

  {
    InSequence s;

    // Expect non-final failures for all attempts but the last.
    EXPECT_CALL(callback_target(),
                ResultCallback(IsResult(
                    PortalDetector::Result(
                        ConnectivityTrial::Result(
                            ConnectivityTrial::kPhaseDNS,
                            ConnectivityTrial::kStatusFailure),
                        kNumAttempts,
                        false))))
        .Times(PortalDetector::kMaxRequestAttempts - 1);

    // Expect a single final failure.
    EXPECT_CALL(callback_target(),
                ResultCallback(IsResult(
                    PortalDetector::Result(
                        ConnectivityTrial::Result(
                            ConnectivityTrial::kPhaseDNS,
                            ConnectivityTrial::kStatusFailure),
                        kNumAttempts,
                        true))))
        .Times(1);
  }

  // Expect the PortalDetector to stop the ConnectivityTrial after
  // the final attempt.
  EXPECT_CALL(*connectivity_trial(), Stop()).Times(1);

  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(true));
  portal_detector()->Start(kURL);
  for (int i = 0; i < PortalDetector::kMaxRequestAttempts; i++) {
    EXPECT_TRUE(portal_detector()->IsInProgress());
    AdvanceTime(PortalDetector::kMinTimeBetweenAttemptsSeconds * 1000);
    ConnectivityTrial::Result r =
        ConnectivityTrial::GetPortalResultForRequestResult(
            HTTPRequest::kResultDNSFailure);
    portal_detector()->CompleteAttempt(r);
  }

  EXPECT_FALSE(portal_detector()->IsInProgress());
  ExpectReset();
}

// Exactly like AttemptCount, except that the termination conditions are
// different because we're triggering a different sort of error.
TEST_F(PortalDetectorTest, ReadBadHeadersRetry) {
  EXPECT_FALSE(portal_detector()->IsInProgress());
  // Expect the PortalDetector to immediately post a task for the each attempt.
  EXPECT_CALL(*connectivity_trial(), Start(_, 0))
      .Times(2).WillRepeatedly(Return(true));

  EXPECT_TRUE(StartPortalRequest(kURL));

  // Expect that the request will be started -- return failure.
  EXPECT_CALL(*connectivity_trial(), Retry(0))
      .Times(PortalDetector::kMaxFailuresInContentPhase - 1);
  {
    InSequence s;

    // Expect non-final failures for all attempts but the last.
    EXPECT_CALL(callback_target(),
                ResultCallback(IsResult(
                    PortalDetector::Result(
                        ConnectivityTrial::Result(
                            ConnectivityTrial::kPhaseContent,
                            ConnectivityTrial::kStatusFailure),
                        kNumAttempts,
                        false))))
        .Times(PortalDetector::kMaxFailuresInContentPhase - 1);

    // Expect a single final failure.
    EXPECT_CALL(callback_target(),
                ResultCallback(IsResult(
                    PortalDetector::Result(
                        ConnectivityTrial::Result(
                            ConnectivityTrial::kPhaseContent,
                            ConnectivityTrial::kStatusFailure),
                        kNumAttempts,
                        true))))
        .Times(1);
  }

  // Expect the PortalDetector to stop the current request each time, plus
  // an extra time in PortalDetector::Stop().
  EXPECT_CALL(*connectivity_trial(), Stop()).Times(1);

  EXPECT_CALL(*connectivity_trial(), IsActive()).WillOnce(Return(true));
  portal_detector()->Start(kURL);
  for (int i = 0; i < PortalDetector::kMaxFailuresInContentPhase; i++) {
    EXPECT_TRUE(portal_detector()->IsInProgress());
    AdvanceTime(PortalDetector::kMinTimeBetweenAttemptsSeconds * 1000);
    ConnectivityTrial::Result r =
        ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                  ConnectivityTrial::kStatusFailure);
    portal_detector()->CompleteAttempt(r);
  }

  EXPECT_FALSE(portal_detector()->IsInProgress());
}

TEST_F(PortalDetectorTest, ReadBadHeader) {
  StartAttempt();

  ExpectAttemptRetry(
      PortalDetector::Result(
          ConnectivityTrial::Result(
              ConnectivityTrial::kPhaseContent,
              ConnectivityTrial::kStatusFailure),
          kNumAttempts,
          false));

  ConnectivityTrial::Result r =
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                ConnectivityTrial::kStatusFailure);
  portal_detector()->CompleteAttempt(r);
}

TEST_F(PortalDetectorTest, RequestTimeout) {
  StartAttempt();
  ExpectAttemptRetry(
      PortalDetector::Result(
          ConnectivityTrial::Result(
              ConnectivityTrial::kPhaseUnknown,
              ConnectivityTrial::kStatusTimeout),
          kNumAttempts,
          false));

  ConnectivityTrial::Result r =
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseUnknown,
                                ConnectivityTrial::kStatusTimeout);
  portal_detector()->CompleteAttempt(r);
}

TEST_F(PortalDetectorTest, ReadPartialHeaderTimeout) {
  StartAttempt();

  ExpectAttemptRetry(
      PortalDetector::Result(
          ConnectivityTrial::Result(
              ConnectivityTrial::kPhaseContent,
              ConnectivityTrial::kStatusTimeout),
          kNumAttempts,
          false));

  ConnectivityTrial::Result r =
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                ConnectivityTrial::kStatusTimeout);
  portal_detector()->CompleteAttempt(r);
}

TEST_F(PortalDetectorTest, ReadCompleteHeader) {
  StartAttempt();

  EXPECT_CALL(callback_target(),
              ResultCallback(IsResult(
                  PortalDetector::Result(
                      ConnectivityTrial::Result(
                          ConnectivityTrial::kPhaseContent,
                          ConnectivityTrial::kStatusSuccess),
                      kNumAttempts,
                      true))));

  EXPECT_CALL(*connectivity_trial(), Stop()).Times(1);
  ConnectivityTrial::Result r =
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                ConnectivityTrial::kStatusSuccess);
  portal_detector()->CompleteAttempt(r);
}

TEST_F(PortalDetectorTest, ReadMatchingHeader) {
  StartAttempt();

  EXPECT_CALL(callback_target(),
              ResultCallback(IsResult(
                  PortalDetector::Result(
                      ConnectivityTrial::Result(
                          ConnectivityTrial::kPhaseContent,
                          ConnectivityTrial::kStatusSuccess),
                      kNumAttempts,
                      true))));
  EXPECT_CALL(*connectivity_trial(), Stop()).Times(1);
  ConnectivityTrial::Result r =
      ConnectivityTrial::Result(ConnectivityTrial::kPhaseContent,
                                ConnectivityTrial::kStatusSuccess);
  portal_detector()->CompleteAttempt(r);
}

}  // namespace shill
