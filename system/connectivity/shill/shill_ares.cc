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

#include "shill/shill_ares.h"

namespace shill {

namespace {
base::LazyInstance<Ares> g_ares = LAZY_INSTANCE_INITIALIZER;
}  // namespace

Ares::Ares() { }

Ares::~Ares() { }

Ares* Ares::GetInstance() {
  return g_ares.Pointer();
}

void Ares::Destroy(ares_channel channel) {
  ares_destroy(channel);
}

void Ares::GetHostByName(ares_channel channel,
                         const char* hostname,
                         int family,
                         ares_host_callback callback,
                         void* arg) {
  ares_gethostbyname(channel, hostname, family, callback, arg);
}

int Ares::GetSock(ares_channel channel,
                  ares_socket_t* socks,
                  int numsocks) {
  return ares_getsock(channel, socks, numsocks);
}

int Ares::InitOptions(ares_channel* channelptr,
                      struct ares_options* options,
                      int optmask) {
  return ares_init_options(channelptr, options, optmask);
}


void Ares::ProcessFd(ares_channel channel,
                     ares_socket_t read_fd,
                     ares_socket_t write_fd) {
  return ares_process_fd(channel, read_fd, write_fd);
}

void Ares::SetLocalDev(ares_channel channel, const char* local_dev_name) {
  ares_set_local_dev(channel, local_dev_name);
}

struct timeval* Ares::Timeout(ares_channel channel,
                              struct timeval* maxtv,
                              struct timeval* tv) {
  return ares_timeout(channel, maxtv, tv);
}

int Ares::SetServersCsv(ares_channel channel, const char* servers) {
  return ares_set_servers_csv(channel, servers);
}

}  // namespace shill
