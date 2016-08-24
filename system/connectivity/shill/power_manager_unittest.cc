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

#include "shill/power_manager.h"

#include <memory>
#include <string>

#include <base/bind.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/mock_control.h"
#include "shill/mock_event_dispatcher.h"
#include "shill/mock_metrics.h"
#include "shill/mock_power_manager_proxy.h"
#include "shill/power_manager_proxy_interface.h"

using base::Bind;
using base::Unretained;
using std::map;
using std::string;
using testing::_;
using testing::DoAll;
using testing::IgnoreResult;
using testing::InvokeWithoutArgs;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::Test;

namespace shill {

namespace {

class FakeControl : public MockControl {
 public:
  FakeControl()
      : delegate_(nullptr),
        power_manager_proxy_raw_(new MockPowerManagerProxy),
        power_manager_proxy_(power_manager_proxy_raw_) {}

  virtual PowerManagerProxyInterface* CreatePowerManagerProxy(
      PowerManagerProxyDelegate* delegate,
      const base::Closure& service_appeared_callback,
      const base::Closure& service_vanished_callback) {
    CHECK(power_manager_proxy_);
    delegate_ = delegate;
    // Passes ownership.
    return power_manager_proxy_.release();
  }

  PowerManagerProxyDelegate* delegate() const { return delegate_; }
  // Can not guarantee that the returned object is alive.
  MockPowerManagerProxy* power_manager_proxy() const {
    return power_manager_proxy_raw_;
  }

 private:
  PowerManagerProxyDelegate* delegate_;
  MockPowerManagerProxy* const power_manager_proxy_raw_;
  std::unique_ptr<MockPowerManagerProxy> power_manager_proxy_;
};

}  // namespace

class PowerManagerTest : public Test {
 public:
  static const char kDescription[];
  static const char kDarkDescription[];
  static const char kPowerManagerDefaultOwner[];
  static const int kSuspendId1 = 123;
  static const int kSuspendId2 = 456;
  static const int kDelayId = 4;
  static const int kDelayId2 = 5;

  PowerManagerTest()
      : kTimeout(base::TimeDelta::FromSeconds(3)),
        power_manager_(&dispatcher_, &control_),
        power_manager_proxy_(control_.power_manager_proxy()),
        delegate_(control_.delegate()) {
    suspend_imminent_callback_ =
        Bind(&PowerManagerTest::SuspendImminentAction, Unretained(this));
    suspend_done_callback_ =
        Bind(&PowerManagerTest::SuspendDoneAction, Unretained(this));
    dark_suspend_imminent_callback_ =
        Bind(&PowerManagerTest::DarkSuspendImminentAction, Unretained(this));
  }

  MOCK_METHOD0(SuspendImminentAction, void());
  MOCK_METHOD0(SuspendDoneAction, void());
  MOCK_METHOD0(DarkSuspendImminentAction, void());

 protected:
  virtual void SetUp() {
    power_manager_.Start(kTimeout, suspend_imminent_callback_,
                         suspend_done_callback_,
                         dark_suspend_imminent_callback_);
  }

  virtual void TearDown() {
    power_manager_.Stop();
  }

  void AddProxyExpectationForRegisterSuspendDelay(int delay_id,
                                                  bool return_value) {
    EXPECT_CALL(*power_manager_proxy_,
                RegisterSuspendDelay(kTimeout, kDescription, _))
        .WillOnce(DoAll(SetArgumentPointee<2>(delay_id), Return(return_value)));
  }

  void AddProxyExpectationForUnregisterSuspendDelay(int delay_id,
                                                    bool return_value) {
    EXPECT_CALL(*power_manager_proxy_, UnregisterSuspendDelay(delay_id))
        .WillOnce(Return(return_value));
  }

  void AddProxyExpectationForReportSuspendReadiness(int delay_id,
                                                    int suspend_id,
                                                    bool return_value) {
    EXPECT_CALL(*power_manager_proxy_,
                ReportSuspendReadiness(delay_id, suspend_id))
        .WillOnce(Return(return_value));
  }

