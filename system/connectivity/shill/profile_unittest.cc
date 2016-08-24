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

#include "shill/profile.h"

#include <string>
#include <vector>

#include <base/files/file_util.h>
#include <base/strings/stringprintf.h>
#include <base/strings/string_util.h>
#include <gtest/gtest.h>

#include "shill/fake_store.h"
#include "shill/mock_manager.h"
#include "shill/mock_metrics.h"
#include "shill/mock_profile.h"
#include "shill/mock_service.h"
#include "shill/mock_store.h"
#include "shill/property_store_unittest.h"
#include "shill/service_under_test.h"
#include "shill/store_factory.h"

using base::FilePath;
using std::set;
using std::string;
using std::vector;
using testing::_;
using testing::Invoke;
using testing::Mock;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrictMock;

namespace shill {

class ProfileTest : public PropertyStoreTest {
 public:
  ProfileTest() : mock_metrics_(new MockMetrics(nullptr)) {
    Profile::Identifier id("rather", "irrelevant");
    profile_ = new Profile(
        control_interface(), metrics(), manager(), id, FilePath(), false);

    // Install a FakeStore by default. In tests that actually care
    // about the interaction between Profile and StoreInterface, we'll
    // replace this with a MockStore.
    profile_->set_storage(new FakeStore());
  }

  MockService* CreateMockService() {
    return new StrictMock<MockService>(control_interface(),
                                       dispatcher(),
                                       metrics(),
                                       manager());
  }

  bool ProfileInitStorage(const Profile::Identifier& id,
                          Profile::InitStorageOption storage_option,
                          bool save,
                          Error::Type error_type) {
    // Note: this code uses neither FakeStore, nor MockStore. Instead,
    // it exercises a real StoreInterface implemenation.
    Error error;
    ProfileRefPtr profile(
        new Profile(control_interface(), mock_metrics_.get(), manager(), id,
                    FilePath(storage_path()), false));
    bool ret = profile->InitStorage(storage_option, &error);
    EXPECT_EQ(error_type, error.type());
    if (ret && save) {
      EXPECT_TRUE(profile->Save());
    }
    return ret;
  }

