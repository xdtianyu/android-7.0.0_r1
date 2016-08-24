//
// Copyright (C) 2015 The Android Open Source Project
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

#include "attestation/common/mock_tpm_utility.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

namespace {

class TransformString {
 public:
  explicit TransformString(std::string method) : method_(method) {}
  bool operator()(const std::string& in, std::string* out) {
    *out = attestation::MockTpmUtility::Transform(method_, in);
    return true;
  }

 private:
  std::string method_;
};

class UntransformString {
 public:
  explicit UntransformString(std::string method) : method_(method) {}
  bool operator()(const std::string& in, std::string* out) {
    std::string suffix = "_fake_transform_" + method_;
    auto position = in.find(suffix);
    if (position == std::string::npos) {
      return false;
    }
    *out = in.substr(0, position);
    return true;
  }

 private:
  std::string method_;
};

}  // namespace

namespace attestation {

MockTpmUtility::MockTpmUtility() {
  ON_CALL(*this, IsTpmReady()).WillByDefault(Return(true));
  ON_CALL(*this, ActivateIdentity(_, _, _, _, _, _))
      .WillByDefault(Return(true));
  ON_CALL(*this, CreateCertifiedKey(_, _, _, _, _, _, _, _, _))
      .WillByDefault(Return(true));
  ON_CALL(*this, SealToPCR0(_, _))
      .WillByDefault(Invoke(TransformString("SealToPCR0")));
  ON_CALL(*this, Unseal(_, _))
      .WillByDefault(Invoke(UntransformString("SealToPCR0")));
  ON_CALL(*this, Unbind(_, _, _))
      .WillByDefault(WithArgs<1, 2>(Invoke(TransformString("Unbind"))));
  ON_CALL(*this, Sign(_, _, _))
      .WillByDefault(WithArgs<1, 2>(Invoke(TransformString("Sign"))));
}

MockTpmUtility::~MockTpmUtility() {}

// static
std::string MockTpmUtility::Transform(const std::string& method,
                                      const std::string& input) {
  return input + "_fake_transform_" + method;
}

}  // namespace attestation