  void AddProxyExpectationForRecordDarkResumeWakeReason(
      const string& wake_reason, bool return_value) {
    EXPECT_CALL(*power_manager_proxy_, RecordDarkResumeWakeReason(wake_reason))
        .WillOnce(Return(return_value));
  }

  void AddProxyExpectationForRegisterDarkSuspendDelay(int delay_id,
                                                      bool return_value) {
    EXPECT_CALL(*power_manager_proxy_,
                RegisterDarkSuspendDelay(kTimeout, kDarkDescription, _))
        .WillOnce(DoAll(SetArgumentPointee<2>(delay_id), Return(return_value)));
  }

  void AddProxyExpectationForReportDarkSuspendReadiness(int delay_id,
                                                        int suspend_id,
                                                        bool return_value) {
    EXPECT_CALL(*power_manager_proxy_,
                ReportDarkSuspendReadiness(delay_id, suspend_id))
        .WillOnce(Return(return_value));
  }

  void AddProxyExpectationForUnregisterDarkSuspendDelay(int delay_id,
                                                        bool return_value) {
    EXPECT_CALL(*power_manager_proxy_, UnregisterDarkSuspendDelay(delay_id))
        .WillOnce(Return(return_value));
  }

  void RegisterSuspendDelays() {
    AddProxyExpectationForRegisterSuspendDelay(kDelayId, true);
    AddProxyExpectationForRegisterDarkSuspendDelay(kDelayId, true);
    OnPowerManagerAppeared();
    Mock::VerifyAndClearExpectations(power_manager_proxy_);
  }

  void OnSuspendImminent(int suspend_id) {
    control_.delegate()->OnSuspendImminent(suspend_id);
    EXPECT_TRUE(power_manager_.suspending());
  }

  void OnSuspendDone(int suspend_id) {
    control_.delegate()->OnSuspendDone(suspend_id);
    EXPECT_FALSE(power_manager_.suspending());
  }

  void OnDarkSuspendImminent(int suspend_id) {
    control_.delegate()->OnDarkSuspendImminent(suspend_id);
  }

  void OnPowerManagerAppeared() {
    power_manager_.OnPowerManagerAppeared();
  }

  void OnPowerManagerVanished() {
    power_manager_.OnPowerManagerVanished();
  }

  // This is non-static since it's a non-POD type.
  const base::TimeDelta kTimeout;

  MockEventDispatcher dispatcher_;
  FakeControl control_;
  PowerManager power_manager_;
  MockPowerManagerProxy* const power_manager_proxy_;
  PowerManagerProxyDelegate* const delegate_;
  PowerManager::SuspendImminentCallback suspend_imminent_callback_;
  PowerManager::SuspendDoneCallback suspend_done_callback_;
  PowerManager::DarkSuspendImminentCallback dark_suspend_imminent_callback_;
};

const char PowerManagerTest::kDescription[] = "shill";
const char PowerManagerTest::kDarkDescription[] = "shill";
const char PowerManagerTest::kPowerManagerDefaultOwner[] =
    "PowerManagerDefaultOwner";

TEST_F(PowerManagerTest, SuspendingState) {
  const int kSuspendId = 3;
  EXPECT_FALSE(power_manager_.suspending());
  OnSuspendImminent(kSuspendId);
  EXPECT_TRUE(power_manager_.suspending());
  OnSuspendDone(kSuspendId);
  EXPECT_FALSE(power_manager_.suspending());
}

TEST_F(PowerManagerTest, RegisterSuspendDelayFailure) {
  AddProxyExpectationForRegisterSuspendDelay(kDelayId, false);
  OnPowerManagerAppeared();
  Mock::VerifyAndClearExpectations(power_manager_proxy_);

  // Outstanding shill callbacks should still be invoked.
  // - suspend_done_callback: If powerd died in the middle of a suspend
  //   we want to wake shill up with suspend_done_action, so this callback
  //   should be invoked anyway.
  //   See PowerManagerTest::PowerManagerDiedInSuspend and
  //   PowerManagerTest::PowerManagerReappearedInSuspend.
  EXPECT_CALL(*this, SuspendDoneAction());
  // - suspend_imminent_callback: The only case this can happen is if this
  //   callback was put on the queue, and then powerd reappeared, but we failed
  //   to registered a suspend delay with it.
  //   It is safe to go through the suspend_imminent -> timeout -> suspend_done
  //   path in this black swan case.
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId1);
  OnSuspendDone(kSuspendId1);
  Mock::VerifyAndClearExpectations(this);
}

