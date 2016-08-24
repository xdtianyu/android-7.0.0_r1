// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/component_manager_impl.h"

#include <map>

#include <gtest/gtest.h>
#include <weave/provider/test/fake_task_runner.h>
#include <weave/test/unittest_utils.h>

#include "src/bind_lambda.h"
#include "src/commands/schema_constants.h"
#include "src/mock_component_manager.h"
#include "src/test/mock_clock.h"

namespace weave {

using test::CreateDictionaryValue;
using testing::Return;
using testing::StrictMock;

namespace {

bool HasTrait(const base::DictionaryValue& comp, const std::string& trait) {
  const base::ListValue* list = nullptr;
  if (!comp.GetList("traits", &list))
    return false;
  for (const base::Value* item : *list) {
    std::string value;
    if (item->GetAsString(&value) && value == trait)
      return true;
  }
  return false;
}

// Creates sample trait/component trees:
// {
//   "traits": {
//     "t1": {},
//     "t2": {},
//     "t3": {},
//     "t4": {},
//     "t5": {},
//     "t6": {},
//   },
//   "components": {
//     "comp1": {
//       "traits": [ "t1" ],
//       "components": {
//         "comp2": [
//           { "traits": [ "t2" ] },
//           {
//             "traits": [ "t3" ],
//             "components": {
//               "comp3": {
//                 "traits": [ "t4" ],
//                 "components": {
//                   "comp4": {
//                     "traits": [ "t5", "t6" ]
//                   }
//                 }
//               }
//             }
//           }
//         ],
//       }
//     }
//   }
// }
class ComponentManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(base::Time::Now()));
  }

  void CreateTestComponentTree(ComponentManager* manager) {
    const char kTraits[] =
        R"({"t1":{},"t2":{},"t3":{},"t4":{},"t5":{},"t6":{}})";
    auto json = CreateDictionaryValue(kTraits);
    ASSERT_TRUE(manager->LoadTraits(*json, nullptr));
    EXPECT_TRUE(manager->AddComponent("", "comp1", {"t1"}, nullptr));
    EXPECT_TRUE(
        manager->AddComponentArrayItem("comp1", "comp2", {"t2"}, nullptr));
    EXPECT_TRUE(
        manager->AddComponentArrayItem("comp1", "comp2", {"t3"}, nullptr));
    EXPECT_TRUE(
        manager->AddComponent("comp1.comp2[1]", "comp3", {"t4"}, nullptr));
    EXPECT_TRUE(manager->AddComponent("comp1.comp2[1].comp3", "comp4",
                                      {"t5", "t6"}, nullptr));
  }

  StrictMock<provider::test::FakeTaskRunner> task_runner_;
  StrictMock<test::MockClock> clock_;
  ComponentManagerImpl manager_{&task_runner_, &clock_};
};

}  // anonymous namespace

TEST_F(ComponentManagerTest, Empty) {
  EXPECT_TRUE(manager_.GetTraits().empty());
  EXPECT_TRUE(manager_.GetComponents().empty());
}

TEST_F(ComponentManagerTest, LoadTraits) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  EXPECT_JSON_EQ(kTraits, manager_.GetTraits());
  EXPECT_TRUE(manager_.GetComponents().empty());
}

