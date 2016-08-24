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

#include "shill/dhcp/dhcp_provider.h"

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/stringprintf.h>

#include "shill/dhcp/dhcp_config.h"
#include "shill/mock_control.h"
#include "shill/mock_dhcp_properties.h"
#include "shill/mock_event_dispatcher.h"

using base::FilePath;
using base::ScopedTempDir;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::Test;

namespace shill {

namespace {
const char kDeviceName[] = "testdevicename";
const char kStorageIdentifier[] = "teststorageidentifier";
const bool kArpGateway = false;
}  // namespace

class DHCPProviderTest : public Test {
 public:
  DHCPProviderTest() : provider_(DHCPProvider::GetInstance()) {
    provider_->control_interface_ = &control_;
    provider_->dispatcher_ = &dispatcher_;
  }

  void SetUp() {
    // DHCPProvider is a singleton, there is no guarentee that it is
    // not setup/used elsewhere, so reset its state before running our
    // tests.
    provider_->configs_.clear();
    provider_->recently_unbound_pids_.clear();
  }

 protected:
  void RetireUnboundPID(int pid) { provider_->RetireUnboundPID(pid); }

  MockControl control_;
  DHCPProvider* provider_;
  StrictMock<MockEventDispatcher> dispatcher_;
};

TEST_F(DHCPProviderTest, CreateIPv4Config) {
  DhcpProperties dhcp_props;

  DHCPConfigRefPtr config = provider_->CreateIPv4Config(kDeviceName,
                                                        kStorageIdentifier,
                                                        kArpGateway,
                                                        dhcp_props);
  EXPECT_TRUE(config.get());
  EXPECT_EQ(kDeviceName, config->device_name());
  EXPECT_TRUE(provider_->configs_.empty());
}

TEST_F(DHCPProviderTest, DestroyLease) {
  ScopedTempDir temp_dir;
  FilePath lease_file;
  EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
  provider_->root_ = temp_dir.path();
  lease_file = provider_->root_.Append(base::StringPrintf(
      DHCPProvider::kDHCPCDPathFormatLease,
      kDeviceName));
  EXPECT_TRUE(base::CreateDirectory(lease_file.DirName()));
  EXPECT_EQ(0, base::WriteFile(lease_file, "", 0));
  EXPECT_TRUE(base::PathExists(lease_file));
  provider_->DestroyLease(kDeviceName);
  EXPECT_FALSE(base::PathExists(lease_file));
}

TEST_F(DHCPProviderTest, BindAndUnbind) {
  int kPid = 999;
  EXPECT_EQ(nullptr, provider_->GetConfig(kPid));
  EXPECT_FALSE(provider_->IsRecentlyUnbound(kPid));
  DhcpProperties dhcp_props;

  DHCPConfigRefPtr config = provider_->CreateIPv4Config(kDeviceName,
                                                        kStorageIdentifier,
                                                        kArpGateway,
                                                        dhcp_props);
  provider_->BindPID(kPid, config);
  EXPECT_NE(nullptr, provider_->GetConfig(kPid));
  EXPECT_FALSE(provider_->IsRecentlyUnbound(kPid));

  base::Closure task;
  EXPECT_CALL(dispatcher_, PostDelayedTask(_, _));  // TODO(pstew): crbug/502320
  provider_->UnbindPID(kPid);
  EXPECT_EQ(nullptr, provider_->GetConfig(kPid));
  EXPECT_TRUE(provider_->IsRecentlyUnbound(kPid));

  RetireUnboundPID(kPid);  // Execute as if the PostDelayedTask() timer expired.
  EXPECT_EQ(nullptr, provider_->GetConfig(kPid));
  EXPECT_FALSE(provider_->IsRecentlyUnbound(kPid));
}

}  // namespace shill
