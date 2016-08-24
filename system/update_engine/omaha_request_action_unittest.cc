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

#include "update_engine/omaha_request_action.h"

#include <stdint.h>

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <base/time/time.h>
#include <brillo/bind_lambda.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>
#include <brillo/message_loops/message_loop_utils.h>
#include <gtest/gtest.h>

#include "update_engine/common/action_pipe.h"
#include "update_engine/common/constants.h"
#include "update_engine/common/fake_prefs.h"
#include "update_engine/common/hash_calculator.h"
#include "update_engine/common/mock_http_fetcher.h"
#include "update_engine/common/platform_constants.h"
#include "update_engine/common/prefs.h"
#include "update_engine/common/test_utils.h"
#include "update_engine/common/utils.h"
#include "update_engine/fake_system_state.h"
#include "update_engine/metrics.h"
#include "update_engine/mock_connection_manager.h"
#include "update_engine/mock_payload_state.h"
#include "update_engine/omaha_request_params.h"

using base::Time;
using base::TimeDelta;
using chromeos_update_engine::test_utils::System;
using chromeos_update_engine::test_utils::WriteFileString;
using std::string;
using std::vector;
using testing::AllOf;
using testing::AnyNumber;
using testing::DoAll;
using testing::Ge;
using testing::Le;
using testing::NiceMock;
using testing::Return;
using testing::ReturnPointee;
using testing::SaveArg;
using testing::SetArgumentPointee;
using testing::_;

namespace {

const char kTestAppId[] = "test-app-id";

// This is a helper struct to allow unit tests build an update response with the
// values they care about.
struct FakeUpdateResponse {
  string GetNoUpdateResponse() const {
    string entity_str;
    if (include_entity)
      entity_str = "<!DOCTYPE response [<!ENTITY CrOS \"ChromeOS\">]>";
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" +
        entity_str + "<response protocol=\"3.0\">"
        "<daystart elapsed_seconds=\"100\"/>"
        "<app appid=\"" + app_id + "\" " +
        (include_cohorts ? "cohort=\"" + cohort + "\" cohorthint=\"" +
         cohorthint + "\" cohortname=\"" + cohortname + "\" " : "") +
        " status=\"ok\">"
        "<ping status=\"ok\"/>"
        "<updatecheck status=\"noupdate\"/></app></response>";
  }

  string GetUpdateResponse() const {
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response "
        "protocol=\"3.0\">"
        "<daystart elapsed_seconds=\"100\"" +
        (elapsed_days.empty() ? "" : (" elapsed_days=\"" + elapsed_days + "\""))
        + "/>"
        "<app appid=\"" + app_id + "\" " +
        (include_cohorts ? "cohort=\"" + cohort + "\" cohorthint=\"" +
         cohorthint + "\" cohortname=\"" + cohortname + "\" " : "") +
        " status=\"ok\">"
        "<ping status=\"ok\"/><updatecheck status=\"ok\">"
        "<urls><url codebase=\"" + codebase + "\"/></urls>"
        "<manifest version=\"" + version + "\">"
        "<packages><package hash=\"not-used\" name=\"" + filename +  "\" "
        "size=\"" + base::Int64ToString(size) + "\"/></packages>"
        "<actions><action event=\"postinstall\" "
        "ChromeOSVersion=\"" + version + "\" "
        "MoreInfo=\"" + more_info_url + "\" Prompt=\"" + prompt + "\" "
        "IsDelta=\"true\" "
        "IsDeltaPayload=\"true\" "
        "MaxDaysToScatter=\"" + max_days_to_scatter + "\" "
        "sha256=\"" + hash + "\" "
        "needsadmin=\"" + needsadmin + "\" " +
        (deadline.empty() ? "" : ("deadline=\"" + deadline + "\" ")) +
        (disable_p2p_for_downloading ?
            "DisableP2PForDownloading=\"true\" " : "") +
        (disable_p2p_for_sharing ? "DisableP2PForSharing=\"true\" " : "") +
        "/></actions></manifest></updatecheck></app></response>";
  }

  // Return the payload URL, which is split in two fields in the XML response.
  string GetPayloadUrl() {
    return codebase + filename;
  }

  string app_id = kTestAppId;
  string version = "1.2.3.4";
  string more_info_url = "http://more/info";
  string prompt = "true";
  string codebase = "http://code/base/";
  string filename = "file.signed";
  string hash = "HASH1234=";
  string needsadmin = "false";
  int64_t size = 123;
  string deadline = "";
  string max_days_to_scatter = "7";
  string elapsed_days = "42";

  // P2P setting defaults to allowed.
  bool disable_p2p_for_downloading = false;
  bool disable_p2p_for_sharing = false;

  // Omaha cohorts settings.
  bool include_cohorts = false;
  string cohort = "";
  string cohorthint = "";
  string cohortname = "";

  // Whether to include the CrOS <!ENTITY> in the XML response.
  bool include_entity = false;
};

}  // namespace

namespace chromeos_update_engine {

class OmahaRequestActionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fake_system_state_.set_request_params(&request_params_);
    fake_system_state_.set_prefs(&fake_prefs_);
  }

  // Returns true iff an output response was obtained from the
  // OmahaRequestAction. |prefs| may be null, in which case a local MockPrefs
  // is used. |payload_state| may be null, in which case a local mock is used.
  // |p2p_manager| may be null, in which case a local mock is used.
  // |connection_manager| may be null, in which case a local mock is used.
  // out_response may be null. If |fail_http_response_code| is non-negative,
  // the transfer will fail with that code. |ping_only| is passed through to the
  // OmahaRequestAction constructor. out_post_data may be null; if non-null, the
  // post-data received by the mock HttpFetcher is returned.
  //
  // The |expected_check_result|, |expected_check_reaction| and
  // |expected_error_code| parameters are for checking expectations
  // about reporting UpdateEngine.Check.{Result,Reaction,DownloadError}
  // UMA statistics. Use the appropriate ::kUnset value to specify that
  // the given metric should not be reported.
  bool TestUpdateCheck(OmahaRequestParams* request_params,
                       const string& http_response,
                       int fail_http_response_code,
                       bool ping_only,
                       ErrorCode expected_code,
                       metrics::CheckResult expected_check_result,
                       metrics::CheckReaction expected_check_reaction,
                       metrics::DownloadErrorCode expected_download_error_code,
                       OmahaResponse* out_response,
                       brillo::Blob* out_post_data);

  // Runs and checks a ping test. |ping_only| indicates whether it should send
  // only a ping or also an updatecheck.
  void PingTest(bool ping_only);

  // InstallDate test helper function.
  bool InstallDateParseHelper(const string &elapsed_days,
                              OmahaResponse *response);

  // P2P test helper function.
  void P2PTest(
      bool initial_allow_p2p_for_downloading,
      bool initial_allow_p2p_for_sharing,
      bool omaha_disable_p2p_for_downloading,
      bool omaha_disable_p2p_for_sharing,
      bool payload_state_allow_p2p_attempt,
      bool expect_p2p_client_lookup,
      const string& p2p_client_result_url,
      bool expected_allow_p2p_for_downloading,
      bool expected_allow_p2p_for_sharing,
      const string& expected_p2p_url);

  FakeSystemState fake_system_state_;
  FakeUpdateResponse fake_update_response_;

  // By default, all tests use these objects unless they replace them in the
  // fake_system_state_.
  OmahaRequestParams request_params_ = OmahaRequestParams{
      &fake_system_state_,
      constants::kOmahaPlatformName,
      OmahaRequestParams::kOsVersion,
      "service_pack",
      "x86-generic",
      kTestAppId,
      "0.1.0.0",
      "en-US",
      "unittest",
      "OEM MODEL 09235 7471",
      "ChromeOSFirmware.1.0",
      "0X0A1",
      false,   // delta okay
      false,   // interactive
      "http://url",
      ""};     // target_version_prefix

  FakePrefs fake_prefs_;
};

namespace {
class OmahaRequestActionTestProcessorDelegate : public ActionProcessorDelegate {
 public:
  OmahaRequestActionTestProcessorDelegate()
      : expected_code_(ErrorCode::kSuccess) {}
  ~OmahaRequestActionTestProcessorDelegate() override {
  }
  void ProcessingDone(const ActionProcessor* processor,
                      ErrorCode code) override {
    brillo::MessageLoop::current()->BreakLoop();
  }

  void ActionCompleted(ActionProcessor* processor,
                       AbstractAction* action,
                       ErrorCode code) override {
    // make sure actions always succeed
    if (action->Type() == OmahaRequestAction::StaticType())
      EXPECT_EQ(expected_code_, code);
    else
      EXPECT_EQ(ErrorCode::kSuccess, code);
  }
  ErrorCode expected_code_;
};
}  // namespace

