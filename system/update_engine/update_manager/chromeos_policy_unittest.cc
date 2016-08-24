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

#include "update_engine/update_manager/chromeos_policy.h"

#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <base/time/time.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <gtest/gtest.h>

#include "update_engine/common/fake_clock.h"
#include "update_engine/update_manager/evaluation_context.h"
#include "update_engine/update_manager/fake_state.h"
#include "update_engine/update_manager/umtest_utils.h"

using base::Time;
using base::TimeDelta;
using chromeos_update_engine::ErrorCode;
using chromeos_update_engine::FakeClock;
using std::set;
using std::string;
using std::tuple;
using std::vector;

namespace chromeos_update_manager {

class UmChromeOSPolicyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_.SetAsCurrent();
    SetUpDefaultClock();
    eval_ctx_ = new EvaluationContext(&fake_clock_, TimeDelta::FromSeconds(5));
    SetUpDefaultState();
    SetUpDefaultDevicePolicy();
  }

  void TearDown() override {
    EXPECT_FALSE(loop_.PendingTasks());
  }

  // Sets the clock to fixed values.
  void SetUpDefaultClock() {
    fake_clock_.SetMonotonicTime(Time::FromInternalValue(12345678L));
    fake_clock_.SetWallclockTime(Time::FromInternalValue(12345678901234L));
  }

  void SetUpDefaultState() {
    fake_state_.updater_provider()->var_updater_started_time()->reset(
        new Time(fake_clock_.GetWallclockTime()));
    fake_state_.updater_provider()->var_last_checked_time()->reset(
        new Time(fake_clock_.GetWallclockTime()));
    fake_state_.updater_provider()->var_consecutive_failed_update_checks()->
        reset(new unsigned int{0});
    fake_state_.updater_provider()->var_server_dictated_poll_interval()->
        reset(new unsigned int{0});
    fake_state_.updater_provider()->var_forced_update_requested()->
        reset(new UpdateRequestStatus{UpdateRequestStatus::kNone});

    fake_state_.random_provider()->var_seed()->reset(
        new uint64_t(4));  // chosen by fair dice roll.
                           // guaranteed to be random.

    // No device policy loaded by default.
    fake_state_.device_policy_provider()->var_device_policy_is_loaded()->reset(
        new bool(false));

    // OOBE is enabled by default.
    fake_state_.config_provider()->var_is_oobe_enabled()->reset(
        new bool(true));

    // For the purpose of the tests, this is an official build and OOBE was
    // completed.
    fake_state_.system_provider()->var_is_official_build()->reset(
        new bool(true));
    fake_state_.system_provider()->var_is_oobe_complete()->reset(
        new bool(true));
    fake_state_.system_provider()->var_num_slots()->reset(new unsigned int(2));

    // Connection is wifi, untethered.
    fake_state_.shill_provider()->var_conn_type()->
        reset(new ConnectionType(ConnectionType::kWifi));
    fake_state_.shill_provider()->var_conn_tethering()->
        reset(new ConnectionTethering(ConnectionTethering::kNotDetected));
  }

  // Sets up a default device policy that does not impose any restrictions
  // (HTTP) nor enables any features (P2P).
  void SetUpDefaultDevicePolicy() {
    fake_state_.device_policy_provider()->var_device_policy_is_loaded()->reset(
        new bool(true));
    fake_state_.device_policy_provider()->var_update_disabled()->reset(
        new bool(false));
    fake_state_.device_policy_provider()->
        var_allowed_connection_types_for_update()->reset(nullptr);
    fake_state_.device_policy_provider()->var_scatter_factor()->reset(
        new TimeDelta());
    fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
        new bool(true));
    fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
        new bool(false));
    fake_state_.device_policy_provider()->var_release_channel_delegated()->
        reset(new bool(true));
  }

  // Configures the UpdateCheckAllowed policy to return a desired value by
  // faking the current wall clock time as needed. Restores the default state.
  // This is used when testing policies that depend on this one.
  void SetUpdateCheckAllowed(bool allow_check) {
    Time next_update_check;
    ExpectPolicyStatus(EvalStatus::kSucceeded,
                       &ChromeOSPolicy::NextUpdateCheckTime,
                       &next_update_check);
    SetUpDefaultState();
    SetUpDefaultDevicePolicy();
    Time curr_time = next_update_check;
    if (allow_check)
      curr_time += TimeDelta::FromSeconds(1);
    else
      curr_time -= TimeDelta::FromSeconds(1);
    fake_clock_.SetWallclockTime(curr_time);
  }

  // Returns a default UpdateState structure:
  UpdateState GetDefaultUpdateState(TimeDelta first_seen_period) {
    Time first_seen_time = fake_clock_.GetWallclockTime() - first_seen_period;
    UpdateState update_state = UpdateState();

    // This is a non-interactive check returning a delta payload, seen for the
    // first time (|first_seen_period| ago). Clearly, there were no failed
    // attempts so far.
    update_state.is_interactive = false;
    update_state.is_delta_payload = false;
    update_state.first_seen = first_seen_time;
    update_state.num_checks = 1;
    update_state.num_failures = 0;
    update_state.failures_last_updated = Time();  // Needs to be zero.
    // There's a single HTTP download URL with a maximum of 10 retries.
    update_state.download_urls = vector<string>{"http://fake/url/"};
    update_state.download_errors_max = 10;
    // Download was never attempted.
    update_state.last_download_url_idx = -1;
    update_state.last_download_url_num_errors = 0;
    // There were no download errors.
    update_state.download_errors = vector<tuple<int, ErrorCode, Time>>();
    // P2P is not disabled by Omaha.
    update_state.p2p_downloading_disabled = false;
    update_state.p2p_sharing_disabled = false;
    // P2P was not attempted.
    update_state.p2p_num_attempts = 0;
    update_state.p2p_first_attempted = Time();
    // No active backoff period, backoff is not disabled by Omaha.
    update_state.backoff_expiry = Time();
    update_state.is_backoff_disabled = false;
    // There is no active scattering wait period (max 7 days allowed) nor check
    // threshold (none allowed).
    update_state.scatter_wait_period = TimeDelta();
    update_state.scatter_check_threshold = 0;
    update_state.scatter_wait_period_max = TimeDelta::FromDays(7);
    update_state.scatter_check_threshold_min = 0;
    update_state.scatter_check_threshold_max = 0;

    return update_state;
  }

  // Runs the passed |policy_method| policy and expects it to return the
  // |expected| return value.
  template<typename T, typename R, typename... Args>
  void ExpectPolicyStatus(
      EvalStatus expected,
      T policy_method,
      R* result, Args... args) {
    string error = "<None>";
    eval_ctx_->ResetEvaluation();
    EXPECT_EQ(expected,
              (policy_.*policy_method)(eval_ctx_.get(), &fake_state_, &error,
                                       result, args...))
        << "Returned error: " << error
        << "\nEvaluation context: " << eval_ctx_->DumpContext();
  }

  brillo::FakeMessageLoop loop_{nullptr};
  FakeClock fake_clock_;
  FakeState fake_state_;
  scoped_refptr<EvaluationContext> eval_ctx_;
  ChromeOSPolicy policy_;  // ChromeOSPolicy under test.
};

