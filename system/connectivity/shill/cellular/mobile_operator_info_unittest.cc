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

#include "shill/cellular/mobile_operator_info.h"

#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <vector>

#include <base/files/file_util.h>
#include <base/macros.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/cellular/mobile_operator_info_impl.h"
#include "shill/logging.h"
#include "shill/test_event_dispatcher.h"

// These files contain binary protobuf definitions used by the following tests
// inside the namespace ::mobile_operator_db
#define IN_MOBILE_OPERATOR_INFO_UNITTEST_CC
#include "shill/mobile_operator_db/test_protos/data_test.h"
#include "shill/mobile_operator_db/test_protos/init_test_empty_db_init.h"
#include "shill/mobile_operator_db/test_protos/init_test_multiple_db_init_1.h"
#include "shill/mobile_operator_db/test_protos/init_test_multiple_db_init_2.h"
#include "shill/mobile_operator_db/test_protos/init_test_successful_init.h"
#include "shill/mobile_operator_db/test_protos/main_test.h"
#undef IN_MOBILE_OPERATOR_INFO_UNITTEST_CC

using base::FilePath;
using shill::mobile_operator_db::MobileOperatorDB;
using std::map;
using std::ofstream;
using std::set;
using std::string;
using std::vector;
using testing::Mock;
using testing::Test;
using testing::Values;
using testing::WithParamInterface;

// The tests run from the fixture |MobileOperatorInfoMainTest| and
// |MobileOperatorDataTest| can be run in two modes:
//   - strict event checking: We check that an event is raised for each update
//     to the state of the object.
//   - non-strict event checking: We check that a single event is raised as a
//     result of many updates to the object.
// The first case corresponds to a very aggressive event loop, that dispatches
// events as soon as they are posted; the second one corresponds to an
// over-crowded event loop that only dispatches events just before we verify
// that events were raised.
//
// We use ::testing::WithParamInterface to templatize the test fixtures to do
// string/non-strict event checking. When writing test cases using these
// fixtures, use the |Update*|, |ExpectEventCount|, |VerifyEventCount| functions
// provided by the fixture, and write the test as if event checking is strict.
//
// For |MobileOperatorObserverTest|, only the strict event checking case makes
// sense, so we only instantiate that.
namespace shill {

namespace {

enum EventCheckingPolicy {
  kEventCheckingPolicyStrict,
  kEventCheckingPolicyNonStrict
};

}  // namespace

class MockMobileOperatorInfoObserver : public MobileOperatorInfo::Observer {
 public:
  MockMobileOperatorInfoObserver() {}
  virtual ~MockMobileOperatorInfoObserver() {}

  MOCK_METHOD0(OnOperatorChanged, void());
};

class MobileOperatorInfoInitTest : public Test {
 public:
  MobileOperatorInfoInitTest()
      : operator_info_(new MobileOperatorInfo(&dispatcher_, "Operator")),
        operator_info_impl_(operator_info_->impl()) {}

  void TearDown() override {
    for (const auto& tmp_db_path : tmp_db_paths_) {
      base::DeleteFile(tmp_db_path, false);
    }
  }

 protected:
  void AddDatabase(const unsigned char database_data[], size_t num_elems) {
    FilePath tmp_db_path;
    CHECK(base::CreateTemporaryFile(&tmp_db_path));
    tmp_db_paths_.push_back(tmp_db_path);

    ofstream tmp_db(tmp_db_path.value(), ofstream::binary);
    for (size_t i = 0; i < num_elems; ++i) {
      tmp_db << database_data[i];
    }
    tmp_db.close();
    operator_info_->AddDatabasePath(tmp_db_path);
  }

  void AssertDatabaseEmpty() {
    EXPECT_EQ(0, operator_info_impl_->database()->mno_size());
    EXPECT_EQ(0, operator_info_impl_->database()->imvno_size());
  }

  const MobileOperatorDB* GetDatabase() {
    return operator_info_impl_->database();
  }

  EventDispatcherForTest dispatcher_;
  vector<FilePath> tmp_db_paths_;
  std::unique_ptr<MobileOperatorInfo> operator_info_;
  // Owned by |operator_info_| and tied to its life cycle.
  MobileOperatorInfoImpl* operator_info_impl_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoInitTest);
};

TEST_F(MobileOperatorInfoInitTest, FailedInitNoPath) {
  // - Initialize object with no database paths set
  // - Verify that initialization fails.
  operator_info_->ClearDatabasePaths();
  EXPECT_FALSE(operator_info_->Init());
  AssertDatabaseEmpty();
}

TEST_F(MobileOperatorInfoInitTest, FailedInitBadPath) {
  // - Initialize object with non-existent path.
  // - Verify that initialization fails.
  const FilePath database_path("nonexistent.pbf");
  operator_info_->ClearDatabasePaths();
  operator_info_->AddDatabasePath(database_path);
  EXPECT_FALSE(operator_info_->Init());
  AssertDatabaseEmpty();
}

TEST_F(MobileOperatorInfoInitTest, FailedInitBadDatabase) {
  // - Initialize object with malformed database.
  // - Verify that initialization fails.
  // TODO(pprabhu): It's hard to get a malformed database in binary format.
}

TEST_F(MobileOperatorInfoInitTest, EmptyDBInit) {
  // - Initialize the object with a database file that is empty.
  // - Verify that initialization succeeds, and that the database is empty.
  operator_info_->ClearDatabasePaths();
  // Can't use arraysize on empty array.
  AddDatabase(mobile_operator_db::init_test_empty_db_init, 0);
  EXPECT_TRUE(operator_info_->Init());
  AssertDatabaseEmpty();
}

TEST_F(MobileOperatorInfoInitTest, SuccessfulInit) {
  operator_info_->ClearDatabasePaths();
  AddDatabase(mobile_operator_db::init_test_successful_init,
              arraysize(mobile_operator_db::init_test_successful_init));
  EXPECT_TRUE(operator_info_->Init());
  EXPECT_GT(GetDatabase()->mno_size(), 0);
  EXPECT_GT(GetDatabase()->imvno_size(), 0);
}