class OutputObjectCollectorAction;

template<>
class ActionTraits<OutputObjectCollectorAction> {
 public:
  // Does not take an object for input
  typedef OmahaResponse InputObjectType;
  // On success, puts the output path on output
  typedef NoneType OutputObjectType;
};

class OutputObjectCollectorAction : public Action<OutputObjectCollectorAction> {
 public:
  OutputObjectCollectorAction() : has_input_object_(false) {}
  void PerformAction() {
    // copy input object
    has_input_object_ = HasInputObject();
    if (has_input_object_)
      omaha_response_ = GetInputObject();
    processor_->ActionComplete(this, ErrorCode::kSuccess);
  }
  // Should never be called
  void TerminateProcessing() {
    CHECK(false);
  }
  // Debugging/logging
  static string StaticType() {
    return "OutputObjectCollectorAction";
  }
  string Type() const { return StaticType(); }
  using InputObjectType =
      ActionTraits<OutputObjectCollectorAction>::InputObjectType;
  using OutputObjectType =
      ActionTraits<OutputObjectCollectorAction>::OutputObjectType;
  bool has_input_object_;
  OmahaResponse omaha_response_;
};

bool OmahaRequestActionTest::TestUpdateCheck(
    OmahaRequestParams* request_params,
    const string& http_response,
    int fail_http_response_code,
    bool ping_only,
    ErrorCode expected_code,
    metrics::CheckResult expected_check_result,
    metrics::CheckReaction expected_check_reaction,
    metrics::DownloadErrorCode expected_download_error_code,
    OmahaResponse* out_response,
    brillo::Blob* out_post_data) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();
  MockHttpFetcher* fetcher = new MockHttpFetcher(http_response.data(),
                                                 http_response.size(),
                                                 nullptr);
  if (fail_http_response_code >= 0) {
    fetcher->FailTransfer(fail_http_response_code);
  }
  if (request_params)
    fake_system_state_.set_request_params(request_params);
  OmahaRequestAction action(&fake_system_state_,
                            nullptr,
                            brillo::make_unique_ptr(fetcher),
                            ping_only);
  OmahaRequestActionTestProcessorDelegate delegate;
  delegate.expected_code_ = expected_code;

  ActionProcessor processor;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&action);

  OutputObjectCollectorAction collector_action;
  BondActions(&action, &collector_action);
  processor.EnqueueAction(&collector_action);

  EXPECT_CALL(*fake_system_state_.mock_metrics_lib(), SendEnumToUMA(_, _, _))
      .Times(AnyNumber());
  EXPECT_CALL(*fake_system_state_.mock_metrics_lib(),
      SendEnumToUMA(metrics::kMetricCheckResult,
          static_cast<int>(expected_check_result),
          static_cast<int>(metrics::CheckResult::kNumConstants) - 1))
      .Times(expected_check_result == metrics::CheckResult::kUnset ? 0 : 1);
  EXPECT_CALL(*fake_system_state_.mock_metrics_lib(),
      SendEnumToUMA(metrics::kMetricCheckReaction,
          static_cast<int>(expected_check_reaction),
          static_cast<int>(metrics::CheckReaction::kNumConstants) - 1))
      .Times(expected_check_reaction == metrics::CheckReaction::kUnset ? 0 : 1);
  EXPECT_CALL(*fake_system_state_.mock_metrics_lib(),
      SendSparseToUMA(metrics::kMetricCheckDownloadErrorCode,
          static_cast<int>(expected_download_error_code)))
      .Times(expected_download_error_code == metrics::DownloadErrorCode::kUnset
             ? 0 : 1);

  loop.PostTask(base::Bind([&processor] { processor.StartProcessing(); }));
  LOG(INFO) << "loop.PendingTasks() = " << loop.PendingTasks();
  loop.Run();
  LOG(INFO) << "loop.PendingTasks() = " << loop.PendingTasks();
  EXPECT_FALSE(loop.PendingTasks());
  if (collector_action.has_input_object_ && out_response)
    *out_response = collector_action.omaha_response_;
  if (out_post_data)
    *out_post_data = fetcher->post_data();
  return collector_action.has_input_object_;
}

// Tests Event requests -- they should always succeed. |out_post_data|
// may be null; if non-null, the post-data received by the mock
// HttpFetcher is returned.
void TestEvent(OmahaRequestParams params,
               OmahaEvent* event,
               const string& http_response,
               brillo::Blob* out_post_data) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();
  MockHttpFetcher* fetcher = new MockHttpFetcher(http_response.data(),
                                                 http_response.size(),
                                                 nullptr);
  FakeSystemState fake_system_state;
  fake_system_state.set_request_params(&params);
  OmahaRequestAction action(&fake_system_state,
                            event,
                            brillo::make_unique_ptr(fetcher),
                            false);
  OmahaRequestActionTestProcessorDelegate delegate;
  ActionProcessor processor;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&action);

  loop.PostTask(base::Bind([&processor] { processor.StartProcessing(); }));
  loop.Run();

  // This test should schedule a callback to notify the crash reporter if
  // the passed event is an error.
  EXPECT_EQ(event->result == OmahaEvent::kResultError, loop.PendingTasks());

  if (out_post_data)
    *out_post_data = fetcher->post_data();
}

TEST_F(OmahaRequestActionTest, RejectEntities) {
  OmahaResponse response;
  fake_update_response_.include_entity = true;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaRequestXMLHasEntityDecl,
                      metrics::CheckResult::kParsingError,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, NoUpdateTest) {
  OmahaResponse response;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

// Test that all the values in the response are parsed in a normal update
// response.
TEST_F(OmahaRequestActionTest, ValidUpdateTest) {
  OmahaResponse response;
  fake_update_response_.deadline = "20101020";
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ(fake_update_response_.version, response.version);
  EXPECT_EQ(fake_update_response_.GetPayloadUrl(), response.payload_urls[0]);
  EXPECT_EQ(fake_update_response_.more_info_url, response.more_info_url);
  EXPECT_EQ(fake_update_response_.hash, response.hash);
  EXPECT_EQ(fake_update_response_.size, response.size);
  EXPECT_EQ(fake_update_response_.prompt == "true", response.prompt);
  EXPECT_EQ(fake_update_response_.deadline, response.deadline);
  // Omaha cohort attribets are not set in the response, so they should not be
  // persisted.
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsOmahaCohort));
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsOmahaCohortHint));
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsOmahaCohortName));
}

TEST_F(OmahaRequestActionTest, ValidUpdateBlockedByConnection) {
  OmahaResponse response;
  // Set up a connection manager that doesn't allow a valid update over
  // the current ethernet connection.
  MockConnectionManager mock_cm;
  fake_system_state_.set_connection_manager(&mock_cm);

  EXPECT_CALL(mock_cm, GetConnectionProperties(_, _))
      .WillRepeatedly(
          DoAll(SetArgumentPointee<0>(NetworkConnectionType::kEthernet),
                SetArgumentPointee<1>(NetworkTethering::kUnknown),
                Return(true)));
  EXPECT_CALL(mock_cm, IsUpdateAllowedOver(NetworkConnectionType::kEthernet, _))
    .WillRepeatedly(Return(false));

  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateIgnoredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kIgnored,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, ValidUpdateBlockedByRollback) {
  string rollback_version = "1234.0.0";
  OmahaResponse response;

  MockPayloadState mock_payload_state;
  fake_system_state_.set_payload_state(&mock_payload_state);

  EXPECT_CALL(mock_payload_state, GetRollbackVersion())
    .WillRepeatedly(Return(rollback_version));

  fake_update_response_.version = rollback_version;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateIgnoredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kIgnored,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, WallClockBasedWaitAloneCausesScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_update_check_count_wait_enabled(false);
  params.set_waiting_period(TimeDelta::FromDays(2));

  ASSERT_FALSE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateDeferredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kDeferring,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);

  // Verify if we are interactive check we don't defer.
  params.set_interactive(true);
  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, NoWallClockBasedWaitCausesNoScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(false);
  params.set_waiting_period(TimeDelta::FromDays(2));

  params.set_update_check_count_wait_enabled(true);
  params.set_min_update_checks_needed(1);
  params.set_max_update_checks_allowed(8);

  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, ZeroMaxDaysToScatterCausesNoScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta::FromDays(2));

  params.set_update_check_count_wait_enabled(true);
  params.set_min_update_checks_needed(1);
  params.set_max_update_checks_allowed(8);

  fake_update_response_.max_days_to_scatter = "0";
  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}