TEST_F(ComponentManagerTest, LoadTraitsDuplicateIdentical) {
  const char kTraits1[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits1);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  const char kTraits2[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  const char kExpected[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetTraits());
}

TEST_F(ComponentManagerTest, LoadTraitsDuplicateOverride) {
  const char kTraits1[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits1);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  const char kTraits2[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "string"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_FALSE(manager_.LoadTraits(*json, nullptr));
}

TEST_F(ComponentManagerTest, AddTraitDefChangedCallback) {
  int count = 0;
  int count2 = 0;
  manager_.AddTraitDefChangedCallback(base::Bind([&count]() { count++; }));
  manager_.AddTraitDefChangedCallback(base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  // New definitions.
  const char kTraits1[] = R"({
    "trait1": {
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits1);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  EXPECT_EQ(2, count);
  // Duplicate definition, shouldn't call the callback.
  const char kTraits2[] = R"({
    "trait1": {
      "state": {
        "property1": {"type": "boolean"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits2);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  EXPECT_EQ(2, count);
  // New definition, should call the callback now.
  const char kTraits3[] = R"({
    "trait3": {
      "state": {
        "property3": {"type": "string"}
      }
    }
  })";
  json = CreateDictionaryValue(kTraits3);
  EXPECT_TRUE(manager_.LoadTraits(*json, nullptr));
  EXPECT_EQ(3, count);
  // Wrong definition, shouldn't call the callback.
  const char kTraits4[] = R"({
    "trait4": "foo"
  })";
  json = CreateDictionaryValue(kTraits4);
  EXPECT_FALSE(manager_.LoadTraits(*json, nullptr));
  EXPECT_EQ(3, count);
  // Make sure both callbacks were called the same number of times.
  EXPECT_EQ(count2, count);
}

TEST_F(ComponentManagerTest, LoadTraitsNotAnObject) {
  const char kTraits1[] = R"({"trait1": 0})";
  auto json = CreateDictionaryValue(kTraits1);
  ErrorPtr error;
  EXPECT_FALSE(manager_.LoadTraits(*json, &error));
  EXPECT_EQ(errors::commands::kTypeMismatch, error->GetCode());
}

TEST_F(ComponentManagerTest, FindTraitDefinition) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      },
      "state": {
        "property1": {"type": "boolean"}
      }
    },
    "trait2": {
      "state": {
        "property2": {"type": "string"}
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*json, nullptr));

  const base::DictionaryValue* trait = manager_.FindTraitDefinition("trait1");
  ASSERT_NE(nullptr, trait);
  const char kExpected1[] = R"({
    "commands": {
      "command1": {
        "minimalRole": "user",
        "parameters": {"height": {"type": "integer"}}
      }
    },
    "state": {
      "property1": {"type": "boolean"}
    }
  })";
  EXPECT_JSON_EQ(kExpected1, *trait);

  trait = manager_.FindTraitDefinition("trait2");
  ASSERT_NE(nullptr, trait);
  const char kExpected2[] = R"({
    "state": {
      "property2": {"type": "string"}
    }
  })";
  EXPECT_JSON_EQ(kExpected2, *trait);

  EXPECT_EQ(nullptr, manager_.FindTraitDefinition("trait3"));
}

TEST_F(ComponentManagerTest, FindCommandDefinition) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        }
      }
    },
    "trait2": {
      "commands": {
        "command1": {
          "minimalRole": "manager"
        },
        "command2": {
          "minimalRole": "owner"
        }
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*json, nullptr));

  const auto* cmd_def = manager_.FindCommandDefinition("trait1.command1");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected1[] = R"({
    "minimalRole": "user",
    "parameters": {"height": {"type": "integer"}}
  })";
  EXPECT_JSON_EQ(kExpected1, *cmd_def);

  cmd_def = manager_.FindCommandDefinition("trait2.command1");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected2[] = R"({
    "minimalRole": "manager"
  })";
  EXPECT_JSON_EQ(kExpected2, *cmd_def);

  cmd_def = manager_.FindCommandDefinition("trait2.command2");
  ASSERT_NE(nullptr, cmd_def);
  const char kExpected3[] = R"({
    "minimalRole": "owner"
  })";
  EXPECT_JSON_EQ(kExpected3, *cmd_def);

  EXPECT_EQ(nullptr, manager_.FindTraitDefinition("trait1.command2"));
  EXPECT_EQ(nullptr, manager_.FindTraitDefinition("trait3.command1"));
  EXPECT_EQ(nullptr, manager_.FindTraitDefinition("trait"));
  EXPECT_EQ(nullptr,
            manager_.FindTraitDefinition("trait1.command1.parameters"));
}

TEST_F(ComponentManagerTest, GetMinimalRole) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "viewer" }
      }
    },
    "trait2": {
      "commands": {
        "command1": { "minimalRole": "manager" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*json, nullptr));

  UserRole role;
  ASSERT_TRUE(manager_.GetMinimalRole("trait1.command1", &role, nullptr));
  EXPECT_EQ(UserRole::kUser, role);

  ASSERT_TRUE(manager_.GetMinimalRole("trait1.command2", &role, nullptr));
  EXPECT_EQ(UserRole::kViewer, role);

  ASSERT_TRUE(manager_.GetMinimalRole("trait2.command1", &role, nullptr));
  EXPECT_EQ(UserRole::kManager, role);

  ASSERT_TRUE(manager_.GetMinimalRole("trait2.command2", &role, nullptr));
  EXPECT_EQ(UserRole::kOwner, role);

  EXPECT_FALSE(manager_.GetMinimalRole("trait1.command3", &role, nullptr));
}

