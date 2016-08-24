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

#ifndef SHILL_SHIMS_PPP_H_
#define SHILL_SHIMS_PPP_H_

#include <memory>
#include <string>

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <base/lazy_instance.h>

namespace dbus {
class Bus;
}  // namespace dbus

namespace shill {

namespace shims {

class TaskProxy;

class PPP {
 public:
  ~PPP();

  // This is a singleton -- use PPP::GetInstance()->Foo().
  static PPP* GetInstance();

  void Init();

  bool GetSecret(std::string* username, std::string* password);
  void OnAuthenticateStart();
  void OnAuthenticateDone();
  void OnConnect(const std::string& ifname);
  void OnDisconnect();

 protected:
  PPP();

 private:
  friend struct base::DefaultLazyInstanceTraits<PPP>;

  bool CreateProxy();
  void DestroyProxy();

  static std::string ConvertIPToText(const void* addr);

  scoped_refptr<dbus::Bus> bus_;
  std::unique_ptr<TaskProxy> proxy_;
  bool running_;

  DISALLOW_COPY_AND_ASSIGN(PPP);
};

}  // namespace shims

}  // namespace shill

#endif  // SHILL_SHIMS_PPP_H_
