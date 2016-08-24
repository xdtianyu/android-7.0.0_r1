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

#ifndef SHILL_SUPPLICANT_MOCK_SUPPLICANT_BSS_PROXY_H_
#define SHILL_SUPPLICANT_MOCK_SUPPLICANT_BSS_PROXY_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/supplicant/supplicant_bss_proxy_interface.h"

namespace shill {

class MockSupplicantBSSProxy : public SupplicantBSSProxyInterface {
 public:
  MockSupplicantBSSProxy();
  ~MockSupplicantBSSProxy() override;

  MOCK_METHOD0(Die, void());  // So we can EXPECT the dtor.

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSupplicantBSSProxy);
};

}  // namespace shill

#endif  // SHILL_SUPPLICANT_MOCK_SUPPLICANT_BSS_PROXY_H_
