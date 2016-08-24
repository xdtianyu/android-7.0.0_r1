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

// pppd.h, which is required by lcp.h, is not C++ compatible.  The following
// contortions are required before including anything else to ensure that we
// control the definition of bool before stdbool get indirectly included so that
// we can redefine it.

#include <sys/types.h>

extern "C" {
#include <pppd/fsm.h>
#include <pppd/ipcp.h>

#define class class_num
#define bool pppd_bool_t
#include <pppd/pppd.h>
#undef bool
#undef class
#undef STOPPED
#include <pppd/lcp.h>
}

#include "shill/shims/ppp.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <map>

#include <base/command_line.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <brillo/syslog_logging.h>

#include "shill/ppp_device.h"
#include "shill/rpc_task.h"
#include "shill/shims/environment.h"
#include "shill/shims/task_proxy.h"

using std::map;
using std::string;

namespace shill {

namespace shims {

static base::LazyInstance<PPP> g_ppp = LAZY_INSTANCE_INITIALIZER;

PPP::PPP() : running_(false) {}

PPP::~PPP() {}

// static
PPP* PPP::GetInstance() {
  return g_ppp.Pointer();
}

void PPP::Init() {
  if (running_) {
    return;
  }
  running_ = true;
  base::CommandLine::Init(0, NULL);
  brillo::InitLog(brillo::kLogToSyslog | brillo::kLogHeader);
  LOG(INFO) << "PPP started.";
}

bool PPP::GetSecret(string* username, string* password) {
  LOG(INFO) << __func__;
  if (!CreateProxy()) {
    return false;
  }
  bool success = proxy_->GetSecret(username, password);
  DestroyProxy();
  return success;
}

void PPP::OnAuthenticateStart() {
  LOG(INFO) << __func__;
  if (CreateProxy()) {
    map<string, string> details;
    proxy_->Notify(kPPPReasonAuthenticating, details);
    DestroyProxy();
  }
}

void PPP::OnAuthenticateDone() {
  LOG(INFO) << __func__;
  if (CreateProxy()) {
    map<string, string> details;
    proxy_->Notify(kPPPReasonAuthenticated, details);
    DestroyProxy();
  }
}

void PPP::OnConnect(const string& ifname) {
  LOG(INFO) << __func__ << "(" << ifname << ")";
  if (!ipcp_gotoptions[0].ouraddr) {
    LOG(ERROR) << "ouraddr not set.";
    return;
  }
  map<string, string> dict;
  dict[kPPPInterfaceName] = ifname;
  dict[kPPPInternalIP4Address] = ConvertIPToText(&ipcp_gotoptions[0].ouraddr);
  dict[kPPPExternalIP4Address] = ConvertIPToText(&ipcp_hisoptions[0].hisaddr);
  if (ipcp_gotoptions[0].default_route) {
    dict[kPPPGatewayAddress] = dict[kPPPExternalIP4Address];
  }
  if (ipcp_gotoptions[0].dnsaddr[0]) {
    dict[kPPPDNS1] = ConvertIPToText(&ipcp_gotoptions[0].dnsaddr[0]);
  }
  if (ipcp_gotoptions[0].dnsaddr[1]) {
    dict[kPPPDNS2] = ConvertIPToText(&ipcp_gotoptions[0].dnsaddr[1]);
  }
  if (lcp_gotoptions[0].mru) {
    dict[kPPPMRU] = base::IntToString(lcp_gotoptions[0].mru);
  }
  string lns_address;
  if (Environment::GetInstance()->GetVariable("LNS_ADDRESS", &lns_address)) {
    // Really an L2TP/IPSec option rather than a PPP one. But oh well.
    dict[kPPPLNSAddress] = lns_address;
  }
  if (CreateProxy()) {
    proxy_->Notify(kPPPReasonConnect, dict);
    DestroyProxy();
  }
}

void PPP::OnDisconnect() {
  LOG(INFO) << __func__;
  if (CreateProxy()) {
    map<string, string> dict;
    proxy_->Notify(kPPPReasonDisconnect, dict);
    DestroyProxy();
  }
}

bool PPP::CreateProxy() {
  Environment* environment = Environment::GetInstance();
  string service, path;
  if (!environment->GetVariable(kRPCTaskServiceVariable, &service) ||
      !environment->GetVariable(kRPCTaskPathVariable, &path)) {
    LOG(ERROR) << "Environment variables not available.";
    return false;
  }

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  bus_ = new dbus::Bus(options);
  CHECK(bus_->Connect());

  proxy_.reset(new TaskProxy(bus_, path, service));

  LOG(INFO) << "Task proxy created: " << service << " - " << path;
  return true;
}

void PPP::DestroyProxy() {
  proxy_.reset();
  if (bus_) {
    bus_->ShutdownAndBlock();
  }
  LOG(INFO) << "Task proxy destroyed.";
}

// static
string PPP::ConvertIPToText(const void* addr) {
  char text[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, addr, text, INET_ADDRSTRLEN);
  return text;
}

}  // namespace shims

}  // namespace shill