TEST_F(OmahaRequestActionTest, ZeroUpdateCheckCountCausesNoScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta());

  params.set_update_check_count_wait_enabled(true);
  params.set_min_update_checks_needed(0);
  params.set_max_update_checks_allowed(0);

  ASSERT_TRUE(TestUpdateCheck(
                      &params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  int64_t count;
  ASSERT_TRUE(fake_prefs_.GetInt64(kPrefsUpdateCheckCount, &count));
  ASSERT_EQ(count, 0);
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, NonZeroUpdateCheckCountCausesScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta());

  params.set_update_check_count_wait_enabled(true);
  params.set_min_update_checks_needed(1);
  params.set_max_update_checks_allowed(8);

  ASSERT_FALSE(TestUpdateCheck(
                      &params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateDeferredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kDeferring,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  int64_t count;
  ASSERT_TRUE(fake_prefs_.GetInt64(kPrefsUpdateCheckCount, &count));
  ASSERT_GT(count, 0);
  EXPECT_FALSE(response.update_exists);

  // Verify if we are interactive check we don't defer.
  params.set_interactive(true);
  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, ExistingUpdateCheckCountCausesScattering) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta());

  params.set_update_check_count_wait_enabled(true);
  params.set_min_update_checks_needed(1);
  params.set_max_update_checks_allowed(8);

  ASSERT_TRUE(fake_prefs_.SetInt64(kPrefsUpdateCheckCount, 5));

  ASSERT_FALSE(TestUpdateCheck(
                      &params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateDeferredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kDeferring,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  int64_t count;
  ASSERT_TRUE(fake_prefs_.GetInt64(kPrefsUpdateCheckCount, &count));
  // count remains the same, as the decrementing happens in update_attempter
  // which this test doesn't exercise.
  ASSERT_EQ(count, 5);
  EXPECT_FALSE(response.update_exists);

  // Verify if we are interactive check we don't defer.
  params.set_interactive(true);
  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, CohortsArePersisted) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  fake_update_response_.include_cohorts = true;
  fake_update_response_.cohort = "s/154454/8479665";
  fake_update_response_.cohorthint = "please-put-me-on-beta";
  fake_update_response_.cohortname = "stable";

  ASSERT_TRUE(TestUpdateCheck(&params,
                              fake_update_response_.GetUpdateResponse(),
                              -1,
                              false,  // ping_only
                              ErrorCode::kSuccess,
                              metrics::CheckResult::kUpdateAvailable,
                              metrics::CheckReaction::kUpdating,
                              metrics::DownloadErrorCode::kUnset,
                              &response,
                              nullptr));

  string value;
  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohort, &value));
  EXPECT_EQ(fake_update_response_.cohort, value);

  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohortHint, &value));
  EXPECT_EQ(fake_update_response_.cohorthint, value);

  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohortName, &value));
  EXPECT_EQ(fake_update_response_.cohortname, value);
}

TEST_F(OmahaRequestActionTest, CohortsAreUpdated) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  EXPECT_TRUE(fake_prefs_.SetString(kPrefsOmahaCohort, "old_value"));
  EXPECT_TRUE(fake_prefs_.SetString(kPrefsOmahaCohortHint, "old_hint"));
  EXPECT_TRUE(fake_prefs_.SetString(kPrefsOmahaCohortName, "old_name"));
  fake_update_response_.include_cohorts = true;
  fake_update_response_.cohort = "s/154454/8479665";
  fake_update_response_.cohorthint = "please-put-me-on-beta";
  fake_update_response_.cohortname = "";

  ASSERT_TRUE(TestUpdateCheck(&params,
                              fake_update_response_.GetUpdateResponse(),
                              -1,
                              false,  // ping_only
                              ErrorCode::kSuccess,
                              metrics::CheckResult::kUpdateAvailable,
                              metrics::CheckReaction::kUpdating,
                              metrics::DownloadErrorCode::kUnset,
                              &response,
                              nullptr));

  string value;
  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohort, &value));
  EXPECT_EQ(fake_update_response_.cohort, value);

  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohortHint, &value));
  EXPECT_EQ(fake_update_response_.cohorthint, value);

  EXPECT_FALSE(fake_prefs_.GetString(kPrefsOmahaCohortName, &value));
}

TEST_F(OmahaRequestActionTest, CohortsAreNotModifiedWhenMissing) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  EXPECT_TRUE(fake_prefs_.SetString(kPrefsOmahaCohort, "old_value"));

  ASSERT_TRUE(TestUpdateCheck(&params,
                              fake_update_response_.GetUpdateResponse(),
                              -1,
                              false,  // ping_only
                              ErrorCode::kSuccess,
                              metrics::CheckResult::kUpdateAvailable,
                              metrics::CheckReaction::kUpdating,
                              metrics::DownloadErrorCode::kUnset,
                              &response,
                              nullptr));

  string value;
  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohort, &value));
  EXPECT_EQ("old_value", value);

  EXPECT_FALSE(fake_prefs_.GetString(kPrefsOmahaCohortHint, &value));
  EXPECT_FALSE(fake_prefs_.GetString(kPrefsOmahaCohortName, &value));
}

TEST_F(OmahaRequestActionTest, CohortsArePersistedWhenNoUpdate) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  fake_update_response_.include_cohorts = true;
  fake_update_response_.cohort = "s/154454/8479665";
  fake_update_response_.cohorthint = "please-put-me-on-beta";
  fake_update_response_.cohortname = "stable";

  ASSERT_TRUE(TestUpdateCheck(&params,
                              fake_update_response_.GetNoUpdateResponse(),
                              -1,
                              false,  // ping_only
                              ErrorCode::kSuccess,
                              metrics::CheckResult::kNoUpdateAvailable,
                              metrics::CheckReaction::kUnset,
                              metrics::DownloadErrorCode::kUnset,
                              &response,
                              nullptr));

  string value;
  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohort, &value));
  EXPECT_EQ(fake_update_response_.cohort, value);

  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohortHint, &value));
  EXPECT_EQ(fake_update_response_.cohorthint, value);

  EXPECT_TRUE(fake_prefs_.GetString(kPrefsOmahaCohortName, &value));
  EXPECT_EQ(fake_update_response_.cohortname, value);
}

TEST_F(OmahaRequestActionTest, NoOutputPipeTest) {
  const string http_response(fake_update_response_.GetNoUpdateResponse());

  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();

  OmahaRequestParams params = request_params_;
  fake_system_state_.set_request_params(&params);
  OmahaRequestAction action(&fake_system_state_, nullptr,
                            brillo::make_unique_ptr(
                                new MockHttpFetcher(http_response.data(),
                                                    http_response.size(),
                                                    nullptr)),
                            false);
  OmahaRequestActionTestProcessorDelegate delegate;
  ActionProcessor processor;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&action);

  loop.PostTask(base::Bind([&processor] { processor.StartProcessing(); }));
  loop.Run();
  EXPECT_FALSE(loop.PendingTasks());
  EXPECT_FALSE(processor.IsRunning());
}