TEST_F(MobileOperatorInfoInitTest, MultipleDBInit) {
  // - Initialize the object with two database files.
  // - Verify that intialization succeeds, and both databases are loaded.
  operator_info_->ClearDatabasePaths();
  AddDatabase(mobile_operator_db::init_test_multiple_db_init_1,
              arraysize(mobile_operator_db::init_test_multiple_db_init_1));
  AddDatabase(mobile_operator_db::init_test_multiple_db_init_2,
              arraysize(mobile_operator_db::init_test_multiple_db_init_2));
  operator_info_->Init();
  EXPECT_GT(GetDatabase()->mno_size(), 0);
  EXPECT_GT(GetDatabase()->imvno_size(), 0);
}

TEST_F(MobileOperatorInfoInitTest, InitWithObserver) {
  // - Add an Observer.
  // - Initialize the object with empty database file.
  // - Verify innitialization succeeds.
  MockMobileOperatorInfoObserver dumb_observer;

  operator_info_->ClearDatabasePaths();
  // Can't use arraysize with empty array.
  AddDatabase(mobile_operator_db::init_test_empty_db_init, 0);
  operator_info_->AddObserver(&dumb_observer);
  EXPECT_TRUE(operator_info_->Init());
}

class MobileOperatorInfoMainTest
    : public MobileOperatorInfoInitTest,
      public WithParamInterface<EventCheckingPolicy> {
 public:
  MobileOperatorInfoMainTest()
      : MobileOperatorInfoInitTest(),
        event_checking_policy_(GetParam()) {}

  virtual void SetUp() {
    operator_info_->ClearDatabasePaths();
    AddDatabase(mobile_operator_db::main_test,
                arraysize(mobile_operator_db::main_test));
    operator_info_->Init();
    operator_info_->AddObserver(&observer_);
  }

 protected:
  // ///////////////////////////////////////////////////////////////////////////
  // Helper functions.
  void VerifyMNOWithUUID(const string& uuid) {
    EXPECT_TRUE(operator_info_->IsMobileNetworkOperatorKnown());
    EXPECT_FALSE(operator_info_->IsMobileVirtualNetworkOperatorKnown());
    EXPECT_EQ(uuid, operator_info_->uuid());
  }

  void VerifyMVNOWithUUID(const string& uuid) {
    EXPECT_TRUE(operator_info_->IsMobileNetworkOperatorKnown());
    EXPECT_TRUE(operator_info_->IsMobileVirtualNetworkOperatorKnown());
    EXPECT_EQ(uuid, operator_info_->uuid());
  }

  void VerifyNoMatch() {
    EXPECT_FALSE(operator_info_->IsMobileNetworkOperatorKnown());
    EXPECT_FALSE(operator_info_->IsMobileVirtualNetworkOperatorKnown());
    EXPECT_EQ("", operator_info_->uuid());
  }

  void ExpectEventCount(int count) {
    // In case we're running in the non-strict event checking mode, we only
    // expect one overall event to be raised for all the updates.
    if (event_checking_policy_ == kEventCheckingPolicyNonStrict) {
      count = (count > 0) ? 1 : 0;
    }
    EXPECT_CALL(observer_, OnOperatorChanged()).Times(count);
  }

  void VerifyEventCount() {
    dispatcher_.DispatchPendingEvents();
    Mock::VerifyAndClearExpectations(&observer_);
  }

  void ResetOperatorInfo() {
    operator_info_->Reset();
    // Eat up any events caused by |Reset|.
    dispatcher_.DispatchPendingEvents();
    VerifyNoMatch();
  }

  // Use these wrappers to send updates to |operator_info_|. These wrappers
  // optionally run the dispatcher if we want strict checking of the number of
  // events raised.
  void UpdateMCCMNC(const std::string& mccmnc) {
    operator_info_->UpdateMCCMNC(mccmnc);
    DispatchPendingEventsIfStrict();
  }

  void UpdateSID(const std::string& sid) {
    operator_info_->UpdateSID(sid);
    DispatchPendingEventsIfStrict();
  }

  void UpdateIMSI(const std::string& imsi) {
    operator_info_->UpdateIMSI(imsi);
    DispatchPendingEventsIfStrict();
  }

  void UpdateICCID(const std::string& iccid) {
    operator_info_->UpdateICCID(iccid);
    DispatchPendingEventsIfStrict();
  }

  void UpdateNID(const std::string& nid) {
    operator_info_->UpdateNID(nid);
    DispatchPendingEventsIfStrict();
  }

  void UpdateOperatorName(const std::string& operator_name) {
    operator_info_->UpdateOperatorName(operator_name);
    DispatchPendingEventsIfStrict();
  }

  void UpdateOnlinePortal(const std::string& url,
                          const std::string& method,
                          const std::string& post_data) {
    operator_info_->UpdateOnlinePortal(url, method, post_data);
    DispatchPendingEventsIfStrict();
  }

  void DispatchPendingEventsIfStrict() {
    if (event_checking_policy_ == kEventCheckingPolicyStrict) {
      dispatcher_.DispatchPendingEvents();
    }
  }

  // ///////////////////////////////////////////////////////////////////////////
  // Data.
  MockMobileOperatorInfoObserver observer_;
  const EventCheckingPolicy event_checking_policy_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoMainTest);
};

