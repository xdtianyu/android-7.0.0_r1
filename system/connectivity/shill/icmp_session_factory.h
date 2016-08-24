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

#ifndef SHILL_ICMP_SESSION_FACTORY_H_
#define SHILL_ICMP_SESSION_FACTORY_H_

#include <base/lazy_instance.h>

#include "shill/icmp_session.h"

namespace shill {

class IcmpSessionFactory {
 public:
  virtual ~IcmpSessionFactory();

  // This is a singleton. Use IcmpSessionFactory::GetInstance()->Foo().
  static IcmpSessionFactory* GetInstance();

  virtual IcmpSession* CreateIcmpSession(EventDispatcher* dispatcher);

 protected:
  IcmpSessionFactory();

 private:
  friend struct base::DefaultLazyInstanceTraits<IcmpSessionFactory>;

  DISALLOW_COPY_AND_ASSIGN(IcmpSessionFactory);
};

}  // namespace shill

#endif  // SHILL_ICMP_SESSION_FACTORY_H_
