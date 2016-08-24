//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_MOCK_EAP_LISTENER_H_
#define SHILL_MOCK_EAP_LISTENER_H_

#include "shill/eap_listener.h"

#include <gmock/gmock.h>

namespace shill {

class MockEapListener : public EapListener {
 public:
  MockEapListener();
  ~MockEapListener() override;

  MOCK_METHOD0(Start, bool());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(set_request_received_callback, void(
      const EapListener::EapRequestReceivedCallback& callback));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEapListener);
};

}  // namespace shill

#endif  // SHILL_MOCK_EAP_LISTENER_H_
