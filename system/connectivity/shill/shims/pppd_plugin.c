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

// pppd.h declares a field member called |class| which forces this file to be C.
#include <pppd/pppd.h>

#include "shill/shims/c_ppp.h"

char pppd_version[] = VERSION;

static void PPPOnUp(void* data, int arg) {
  PPPOnConnect(ifname);
}

static void PPPOnPhaseChange(void* data, int arg) {
  if (arg == PHASE_AUTHENTICATE) {
    PPPOnAuthenticateStart();
  } else if (arg == PHASE_NETWORK) {
    // Either no authentication was required, or authentication has
    // completed.
    //
    // TODO(quiche): We can also transition backwards to PHASE_NETWORK,
    // when disconnecting. In such cases, the may want to omit this
    // (spurious) call.
    PPPOnAuthenticateDone();
  } else if (arg == PHASE_DISCONNECT || arg == PHASE_DEAD) {
    PPPOnDisconnect();
  }
}

__attribute__ ((visibility("default"))) int plugin_init() {
  PPPInit();

  chap_check_hook = PPPHasSecret;
  pap_check_hook = PPPHasSecret;

  pap_passwd_hook = PPPGetSecret;
  chap_passwd_hook = PPPGetSecret;

  add_notifier(&ip_up_notifier, PPPOnUp, NULL);
  add_notifier(&phasechange, PPPOnPhaseChange, NULL);
  add_notifier(&exitnotify, PPPOnExit, NULL);

  return 0;
}