TEST_F(PowerManagerTest, RegisterDarkSuspendDelayFailure) {
  AddProxyExpectationForRegisterDarkSuspendDelay(kDelayId, false);
  OnPowerManagerAppeared();
  Mock::VerifyAndClearExpectations(power_manager_proxy_);

  // Outstanding dark suspend imminent signal should be ignored, since we
  // probably won't have time to cleanly do dark resume actions. Might as well
  // ignore the signal.
  EXPECT_CALL(*this, DarkSuspendImminentAction()).Times(0);
  OnDarkSuspendImminent(kSuspendId1);
}

TEST_F(PowerManagerTest, ReportSuspendReadinessFailure) {
  RegisterSuspendDelays();
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId1);
  AddProxyExpectationForReportSuspendReadiness(kDelayId, kSuspendId1, false);
  EXPECT_FALSE(power_manager_.ReportSuspendReadiness());
}

TEST_F(PowerManagerTest, RecordDarkResumeWakeReasonFailure) {
  const string kWakeReason = "WiFi.Disconnect";
  RegisterSuspendDelays();
  EXPECT_CALL(*this, DarkSuspendImminentAction());
  OnDarkSuspendImminent(kSuspendId1);
  AddProxyExpectationForRecordDarkResumeWakeReason(kWakeReason, false);
  EXPECT_FALSE(power_manager_.RecordDarkResumeWakeReason(kWakeReason));
}

TEST_F(PowerManagerTest, RecordDarkResumeWakeReasonSuccess) {
  const string kWakeReason = "WiFi.Disconnect";
  RegisterSuspendDelays();
  EXPECT_CALL(*this, DarkSuspendImminentAction());
  OnDarkSuspendImminent(kSuspendId1);
  AddProxyExpectationForRecordDarkResumeWakeReason(kWakeReason, true);
  EXPECT_TRUE(power_manager_.RecordDarkResumeWakeReason(kWakeReason));
}

TEST_F(PowerManagerTest, ReportDarkSuspendReadinessFailure) {
  RegisterSuspendDelays();
  EXPECT_CALL(*this, DarkSuspendImminentAction());
  OnDarkSuspendImminent(kSuspendId1);
  AddProxyExpectationForReportDarkSuspendReadiness(kDelayId, kSuspendId1,
                                                   false);
  EXPECT_FALSE(power_manager_.ReportDarkSuspendReadiness());
}

TEST_F(PowerManagerTest, ReportSuspendReadinessFailsOutsideSuspend) {
  RegisterSuspendDelays();
  EXPECT_CALL(*power_manager_proxy_, ReportSuspendReadiness(_, _)).Times(0);
  EXPECT_FALSE(power_manager_.ReportSuspendReadiness());
}

TEST_F(PowerManagerTest, ReportSuspendReadinessSynchronous) {
  // Verifies that a synchronous ReportSuspendReadiness call by shill on a
  // SuspendImminent callback is routed back to powerd.
  RegisterSuspendDelays();
  EXPECT_CALL(*power_manager_proxy_, ReportSuspendReadiness(_, _))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, SuspendImminentAction())
      .WillOnce(IgnoreResult(InvokeWithoutArgs(
          &power_manager_, &PowerManager::ReportSuspendReadiness)));
  OnSuspendImminent(kSuspendId1);
}