TEST_F(OmahaRequestActionTest, InvalidXmlTest) {
  OmahaResponse response;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      "invalid xml>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaRequestXMLParseError,
                      metrics::CheckResult::kParsingError,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, EmptyResponseTest) {
  OmahaResponse response;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      "",
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaRequestEmptyResponseError,
                      metrics::CheckResult::kParsingError,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, MissingStatusTest) {
  OmahaResponse response;
  ASSERT_FALSE(TestUpdateCheck(
      nullptr,  // request_params
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\">"
      "<daystart elapsed_seconds=\"100\"/>"
      "<app appid=\"foo\" status=\"ok\">"
      "<ping status=\"ok\"/>"
      "<updatecheck/></app></response>",
      -1,
      false,  // ping_only
      ErrorCode::kOmahaResponseInvalid,
      metrics::CheckResult::kParsingError,
      metrics::CheckReaction::kUnset,
      metrics::DownloadErrorCode::kUnset,
      &response,
      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, InvalidStatusTest) {
  OmahaResponse response;
  ASSERT_FALSE(TestUpdateCheck(
      nullptr,  // request_params
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\">"
      "<daystart elapsed_seconds=\"100\"/>"
      "<app appid=\"foo\" status=\"ok\">"
      "<ping status=\"ok\"/>"
      "<updatecheck status=\"InvalidStatusTest\"/></app></response>",
      -1,
      false,  // ping_only
      ErrorCode::kOmahaResponseInvalid,
      metrics::CheckResult::kParsingError,
      metrics::CheckReaction::kUnset,
      metrics::DownloadErrorCode::kUnset,
      &response,
      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, MissingNodesetTest) {
  OmahaResponse response;
  ASSERT_FALSE(TestUpdateCheck(
      nullptr,  // request_params
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\">"
      "<daystart elapsed_seconds=\"100\"/>"
      "<app appid=\"foo\" status=\"ok\">"
      "<ping status=\"ok\"/>"
      "</app></response>",
      -1,
      false,  // ping_only
      ErrorCode::kOmahaResponseInvalid,
      metrics::CheckResult::kParsingError,
      metrics::CheckReaction::kUnset,
      metrics::DownloadErrorCode::kUnset,
      &response,
      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, MissingFieldTest) {
  string input_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\">"
      "<daystart elapsed_seconds=\"100\"/>"
      "<app appid=\"xyz\" status=\"ok\">"
      "<updatecheck status=\"ok\">"
      "<urls><url codebase=\"http://missing/field/test/\"/></urls>"
      "<manifest version=\"10.2.3.4\">"
      "<packages><package hash=\"not-used\" name=\"f\" "
      "size=\"587\"/></packages>"
      "<actions><action event=\"postinstall\" "
      "ChromeOSVersion=\"10.2.3.4\" "
      "Prompt=\"false\" "
      "IsDelta=\"true\" "
      "IsDeltaPayload=\"false\" "
      "sha256=\"lkq34j5345\" "
      "needsadmin=\"true\" "
      "/></actions></manifest></updatecheck></app></response>";
  LOG(INFO) << "Input Response = " << input_response;

  OmahaResponse response;
  ASSERT_TRUE(TestUpdateCheck(nullptr,  // request_params
                              input_response,
                              -1,
                              false,  // ping_only
                              ErrorCode::kSuccess,
                              metrics::CheckResult::kUpdateAvailable,
                              metrics::CheckReaction::kUpdating,
                              metrics::DownloadErrorCode::kUnset,
                              &response,
                              nullptr));
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ("10.2.3.4", response.version);
  EXPECT_EQ("http://missing/field/test/f", response.payload_urls[0]);
  EXPECT_EQ("", response.more_info_url);
  EXPECT_EQ("lkq34j5345", response.hash);
  EXPECT_EQ(587, response.size);
  EXPECT_FALSE(response.prompt);
  EXPECT_TRUE(response.deadline.empty());
}

namespace {
class TerminateEarlyTestProcessorDelegate : public ActionProcessorDelegate {
 public:
  void ProcessingStopped(const ActionProcessor* processor) {
    brillo::MessageLoop::current()->BreakLoop();
  }
};

void TerminateTransferTestStarter(ActionProcessor* processor) {
  processor->StartProcessing();
  CHECK(processor->IsRunning());
  processor->StopProcessing();
}
}  // namespace

TEST_F(OmahaRequestActionTest, TerminateTransferTest) {
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();

  string http_response("doesn't matter");
  OmahaRequestAction action(&fake_system_state_, nullptr,
                            brillo::make_unique_ptr(
                                new MockHttpFetcher(http_response.data(),
                                                    http_response.size(),
                                                    nullptr)),
                            false);
  TerminateEarlyTestProcessorDelegate delegate;
  ActionProcessor processor;
  processor.set_delegate(&delegate);
  processor.EnqueueAction(&action);

  loop.PostTask(base::Bind(&TerminateTransferTestStarter, &processor));
  loop.Run();
  EXPECT_FALSE(loop.PendingTasks());
}

TEST_F(OmahaRequestActionTest, XmlEncodeTest) {
  string output;
  EXPECT_TRUE(XmlEncode("ab", &output));
  EXPECT_EQ("ab", output);
  EXPECT_TRUE(XmlEncode("a<b", &output));
  EXPECT_EQ("a&lt;b", output);
  EXPECT_TRUE(XmlEncode("<&>\"\'\\", &output));
  EXPECT_EQ("&lt;&amp;&gt;&quot;&apos;\\", output);
  EXPECT_TRUE(XmlEncode("&lt;&amp;&gt;", &output));
  EXPECT_EQ("&amp;lt;&amp;amp;&amp;gt;", output);
  // Check that unterminated UTF-8 strings are handled properly.
  EXPECT_FALSE(XmlEncode("\xc2", &output));
  // Fail with invalid ASCII-7 chars.
  EXPECT_FALSE(XmlEncode("This is an 'n' with a tilde: \xc3\xb1", &output));
}

TEST_F(OmahaRequestActionTest, XmlEncodeWithDefaultTest) {
  EXPECT_EQ("&lt;&amp;&gt;", XmlEncodeWithDefault("<&>", "something else"));
  EXPECT_EQ("<not escaped>", XmlEncodeWithDefault("\xc2", "<not escaped>"));
}

TEST_F(OmahaRequestActionTest, XmlEncodeIsUsedForParams) {
  brillo::Blob post_data;

  // Make sure XML Encode is being called on the params
  OmahaRequestParams params(&fake_system_state_,
                            constants::kOmahaPlatformName,
                            OmahaRequestParams::kOsVersion,
                            "testtheservice_pack>",
                            "x86 generic<id",
                            kTestAppId,
                            "0.1.0.0",
                            "en-US",
                            "unittest_track&lt;",
                            "<OEM MODEL>",
                            "ChromeOSFirmware.1.0",
                            "EC100",
                            false,   // delta okay
                            false,   // interactive
                            "http://url",
                            "");     // target_version_prefix
  fake_prefs_.SetString(kPrefsOmahaCohort, "evil\nstring");
  fake_prefs_.SetString(kPrefsOmahaCohortHint, "evil&string\\");
  fake_prefs_.SetString(kPrefsOmahaCohortName,
                        base::JoinString(
                            vector<string>(100, "My spoon is too big."), " "));
  OmahaResponse response;
  ASSERT_FALSE(
      TestUpdateCheck(&params,
                      "invalid xml>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaRequestXMLParseError,
                      metrics::CheckResult::kParsingError,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      &post_data));
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(string::npos, post_str.find("testtheservice_pack&gt;"));
  EXPECT_EQ(string::npos, post_str.find("testtheservice_pack>"));
  EXPECT_NE(string::npos, post_str.find("x86 generic&lt;id"));
  EXPECT_EQ(string::npos, post_str.find("x86 generic<id"));
  EXPECT_NE(string::npos, post_str.find("unittest_track&amp;lt;"));
  EXPECT_EQ(string::npos, post_str.find("unittest_track&lt;"));
  EXPECT_NE(string::npos, post_str.find("&lt;OEM MODEL&gt;"));
  EXPECT_EQ(string::npos, post_str.find("<OEM MODEL>"));
  EXPECT_NE(string::npos, post_str.find("cohort=\"evil\nstring\""));
  EXPECT_EQ(string::npos, post_str.find("cohorthint=\"evil&string\\\""));
  EXPECT_NE(string::npos, post_str.find("cohorthint=\"evil&amp;string\\\""));
  // Values from Prefs that are too big are removed from the XML instead of
  // encoded.
  EXPECT_EQ(string::npos, post_str.find("cohortname="));
}

TEST_F(OmahaRequestActionTest, XmlDecodeTest) {
  OmahaResponse response;
  fake_update_response_.deadline = "&lt;20110101";
  fake_update_response_.more_info_url = "testthe&lt;url";
  fake_update_response_.codebase = "testthe&amp;codebase/";
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  EXPECT_EQ(response.more_info_url, "testthe<url");
  EXPECT_EQ(response.payload_urls[0], "testthe&codebase/file.signed");
  EXPECT_EQ(response.deadline, "<20110101");
}

TEST_F(OmahaRequestActionTest, ParseIntTest) {
  OmahaResponse response;
  // overflows int32_t:
  fake_update_response_.size = 123123123123123ll;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  EXPECT_EQ(response.size, 123123123123123ll);
}

TEST_F(OmahaRequestActionTest, FormatUpdateCheckOutputTest) {
  brillo::Blob post_data;
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);

  EXPECT_CALL(prefs, GetString(kPrefsPreviousVersion, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(string("")), Return(true)));
  // An existing but empty previous version means that we didn't reboot to a new
  // update, therefore, no need to update the previous version.
  EXPECT_CALL(prefs, SetString(kPrefsPreviousVersion, _)).Times(0);
  ASSERT_FALSE(TestUpdateCheck(nullptr,  // request_params
                               "invalid xml>",
                               -1,
                               false,  // ping_only
                               ErrorCode::kOmahaRequestXMLParseError,
                               metrics::CheckResult::kParsingError,
                               metrics::CheckReaction::kUnset,
                               metrics::DownloadErrorCode::kUnset,
                               nullptr,  // response
                               &post_data));
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(post_str.find(
      "        <ping active=\"1\" a=\"-1\" r=\"-1\"></ping>\n"
      "        <updatecheck targetversionprefix=\"\"></updatecheck>\n"),
      string::npos);
  EXPECT_NE(post_str.find("hardware_class=\"OEM MODEL 09235 7471\""),
            string::npos);
  EXPECT_NE(post_str.find("fw_version=\"ChromeOSFirmware.1.0\""),
            string::npos);
  EXPECT_NE(post_str.find("ec_version=\"0X0A1\""),
            string::npos);
  // No <event> tag should be sent if we didn't reboot to an update.
  EXPECT_EQ(post_str.find("<event"), string::npos);
}


TEST_F(OmahaRequestActionTest, FormatSuccessEventOutputTest) {
  brillo::Blob post_data;
  TestEvent(request_params_,
            new OmahaEvent(OmahaEvent::kTypeUpdateDownloadStarted),
            "invalid xml>",
            &post_data);
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  string expected_event = base::StringPrintf(
      "        <event eventtype=\"%d\" eventresult=\"%d\"></event>\n",
      OmahaEvent::kTypeUpdateDownloadStarted,
      OmahaEvent::kResultSuccess);
  EXPECT_NE(post_str.find(expected_event), string::npos);
  EXPECT_EQ(post_str.find("ping"), string::npos);
  EXPECT_EQ(post_str.find("updatecheck"), string::npos);
}

TEST_F(OmahaRequestActionTest, FormatErrorEventOutputTest) {
  brillo::Blob post_data;
  TestEvent(request_params_,
            new OmahaEvent(OmahaEvent::kTypeDownloadComplete,
                           OmahaEvent::kResultError,
                           ErrorCode::kError),
            "invalid xml>",
            &post_data);
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  string expected_event = base::StringPrintf(
      "        <event eventtype=\"%d\" eventresult=\"%d\" "
      "errorcode=\"%d\"></event>\n",
      OmahaEvent::kTypeDownloadComplete,
      OmahaEvent::kResultError,
      static_cast<int>(ErrorCode::kError));
  EXPECT_NE(post_str.find(expected_event), string::npos);
  EXPECT_EQ(post_str.find("updatecheck"), string::npos);
}

TEST_F(OmahaRequestActionTest, IsEventTest) {
  string http_response("doesn't matter");
  // Create a copy of the OmahaRequestParams to reuse it later.
  OmahaRequestParams params = request_params_;
  fake_system_state_.set_request_params(&params);
  OmahaRequestAction update_check_action(
      &fake_system_state_,
      nullptr,
      brillo::make_unique_ptr(
          new MockHttpFetcher(http_response.data(),
                              http_response.size(),
                              nullptr)),
      false);
  EXPECT_FALSE(update_check_action.IsEvent());

  params = request_params_;
  fake_system_state_.set_request_params(&params);
  OmahaRequestAction event_action(
      &fake_system_state_,
      new OmahaEvent(OmahaEvent::kTypeUpdateComplete),
      brillo::make_unique_ptr(
          new MockHttpFetcher(http_response.data(),
                              http_response.size(),
                              nullptr)),
      false);
  EXPECT_TRUE(event_action.IsEvent());
}

TEST_F(OmahaRequestActionTest, FormatDeltaOkayOutputTest) {
  for (int i = 0; i < 2; i++) {
    bool delta_okay = i == 1;
    const char* delta_okay_str = delta_okay ? "true" : "false";
    brillo::Blob post_data;
    OmahaRequestParams params(&fake_system_state_,
                              constants::kOmahaPlatformName,
                              OmahaRequestParams::kOsVersion,
                              "service_pack",
                              "x86-generic",
                              kTestAppId,
                              "0.1.0.0",
                              "en-US",
                              "unittest_track",
                              "OEM MODEL REV 1234",
                              "ChromeOSFirmware.1.0",
                              "EC100",
                              delta_okay,
                              false,  // interactive
                              "http://url",
                              "");    // target_version_prefix
    ASSERT_FALSE(TestUpdateCheck(&params,
                                 "invalid xml>",
                                 -1,
                                 false,  // ping_only
                                 ErrorCode::kOmahaRequestXMLParseError,
                                 metrics::CheckResult::kParsingError,
                                 metrics::CheckReaction::kUnset,
                                 metrics::DownloadErrorCode::kUnset,
                                 nullptr,
                                 &post_data));
    // convert post_data to string
    string post_str(post_data.begin(), post_data.end());
    EXPECT_NE(post_str.find(base::StringPrintf(" delta_okay=\"%s\"",
                                               delta_okay_str)),
              string::npos)
        << "i = " << i;
  }
}

TEST_F(OmahaRequestActionTest, FormatInteractiveOutputTest) {
  for (int i = 0; i < 2; i++) {
    bool interactive = i == 1;
    const char* interactive_str = interactive ? "ondemandupdate" : "scheduler";
    brillo::Blob post_data;
    FakeSystemState fake_system_state;
    OmahaRequestParams params(&fake_system_state_,
                              constants::kOmahaPlatformName,
                              OmahaRequestParams::kOsVersion,
                              "service_pack",
                              "x86-generic",
                              kTestAppId,
                              "0.1.0.0",
                              "en-US",
                              "unittest_track",
                              "OEM MODEL REV 1234",
                              "ChromeOSFirmware.1.0",
                              "EC100",
                              true,   // delta_okay
                              interactive,
                              "http://url",
                              "");    // target_version_prefix
    ASSERT_FALSE(TestUpdateCheck(&params,
                                 "invalid xml>",
                                 -1,
                                 false,  // ping_only
                                 ErrorCode::kOmahaRequestXMLParseError,
                                 metrics::CheckResult::kParsingError,
                                 metrics::CheckReaction::kUnset,
                                 metrics::DownloadErrorCode::kUnset,
                                 nullptr,
                                 &post_data));
    // convert post_data to string
    string post_str(post_data.begin(), post_data.end());
    EXPECT_NE(post_str.find(base::StringPrintf("installsource=\"%s\"",
                                               interactive_str)),
              string::npos)
        << "i = " << i;
  }
}

TEST_F(OmahaRequestActionTest, OmahaEventTest) {
  OmahaEvent default_event;
  EXPECT_EQ(OmahaEvent::kTypeUnknown, default_event.type);
  EXPECT_EQ(OmahaEvent::kResultError, default_event.result);
  EXPECT_EQ(ErrorCode::kError, default_event.error_code);

  OmahaEvent success_event(OmahaEvent::kTypeUpdateDownloadStarted);
  EXPECT_EQ(OmahaEvent::kTypeUpdateDownloadStarted, success_event.type);
  EXPECT_EQ(OmahaEvent::kResultSuccess, success_event.result);
  EXPECT_EQ(ErrorCode::kSuccess, success_event.error_code);

  OmahaEvent error_event(OmahaEvent::kTypeUpdateDownloadFinished,
                         OmahaEvent::kResultError,
                         ErrorCode::kError);
  EXPECT_EQ(OmahaEvent::kTypeUpdateDownloadFinished, error_event.type);
  EXPECT_EQ(OmahaEvent::kResultError, error_event.result);
  EXPECT_EQ(ErrorCode::kError, error_event.error_code);
}

void OmahaRequestActionTest::PingTest(bool ping_only) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(kPrefsMetricsCheckLastReportingTime, _))
    .Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  // Add a few hours to the day difference to test no rounding, etc.
  int64_t five_days_ago =
      (Time::Now() - TimeDelta::FromHours(5 * 24 + 13)).ToInternalValue();
  int64_t six_days_ago =
      (Time::Now() - TimeDelta::FromHours(6 * 24 + 11)).ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsInstallDateDays, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(0), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(six_days_ago), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(five_days_ago), Return(true)));
  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      ping_only,
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUnset,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(post_str.find("<ping active=\"1\" a=\"6\" r=\"5\"></ping>"),
            string::npos);
  if (ping_only) {
    EXPECT_EQ(post_str.find("updatecheck"), string::npos);
    EXPECT_EQ(post_str.find("previousversion"), string::npos);
  } else {
    EXPECT_NE(post_str.find("updatecheck"), string::npos);
    EXPECT_NE(post_str.find("previousversion"), string::npos);
  }
}

TEST_F(OmahaRequestActionTest, PingTestSendOnlyAPing) {
  PingTest(true  /* ping_only */);
}

TEST_F(OmahaRequestActionTest, PingTestSendAlsoAnUpdateCheck) {
  PingTest(false  /* ping_only */);
}

TEST_F(OmahaRequestActionTest, ActivePingTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(kPrefsMetricsCheckLastReportingTime, _))
    .Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  int64_t three_days_ago =
      (Time::Now() - TimeDelta::FromHours(3 * 24 + 12)).ToInternalValue();
  int64_t now = Time::Now().ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsInstallDateDays, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(0), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(three_days_ago), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(now), Return(true)));
  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(post_str.find("<ping active=\"1\" a=\"3\"></ping>"),
            string::npos);
}

