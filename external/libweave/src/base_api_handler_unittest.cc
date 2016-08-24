// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/base_api_handler.h"

#include <base/strings/string_number_conversions.h>
#include <base/time/default_clock.h>
#include <base/values.h>
#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/provider/test/mock_config_store.h>
#include <weave/provider/test/mock_http_client.h>
#include <weave/test/mock_device.h>
#include <weave/test/unittest_utils.h>

#include "src/component_manager_impl.h"
#include "src/config.h"
#include "src/device_registration_info.h"

using testing::_;
using testing::AnyOf;
using testing::Eq;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;
using testing::StrictMock;

namespace weave {

class BaseApiHandlerTest : public ::testing::Test {
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
                AddCommandHandler(_, AnyOf("base.updateBaseConfiguration",
                                           "base.updateDeviceInfo"),
                                  _))
        .WillRepeatedly(
            Invoke(&component_manager_, &ComponentManager::AddCommandHandler));

    dev_reg_.reset(new DeviceRegistrationInfo(&config_, &component_manager_,
                                              nullptr, &http_client_, nullptr,
                                              nullptr));

    EXPECT_CALL(device_, GetSettings())
        .WillRepeatedly(ReturnRef(dev_reg_->GetSettings()));

    handler_.reset(new BaseApiHandler{dev_reg_.get(), &device_});
  }

  void AddCommand(const std::string& command) {
    std::string id;
    auto command_instance = component_manager_.ParseCommandInstance(
        *test::CreateDictionaryValue(command.c_str()), Command::Origin::kLocal,
        UserRole::kOwner, &id, nullptr);
    ASSERT_NE(nullptr, command_instance.get());
    component_manager_.AddCommand(std::move(command_instance));
    EXPECT_EQ(Command::State::kDone,
              component_manager_.FindCommand(id)->GetState());
  }

  std::unique_ptr<base::DictionaryValue> GetBaseState() {
    std::unique_ptr<base::DictionaryValue> state;
    std::string path = component_manager_.FindComponentWithTrait("base");
    EXPECT_FALSE(path.empty());
    const auto* component = component_manager_.FindComponent(path, nullptr);
    CHECK(component);
    const base::DictionaryValue* base_state = nullptr;
    if (component->GetDictionary("state.base", &base_state))
      state.reset(base_state->DeepCopy());
    else
      state.reset(new base::DictionaryValue);
    return state;
  }

  provider::test::MockConfigStore config_store_;
  Config config_{&config_store_};
  StrictMock<provider::test::MockHttpClient> http_client_;
  std::unique_ptr<DeviceRegistrationInfo> dev_reg_;
  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  ComponentManagerImpl component_manager_{&task_runner_};
  std::unique_ptr<BaseApiHandler> handler_;
  StrictMock<test::MockDevice> device_;
};

TEST_F(BaseApiHandlerTest, Initialization) {
  const base::DictionaryValue* trait = nullptr;
  ASSERT_TRUE(component_manager_.GetTraits().GetDictionary("base", &trait));

  auto expected = R"({
    "commands": {
      "updateBaseConfiguration": {
        "minimalRole": "manager",
        "parameters": {
          "localAnonymousAccessMaxRole": {
            "enum": [ "none", "viewer", "user" ],
            "type": "string"
          },
          "localDiscoveryEnabled": {
            "type": "boolean"
          },
          "localPairingEnabled": {
            "type": "boolean"
          }
        }
      },
      "updateDeviceInfo": {
        "minimalRole": "manager",
        "parameters": {
          "description": {
            "type": "string"
          },
          "location": {
            "type": "string"
          },
          "name": {
            "type": "string"
          }
        }
      },
      "reboot": {
        "minimalRole": "user",
        "parameters": {},
        "errors": ["notEnoughBattery"]
      },
      "identify": {
        "minimalRole": "user",
        "parameters": {}
      }
    },
    "state": {
      "firmwareVersion": {
        "type": "string",
        "isRequired": true
      },
      "localDiscoveryEnabled": {
        "type": "boolean",
        "isRequired": true
      },
      "localAnonymousAccessMaxRole": {
        "type": "string",
        "enum": [ "none", "viewer", "user" ],
        "isRequired": true
      },
      "localPairingEnabled": {
        "type": "boolean",
        "isRequired": true
      },
      "connectionStatus": {
        "type": "string"
      },
      "network": {
        "type": "object",
        "additionalProperties": false,
        "properties": {
          "name": { "type": "string" }
        }
      }
    }
  })";
  EXPECT_JSON_EQ(expected, *trait);
}

TEST_F(BaseApiHandlerTest, UpdateBaseConfiguration) {
  const Settings& settings = dev_reg_->GetSettings();

  AddCommand(R"({
    'name' : 'base.updateBaseConfiguration',
    'component': 'base',
    'parameters': {
      'localDiscoveryEnabled': false,
      'localAnonymousAccessMaxRole': 'none',
      'localPairingEnabled': false
    }
  })");
  EXPECT_EQ(AuthScope::kNone, settings.local_anonymous_access_role);
  EXPECT_FALSE(settings.local_discovery_enabled);
  EXPECT_FALSE(settings.local_pairing_enabled);

  auto expected = R"({
    'firmwareVersion': 'TEST_FIRMWARE',
    'localAnonymousAccessMaxRole': 'none',
    'localDiscoveryEnabled': false,
    'localPairingEnabled': false
  })";
  EXPECT_JSON_EQ(expected, *GetBaseState());

  AddCommand(R"({
    'name' : 'base.updateBaseConfiguration',
    'component': 'base',
    'parameters': {
      'localDiscoveryEnabled': true,
      'localAnonymousAccessMaxRole': 'user',
      'localPairingEnabled': true
    }
  })");
  EXPECT_EQ(AuthScope::kUser, settings.local_anonymous_access_role);
  EXPECT_TRUE(settings.local_discovery_enabled);
  EXPECT_TRUE(settings.local_pairing_enabled);
  expected = R"({
    'firmwareVersion': 'TEST_FIRMWARE',
    'localAnonymousAccessMaxRole': 'user',
    'localDiscoveryEnabled': true,
    'localPairingEnabled': true
  })";
  EXPECT_JSON_EQ(expected, *GetBaseState());

  {
    Config::Transaction change{dev_reg_->GetMutableConfig()};
    change.set_local_anonymous_access_role(AuthScope::kViewer);
  }
  expected = R"({
    'firmwareVersion': 'TEST_FIRMWARE',
    'localAnonymousAccessMaxRole': 'viewer',
    'localDiscoveryEnabled': true,
    'localPairingEnabled': true
  })";
  EXPECT_JSON_EQ(expected, *GetBaseState());
}

TEST_F(BaseApiHandlerTest, UpdateDeviceInfo) {
  AddCommand(R"({
    'name' : 'base.updateDeviceInfo',
    'component': 'base',
    'parameters': {
      'name': 'testName',
      'description': 'testDescription',
      'location': 'testLocation'
    }
  })");

  const Settings& config = dev_reg_->GetSettings();
  EXPECT_EQ("testName", config.name);
  EXPECT_EQ("testDescription", config.description);
  EXPECT_EQ("testLocation", config.location);

  AddCommand(R"({
    'name' : 'base.updateDeviceInfo',
    'component': 'base',
    'parameters': {
      'location': 'newLocation'
    }
  })");

  EXPECT_EQ("testName", config.name);
  EXPECT_EQ("testDescription", config.description);
  EXPECT_EQ("newLocation", config.location);
}

}  // namespace weave
