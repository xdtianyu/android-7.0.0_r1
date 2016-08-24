// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_

#include <weave/command.h>

#include <memory>
#include <string>

#include <base/values.h>
#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockCommand : public Command {
 public:
  ~MockCommand() override = default;

  MOCK_CONST_METHOD0(GetID, const std::string&());
  MOCK_CONST_METHOD0(GetName, const std::string&());
  MOCK_CONST_METHOD0(GetComponent, const std::string&());
  MOCK_CONST_METHOD0(GetState, Command::State());
  MOCK_CONST_METHOD0(GetOrigin, Command::Origin());
  MOCK_CONST_METHOD0(GetParameters, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetProgress, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetResults, const base::DictionaryValue&());
  MOCK_CONST_METHOD0(GetError, const Error*());
  MOCK_METHOD2(SetProgress, bool(const base::DictionaryValue&, ErrorPtr*));
  MOCK_METHOD2(Complete, bool(const base::DictionaryValue&, ErrorPtr*));
  MOCK_METHOD1(Pause, bool(ErrorPtr*));
  MOCK_METHOD2(SetError, bool(const Error*, ErrorPtr*));
  MOCK_METHOD2(Abort, bool(const Error*, ErrorPtr*));
  MOCK_METHOD1(Cancel, bool(ErrorPtr*));
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMAND_H_