TEST_F(OmahaRequestActionTest, RollCallPingTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(kPrefsMetricsCheckLastReportingTime, _))
    .Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  int64_t four_days_ago =
      (Time::Now() - TimeDelta::FromHours(4 * 24)).ToInternalValue();
  int64_t now = Time::Now().ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsInstallDateDays, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(0), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(now), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(four_days_ago), Return(true)));
  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(post_str.find("<ping active=\"1\" r=\"4\"></ping>\n"),
            string::npos);
}

TEST_F(OmahaRequestActionTest, NoPingTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(kPrefsMetricsCheckLastReportingTime, _))
    .Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  int64_t one_hour_ago =
      (Time::Now() - TimeDelta::FromHours(1)).ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsInstallDateDays, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(0), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(one_hour_ago), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(one_hour_ago), Return(true)));
  // LastActivePingDay and PrefsLastRollCallPingDay are set even if we didn't
  // send a ping.
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(Return(true));
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(Return(true));
  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());
  EXPECT_EQ(post_str.find("ping"), string::npos);
}

TEST_F(OmahaRequestActionTest, IgnoreEmptyPingTest) {
  // This test ensures that we ignore empty ping only requests.
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  int64_t now = Time::Now().ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(now), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(now), Return(true)));
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay, _)).Times(0);
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay, _)).Times(0);
  brillo::Blob post_data;
  EXPECT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      true,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUnset,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  EXPECT_EQ(0U, post_data.size());
}

