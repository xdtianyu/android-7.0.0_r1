/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/logging.h>
#include <base/macros.h>
#include <binder/IBinder.h>
#include <binderwrapper/binder_test_base.h>
#include <binderwrapper/stub_binder_wrapper.h>

#include "wake_lock_manager.h"

namespace android {

class WakeLockManagerTest : public BinderTestBase {
 public:
  WakeLockManagerTest() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    lock_path_ = temp_dir_.path().Append("lock");
    unlock_path_ = temp_dir_.path().Append("unlock");
    ClearFiles();

    manager_.set_paths_for_testing(lock_path_, unlock_path_);
    CHECK(manager_.Init());
  }
  ~WakeLockManagerTest() override = default;

 protected:
  // Returns the contents of |path|.
  std::string ReadFile(const base::FilePath& path) const {
    std::string value;
    CHECK(base::ReadFileToString(path, &value));
    return value;
  }

  // Clears |lock_path_| and |unlock_path_|.
  void ClearFiles() {
    CHECK(base::WriteFile(lock_path_, "", 0) == 0);
    CHECK(base::WriteFile(unlock_path_, "", 0) == 0);
  }

  base::ScopedTempDir temp_dir_;

  // Files within |temp_dir_| simulating /sys/power/wake_lock and wake_unlock.
  base::FilePath lock_path_;
  base::FilePath unlock_path_;

  WakeLockManager manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WakeLockManagerTest);
};

TEST_F(WakeLockManagerTest, AddAndRemoveRequests) {
  // A kernel wake lock should be created for the first request.
  sp<BBinder> binder1 = binder_wrapper()->CreateLocalBinder();
  EXPECT_TRUE(manager_.AddRequest(binder1, "1", "1", -1));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  // Nothing should happen when a second request is made.
  ClearFiles();
  sp<BBinder> binder2 = binder_wrapper()->CreateLocalBinder();
  EXPECT_TRUE(manager_.AddRequest(binder2, "2", "2", -1));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  // The wake lock should still be held after the first request is withdrawn.
  ClearFiles();
  EXPECT_TRUE(manager_.RemoveRequest(binder1));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  // When there are no more requests, the wake lock should be released.
  ClearFiles();
  EXPECT_TRUE(manager_.RemoveRequest(binder2));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(unlock_path_));
}

TEST_F(WakeLockManagerTest, DuplicateRequest) {
  sp<BBinder> binder = binder_wrapper()->CreateLocalBinder();
  EXPECT_TRUE(manager_.AddRequest(binder, "foo", "bar", -1));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  // Send a second request using the same binder and check a new wake lock isn't
  // created.
  ClearFiles();
  EXPECT_TRUE(manager_.AddRequest(binder, "a", "b", -1));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  ClearFiles();
  EXPECT_TRUE(manager_.RemoveRequest(binder));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(unlock_path_));
}

TEST_F(WakeLockManagerTest, InvalidRemoval) {
  // Trying to remove an unknown binder should fail and not do anything.
  sp<BBinder> binder = binder_wrapper()->CreateLocalBinder();
  EXPECT_FALSE(manager_.RemoveRequest(binder));
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));
}

TEST_F(WakeLockManagerTest, BinderDeath) {
  sp<BBinder> binder = binder_wrapper()->CreateLocalBinder();
  EXPECT_TRUE(manager_.AddRequest(binder, "foo", "bar", -1));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));

  // If the binder dies, the wake lock should be released.
  ClearFiles();
  binder_wrapper()->NotifyAboutBinderDeath(binder);
  EXPECT_EQ("", ReadFile(lock_path_));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(unlock_path_));

  // Check that a new request can be created using the same binder.
  ClearFiles();
  EXPECT_TRUE(manager_.AddRequest(binder, "foo", "bar", -1));
  EXPECT_EQ(WakeLockManager::kLockName, ReadFile(lock_path_));
  EXPECT_EQ("", ReadFile(unlock_path_));
}

}  // namespace android
