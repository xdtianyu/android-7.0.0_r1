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

#ifndef UPDATE_ENGINE_UPDATE_MANAGER_CHROMEOS_POLICY_H_
#define UPDATE_ENGINE_UPDATE_MANAGER_CHROMEOS_POLICY_H_

#include <string>

#include <base/time/time.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "update_engine/update_manager/policy.h"
#include "update_engine/update_manager/prng.h"

namespace chromeos_update_manager {

// Output information from UpdateBackoffAndDownloadUrl.
struct UpdateBackoffAndDownloadUrlResult {
  // Whether the failed attempt count (maintained by the caller) needs to be
  // incremented.
  bool do_increment_failures;
  // The current backoff expiry. Null if backoff is not in effect.
  base::Time backoff_expiry;
  // The new URL index to use and number of download errors associated with it.
  // Significant iff |do_increment_failures| is false and |backoff_expiry| is
  // null. Negative value means no usable URL was found.
  int url_idx;
  int url_num_errors;
};

// Parameters for update scattering, as returned by UpdateScattering.
struct UpdateScatteringResult {
  bool is_scattering;
  base::TimeDelta wait_period;
  int check_threshold;
};

// ChromeOSPolicy implements the policy-related logic used in ChromeOS.
class ChromeOSPolicy : public Policy {
 public:
  ChromeOSPolicy() {}
  ~ChromeOSPolicy() override {}

  // Policy overrides.
  EvalStatus UpdateCheckAllowed(
      EvaluationContext* ec, State* state, std::string* error,
      UpdateCheckParams* result) const override;

  EvalStatus UpdateCanStart(
      EvaluationContext* ec,
      State* state,
      std::string* error,
      UpdateDownloadParams* result,
      UpdateState update_state) const override;

  EvalStatus UpdateDownloadAllowed(
      EvaluationContext* ec,
      State* state,
      std::string* error,
      bool* result) const override;

  EvalStatus P2PEnabled(
      EvaluationContext* ec,
      State* state,
      std::string* error,
      bool* result) const override;

  EvalStatus P2PEnabledChanged(
      EvaluationContext* ec,
      State* state,
      std::string* error,
      bool* result,
      bool prev_result) const override;

 protected:
  // Policy override.
  std::string PolicyName() const override { return "ChromeOSPolicy"; }