TEST_F(OmahaRequestActionTest, BackInTimePingTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(kPrefsMetricsCheckLastReportingTime, _))
    .Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  int64_t future =
      (Time::Now() + TimeDelta::FromHours(3 * 24 + 4)).ToInternalValue();
  EXPECT_CALL(prefs, GetInt64(kPrefsInstallDateDays, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(0), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(future), Return(true)));
  EXPECT_CALL(prefs, GetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(DoAll(SetArgumentPointee<1>(future), Return(true)));
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay, _))
      .WillOnce(Return(true));
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay, _))
      .WillOnce(Return(true));
  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response "
                      "protocol=\"3.0\"><daystart elapsed_seconds=\"100\"/>"
                      "<app appid=\"foo\" status=\"ok\"><ping status=\"ok\"/>"
                      "<updatecheck status=\"noupdate\"/></app></response>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());
  EXPECT_EQ(post_str.find("ping"), string::npos);
}

TEST_F(OmahaRequestActionTest, LastPingDayUpdateTest) {
  // This test checks that the action updates the last ping day to now
  // minus 200 seconds with a slack of 5 seconds. Therefore, the test
  // may fail if it runs for longer than 5 seconds. It shouldn't run
  // that long though.
  int64_t midnight =
      (Time::Now() - TimeDelta::FromSeconds(200)).ToInternalValue();
  int64_t midnight_slack =
      (Time::Now() - TimeDelta::FromSeconds(195)).ToInternalValue();
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay,
                              AllOf(Ge(midnight), Le(midnight_slack))))
      .WillOnce(Return(true));
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay,
                              AllOf(Ge(midnight), Le(midnight_slack))))
      .WillOnce(Return(true));
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response "
                      "protocol=\"3.0\"><daystart elapsed_seconds=\"200\"/>"
                      "<app appid=\"foo\" status=\"ok\"><ping status=\"ok\"/>"
                      "<updatecheck status=\"noupdate\"/></app></response>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      nullptr));
}

TEST_F(OmahaRequestActionTest, NoElapsedSecondsTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay, _)).Times(0);
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay, _)).Times(0);
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response "
                      "protocol=\"3.0\"><daystart blah=\"200\"/>"
                      "<app appid=\"foo\" status=\"ok\"><ping status=\"ok\"/>"
                      "<updatecheck status=\"noupdate\"/></app></response>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      nullptr));
}

TEST_F(OmahaRequestActionTest, BadElapsedSecondsTest) {
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  EXPECT_CALL(prefs, GetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(_, _)).Times(AnyNumber());
  EXPECT_CALL(prefs, SetInt64(kPrefsLastActivePingDay, _)).Times(0);
  EXPECT_CALL(prefs, SetInt64(kPrefsLastRollCallPingDay, _)).Times(0);
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response "
                      "protocol=\"3.0\"><daystart elapsed_seconds=\"x\"/>"
                      "<app appid=\"foo\" status=\"ok\"><ping status=\"ok\"/>"
                      "<updatecheck status=\"noupdate\"/></app></response>",
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      nullptr));
}

TEST_F(OmahaRequestActionTest, NoUniqueIDTest) {
  brillo::Blob post_data;
  ASSERT_FALSE(TestUpdateCheck(nullptr,  // request_params
                               "invalid xml>",
                               -1,
                               false,  // ping_only
                               ErrorCode::kOmahaRequestXMLParseError,
                               metrics::CheckResult::kParsingError,
                               metrics::CheckReaction::kUnset,
                               metrics::DownloadErrorCode::kUnset,
                               nullptr,  // response
                               &post_data));
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  EXPECT_EQ(post_str.find("machineid="), string::npos);
  EXPECT_EQ(post_str.find("userid="), string::npos);
}