TEST_F(UmChromeOSPolicyTest, FirstCheckIsAtMostInitialIntervalAfterStart) {
  Time next_update_check;

  // Set the last update time so it'll appear as if this is a first update check
  // in the lifetime of the current updater.
  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(fake_clock_.GetWallclockTime() - TimeDelta::FromMinutes(10)));

  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  EXPECT_LE(fake_clock_.GetWallclockTime(), next_update_check);
  EXPECT_GE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          ChromeOSPolicy::kTimeoutInitialInterval +
          ChromeOSPolicy::kTimeoutRegularFuzz / 2),
      next_update_check);
}

TEST_F(UmChromeOSPolicyTest, RecurringCheckBaseIntervalAndFuzz) {
  // Ensure that we're using the correct interval (kPeriodicInterval) and fuzz
  // (kTimeoutRegularFuzz) as base values for period updates.
  Time next_update_check;

  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  EXPECT_LE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          ChromeOSPolicy::kTimeoutPeriodicInterval -
          ChromeOSPolicy::kTimeoutRegularFuzz / 2),
      next_update_check);
  EXPECT_GE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          ChromeOSPolicy::kTimeoutPeriodicInterval +
          ChromeOSPolicy::kTimeoutRegularFuzz / 2),
      next_update_check);
}

TEST_F(UmChromeOSPolicyTest, RecurringCheckBackoffIntervalAndFuzz) {
  // Ensure that we're properly backing off and fuzzing in the presence of
  // failed updates attempts.
  Time next_update_check;

  fake_state_.updater_provider()->var_consecutive_failed_update_checks()->
      reset(new unsigned int{2});

  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  int expected_interval = ChromeOSPolicy::kTimeoutPeriodicInterval * 4;
  EXPECT_LE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          expected_interval - expected_interval / 2),
      next_update_check);
  EXPECT_GE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          expected_interval + expected_interval / 2),
      next_update_check);
}