TEST_P(MobileOperatorInfoMainTest, InitialConditions) {
  // - Initialize a new object.
  // - Verify that all initial values of properties are reasonable.
  EXPECT_FALSE(operator_info_->IsMobileNetworkOperatorKnown());
  EXPECT_FALSE(operator_info_->IsMobileVirtualNetworkOperatorKnown());
  EXPECT_TRUE(operator_info_->uuid().empty());
  EXPECT_TRUE(operator_info_->operator_name().empty());
  EXPECT_TRUE(operator_info_->country().empty());
  EXPECT_TRUE(operator_info_->mccmnc().empty());
  EXPECT_TRUE(operator_info_->sid().empty());
  EXPECT_TRUE(operator_info_->nid().empty());
  EXPECT_TRUE(operator_info_->mccmnc_list().empty());
  EXPECT_TRUE(operator_info_->sid_list().empty());
  EXPECT_TRUE(operator_info_->operator_name_list().empty());
  EXPECT_TRUE(operator_info_->apn_list().empty());
  EXPECT_TRUE(operator_info_->olp_list().empty());
  EXPECT_TRUE(operator_info_->activation_code().empty());
  EXPECT_FALSE(operator_info_->requires_roaming());
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNC) {
  // message: Has an MNO with no MVNO.
  // match by: MCCMNC.
  // verify: Observer event, uuid.

  ExpectEventCount(0);
  UpdateMCCMNC("101999");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("101001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid101");

  ExpectEventCount(1);
  UpdateMCCMNC("101999");
  VerifyEventCount();
  VerifyNoMatch();
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCMultipleMCCMNCOptions) {
  // message: Has an MNO with no MCCMNC.
  // match by: One of the MCCMNCs of the multiple ones in the MNO.
  // verify: Observer event, uuid.
  ExpectEventCount(1);
  UpdateMCCMNC("102002");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid102");
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCMultipleMNOOptions) {
  // message: Two messages with the same MCCMNC.
  // match by: Both MNOs matched, one is earmarked.
  // verify: The earmarked MNO is picked.
  ExpectEventCount(1);
  UpdateMCCMNC("124001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid124002");
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorName) {
  // message: Has an MNO with no MVNO.
  // match by: OperatorName.
  // verify: Observer event, uuid.
  ExpectEventCount(0);
  UpdateOperatorName("name103999");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateOperatorName("name103");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid103");

  ExpectEventCount(1);
  UpdateOperatorName("name103999");  // No match.
  VerifyEventCount();
  VerifyNoMatch();
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorNameMultipleMNOOptions) {
  // message: Two messages with the same operator name.
  // match by: Both MNOs matched, one is earmarked.
  // verify: The earmarked MNO is picked.
  ExpectEventCount(1);
  UpdateOperatorName("name125001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid125002");
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorNameAggressiveMatch) {
  // These network operators match by name but only after normalizing the names.
  // Both the name from the database and the name provided to
  // |UpdateOperatoName| must be normalized for this test to pass.
  ExpectEventCount(1);
  UpdateOperatorName("name126001 casedoesnotmatch");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid126001");

  ResetOperatorInfo();
  ExpectEventCount(1);
  UpdateOperatorName("name126002 CaseStillDoesNotMatch");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid126002");

  ResetOperatorInfo();
  ExpectEventCount(1);
  UpdateOperatorName("name126003GiveMeMoreSpace");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid126003");

  ResetOperatorInfo();
  ExpectEventCount(1);
  UpdateOperatorName("name126004  Too  Much   Air Here");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid126004");

  ResetOperatorInfo();
  ExpectEventCount(1);
  UpdateOperatorName("näméwithNon-Äσ¢ii");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid126005");
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorNameWithLang) {
  // message: Has an MNO with no MVNO.
  // match by: OperatorName.
  // verify: Observer event, fields.
  ExpectEventCount(1);
  UpdateOperatorName("name105");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid105");
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorNameMultipleNameOptions) {
  // message: Has an MNO with no MVNO.
  // match by: OperatorName, one of the multiple present in the MNO.
  // verify: Observer event, fields.
  ExpectEventCount(1);
  UpdateOperatorName("name104002");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid104");
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCAndOperatorName) {
  // message: Has MNOs with no MVNO.
  // match by: MCCMNC finds two candidates (first one is chosen), Name narrows
  //           down to one.
  // verify: Observer event, fields.
  // This is merely a MCCMNC update.
  ExpectEventCount(1);
  UpdateMCCMNC("106001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid106001");

  ExpectEventCount(1);
  UpdateOperatorName("name106002");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid106002");

  ResetOperatorInfo();
  // Try updates in reverse order.
  ExpectEventCount(1);
  UpdateOperatorName("name106001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid106001");
}

TEST_P(MobileOperatorInfoMainTest, MNOByOperatorNameAndMCCMNC) {
  // message: Has MNOs with no MVNO.
  // match by: OperatorName finds two (first one is chosen), MCCMNC narrows down
  //           to one.
  // verify: Observer event, fields.
  // This is merely an OperatorName update.
  ExpectEventCount(1);
  UpdateOperatorName("name107");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid107001");

  ExpectEventCount(1);
  UpdateMCCMNC("107002");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid107002");

  ResetOperatorInfo();
  // Try updates in reverse order.
  ExpectEventCount(1);
  UpdateMCCMNC("107001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid107001");
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCOverridesOperatorName) {
  // message: Has MNOs with no MVNO.
  // match by: First MCCMNC finds one. Then, OperatorName matches another.
  // verify: MCCMNC match prevails. No change on OperatorName update.
  ExpectEventCount(1);
  UpdateMCCMNC("108001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid108001");

  // An event is sent for the updated OperatorName.
  ExpectEventCount(1);
  UpdateOperatorName("name108002");  // Does not match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid108001");
  // OperatorName from the database is given preference over the user supplied
  // one.
  EXPECT_EQ("name108001", operator_info_->operator_name());

  ResetOperatorInfo();
  // message: Same as above.
  // match by: First OperatorName finds one, then MCCMNC overrides it.
  // verify: Two events, MCCMNC one overriding the OperatorName one.
  ExpectEventCount(1);
  UpdateOperatorName("name108001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid108001");

  ExpectEventCount(1);
  UpdateMCCMNC("108002");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid108002");
  EXPECT_EQ("name108002", operator_info_->operator_name());

  // message: Same as above.
  // match by: First a *wrong* MCCMNC update, followed by the correct Name
  // update.
  // verify: No MNO, since MCCMNC is given precedence.
  ResetOperatorInfo();
  ExpectEventCount(0);
  UpdateMCCMNC("108999");  // Does not match.
  UpdateOperatorName("name108001");
  VerifyEventCount();
  VerifyNoMatch();
}