TEST_F(OmahaRequestActionTest, NetworkFailureTest) {
  OmahaResponse response;
  const int http_error_code =
      static_cast<int>(ErrorCode::kOmahaRequestHTTPResponseBase) + 501;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      "",
                      501,
                      false,  // ping_only
                      static_cast<ErrorCode>(http_error_code),
                      metrics::CheckResult::kDownloadError,
                      metrics::CheckReaction::kUnset,
                      static_cast<metrics::DownloadErrorCode>(501),
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, NetworkFailureBadHTTPCodeTest) {
  OmahaResponse response;
  const int http_error_code =
      static_cast<int>(ErrorCode::kOmahaRequestHTTPResponseBase) + 999;
  ASSERT_FALSE(
      TestUpdateCheck(nullptr,  // request_params
                      "",
                      1500,
                      false,  // ping_only
                      static_cast<ErrorCode>(http_error_code),
                      metrics::CheckResult::kDownloadError,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kHttpStatusOther,
                      &response,
                      nullptr));
  EXPECT_FALSE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, TestUpdateFirstSeenAtGetsPersistedFirstTime) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta().FromDays(1));
  params.set_update_check_count_wait_enabled(false);

  ASSERT_FALSE(TestUpdateCheck(
                      &params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kOmahaUpdateDeferredPerPolicy,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kDeferring,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  int64_t timestamp = 0;
  ASSERT_TRUE(fake_prefs_.GetInt64(kPrefsUpdateFirstSeenAt, &timestamp));
  ASSERT_GT(timestamp, 0);
  EXPECT_FALSE(response.update_exists);

  // Verify if we are interactive check we don't defer.
  params.set_interactive(true);
  ASSERT_TRUE(
      TestUpdateCheck(&params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);
}

TEST_F(OmahaRequestActionTest, TestUpdateFirstSeenAtGetsUsedIfAlreadyPresent) {
  OmahaResponse response;
  OmahaRequestParams params = request_params_;
  params.set_wall_clock_based_wait_enabled(true);
  params.set_waiting_period(TimeDelta().FromDays(1));
  params.set_update_check_count_wait_enabled(false);

  // Set the timestamp to a very old value such that it exceeds the
  // waiting period set above.
  Time t1;
  Time::FromString("1/1/2012", &t1);
  ASSERT_TRUE(fake_prefs_.SetInt64(
      kPrefsUpdateFirstSeenAt, t1.ToInternalValue()));
  ASSERT_TRUE(TestUpdateCheck(
                      &params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));

  EXPECT_TRUE(response.update_exists);

  // Make sure the timestamp t1 is unchanged showing that it was reused.
  int64_t timestamp = 0;
  ASSERT_TRUE(fake_prefs_.GetInt64(kPrefsUpdateFirstSeenAt, &timestamp));
  ASSERT_TRUE(timestamp == t1.ToInternalValue());
}

TEST_F(OmahaRequestActionTest, TestChangingToMoreStableChannel) {
  // Create a uniquely named test directory.
  string test_dir;
  ASSERT_TRUE(utils::MakeTempDirectory(
          "omaha_request_action-test-XXXXXX", &test_dir));

  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir + "/etc"));
  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir +
                      kStatefulPartition + "/etc"));
  brillo::Blob post_data;
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  ASSERT_TRUE(WriteFileString(
      test_dir + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID={11111111-1111-1111-1111-111111111111}\n"
      "CHROMEOS_BOARD_APPID={22222222-2222-2222-2222-222222222222}\n"
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"));
  ASSERT_TRUE(WriteFileString(
      test_dir + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_IS_POWERWASH_ALLOWED=true\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  OmahaRequestParams params = request_params_;
  params.set_root(test_dir);
  params.Init("1.2.3.4", "", 0);
  EXPECT_EQ("canary-channel", params.current_channel());
  EXPECT_EQ("stable-channel", params.target_channel());
  EXPECT_TRUE(params.to_more_stable_channel());
  EXPECT_TRUE(params.is_powerwash_allowed());
  ASSERT_FALSE(TestUpdateCheck(&params,
                               "invalid xml>",
                               -1,
                               false,  // ping_only
                               ErrorCode::kOmahaRequestXMLParseError,
                               metrics::CheckResult::kParsingError,
                               metrics::CheckReaction::kUnset,
                               metrics::DownloadErrorCode::kUnset,
                               nullptr,  // response
                               &post_data));
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(string::npos, post_str.find(
      "appid=\"{22222222-2222-2222-2222-222222222222}\" "
      "version=\"0.0.0.0\" from_version=\"1.2.3.4\" "
      "track=\"stable-channel\" from_track=\"canary-channel\" "));

  ASSERT_TRUE(base::DeleteFile(base::FilePath(test_dir), true));
}

TEST_F(OmahaRequestActionTest, TestChangingToLessStableChannel) {
  // Create a uniquely named test directory.
  string test_dir;
  ASSERT_TRUE(utils::MakeTempDirectory(
          "omaha_request_action-test-XXXXXX", &test_dir));

  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir + "/etc"));
  ASSERT_EQ(0, System(string("mkdir -p ") + test_dir +
                      kStatefulPartition + "/etc"));
  brillo::Blob post_data;
  NiceMock<MockPrefs> prefs;
  fake_system_state_.set_prefs(&prefs);
  ASSERT_TRUE(WriteFileString(
      test_dir + "/etc/lsb-release",
      "CHROMEOS_RELEASE_APPID={11111111-1111-1111-1111-111111111111}\n"
      "CHROMEOS_BOARD_APPID={22222222-2222-2222-2222-222222222222}\n"
      "CHROMEOS_RELEASE_TRACK=stable-channel\n"));
  ASSERT_TRUE(WriteFileString(
      test_dir + kStatefulPartition + "/etc/lsb-release",
      "CHROMEOS_RELEASE_TRACK=canary-channel\n"));
  OmahaRequestParams params = request_params_;
  params.set_root(test_dir);
  params.Init("5.6.7.8", "", 0);
  EXPECT_EQ("stable-channel", params.current_channel());
  EXPECT_EQ("canary-channel", params.target_channel());
  EXPECT_FALSE(params.to_more_stable_channel());
  EXPECT_FALSE(params.is_powerwash_allowed());
  ASSERT_FALSE(TestUpdateCheck(&params,
                               "invalid xml>",
                               -1,
                               false,  // ping_only
                               ErrorCode::kOmahaRequestXMLParseError,
                               metrics::CheckResult::kParsingError,
                               metrics::CheckReaction::kUnset,
                               metrics::DownloadErrorCode::kUnset,
                               nullptr,  // response
                               &post_data));
  // convert post_data to string
  string post_str(post_data.begin(), post_data.end());
  EXPECT_NE(string::npos, post_str.find(
      "appid=\"{11111111-1111-1111-1111-111111111111}\" "
      "version=\"5.6.7.8\" "
      "track=\"canary-channel\" from_track=\"stable-channel\""));
  EXPECT_EQ(string::npos, post_str.find("from_version"));
}

// Checks that the initial ping with a=-1 r=-1 is not send when the device
// was powerwashed.
TEST_F(OmahaRequestActionTest, PingWhenPowerwashed) {
  fake_prefs_.SetString(kPrefsPreviousVersion, "");

  // Flag that the device was powerwashed in the past.
  fake_system_state_.fake_hardware()->SetPowerwashCount(1);

  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  // We shouldn't send a ping in this case since powerwash > 0.
  string post_str(post_data.begin(), post_data.end());
  EXPECT_EQ(string::npos, post_str.find("<ping"));
}

// Checks that the event 54 is sent on a reboot to a new update.
TEST_F(OmahaRequestActionTest, RebootAfterUpdateEvent) {
  // Flag that the device was updated in a previous boot.
  fake_prefs_.SetString(kPrefsPreviousVersion, "1.2.3.4");

  brillo::Blob post_data;
  ASSERT_TRUE(
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetNoUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kNoUpdateAvailable,
                      metrics::CheckReaction::kUnset,
                      metrics::DownloadErrorCode::kUnset,
                      nullptr,
                      &post_data));
  string post_str(post_data.begin(), post_data.end());

  // An event 54 is included and has the right version.
  EXPECT_NE(string::npos,
            post_str.find(base::StringPrintf(
                              "<event eventtype=\"%d\"",
                              OmahaEvent::kTypeRebootedAfterUpdate)));
  EXPECT_NE(string::npos,
            post_str.find("previousversion=\"1.2.3.4\"></event>"));

  // The previous version flag should have been removed.
  EXPECT_TRUE(fake_prefs_.Exists(kPrefsPreviousVersion));
  string prev_version;
  EXPECT_TRUE(fake_prefs_.GetString(kPrefsPreviousVersion, &prev_version));
  EXPECT_TRUE(prev_version.empty());
}

void OmahaRequestActionTest::P2PTest(
    bool initial_allow_p2p_for_downloading,
    bool initial_allow_p2p_for_sharing,
    bool omaha_disable_p2p_for_downloading,
    bool omaha_disable_p2p_for_sharing,
    bool payload_state_allow_p2p_attempt,
    bool expect_p2p_client_lookup,
    const string& p2p_client_result_url,
    bool expected_allow_p2p_for_downloading,
    bool expected_allow_p2p_for_sharing,
    const string& expected_p2p_url) {
  OmahaResponse response;
  OmahaRequestParams request_params = request_params_;
  bool actual_allow_p2p_for_downloading = initial_allow_p2p_for_downloading;
  bool actual_allow_p2p_for_sharing = initial_allow_p2p_for_sharing;
  string actual_p2p_url;

  MockPayloadState mock_payload_state;
  fake_system_state_.set_payload_state(&mock_payload_state);
  EXPECT_CALL(mock_payload_state, P2PAttemptAllowed())
      .WillRepeatedly(Return(payload_state_allow_p2p_attempt));
  EXPECT_CALL(mock_payload_state, GetUsingP2PForDownloading())
      .WillRepeatedly(ReturnPointee(&actual_allow_p2p_for_downloading));
  EXPECT_CALL(mock_payload_state, GetUsingP2PForSharing())
      .WillRepeatedly(ReturnPointee(&actual_allow_p2p_for_sharing));
  EXPECT_CALL(mock_payload_state, SetUsingP2PForDownloading(_))
      .WillRepeatedly(SaveArg<0>(&actual_allow_p2p_for_downloading));
  EXPECT_CALL(mock_payload_state, SetUsingP2PForSharing(_))
      .WillRepeatedly(SaveArg<0>(&actual_allow_p2p_for_sharing));
  EXPECT_CALL(mock_payload_state, SetP2PUrl(_))
      .WillRepeatedly(SaveArg<0>(&actual_p2p_url));

  MockP2PManager mock_p2p_manager;
  fake_system_state_.set_p2p_manager(&mock_p2p_manager);
  mock_p2p_manager.fake().SetLookupUrlForFileResult(p2p_client_result_url);

  TimeDelta timeout = TimeDelta::FromSeconds(kMaxP2PNetworkWaitTimeSeconds);
  EXPECT_CALL(mock_p2p_manager, LookupUrlForFile(_, _, timeout, _))
      .Times(expect_p2p_client_lookup ? 1 : 0);

  fake_update_response_.disable_p2p_for_downloading =
      omaha_disable_p2p_for_downloading;
  fake_update_response_.disable_p2p_for_sharing = omaha_disable_p2p_for_sharing;
  ASSERT_TRUE(
      TestUpdateCheck(&request_params,
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      &response,
                      nullptr));
  EXPECT_TRUE(response.update_exists);

  EXPECT_EQ(omaha_disable_p2p_for_downloading,
            response.disable_p2p_for_downloading);
  EXPECT_EQ(omaha_disable_p2p_for_sharing,
            response.disable_p2p_for_sharing);

  EXPECT_EQ(expected_allow_p2p_for_downloading,
            actual_allow_p2p_for_downloading);
  EXPECT_EQ(expected_allow_p2p_for_sharing, actual_allow_p2p_for_sharing);
  EXPECT_EQ(expected_p2p_url, actual_p2p_url);
}

