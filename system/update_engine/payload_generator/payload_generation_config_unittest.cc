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

#include "update_engine/payload_generator/payload_generation_config.h"

#include <gtest/gtest.h>

namespace chromeos_update_engine {

class PayloadGenerationConfigTest : public ::testing::Test {};

TEST_F(PayloadGenerationConfigTest, SimpleLoadPostInstallConfigTest) {
  ImageConfig image_config;
  image_config.partitions.emplace_back("root");
  brillo::KeyValueStore store;
  EXPECT_TRUE(
      store.LoadFromString("RUN_POSTINSTALL_root=true\n"
                           "POSTINSTALL_PATH_root=postinstall\n"
                           "FILESYSTEM_TYPE_root=ext4"));
  EXPECT_TRUE(image_config.LoadPostInstallConfig(store));
  EXPECT_FALSE(image_config.partitions[0].postinstall.IsEmpty());
  EXPECT_EQ(true, image_config.partitions[0].postinstall.run);
  EXPECT_EQ("postinstall", image_config.partitions[0].postinstall.path);
  EXPECT_EQ("ext4", image_config.partitions[0].postinstall.filesystem_type);
}

TEST_F(PayloadGenerationConfigTest, LoadPostInstallConfigNameMismatchTest) {
  ImageConfig image_config;
  image_config.partitions.emplace_back("system");
  brillo::KeyValueStore store;
  EXPECT_TRUE(
      store.LoadFromString("RUN_POSTINSTALL_root=true\n"
                           "POSTINSTALL_PATH_root=postinstall\n"
                           "FILESYSTEM_TYPE_root=ext4"));
  EXPECT_FALSE(image_config.LoadPostInstallConfig(store));
  EXPECT_TRUE(image_config.partitions[0].postinstall.IsEmpty());
}

}  // namespace chromeos_update_engine