 private:
  friend class UmChromeOSPolicyTest;
  FRIEND_TEST(UmChromeOSPolicyTest,
              FirstCheckIsAtMostInitialIntervalAfterStart);
  FRIEND_TEST(UmChromeOSPolicyTest, RecurringCheckBaseIntervalAndFuzz);
  FRIEND_TEST(UmChromeOSPolicyTest, RecurringCheckBackoffIntervalAndFuzz);
  FRIEND_TEST(UmChromeOSPolicyTest, RecurringCheckServerDictatedPollInterval);
  FRIEND_TEST(UmChromeOSPolicyTest, ExponentialBackoffIsCapped);
  FRIEND_TEST(UmChromeOSPolicyTest, UpdateCheckAllowedWaitsForTheTimeout);
  FRIEND_TEST(UmChromeOSPolicyTest, UpdateCheckAllowedWaitsForOOBE);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartNotAllowedScatteringNewWaitPeriodApplies);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartNotAllowedScatteringPrevWaitPeriodStillApplies);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartNotAllowedScatteringNewCountThresholdApplies);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartNotAllowedScatteringPrevCountThresholdStillApplies);
  FRIEND_TEST(UmChromeOSPolicyTest, UpdateCanStartAllowedScatteringSatisfied);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartAllowedInteractivePreventsScattering);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartAllowedP2PDownloadingBlockedDueToNumAttempts);
  FRIEND_TEST(UmChromeOSPolicyTest,
              UpdateCanStartAllowedP2PDownloadingBlockedDueToAttemptsPeriod);

  // Auxiliary constant (zero by default).
  const base::TimeDelta kZeroInterval;

  // Default update check timeout interval/fuzz values used to compute the
  // NextUpdateCheckTime(), in seconds. Actual fuzz is within +/- half of the
  // indicated value.
  static const int kTimeoutInitialInterval;
  static const int kTimeoutPeriodicInterval;
  static const int kTimeoutMaxBackoffInterval;
  static const int kTimeoutRegularFuzz;

  // Maximum update attempt backoff interval and fuzz.
  static const int kAttemptBackoffMaxIntervalInDays;
  static const int kAttemptBackoffFuzzInHours;

  // Maximum number of times we'll allow using P2P for the same update payload.
  static const int kMaxP2PAttempts;
  // Maximum period of time allowed for download a payload via P2P, in seconds.
  static const int kMaxP2PAttemptsPeriodInSeconds;

  // A private policy implementation returning the wallclock timestamp when
  // the next update check should happen.
  // TODO(garnold) We should probably change that to infer a monotonic
  // timestamp, which will make the update check intervals more resilient to
  // clock skews. Might require switching some of the variables exported by the
  // UpdaterProvider to report monotonic time, as well.
  EvalStatus NextUpdateCheckTime(EvaluationContext* ec, State* state,
                                 std::string* error,
                                 base::Time* next_update_check) const;

  // Returns a TimeDelta based on the provided |interval| seconds +/- half
  // |fuzz| seconds. The return value is guaranteed to be a non-negative
  // TimeDelta.
  static base::TimeDelta FuzzedInterval(PRNG* prng, int interval, int fuzz);

  // A private policy for determining backoff and the download URL to use.
  // Within |update_state|, |backoff_expiry| and |is_backoff_disabled| are used
  // for determining whether backoff is still in effect; if not,
  // |download_errors| is scanned past |failures_last_updated|, and a new
  // download URL from |download_urls| is found and written to |result->url_idx|
  // (-1 means no usable URL exists); |download_errors_max| determines the
  // maximum number of attempts per URL, according to the Omaha response. If an
  // update failure is identified then |result->do_increment_failures| is set to
  // true; if backoff is enabled, a new backoff period is computed (from the
  // time of failure) based on |num_failures|. Otherwise, backoff expiry is
  // nullified, indicating that no backoff is in effect.
  //
  // If backing off but the previous backoff expiry is unchanged, returns
  // |EvalStatus::kAskMeAgainLater|. Otherwise:
  //
  // * If backing off with a new expiry time, then |result->backoff_expiry| is
  //   set to this time.
  //
  // * Else, |result->backoff_expiry| is set to null, indicating that no backoff
  //   is in effect.
  //
  // In any of these cases, returns |EvalStatus::kSucceeded|. If an error
  // occurred, returns |EvalStatus::kFailed|.
  EvalStatus UpdateBackoffAndDownloadUrl(
      EvaluationContext* ec, State* state, std::string* error,
      UpdateBackoffAndDownloadUrlResult* result,
      const UpdateState& update_state) const;

  // A private policy for checking whether scattering is due. Writes in |result|
  // the decision as to whether or not to scatter; a wallclock-based scatter
  // wait period, which ranges from zero (do not wait) and no greater than the
  // current scatter factor provided by the device policy (if available) or the
  // maximum wait period determined by Omaha; and an update check-based
  // threshold between zero (no threshold) and the maximum number determined by
  // the update engine. Within |update_state|, |scatter_wait_period| should
  // contain the last scattering period returned by this function, or zero if no
  // wait period is known; |scatter_check_threshold| is the last update check
  // threshold, or zero if no such threshold is known. If not scattering, or if
  // any of the scattering values has changed, returns |EvalStatus::kSucceeded|;
  // otherwise, |EvalStatus::kAskMeAgainLater|.
  EvalStatus UpdateScattering(EvaluationContext* ec, State* state,
                              std::string* error,
                              UpdateScatteringResult* result,
                              const UpdateState& update_state) const;

  DISALLOW_COPY_AND_ASSIGN(ChromeOSPolicy);
};

}  // namespace chromeos_update_manager

#endif  // UPDATE_ENGINE_UPDATE_MANAGER_CHROMEOS_POLICY_H_
