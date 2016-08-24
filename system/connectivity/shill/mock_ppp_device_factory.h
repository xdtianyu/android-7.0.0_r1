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

#ifndef SHILL_MOCK_PPP_DEVICE_FACTORY_H_
#define SHILL_MOCK_PPP_DEVICE_FACTORY_H_

#include <string>

#include <base/lazy_instance.h>
#include <gmock/gmock.h>

#include "shill/ppp_device_factory.h"

namespace shill {

class MockPPPDeviceFactory : public PPPDeviceFactory {
 public:
  ~MockPPPDeviceFactory() override;

  // This is a singleton. Use MockPPPDeviceFactory::GetInstance()->Foo().
  static MockPPPDeviceFactory* GetInstance();

  MOCK_METHOD6(CreatePPPDevice,
               PPPDevice* (ControlInterface* control,
                           EventDispatcher* dispatcher,
                           Metrics* metrics,
                           Manager* manager,
                           const std::string& link_name,
                           int interface_index));

 protected:
  MockPPPDeviceFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<MockPPPDeviceFactory>;

  DISALLOW_COPY_AND_ASSIGN(MockPPPDeviceFactory);
};

}  // namespace shill

#endif  // SHILL_MOCK_PPP_DEVICE_FACTORY_H_
