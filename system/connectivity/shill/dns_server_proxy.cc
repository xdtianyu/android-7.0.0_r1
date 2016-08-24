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

#include "shill/dns_server_proxy.h"

#include <map>

#include <base/bind.h>
#include <base/strings/stringprintf.h>

#include "shill/logging.h"
#include "shill/process_manager.h"

using std::string;
using std::vector;

namespace shill {

namespace {
const char kDnsmasqPath[] = "/system/bin/dnsmasq";
const char kDnsmasqPidFilePath[] = "/data/misc/shill/dnsmasq.pid";
const char kDnsmasqUser[] = "system";
const char kDnsmasqGroup[] = "system";
const int kInvalidPID = -1;
}

DNSServerProxy::DNSServerProxy(const vector<string>& dns_servers)
    : process_manager_(ProcessManager::GetInstance()),
      pid_(kInvalidPID),
      dns_servers_(dns_servers) {}

DNSServerProxy::~DNSServerProxy() {
  if (pid_ != kInvalidPID) {
    Stop();
  }
}

bool DNSServerProxy::Start() {
  if (pid_ != kInvalidPID) {
    LOG(ERROR) << __func__ << ": already started";
    return false;
  }
  // Setup command line arguments for dnsmasq.
  vector<string> args;
  args.push_back("--no-hosts");
  args.push_back("--listen-address=127.0.0.1");
  args.push_back("--no-resolv");
  args.push_back("--keep-in-foreground");
  args.push_back(base::StringPrintf("--user=%s", kDnsmasqUser));
  args.push_back(base::StringPrintf("--group=%s", kDnsmasqGroup));
  for (const auto& server : dns_servers_) {
    args.push_back(base::StringPrintf("--server=%s", server.c_str()));
  }
  args.push_back(base::StringPrintf("--pid-file=%s", kDnsmasqPidFilePath));
  // Start dnsmasq.
  // TODO(zqiu): start dnsmasq with Minijail when the latter is working on
  // Android (b/24572800).
  pid_t pid =
      process_manager_->StartProcess(
          FROM_HERE,
          base::FilePath(kDnsmasqPath),
          args,
          std::map<string, string>(),    // No environment variables needed.
          true,                          // Terminate with parent.
          base::Bind(&DNSServerProxy::OnProcessExited,
                     weak_factory_.GetWeakPtr()));
  if (pid < 0) {
    return false;
  }

  pid_ = pid;
  LOG(INFO) << "Spawned " << kDnsmasqPath << " with pid: " << pid_;
  return true;
}

void DNSServerProxy::Stop() {
  if (pid_ == kInvalidPID) {
    LOG(ERROR) << __func__ << ": already stopped";
    return;
  }
  process_manager_->StopProcess(pid_);
}

void DNSServerProxy::OnProcessExited(int exit_status) {
  CHECK(pid_);
  if (exit_status != EXIT_SUCCESS) {
    LOG(WARNING) << "pid " << pid_ << " exit status " << exit_status;
  }
  pid_ = kInvalidPID;
}

}  // namespace shill