TEST_F(PowerManagerTest, ReportDarkSuspendReadinessSynchronous) {
  // Verifies that a synchronous ReportDarkSuspendReadiness call by shill on a
  // DarkSuspendImminent callback is routed back to powerd.
  RegisterSuspendDelays();
  EXPECT_CALL(*power_manager_proxy_, ReportDarkSuspendReadiness(_, _))
      .WillOnce(Return(true));
  EXPECT_CALL(*this, DarkSuspendImminentAction())
      .WillOnce(IgnoreResult(InvokeWithoutArgs(
          &power_manager_, &PowerManager::ReportDarkSuspendReadiness)));
  OnDarkSuspendImminent(kSuspendId1);
}

TEST_F(PowerManagerTest, Stop) {
  RegisterSuspendDelays();
  AddProxyExpectationForUnregisterSuspendDelay(kDelayId, true);
  AddProxyExpectationForUnregisterDarkSuspendDelay(kDelayId, true);
  power_manager_.Stop();
}

TEST_F(PowerManagerTest, StopFailure) {
  RegisterSuspendDelays();

  AddProxyExpectationForUnregisterSuspendDelay(kDelayId, false);
  power_manager_.Stop();
  Mock::VerifyAndClearExpectations(power_manager_proxy_);

  // As a result, callbacks should still be invoked.
  EXPECT_CALL(*this, SuspendImminentAction());
  EXPECT_CALL(*this, SuspendDoneAction());
  OnSuspendImminent(kSuspendId1);
  OnSuspendDone(kSuspendId1);
}

TEST_F(PowerManagerTest, OnPowerManagerReappeared) {
  RegisterSuspendDelays();

  // Check that we re-register suspend delay on powerd restart.
  AddProxyExpectationForRegisterSuspendDelay(kDelayId2, true);
  AddProxyExpectationForRegisterDarkSuspendDelay(kDelayId2, true);
  OnPowerManagerVanished();
  OnPowerManagerAppeared();
  Mock::VerifyAndClearExpectations(power_manager_proxy_);

  // Check that a |ReportSuspendReadiness| message is sent with the new delay
  // id.
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId1);
  AddProxyExpectationForReportSuspendReadiness(kDelayId2, kSuspendId1, true);
  EXPECT_TRUE(power_manager_.ReportSuspendReadiness());
  Mock::VerifyAndClearExpectations(power_manager_proxy_);

  // Check that a |ReportDarkSuspendReadiness| message is sent with the new
  // delay id.
  EXPECT_CALL(*this, DarkSuspendImminentAction());
  OnDarkSuspendImminent(kSuspendId1);
  AddProxyExpectationForReportDarkSuspendReadiness(kDelayId2, kSuspendId1,
                                                   true);
  EXPECT_TRUE(power_manager_.ReportDarkSuspendReadiness());
}

TEST_F(PowerManagerTest, PowerManagerDiedInSuspend) {
  RegisterSuspendDelays();
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId1);
  Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(*this, SuspendDoneAction());
  OnPowerManagerVanished();
  EXPECT_FALSE(power_manager_.suspending());
}

TEST_F(PowerManagerTest, PowerManagerReappearedInSuspend) {
  RegisterSuspendDelays();
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId1);
  Mock::VerifyAndClearExpectations(this);

  AddProxyExpectationForRegisterSuspendDelay(kDelayId2, true);
  AddProxyExpectationForRegisterDarkSuspendDelay(kDelayId2, true);
  EXPECT_CALL(*this, SuspendDoneAction());
  OnPowerManagerVanished();
  OnPowerManagerAppeared();
  EXPECT_FALSE(power_manager_.suspending());
  Mock::VerifyAndClearExpectations(this);

  // Let's check a normal suspend request after the fact.
  EXPECT_CALL(*this, SuspendImminentAction());
  OnSuspendImminent(kSuspendId2);
}

}  // namespace shill