TEST_P(MobileOperatorInfoMainTest, MNOByIMSI) {
  // message: Has MNO with no MVNO.
  // match by: MCCMNC part of IMSI of length 5 / 6.
  ExpectEventCount(0);
  UpdateIMSI("109");  // Too short.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(0);
  UpdateIMSI("109995432154321");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ResetOperatorInfo();
  // Short MCCMNC match.
  ExpectEventCount(1);
  UpdateIMSI("109015432154321");  // First 5 digits match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid10901");

  ResetOperatorInfo();
  // Long MCCMNC match.
  ExpectEventCount(1);
  UpdateIMSI("10900215432154321");  // First 6 digits match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid109002");
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCOverridesIMSI) {
  // message: Has MNOs with no MVNO.
  // match by: One matches MCCMNC, then one matches a different MCCMNC substring
  //    of IMSI
  // verify: Observer event for the first match, all fields. Second Update
  // ignored.
  ExpectEventCount(1);
  UpdateMCCMNC("110001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110001");

  // MNO remains unchanged on a mismatched IMSI update.
  ExpectEventCount(0);
  UpdateIMSI("1100025432154321");  // First 6 digits match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110001");

  // MNO remains uncnaged on an invalid IMSI update.
  ExpectEventCount(0);
  UpdateIMSI("1100035432154321");  // Prefix does not match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110001");

  ExpectEventCount(0);
  UpdateIMSI("110");  // Too small.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110001");

  ResetOperatorInfo();
  // Same as above, but this time, match with IMSI, followed by a contradictory
  // MCCMNC update. The second update should override the first one.
  ExpectEventCount(1);
  UpdateIMSI("1100025432154321");  // First 6 digits match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110002");

  ExpectEventCount(1);
  UpdateMCCMNC("110001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid110001");
}

TEST_P(MobileOperatorInfoMainTest, MNOUchangedBySecondaryUpdates) {
  // This test verifies that only some updates affect the MNO.
  // message: Has MNOs with no MVNO.
  // match by: First matches the MCCMNC. Later, MNOs with a different MCCMNC
  //    matchs the given SID, NID, ICCID.
  // verify: Only one Observer event, on the first MCCMNC match.
  ExpectEventCount(1);
  UpdateMCCMNC("111001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid111001");

  ExpectEventCount(1);  // NID change event.
  UpdateNID("111202");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid111001");
}

TEST_P(MobileOperatorInfoMainTest, MVNODefaultMatch) {
  // message: MNO with one MVNO (no filter).
  // match by: MNO matches by MCCMNC.
  // verify: Observer event for MVNO match. Uuid match the MVNO.
  // second update: ICCID.
  // verify: No observer event, match remains unchanged.
  ExpectEventCount(1);
  UpdateMCCMNC("112001");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid112002");

  ExpectEventCount(0);
  UpdateICCID("112002");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid112002");
}

TEST_P(MobileOperatorInfoMainTest, MVNONameMatch) {
  // message: MNO with one MVNO (name filter).
  // match by: MNO matches by MCCMNC,
  //           MVNO fails to match by fist name update,
  //           then MVNO matches by name.
  // verify: Two Observer events: MNO followed by MVNO.
  ExpectEventCount(1);
  UpdateMCCMNC("113001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid113001");

  ExpectEventCount(1);
  UpdateOperatorName("name113999");  // No match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid113001");
  // Name from the database is given preference.
  EXPECT_EQ("name113001", operator_info_->operator_name());

  ExpectEventCount(1);
  UpdateOperatorName("name113002");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid113002");
  EXPECT_EQ("name113002", operator_info_->operator_name());
}

TEST_P(MobileOperatorInfoMainTest, MVNONameMalformedRegexMatch) {
  // message: MNO with one MVNO (name filter with a malformed regex).
  // match by: MNO matches by MCCMNC.
  //           MVNO does not match
  ExpectEventCount(2);
  UpdateMCCMNC("114001");
  UpdateOperatorName("name[");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid114001");
}

TEST_P(MobileOperatorInfoMainTest, MVNONameSubexpressionRegexMatch) {
  // message: MNO with one MVNO (name filter with simple regex).
  // match by: MNO matches by MCCMNC.
  //           MVNO does not match with a name whose subexpression matches the
  //           regex.
  ExpectEventCount(2);  // One event for just the name update.
  UpdateMCCMNC("115001");
  UpdateOperatorName("name115_ExtraCrud");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid115001");

  ResetOperatorInfo();
  ExpectEventCount(2);  // One event for just the name update.
  UpdateMCCMNC("115001");
  UpdateOperatorName("ExtraCrud_name115");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid115001");

  ResetOperatorInfo();
  ExpectEventCount(2);  // One event for just the name update.
  UpdateMCCMNC("115001");
  UpdateOperatorName("ExtraCrud_name115_ExtraCrud");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid115001");

  ResetOperatorInfo();
  ExpectEventCount(2);  // One event for just the name update.
  UpdateMCCMNC("115001");
  UpdateOperatorName("name_ExtraCrud_115");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid115001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("115001");
  UpdateOperatorName("name115");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid115002");
}

TEST_P(MobileOperatorInfoMainTest, MVNONameRegexMatch) {
  // message: MNO with one MVNO (name filter with non-trivial regex).
  // match by: MNO matches by MCCMNC.
  //           MVNO fails to match several times with different strings.
  //           MVNO matches several times with different values.

  // Make sure we're not taking the regex literally!
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("name[a-zA-Z_]*116[0-9]{0,3}");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid116001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("name[a-zA-Z_]116[0-9]");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid116001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("nameb*1167");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid116001");

  // Success!
  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("name116");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid116002");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("nameSomeWord116");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid116002");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("116001");
  UpdateOperatorName("name116567");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid116002");
}

