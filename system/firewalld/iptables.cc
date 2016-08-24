// Copyright 2014 The Android Open Source Project
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

#include "iptables.h"

#include <linux/capability.h>

#include <string>
#include <vector>

#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/callback.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#include <brillo/minijail/minijail.h>
#include <brillo/process.h>

namespace {

using IpTablesCallback = base::Callback<bool(const std::string&, bool)>;

#if defined(__ANDROID__)
const char kIpTablesPath[] = "/system/bin/iptables";
const char kIp6TablesPath[] = "/system/bin/ip6tables";
const char kIpPath[] = "/system/bin/ip";
#else
const char kIpTablesPath[] = "/sbin/iptables";
const char kIp6TablesPath[] = "/sbin/ip6tables";
const char kIpPath[] = "/bin/ip";
const char kUnprivilegedUser[] = "nobody";
#endif  // __ANDROID__

const char kIPv4[] = "IPv4";
const char kIPv6[] = "IPv6";

const uint64_t kIpTablesCapMask =
    CAP_TO_MASK(CAP_NET_ADMIN) | CAP_TO_MASK(CAP_NET_RAW);

// Interface names must be shorter than 'IFNAMSIZ' chars.
// See http://man7.org/linux/man-pages/man7/netdevice.7.html
// 'IFNAMSIZ' is 16 in recent kernels.
// See http://lxr.free-electrons.com/source/include/uapi/linux/if.h#L26
const size_t kInterfaceNameSize = 16;

const char kMarkForUserTraffic[] = "1";

const char kTableIdForUserTraffic[] = "1";

bool IsValidInterfaceName(const std::string& iface) {
  // |iface| should be shorter than |kInterfaceNameSize| chars and have only
  // alphanumeric characters (embedded hypens and periods are also permitted).
  if (iface.length() >= kInterfaceNameSize) {
    return false;
  }
  if (base::StartsWith(iface, "-", base::CompareCase::SENSITIVE) ||
      base::EndsWith(iface, "-", base::CompareCase::SENSITIVE) ||
      base::StartsWith(iface, ".", base::CompareCase::SENSITIVE) ||
      base::EndsWith(iface, ".", base::CompareCase::SENSITIVE)) {
    return false;
  }
  for (auto c : iface) {
    if (!std::isalnum(c) && (c != '-') && (c != '.')) {
      return false;
    }
  }
  return true;
}

bool RunForAllArguments(const IpTablesCallback& iptables_cmd,
                        const std::vector<std::string>& arguments,
                        bool add) {
  bool success = true;
  for (const auto& argument : arguments) {
    if (!iptables_cmd.Run(argument, add)) {
      // On failure, only abort if rules are being added.
      // If removing a rule fails, attempt the remaining removals but still
      // return 'false'.
      success = false;
      if (add)
        break;
    }
  }
  return success;
}

}  // namespace