TEST_F(UmChromeOSPolicyTest, RecurringCheckServerDictatedPollInterval) {
  // Policy honors the server provided check poll interval.
  Time next_update_check;

  const unsigned int kInterval = ChromeOSPolicy::kTimeoutPeriodicInterval * 4;
  fake_state_.updater_provider()->var_server_dictated_poll_interval()->
      reset(new unsigned int{kInterval});
  // We should not be backing off in this case.
  fake_state_.updater_provider()->var_consecutive_failed_update_checks()->
      reset(new unsigned int{2});

  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  EXPECT_LE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          kInterval - kInterval / 2),
      next_update_check);
  EXPECT_GE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          kInterval + kInterval / 2),
      next_update_check);
}

TEST_F(UmChromeOSPolicyTest, ExponentialBackoffIsCapped) {
  Time next_update_check;

  fake_state_.updater_provider()->var_consecutive_failed_update_checks()->
      reset(new unsigned int{100});

  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  EXPECT_LE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          ChromeOSPolicy::kTimeoutMaxBackoffInterval -
          ChromeOSPolicy::kTimeoutMaxBackoffInterval / 2),
      next_update_check);
  EXPECT_GE(
      fake_clock_.GetWallclockTime() + TimeDelta::FromSeconds(
          ChromeOSPolicy::kTimeoutMaxBackoffInterval +
          ChromeOSPolicy::kTimeoutMaxBackoffInterval /2),
      next_update_check);
}

TEST_F(UmChromeOSPolicyTest, UpdateCheckAllowedWaitsForTheTimeout) {
  // We get the next update_check timestamp from the policy's private method
  // and then we check the public method respects that value on the normal
  // case.
  Time next_update_check;
  Time last_checked_time =
      fake_clock_.GetWallclockTime() + TimeDelta::FromMinutes(1234);

  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  UpdateCheckParams result;

  // Check that the policy blocks until the next_update_check is reached.
  SetUpDefaultClock();
  SetUpDefaultState();
  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  fake_clock_.SetWallclockTime(next_update_check - TimeDelta::FromSeconds(1));
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateCheckAllowed, &result);

  SetUpDefaultClock();
  SetUpDefaultState();
  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  fake_clock_.SetWallclockTime(next_update_check + TimeDelta::FromSeconds(1));
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_TRUE(result.updates_enabled);
  EXPECT_FALSE(result.is_interactive);
}

TEST_F(UmChromeOSPolicyTest, UpdateCheckAllowedWaitsForOOBE) {
  // Update checks are deferred until OOBE is completed.

  // Ensure that update is not allowed even if wait period is satisfied.
  Time next_update_check;
  Time last_checked_time =
      fake_clock_.GetWallclockTime() + TimeDelta::FromMinutes(1234);

  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &ChromeOSPolicy::NextUpdateCheckTime, &next_update_check);

  SetUpDefaultClock();
  SetUpDefaultState();
  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  fake_clock_.SetWallclockTime(next_update_check + TimeDelta::FromSeconds(1));
  fake_state_.system_provider()->var_is_oobe_complete()->reset(
      new bool(false));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateCheckAllowed, &result);

  // Now check that it is allowed if OOBE is completed.
  SetUpDefaultClock();
  SetUpDefaultState();
  fake_state_.updater_provider()->var_last_checked_time()->reset(
      new Time(last_checked_time));
  fake_clock_.SetWallclockTime(next_update_check + TimeDelta::FromSeconds(1));
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_TRUE(result.updates_enabled);
  EXPECT_FALSE(result.is_interactive);
}

TEST_F(UmChromeOSPolicyTest, UpdateCheckAllowedWithAttributes) {
  // Update check is allowed, response includes attributes for use in the
  // request.
  SetUpdateCheckAllowed(true);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_target_version_prefix()->
      reset(new string("1.2"));
  fake_state_.device_policy_provider()->var_release_channel_delegated()->
      reset(new bool(false));
  fake_state_.device_policy_provider()->var_release_channel()->
      reset(new string("foo-channel"));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_TRUE(result.updates_enabled);
  EXPECT_EQ("1.2", result.target_version_prefix);
  EXPECT_EQ("foo-channel", result.target_channel);
  EXPECT_FALSE(result.is_interactive);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCheckAllowedUpdatesDisabledForUnofficialBuilds) {
  // UpdateCheckAllowed should return kAskMeAgainLater if this is an unofficial
  // build; we don't want periodic update checks on developer images.

  fake_state_.system_provider()->var_is_official_build()->reset(
      new bool(false));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateCheckAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCheckAllowedUpdatesDisabledForRemovableBootDevice) {
  // UpdateCheckAllowed should return false (kSucceeded) if the image booted
  // from a removable device.

  fake_state_.system_provider()->var_num_slots()->reset(new unsigned int(1));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_FALSE(result.updates_enabled);
}

