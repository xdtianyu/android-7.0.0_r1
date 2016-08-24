//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_NET_RTNL_LISTENER_H_
#define SHILL_NET_RTNL_LISTENER_H_

#include <base/callback.h>

#include "shill/net/shill_export.h"

namespace shill {

class RTNLHandler;
class RTNLMessage;

class SHILL_EXPORT RTNLListener {
 public:
  RTNLListener(int listen_flags,
               const base::Callback<void(const RTNLMessage&)>& callback);
  RTNLListener(int listen_flags,
               const base::Callback<void(const RTNLMessage&)>& callback,
               RTNLHandler *rtnl_handler);
   ~RTNLListener();

  void NotifyEvent(int type, const RTNLMessage& msg);

 private:
  int listen_flags_;
  base::Callback<void(const RTNLMessage&)> callback_;
  RTNLHandler *rtnl_handler_;

  DISALLOW_COPY_AND_ASSIGN(RTNLListener);
};

}  // namespace shill

#endif  // SHILL_NET_RTNL_LISTENER_H_
