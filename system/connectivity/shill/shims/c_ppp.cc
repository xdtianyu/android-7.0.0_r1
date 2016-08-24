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

#include "shill/shims/c_ppp.h"

#include <string>

#include <base/at_exit.h>

#include "shill/shims/ppp.h"

using shill::shims::PPP;
using std::string;

namespace {
base::AtExitManager* g_exit_manager = NULL;  // Cleans up LazyInstances.
}  // namespace

void PPPInit() {
  g_exit_manager = new base::AtExitManager();
  PPP::GetInstance()->Init();
}

int PPPHasSecret() {
  return 1;
}

int PPPGetSecret(char* username, char* password) {
  string user, pass;
  if (!PPP::GetInstance()->GetSecret(&user, &pass)) {
    return -1;
  }
  if (username) {
    strcpy(username, user.c_str());  // NOLINT(runtime/printf)
  }
  if (password) {
    strcpy(password, pass.c_str());  // NOLINT(runtime/printf)
  }
  return 1;
}

void PPPOnAuthenticateStart() {
  PPP::GetInstance()->OnAuthenticateStart();
}

void PPPOnAuthenticateDone() {
  PPP::GetInstance()->OnAuthenticateDone();
}

void PPPOnConnect(const char* ifname) {
  PPP::GetInstance()->OnConnect(ifname);
}

void PPPOnDisconnect() {
  PPP::GetInstance()->OnDisconnect();
}

void PPPOnExit(void* /*data*/, int /*arg*/) {
  LOG(INFO) << __func__;
  delete g_exit_manager;
  g_exit_manager = NULL;
}
