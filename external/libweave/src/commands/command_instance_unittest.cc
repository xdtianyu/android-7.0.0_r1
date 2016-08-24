// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/commands/command_instance.h"

#include <gtest/gtest.h>
#include <weave/test/unittest_utils.h>

namespace weave {

using test::CreateDictionaryValue;
using test::CreateValue;

TEST(CommandInstanceTest, Test) {
  auto params = CreateDictionaryValue(R"({
    'phrase': 'iPityDaFool',
    'volume': 5
  })");
  CommandInstance instance{"robot.speak", Command::Origin::kCloud, *params};

  EXPECT_TRUE(
      instance.Complete(*CreateDictionaryValue("{'foo': 239}"), nullptr));

  EXPECT_EQ("", instance.GetID());
  EXPECT_EQ("robot.speak", instance.GetName());
  EXPECT_EQ(Command::Origin::kCloud, instance.GetOrigin());
  EXPECT_JSON_EQ("{'phrase': 'iPityDaFool', 'volume': 5}",
                 instance.GetParameters());
  EXPECT_JSON_EQ("{'foo': 239}", instance.GetResults());

  CommandInstance instance2{"base.reboot", Command::Origin::kLocal, {}};
  EXPECT_EQ(Command::Origin::kLocal, instance2.GetOrigin());
}

TEST(CommandInstanceTest, SetID) {
  CommandInstance instance{"base.reboot", Command::Origin::kLocal, {}};
  instance.SetID("command_id");
  EXPECT_EQ("command_id", instance.GetID());
}

TEST(CommandInstanceTest, FromJson) {
  auto json = CreateDictionaryValue(R"({
    'name': 'robot.jump',
    'component': 'comp1.comp2',
    'id': 'abcd',
    'parameters': {
      'height': 53,
      '_jumpType': '_withKick'
    },
    'results': {}
  })");
  std::string id;
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            &id, nullptr);
  EXPECT_EQ("abcd", id);
  EXPECT_EQ("abcd", instance->GetID());
  EXPECT_EQ("robot.jump", instance->GetName());
  EXPECT_EQ("comp1.comp2", instance->GetComponent());
  EXPECT_JSON_EQ("{'height': 53, '_jumpType': '_withKick'}",
                 instance->GetParameters());
}

TEST(CommandInstanceTest, FromJson_ParamsOmitted) {
  auto json = CreateDictionaryValue("{'name': 'base.reboot'}");
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, nullptr);
  EXPECT_EQ("base.reboot", instance->GetName());
  EXPECT_JSON_EQ("{}", instance->GetParameters());
}

TEST(CommandInstanceTest, FromJson_NotObject) {
  auto json = CreateValue("'string'");
  ErrorPtr error;
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, &error);
  EXPECT_EQ(nullptr, instance.get());
  EXPECT_EQ("json_object_expected", error->GetCode());
}

TEST(CommandInstanceTest, FromJson_NameMissing) {
  auto json = CreateDictionaryValue("{'param': 'value'}");
  ErrorPtr error;
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, &error);
  EXPECT_EQ(nullptr, instance.get());
  EXPECT_EQ("parameter_missing", error->GetCode());
}

TEST(CommandInstanceTest, FromJson_ParamsNotObject) {
  auto json = CreateDictionaryValue(R"({
    'name': 'robot.speak',
    'parameters': 'hello'
  })");
  ErrorPtr error;
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, &error);
  EXPECT_EQ(nullptr, instance.get());
  auto inner = error->GetInnerError();
  EXPECT_EQ("json_object_expected", inner->GetCode());
  EXPECT_EQ("command_failed", error->GetCode());
}

TEST(CommandInstanceTest, ToJson) {
  auto json = CreateDictionaryValue(R"({
    'name': 'robot.jump',
    'parameters': {
      'height': 53,
      '_jumpType': '_withKick'
    },
    'results': {}
  })");
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, nullptr);
  EXPECT_TRUE(instance->SetProgress(*CreateDictionaryValue("{'progress': 15}"),
                                    nullptr));
  EXPECT_TRUE(instance->SetProgress(*CreateDictionaryValue("{'progress': 15}"),
                                    nullptr));
  instance->SetID("testId");
  EXPECT_TRUE(instance->Complete(*CreateDictionaryValue("{'testResult': 17}"),
                                 nullptr));

  json->MergeDictionary(CreateDictionaryValue(R"({
    'id': 'testId',
    'progress': {'progress': 15},
    'state': 'done',
    'results': {'testResult': 17}
  })").get());

  auto converted = instance->ToJson();
  EXPECT_PRED2([](const base::Value& val1,
                  const base::Value& val2) { return val1.Equals(&val2); },
               *json, *converted);
}

TEST(CommandInstanceTest, ToJsonError) {
  auto json = CreateDictionaryValue(R"({
    'name': 'base.reboot',
    'parameters': {}
  })");
  auto instance = CommandInstance::FromJson(json.get(), Command::Origin::kCloud,
                                            nullptr, nullptr);
  instance->SetID("testId");

  ErrorPtr error;
  Error::AddTo(&error, FROM_HERE, "CODE", "MESSAGE");
  instance->Abort(error.get(), nullptr);

  json->MergeDictionary(CreateDictionaryValue(R"({
    'id': 'testId',
    'state': 'aborted',
    'progress': {},
    'results': {},
    'error': {'code': 'CODE', 'message': 'MESSAGE'}
  })").get());

  auto converted = instance->ToJson();
  EXPECT_PRED2([](const base::Value& val1,
                  const base::Value& val2) { return val1.Equals(&val2); },
               *json, *converted);
}

}  // namespace weave
