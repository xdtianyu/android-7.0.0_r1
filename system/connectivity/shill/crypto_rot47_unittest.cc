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

#include "shill/crypto_rot47.h"

#include <string>

#include <gtest/gtest.h>

using std::string;
using testing::Test;

namespace shill {

namespace {
const char kEmpty[] = "";
const char kPlainText[] = "~{\"Hello world!\" OPQ ['1234']}";
const char kCipherText[] = "OLQw6==@ H@C=5PQ ~!\" ,V`abcV.N";
}  // namespace

class CryptoROT47Test : public Test {
 protected:
  CryptoROT47 crypto_;
};

TEST_F(CryptoROT47Test, GetID) {
  EXPECT_EQ(CryptoROT47::kID, crypto_.GetID());
}

TEST_F(CryptoROT47Test, Encrypt) {
  string text;
  EXPECT_TRUE(crypto_.Encrypt(kPlainText, &text));
  EXPECT_EQ(kCipherText, text);
  EXPECT_TRUE(crypto_.Encrypt(kEmpty, &text));
  EXPECT_EQ(kEmpty, text);
}

TEST_F(CryptoROT47Test, Decrypt) {
  string text;
  EXPECT_TRUE(crypto_.Decrypt(kCipherText, &text));
  EXPECT_EQ(kPlainText, text);
  EXPECT_TRUE(crypto_.Decrypt(kEmpty, &text));
  EXPECT_EQ(kEmpty, text);
}

}  // namespace shill
