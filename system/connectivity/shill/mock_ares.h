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

#ifndef SHILL_MOCK_ARES_H_
#define SHILL_MOCK_ARES_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/shill_ares.h"

namespace shill {

class MockAres : public Ares {
 public:
  MockAres();
  ~MockAres() override;

  MOCK_METHOD1(Destroy, void(ares_channel channel));
  MOCK_METHOD5(GetHostByName, void(ares_channel channel,
                                   const char* hostname,
                                   int family,
                                   ares_host_callback callback,
                                   void* arg));
  MOCK_METHOD3(GetSock, int(ares_channel channel,
                            ares_socket_t* socks,
                            int numsocks));
  MOCK_METHOD3(InitOptions, int(ares_channel* channelptr,
                                struct ares_options* options,
                                int optmask));
  MOCK_METHOD3(ProcessFd, void(ares_channel channel,
                               ares_socket_t read_fd,
                               ares_socket_t write_fd));
  MOCK_METHOD2(SetLocalDev, void(ares_channel channel,
                                 const char* local_dev_name));
  MOCK_METHOD3(Timeout, struct timeval* (ares_channel channel,
                                         struct timeval* maxtv,
                                         struct timeval* tv));
  MOCK_METHOD2(SetServersCsv, int(ares_channel channel,
                                  const char* servers));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAres);
};

}  // namespace shill

#endif  // SHILL_MOCK_ARES_H_