 protected:
  std::unique_ptr<MockMetrics> mock_metrics_;
  ProfileRefPtr profile_;
};

TEST_F(ProfileTest, DeleteEntry) {
  std::unique_ptr<MockManager> manager(new StrictMock<MockManager>(
      control_interface(), dispatcher(), metrics()));
  profile_->manager_ = manager.get();

  MockStore* storage(new StrictMock<MockStore>());
  profile_->storage_.reset(storage);  // Passes ownership
  const string kEntryName("entry_name");

  // If entry does not appear in storage, DeleteEntry() should return an error.
  EXPECT_CALL(*storage, ContainsGroup(kEntryName))
      .WillOnce(Return(false));
  {
    Error error;
    profile_->DeleteEntry(kEntryName, &error);
    EXPECT_EQ(Error::kNotFound, error.type());
  }

  Mock::VerifyAndClearExpectations(storage);

  // If HandleProfileEntryDeletion() returns false, Profile should call
  // DeleteGroup() itself.
  EXPECT_CALL(*storage, ContainsGroup(kEntryName))
      .WillOnce(Return(true));
  EXPECT_CALL(*manager.get(), HandleProfileEntryDeletion(_, kEntryName))
      .WillOnce(Return(false));
  EXPECT_CALL(*storage, DeleteGroup(kEntryName))
      .WillOnce(Return(true));
  EXPECT_CALL(*storage, Flush())
      .WillOnce(Return(true));
  {
    Error error;
    profile_->DeleteEntry(kEntryName, &error);
    EXPECT_TRUE(error.IsSuccess());
  }

  Mock::VerifyAndClearExpectations(storage);

  // If HandleProfileEntryDeletion() returns true, Profile should not call
  // DeleteGroup() itself.
  EXPECT_CALL(*storage, ContainsGroup(kEntryName))
      .WillOnce(Return(true));
  EXPECT_CALL(*manager.get(), HandleProfileEntryDeletion(_, kEntryName))
      .WillOnce(Return(true));
  EXPECT_CALL(*storage, DeleteGroup(kEntryName))
      .Times(0);
  EXPECT_CALL(*storage, Flush())
      .WillOnce(Return(true));
  {
    Error error;
    profile_->DeleteEntry(kEntryName, &error);
    EXPECT_TRUE(error.IsSuccess());
  }
}

TEST_F(ProfileTest, IsValidIdentifierToken) {
  EXPECT_FALSE(Profile::IsValidIdentifierToken(""));
  EXPECT_FALSE(Profile::IsValidIdentifierToken(" "));
  EXPECT_FALSE(Profile::IsValidIdentifierToken("-"));
  EXPECT_FALSE(Profile::IsValidIdentifierToken("~"));
  EXPECT_FALSE(Profile::IsValidIdentifierToken("_"));
  EXPECT_TRUE(Profile::IsValidIdentifierToken("a"));
  EXPECT_TRUE(Profile::IsValidIdentifierToken("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  EXPECT_TRUE(Profile::IsValidIdentifierToken("abcdefghijklmnopqrstuvwxyz"));
  EXPECT_TRUE(Profile::IsValidIdentifierToken("0123456789"));
}

TEST_F(ProfileTest, ParseIdentifier) {
  Profile::Identifier identifier;
  EXPECT_FALSE(Profile::ParseIdentifier("", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~foo", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~/", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~bar/", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~/zoo", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~./moo", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~valid/?", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~no//no", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("~no~no", &identifier));

  static const char kUser[] = "user";
  static const char kIdentifier[] = "identifier";
  EXPECT_TRUE(Profile::ParseIdentifier(
      base::StringPrintf("~%s/%s", kUser, kIdentifier),
      &identifier));
  EXPECT_EQ(kUser, identifier.user);
  EXPECT_EQ(kIdentifier, identifier.identifier);

  EXPECT_FALSE(Profile::ParseIdentifier("!", &identifier));
  EXPECT_FALSE(Profile::ParseIdentifier("/nope", &identifier));

  static const char kIdentifier2[] = "something";
  EXPECT_TRUE(Profile::ParseIdentifier(kIdentifier2, &identifier));
  EXPECT_EQ("", identifier.user);
  EXPECT_EQ(kIdentifier2, identifier.identifier);
}

TEST_F(ProfileTest, IdentifierToString) {
  Profile::Identifier identifier;
  static const char kUser[] = "user";
  static const char kIdentifier[] = "identifier";
  identifier.user = kUser;
  identifier.identifier = kIdentifier;
  EXPECT_EQ(base::StringPrintf("~%s/%s", kUser, kIdentifier),
            Profile::IdentifierToString(identifier));
}

TEST_F(ProfileTest, GetFriendlyName) {
  static const char kUser[] = "theUser";
  static const char kIdentifier[] = "theIdentifier";
  Profile::Identifier id(kIdentifier);
  ProfileRefPtr profile(new Profile(
      control_interface(), metrics(), manager(), id, FilePath(), false));
  EXPECT_EQ(kIdentifier, profile->GetFriendlyName());
  id.user = kUser;
  profile = new Profile(
      control_interface(), metrics(), manager(), id, FilePath(), false);
  EXPECT_EQ(string(kUser) + "/" + kIdentifier, profile->GetFriendlyName());
}

TEST_F(ProfileTest, GetStoragePath) {
  static const char kUser[] = "chronos";
  static const char kIdentifier[] = "someprofile";
  static const char kDirectory[] = "/a/place/for/";
  Profile::Identifier id(kIdentifier);
  ProfileRefPtr profile(new Profile(
      control_interface(), metrics(), manager(), id, FilePath(), false));
  EXPECT_TRUE(profile->persistent_profile_path_.empty());
  id.user = kUser;
  profile = new Profile(
      control_interface(), metrics(), manager(), id, FilePath(kDirectory),
      false);
#if defined(ENABLE_JSON_STORE)
  EXPECT_EQ("/a/place/for/chronos/someprofile.profile.json",
            profile->persistent_profile_path_.value());
#else
  EXPECT_EQ("/a/place/for/chronos/someprofile.profile",
            profile->persistent_profile_path_.value());
#endif
}

TEST_F(ProfileTest, ServiceManagement) {
  scoped_refptr<MockService> service1(CreateMockService());
  scoped_refptr<MockService> service2(CreateMockService());

  EXPECT_CALL(*service1.get(), Save(_))
      .WillRepeatedly(Invoke(service1.get(), &MockService::FauxSave));
  EXPECT_CALL(*service2.get(), Save(_))
      .WillRepeatedly(Invoke(service2.get(), &MockService::FauxSave));

  ASSERT_TRUE(profile_->AdoptService(service1));
  ASSERT_TRUE(profile_->AdoptService(service2));

  // Ensure services are in the profile now.
  ASSERT_TRUE(profile_->ContainsService(service1));
  ASSERT_TRUE(profile_->ContainsService(service2));

  // Ensure we can't add them twice.
  ASSERT_FALSE(profile_->AdoptService(service1));
  ASSERT_FALSE(profile_->AdoptService(service2));

  // Ensure that we can abandon individually, and that doing so is idempotent.
  ASSERT_TRUE(profile_->AbandonService(service1));
  ASSERT_FALSE(profile_->ContainsService(service1));
  ASSERT_TRUE(profile_->AbandonService(service1));
  ASSERT_TRUE(profile_->ContainsService(service2));

  // Clean up.
  ASSERT_TRUE(profile_->AbandonService(service2));
  ASSERT_FALSE(profile_->ContainsService(service1));
  ASSERT_FALSE(profile_->ContainsService(service2));
}

TEST_F(ProfileTest, ServiceConfigure) {
  ServiceRefPtr service1(new ServiceUnderTest(control_interface(),
                                              dispatcher(),
                                              metrics(),
                                              manager()));
  // Change priority from default.
  service1->SetPriority(service1->priority() + 1, nullptr);
  ASSERT_TRUE(profile_->AdoptService(service1));
  ASSERT_TRUE(profile_->ContainsService(service1));

  // Create new service; ask Profile to merge it with a known, matching,
  // service; ensure that settings from |service1| wind up in |service2|.
  ServiceRefPtr service2(new ServiceUnderTest(control_interface(),
                                              dispatcher(),
                                              metrics(),
                                              manager()));
  int32_t orig_priority = service2->priority();
  ASSERT_TRUE(profile_->ConfigureService(service2));
  ASSERT_EQ(service1->priority(), service2->priority());
  ASSERT_NE(orig_priority, service2->priority());

  // Clean up.
  ASSERT_TRUE(profile_->AbandonService(service1));
  ASSERT_FALSE(profile_->ContainsService(service1));
  ASSERT_FALSE(profile_->ContainsService(service2));
}

TEST_F(ProfileTest, Save) {
  scoped_refptr<MockService> service1(CreateMockService());
  scoped_refptr<MockService> service2(CreateMockService());
  EXPECT_CALL(*service1.get(), Save(_)).WillOnce(Return(true));
  EXPECT_CALL(*service2.get(), Save(_)).WillOnce(Return(true));

  ASSERT_TRUE(profile_->AdoptService(service1));
  ASSERT_TRUE(profile_->AdoptService(service2));

  profile_->Save();
}

TEST_F(ProfileTest, EntryEnumeration) {
  scoped_refptr<MockService> service1(CreateMockService());
  scoped_refptr<MockService> service2(CreateMockService());
  string service1_storage_name = Technology::NameFromIdentifier(
      Technology::kCellular) + "_1";
  string service2_storage_name = Technology::NameFromIdentifier(
      Technology::kCellular) + "_2";
  EXPECT_CALL(*service1.get(), Save(_))
      .WillRepeatedly(Invoke(service1.get(), &MockService::FauxSave));
  EXPECT_CALL(*service2.get(), Save(_))
      .WillRepeatedly(Invoke(service2.get(), &MockService::FauxSave));
  EXPECT_CALL(*service1.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(service1_storage_name));
  EXPECT_CALL(*service2.get(), GetStorageIdentifier())
      .WillRepeatedly(Return(service2_storage_name));

  string service1_name(service1->unique_name());
  string service2_name(service2->unique_name());

  ASSERT_TRUE(profile_->AdoptService(service1));
  ASSERT_TRUE(profile_->AdoptService(service2));

  Error error;
  ASSERT_EQ(2, profile_->EnumerateEntries(&error).size());

  ASSERT_TRUE(profile_->AbandonService(service1));
  ASSERT_EQ(service2_storage_name, profile_->EnumerateEntries(&error)[0]);

  ASSERT_TRUE(profile_->AbandonService(service2));
  ASSERT_EQ(0, profile_->EnumerateEntries(&error).size());
}

TEST_F(ProfileTest, LoadUserProfileList) {
  FilePath list_path(FilePath(storage_path()).Append("test.profile"));
  vector<Profile::Identifier> identifiers =
      Profile::LoadUserProfileList(list_path);
  EXPECT_TRUE(identifiers.empty());

  const char kUser0[] = "scarecrow";
  const char kUser1[] = "jeans";
  const char kIdentifier0[] = "rattlesnake";
  const char kIdentifier1[] = "ceiling";
  const char kHash0[] = "neighbors";
  string data(base::StringPrintf("\n"
                                 "~userbut/nospacehere\n"
                                 "defaultprofile notaccepted\n"
                                 "~%s/%s %s\n"
                                 "~userbutno/hash\n"
                                 " ~dontaccept/leadingspaces hash\n"
                                 "~this_username_fails_to_parse/id hash\n"
                                 "~%s/%s \n\n",
                                 kUser0, kIdentifier0, kHash0,
                                 kUser1, kIdentifier1));
  EXPECT_EQ(data.size(), base::WriteFile(list_path, data.data(), data.size()));
  identifiers = Profile::LoadUserProfileList(list_path);
  EXPECT_EQ(2, identifiers.size());
  EXPECT_EQ(kUser0, identifiers[0].user);
  EXPECT_EQ(kIdentifier0, identifiers[0].identifier);
  EXPECT_EQ(kHash0, identifiers[0].user_hash);
  EXPECT_EQ(kUser1, identifiers[1].user);
  EXPECT_EQ(kIdentifier1, identifiers[1].identifier);
  EXPECT_EQ("", identifiers[1].user_hash);
}

TEST_F(ProfileTest, SaveUserProfileList) {
  const char kUser0[] = "user0";
  const char kIdentifier0[] = "id0";
  Profile::Identifier id0(kUser0, kIdentifier0);
  const char kHash0[] = "hash0";
  id0.user_hash = kHash0;
  vector<ProfileRefPtr> profiles;
  profiles.push_back(new Profile(
      control_interface(), metrics(), manager(), id0, FilePath(), false));

  const char kUser1[] = "user1";
  const char kIdentifier1[] = "id1";
  Profile::Identifier id1(kUser1, kIdentifier1);
  const char kHash1[] = "hash1";
  id1.user_hash = kHash1;
  profiles.push_back(new Profile(
      control_interface(), metrics(), manager(), id1, FilePath(), false));


  const char kIdentifier2[] = "id2";
  Profile::Identifier id2("", kIdentifier2);
  const char kHash2[] = "hash2";
  id1.user_hash = kHash2;
  profiles.push_back(new Profile(
      control_interface(), metrics(), manager(), id2, FilePath(), false));

  FilePath list_path(FilePath(storage_path()).Append("test.profile"));
  EXPECT_TRUE(Profile::SaveUserProfileList(list_path, profiles));

  string profile_data;
  EXPECT_TRUE(base::ReadFileToString(list_path, &profile_data));
  EXPECT_EQ(base::StringPrintf("~%s/%s %s\n~%s/%s %s\n",
                               kUser0, kIdentifier0, kHash0,
                               kUser1, kIdentifier1, kHash1),
            profile_data);
}

TEST_F(ProfileTest, MatchesIdentifier) {
  static const char kUser[] = "theUser";
  static const char kIdentifier[] = "theIdentifier";
  Profile::Identifier id(kUser, kIdentifier);
  ProfileRefPtr profile(new Profile(
      control_interface(), metrics(), manager(), id, FilePath(), false));
  EXPECT_TRUE(profile->MatchesIdentifier(id));
  EXPECT_FALSE(profile->MatchesIdentifier(Profile::Identifier(kUser, "")));
  EXPECT_FALSE(
      profile->MatchesIdentifier(Profile::Identifier("", kIdentifier)));
  EXPECT_FALSE(
      profile->MatchesIdentifier(Profile::Identifier(kIdentifier, kUser)));
}

TEST_F(ProfileTest, InitStorage) {
  Profile::Identifier id("theUser", "theIdentifier");
  ASSERT_TRUE(base::CreateDirectory(
      base::FilePath(storage_path()).Append("theUser")));

  // Profile doesn't exist but we wanted it to.
  EXPECT_FALSE(ProfileInitStorage(id, Profile::kOpenExisting, false,
                                  Error::kNotFound));

  // Success case, with a side effect of creating the profile.
  EXPECT_TRUE(ProfileInitStorage(id, Profile::kCreateNew, true,
                                 Error::kSuccess));

  // The results from our two test cases above will now invert since
  // the profile now exists.  First, we now succeed if we require that
  // the profile already exist...
  EXPECT_TRUE(ProfileInitStorage(id, Profile::kOpenExisting, false,
                                 Error::kSuccess));

  // And we fail if we require that it doesn't.
  EXPECT_FALSE(ProfileInitStorage(id, Profile::kCreateNew, false,
                                  Error::kAlreadyExists));

  // As a sanity check, ensure "create or open" works for both profile-exists...
  EXPECT_TRUE(ProfileInitStorage(id, Profile::kCreateOrOpenExisting, false,
                                 Error::kSuccess));

  // ...and for a new profile that doesn't exist.
  Profile::Identifier id2("theUser", "theIdentifier2");
  // Let's just make double-check that this profile really doesn't exist.
  ASSERT_FALSE(ProfileInitStorage(id2, Profile::kOpenExisting, false,
                                  Error::kNotFound));

  // Then test that with "create or open" we succeed.
  EXPECT_TRUE(ProfileInitStorage(id2, Profile::kCreateOrOpenExisting, false,
                                 Error::kSuccess));

  // Corrupt the profile storage.
  FilePath final_path(
      base::StringPrintf("%s/%s/%s.profile", storage_path().c_str(),
                         id.user.c_str(), id.identifier.c_str()));
#ifdef ENABLE_JSON_STORE
  final_path = final_path.AddExtension("json");
#endif
  string data = "]corrupt_data[";
  EXPECT_EQ(data.size(), base::WriteFile(final_path, data.data(), data.size()));

  // Then test that we fail to open this file.
  EXPECT_CALL(*mock_metrics_, NotifyCorruptedProfile());
  EXPECT_FALSE(ProfileInitStorage(id, Profile::kOpenExisting, false,
                                  Error::kInternalError));
  Mock::VerifyAndClearExpectations(mock_metrics_.get());

  // But then on a second try the file no longer exists.
  EXPECT_CALL(*mock_metrics_, NotifyCorruptedProfile()).Times(0);
  ASSERT_FALSE(ProfileInitStorage(id, Profile::kOpenExisting, false,
                                  Error::kNotFound));
}

TEST_F(ProfileTest, UpdateDevice) {
  EXPECT_FALSE(profile_->UpdateDevice(nullptr));
}

TEST_F(ProfileTest, GetServiceFromEntry) {
  std::unique_ptr<MockManager> manager(new StrictMock<MockManager>(
      control_interface(), dispatcher(), metrics()));
  profile_->manager_ = manager.get();

  MockStore* storage(new StrictMock<MockStore>());
  profile_->storage_.reset(storage);  // Passes ownership
  const string kEntryName("entry_name");

  // If entry does not appear in storage, GetServiceFromEntry() should return
  // an error.
  EXPECT_CALL(*storage, ContainsGroup(kEntryName))
      .WillOnce(Return(false));
  {
    Error error;
    profile_->GetServiceFromEntry(kEntryName, &error);
    EXPECT_EQ(Error::kNotFound, error.type());
  }
  Mock::VerifyAndClearExpectations(storage);

  EXPECT_CALL(*storage, ContainsGroup(kEntryName))
      .WillRepeatedly(Return(true));

  // Service entry already registered with the manager, the registered service
  // is returned.
  scoped_refptr<MockService> registered_service(CreateMockService());
  EXPECT_CALL(*manager.get(),
              GetServiceWithStorageIdentifier(profile_, kEntryName, _))
      .WillOnce(Return(registered_service));
  {
    Error error;
    EXPECT_EQ(registered_service,
              profile_->GetServiceFromEntry(kEntryName, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  Mock::VerifyAndClearExpectations(manager.get());

  // Service entry not registered with the manager, a temporary service is
  // created/returned.
  scoped_refptr<MockService> temporary_service(CreateMockService());
  EXPECT_CALL(*manager.get(),
              GetServiceWithStorageIdentifier(profile_, kEntryName, _))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(*manager.get(),
              CreateTemporaryServiceFromProfile(profile_, kEntryName, _))
      .WillOnce(Return(temporary_service));
  {
    Error error;
    EXPECT_EQ(temporary_service,
              profile_->GetServiceFromEntry(kEntryName, &error));
    EXPECT_TRUE(error.IsSuccess());
  }
  Mock::VerifyAndClearExpectations(manager.get());
}

}  // namespace shill