TEST_F(ComponentManagerTest, AddComponent) {
  const char kTraits[] = R"({"trait1": {}, "trait2": {}, "trait3": {}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*json, nullptr));
  EXPECT_TRUE(
      manager_.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("", "comp2", {"trait3"}, nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": ["trait1", "trait2"]
    },
    "comp2": {
      "traits": ["trait3"]
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetComponents());

  // 'trait4' is undefined, so can't add a component referring to it.
  EXPECT_FALSE(manager_.AddComponent("", "comp3", {"trait4"}, nullptr));
}

TEST_F(ComponentManagerTest, AddSubComponent) {
  EXPECT_TRUE(manager_.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("comp1", "comp3", {}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("comp1.comp2", "comp4", {}, nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": [],
      "components": {
        "comp2": {
          "traits": [],
          "components": {
            "comp4": {
              "traits": []
            }
          }
        },
        "comp3": {
          "traits": []
        }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetComponents());
}

TEST_F(ComponentManagerTest, AddComponentArrayItem) {
  const char kTraits[] = R"({"foo": {}, "bar": {}})";
  auto json = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*json, nullptr));

  EXPECT_TRUE(manager_.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(
      manager_.AddComponentArrayItem("comp1", "comp2", {"foo"}, nullptr));
  EXPECT_TRUE(
      manager_.AddComponentArrayItem("comp1", "comp2", {"bar"}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("comp1.comp2[1]", "comp3", {}, nullptr));
  EXPECT_TRUE(
      manager_.AddComponent("comp1.comp2[1].comp3", "comp4", {}, nullptr));
  const char kExpected[] = R"({
    "comp1": {
      "traits": [],
      "components": {
        "comp2": [
          {
            "traits": ["foo"]
          },
          {
            "traits": ["bar"],
            "components": {
              "comp3": {
                "traits": [],
                "components": {
                  "comp4": {
                    "traits": []
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetComponents());
}

TEST_F(ComponentManagerTest, RemoveComponent) {
  CreateTestComponentTree(&manager_);
  EXPECT_TRUE(manager_.RemoveComponent("comp1.comp2[1].comp3", "comp4",
                                       nullptr));
  const char kExpected1[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {}
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager_.GetComponents());
  EXPECT_TRUE(manager_.RemoveComponentArrayItem("comp1", "comp2", 1, nullptr));
  const char kExpected2[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager_.GetComponents());
}

TEST_F(ComponentManagerTest, AddComponentExist) {
  EXPECT_TRUE(manager_.AddComponent("", "comp1", {}, nullptr));
  EXPECT_FALSE(manager_.AddComponent("", "comp1", {}, nullptr));
  EXPECT_TRUE(manager_.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_FALSE(manager_.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST_F(ComponentManagerTest, AddComponentDoesNotExist) {
  EXPECT_FALSE(manager_.AddComponent("comp1", "comp2", {}, nullptr));
}

TEST_F(ComponentManagerTest, AddComponentTreeChangedCallback) {
  int count = 0;
  int count2 = 0;
  manager_.AddComponentTreeChangedCallback(base::Bind([&count]() { count++; }));
  manager_.AddComponentTreeChangedCallback(
      base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  EXPECT_TRUE(manager_.AddComponent("", "comp1", {}, nullptr));
  EXPECT_EQ(2, count);
  EXPECT_TRUE(manager_.AddComponent("comp1", "comp2", {}, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_TRUE(manager_.AddComponent("comp1.comp2", "comp4", {}, nullptr));
  EXPECT_EQ(4, count);
  EXPECT_TRUE(manager_.AddComponentArrayItem("comp1", "comp3", {}, nullptr));
  EXPECT_EQ(5, count);
  EXPECT_TRUE(manager_.AddComponentArrayItem("comp1", "comp3", {}, nullptr));
  EXPECT_EQ(6, count);
  EXPECT_TRUE(manager_.RemoveComponentArrayItem("comp1", "comp3", 1, nullptr));
  EXPECT_EQ(7, count);
  EXPECT_TRUE(manager_.RemoveComponent("", "comp1", nullptr));
  EXPECT_EQ(8, count);
  // Make sure both callbacks were called the same number of times.
  EXPECT_EQ(count2, count);
}

TEST_F(ComponentManagerTest, FindComponent) {
  CreateTestComponentTree(&manager_);

  const base::DictionaryValue* comp = manager_.FindComponent("comp1", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t1"));

  comp = manager_.FindComponent("comp1.comp2[0]", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t2"));

  comp = manager_.FindComponent("comp1.comp2[1]", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t3"));

  comp = manager_.FindComponent("comp1.comp2[1].comp3", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t4"));

  comp = manager_.FindComponent("comp1.comp2[1].comp3.comp4", nullptr);
  ASSERT_NE(nullptr, comp);
  EXPECT_TRUE(HasTrait(*comp, "t5"));

  // Some whitespaces don't hurt.
  comp = manager_.FindComponent(" comp1 . comp2 [  \t 1 ] .   comp3.comp4 ",
                                nullptr);
  EXPECT_NE(nullptr, comp);

  // Now check some failure cases.
  ErrorPtr error;
  EXPECT_EQ(nullptr, manager_.FindComponent("", &error));
  EXPECT_NE(nullptr, error.get());
  // 'comp2' doesn't exist:
  EXPECT_EQ(nullptr, manager_.FindComponent("comp2", nullptr));
  // 'comp1.comp2' is an array, not a component:
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2", nullptr));
  // 'comp1.comp2[3]' doesn't exist:
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2[3]", nullptr));
  // Empty component names:
  EXPECT_EQ(nullptr, manager_.FindComponent(".comp2[1]", nullptr));
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.[1]", nullptr));
  // Invalid array indices:
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2[s]", nullptr));
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2[-2]", nullptr));
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2[1e1]", nullptr));
  EXPECT_EQ(nullptr, manager_.FindComponent("comp1.comp2[1", nullptr));
}

TEST_F(ComponentManagerTest, ParseCommandInstance) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "viewer" }
      }
    },
    "trait2": {
      "commands": {
        "command1": { "minimalRole": "manager" },
        "command2": { "minimalRole": "owner" }
      }
    },
    "trait3": {
      "commands": {
        "command1": { "minimalRole": "manager" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp1", {"trait1"}, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp2", {"trait2"}, nullptr));

  std::string id;
  const char kCommand1[] = R"({
    "name": "trait1.command1",
    "id": "1234-12345",
    "component": "comp1",
    "parameters": {}
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  EXPECT_NE(nullptr,
            manager_.ParseCommandInstance(*command1, Command::Origin::kLocal,
                                          UserRole::kUser, &id, nullptr)
                .get());
  EXPECT_EQ("1234-12345", id);
  // Not enough access rights
  EXPECT_EQ(nullptr,
            manager_.ParseCommandInstance(*command1, Command::Origin::kLocal,
                                          UserRole::kViewer, &id, nullptr)
                .get());

  const char kCommand2[] = R"({
    "name": "trait1.command3",
    "component": "comp1",
    "parameters": {}
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  // trait1.command3 doesn't exist
  EXPECT_EQ(nullptr,
            manager_.ParseCommandInstance(*command2, Command::Origin::kLocal,
                                          UserRole::kOwner, &id, nullptr)
                .get());
  EXPECT_TRUE(id.empty());

  const char kCommand3[] = R"({
    "name": "trait2.command1",
    "component": "comp1",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  // Component comp1 doesn't have trait2.
  EXPECT_EQ(nullptr,
            manager_.ParseCommandInstance(*command3, Command::Origin::kLocal,
                                          UserRole::kOwner, &id, nullptr)
                .get());

  // No component specified, find the suitable component
  const char kCommand4[] = R"({
    "name": "trait1.command1",
    "parameters": {}
  })";
  auto command4 = CreateDictionaryValue(kCommand4);
  auto command_instance = manager_.ParseCommandInstance(
      *command4, Command::Origin::kLocal, UserRole::kOwner, &id, nullptr);
  EXPECT_NE(nullptr, command_instance.get());
  EXPECT_EQ("comp1", command_instance->GetComponent());

  const char kCommand5[] = R"({
    "name": "trait2.command1",
    "parameters": {}
  })";
  auto command5 = CreateDictionaryValue(kCommand5);
  command_instance = manager_.ParseCommandInstance(
      *command5, Command::Origin::kLocal, UserRole::kOwner, &id, nullptr);
  EXPECT_NE(nullptr, command_instance.get());
  EXPECT_EQ("comp2", command_instance->GetComponent());

  // Cannot route the command, no component with 'trait3'.
  const char kCommand6[] = R"({
    "name": "trait3.command1",
    "parameters": {}
  })";
  auto command6 = CreateDictionaryValue(kCommand6);
  EXPECT_EQ(nullptr,
            manager_.ParseCommandInstance(*command6, Command::Origin::kLocal,
                                          UserRole::kOwner, &id, nullptr)
                .get());
}

TEST_F(ComponentManagerTest, AddCommand) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp1", {"trait1"}, nullptr));

  std::string id;
  const char kCommand[] = R"({
    "name": "trait1.command1",
    "id": "1234-12345",
    "component": "comp1",
    "parameters": {}
  })";
  auto command = CreateDictionaryValue(kCommand);
  auto command_instance = manager_.ParseCommandInstance(
      *command, Command::Origin::kLocal, UserRole::kUser, &id, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  const auto* queued_command = manager_.FindCommand(id);
  ASSERT_NE(nullptr, queued_command);
  EXPECT_EQ("trait1.command1", queued_command->GetName());
}

TEST_F(ComponentManagerTest, AddCommandHandler) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" }
      }
    },
    "trait2": {
      "commands": {
        "command2": { "minimalRole": "user" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp1", {"trait1"}, nullptr));
  ASSERT_TRUE(
      manager_.AddComponent("", "comp2", {"trait1", "trait2"}, nullptr));

  std::string last_tags;
  auto handler = [&last_tags](int tag, const std::weak_ptr<Command>& command) {
    if (!last_tags.empty())
      last_tags += ',';
    last_tags += std::to_string(tag);
  };

  manager_.AddCommandHandler("comp1", "trait1.command1",
                             base::Bind(handler, 1));
  manager_.AddCommandHandler("comp2", "trait1.command1",
                             base::Bind(handler, 2));
  manager_.AddCommandHandler("comp2", "trait2.command2",
                             base::Bind(handler, 3));
  EXPECT_TRUE(last_tags.empty());

  const char kCommand1[] = R"({
    "name": "trait1.command1",
    "component": "comp1"
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  auto command_instance = manager_.ParseCommandInstance(
      *command1, Command::Origin::kCloud, UserRole::kUser, nullptr, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  EXPECT_EQ("1", last_tags);
  last_tags.clear();

  const char kCommand2[] = R"({
    "name": "trait1.command1",
    "component": "comp2"
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  command_instance = manager_.ParseCommandInstance(
      *command2, Command::Origin::kCloud, UserRole::kUser, nullptr, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  EXPECT_EQ("2", last_tags);
  last_tags.clear();

  const char kCommand3[] = R"({
    "name": "trait2.command2",
    "component": "comp2",
    "parameters": {}
  })";
  auto command3 = CreateDictionaryValue(kCommand3);
  command_instance = manager_.ParseCommandInstance(
      *command3, Command::Origin::kCloud, UserRole::kUser, nullptr, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  EXPECT_EQ("3", last_tags);
  last_tags.clear();
}

TEST_F(ComponentManagerTest, AddDefaultCommandHandler) {
  const char kTraits[] = R"({
    "trait1": {
      "commands": {
        "command1": { "minimalRole": "user" }
      }
    },
    "trait2": {
      "commands": {
        "command2": { "minimalRole": "user" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp", {"trait1", "trait2"}, nullptr));

  int count = 0;
  auto handler = [&count](int tag, const std::weak_ptr<Command>& command) {
    count++;
  };

  manager_.AddCommandHandler("", "", base::Bind(handler, 1));
  EXPECT_EQ(0, count);

  const char kCommand1[] = R"({
    "name": "trait1.command1",
    "component": "comp"
  })";
  auto command1 = CreateDictionaryValue(kCommand1);
  auto command_instance = manager_.ParseCommandInstance(
      *command1, Command::Origin::kCloud, UserRole::kUser, nullptr, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  EXPECT_EQ(1, count);

  const char kCommand2[] = R"({
    "name": "trait2.command2",
    "component": "comp"
  })";
  auto command2 = CreateDictionaryValue(kCommand2);
  command_instance = manager_.ParseCommandInstance(
      *command2, Command::Origin::kCloud, UserRole::kUser, nullptr, nullptr);
  ASSERT_NE(nullptr, command_instance.get());
  manager_.AddCommand(std::move(command_instance));
  EXPECT_EQ(2, count);
}

TEST_F(ComponentManagerTest, SetStateProperties) {
  CreateTestComponentTree(&manager_);

  const char kState1[] = R"({"t1": {"p1": 0, "p2": "foo"}})";
  auto state1 = CreateDictionaryValue(kState1);
  ASSERT_TRUE(manager_.SetStateProperties("comp1", *state1, nullptr));
  const char kExpected1[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": 0, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ]
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager_.GetComponents());

  const char kState2[] = R"({"t1": {"p1": {"bar": "baz"}}})";
  auto state2 = CreateDictionaryValue(kState2);
  ASSERT_TRUE(manager_.SetStateProperties("comp1", *state2, nullptr));

  const char kExpected2[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": {"bar": "baz"}, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ]
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager_.GetComponents());

  const char kState3[] = R"({"t5": {"p1": 1}})";
  auto state3 = CreateDictionaryValue(kState3);
  ASSERT_TRUE(manager_.SetStateProperties("comp1.comp2[1].comp3.comp4", *state3,
                                          nullptr));

  const char kExpected3[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "state": {"t1": {"p1": {"bar": "baz"}, "p2": "foo"}},
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ],
                    "state": { "t5": { "p1": 1 } }
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected3, manager_.GetComponents());
}

TEST_F(ComponentManagerTest, SetStatePropertiesFromJson) {
  CreateTestComponentTree(&manager_);

  ASSERT_TRUE(manager_.SetStatePropertiesFromJson(
      "comp1.comp2[1].comp3.comp4",
      R"({"t5": {"p1": 3}, "t6": {"p2": 5}})", nullptr));

  const char kExpected[] = R"({
    "comp1": {
      "traits": [ "t1" ],
      "components": {
        "comp2": [
          {
            "traits": [ "t2" ]
          },
          {
            "traits": [ "t3" ],
            "components": {
              "comp3": {
                "traits": [ "t4" ],
                "components": {
                  "comp4": {
                    "traits": [ "t5", "t6" ],
                    "state": {
                      "t5": { "p1": 3 },
                      "t6": { "p2": 5 }
                    }
                  }
                }
              }
            }
          }
        ]
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetComponents());
}

TEST_F(ComponentManagerTest, SetGetStateProperty) {
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "integer" }
      }
    },
    "trait2": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(
      manager_.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));

  base::StringValue p1("foo");
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop1", p1, nullptr));

  const char kExpected1[] = R"({
    "comp1": {
      "traits": [ "trait1", "trait2" ],
      "state": {
        "trait1": { "prop1": "foo" }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager_.GetComponents());

  base::FundamentalValue p2(2);
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait2.prop3", p2, nullptr));

  const char kExpected2[] = R"({
    "comp1": {
      "traits": [ "trait1", "trait2" ],
      "state": {
        "trait1": { "prop1": "foo" },
        "trait2": { "prop3": 2 }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager_.GetComponents());
  // Just the package name without property:
  EXPECT_FALSE(manager_.SetStateProperty("comp1", "trait2", p2, nullptr));

  const base::Value* value =
      manager_.GetStateProperty("comp1", "trait1.prop1", nullptr);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(p1.Equals(value));
  value = manager_.GetStateProperty("comp1", "trait2.prop3", nullptr);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(p2.Equals(value));

  // Non-existing property:
  EXPECT_EQ(nullptr, manager_.GetStateProperty("comp1", "trait2.p", nullptr));
  // Non-existing component
  EXPECT_EQ(nullptr, manager_.GetStateProperty("comp2", "trait.prop", nullptr));
  // Just the package name without property:
  EXPECT_EQ(nullptr, manager_.GetStateProperty("comp1", "trait2", nullptr));
}

TEST_F(ComponentManagerTest, AddStateChangedCallback) {
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp1", {"trait1"}, nullptr));

  int count = 0;
  int count2 = 0;
  manager_.AddStateChangedCallback(base::Bind([&count]() { count++; }));
  manager_.AddStateChangedCallback(base::Bind([&count2]() { count2++; }));
  EXPECT_EQ(1, count);
  EXPECT_EQ(1, count2);
  EXPECT_EQ(0u, manager_.GetLastStateChangeId());

  base::StringValue p1("foo");
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop1", p1, nullptr));
  EXPECT_EQ(2, count);
  EXPECT_EQ(2, count2);
  EXPECT_EQ(1u, manager_.GetLastStateChangeId());

  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop2", p1, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_EQ(3, count2);
  EXPECT_EQ(2u, manager_.GetLastStateChangeId());

  // Fail - no component.
  ASSERT_FALSE(manager_.SetStateProperty("comp2", "trait1.prop2", p1, nullptr));
  EXPECT_EQ(3, count);
  EXPECT_EQ(3, count2);
  EXPECT_EQ(2u, manager_.GetLastStateChangeId());
}

TEST_F(ComponentManagerTest, ComponentStateUpdates) {
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    },
    "trait2": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(
      manager_.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  ASSERT_TRUE(
      manager_.AddComponent("", "comp2", {"trait1", "trait2"}, nullptr));

  std::vector<ComponentManager::UpdateID> updates1;
  auto callback1 = [&updates1](ComponentManager::UpdateID id) {
    updates1.push_back(id);
  };
  // State change queue is empty, callback should be called immediately.
  auto token1 = manager_.AddServerStateUpdatedCallback(base::Bind(callback1));
  ASSERT_EQ(1u, updates1.size());
  EXPECT_EQ(manager_.GetLastStateChangeId(), updates1.front());
  updates1.clear();

  base::StringValue foo("foo");
  base::Time time1 = base::Time::Now();
  EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(time1));
  // These three updates should be grouped into two separate state change queue
  // items, since they all happen at the same time, but for two different
  // components.
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop1", foo, nullptr));
  ASSERT_TRUE(manager_.SetStateProperty("comp2", "trait2.prop3", foo, nullptr));
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop2", foo, nullptr));

  std::vector<ComponentManager::UpdateID> updates2;
  auto callback2 = [&updates2](ComponentManager::UpdateID id) {
    updates2.push_back(id);
  };
  // State change queue is not empty, so callback will be called later.
  auto token2 = manager_.AddServerStateUpdatedCallback(base::Bind(callback2));
  EXPECT_TRUE(updates2.empty());

  base::StringValue bar("bar");
  base::Time time2 = time1 + base::TimeDelta::FromSeconds(1);
  EXPECT_CALL(clock_, Now()).WillRepeatedly(Return(time2));
  // Two more update events (as above) but at |time2|.
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop1", bar, nullptr));
  ASSERT_TRUE(manager_.SetStateProperty("comp2", "trait2.prop3", bar, nullptr));
  ASSERT_TRUE(manager_.SetStateProperty("comp1", "trait1.prop2", bar, nullptr));

  auto snapshot = manager_.GetAndClearRecordedStateChanges();
  EXPECT_EQ(manager_.GetLastStateChangeId(), snapshot.update_id);
  ASSERT_EQ(4u, snapshot.state_changes.size());

  EXPECT_EQ("comp1", snapshot.state_changes[0].component);
  EXPECT_EQ(time1, snapshot.state_changes[0].timestamp);
  EXPECT_JSON_EQ(R"({"trait1":{"prop1":"foo","prop2":"foo"}})",
                 *snapshot.state_changes[0].changed_properties);

  EXPECT_EQ("comp2", snapshot.state_changes[1].component);
  EXPECT_EQ(time1, snapshot.state_changes[1].timestamp);
  EXPECT_JSON_EQ(R"({"trait2":{"prop3":"foo"}})",
                 *snapshot.state_changes[1].changed_properties);

  EXPECT_EQ("comp1", snapshot.state_changes[2].component);
  EXPECT_EQ(time2, snapshot.state_changes[2].timestamp);
  EXPECT_JSON_EQ(R"({"trait1":{"prop1":"bar","prop2":"bar"}})",
                 *snapshot.state_changes[2].changed_properties);

  EXPECT_EQ("comp2", snapshot.state_changes[3].component);
  EXPECT_EQ(time2, snapshot.state_changes[3].timestamp);
  EXPECT_JSON_EQ(R"({"trait2":{"prop3":"bar"}})",
                 *snapshot.state_changes[3].changed_properties);

  // Make sure previous GetAndClearRecordedStateChanges() clears the queue.
  auto snapshot2 = manager_.GetAndClearRecordedStateChanges();
  EXPECT_EQ(manager_.GetLastStateChangeId(), snapshot2.update_id);
  EXPECT_TRUE(snapshot2.state_changes.empty());

  // Now indicate that we have update the changes on the server.
  manager_.NotifyStateUpdatedOnServer(snapshot.update_id);
  ASSERT_EQ(1u, updates1.size());
  EXPECT_EQ(snapshot.update_id, updates1.front());
  ASSERT_EQ(1u, updates2.size());
  EXPECT_EQ(snapshot.update_id, updates2.front());
}

TEST_F(ComponentManagerTest, FindComponentWithTrait) {
  const char kTraits[] = R"({
    "trait1": {},
    "trait2": {},
    "trait3": {}
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(
      manager_.AddComponent("", "comp1", {"trait1", "trait2"}, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp2", {"trait3"}, nullptr));

  EXPECT_EQ("comp1", manager_.FindComponentWithTrait("trait1"));
  EXPECT_EQ("comp1", manager_.FindComponentWithTrait("trait2"));
  EXPECT_EQ("comp2", manager_.FindComponentWithTrait("trait3"));
  EXPECT_EQ("", manager_.FindComponentWithTrait("trait4"));
}

TEST_F(ComponentManagerTest, AddLegacyCommandAndStateDefinitions) {
  const char kCommandDefs1[] = R"({
    "package1": {
      "command1": {
        "minimalRole": "user",
        "parameters": {"height": {"type": "integer"}}
      },
      "command2": {
        "minimalRole": "owner",
        "parameters": {}
      }
    },
    "package2": {
      "command1": { "minimalRole": "user" },
      "command2": { "minimalRole": "owner" }
    }
  })";
  auto json = CreateDictionaryValue(kCommandDefs1);
  EXPECT_TRUE(manager_.AddLegacyCommandDefinitions(*json, nullptr));
  const char kExpected1[] = R"({
    "package1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        },
        "command2": {
          "minimalRole": "owner",
          "parameters": {}
        }
      }
    },
    "package2": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected1, manager_.GetTraits());
  const char kExpectedComponents1[] = R"({
    "__weave__": { "traits": ["package1", "package2"] }
  })";
  EXPECT_JSON_EQ(kExpectedComponents1, manager_.GetComponents());

  const char kCommandDefs2[] = R"({
    "package2": {
      "command3": { "minimalRole": "user" }
    },
    "package3": {
      "command1": { "minimalRole": "user" },
      "command2": { "minimalRole": "owner" }
    }
  })";
  json = CreateDictionaryValue(kCommandDefs2);
  EXPECT_TRUE(manager_.AddLegacyCommandDefinitions(*json, nullptr));
  const char kExpected2[] = R"({
    "package1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        },
        "command2": {
          "minimalRole": "owner",
          "parameters": {}
        }
      }
    },
    "package2": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "owner" },
        "command3": { "minimalRole": "user" }
      }
    },
    "package3": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "owner" }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected2, manager_.GetTraits());
  const char kExpectedComponents2[] = R"({
    "__weave__": { "traits": ["package1", "package2", "package3"] }
  })";
  EXPECT_JSON_EQ(kExpectedComponents2, manager_.GetComponents());

  // Redefining existing commands.
  EXPECT_FALSE(manager_.AddLegacyCommandDefinitions(*json, nullptr));

  const char kStateDefs1[] = R"({
    "package1": {
      "prop1": { "type": "string" },
      "prop2": { "type": "string" }
    },
    "package4": {
      "prop3": { "type": "string" },
      "prop4": { "type": "string" }
    }
  })";
  json = CreateDictionaryValue(kStateDefs1);
  EXPECT_TRUE(manager_.AddLegacyStateDefinitions(*json, nullptr));
  const char kExpectedComponents3[] = R"({
    "__weave__": { "traits": ["package1", "package2", "package3", "package4"] }
  })";
  EXPECT_JSON_EQ(kExpectedComponents3, manager_.GetComponents());

  const char kExpected3[] = R"({
    "package1": {
      "commands": {
        "command1": {
          "minimalRole": "user",
          "parameters": {"height": {"type": "integer"}}
        },
        "command2": {
          "minimalRole": "owner",
          "parameters": {}
        }
      },
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    },
    "package2": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "owner" },
        "command3": { "minimalRole": "user" }
      }
    },
    "package3": {
      "commands": {
        "command1": { "minimalRole": "user" },
        "command2": { "minimalRole": "owner" }
      }
    },
    "package4": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  EXPECT_JSON_EQ(kExpected3, manager_.GetTraits());
  const char kExpectedComponents4[] = R"({
    "__weave__": { "traits": ["package1", "package2", "package3", "package4"] }
  })";
  EXPECT_JSON_EQ(kExpectedComponents4, manager_.GetComponents());

  // Redefining existing commands.
  EXPECT_FALSE(manager_.AddLegacyStateDefinitions(*json, nullptr));

  const char kExpected4[] = R"({
    "package1": {
      "command1": {
        "minimalRole": "user",
        "parameters": {"height": {"type": "integer"}}
      },
      "command2": {
        "minimalRole": "owner",
        "parameters": {}
      }
    },
    "package2": {
      "command1": { "minimalRole": "user" },
      "command2": { "minimalRole": "owner" },
      "command3": { "minimalRole": "user" }
    },
    "package3": {
      "command1": { "minimalRole": "user" },
      "command2": { "minimalRole": "owner" }
    }
  })";
  EXPECT_JSON_EQ(kExpected4, manager_.GetLegacyCommandDefinitions());
}

