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

#include "shill/property_observer.h"

#include <string>

#include <base/bind.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/accessor_interface.h"
#include "shill/error.h"

using base::Bind;
using base::Unretained;
using testing::_;
using testing::DoAll;
using testing::Mock;
using testing::Return;

namespace shill {

class TestPropertyAccessor : public AccessorInterface<bool> {
 public:
  MOCK_METHOD1(Clear, void(Error* error));
  MOCK_METHOD1(Get, bool(Error* error));
  MOCK_METHOD2(Set, bool(const bool& value, Error* error));
};

class PropertyObserverTest : public testing::Test {
 public:
  PropertyObserverTest()
     : test_accessor_(new TestPropertyAccessor()),
       bool_accessor_(test_accessor_) {}
  virtual ~PropertyObserverTest() {}

  MOCK_METHOD1(TestCallback, void(const bool& value));

  // Invoked method during expectations.
  void SetError(Error* error) {
    error->Populate(Error::kPermissionDenied);
  }

 protected:
  bool GetSavedValue(const PropertyObserver<bool>& observer) {
    return observer.saved_value_;
  }

  TestPropertyAccessor* test_accessor_;
  BoolAccessor bool_accessor_;  // Owns reference to |test_accessor_|.
};

TEST_F(PropertyObserverTest, Callback) {
  EXPECT_CALL(*test_accessor_, Get(_)).WillOnce(Return(true));
  EXPECT_CALL(*this, TestCallback(_)).Times(0);
  PropertyObserver<bool> observer(bool_accessor_,
                                  Bind(&PropertyObserverTest::TestCallback,
                                       Unretained(this)));
  EXPECT_TRUE(GetSavedValue(observer));
  Mock::VerifyAndClearExpectations(test_accessor_);

  // Accessor returns an error.
  EXPECT_CALL(*test_accessor_, Get(_))
      .WillOnce(DoAll(Invoke(this, &PropertyObserverTest::SetError),
                      Return(false)));
  observer.Update();

  // Value remains unchanged.
  EXPECT_CALL(*test_accessor_, Get(_)).WillOnce(Return(true));
  observer.Update();
  Mock::VerifyAndClearExpectations(test_accessor_);
  Mock::VerifyAndClearExpectations(this);

  // Value changes.
  EXPECT_CALL(*test_accessor_, Get(_)).WillOnce(Return(false));
  EXPECT_CALL(*this, TestCallback(false));
  observer.Update();
  EXPECT_FALSE(GetSavedValue(observer));
  Mock::VerifyAndClearExpectations(test_accessor_);
  Mock::VerifyAndClearExpectations(this);

  // Value remains unchanged (false).
  EXPECT_CALL(*test_accessor_, Get(_)).WillOnce(Return(false));
  EXPECT_CALL(*this, TestCallback(_)).Times(0);
  observer.Update();
}

}  // namespace shill
