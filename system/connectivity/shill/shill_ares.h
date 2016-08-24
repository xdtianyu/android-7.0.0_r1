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

#ifndef SHILL_SHILL_ARES_H_
#define SHILL_SHILL_ARES_H_

#include <ares.h>

#include <base/lazy_instance.h>

namespace shill {

// A "ares.h" abstraction allowing mocking in tests.
class Ares {
 public:
  virtual ~Ares();

  static Ares* GetInstance();

  // ares_destroy
  virtual void Destroy(ares_channel channel);

  // ares_gethostbyname
  virtual void GetHostByName(ares_channel channel,
                             const char* hostname,
                             int family,
                             ares_host_callback callback,
                             void* arg);

  // ares_getsock
  virtual int GetSock(ares_channel channel,
                      ares_socket_t* socks,
                      int numsocks);

  // ares_init_options
  virtual int InitOptions(ares_channel* channelptr,
                          struct ares_options* options,
                          int optmask);

  // ares_process_fd
  virtual void ProcessFd(ares_channel channel,
                         ares_socket_t read_fd,
                         ares_socket_t write_fd);

  // ares_set_local_dev
  virtual void SetLocalDev(ares_channel channel, const char* local_dev_name);

  // ares_timeout
  virtual struct timeval* Timeout(ares_channel channel,
                                  struct timeval* maxtv,
                                  struct timeval* tv);

  // ares_set_servers_csv
  virtual int SetServersCsv(ares_channel channel, const char* servers);

 protected:
  Ares();

 private:
  friend struct base::DefaultLazyInstanceTraits<Ares>;

  DISALLOW_COPY_AND_ASSIGN(Ares);
};

}  // namespace shill

#endif  // SHILL_SHILL_ARES_H_
