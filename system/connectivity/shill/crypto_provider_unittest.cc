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

#include "shill/crypto_provider.h"

#include <string>

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <gtest/gtest.h>

#include "shill/crypto_des_cbc.h"
#include "shill/crypto_rot47.h"

using base::FilePath;
using base::ScopedTempDir;
using std::string;
using testing::Test;

namespace shill {

namespace {
const char kTestKey[] = "12345678";
const char kTestIV[] = "abcdefgh";
const char kKeyMatterFile[] = "key-matter-file";
const char kEmptyText[] = "";
const char kPlainText[] = "This is a test!";
const char kROT47Text[] = "rot47:%9:D :D 2 E6DEP";
const char kDESCBCText[] = "des-cbc:02:bKlHDISdHMFc0teQd4mAVrXgwlSj6iA+";
}  // namespace

class CryptoProviderTest : public Test {
 public:
  CryptoProviderTest() {}

 protected:
  FilePath InitKeyMatterFile(const FilePath& dir);

  CryptoProvider provider_;
};

FilePath CryptoProviderTest::InitKeyMatterFile(const FilePath& dir) {
  FilePath path = dir.Append(kKeyMatterFile);
  string matter = string(kTestIV) + kTestKey;
  base::WriteFile(path, matter.data(), matter.size());
  return path;
}

TEST_F(CryptoProviderTest, Init) {
  EXPECT_EQ(CryptoProvider::kKeyMatterFile, provider_.key_matter_file_.value());

  provider_.set_key_matter_file(FilePath("/some/non/existent/file"));
  provider_.Init();
  ASSERT_EQ(1, provider_.cryptos_.size());
  EXPECT_EQ(CryptoROT47::kID, provider_.cryptos_[0]->GetID());

  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  provider_.set_key_matter_file(InitKeyMatterFile(temp_dir.path()));
  provider_.Init();
  ASSERT_EQ(2, provider_.cryptos_.size());
  EXPECT_EQ(CryptoDESCBC::kID, provider_.cryptos_[0]->GetID());
  EXPECT_EQ(CryptoROT47::kID, provider_.cryptos_[1]->GetID());

  provider_.set_key_matter_file(FilePath("/other/missing/file"));
  provider_.Init();
  ASSERT_EQ(1, provider_.cryptos_.size());
  EXPECT_EQ(CryptoROT47::kID, provider_.cryptos_[0]->GetID());
}

TEST_F(CryptoProviderTest, Encrypt) {
  EXPECT_EQ(kPlainText, provider_.Encrypt(kPlainText));
  EXPECT_EQ(kEmptyText, provider_.Encrypt(kEmptyText));

  provider_.Init();
  EXPECT_EQ(kROT47Text, provider_.Encrypt(kPlainText));

  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  provider_.set_key_matter_file(InitKeyMatterFile(temp_dir.path()));
  provider_.Init();
  EXPECT_EQ(kROT47Text, provider_.Encrypt(kPlainText));
}

TEST_F(CryptoProviderTest, Decrypt) {
  EXPECT_EQ(kPlainText, provider_.Decrypt(kPlainText));
  EXPECT_EQ(kEmptyText, provider_.Decrypt(kEmptyText));

  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  provider_.set_key_matter_file(InitKeyMatterFile(temp_dir.path()));
  provider_.Init();
  EXPECT_EQ(kPlainText, provider_.Decrypt(kROT47Text));
  EXPECT_EQ(kPlainText, provider_.Decrypt(kDESCBCText));
  EXPECT_EQ(kPlainText, provider_.Decrypt(kPlainText));
  EXPECT_EQ(kEmptyText, provider_.Decrypt(kEmptyText));
}

}  // namespace shill