TEST_F(ComponentManagerTest, GetLegacyState) {
  const char kTraits[] = R"({
    "trait1": {
      "state": {
        "prop1": { "type": "string" },
        "prop2": { "type": "string" }
      }
    },
    "trait2": {
      "state": {
        "prop3": { "type": "string" },
        "prop4": { "type": "string" }
      }
    }
  })";
  auto traits = CreateDictionaryValue(kTraits);
  ASSERT_TRUE(manager_.LoadTraits(*traits, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp1", {"trait1"}, nullptr));
  ASSERT_TRUE(manager_.AddComponent("", "comp2", {"trait2"}, nullptr));

  ASSERT_TRUE(manager_.SetStatePropertiesFromJson(
      "comp1", R"({"trait1": {"prop1": "foo", "prop2": "bar"}})", nullptr));
  ASSERT_TRUE(manager_.SetStatePropertiesFromJson(
      "comp2", R"({"trait2": {"prop3": "baz", "prop4": "quux"}})", nullptr));

  const char kExpected[] = R"({
    "trait1": {
      "prop1": "foo",
      "prop2": "bar"
    },
    "trait2": {
      "prop3": "baz",
      "prop4": "quux"
    }
  })";
  EXPECT_JSON_EQ(kExpected, manager_.GetLegacyState());
}

TEST_F(ComponentManagerTest, TestMockComponentManager) {
  // Check that all the virtual methods are mocked out.
  MockComponentManager mock;
}

}  // namespace weave