namespace firewalld {

IpTables::IpTables() {
}

IpTables::~IpTables() {
  // Plug all holes when destructed.
  PlugAllHoles();
}

bool IpTables::PunchTcpHole(uint16_t in_port, const std::string& in_interface) {
  return PunchHole(in_port, in_interface, &tcp_holes_, kProtocolTcp);
}

bool IpTables::PunchUdpHole(uint16_t in_port, const std::string& in_interface) {
  return PunchHole(in_port, in_interface, &udp_holes_, kProtocolUdp);
}

bool IpTables::PlugTcpHole(uint16_t in_port, const std::string& in_interface) {
  return PlugHole(in_port, in_interface, &tcp_holes_, kProtocolTcp);
}

bool IpTables::PlugUdpHole(uint16_t in_port, const std::string& in_interface) {
  return PlugHole(in_port, in_interface, &udp_holes_, kProtocolUdp);
}

bool IpTables::RequestVpnSetup(const std::vector<std::string>& usernames,
                               const std::string& interface) {
  return ApplyVpnSetup(usernames, interface, true /* add */);
}

bool IpTables::RemoveVpnSetup(const std::vector<std::string>& usernames,
                              const std::string& interface) {
  return ApplyVpnSetup(usernames, interface, false /* delete */);
}

bool IpTables::PunchHole(uint16_t port,
                         const std::string& interface,
                         std::set<Hole>* holes,
                         ProtocolEnum protocol) {
  if (port == 0) {
    // Port 0 is not a valid TCP/UDP port.
    return false;
  }

  if (!IsValidInterfaceName(interface)) {
    LOG(ERROR) << "Invalid interface name '" << interface << "'";
    return false;
  }

  Hole hole = std::make_pair(port, interface);
  if (holes->find(hole) != holes->end()) {
    // We have already punched a hole for |port| on |interface|.
    // Be idempotent: do nothing and succeed.
    return true;
  }

  std::string sprotocol = protocol == kProtocolTcp ? "TCP" : "UDP";
  LOG(INFO) << "Punching hole for " << sprotocol << " port " << port
            << " on interface '" << interface << "'";
  if (!AddAcceptRules(protocol, port, interface)) {
    // If the 'iptables' command fails, this method fails.
    LOG(ERROR) << "Adding ACCEPT rules failed.";
    return false;
  }

  // Track the hole we just punched.
  holes->insert(hole);

  return true;
}

bool IpTables::PlugHole(uint16_t port,
                        const std::string& interface,
                        std::set<Hole>* holes,
                        ProtocolEnum protocol) {
  if (port == 0) {
    // Port 0 is not a valid TCP/UDP port.
    return false;
  }

  Hole hole = std::make_pair(port, interface);

  if (holes->find(hole) == holes->end()) {
    // There is no firewall hole for |port| on |interface|.
    // Even though this makes |PlugHole| not idempotent,
    // and Punch/Plug not entirely symmetrical, fail. It might help catch bugs.
    return false;
  }

  std::string sprotocol = protocol == kProtocolTcp ? "TCP" : "UDP";
  LOG(INFO) << "Plugging hole for " << sprotocol << " port " << port
            << " on interface '" << interface << "'";
  if (!DeleteAcceptRules(protocol, port, interface)) {
    // If the 'iptables' command fails, this method fails.
    LOG(ERROR) << "Deleting ACCEPT rules failed.";
    return false;
  }

  // Stop tracking the hole we just plugged.
  holes->erase(hole);

  return true;
}

void IpTables::PlugAllHoles() {
  // Copy the container so that we can remove elements from the original.
  // TCP
  std::set<Hole> holes = tcp_holes_;
  for (auto hole : holes) {
    PlugHole(hole.first /* port */, hole.second /* interface */, &tcp_holes_,
             kProtocolTcp);
  }

  // UDP
  holes = udp_holes_;
  for (auto hole : holes) {
    PlugHole(hole.first /* port */, hole.second /* interface */, &udp_holes_,
             kProtocolUdp);
  }

  CHECK(tcp_holes_.size() == 0) << "Failed to plug all TCP holes.";
  CHECK(udp_holes_.size() == 0) << "Failed to plug all UDP holes.";
}

bool IpTables::AddAcceptRules(ProtocolEnum protocol,
                              uint16_t port,
                              const std::string& interface) {
  if (!AddAcceptRule(kIpTablesPath, protocol, port, interface)) {
    LOG(ERROR) << "Could not add ACCEPT rule using '" << kIpTablesPath << "'";
    return false;
  }

  if (AddAcceptRule(kIp6TablesPath, protocol, port, interface)) {
    // This worked, record this fact and insist that it works thereafter.
    ip6_enabled_ = true;
  } else if (ip6_enabled_) {
    // It's supposed to work, fail.
    LOG(ERROR) << "Could not add ACCEPT rule using '" << kIp6TablesPath
               << "', aborting operation.";
    DeleteAcceptRule(kIpTablesPath, protocol, port, interface);
    return false;
  } else {
    // It never worked, just ignore it.
    LOG(WARNING) << "Could not add ACCEPT rule using '" << kIp6TablesPath
                 << "', ignoring.";
  }

  return true;
}

bool IpTables::DeleteAcceptRules(ProtocolEnum protocol,
                                 uint16_t port,
                                 const std::string& interface) {
  bool ip4_success = DeleteAcceptRule(kIpTablesPath, protocol, port,
                                      interface);
  bool ip6_success = !ip6_enabled_ || DeleteAcceptRule(kIp6TablesPath, protocol,
                                                       port, interface);
  return ip4_success && ip6_success;
}

bool IpTables::ApplyVpnSetup(const std::vector<std::string>& usernames,
                             const std::string& interface,
                             bool add) {
  bool success = true;
  std::vector<std::string> added_usernames;

  if (!ApplyRuleForUserTraffic(add)) {
    if (add) {
      ApplyRuleForUserTraffic(false /* remove */);
      return false;
    }
    success = false;
  }

  if (!ApplyMasquerade(interface, add)) {
    if (add) {
      ApplyVpnSetup(added_usernames, interface, false /* remove */);
      return false;
    }
    success = false;
  }

  for (const auto& username : usernames) {
    if (!ApplyMarkForUserTraffic(username, add)) {
      if (add) {
        ApplyVpnSetup(added_usernames, interface, false /* remove */);
        return false;
      }
      success = false;
    }
    if (add) {
      added_usernames.push_back(username);
    }
  }

  return success;
}

bool IpTables::ApplyMasquerade(const std::string& interface, bool add) {
  const IpTablesCallback apply_masquerade =
      base::Bind(&IpTables::ApplyMasqueradeWithExecutable,
                 base::Unretained(this),
                 interface);

  return RunForAllArguments(
      apply_masquerade, {kIpTablesPath, kIp6TablesPath}, add);
}

bool IpTables::ApplyMarkForUserTraffic(const std::string& username, bool add) {
  const IpTablesCallback apply_mark =
      base::Bind(&IpTables::ApplyMarkForUserTrafficWithExecutable,
                 base::Unretained(this),
                 username);

  return RunForAllArguments(apply_mark, {kIpTablesPath, kIp6TablesPath}, add);
}

bool IpTables::ApplyRuleForUserTraffic(bool add) {
  const IpTablesCallback apply_rule = base::Bind(
      &IpTables::ApplyRuleForUserTrafficWithVersion, base::Unretained(this));

  return RunForAllArguments(apply_rule, {kIPv4, kIPv6}, add);
}

bool IpTables::AddAcceptRule(const std::string& executable_path,
                             ProtocolEnum protocol,
                             uint16_t port,
                             const std::string& interface) {
  std::vector<std::string> argv;
  argv.push_back(executable_path);
  argv.push_back("-I");  // insert
  argv.push_back("INPUT");
  argv.push_back("-p");  // protocol
  argv.push_back(protocol == kProtocolTcp ? "tcp" : "udp");
  argv.push_back("--dport");  // destination port
  argv.push_back(std::to_string(port));
  if (!interface.empty()) {
    argv.push_back("-i");  // interface
    argv.push_back(interface);
  }
  argv.push_back("-j");
  argv.push_back("ACCEPT");
  argv.push_back("-w");  // Wait for xtables lock.

  // Use CAP_NET_ADMIN|CAP_NET_RAW.
  return ExecvNonRoot(argv, kIpTablesCapMask) == 0;
}

bool IpTables::DeleteAcceptRule(const std::string& executable_path,
                                ProtocolEnum protocol,
                                uint16_t port,
                                const std::string& interface) {
  std::vector<std::string> argv;
  argv.push_back(executable_path);
  argv.push_back("-D");  // delete
  argv.push_back("INPUT");
  argv.push_back("-p");  // protocol
  argv.push_back(protocol == kProtocolTcp ? "tcp" : "udp");
  argv.push_back("--dport");  // destination port
  argv.push_back(std::to_string(port));
  if (interface != "") {
    argv.push_back("-i");  // interface
    argv.push_back(interface);
  }
  argv.push_back("-j");
  argv.push_back("ACCEPT");
  argv.push_back("-w");  // Wait for xtables lock.

  // Use CAP_NET_ADMIN|CAP_NET_RAW.
  return ExecvNonRoot(argv, kIpTablesCapMask) == 0;
}

bool IpTables::ApplyMasqueradeWithExecutable(const std::string& interface,
                                             const std::string& executable_path,
                                             bool add) {
  std::vector<std::string> argv;
  argv.push_back(executable_path);
  argv.push_back("-t");  // table
  argv.push_back("nat");
  argv.push_back(add ? "-A" : "-D");  // rule
  argv.push_back("POSTROUTING");
  argv.push_back("-o");  // output interface
  argv.push_back(interface);
  argv.push_back("-j");
  argv.push_back("MASQUERADE");

  // Use CAP_NET_ADMIN|CAP_NET_RAW.
  bool success = ExecvNonRoot(argv, kIpTablesCapMask) == 0;

  if (!success) {
    LOG(ERROR) << (add ? "Adding" : "Removing")
               << " masquerade failed for interface " << interface
               << " using '" << executable_path << "'";
  }
  return success;
}

bool IpTables::ApplyMarkForUserTrafficWithExecutable(
    const std::string& username, const std::string& executable_path, bool add) {
  std::vector<std::string> argv;
  argv.push_back(executable_path);
  argv.push_back("-t");  // table
  argv.push_back("mangle");
  argv.push_back(add ? "-A" : "-D");  // rule
  argv.push_back("OUTPUT");
  argv.push_back("-m");
  argv.push_back("owner");
  argv.push_back("--uid-owner");
  argv.push_back(username);
  argv.push_back("-j");
  argv.push_back("MARK");
  argv.push_back("--set-mark");
  argv.push_back(kMarkForUserTraffic);

  // Use CAP_NET_ADMIN|CAP_NET_RAW.
  bool success = ExecvNonRoot(argv, kIpTablesCapMask) == 0;

  if (!success) {
      LOG(ERROR) << (add ? "Adding" : "Removing")
                 << " mark failed for user " << username
                 << " using '" << kIpTablesPath << "'";
  }
  return success;
}

bool IpTables::ApplyRuleForUserTrafficWithVersion(const std::string& ip_version,
                                                  bool add) {
  brillo::ProcessImpl ip;
  ip.AddArg(kIpPath);
  if (ip_version == kIPv6)
    ip.AddArg("-6");
  ip.AddArg("rule");
  ip.AddArg(add ? "add" : "delete");
  ip.AddArg("fwmark");
  ip.AddArg(kMarkForUserTraffic);
  ip.AddArg("table");
  ip.AddArg(kTableIdForUserTraffic);

  bool success = ip.Run() == 0;

  if (!success) {
    LOG(ERROR) << (add ? "Adding" : "Removing") << " rule for " << ip_version
               << " user traffic failed";
  }
  return success;
}

int IpTables::ExecvNonRoot(const std::vector<std::string>& argv,
                           uint64_t capmask) {
  brillo::Minijail* m = brillo::Minijail::GetInstance();
  minijail* jail = m->New();
#if !defined(__ANDROID__)
  // TODO(garnold) This needs to be re-enabled once we figure out which
  // unprivileged user we want to use.
  m->DropRoot(jail, kUnprivilegedUser, kUnprivilegedUser);
#endif  // __ANDROID__
  m->UseCapabilities(jail, capmask);

  std::vector<char*> args;
  for (const auto& arg : argv) {
    args.push_back(const_cast<char*>(arg.c_str()));
  }
  args.push_back(nullptr);

  int status;
  bool ran = m->RunSyncAndDestroy(jail, args, &status);
  return ran ? status : -1;
}

}  // namespace firewalld
