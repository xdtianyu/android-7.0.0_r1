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

#ifndef SHILL_DBUS_CHROMEOS_UPSTART_PROXY_H_
#define SHILL_DBUS_CHROMEOS_UPSTART_PROXY_H_

#include <string>
#include <vector>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>

#include "shill/upstart/upstart_proxy_interface.h"
#include "upstart/dbus-proxies.h"

namespace shill {

class ChromeosUpstartProxy : public UpstartProxyInterface {
 public:
  explicit ChromeosUpstartProxy(const scoped_refptr<dbus::Bus>& bus);
  ~ChromeosUpstartProxy() override = default;

  // Inherited from UpstartProxyInterface.
  void EmitEvent(const std::string& name,
                 const std::vector<std::string>& env,
                 bool wait) override;

 private:
  // Service path is provided in the xml file and will be populated by the
  // generator.
  static const char kServiceName[];

  // Callback for async call to EmitEvent.
  void OnEmitEventSuccess();
  void OnEmitEventFailure(brillo::Error* error);

  std::unique_ptr<com::ubuntu::Upstart0_6Proxy> upstart_proxy_;

  base::WeakPtrFactory<ChromeosUpstartProxy> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ChromeosUpstartProxy);
};

}  // namespace shill

#endif  // SHILL_DBUS_CHROMEOS_UPSTART_PROXY_H_
