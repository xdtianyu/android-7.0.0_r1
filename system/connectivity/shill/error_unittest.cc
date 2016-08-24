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

#include "shill/error.h"

#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <brillo/errors/error.h>
#include <brillo/errors/error_codes.h>
#include <gtest/gtest.h>

using testing::Test;

namespace shill {

class ErrorTest : public Test {};

TEST_F(ErrorTest, ConstructorDefault) {
  Error e;
  EXPECT_EQ(Error::kSuccess, e.type());
  EXPECT_EQ(Error::GetDefaultMessage(Error::kSuccess), e.message());
}

TEST_F(ErrorTest, ConstructorDefaultMessage) {
  Error e(Error::kAlreadyExists);
  EXPECT_EQ(Error::kAlreadyExists, e.type());
  EXPECT_EQ(Error::GetDefaultMessage(Error::kAlreadyExists), e.message());
}

TEST_F(ErrorTest, ConstructorCustomMessage) {
  static const char kMessage[] = "Custom error message";
  Error e(Error::kInProgress, kMessage);
  EXPECT_EQ(Error::kInProgress, e.type());
  EXPECT_EQ(kMessage, e.message());
}

TEST_F(ErrorTest, Reset) {
  Error e(Error::kAlreadyExists);
  e.Reset();
  EXPECT_EQ(Error::kSuccess, e.type());
  EXPECT_EQ(Error::GetDefaultMessage(Error::kSuccess), e.message());
}

TEST_F(ErrorTest, PopulateDefaultMessage) {
  Error e;
  e.Populate(Error::kInternalError);
  EXPECT_EQ(Error::kInternalError, e.type());
  EXPECT_EQ(Error::GetDefaultMessage(Error::kInternalError), e.message());
}

TEST_F(ErrorTest, PopulateCustomMessage) {
  static const char kMessage[] = "Another custom error message";
  Error e;
  e.Populate(Error::kInvalidArguments, kMessage);
  EXPECT_EQ(Error::kInvalidArguments, e.type());
  EXPECT_EQ(kMessage, e.message());
}

TEST_F(ErrorTest, CopyFrom) {
  Error e(Error::kInvalidArguments, "Some message");
  Error copy;
  copy.CopyFrom(e);
  EXPECT_EQ(e.type(), copy.type());
  EXPECT_EQ(e.message(), copy.message());
}

TEST_F(ErrorTest, ToChromeosError) {
  brillo::ErrorPtr chromeos_error;
  EXPECT_EQ(nullptr, chromeos_error.get());
  Error().ToChromeosError(&chromeos_error);
  EXPECT_EQ(nullptr, chromeos_error.get());
  static const std::string kMessage = "Test error message";
  Error(Error::kPermissionDenied, kMessage).ToChromeosError(&chromeos_error);
  EXPECT_NE(nullptr, chromeos_error.get());
  EXPECT_EQ(brillo::errors::dbus::kDomain, chromeos_error->GetDomain());
  EXPECT_EQ(kErrorResultPermissionDenied, chromeos_error->GetCode());
  EXPECT_EQ(kMessage, chromeos_error->GetMessage());
}

TEST_F(ErrorTest, IsSuccessFailure) {
  EXPECT_TRUE(Error().IsSuccess());
  EXPECT_FALSE(Error().IsFailure());
  EXPECT_FALSE(Error(Error::kInvalidNetworkName).IsSuccess());
  EXPECT_TRUE(Error(Error::kInvalidPassphrase).IsFailure());
}

TEST_F(ErrorTest, GetDBusResult) {
  // Make sure the Error::Type enum matches up to the Error::Info array.
  EXPECT_EQ(kErrorResultSuccess, Error::GetDBusResult(Error::kSuccess));
  EXPECT_EQ(kErrorResultFailure, Error::GetDBusResult(Error::kOperationFailed));
  EXPECT_EQ(kErrorResultAlreadyConnected,
            Error::GetDBusResult(Error::kAlreadyConnected));
  EXPECT_EQ(kErrorResultAlreadyExists,
            Error::GetDBusResult(Error::kAlreadyExists));
  EXPECT_EQ(kErrorResultIncorrectPin,
            Error::GetDBusResult(Error::kIncorrectPin));
  EXPECT_EQ(kErrorResultInProgress, Error::GetDBusResult(Error::kInProgress));
  EXPECT_EQ(kErrorResultInternalError,
            Error::GetDBusResult(Error::kInternalError));
  EXPECT_EQ(kErrorResultInvalidApn, Error::GetDBusResult(Error::kInvalidApn));
  EXPECT_EQ(kErrorResultInvalidArguments,
            Error::GetDBusResult(Error::kInvalidArguments));
  EXPECT_EQ(kErrorResultInvalidNetworkName,
            Error::GetDBusResult(Error::kInvalidNetworkName));
  EXPECT_EQ(kErrorResultInvalidPassphrase,
            Error::GetDBusResult(Error::kInvalidPassphrase));
  EXPECT_EQ(kErrorResultInvalidProperty,
            Error::GetDBusResult(Error::kInvalidProperty));
  EXPECT_EQ(kErrorResultNoCarrier, Error::GetDBusResult(Error::kNoCarrier));
  EXPECT_EQ(kErrorResultNotConnected,
            Error::GetDBusResult(Error::kNotConnected));
  EXPECT_EQ(kErrorResultNotFound, Error::GetDBusResult(Error::kNotFound));
  EXPECT_EQ(kErrorResultNotImplemented,
            Error::GetDBusResult(Error::kNotImplemented));
  EXPECT_EQ(kErrorResultNotOnHomeNetwork,
            Error::GetDBusResult(Error::kNotOnHomeNetwork));
  EXPECT_EQ(kErrorResultNotRegistered,
            Error::GetDBusResult(Error::kNotRegistered));
  EXPECT_EQ(kErrorResultNotSupported,
            Error::GetDBusResult(Error::kNotSupported));
  EXPECT_EQ(kErrorResultOperationAborted,
            Error::GetDBusResult(Error::kOperationAborted));
  EXPECT_EQ(kErrorResultOperationInitiated,
            Error::GetDBusResult(Error::kOperationInitiated));
  EXPECT_EQ(kErrorResultOperationTimeout,
            Error::GetDBusResult(Error::kOperationTimeout));
  EXPECT_EQ(kErrorResultPassphraseRequired,
            Error::GetDBusResult(Error::kPassphraseRequired));
  EXPECT_EQ(kErrorResultPermissionDenied,
            Error::GetDBusResult(Error::kPermissionDenied));
  EXPECT_EQ(kErrorResultPinBlocked, Error::GetDBusResult(Error::kPinBlocked));
  EXPECT_EQ(kErrorResultPinRequired, Error::GetDBusResult(Error::kPinRequired));
  EXPECT_EQ(kErrorResultWrongState, Error::GetDBusResult(Error::kWrongState));
}

TEST_F(ErrorTest, GetDefaultMessage) {
  // Check the last error code to try to prevent off-by-one bugs when adding or
  // removing error types.
  ASSERT_EQ(Error::kWrongState, Error::kNumErrors - 1);
  EXPECT_EQ("Permission denied",
            Error::GetDefaultMessage(Error::kPermissionDenied));
}

}  // namespace shill