TEST_F(UmChromeOSPolicyTest, UpdateCheckAllowedUpdatesDisabledByPolicy) {
  // UpdateCheckAllowed should return kAskMeAgainLater because a device policy
  // is loaded and prohibits updates.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_update_disabled()->reset(
      new bool(true));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateCheckAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCheckAllowedForcedUpdateRequestedInteractive) {
  // UpdateCheckAllowed should return true because a forced update request was
  // signaled for an interactive update.

  SetUpdateCheckAllowed(true);
  fake_state_.updater_provider()->var_forced_update_requested()->reset(
      new UpdateRequestStatus(UpdateRequestStatus::kInteractive));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_TRUE(result.updates_enabled);
  EXPECT_TRUE(result.is_interactive);
}

TEST_F(UmChromeOSPolicyTest, UpdateCheckAllowedForcedUpdateRequestedPeriodic) {
  // UpdateCheckAllowed should return true because a forced update request was
  // signaled for a periodic check.

  SetUpdateCheckAllowed(true);
  fake_state_.updater_provider()->var_forced_update_requested()->reset(
      new UpdateRequestStatus(UpdateRequestStatus::kPeriodic));

  UpdateCheckParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCheckAllowed, &result);
  EXPECT_TRUE(result.updates_enabled);
  EXPECT_FALSE(result.is_interactive);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartFailsCheckAllowedError) {
  // The UpdateCanStart policy fails, not being able to query
  // UpdateCheckAllowed.

  // Configure the UpdateCheckAllowed policy to fail.
  fake_state_.updater_provider()->var_updater_started_time()->reset(nullptr);

  // Check that the UpdateCanStart fails.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kFailed,
                     &Policy::UpdateCanStart, &result, update_state);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartNotAllowedCheckDue) {
  // The UpdateCanStart policy returns false because we are due for another
  // update check. Ensure that download related values are still returned.

  SetUpdateCheckAllowed(true);

  // Check that the UpdateCanStart returns false.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCanStart, &result, update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kCheckDue, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_EQ(0, result.download_url_num_errors);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedNoDevicePolicy) {
  // The UpdateCanStart policy returns true; no device policy is loaded.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_device_policy_is_loaded()->reset(
      new bool(false));

  // Check that the UpdateCanStart returns true with no further attributes.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCanStart, &result, update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_FALSE(result.p2p_downloading_allowed);
  EXPECT_FALSE(result.p2p_sharing_allowed);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedBlankPolicy) {
  // The UpdateCanStart policy returns true; device policy is loaded but imposes
  // no restrictions on updating.

  SetUpdateCheckAllowed(false);

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateCanStart, &result, update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_FALSE(result.p2p_downloading_allowed);
  EXPECT_FALSE(result.p2p_sharing_allowed);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedBackoffNewWaitPeriodApplies) {
  // The UpdateCanStart policy returns false; failures are reported and a new
  // backoff period is enacted.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kBackoff, result.cannot_start_reason);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_LT(curr_time, result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedBackoffPrevWaitPeriodStillApplies) {
  // The UpdateCanStart policy returns false; a previously enacted backoff
  // period still applies.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  update_state.failures_last_updated = curr_time;
  update_state.backoff_expiry = curr_time + TimeDelta::FromMinutes(3);

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater, &Policy::UpdateCanStart,
                     &result, update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kBackoff, result.cannot_start_reason);
  EXPECT_FALSE(result.do_increment_failures);
  EXPECT_LT(curr_time, result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedBackoffSatisfied) {
  // The UpdateCanStart policy returns true; a previously enacted backoff period
  // has elapsed, we're good to go.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  update_state.failures_last_updated = curr_time - TimeDelta::FromSeconds(1);
  update_state.backoff_expiry = curr_time - TimeDelta::FromSeconds(1);

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart,
                     &result, update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kUndefined, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
  EXPECT_EQ(Time(), result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedBackoffDisabled) {
  // The UpdateCanStart policy returns false; failures are reported but backoff
  // is disabled.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  update_state.is_backoff_disabled = true;

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kUndefined, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_EQ(Time(), result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedNoBackoffInteractive) {
  // The UpdateCanStart policy returns false; failures are reported but this is
  // an interactive update check.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  update_state.is_interactive = true;

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kUndefined, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_EQ(Time(), result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedNoBackoffDelta) {
  // The UpdateCanStart policy returns false; failures are reported but this is
  // a delta payload.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  update_state.is_delta_payload = true;

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kUndefined, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_EQ(Time(), result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedNoBackoffUnofficialBuild) {
  // The UpdateCanStart policy returns false; failures are reported but this is
  // an unofficial build.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));

  fake_state_.system_provider()->var_is_official_build()->
      reset(new bool(false));

  // Check that UpdateCanStart returns false and a new backoff expiry is
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kUndefined, result.cannot_start_reason);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_EQ(Time(), result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartFailsScatteringFailed) {
  // The UpdateCanStart policy fails because the UpdateScattering policy it
  // depends on fails (unset variable).

  SetUpdateCheckAllowed(false);

  // Override the default seed variable with a null value so that the policy
  // request would fail.
  // TODO(garnold) This failure may or may not fail a number
  // sub-policies/decisions, like scattering and backoff. We'll need a more
  // deliberate setup to ensure that we're failing what we want to be failing.
  fake_state_.random_provider()->var_seed()->reset(nullptr);

  // Check that the UpdateCanStart fails.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kFailed,
                     &Policy::UpdateCanStart, &result, update_state);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedScatteringNewWaitPeriodApplies) {
  // The UpdateCanStart policy returns false; device policy is loaded and
  // scattering applies due to an unsatisfied wait period, which was newly
  // generated.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromMinutes(2)));


  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));

  // Check that the UpdateCanStart returns false and a new wait period
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kScattering, result.cannot_start_reason);
  EXPECT_LT(TimeDelta(), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedScatteringPrevWaitPeriodStillApplies) {
  // The UpdateCanStart policy returns false w/ kAskMeAgainLater; device policy
  // is loaded and a previously generated scattering period still applies, none
  // of the scattering values has changed.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromMinutes(2)));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.scatter_wait_period = TimeDelta::FromSeconds(35);

  // Check that the UpdateCanStart returns false and a new wait period
  // generated.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater, &Policy::UpdateCanStart,
                     &result, update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kScattering, result.cannot_start_reason);
  EXPECT_EQ(TimeDelta::FromSeconds(35), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedScatteringNewCountThresholdApplies) {
  // The UpdateCanStart policy returns false; device policy is loaded and
  // scattering applies due to an unsatisfied update check count threshold.
  //
  // This ensures a non-zero check threshold, which may or may not be combined
  // with a non-zero wait period (for which we cannot reliably control).

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromSeconds(1)));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.scatter_check_threshold_min = 2;
  update_state.scatter_check_threshold_max = 5;

  // Check that the UpdateCanStart returns false.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kScattering, result.cannot_start_reason);
  EXPECT_LE(2, result.scatter_check_threshold);
  EXPECT_GE(5, result.scatter_check_threshold);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartNotAllowedScatteringPrevCountThresholdStillApplies) {
  // The UpdateCanStart policy returns false; device policy is loaded and
  // scattering due to a previously generated count threshold still applies.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromSeconds(1)));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.scatter_check_threshold = 3;
  update_state.scatter_check_threshold_min = 2;
  update_state.scatter_check_threshold_max = 5;

  // Check that the UpdateCanStart returns false.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kScattering, result.cannot_start_reason);
  EXPECT_EQ(3, result.scatter_check_threshold);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedScatteringSatisfied) {
  // The UpdateCanStart policy returns true; device policy is loaded and
  // scattering is enabled, but both wait period and check threshold are
  // satisfied.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromSeconds(120)));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(75));
  update_state.num_checks = 4;
  update_state.scatter_wait_period = TimeDelta::FromSeconds(60);
  update_state.scatter_check_threshold = 3;
  update_state.scatter_check_threshold_min = 2;
  update_state.scatter_check_threshold_max = 5;

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(TimeDelta(), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedInteractivePreventsScattering) {
  // The UpdateCanStart policy returns true; device policy is loaded and
  // scattering would have applied, except that the update check is interactive
  // and so it is suppressed.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromSeconds(1)));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.is_interactive = true;
  update_state.scatter_check_threshold = 0;
  update_state.scatter_check_threshold_min = 2;
  update_state.scatter_check_threshold_max = 5;

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(TimeDelta(), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedOobePreventsScattering) {
  // The UpdateCanStart policy returns true; device policy is loaded and
  // scattering would have applied, except that OOBE was not completed and so it
  // is suppressed.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromSeconds(1)));
  fake_state_.system_provider()->var_is_oobe_complete()->reset(new bool(false));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.is_interactive = true;
  update_state.scatter_check_threshold = 0;
  update_state.scatter_check_threshold_min = 2;
  update_state.scatter_check_threshold_max = 5;

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(TimeDelta(), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedWithAttributes) {
  // The UpdateCanStart policy returns true; device policy permits both HTTP and
  // P2P updates, as well as a non-empty target channel string.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedWithP2PFromUpdater) {
  // The UpdateCanStart policy returns true; device policy forbids both HTTP and
  // P2P updates, but the updater is configured to allow P2P and overrules the
  // setting.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.updater_provider()->var_p2p_enabled()->reset(new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedP2PDownloadingBlockedDueToOmaha) {
  // The UpdateCanStart policy returns true; device policy permits HTTP, but
  // policy blocks P2P downloading because Omaha forbids it.  P2P sharing is
  // still permitted.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.p2p_downloading_disabled = true;
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_FALSE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedP2PSharingBlockedDueToOmaha) {
  // The UpdateCanStart policy returns true; device policy permits HTTP, but
  // policy blocks P2P sharing because Omaha forbids it.  P2P downloading is
  // still permitted.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.p2p_sharing_disabled = true;
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_FALSE(result.p2p_sharing_allowed);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedP2PDownloadingBlockedDueToNumAttempts) {
  // The UpdateCanStart policy returns true; device policy permits HTTP but
  // blocks P2P download, because the max number of P2P downloads have been
  // attempted. P2P sharing is still permitted.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.p2p_num_attempts = ChromeOSPolicy::kMaxP2PAttempts;
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_FALSE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedP2PDownloadingBlockedDueToAttemptsPeriod) {
  // The UpdateCanStart policy returns true; device policy permits HTTP but
  // blocks P2P download, because the max period for attempt to download via P2P
  // has elapsed. P2P sharing is still permitted.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.p2p_num_attempts = 1;
  update_state.p2p_first_attempted =
      fake_clock_.GetWallclockTime() -
      TimeDelta::FromSeconds(
          ChromeOSPolicy::kMaxP2PAttemptsPeriodInSeconds + 1);
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_FALSE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedWithHttpUrlForUnofficialBuild) {
  // The UpdateCanStart policy returns true; device policy forbids both HTTP and
  // P2P updates, but marking this an unofficial build overrules the HTTP
  // setting.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(false));
  fake_state_.system_provider()->var_is_official_build()->
      reset(new bool(false));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedWithHttpsUrl) {
  // The UpdateCanStart policy returns true; device policy forbids both HTTP and
  // P2P updates, but an HTTPS URL is provided and selected for download.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(false));

  // Add an HTTPS URL.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.download_urls.emplace_back("https://secure/url/");

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(1, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedMaxErrorsNotExceeded) {
  // The UpdateCanStart policy returns true; the first URL has download errors
  // but does not exceed the maximum allowed number of failures, so it is stilli
  // usable.

  SetUpdateCheckAllowed(false);

  // Add a second URL; update with this URL attempted and failed enough times to
  // disqualify the current (first) URL.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.num_checks = 5;
  update_state.download_urls.emplace_back("http://another/fake/url/");
  Time t = fake_clock_.GetWallclockTime() - TimeDelta::FromSeconds(12);
  for (int i = 0; i < 5; i++) {
    update_state.download_errors.emplace_back(
        0, ErrorCode::kDownloadTransferError, t);
    t += TimeDelta::FromSeconds(1);
  }

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(5, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedWithSecondUrlMaxExceeded) {
  // The UpdateCanStart policy returns true; the first URL exceeded the maximum
  // allowed number of failures, but a second URL is available.

  SetUpdateCheckAllowed(false);

  // Add a second URL; update with this URL attempted and failed enough times to
  // disqualify the current (first) URL.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.num_checks = 10;
  update_state.download_urls.emplace_back("http://another/fake/url/");
  Time t = fake_clock_.GetWallclockTime() - TimeDelta::FromSeconds(12);
  for (int i = 0; i < 11; i++) {
    update_state.download_errors.emplace_back(
        0, ErrorCode::kDownloadTransferError, t);
    t += TimeDelta::FromSeconds(1);
  }

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(1, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedWithSecondUrlHardError) {
  // The UpdateCanStart policy returns true; the first URL fails with a hard
  // error, but a second URL is available.

  SetUpdateCheckAllowed(false);

  // Add a second URL; update with this URL attempted and failed in a way that
  // causes it to switch directly to the next URL.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.num_checks = 10;
  update_state.download_urls.emplace_back("http://another/fake/url/");
  update_state.download_errors.emplace_back(
      0, ErrorCode::kPayloadHashMismatchError,
      fake_clock_.GetWallclockTime() - TimeDelta::FromSeconds(1));

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(1, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedUrlWrapsAround) {
  // The UpdateCanStart policy returns true; URL search properly wraps around
  // the last one on the list.

  SetUpdateCheckAllowed(false);

  // Add a second URL; update with this URL attempted and failed in a way that
  // causes it to switch directly to the next URL. We must disable backoff in
  // order for it not to interfere.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  update_state.num_checks = 1;
  update_state.is_backoff_disabled = true;
  update_state.download_urls.emplace_back("http://another/fake/url/");
  update_state.download_errors.emplace_back(
      1, ErrorCode::kPayloadHashMismatchError,
      fake_clock_.GetWallclockTime() - TimeDelta::FromSeconds(1));

  // Check that the UpdateCanStart returns true.
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartNotAllowedNoUsableUrls) {
  // The UpdateCanStart policy returns false; there's a single HTTP URL but its
  // use is forbidden by policy.
  //
  // Note: In the case where no usable URLs are found, the policy should not
  // increment the number of failed attempts! Doing so would result in a
  // non-idempotent semantics, and does not fall within the intended purpose of
  // the backoff mechanism anyway.

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(false));

  // Check that the UpdateCanStart returns false.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_FALSE(result.update_can_start);
  EXPECT_EQ(UpdateCannotStartReason::kCannotDownload,
            result.cannot_start_reason);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateCanStartAllowedNoUsableUrlsButP2PEnabled) {
  // The UpdateCanStart policy returns true; there's a single HTTP URL but its
  // use is forbidden by policy, however P2P is enabled. The result indicates
  // that no URL can be used.
  //
  // Note: The number of failed attempts should not increase in this case (see
  // above test).

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(false));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_GT(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedNoUsableUrlsButEnterpriseEnrolled) {
  // The UpdateCanStart policy returns true; there's a single HTTP URL but its
  // use is forbidden by policy, and P2P is unset on the policy, however the
  // device is enterprise-enrolled so P2P is allowed. The result indicates that
  // no URL can be used.
  //
  // Note: The number of failed attempts should not increase in this case (see
  // above test).

  SetUpdateCheckAllowed(false);

  // Override specific device policy attributes.
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(nullptr);
  fake_state_.device_policy_provider()->var_owner()->reset(nullptr);
  fake_state_.device_policy_provider()->var_http_downloads_enabled()->reset(
      new bool(false));

  // Check that the UpdateCanStart returns true.
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromMinutes(10));
  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_GT(0, result.download_url_idx);
  EXPECT_TRUE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_FALSE(result.do_increment_failures);
}

TEST_F(UmChromeOSPolicyTest, UpdateDownloadAllowedEthernetDefault) {
  // Ethernet is always allowed.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kEthernet));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest, UpdateDownloadAllowedWifiDefault) {
  // Wifi is allowed if not tethered.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kWifi));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCurrentConnectionNotAllowedWifiTetheredDefault) {
  // Tethered wifi is not allowed by default.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kWifi));
  fake_state_.shill_provider()->var_conn_tethering()->
      reset(new ConnectionTethering(ConnectionTethering::kConfirmed));

  bool result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateDownloadAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateDownloadAllowedWifiTetheredPolicyOverride) {
  // Tethered wifi can be allowed by policy.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kWifi));
  fake_state_.shill_provider()->var_conn_tethering()->
      reset(new ConnectionTethering(ConnectionTethering::kConfirmed));
  set<ConnectionType> allowed_connections;
  allowed_connections.insert(ConnectionType::kCellular);
  fake_state_.device_policy_provider()->
      var_allowed_connection_types_for_update()->
      reset(new set<ConnectionType>(allowed_connections));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest, UpdateDownloadAllowedWimaxDefault) {
  // Wimax is always allowed.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kWifi));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCurrentConnectionNotAllowedBluetoothDefault) {
  // Bluetooth is never allowed.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kBluetooth));

  bool result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateDownloadAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCurrentConnectionNotAllowedBluetoothPolicyCannotOverride) {
  // Bluetooth cannot be allowed even by policy.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kBluetooth));
  set<ConnectionType> allowed_connections;
  allowed_connections.insert(ConnectionType::kBluetooth);
  fake_state_.device_policy_provider()->
      var_allowed_connection_types_for_update()->
      reset(new set<ConnectionType>(allowed_connections));

  bool result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateDownloadAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest, UpdateCurrentConnectionNotAllowedCellularDefault) {
  // Cellular is not allowed by default.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kCellular));

  bool result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater,
                     &Policy::UpdateDownloadAllowed, &result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateDownloadAllowedCellularPolicyOverride) {
  // Update over cellular can be enabled by policy.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kCellular));
  set<ConnectionType> allowed_connections;
  allowed_connections.insert(ConnectionType::kCellular);
  fake_state_.device_policy_provider()->
      var_allowed_connection_types_for_update()->
      reset(new set<ConnectionType>(allowed_connections));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateDownloadAllowedCellularUserOverride) {
  // Update over cellular can be enabled by user settings, but only if policy
  // is present and does not determine allowed connections.

  fake_state_.shill_provider()->var_conn_type()->
      reset(new ConnectionType(ConnectionType::kCellular));
  set<ConnectionType> allowed_connections;
  allowed_connections.insert(ConnectionType::kCellular);
  fake_state_.updater_provider()->var_cellular_enabled()->
      reset(new bool(true));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded,
                     &Policy::UpdateDownloadAllowed, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedScatteringSupressedDueToP2P) {
  // The UpdateCanStart policy returns true; scattering should have applied, but
  // P2P download is allowed. Scattering values are nonetheless returned, and so
  // are download URL values, albeit the latter are not allowed to be used.

  SetUpdateCheckAllowed(false);
  fake_state_.device_policy_provider()->var_scatter_factor()->reset(
      new TimeDelta(TimeDelta::FromMinutes(2)));
  fake_state_.updater_provider()->var_p2p_enabled()->reset(new bool(true));

  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(1));
  update_state.scatter_wait_period = TimeDelta::FromSeconds(35);

  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart,
                     &result, update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_FALSE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_FALSE(result.do_increment_failures);
  EXPECT_EQ(TimeDelta::FromSeconds(35), result.scatter_wait_period);
  EXPECT_EQ(0, result.scatter_check_threshold);
}

TEST_F(UmChromeOSPolicyTest,
       UpdateCanStartAllowedBackoffSupressedDueToP2P) {
  // The UpdateCanStart policy returns true; backoff should have applied, but
  // P2P download is allowed. Backoff values are nonetheless returned, and so
  // are download URL values, albeit the latter are not allowed to be used.

  SetUpdateCheckAllowed(false);

  const Time curr_time = fake_clock_.GetWallclockTime();
  UpdateState update_state = GetDefaultUpdateState(TimeDelta::FromSeconds(10));
  update_state.download_errors_max = 1;
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(8));
  update_state.download_errors.emplace_back(
      0, ErrorCode::kDownloadTransferError,
      curr_time - TimeDelta::FromSeconds(2));
  fake_state_.updater_provider()->var_p2p_enabled()->reset(new bool(true));

  UpdateDownloadParams result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::UpdateCanStart, &result,
                     update_state);
  EXPECT_TRUE(result.update_can_start);
  EXPECT_EQ(0, result.download_url_idx);
  EXPECT_FALSE(result.download_url_allowed);
  EXPECT_EQ(0, result.download_url_num_errors);
  EXPECT_TRUE(result.p2p_downloading_allowed);
  EXPECT_TRUE(result.p2p_sharing_allowed);
  EXPECT_TRUE(result.do_increment_failures);
  EXPECT_LT(curr_time, result.backoff_expiry);
}

TEST_F(UmChromeOSPolicyTest, P2PEnabledNotAllowed) {
  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::P2PEnabled, &result);
  EXPECT_FALSE(result);
}

TEST_F(UmChromeOSPolicyTest, P2PEnabledAllowedByDevicePolicy) {
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(
      new bool(true));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::P2PEnabled, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest, P2PEnabledAllowedByUpdater) {
  fake_state_.updater_provider()->var_p2p_enabled()->reset(new bool(true));

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::P2PEnabled, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest, P2PEnabledAllowedDeviceEnterpriseEnrolled) {
  fake_state_.device_policy_provider()->var_au_p2p_enabled()->reset(nullptr);
  fake_state_.device_policy_provider()->var_owner()->reset(nullptr);

  bool result;
  ExpectPolicyStatus(EvalStatus::kSucceeded, &Policy::P2PEnabled, &result);
  EXPECT_TRUE(result);
}

TEST_F(UmChromeOSPolicyTest, P2PEnabledChangedBlocks) {
  bool result;
  ExpectPolicyStatus(EvalStatus::kAskMeAgainLater, &Policy::P2PEnabledChanged,
                     &result, false);
}

}  // namespace chromeos_update_manager
