//
// Copyright (C) 2011 The Android Open Source Project
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

#include "apmanager/error.h"

#include <string>

#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <gtest/gtest.h>

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

using std::string;
using testing::Test;

namespace apmanager {

class ErrorTest : public Test {
 public:
  ErrorTest() {}

  void PopulateError(Error* error, Error::Type type) {
    error->type_ = type;
  }

  void PopulateError(Error* error, Error::Type type, string message) {
    error->type_ = type;
    error->message_ = message;
  }

  void VerifyDBusError(Error::Type type, const string& expected_error_code) {
    static const std::string kMessage = "Test error message";
    Error e;
    PopulateError(&e, type, kMessage);
    brillo::ErrorPtr dbus_error;
    EXPECT_TRUE(e.ToDBusError(&dbus_error));
    EXPECT_NE(nullptr, dbus_error.get());
    EXPECT_EQ(brillo::errors::dbus::kDomain, dbus_error->GetDomain());
    EXPECT_EQ(expected_error_code, dbus_error->GetCode());
    EXPECT_EQ(kMessage, dbus_error->GetMessage());
  }
};

TEST_F(ErrorTest, Constructor) {
  Error e;
  EXPECT_EQ(Error::kSuccess, e.type());
}

TEST_F(ErrorTest, Reset) {
  Error e;
  PopulateError(&e, Error::kInternalError);
  EXPECT_TRUE(e.IsFailure());
  e.Reset();
  EXPECT_TRUE(e.IsSuccess());
}

TEST_F(ErrorTest, ToDBusError) {
  brillo::ErrorPtr dbus_error;

  // No error.
  EXPECT_EQ(nullptr, dbus_error.get());
  EXPECT_FALSE(Error().ToDBusError(&dbus_error));
  EXPECT_EQ(nullptr, dbus_error.get());

  VerifyDBusError(Error::kInternalError, kErrorInternalError);
  VerifyDBusError(Error::kInvalidArguments, kErrorInvalidArguments);
  VerifyDBusError(Error::kInvalidConfiguration, kErrorInvalidConfiguration);
}

}  // namespace shill
