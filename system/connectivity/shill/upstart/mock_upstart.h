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

#ifndef SHILL_UPSTART_MOCK_UPSTART_H_
#define SHILL_UPSTART_MOCK_UPSTART_H_

#include <string>

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/upstart/upstart.h"

namespace shill {

class MockUpstart : public Upstart {
 public:
  explicit MockUpstart(ControlInterface* control_interface);
  ~MockUpstart() override;

  MOCK_METHOD0(NotifyDisconnected, void());
  MOCK_METHOD0(NotifyConnected, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockUpstart);
};

}  // namespace shill

#endif  // SHILL_UPSTART_MOCK_UPSTART_H_
