//
// Copyright (C) 2012 The Android Open Source Project
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

#include "shill/crypto_des_cbc.h"

#include <string>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

using base::FilePath;
using std::string;
using std::vector;
using testing::Test;

namespace shill {

namespace {
const char kTestKey[] = "12345678";
const char kTestIV[] = "abcdefgh";
const char kEmptyPlain[] = "";
const char kEmptyCipher[] = "02:4+O1a2KJVRM=";
const char kEmptyCipherNoSentinel[] = "02:lNRDa8O1tpM=";
const char kPlainText[] = "Hello world! ~123";
const char kCipherText[] = "02:MbxzeBqK3HVeS3xfjyhbe47Xx+szYgOp";
const char kPlainVersion1[] = "This is a test!";
const char kCipherVersion1[] = "bKlHDISdHMFfmfgBTT5I0w==";
}  // namespace

class CryptoDESCBCTest : public Test {
 public:
  CryptoDESCBCTest() {}

 protected:
  CryptoDESCBC crypto_;
};

TEST_F(CryptoDESCBCTest, GetID) {
  EXPECT_EQ(CryptoDESCBC::kID, crypto_.GetID());
}

TEST_F(CryptoDESCBCTest, LoadKeyMatter) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const char kKeyMatterFile[] = "key-matter-file";
  FilePath key_matter = temp_dir.path().Append(kKeyMatterFile);

  EXPECT_FALSE(crypto_.LoadKeyMatter(key_matter));
  EXPECT_TRUE(crypto_.key().empty());
  EXPECT_TRUE(crypto_.iv().empty());

  string matter = string(kTestIV) + kTestKey;

  base::WriteFile(key_matter, matter.data(), matter.size() - 1);
  EXPECT_FALSE(crypto_.LoadKeyMatter(key_matter));
  EXPECT_TRUE(crypto_.key().empty());
  EXPECT_TRUE(crypto_.iv().empty());

  base::WriteFile(key_matter, matter.data(), matter.size());
  EXPECT_TRUE(crypto_.LoadKeyMatter(key_matter));
  EXPECT_EQ(kTestKey, string(crypto_.key().begin(), crypto_.key().end()));
  EXPECT_EQ(kTestIV, string(crypto_.iv().begin(), crypto_.iv().end()));

  const char kKey2[] = "ABCDEFGH";
  const char kIV2[] = "87654321";
  matter = string("X") + kIV2 + kKey2;

  base::WriteFile(key_matter, matter.data(), matter.size());
  EXPECT_TRUE(crypto_.LoadKeyMatter(key_matter));
  EXPECT_EQ(kKey2, string(crypto_.key().begin(), crypto_.key().end()));
  EXPECT_EQ(kIV2, string(crypto_.iv().begin(), crypto_.iv().end()));

  base::WriteFile(key_matter, " ", 1);
  EXPECT_FALSE(crypto_.LoadKeyMatter(key_matter));
  EXPECT_TRUE(crypto_.key().empty());
  EXPECT_TRUE(crypto_.iv().empty());
}

TEST_F(CryptoDESCBCTest, Encrypt) {
  crypto_.key_.assign(kTestKey, kTestKey + strlen(kTestKey));
  crypto_.iv_.assign(kTestIV, kTestIV + strlen(kTestIV));

  string ciphertext;
  EXPECT_FALSE(crypto_.Encrypt(kPlainText, &ciphertext));
}

TEST_F(CryptoDESCBCTest, Decrypt) {
  crypto_.key_.assign(kTestKey, kTestKey + strlen(kTestKey));
  crypto_.iv_.assign(kTestIV, kTestIV + strlen(kTestIV));

  string plaintext;
  EXPECT_TRUE(crypto_.Decrypt(kEmptyCipher, &plaintext));
  EXPECT_EQ(kEmptyPlain, plaintext);
  EXPECT_TRUE(crypto_.Decrypt(kCipherText, &plaintext));
  EXPECT_EQ(kPlainText, plaintext);
  EXPECT_TRUE(crypto_.Decrypt(kCipherVersion1, &plaintext));
  EXPECT_EQ(kPlainVersion1, plaintext);

  EXPECT_FALSE(crypto_.Decrypt("random", &plaintext));
  EXPECT_FALSE(crypto_.Decrypt("02:random", &plaintext));
  EXPECT_FALSE(crypto_.Decrypt("~", &plaintext));
  EXPECT_FALSE(crypto_.Decrypt("02:~", &plaintext));
  EXPECT_FALSE(crypto_.Decrypt(kEmptyPlain, &plaintext));
  EXPECT_FALSE(crypto_.Decrypt(kEmptyCipherNoSentinel, &plaintext));

  // echo -n 12345678 | base64
  EXPECT_FALSE(crypto_.Decrypt("MTIzNDU2Nzg=", &plaintext));
}

}  // namespace shill
