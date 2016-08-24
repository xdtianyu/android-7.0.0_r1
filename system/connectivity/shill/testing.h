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

#ifndef SHILL_TESTING_H_
#define SHILL_TESTING_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/error.h"
#include "shill/key_value_store.h"
#include "shill/logging.h"

namespace shill {

// A Google Mock action (similar to testing::ReturnPointee) that takes a pointer
// to a unique_ptr object, releases and returns the raw pointer managed by the
// unique_ptr object when the action is invoked.
//
// Example usage:
//
//   TEST(FactoryTest, CreateStuff) {
//     MockFactory factory;
//     unique_ptr<Stuff> stuff(new Stuff());
//     EXPECT_CALL(factory, CreateStuff())
//         .WillOnce(ReturnAndReleasePointee(&stuff));
//   }
//
// If |factory.CreateStuff()| is called, the ownership of the Stuff object
// managed by |stuff| is transferred to the caller of |factory.CreateStuff()|.
// Otherwise, the Stuff object will be destroyed once |stuff| goes out of
// scope when the test completes.
ACTION_P(ReturnAndReleasePointee, unique_pointer) {
  return unique_pointer->release();
}

MATCHER(IsSuccess, "") {
  return arg.IsSuccess();
}

MATCHER(IsFailure, "") {
  return arg.IsFailure();
}

MATCHER_P2(ErrorIs, error_type, error_message, "") {
  return error_type == arg.type() && error_message == arg.message();
}

MATCHER_P(ErrorTypeIs, error_type, "") {
  return error_type == arg.type();
}

MATCHER(IsNullRefPtr, "") {
  return !arg.get();
}

MATCHER(NotNullRefPtr, "") {
  return arg.get();
}

// Use this matcher instead of passing RefPtrs directly into the arguments
// of EXPECT_CALL() because otherwise we may create un-cleaned-up references at
// system teardown.
MATCHER_P(IsRefPtrTo, ref_address, "") {
  return arg.get() == ref_address;
}

MATCHER_P(KeyValueStoreEq, value, "") {
  bool match = value.properties() == arg.properties();
  if (!match) {
    *result_listener << "\nExpected KeyValueStore:\n"
                     << "\tproperties: "
                     << testing::PrintToString(value.properties());
  }
  return match;
}

template<int error_argument_index>
class SetErrorTypeInArgumentAction {
 public:
  SetErrorTypeInArgumentAction(Error::Type error_type, bool warn_default)
      : error_type_(error_type),
        warn_default_(warn_default) {}

  template <typename Result, typename ArgumentTuple>
  Result Perform(const ArgumentTuple& args) const {
    Error* error_arg = ::std::tr1::get<error_argument_index>(args);
    if (error_arg)
      error_arg->Populate(error_type_);

    // You should be careful if you see this warning in your log messages: it is
    // likely that you want to instead set a non-default expectation on this
    // mock, to test the success code-paths.
    if (warn_default_)
      LOG(WARNING) << "Default action taken: set error to "
                   << error_type_
                   << "(" << (error_arg ? error_arg->message() : "") << ")";
  }

 private:
  Error::Type error_type_;
  bool warn_default_;
};

// Many functions in the the DBus proxy classes take a (shill::Error*) output
// argument that is set to shill::Error::kOperationFailed to notify the caller
// synchronously of error conditions.
//
// If an error is not returned synchronously, a callback (passed as another
// argument to the function) must eventually be called with the result/error.
// Mock classes for these proxies should by default return failure synchronously
// so that callers do not expect the callback to be called.
template<int error_argument_index>
::testing::PolymorphicAction<SetErrorTypeInArgumentAction<error_argument_index>>
SetOperationFailedInArgumentAndWarn() {
  return ::testing::MakePolymorphicAction(
      SetErrorTypeInArgumentAction<error_argument_index>(
          Error::kOperationFailed,
          true));
}

// Use this action to set the (shill::Error*) output argument to any
// shill::Error value on mock DBus proxy method calls.
template<int error_argument_index>
::testing::PolymorphicAction<SetErrorTypeInArgumentAction<error_argument_index>>
SetErrorTypeInArgument(Error::Type error_type) {
  return ::testing::MakePolymorphicAction(
      SetErrorTypeInArgumentAction<error_argument_index>(error_type, false));
}

}  // namespace shill

#endif  // SHILL_TESTING_H_
