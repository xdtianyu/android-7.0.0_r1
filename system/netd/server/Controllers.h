/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CONTROLLERS_H__
#define _CONTROLLERS_H__

#include <sysutils/FrameworkListener.h>

#include "NetworkController.h"
#include "TetherController.h"
#include "NatController.h"
#include "PppController.h"
#include "SoftapController.h"
#include "BandwidthController.h"
#include "IdletimerController.h"
#include "InterfaceController.h"
#include "ResolverController.h"
#include "FirewallController.h"
#include "ClatdController.h"
#include "StrictController.h"

namespace android {
namespace net {

struct Controllers {
    Controllers();

    NetworkController netCtrl;
    TetherController tetherCtrl;
    NatController natCtrl;
    PppController pppCtrl;
    SoftapController softapCtrl;
    BandwidthController bandwidthCtrl;
    IdletimerController idletimerCtrl;
    ResolverController resolverCtrl;
    FirewallController firewallCtrl;
    InterfaceController interfaceCtrl;
    ClatdController clatdCtrl;
    StrictController strictCtrl;
};

extern Controllers* gCtls;

}  // namespace net
}  // namespace android

#endif  // _CONTROLLERS_H__