TEST_P(MobileOperatorInfoMainTest, MVNONameMatchMultipleFilters) {
  // message: MNO with one MVNO with two name filters.
  // match by: MNO matches by MCCMNC.
  //           MVNO first fails on the second filter alone.
  //           MVNO fails on the first filter alone.
  //           MVNO matches on both filters.
  ExpectEventCount(2);
  UpdateMCCMNC("117001");
  UpdateOperatorName("nameA_crud");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid117001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("117001");
  UpdateOperatorName("crud_nameB");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid117001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("117001");
  UpdateOperatorName("crud_crud");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid117001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("117001");
  UpdateOperatorName("nameA_nameB");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid117002");
}

TEST_P(MobileOperatorInfoMainTest, MVNOIMSIMatch) {
  // message: MNO with one MVNO (imsi filter).
  // match by: MNO matches by MCCMNC,
  //           MVNO fails to match by fist imsi update,
  //           then MVNO matches by imsi.
  // verify: Two Observer events: MNO followed by MVNO.
  ExpectEventCount(1);
  UpdateMCCMNC("118001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid118001");

  ExpectEventCount(0);
  UpdateIMSI("1180011234512345");  // No match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid118001");

  ExpectEventCount(1);
  UpdateIMSI("1180015432154321");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid118002");
}

TEST_P(MobileOperatorInfoMainTest, MVNOICCIDMatch) {
  // message: MNO with one MVNO (iccid filter).
  // match by: MNO matches by MCCMNC,
  //           MVNO fails to match by fist iccid update,
  //           then MVNO matches by iccid.
  // verify: Two Observer events: MNO followed by MVNO.
  ExpectEventCount(1);
  UpdateMCCMNC("119001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid119001");

  ExpectEventCount(0);
  UpdateICCID("119987654321");  // No match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid119001");

  ExpectEventCount(1);
  UpdateICCID("119123456789");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid119002");
}

TEST_P(MobileOperatorInfoMainTest, MVNOSIDMatch) {
  // message: MNO with one MVNO (sid filter).
  // match by: MNO matches by SID,
  //           MVNO fails to match by fist sid update,
  //           then MVNO matches by sid.
  // verify: Two Observer events: MNO followed by MVNO.
  ExpectEventCount(0);
  UpdateSID("120999");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateSID("120001");  // Only MNO matches.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid120001");
  EXPECT_EQ("120001", operator_info_->sid());

  ExpectEventCount(1);
  UpdateSID("120002");  // MVNO matches as well.
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid120002");
  EXPECT_EQ("120002", operator_info_->sid());
}

TEST_P(MobileOperatorInfoMainTest, MVNOAllMatch) {
  // message: MNO with following MVNOS:
  //   - one with no filter.
  //   - one with name filter.
  //   - one with imsi filter.
  //   - one with iccid filter.
  //   - one with name and iccid filter.
  // verify:
  //   - initial MCCMNC matches the default MVNO directly (not MNO)
  //   - match each of the MVNOs in turn.
  //   - give super set information that does not match any MVNO correctly,
  //     verify that the MNO matches.
  ExpectEventCount(1);
  UpdateMCCMNC("121001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid121001");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("121001");
  UpdateOperatorName("name121003");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid121003");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("121001");
  UpdateIMSI("1210045432154321");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid121004");

  ResetOperatorInfo();
  ExpectEventCount(2);
  UpdateMCCMNC("121001");
  UpdateICCID("121005123456789");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid121005");

  ResetOperatorInfo();
  ExpectEventCount(3);
  UpdateMCCMNC("121001");
  UpdateOperatorName("name121006");
  VerifyMNOWithUUID("uuid121001");
  UpdateICCID("121006123456789");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid121006");
}

TEST_P(MobileOperatorInfoMainTest, MVNOMatchAndMismatch) {
  // message: MNO with one MVNO with name filter.
  // match by: MNO matches by MCCMNC
  //           MVNO matches by name.
  //           Second name update causes the MVNO to not match again.
  ExpectEventCount(1);
  UpdateMCCMNC("113001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid113001");

  ExpectEventCount(1);
  UpdateOperatorName("name113002");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid113002");
  EXPECT_EQ("name113002", operator_info_->operator_name());

  ExpectEventCount(1);
  UpdateOperatorName("name113999");  // No match.
  VerifyEventCount();
  VerifyMNOWithUUID("uuid113001");
  // Name from database is given preference.
  EXPECT_EQ("name113001", operator_info_->operator_name());
}

TEST_P(MobileOperatorInfoMainTest, MVNOMatchAndReset) {
  // message: MVNO with name filter.
  // verify;
  //   - match MVNO by name.
  //   - Reset object, verify Observer event, and not match.
  //   - match MVNO by name again.
  ExpectEventCount(1);
  UpdateMCCMNC("113001");
  VerifyEventCount();
  ExpectEventCount(1);
  VerifyMNOWithUUID("uuid113001");
  UpdateOperatorName("name113002");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid113002");
  EXPECT_EQ("name113002", operator_info_->operator_name());

  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("113001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid113001");
  ExpectEventCount(1);
  UpdateOperatorName("name113002");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid113002");
  EXPECT_EQ("name113002", operator_info_->operator_name());
}

// Here, we rely on our knowledge about the implementation: The SID and MCCMNC
// updates follow the same code paths, and so we can get away with not testing
// all the scenarios we test above for MCCMNC. Instead, we only do basic testing
// to make sure that SID upates operator as MCCMNC updates do.
TEST_P(MobileOperatorInfoMainTest, MNOBySID) {
  // message: Has an MNO with no MVNO.
  // match by: SID.
  // verify: Observer event, uuid.

  ExpectEventCount(0);
  UpdateSID("1229");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateSID("1221");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid1221");

  ExpectEventCount(1);
  UpdateSID("1229");  // No Match.
  VerifyEventCount();
  VerifyNoMatch();
}

