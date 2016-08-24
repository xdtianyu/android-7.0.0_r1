// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/wifi_manager.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <fstream>

#include <base/bind.h>
#include <weave/provider/task_runner.h>

#include "examples/provider/event_network.h"
#include "examples/provider/ssl_stream.h"

namespace weave {
namespace examples {

namespace {

int ForkCmd(const std::string& path, const std::vector<std::string>& args) {
  int pid = fork();
  if (pid != 0)
    return pid;

  std::vector<const char*> args_vector;
  args_vector.push_back(path.c_str());
  for (auto& i : args)
    args_vector.push_back(i.c_str());
  args_vector.push_back(nullptr);
  execvp(path.c_str(), const_cast<char**>(args_vector.data()));
  NOTREACHED();
  return 0;
}

int ForkCmdAndWait(const std::string& path,
                   const std::vector<std::string>& args) {
  int pid = ForkCmd(path, args);
  int status = 0;
  CHECK_EQ(pid, waitpid(pid, &status, 0));
  return status;
}

std::string FindWirelessInterface() {
  std::string sysfs_net{"/sys/class/net"};
  DIR* net_dir = opendir(sysfs_net.c_str());
  dirent* iface;
  while ((iface = readdir(net_dir))) {
    auto path = sysfs_net + "/" + iface->d_name + "/wireless";
    DIR* wireless_dir = opendir(path.c_str());
    if (wireless_dir != nullptr) {
      closedir(net_dir);
      closedir(wireless_dir);
      return iface->d_name;
    }
  }
  closedir(net_dir);
  return "";
}

}  // namespace

WifiImpl::WifiImpl(provider::TaskRunner* task_runner, EventNetworkImpl* network)
  : task_runner_{task_runner}, network_{network}, iface_{FindWirelessInterface()} {
  CHECK(!iface_.empty()) <<  "WiFi interface not found";
  CHECK_EQ(0u, getuid())
      << "\nWiFi manager expects root access to control WiFi capabilities";
  StopAccessPoint();
}
WifiImpl::~WifiImpl() {
  StopAccessPoint();
}

void WifiImpl::TryToConnect(const std::string& ssid,
                            const std::string& passphrase,
                            int pid,
                            base::Time until,
                            const DoneCallback& callback) {
  if (pid) {
    int status = 0;
    if (pid == waitpid(pid, &status, WNOWAIT)) {
      int sockf_d = socket(AF_INET, SOCK_DGRAM, 0);
      CHECK_GE(sockf_d, 0) << strerror(errno);

      iwreq wreq = {};
      strncpy(wreq.ifr_name, iface_.c_str(), sizeof(wreq.ifr_name));
      std::string essid(' ', IW_ESSID_MAX_SIZE + 1);
      wreq.u.essid.pointer = &essid[0];
      wreq.u.essid.length = essid.size();
      CHECK_GE(ioctl(sockf_d, SIOCGIWESSID, &wreq), 0) << strerror(errno);
      essid.resize(wreq.u.essid.length);
      close(sockf_d);

      if (ssid == essid)
        return task_runner_->PostDelayedTask(FROM_HERE,
                                             base::Bind(callback, nullptr), {});
      pid = 0;  // Try again.
    }
  }

  if (pid == 0) {
    pid = ForkCmd("nmcli",
                  {"dev", "wifi", "connect", ssid, "password", passphrase});
  }

  if (base::Time::Now() >= until) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "timeout",
                 "Timeout connecting to WiFI network.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)), {});
    return;
  }

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&WifiImpl::TryToConnect, weak_ptr_factory_.GetWeakPtr(), ssid,
                 passphrase, pid, until, callback),
      base::TimeDelta::FromSeconds(1));
}

void WifiImpl::Connect(const std::string& ssid,
                       const std::string& passphrase,
                       const DoneCallback& callback) {
  network_->SetSimulateOffline(false);
  CHECK(!hostapd_started_);
  if (hostapd_started_) {
    ErrorPtr error;
    Error::AddTo(&error, FROM_HERE, "busy", "Running Access Point.");
    task_runner_->PostDelayedTask(
        FROM_HERE, base::Bind(callback, base::Passed(&error)), {});
    return;
  }

  TryToConnect(ssid, passphrase, 0,
               base::Time::Now() + base::TimeDelta::FromMinutes(1), callback);
}

void WifiImpl::StartAccessPoint(const std::string& ssid) {
  if (hostapd_started_)
    return;

  // Release wifi interface.
  CHECK_EQ(0, ForkCmdAndWait("nmcli", {"nm", "wifi",  "off"}));
  CHECK_EQ(0, ForkCmdAndWait("rfkill", {"unblock", "wlan"}));
  sleep(1);

  std::string hostapd_conf = "/tmp/weave_hostapd.conf";
  {
    std::ofstream ofs(hostapd_conf);
    ofs << "interface=" << iface_ << std::endl;
    ofs << "channel=1" << std::endl;
    ofs << "ssid=" << ssid << std::endl;
  }

  CHECK_EQ(0, ForkCmdAndWait("hostapd", {"-B", "-K", hostapd_conf}));
  hostapd_started_ = true;

  for (size_t i = 0; i < 10; ++i) {
    if (0 == ForkCmdAndWait("ifconfig", {iface_, "192.168.76.1/24"}))
      break;
    sleep(1);
  }

  std::string dnsmasq_conf = "/tmp/weave_dnsmasq.conf";
  {
    std::ofstream ofs(dnsmasq_conf.c_str());
    ofs << "port=0" << std::endl;
    ofs << "bind-interfaces" << std::endl;
    ofs << "log-dhcp" << std::endl;
    ofs << "dhcp-range=192.168.76.10,192.168.76.100" << std::endl;
    ofs << "interface=" << iface_ << std::endl;
    ofs << "dhcp-leasefile=" << dnsmasq_conf << ".leases" << std::endl;
  }

  CHECK_EQ(0, ForkCmdAndWait("dnsmasq", {"--conf-file=" + dnsmasq_conf}));
}

void WifiImpl::StopAccessPoint() {
  base::IgnoreResult(ForkCmdAndWait("pkill", {"-f", "dnsmasq.*/tmp/weave"}));
  base::IgnoreResult(ForkCmdAndWait("pkill", {"-f", "hostapd.*/tmp/weave"}));
  CHECK_EQ(0, ForkCmdAndWait("nmcli", {"nm", "wifi", "on"}));
  hostapd_started_ = false;
}

bool WifiImpl::HasWifiCapability() {
  return !FindWirelessInterface().empty();
}

}  // namespace examples
}  // namespace weave
