// Copyright 2016 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/access_api_handler.h"

#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/test/mock_device.h>
#include <weave/test/unittest_utils.h>

#include "src/component_manager_impl.h"
#include "src/access_black_list_manager.h"
#include "src/data_encoding.h"

using testing::_;
using testing::AnyOf;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::WithArgs;

namespace weave {

class MockAccessBlackListManager : public AccessBlackListManager {
 public:
  MOCK_METHOD4(Block,
               void(const std::vector<uint8_t>&,
                    const std::vector<uint8_t>&,
                    const base::Time&,
                    const DoneCallback&));
  MOCK_METHOD3(Unblock,
               void(const std::vector<uint8_t>&,
                    const std::vector<uint8_t>&,
                    const DoneCallback&));
  MOCK_CONST_METHOD2(IsBlocked,
                     bool(const std::vector<uint8_t>&,
                          const std::vector<uint8_t>&));
  MOCK_CONST_METHOD0(GetEntries, std::vector<Entry>());
  MOCK_CONST_METHOD0(GetSize, size_t());
  MOCK_CONST_METHOD0(GetCapacity, size_t());
};

class AccessApiHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(device_, AddTraitDefinitionsFromJson(_))
        .WillRepeatedly(Invoke([this](const std::string& json) {
          EXPECT_TRUE(component_manager_.LoadTraits(json, nullptr));
        }));
    EXPECT_CALL(device_, SetStateProperties(_, _, _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::SetStateProperties));
    EXPECT_CALL(device_, SetStateProperty(_, _, _, _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::SetStateProperty));
    EXPECT_CALL(device_, AddComponent(_, _, _))
        .WillRepeatedly(Invoke([this](const std::string& name,
                                      const std::vector<std::string>& traits,
                                      ErrorPtr* error) {
          return component_manager_.AddComponent("", name, traits, error);
        }));

    EXPECT_CALL(device_,
                AddCommandHandler(_, AnyOf("_accessControlBlackList.block",
                                           "_accessControlBlackList.unblock",
                                           "_accessControlBlackList.list"),
                                  _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::AddCommandHandler));

    EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(0));

    EXPECT_CALL(access_manager_, GetCapacity()).WillRepeatedly(Return(10));

    handler_.reset(new AccessApiHandler{&device_, &access_manager_});
  }

  const base::DictionaryValue& AddCommand(const std::string& command) {
    std::string id;
    auto command_instance = component_manager_.ParseCommandInstance(
        *test::CreateDictionaryValue(command.c_str()), Command::Origin::kLocal,
        UserRole::kOwner, &id, nullptr);
    EXPECT_NE(nullptr, command_instance.get());
    component_manager_.AddCommand(std::move(command_instance));
    EXPECT_EQ(Command::State::kDone,
              component_manager_.FindCommand(id)->GetState());
    return component_manager_.FindCommand(id)->GetResults();
  }

  std::unique_ptr<base::DictionaryValue> GetState() {
    std::string path =
        component_manager_.FindComponentWithTrait("_accessControlBlackList");
    EXPECT_FALSE(path.empty());
    const auto* component = component_manager_.FindComponent(path, nullptr);
    EXPECT_TRUE(component);
    const base::DictionaryValue* state = nullptr;
    EXPECT_TRUE(
        component->GetDictionary("state._accessControlBlackList", &state));
    return std::unique_ptr<base::DictionaryValue>{state->DeepCopy()};
  }

  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  ComponentManagerImpl component_manager_{&task_runner_};
  StrictMock<test::MockDevice> device_;
  StrictMock<MockAccessBlackListManager> access_manager_;
  std::unique_ptr<AccessApiHandler> handler_;
};

TEST_F(AccessApiHandlerTest, Initialization) {
  const base::DictionaryValue* trait = nullptr;
  ASSERT_TRUE(component_manager_.GetTraits().GetDictionary(
      "_accessControlBlackList", &trait));

  auto expected = R"({
    "commands": {
      "block": {
        "minimalRole": "owner",
        "parameters": {
          "userId": {
            "type": "string"
          },
          "applicationId": {
            "type": "string"
          },
          "expirationTimeoutSec": {
            "type": "integer"
          }
        }
      },
      "unblock": {
        "minimalRole": "owner",
        "parameters": {
          "userId": {
            "type": "string"
          },
          "applicationId": {
            "type": "string"
          }
        }
      },
      "list": {
        "minimalRole": "owner",
        "parameters": {},
        "results": {
          "blackList": {
            "type": "array",
            "items": {
              "type": "object",
              "properties": {
                "userId": {
                  "type": "string"
                },
                "applicationId": {
                  "type": "string"
                }
              },
              "additionalProperties": false
            }
          }
        }
      }
    },
    "state": {
      "size": {
        "type": "integer",
        "isRequired": true
      },
      "capacity": {
        "type": "integer",
        "isRequired": true
      }
    }
  })";
  EXPECT_JSON_EQ(expected, *trait);

  expected = R"({
    "capacity": 10,
    "size": 0
  })";
  EXPECT_JSON_EQ(expected, *GetState());
}

TEST_F(AccessApiHandlerTest, Block) {
  EXPECT_CALL(access_manager_, Block(std::vector<uint8_t>{1, 2, 3},
                                     std::vector<uint8_t>{3, 4, 5}, _, _))
      .WillOnce(WithArgs<3>(
          Invoke([](const DoneCallback& callback) { callback.Run(nullptr); })));
  EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(1));

  AddCommand(R"({
    'name' : '_accessControlBlackList.block',
    'component': 'accessControl',
    'parameters': {
      'userId': 'AQID',
      'applicationId': 'AwQF',
      'expirationTimeoutSec': 1234
    }
  })");

  auto expected = R"({
    "capacity": 10,
    "size": 1
  })";
  EXPECT_JSON_EQ(expected, *GetState());
}

TEST_F(AccessApiHandlerTest, Unblock) {
  EXPECT_CALL(access_manager_, Unblock(std::vector<uint8_t>{1, 2, 3},
                                       std::vector<uint8_t>{3, 4, 5}, _))
      .WillOnce(WithArgs<2>(
          Invoke([](const DoneCallback& callback) { callback.Run(nullptr); })));
  EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(4));

  AddCommand(R"({
    'name' : '_accessControlBlackList.unblock',
    'component': 'accessControl',
    'parameters': {
      'userId': 'AQID',
      'applicationId': 'AwQF',
      'expirationTimeoutSec': 1234
    }
  })");

  auto expected = R"({
    "capacity": 10,
    "size": 4
  })";
  EXPECT_JSON_EQ(expected, *GetState());
}

TEST_F(AccessApiHandlerTest, List) {
  std::vector<AccessBlackListManager::Entry> entries{
      {{11, 12, 13}, {21, 22, 23}, base::Time::FromTimeT(1410000000)},
      {{31, 32, 33}, {41, 42, 43}, base::Time::FromTimeT(1420000000)},
  };
  EXPECT_CALL(access_manager_, GetEntries()).WillOnce(Return(entries));
  EXPECT_CALL(access_manager_, GetSize()).WillRepeatedly(Return(4));

  auto expected = R"({
    "blackList": [ {
      "applicationId": "FRYX",
      "userId": "CwwN"
    }, {
       "applicationId": "KSor",
       "userId": "HyAh"
    } ]
  })";

  const auto& results = AddCommand(R"({
    'name' : '_accessControlBlackList.list',
    'component': 'accessControl',
    'parameters': {
    }
  })");

  EXPECT_JSON_EQ(expected, results);
}
}  // namespace weave