TEST_P(MobileOperatorInfoMainTest, MNOByMCCMNCAndSID) {
  // message: Has an MNO with no MVNO.
  // match by: SID / MCCMNC alternately.
  // verify: Observer event, uuid.

  ExpectEventCount(0);
  UpdateMCCMNC("123999");  // NO match.
  UpdateSID("1239");  // No match.
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("123001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid123001");

  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateSID("1232");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid1232");

  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("123001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid123001");
}

class MobileOperatorInfoDataTest : public MobileOperatorInfoMainTest {
 public:
  MobileOperatorInfoDataTest() : MobileOperatorInfoMainTest() {}

  // Same as MobileOperatorInfoMainTest, except that the database used is
  // different.
  virtual void SetUp() {
    operator_info_->ClearDatabasePaths();
    AddDatabase(mobile_operator_db::data_test,
                arraysize(mobile_operator_db::data_test));
    operator_info_->Init();
    operator_info_->AddObserver(&observer_);
  }

 protected:
  // This is a function that does a best effort verification of the information
  // that is obtained from the database by the MobileOperatorInfo object against
  // expectations stored in the form of data members in this class.
  // This is not a full proof check. In particular:
  //  - It is unspecified in some case which of the values from a list is
  //    exposed as a property. For example, at best, we can check that |sid| is
  //    non-empty.
  //  - It is not robust to "" as property values at times.
  void VerifyDatabaseData() {
    EXPECT_EQ(country_, operator_info_->country());
    EXPECT_EQ(requires_roaming_, operator_info_->requires_roaming());
    EXPECT_EQ(activation_code_, operator_info_->activation_code());

    EXPECT_EQ(mccmnc_list_.size(), operator_info_->mccmnc_list().size());
    set<string> mccmnc_set(operator_info_->mccmnc_list().begin(),
                           operator_info_->mccmnc_list().end());
    for (const auto& mccmnc : mccmnc_list_) {
      EXPECT_TRUE(mccmnc_set.find(mccmnc) != mccmnc_set.end());
    }
    if (mccmnc_list_.size() > 0) {
      // It is not specified which entry will be chosen, but mccmnc() must be
      // non empty.
      EXPECT_FALSE(operator_info_->mccmnc().empty());
    }

    VerifyNameListsMatch(operator_name_list_,
                         operator_info_->operator_name_list());

    // This comparison breaks if two apns have the same |apn| field.
    EXPECT_EQ(apn_list_.size(), operator_info_->apn_list().size());
    map<string, const MobileOperatorInfo::MobileAPN*> mobile_apns;
    for (const auto& apn_node : operator_info_->apn_list()) {
      mobile_apns[apn_node->apn] = apn_node;
    }
    for (const auto& apn_lhs : apn_list_) {
      ASSERT_TRUE(mobile_apns.find(apn_lhs->apn) != mobile_apns.end());
      const auto& apn_rhs = mobile_apns[apn_lhs->apn];
      // Only comparing apn, name, username, password.
      EXPECT_EQ(apn_lhs->apn, apn_rhs->apn);
      EXPECT_EQ(apn_lhs->username, apn_rhs->username);
      EXPECT_EQ(apn_lhs->password, apn_rhs->password);
      VerifyNameListsMatch(apn_lhs->operator_name_list,
                           apn_rhs->operator_name_list);
    }

    EXPECT_EQ(olp_list_.size(), operator_info_->olp_list().size());
    // This comparison breaks if two OLPs have the same |url|.
    map<string, MobileOperatorInfo::OnlinePortal> olps;
    for (const auto& olp : operator_info_->olp_list()) {
      olps[olp.url] = olp;
    }
    for (const auto& olp : olp_list_) {
      ASSERT_TRUE(olps.find(olp.url) != olps.end());
      const auto& olp_rhs = olps[olp.url];
      EXPECT_EQ(olp.method, olp_rhs.method);
      EXPECT_EQ(olp.post_data, olp_rhs.post_data);
    }

    EXPECT_EQ(sid_list_.size(), operator_info_->sid_list().size());
    set<string> sid_set(operator_info_->sid_list().begin(),
                        operator_info_->sid_list().end());
    for (const auto& sid : sid_list_) {
      EXPECT_TRUE(sid_set.find(sid) != sid_set.end());
    }
    if (sid_list_.size() > 0) {
      // It is not specified which entry will be chosen, but |sid()| must be
      // non-empty.
      EXPECT_FALSE(operator_info_->sid().empty());
    }
  }

  // This function does some extra checks for the user data that can not be done
  // when data is obtained from the database.
  void VerifyUserData() {
    EXPECT_EQ(sid_, operator_info_->sid());
  }

  void VerifyNameListsMatch(
      const vector<MobileOperatorInfo::LocalizedName>& operator_name_list_lhs,
      const vector<MobileOperatorInfo::LocalizedName>& operator_name_list_rhs) {
    // This comparison breaks if two localized names have the same |name|.
    map<string, MobileOperatorInfo::LocalizedName> localized_names;
    for (const auto& localized_name : operator_name_list_rhs) {
      localized_names[localized_name.name] = localized_name;
    }
    for (const auto& localized_name : operator_name_list_lhs) {
      EXPECT_TRUE(localized_names.find(localized_name.name) !=
                  localized_names.end());
      EXPECT_EQ(localized_name.language,
                localized_names[localized_name.name].language);
    }
  }

  // Use this function to pre-popluate all the data members of this object with
  // values matching the MNO for the database in |data_test.prototxt|.
  void PopulateMNOData() {
    country_ = "us";
    requires_roaming_ = true;
    activation_code_ = "open sesame";

    mccmnc_list_.clear();
    mccmnc_list_.push_back("200001");
    mccmnc_list_.push_back("200002");
    mccmnc_list_.push_back("200003");

    operator_name_list_.clear();
    operator_name_list_.push_back({"name200001", "en"});
    operator_name_list_.push_back({"name200002", ""});

    apn_list_.clear();
    MobileOperatorInfo::MobileAPN* apn;
    apn = new MobileOperatorInfo::MobileAPN();
    apn->apn = "test@test.com";
    apn->username = "testuser";
    apn->password = "is_public_boohoohoo";
    apn->operator_name_list.push_back({"name200003", "hi"});
    apn_list_.push_back(apn);  // Takes ownership.

    olp_list_.clear();
    olp_list_.push_back({"some@random.com", "POST", "random_data"});

    sid_list_.clear();
    sid_list_.push_back("200123");
    sid_list_.push_back("200234");
    sid_list_.push_back("200345");
  }