TEST_F(OmahaRequestActionTest, P2PWithPeer) {
  P2PTest(true,                   // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          false,                  // omaha_disable_p2p_for_downloading
          false,                  // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          true,                   // expect_p2p_client_lookup
          "http://1.3.5.7/p2p",   // p2p_client_result_url
          true,                   // expected_allow_p2p_for_downloading
          true,                   // expected_allow_p2p_for_sharing
          "http://1.3.5.7/p2p");  // expected_p2p_url
}

TEST_F(OmahaRequestActionTest, P2PWithoutPeer) {
  P2PTest(true,                   // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          false,                  // omaha_disable_p2p_for_downloading
          false,                  // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          true,                   // expect_p2p_client_lookup
          "",                     // p2p_client_result_url
          false,                  // expected_allow_p2p_for_downloading
          true,                   // expected_allow_p2p_for_sharing
          "");                    // expected_p2p_url
}

TEST_F(OmahaRequestActionTest, P2PDownloadNotAllowed) {
  P2PTest(false,                  // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          false,                  // omaha_disable_p2p_for_downloading
          false,                  // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          false,                  // expect_p2p_client_lookup
          "unset",                // p2p_client_result_url
          false,                  // expected_allow_p2p_for_downloading
          true,                   // expected_allow_p2p_for_sharing
          "");                    // expected_p2p_url
}

TEST_F(OmahaRequestActionTest, P2PWithPeerDownloadDisabledByOmaha) {
  P2PTest(true,                   // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          true,                   // omaha_disable_p2p_for_downloading
          false,                  // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          false,                  // expect_p2p_client_lookup
          "unset",                // p2p_client_result_url
          false,                  // expected_allow_p2p_for_downloading
          true,                   // expected_allow_p2p_for_sharing
          "");                    // expected_p2p_url
}

TEST_F(OmahaRequestActionTest, P2PWithPeerSharingDisabledByOmaha) {
  P2PTest(true,                   // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          false,                  // omaha_disable_p2p_for_downloading
          true,                   // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          true,                   // expect_p2p_client_lookup
          "http://1.3.5.7/p2p",   // p2p_client_result_url
          true,                   // expected_allow_p2p_for_downloading
          false,                  // expected_allow_p2p_for_sharing
          "http://1.3.5.7/p2p");  // expected_p2p_url
}

TEST_F(OmahaRequestActionTest, P2PWithPeerBothDisabledByOmaha) {
  P2PTest(true,                   // initial_allow_p2p_for_downloading
          true,                   // initial_allow_p2p_for_sharing
          true,                   // omaha_disable_p2p_for_downloading
          true,                   // omaha_disable_p2p_for_sharing
          true,                   // payload_state_allow_p2p_attempt
          false,                  // expect_p2p_client_lookup
          "unset",                // p2p_client_result_url
          false,                  // expected_allow_p2p_for_downloading
          false,                  // expected_allow_p2p_for_sharing
          "");                    // expected_p2p_url
}

bool OmahaRequestActionTest::InstallDateParseHelper(const string &elapsed_days,
                                                    OmahaResponse *response) {
  fake_update_response_.elapsed_days = elapsed_days;
  return
      TestUpdateCheck(nullptr,  // request_params
                      fake_update_response_.GetUpdateResponse(),
                      -1,
                      false,  // ping_only
                      ErrorCode::kSuccess,
                      metrics::CheckResult::kUpdateAvailable,
                      metrics::CheckReaction::kUpdating,
                      metrics::DownloadErrorCode::kUnset,
                      response,
                      nullptr);
}

TEST_F(OmahaRequestActionTest, ParseInstallDateFromResponse) {
  OmahaResponse response;

  // Check that we parse elapsed_days in the Omaha Response correctly.
  // and that the kPrefsInstallDateDays value is written to.
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsInstallDateDays));
  EXPECT_TRUE(InstallDateParseHelper("42", &response));
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ(42, response.install_date_days);
  EXPECT_TRUE(fake_prefs_.Exists(kPrefsInstallDateDays));
  int64_t prefs_days;
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 42);

  // If there already is a value set, we shouldn't do anything.
  EXPECT_TRUE(InstallDateParseHelper("7", &response));
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ(7, response.install_date_days);
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 42);

  // Note that elapsed_days is not necessarily divisible by 7 so check
  // that we round down correctly when populating kPrefsInstallDateDays.
  EXPECT_TRUE(fake_prefs_.Delete(kPrefsInstallDateDays));
  EXPECT_TRUE(InstallDateParseHelper("23", &response));
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ(23, response.install_date_days);
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 21);

  // Check that we correctly handle elapsed_days not being included in
  // the Omaha Response.
  EXPECT_TRUE(InstallDateParseHelper("", &response));
  EXPECT_TRUE(response.update_exists);
  EXPECT_EQ(-1, response.install_date_days);
}

// If there is no prefs and OOBE is not complete, we should not
// report anything to Omaha.
TEST_F(OmahaRequestActionTest, GetInstallDateWhenNoPrefsNorOOBE) {
  EXPECT_EQ(OmahaRequestAction::GetInstallDate(&fake_system_state_), -1);
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsInstallDateDays));
}

// If OOBE is complete and happened on a valid date (e.g. after Jan
// 1 2007 0:00 PST), that date should be used and written to
// prefs. However, first try with an invalid date and check we do
// nothing.
TEST_F(OmahaRequestActionTest, GetInstallDateWhenOOBECompletedWithInvalidDate) {
  Time oobe_date = Time::FromTimeT(42);  // Dec 31, 1969 16:00:42 PST.
  fake_system_state_.fake_hardware()->SetIsOOBEComplete(oobe_date);
  EXPECT_EQ(OmahaRequestAction::GetInstallDate(&fake_system_state_), -1);
  EXPECT_FALSE(fake_prefs_.Exists(kPrefsInstallDateDays));
}

// Then check with a valid date. The date Jan 20, 2007 0:00 PST
// should yield an InstallDate of 14.
TEST_F(OmahaRequestActionTest, GetInstallDateWhenOOBECompletedWithValidDate) {
  Time oobe_date = Time::FromTimeT(1169280000);  // Jan 20, 2007 0:00 PST.
  fake_system_state_.fake_hardware()->SetIsOOBEComplete(oobe_date);
  EXPECT_EQ(OmahaRequestAction::GetInstallDate(&fake_system_state_), 14);
  EXPECT_TRUE(fake_prefs_.Exists(kPrefsInstallDateDays));

  int64_t prefs_days;
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 14);
}

// Now that we have a valid date in prefs, check that we keep using
// that even if OOBE date reports something else. The date Jan 30,
// 2007 0:00 PST should yield an InstallDate of 28... but since
// there's a prefs file, we should still get 14.
TEST_F(OmahaRequestActionTest, GetInstallDateWhenOOBECompletedDateChanges) {
  // Set a valid date in the prefs first.
  EXPECT_TRUE(fake_prefs_.SetInt64(kPrefsInstallDateDays, 14));

  Time oobe_date = Time::FromTimeT(1170144000);  // Jan 30, 2007 0:00 PST.
  fake_system_state_.fake_hardware()->SetIsOOBEComplete(oobe_date);
  EXPECT_EQ(OmahaRequestAction::GetInstallDate(&fake_system_state_), 14);

  int64_t prefs_days;
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 14);

  // If we delete the prefs file, we should get 28 days.
  EXPECT_TRUE(fake_prefs_.Delete(kPrefsInstallDateDays));
  EXPECT_EQ(OmahaRequestAction::GetInstallDate(&fake_system_state_), 28);
  EXPECT_TRUE(fake_prefs_.GetInt64(kPrefsInstallDateDays, &prefs_days));
  EXPECT_EQ(prefs_days, 28);
}

}  // namespace chromeos_update_engine
