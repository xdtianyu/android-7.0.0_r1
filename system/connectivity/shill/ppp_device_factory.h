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

#ifndef SHILL_PPP_DEVICE_FACTORY_H_
#define SHILL_PPP_DEVICE_FACTORY_H_

#include <string>

#include <base/lazy_instance.h>

namespace shill {

class ControlInterface;
class EventDispatcher;
class Manager;
class Metrics;
class PPPDevice;

class PPPDeviceFactory {
 public:
  virtual ~PPPDeviceFactory();

  // This is a singleton. Use PPPDeviceFactory::GetInstance()->Foo().
  static PPPDeviceFactory* GetInstance();

  virtual PPPDevice* CreatePPPDevice(
      ControlInterface* control,
      EventDispatcher* dispatcher,
      Metrics* metrics,
      Manager* manager,
      const std::string& link_name,
      int interface_index);

 protected:
  PPPDeviceFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<PPPDeviceFactory>;

  DISALLOW_COPY_AND_ASSIGN(PPPDeviceFactory);
};

}  // namespace shill

#endif  // SHILL_PPP_DEVICE_FACTORY_H_