  // Use this function to pre-populate all the data members of this object with
  // values matching the MVNO for the database in |data_test.prototext|.
  void PopulateMVNOData() {
    country_ = "ca";
    requires_roaming_ = false;
    activation_code_ = "khul ja sim sim";

    mccmnc_list_.clear();
    mccmnc_list_.push_back("200001");
    mccmnc_list_.push_back("200102");

    operator_name_list_.clear();
    operator_name_list_.push_back({"name200101", "en"});
    operator_name_list_.push_back({"name200102", ""});

    apn_list_.clear();
    MobileOperatorInfo::MobileAPN* apn;
    apn = new MobileOperatorInfo::MobileAPN();
    apn->apn = "test2@test.com";
    apn->username = "testuser2";
    apn->password = "is_public_boohoohoo_too";
    apn_list_.push_back(apn);  // Takes ownership.

    olp_list_.clear();
    olp_list_.push_back({"someother@random.com", "GET", ""});

    sid_list_.clear();
    sid_list_.push_back("200345");
  }

  // Data to be verified against the database.
  string country_;
  bool requires_roaming_;
  string activation_code_;
  vector<string> mccmnc_list_;
  vector<MobileOperatorInfo::LocalizedName> operator_name_list_;
  ScopedVector<MobileOperatorInfo::MobileAPN> apn_list_;
  vector<MobileOperatorInfo::OnlinePortal> olp_list_;
  vector<string> sid_list_;

  // Extra data to be verified only against user updates.
  string sid_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoDataTest);
};


TEST_P(MobileOperatorInfoDataTest, MNODetailedInformation) {
  // message: MNO with all the information filled in.
  // match by: MNO matches by MCCMNC
  // verify: All information is correctly loaded.
  ExpectEventCount(1);
  UpdateMCCMNC("200001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid200001");

  PopulateMNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, MVNOInheritsInformation) {
  // message: MVNO with name filter.
  // verify: All the missing fields are carried over to the MVNO from MNO.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200201");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200201");

  PopulateMNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, MVNOOverridesInformation) {
  // match by: MNO matches by MCCMNC, MVNO by name.
  // verify: All information is correctly loaded.
  //         The MVNO in this case overrides the information provided by MNO.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");

  PopulateMVNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, NoUpdatesBeforeMNOMatch) {
  // message: MVNO.
  // - do not match MNO with mccmnc/name
  // - on different updates, verify no events.
  ExpectEventCount(0);
  UpdateMCCMNC("200999");  // No match.
  UpdateOperatorName("name200001");  // matches MNO
  UpdateOperatorName("name200101");  // matches MVNO filter.
  UpdateSID("200999");  // No match.
  VerifyEventCount();
  VerifyNoMatch();
}

TEST_P(MobileOperatorInfoDataTest, UserUpdatesOverrideMVNO) {
  // - match MVNO.
  // - send updates to properties and verify events are raised and values of
  //   updated properties override the ones provided by the database.
  string imsi {"2009991234512345"};
  string iccid {"200999123456789"};
  string olp_url {"url@url.com"};
  string olp_method {"POST"};
  string olp_post_data {"data"};

  // Determine MVNO.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");

  // Send updates.
  ExpectEventCount(1);
  UpdateOnlinePortal(olp_url, olp_method, olp_post_data);
  UpdateIMSI(imsi);
  // No event raised because imsi is not exposed.
  UpdateICCID(iccid);
  // No event raised because ICCID is not exposed.

  VerifyEventCount();

  // Update our expectations.
  PopulateMVNOData();
  olp_list_.push_back({olp_url, olp_method, olp_post_data});

  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, CachedUserUpdatesOverrideMVNO) {
  // message: MVNO.
  // - First send updates that don't identify an MNO.
  // - Then identify an MNO and MVNO.
  // - verify that all the earlier updates are cached, and override the MVNO
  //   information.
  string imsi {"2009991234512345"};
  string iccid {"200999123456789"};
  string sid {"200999"};
  string olp_url {"url@url.com"};
  string olp_method {"POST"};
  string olp_post_data {"data"};

  // Send updates.
  ExpectEventCount(0);
  UpdateSID(sid);
  UpdateOnlinePortal(olp_url, olp_method, olp_post_data);
  UpdateIMSI(imsi);
  UpdateICCID(iccid);
  VerifyEventCount();

  // Determine MVNO.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");

  // Update our expectations.
  PopulateMVNOData();
  sid_ = sid;
  sid_list_.push_back(sid);
  olp_list_.push_back({olp_url, olp_method, olp_post_data});

  VerifyDatabaseData();
  VerifyUserData();
}

