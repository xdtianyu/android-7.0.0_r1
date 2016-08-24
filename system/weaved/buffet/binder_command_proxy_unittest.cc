// Copyright 2016 The Android Open Source Project
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

#include "buffet/binder_command_proxy.h"

#include <memory>

#include <gtest/gtest.h>
#include <weave/command.h>
#include <weave/enum_to_string.h>
#include <weave/test/mock_command.h>
#include <weave/test/unittest_utils.h>

#include "common/binder_utils.h"

using weaved::binder_utils::ToString;
using weaved::binder_utils::ToString16;

namespace buffet {

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;
using ::testing::StrictMock;

using weave::test::CreateDictionaryValue;
using weave::test::IsEqualValue;

namespace {

const char kTestCommandId[] = "cmd_1";

MATCHER_P(EqualToJson, json, "") {
  auto json_value = CreateDictionaryValue(json);
  return IsEqualValue(*json_value, arg);
}

MATCHER_P2(ExpectError, code, message, "") {
  return arg->GetCode() == code && arg->GetMessage() == message;
}

}  // namespace

class BinderCommandProxyTest : public ::testing::Test {
 public:
  void SetUp() override {
    command_ = std::make_shared<StrictMock<weave::test::MockCommand>>();

    expected_result_dict_.SetInteger("height", 53);
    expected_result_dict_.SetString("_jumpType", "_withKick");
    EXPECT_CALL(*command_, GetID())
        .WillRepeatedly(ReturnRefOfCopy<std::string>(kTestCommandId));
    EXPECT_CALL(*command_, GetName())
        .WillRepeatedly(ReturnRefOfCopy<std::string>("robot.jump"));
    EXPECT_CALL(*command_, GetComponent())
        .WillRepeatedly(ReturnRefOfCopy<std::string>("myComponent"));
    EXPECT_CALL(*command_, GetState())
        .WillRepeatedly(Return(weave::Command::State::kQueued));
    EXPECT_CALL(*command_, GetOrigin())
        .WillRepeatedly(Return(weave::Command::Origin::kLocal));
    EXPECT_CALL(*command_, GetParameters())
        .WillRepeatedly(ReturnRef(expected_result_dict_));
    EXPECT_CALL(*command_, GetProgress())
        .WillRepeatedly(ReturnRef(empty_dict_));
    EXPECT_CALL(*command_, GetResults())
        .WillRepeatedly(ReturnRef(empty_dict_));

    proxy_.reset(
        new BinderCommandProxy{std::weak_ptr<weave::Command>{command_}});
  }

  BinderCommandProxy* GetCommandProxy() const { return proxy_.get(); }

  weave::Command::State GetCommandState() const {
    weave::Command::State state;
    android::String16 state_string;
    EXPECT_TRUE(GetCommandProxy()->getState(&state_string).isOk());
    EXPECT_TRUE(StringToEnum(ToString(state_string), &state));
    return state;
  }

  weave::Command::Origin GetCommandOrigin() const {
    weave::Command::Origin origin;
    android::String16 origin_string;
    EXPECT_TRUE(GetCommandProxy()->getOrigin(&origin_string).isOk());
    EXPECT_TRUE(StringToEnum(ToString(origin_string), &origin));
    return origin;
  }

  base::DictionaryValue empty_dict_;
  base::DictionaryValue expected_result_dict_;

  std::shared_ptr<StrictMock<weave::test::MockCommand>> command_;
  std::unique_ptr<BinderCommandProxy> proxy_;
};

TEST_F(BinderCommandProxyTest, Init) {
  android::String16 result;
  EXPECT_EQ(weave::Command::State::kQueued, GetCommandState());
  EXPECT_EQ(weave::Command::Origin::kLocal, GetCommandOrigin());
  EXPECT_TRUE(GetCommandProxy()->getParameters(&result).isOk());
  EXPECT_EQ(R"({"_jumpType":"_withKick","height":53})", ToString(result));
  EXPECT_TRUE(GetCommandProxy()->getProgress(&result).isOk());
  EXPECT_EQ("{}", ToString(result));
  EXPECT_TRUE(GetCommandProxy()->getResults(&result).isOk());
  EXPECT_EQ("{}", ToString(result));
  EXPECT_TRUE(GetCommandProxy()->getName(&result).isOk());
  EXPECT_EQ("robot.jump", ToString(result));
  EXPECT_TRUE(GetCommandProxy()->getComponent(&result).isOk());
  EXPECT_EQ("myComponent", ToString(result));
  EXPECT_TRUE(GetCommandProxy()->getId(&result).isOk());
  EXPECT_EQ(kTestCommandId, ToString(result));
}

TEST_F(BinderCommandProxyTest, SetProgress) {
  EXPECT_CALL(*command_, SetProgress(EqualToJson("{'progress': 10}"), _))
      .WillOnce(Return(true));
  EXPECT_TRUE(
      GetCommandProxy()->setProgress(ToString16(R"({"progress": 10})")).isOk());
}

TEST_F(BinderCommandProxyTest, Complete) {
  EXPECT_CALL(
      *command_,
      Complete(
          EqualToJson("{'foo': 42, 'bar': 'foobar', 'resultList': [1, 2, 3]}"),
          _))
      .WillOnce(Return(true));
  const android::String16 result{
      R"({"foo": 42, "bar": "foobar", "resultList": [1, 2, 3]})"};
  EXPECT_TRUE(GetCommandProxy()->complete(result).isOk());
}

TEST_F(BinderCommandProxyTest, Abort) {
  EXPECT_CALL(*command_, Abort(ExpectError("foo", "bar"), _))
      .WillOnce(Return(true));
  EXPECT_TRUE(
      GetCommandProxy()->abort(ToString16("foo"), ToString16("bar")).isOk());
}

TEST_F(BinderCommandProxyTest, Cancel) {
  EXPECT_CALL(*command_, Cancel(_)).WillOnce(Return(true));
  EXPECT_TRUE(GetCommandProxy()->cancel().isOk());
}

TEST_F(BinderCommandProxyTest, Pause) {
  EXPECT_CALL(*command_, Pause(_)).WillOnce(Return(true));
  EXPECT_TRUE(GetCommandProxy()->pause().isOk());
}

}  // namespace buffet
