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

#include "tpm_manager/server/mock_tpm_nvram.h"

namespace tpm_manager {

using testing::_;
using testing::Invoke;
using testing::Return;

MockTpmNvram::MockTpmNvram() {
  ON_CALL(*this, DefineNvram(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeDefineNvram));
  ON_CALL(*this, DestroyNvram(_))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeDestroyNvram));
  ON_CALL(*this, WriteNvram(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeWriteNvram));
  ON_CALL(*this, ReadNvram(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeReadNvram));
  ON_CALL(*this, IsNvramDefined(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeIsNvramDefined));
  ON_CALL(*this, IsNvramLocked(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeIsNvramLocked));
  ON_CALL(*this, GetNvramSize(_, _))
      .WillByDefault(Invoke(this, &MockTpmNvram::FakeGetNvramSize));
}

MockTpmNvram::~MockTpmNvram() {}

bool MockTpmNvram::FakeDefineNvram(uint32_t index, size_t length) {
  if (length == 0) {
    return false;
  }
  NvSpace ns;
  ns.data.resize(length, '\xff');
  ns.written = false;
  nvram_map_[index] = ns;
  return true;
}

bool MockTpmNvram::FakeDestroyNvram(uint32_t index) {
  auto it = nvram_map_.find(index);
  if (it == nvram_map_.end()) {
    return false;
  }
  nvram_map_.erase(it);
  return true;
}

bool MockTpmNvram::FakeWriteNvram(uint32_t index, const std::string& data) {
  auto it = nvram_map_.find(index);
  if (it == nvram_map_.end()) {
    return false;
  }
  NvSpace& nv = it->second;
  if (nv.written || nv.data.size() < data.size()) {
    return false;
  }
  nv.data.replace(0, data.size(), data);
  nv.written = true;
  return true;
}

bool MockTpmNvram::FakeReadNvram(uint32_t index, std::string* data) {
  auto it = nvram_map_.find(index);
  if (it == nvram_map_.end()) {
    return false;
  }
  const NvSpace& nv = it->second;
  if (!nv.written) {
    return false;
  }
  data->assign(nv.data);
  return true;
}

bool MockTpmNvram::FakeIsNvramDefined(uint32_t index, bool* defined) {
  *defined = (nvram_map_.find(index) != nvram_map_.end());
  return true;
}

bool MockTpmNvram::FakeIsNvramLocked(uint32_t index, bool* locked) {
  bool defined;
  if (!IsNvramDefined(index, &defined) || !defined) {
    return false;
  }
  *locked = nvram_map_[index].written;
  return true;
}

bool MockTpmNvram::FakeGetNvramSize(uint32_t index, size_t* size) {
  bool defined;
  if (!IsNvramDefined(index, &defined) || !defined) {
    return false;
  }
  *size = nvram_map_[index].data.size();
  return true;
}

}  // namespace tpm_manager