TEST_P(MobileOperatorInfoDataTest, RedundantUserUpdatesMVNO) {
  // - match MVNO.
  // - send redundant updates to properties.
  // - Verify no events, no updates to properties.

  // Identify MVNO.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");

  // Send redundant updates.
  // TODO(pprabhu)
  // |UpdateOnlinePortal| leads to an event because this is the first time this
  // value are set *by the user*. Although the values from the database were the
  // same, we did not use those values for filters.  It would be ideal to not
  // raise these redundant events (since no public information about the object
  // changed), but I haven't invested in doing so yet.
  ExpectEventCount(1);
  UpdateOperatorName(operator_info_->operator_name());
  UpdateOnlinePortal("someother@random.com", "GET", "");
  VerifyEventCount();
  PopulateMVNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, RedundantCachedUpdatesMVNO) {
  // message: MVNO.
  // - First send updates that don't identify MVNO, but match the data.
  // - Then idenityf an MNO and MVNO.
  // - verify that redundant information occurs only once.

  // Send redundant updates.
  ExpectEventCount(2);
  UpdateSID(operator_info_->sid());
  UpdateOperatorName(operator_info_->operator_name());
  UpdateOnlinePortal("someother@random.com", "GET", "");

  // Identify MVNO.
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");

  PopulateMVNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, ResetClearsInformation) {
  // Repeatedly reset the object and check M[V]NO identification and data.
  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200201");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200201");
  PopulateMNOData();
  VerifyDatabaseData();

  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyEventCount();
  VerifyMVNOWithUUID("uuid200101");
  PopulateMVNOData();
  VerifyDatabaseData();

  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("200001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid200001");
  PopulateMNOData();
  VerifyDatabaseData();
}

TEST_P(MobileOperatorInfoDataTest, FilteredOLP) {
  // We only check basic filter matching, using the fact that the regex matching
  // code is shared with the MVNO filtering, and is already well tested.
  // (1) None of the filters match.
  ExpectEventCount(1);
  UpdateMCCMNC("200001");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid200001");

  ASSERT_EQ(1, operator_info_->olp_list().size());
  // Just check that the filtered OLPs are not in the list.
  EXPECT_NE("olp@mccmnc", operator_info_->olp_list()[0].url);
  EXPECT_NE("olp@sid", operator_info_->olp_list()[0].url);

  // (2) MCCMNC filter matches.
  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateMCCMNC("200003");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid200001");

  ASSERT_EQ(2, operator_info_->olp_list().size());
  EXPECT_NE("olp@sid", operator_info_->olp_list()[0].url);
  bool found_olp_by_mccmnc = false;
  for (const auto& olp : operator_info_->olp_list()) {
    found_olp_by_mccmnc |= ("olp@mccmnc" == olp.url);
  }
  EXPECT_TRUE(found_olp_by_mccmnc);

  // (3) SID filter matches.
  ExpectEventCount(1);
  operator_info_->Reset();
  VerifyEventCount();
  VerifyNoMatch();

  ExpectEventCount(1);
  UpdateSID("200345");
  VerifyEventCount();
  VerifyMNOWithUUID("uuid200001");

  ASSERT_EQ(2, operator_info_->olp_list().size());
  EXPECT_NE("olp@mccmnc", operator_info_->olp_list()[0].url);
  bool found_olp_by_sid = false;
  for (const auto& olp : operator_info_->olp_list()) {
    found_olp_by_sid |= ("olp@sid" == olp.url);
  }
  EXPECT_TRUE(found_olp_by_sid);
}

class MobileOperatorInfoObserverTest : public MobileOperatorInfoMainTest {
 public:
  MobileOperatorInfoObserverTest() : MobileOperatorInfoMainTest() {}

  // Same as |MobileOperatorInfoMainTest::SetUp|, except that we don't add a
  // default observer.
  virtual void SetUp() {
    operator_info_->ClearDatabasePaths();
    AddDatabase(mobile_operator_db::data_test,
                arraysize(mobile_operator_db::data_test));
    operator_info_->Init();
  }

 protected:
  // ///////////////////////////////////////////////////////////////////////////
  // Data.
  MockMobileOperatorInfoObserver second_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileOperatorInfoObserverTest);
};

TEST_P(MobileOperatorInfoObserverTest, NoObserver) {
  // - Don't add any observers, and then cause an MVNO update to occur.
  // - Verify no crash.
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
}

TEST_P(MobileOperatorInfoObserverTest, MultipleObservers) {
  // - Add two observers, and then cause an MVNO update to occur.
  // - Verify both observers are notified.
  operator_info_->AddObserver(&observer_);
  operator_info_->AddObserver(&second_observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(2);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyMVNOWithUUID("uuid200101");

  dispatcher_.DispatchPendingEvents();
}

TEST_P(MobileOperatorInfoObserverTest, LateObserver) {
  // - Add one observer, and verify it gets an MVNO update.
  operator_info_->AddObserver(&observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(2);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(0);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyMVNOWithUUID("uuid200101");
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(1);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(0);
  operator_info_->Reset();
  VerifyNoMatch();
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);

  // - Add another observer, verify both get an MVNO update.
  operator_info_->AddObserver(&second_observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(2);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyMVNOWithUUID("uuid200101");
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(1);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(1);
  operator_info_->Reset();
  VerifyNoMatch();
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);

  // - Remove an observer, verify it no longer gets updates.
  operator_info_->RemoveObserver(&observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(0);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(2);
  UpdateMCCMNC("200001");
  UpdateOperatorName("name200101");
  VerifyMVNOWithUUID("uuid200101");
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);

  EXPECT_CALL(observer_, OnOperatorChanged()).Times(0);
  EXPECT_CALL(second_observer_, OnOperatorChanged()).Times(1);
  operator_info_->Reset();
  VerifyNoMatch();
  dispatcher_.DispatchPendingEvents();
  Mock::VerifyAndClearExpectations(&observer_);
  Mock::VerifyAndClearExpectations(&second_observer_);
}

INSTANTIATE_TEST_CASE_P(MobileOperatorInfoMainTestInstance,
                        MobileOperatorInfoMainTest,
                        Values(kEventCheckingPolicyStrict,
                               kEventCheckingPolicyNonStrict));
INSTANTIATE_TEST_CASE_P(MobileOperatorInfoDataTestInstance,
                        MobileOperatorInfoDataTest,
                        Values(kEventCheckingPolicyStrict,
                               kEventCheckingPolicyNonStrict));
// It only makes sense to do strict checking here.
INSTANTIATE_TEST_CASE_P(MobileOperatorInfoObserverTestInstance,
                        MobileOperatorInfoObserverTest,
                        Values(kEventCheckingPolicyStrict));
}  // namespace shill
