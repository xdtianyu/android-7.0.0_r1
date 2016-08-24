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

#include "shill/device_info.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/ether.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include <base/bind.h>
#include <base/files/file_enumerator.h>
#include <base/files/file_util.h>
#include <base/files/scoped_file.h>
#include <base/stl_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "shill/control_interface.h"
#include "shill/device.h"
#include "shill/device_stub.h"
#include "shill/ethernet/ethernet.h"
#include "shill/ethernet/virtio_ethernet.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/net/ndisc.h"
#include "shill/net/rtnl_handler.h"
#include "shill/net/rtnl_listener.h"
#include "shill/net/rtnl_message.h"
#include "shill/net/shill_time.h"
#include "shill/net/sockets.h"
#include "shill/routing_table.h"
#include "shill/service.h"
#include "shill/vpn/vpn_provider.h"

#if !defined(DISABLE_WIFI)
#include "shill/net/netlink_attribute.h"
#include "shill/net/netlink_manager.h"
#include "shill/net/nl80211_message.h"
#include "shill/wifi/wifi.h"
#endif  // DISABLE_WIFI

using base::Bind;
using base::FileEnumerator;
using base::FilePath;
using base::StringPrintf;
using base::Unretained;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kDevice;
static string ObjectID(const DeviceInfo* d) { return "(device_info)"; }
}

// static
const char DeviceInfo::kModemPseudoDeviceNamePrefix[] = "pseudomodem";
const char DeviceInfo::kEthernetPseudoDeviceNamePrefix[] = "pseudoethernet";
const char DeviceInfo::kIgnoredDeviceNamePrefix[] = "veth";
const char DeviceInfo::kDeviceInfoRoot[] = "/sys/class/net";
const char DeviceInfo::kDriverCdcEther[] = "cdc_ether";
const char DeviceInfo::kDriverCdcNcm[] = "cdc_ncm";
const char DeviceInfo::kDriverGdmWiMax[] = "gdm_wimax";
const char DeviceInfo::kDriverVirtioNet[] = "virtio_net";
const char DeviceInfo::kInterfaceUevent[] = "uevent";
const char DeviceInfo::kInterfaceUeventWifiSignature[] = "DEVTYPE=wlan\n";
const char DeviceInfo::kInterfaceDevice[] = "device";
const char DeviceInfo::kInterfaceDriver[] = "device/driver";
const char DeviceInfo::kInterfaceTunFlags[] = "tun_flags";
const char DeviceInfo::kInterfaceType[] = "type";
const char* DeviceInfo::kModemDrivers[] = {
    "gobi",
    "QCUSBNet2k",
    "GobiNet",
    "cdc_mbim",
    "qmi_wwan"
};
const char DeviceInfo::kTunDeviceName[] = "/dev/net/tun";
const int DeviceInfo::kDelayedDeviceCreationSeconds = 5;
const int DeviceInfo::kRequestLinkStatisticsIntervalMilliseconds = 20000;

DeviceInfo::DeviceInfo(ControlInterface* control_interface,
                       EventDispatcher* dispatcher,
                       Metrics* metrics,
                       Manager* manager)
    : control_interface_(control_interface),
      dispatcher_(dispatcher),
      metrics_(metrics),
      manager_(manager),
      link_callback_(Bind(&DeviceInfo::LinkMsgHandler, Unretained(this))),
      address_callback_(Bind(&DeviceInfo::AddressMsgHandler, Unretained(this))),
      rdnss_callback_(Bind(&DeviceInfo::RdnssMsgHandler, Unretained(this))),
      device_info_root_(kDeviceInfoRoot),
      routing_table_(RoutingTable::GetInstance()),
      rtnl_handler_(RTNLHandler::GetInstance()),
#if !defined(DISABLE_WIFI)
      netlink_manager_(NetlinkManager::GetInstance()),
#endif  // DISABLE_WIFI
      sockets_(new Sockets()),
      time_(Time::GetInstance()) {
}

DeviceInfo::~DeviceInfo() {}

void DeviceInfo::AddDeviceToBlackList(const string& device_name) {
  black_list_.insert(device_name);
  // Remove the current device info if it exist, since it will be out-dated.
  RemoveInfo(GetIndex(device_name));
  // Request link info update to allow device info to be recreated.
  if (manager_->running()) {
    rtnl_handler_->RequestDump(RTNLHandler::kRequestLink);
  }
}

void DeviceInfo::RemoveDeviceFromBlackList(const string& device_name) {
  black_list_.erase(device_name);
  // Remove the current device info if it exist, since it will be out-dated.
  RemoveInfo(GetIndex(device_name));
  // Request link info update to allow device info to be recreated.
  if (manager_->running()) {
    rtnl_handler_->RequestDump(RTNLHandler::kRequestLink);
  }
}

bool DeviceInfo::IsDeviceBlackListed(const string& device_name) {
  return ContainsKey(black_list_, device_name);
}

void DeviceInfo::Start() {
  link_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestLink, link_callback_));
  address_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestAddr, address_callback_));
  rdnss_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestRdnss, rdnss_callback_));
  rtnl_handler_->RequestDump(RTNLHandler::kRequestLink |
                             RTNLHandler::kRequestAddr);
  request_link_statistics_callback_.Reset(
      Bind(&DeviceInfo::RequestLinkStatistics, AsWeakPtr()));
  dispatcher_->PostDelayedTask(request_link_statistics_callback_.callback(),
                               kRequestLinkStatisticsIntervalMilliseconds);
}

void DeviceInfo::Stop() {
  link_listener_.reset();
  address_listener_.reset();
  infos_.clear();
  request_link_statistics_callback_.Cancel();
  delayed_devices_callback_.Cancel();
  delayed_devices_.clear();
}

vector<string> DeviceInfo::GetUninitializedTechnologies() const {
  set<string> unique_technologies;
  set<Technology::Identifier> initialized_technologies;
  for (const auto& info : infos_) {
    Technology::Identifier technology = info.second.technology;
    if (info.second.device) {
      // If there is more than one device for a technology and at least
      // one of them has been initialized, make sure that it doesn't get
      // listed as uninitialized.
      initialized_technologies.insert(technology);
      unique_technologies.erase(Technology::NameFromIdentifier(technology));
      continue;
    }
    if (Technology::IsPrimaryConnectivityTechnology(technology) &&
        !ContainsKey(initialized_technologies, technology))
      unique_technologies.insert(Technology::NameFromIdentifier(technology));
  }
  return vector<string>(unique_technologies.begin(), unique_technologies.end());
}

void DeviceInfo::RegisterDevice(const DeviceRefPtr& device) {
  SLOG(this, 2) << __func__ << "(" << device->link_name() << ", "
                << device->interface_index() << ")";
  device->Initialize();
  delayed_devices_.erase(device->interface_index());
  CHECK(!GetDevice(device->interface_index()).get());
  infos_[device->interface_index()].device = device;
  if (metrics_->IsDeviceRegistered(device->interface_index(),
                                   device->technology())) {
    metrics_->NotifyDeviceInitialized(device->interface_index());
  } else {
    metrics_->RegisterDevice(device->interface_index(), device->technology());
  }
  if (Technology::IsPrimaryConnectivityTechnology(device->technology())) {
    manager_->RegisterDevice(device);
  }
}

void DeviceInfo::DeregisterDevice(const DeviceRefPtr& device) {
  int interface_index = device->interface_index();

  SLOG(this, 2) << __func__ << "(" << device->link_name() << ", "
                << interface_index << ")";
  CHECK((device->technology() == Technology::kCellular) ||
        (device->technology() == Technology::kWiMax));

  // Release reference to the device
  map<int, Info>::iterator iter = infos_.find(interface_index);
  if (iter != infos_.end()) {
    SLOG(this, 2) << "Removing device from info for index: "
                  << interface_index;
    manager_->DeregisterDevice(device);
    // Release the reference to the device, but maintain the mapping
    // for the index.  That will be cleaned up by an RTNL message.
    iter->second.device = nullptr;
  }
  metrics_->DeregisterDevice(device->interface_index());
}

FilePath DeviceInfo::GetDeviceInfoPath(const string& iface_name,
                                       const string& path_name) {
  return device_info_root_.Append(iface_name).Append(path_name);
}

bool DeviceInfo::GetDeviceInfoContents(const string& iface_name,
                                       const string& path_name,
                                       string* contents_out) {
  return base::ReadFileToString(GetDeviceInfoPath(iface_name, path_name),
                                contents_out);
}

bool DeviceInfo::GetDeviceInfoSymbolicLink(const string& iface_name,
                                           const string& path_name,
                                           FilePath* path_out) {
  return base::ReadSymbolicLink(GetDeviceInfoPath(iface_name, path_name),
                                path_out);
}

Technology::Identifier DeviceInfo::GetDeviceTechnology(
    const string& iface_name) {
  string type_string;
  int arp_type = ARPHRD_VOID;
  if (GetDeviceInfoContents(iface_name, kInterfaceType, &type_string) &&
      base::TrimString(type_string, "\n", &type_string) &&
      !base::StringToInt(type_string, &arp_type)) {
    arp_type = ARPHRD_VOID;
  }

  string contents;
  if (!GetDeviceInfoContents(iface_name, kInterfaceUevent, &contents)) {
    LOG(INFO) << StringPrintf("%s: device %s has no uevent file",
                              __func__, iface_name.c_str());
    return Technology::kUnknown;
  }

  // If the "uevent" file contains the string "DEVTYPE=wlan\n" at the
  // start of the file or after a newline, we can safely assume this
  // is a wifi device.
  if (contents.find(kInterfaceUeventWifiSignature) != string::npos) {
    SLOG(this, 2)
        << StringPrintf("%s: device %s has wifi signature in uevent file",
                        __func__, iface_name.c_str());
    if (arp_type == ARPHRD_IEEE80211_RADIOTAP) {
      SLOG(this, 2) << StringPrintf("%s: wifi device %s is in monitor mode",
                                    __func__, iface_name.c_str());
      return Technology::kWiFiMonitor;
    }
    return Technology::kWifi;
  }

  // Special case for pseudo modems which are used for testing
  if (iface_name.find(kModemPseudoDeviceNamePrefix) == 0) {
    SLOG(this, 2) << StringPrintf(
        "%s: device %s is a pseudo modem for testing",
        __func__, iface_name.c_str());
    return Technology::kCellular;
  }

  // Special case for pseudo ethernet devices which are used for testing.
  if (iface_name.find(kEthernetPseudoDeviceNamePrefix) == 0) {
    SLOG(this, 2) << StringPrintf(
        "%s: device %s is a virtual ethernet device for testing",
        __func__, iface_name.c_str());
    return Technology::kEthernet;
  }

  // Special case for devices which should be ignored.
  if (iface_name.find(kIgnoredDeviceNamePrefix) == 0) {
    SLOG(this, 2) << StringPrintf(
        "%s: device %s should be ignored", __func__, iface_name.c_str());
    return Technology::kUnknown;
  }

  FilePath driver_path;
  if (!GetDeviceInfoSymbolicLink(iface_name, kInterfaceDriver, &driver_path)) {
    SLOG(this, 2) << StringPrintf("%s: device %s has no device symlink",
                                  __func__, iface_name.c_str());
    if (arp_type == ARPHRD_LOOPBACK) {
      SLOG(this, 2) << StringPrintf("%s: device %s is a loopback device",
                                    __func__, iface_name.c_str());
      return Technology::kLoopback;
    }
    if (arp_type == ARPHRD_PPP) {
      SLOG(this, 2) << StringPrintf("%s: device %s is a ppp device",
                                    __func__, iface_name.c_str());
      return Technology::kPPP;
    }
    string tun_flags_str;
    int tun_flags = 0;
    if (GetDeviceInfoContents(iface_name, kInterfaceTunFlags, &tun_flags_str) &&
        base::TrimString(tun_flags_str, "\n", &tun_flags_str) &&
        base::HexStringToInt(tun_flags_str, &tun_flags) &&
        (tun_flags & IFF_TUN)) {
      SLOG(this, 2) << StringPrintf("%s: device %s is tun device",
                                    __func__, iface_name.c_str());
      return Technology::kTunnel;
    }

    // We don't know what sort of device it is.
    return Technology::kNoDeviceSymlink;
  }

  string driver_name(driver_path.BaseName().value());
  // See if driver for this interface is in a list of known modem driver names.
  for (size_t modem_idx = 0; modem_idx < arraysize(kModemDrivers);
       ++modem_idx) {
    if (driver_name == kModemDrivers[modem_idx]) {
      SLOG(this, 2)
          << StringPrintf("%s: device %s is matched with modem driver %s",
                          __func__, iface_name.c_str(), driver_name.c_str());
      return Technology::kCellular;
    }
  }

  if (driver_name == kDriverGdmWiMax) {
    SLOG(this, 2) << StringPrintf("%s: device %s is a WiMAX device",
                                  __func__, iface_name.c_str());
    return Technology::kWiMax;
  }

  // For cdc_ether / cdc_ncm devices, make sure it's a modem because this driver
  // can be used for other ethernet devices.
  if (driver_name == kDriverCdcEther || driver_name == kDriverCdcNcm) {
    if (IsCdcEthernetModemDevice(iface_name)) {
      LOG(INFO) << StringPrintf("%s: device %s is a %s modem device", __func__,
                                iface_name.c_str(), driver_name.c_str());
      return Technology::kCellular;
    }
    SLOG(this, 2) << StringPrintf("%s: device %s is a %s device", __func__,
                                  iface_name.c_str(), driver_name.c_str());
    return Technology::kCDCEthernet;
  }

  // Special case for the virtio driver, used when run under KVM. See also
  // the comment in VirtioEthernet::Start.
  if (driver_name == kDriverVirtioNet) {
    SLOG(this, 2) << StringPrintf("%s: device %s is virtio ethernet",
                                  __func__, iface_name.c_str());
    return Technology::kVirtioEthernet;
  }

  SLOG(this, 2) << StringPrintf("%s: device %s, with driver %s, "
                                "is defaulted to type ethernet",
                                __func__, iface_name.c_str(),
                                driver_name.c_str());
  return Technology::kEthernet;
}

bool DeviceInfo::IsCdcEthernetModemDevice(const std::string& iface_name) {
  // A cdc_ether / cdc_ncm device is a modem device if it also exposes tty
  // interfaces. To determine this, we look for the existence of the tty
  // interface in the USB device sysfs tree.
  //
  // A typical sysfs dir hierarchy for a cdc_ether / cdc_ncm modem USB device is
  // as follows:
  //
  //   /sys/devices/pci0000:00/0000:00:1d.7/usb1/1-2
  //     1-2:1.0
  //       tty
  //         ttyACM0
  //     1-2:1.1
  //       net
  //         usb0
  //     1-2:1.2
  //       tty
  //         ttyACM1
  //       ...
  //
  // /sys/class/net/usb0/device symlinks to
  // /sys/devices/pci0000:00/0000:00:1d.7/usb1/1-2/1-2:1.1
  //
  // Note that some modem devices have the tty directory one level deeper
  // (eg. E362), so the device tree for the tty interface is:
  // /sys/devices/pci0000:00/0000:00:1d.7/usb/1-2/1-2:1.0/ttyUSB0/tty/ttyUSB0

  FilePath device_file = GetDeviceInfoPath(iface_name, kInterfaceDevice);
  FilePath device_path;
  if (!base::ReadSymbolicLink(device_file, &device_path)) {
    SLOG(this, 2) << StringPrintf("%s: device %s has no device symlink",
                                  __func__, iface_name.c_str());
    return false;
  }
  if (!device_path.IsAbsolute()) {
    device_path =
        base::MakeAbsoluteFilePath(device_file.DirName().Append(device_path));
  }

  // Look for tty interface by enumerating all directories under the parent
  // USB device and see if there's a subdirectory "tty" inside.  In other
  // words, using the example dir hierarchy above, find
  // /sys/devices/pci0000:00/0000:00:1d.7/usb1/1-2/.../tty.
  // If this exists, then this is a modem device.
  return HasSubdir(device_path.DirName(), FilePath("tty"));
}

// static
bool DeviceInfo::HasSubdir(const FilePath& base_dir, const FilePath& subdir) {
  FileEnumerator::FileType type = static_cast<FileEnumerator::FileType>(
      FileEnumerator::DIRECTORIES | FileEnumerator::SHOW_SYM_LINKS);
  FileEnumerator dir_enum(base_dir, true, type);
  for (FilePath curr_dir = dir_enum.Next(); !curr_dir.empty();
       curr_dir = dir_enum.Next()) {
    if (curr_dir.BaseName() == subdir)
      return true;
  }
  return false;
}

DeviceRefPtr DeviceInfo::CreateDevice(const string& link_name,
                                      const string& address,
                                      int interface_index,
                                      Technology::Identifier technology) {
  DeviceRefPtr device;
  delayed_devices_.erase(interface_index);
  infos_[interface_index].technology = technology;

  switch (technology) {
    case Technology::kCellular:
#if defined(DISABLE_CELLULAR)
      LOG(WARNING) << "Cellular support is not implemented. "
                   << "Ignore cellular device " << link_name << " at index "
                   << interface_index << ".";
      return nullptr;
#else
      // Cellular devices are managed by ModemInfo.
      SLOG(this, 2) << "Cellular link " << link_name
                    << " at index " << interface_index
                    << " -- notifying ModemInfo.";

      // The MAC address provided by RTNL is not reliable for Gobi 2K modems.
      // Clear it here, and it will be fetched from the kernel in
      // GetMACAddress().
      infos_[interface_index].mac_address.Clear();
      manager_->modem_info()->OnDeviceInfoAvailable(link_name);
      break;
#endif  // DISABLE_CELLULAR
    case Technology::kEthernet:
      device = new Ethernet(control_interface_, dispatcher_, metrics_,
                            manager_, link_name, address, interface_index);
      device->EnableIPv6Privacy();
      break;
    case Technology::kVirtioEthernet:
      device = new VirtioEthernet(control_interface_, dispatcher_, metrics_,
                                  manager_, link_name, address,
                                  interface_index);
      device->EnableIPv6Privacy();
      break;
    case Technology::kWifi:
#if defined(DISABLE_WIFI)
      LOG(WARNING) << "WiFi support is not implemented. Ignore WiFi link "
                   << link_name << " at index " << interface_index << ".";
      return nullptr;
#else
      // Defer creating this device until we get information about the
      // type of WiFi interface.
      GetWiFiInterfaceInfo(interface_index);
      break;
#endif  // DISABLE_WIFI
    case Technology::kWiMax:
#if defined(DISABLE_WIMAX)
      LOG(WARNING) << "WiMax support is not implemented. Ignore WiMax link "
                   << link_name << " at index " << interface_index << ".";
      return nullptr;
#else
      // WiMax devices are managed by WiMaxProvider.
      SLOG(this, 2) << "WiMax link " << link_name
                    << " at index " << interface_index
                    << " -- notifying WiMaxProvider.";
      // The MAC address provided by RTNL may not be the final value as the
      // WiMAX device may change the address after initialization. Clear it
      // here, and it will be fetched from the kernel when
      // WiMaxProvider::CreateDevice() is called after the WiMAX device DBus
      // object is created by the WiMAX manager daemon.
      infos_[interface_index].mac_address.Clear();
      manager_->wimax_provider()->OnDeviceInfoAvailable(link_name);
      break;
#endif  // DISABLE_WIMAX
    case Technology::kPPP:
    case Technology::kTunnel:
      // Tunnel and PPP devices are managed by the VPN code (PPP for
      // l2tpipsec).  Notify the VPN Provider of the interface's presence.
      // Since CreateDevice is only called once in the lifetime of an
      // interface index, this notification will only occur the first
      // time the device is seen.
      SLOG(this, 2) << "Tunnel / PPP link " << link_name
                    << " at index " << interface_index
                    << " -- notifying VPNProvider.";
      if (!manager_->vpn_provider()->OnDeviceInfoAvailable(link_name,
                                                           interface_index) &&
          technology == Technology::kTunnel) {
        // If VPN does not know anything about this tunnel, it is probably
        // left over from a previous instance and should not exist.
        SLOG(this, 2) << "Tunnel link is unused.  Deleting.";
        DeleteInterface(interface_index);
      }
      break;
    case Technology::kLoopback:
      // Loopback devices are largely ignored, but we should make sure the
      // link is enabled.
      SLOG(this, 2) << "Bringing up loopback device " << link_name
                    << " at index " << interface_index;
      rtnl_handler_->SetInterfaceFlags(interface_index, IFF_UP, IFF_UP);
      return nullptr;
    case Technology::kCDCEthernet:
      // CDCEthernet devices are of indeterminate type when they are
      // initially created.  Some time later, tty devices may or may
      // not appear under the same USB device root, which will identify
      // it as a modem.  Alternatively, ModemManager may discover the
      // device and create and register a Cellular device.  In either
      // case, we should delay creating a Device until we can make a
      // better determination of what type this Device should be.
    case Technology::kNoDeviceSymlink:  // FALLTHROUGH
      // The same is true for devices that do not report a device
      // symlink.  It has been observed that tunnel devices may not
      // immediately contain a tun_flags component in their
      // /sys/class/net entry.
      LOG(INFO) << "Delaying creation of device for " << link_name
                << " at index " << interface_index;
      DelayDeviceCreation(interface_index);
      return nullptr;
    default:
      // We will not manage this device in shill.  Do not create a device
      // object or do anything to change its state.  We create a stub object
      // which is useful for testing.
      return new DeviceStub(control_interface_, dispatcher_, metrics_,
                            manager_, link_name, address, interface_index,
                            technology);
  }

  // Reset the routing table and addresses.
  routing_table_->FlushRoutes(interface_index);
  FlushAddresses(interface_index);

  manager_->UpdateUninitializedTechnologies();

  return device;
}

// static
bool DeviceInfo::GetLinkNameFromMessage(const RTNLMessage& msg,
                                        string* link_name) {
  if (!msg.HasAttribute(IFLA_IFNAME))
    return false;

  ByteString link_name_bytes(msg.GetAttribute(IFLA_IFNAME));
  link_name->assign(reinterpret_cast<const char*>(
      link_name_bytes.GetConstData()));

  return true;
}

bool DeviceInfo::IsRenamedBlacklistedDevice(const RTNLMessage& msg) {
  int interface_index = msg.interface_index();
  const Info* info = GetInfo(interface_index);
  if (!info)
    return false;

  if (!info->device || info->device->technology() != Technology::kBlacklisted)
    return false;

  string interface_name;
  if (!GetLinkNameFromMessage(msg, &interface_name))
    return false;

  if (interface_name == info->name)
    return false;

  LOG(INFO) << __func__ << ": interface index " << interface_index
            << " renamed from " << info->name << " to " << interface_name;
  return true;
}


void DeviceInfo::AddLinkMsgHandler(const RTNLMessage& msg) {
  DCHECK(msg.type() == RTNLMessage::kTypeLink &&
         msg.mode() == RTNLMessage::kModeAdd);
  int dev_index = msg.interface_index();
  Technology::Identifier technology = Technology::kUnknown;
  unsigned int flags = msg.link_status().flags;
  unsigned int change = msg.link_status().change;

  if (IsRenamedBlacklistedDevice(msg)) {
    // Treat renamed blacklisted devices as new devices.
    RemoveInfo(dev_index);
  }

  bool new_device =
      !ContainsKey(infos_, dev_index) || infos_[dev_index].has_addresses_only;
  SLOG(this, 2) << __func__ << "(index=" << dev_index
                << std::showbase << std::hex
                << ", flags=" << flags << ", change=" << change << ")"
                << std::dec << std::noshowbase
                << ", new_device=" << new_device;
  infos_[dev_index].has_addresses_only = false;
  infos_[dev_index].flags = flags;

  RetrieveLinkStatistics(dev_index, msg);

  DeviceRefPtr device = GetDevice(dev_index);
  if (new_device) {
    CHECK(!device);
    string link_name;
    if (!GetLinkNameFromMessage(msg, &link_name)) {
      LOG(ERROR) << "Add Link message does not contain a link name!";
      return;
    }
    SLOG(this, 2) << "add link index "  << dev_index << " name " << link_name;
    infos_[dev_index].name = link_name;
    indices_[link_name] = dev_index;

    if (!link_name.empty()) {
      if (IsDeviceBlackListed(link_name)) {
        technology = Technology::kBlacklisted;
      } else if (!manager_->DeviceManagementAllowed(link_name)) {
        technology = Technology::kBlacklisted;
        AddDeviceToBlackList(link_name);
      } else {
        technology = GetDeviceTechnology(link_name);
      }
    }
    string address;
    if (msg.HasAttribute(IFLA_ADDRESS)) {
      infos_[dev_index].mac_address = msg.GetAttribute(IFLA_ADDRESS);
      address =
          base::ToLowerASCII(infos_[dev_index].mac_address.HexEncode());
      SLOG(this, 2) << "link index " << dev_index << " address "
                    << infos_[dev_index].mac_address.HexEncode();
    } else if (technology != Technology::kTunnel &&
               technology != Technology::kPPP &&
               technology != Technology::kNoDeviceSymlink) {
      LOG(ERROR) << "Add Link message for link '" << link_name
                 << "' does not have IFLA_ADDRESS!";
      return;
    }
    metrics_->RegisterDevice(dev_index, technology);
    device = CreateDevice(link_name, address, dev_index, technology);
    if (device) {
      RegisterDevice(device);
    }
  }
  if (device) {
    device->LinkEvent(flags, change);
  }
}

void DeviceInfo::DelLinkMsgHandler(const RTNLMessage& msg) {
  SLOG(this, 2) << __func__ << "(index=" << msg.interface_index() << ")";

  DCHECK(msg.type() == RTNLMessage::kTypeLink &&
         msg.mode() == RTNLMessage::kModeDelete);
  SLOG(this, 2) << __func__ << "(index=" << msg.interface_index()
                << std::showbase << std::hex
                  << ", flags=" << msg.link_status().flags
                  << ", change=" << msg.link_status().change << ")";
  RemoveInfo(msg.interface_index());
}

DeviceRefPtr DeviceInfo::GetDevice(int interface_index) const {
  const Info* info = GetInfo(interface_index);
  return info ? info->device : nullptr;
}

int DeviceInfo::GetIndex(const string& interface_name) const {
  map<string, int>::const_iterator it = indices_.find(interface_name);
  return it == indices_.end() ? -1 : it->second;
}

bool DeviceInfo::GetMACAddress(int interface_index, ByteString* address) const {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  // |mac_address| from RTNL is not used for some devices, in which case it will
  // be empty here.
  if (!info->mac_address.IsEmpty()) {
    *address = info->mac_address;
    return true;
  }

  // Ask the kernel for the MAC address.
  *address = GetMACAddressFromKernel(interface_index);
  return !address->IsEmpty();
}

ByteString DeviceInfo::GetMACAddressFromKernel(int interface_index) const {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return ByteString();
  }

  const int fd = sockets_->Socket(PF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    PLOG(ERROR) << __func__ << ": Unable to open socket";
    return ByteString();
  }

  ScopedSocketCloser socket_closer(sockets_.get(), fd);
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_ifindex = interface_index;
  strcpy(ifr.ifr_ifrn.ifrn_name, info->name.c_str());  // NOLINT(runtime/printf)
  int err = sockets_->Ioctl(fd, SIOCGIFHWADDR, &ifr);
  if (err < 0) {
    PLOG(ERROR) << __func__ << ": Unable to read MAC address";
    return ByteString();
  }

  return ByteString(ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
}

bool DeviceInfo::GetMACAddressOfPeer(int interface_index,
                                     const IPAddress& peer,
                                     ByteString* mac_address) const {
  const Info* info = GetInfo(interface_index);
  if (!info || !peer.IsValid()) {
    return false;
  }

  if (peer.family() != IPAddress::kFamilyIPv4) {
    NOTIMPLEMENTED() << ": only implemented for IPv4";
    return false;
  }

  const int fd = sockets_->Socket(PF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    PLOG(ERROR) << __func__ << ": Unable to open socket";
    return false;
  }

  ScopedSocketCloser socket_closer(sockets_.get(), fd);
  struct arpreq areq;
  memset(&areq, 0, sizeof(areq));

  strncpy(areq.arp_dev, info->name.c_str(), sizeof(areq.arp_dev) - 1);
  areq.arp_dev[sizeof(areq.arp_dev) - 1] = '\0';

  struct sockaddr_in* protocol_address =
      reinterpret_cast<struct sockaddr_in*>(&areq.arp_pa);
  protocol_address->sin_family = AF_INET;
  CHECK_EQ(sizeof(protocol_address->sin_addr.s_addr), peer.GetLength());
  memcpy(&protocol_address->sin_addr.s_addr, peer.address().GetConstData(),
         sizeof(protocol_address->sin_addr.s_addr));

  struct sockaddr_in* hardware_address =
      reinterpret_cast<struct sockaddr_in*>(&areq.arp_ha);
  hardware_address->sin_family = ARPHRD_ETHER;

  int err = sockets_->Ioctl(fd, SIOCGARP, &areq);
  if (err < 0) {
    PLOG(ERROR) << __func__ << ": Unable to perform ARP lookup";
    return false;
  }

  ByteString peer_address(areq.arp_ha.sa_data, IFHWADDRLEN);

  if (peer_address.IsZero()) {
    LOG(INFO) << __func__ << ": ARP lookup is still in progress";
    return false;
  }

  CHECK(mac_address);
  *mac_address = peer_address;
  return true;
}

bool DeviceInfo::GetAddresses(int interface_index,
                              vector<AddressData>* addresses) const {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  *addresses = info->ip_addresses;
  return true;
}

void DeviceInfo::FlushAddresses(int interface_index) const {
  SLOG(this, 2) << __func__ << "(" << interface_index << ")";
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return;
  }
  for (const auto& address_info : info->ip_addresses) {
    if (address_info.address.family() == IPAddress::kFamilyIPv4 ||
        (address_info.scope == RT_SCOPE_UNIVERSE &&
         (address_info.flags & ~IFA_F_TEMPORARY) == 0)) {
      SLOG(this, 2) << __func__ << ": removing ip address "
                    << address_info.address.ToString()
                    << " from interface " << interface_index;
      rtnl_handler_->RemoveInterfaceAddress(interface_index,
                                            address_info.address);
    }
  }
}

bool DeviceInfo::HasOtherAddress(
    int interface_index, const IPAddress& this_address) const {
  SLOG(this, 3) << __func__ << "(" << interface_index << ")";
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  bool has_other_address = false;
  bool has_this_address = false;
  for (const auto& local_address : info->ip_addresses) {
    if (local_address.address.family() != this_address.family()) {
      continue;
    }
    if (local_address.address.address().Equals(this_address.address())) {
      has_this_address = true;
    } else if (this_address.family() == IPAddress::kFamilyIPv4) {
      has_other_address = true;
    } else if ((local_address.scope == RT_SCOPE_UNIVERSE &&
                (local_address.flags & IFA_F_TEMPORARY) == 0)) {
      has_other_address = true;
    }
  }
  return has_other_address && !has_this_address;
}

bool DeviceInfo::GetPrimaryIPv6Address(int interface_index,
                                       IPAddress* address) {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  bool has_temporary_address = false;
  bool has_current_address = false;
  bool has_address = false;
  for (const auto& local_address : info->ip_addresses) {
    if (local_address.address.family() != IPAddress::kFamilyIPv6 ||
        local_address.scope != RT_SCOPE_UNIVERSE) {
      continue;
    }

    // Prefer non-deprecated addresses to deprecated addresses to match the
    // kernel's preference.
    bool is_current_address =
        ((local_address.flags & IFA_F_DEPRECATED) == 0);
    if (has_current_address && !is_current_address) {
      continue;
    }

    // Prefer temporary addresses to non-temporary addresses to match the
    // kernel's preference.
    bool is_temporary_address = ((local_address.flags & IFA_F_TEMPORARY) != 0);
    if (has_temporary_address && !is_temporary_address) {
      continue;
    }

    *address = local_address.address;
    has_temporary_address = is_temporary_address;
    has_current_address = is_current_address;
    has_address = true;
  }

  return has_address;
}

bool DeviceInfo::GetIPv6DnsServerAddresses(int interface_index,
                                           std::vector<IPAddress>* address_list,
                                           uint32_t* life_time) {
  const Info* info = GetInfo(interface_index);
  if (!info || info->ipv6_dns_server_addresses.empty()) {
    return false;
  }

  // Determine the remaining DNS server life time.
  if (info->ipv6_dns_server_lifetime_seconds == ND_OPT_LIFETIME_INFINITY) {
    *life_time = ND_OPT_LIFETIME_INFINITY;
  } else {
    time_t cur_time;
    if (!time_->GetSecondsBoottime(&cur_time)) {
      NOTREACHED();
    }
    uint32_t time_elapsed = static_cast<uint32_t>(
        cur_time - info->ipv6_dns_server_received_time_seconds);
    if (time_elapsed >= info->ipv6_dns_server_lifetime_seconds) {
      *life_time = 0;
    } else {
      *life_time = info->ipv6_dns_server_lifetime_seconds - time_elapsed;
    }
  }
  *address_list = info->ipv6_dns_server_addresses;
  return true;
}

bool DeviceInfo::HasDirectConnectivityTo(
    int interface_index, const IPAddress& address) const {
  SLOG(this, 3) << __func__ << "(" << interface_index << ")";
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }

  for (const auto& local_address : info->ip_addresses) {
    if (local_address.address.family() == address.family() &&
        local_address.address.CanReachAddress(address)) {
      return true;
    }
  }

  return false;
}

bool DeviceInfo::GetFlags(int interface_index, unsigned int* flags) const {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  *flags = info->flags;
  return true;
}

bool DeviceInfo::GetByteCounts(int interface_index,
                               uint64_t* rx_bytes,
                               uint64_t* tx_bytes) const {
  const Info* info = GetInfo(interface_index);
  if (!info) {
    return false;
  }
  *rx_bytes = info->rx_bytes;
  *tx_bytes = info->tx_bytes;
  return true;
}

bool DeviceInfo::CreateTunnelInterface(string* interface_name) const {
  int fd = HANDLE_EINTR(open(kTunDeviceName, O_RDWR));
  if (fd < 0) {
    PLOG(ERROR) << "failed to open " << kTunDeviceName;
    return false;
  }
  base::ScopedFD scoped_fd(fd);

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if (HANDLE_EINTR(ioctl(fd, TUNSETIFF, &ifr))) {
    PLOG(ERROR) << "failed to create tunnel interface";
    return false;
  }

  if (HANDLE_EINTR(ioctl(fd, TUNSETPERSIST, 1))) {
    PLOG(ERROR) << "failed to set tunnel interface to be persistent";
    return false;
  }

  *interface_name = string(ifr.ifr_name);

  return true;
}

int DeviceInfo::OpenTunnelInterface(const std::string& interface_name) const {
  int fd = HANDLE_EINTR(open(kTunDeviceName, O_RDWR));
  if (fd < 0) {
    PLOG(ERROR) << "failed to open " << kTunDeviceName;
    return -1;
  }

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, interface_name.c_str(), sizeof(ifr.ifr_name));
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if (HANDLE_EINTR(ioctl(fd, TUNSETIFF, &ifr))) {
    PLOG(ERROR) << "failed to set tunnel interface name";
    return -1;
  }

  return fd;
}

bool DeviceInfo::DeleteInterface(int interface_index) const {
  return rtnl_handler_->RemoveInterface(interface_index);
}

const DeviceInfo::Info* DeviceInfo::GetInfo(int interface_index) const {
  map<int, Info>::const_iterator iter = infos_.find(interface_index);
  if (iter == infos_.end()) {
    return nullptr;
  }
  return &iter->second;
}

void DeviceInfo::RemoveInfo(int interface_index) {
  map<int, Info>::iterator iter = infos_.find(interface_index);
  if (iter != infos_.end()) {
    SLOG(this, 2) << "Removing info for device index: " << interface_index;
    // Deregister the device if not deregistered yet. Cellular and WiMax devices
    // are deregistered through a call to DeviceInfo::DeregisterDevice.
    if (iter->second.device.get()) {
      manager_->DeregisterDevice(iter->second.device);
      metrics_->DeregisterDevice(interface_index);
    }
    indices_.erase(iter->second.name);
    infos_.erase(iter);
    delayed_devices_.erase(interface_index);
  } else {
    SLOG(this, 2) << __func__ << ": Unknown device index: "
                  << interface_index;
  }
}

void DeviceInfo::LinkMsgHandler(const RTNLMessage& msg) {
  DCHECK(msg.type() == RTNLMessage::kTypeLink);
  if (msg.mode() == RTNLMessage::kModeAdd) {
    AddLinkMsgHandler(msg);
  } else if (msg.mode() == RTNLMessage::kModeDelete) {
    DelLinkMsgHandler(msg);
  } else {
    NOTREACHED();
  }
}

void DeviceInfo::AddressMsgHandler(const RTNLMessage& msg) {
  SLOG(this, 2) << __func__;
  DCHECK(msg.type() == RTNLMessage::kTypeAddress);
  int interface_index = msg.interface_index();
  if (!ContainsKey(infos_, interface_index)) {
    SLOG(this, 2) << "Got advance address information for unknown index "
                  << interface_index;
    infos_[interface_index].has_addresses_only = true;
  }
  const RTNLMessage::AddressStatus& status = msg.address_status();
  IPAddress address(msg.family(),
                    msg.HasAttribute(IFA_LOCAL) ?
                    msg.GetAttribute(IFA_LOCAL) : msg.GetAttribute(IFA_ADDRESS),
                    status.prefix_len);

  SLOG_IF(Device, 2, msg.HasAttribute(IFA_LOCAL))
      << "Found local address attribute for interface " << interface_index;

  vector<AddressData>& address_list = infos_[interface_index].ip_addresses;
  vector<AddressData>::iterator iter;
  for (iter = address_list.begin(); iter != address_list.end(); ++iter) {
    if (address.Equals(iter->address)) {
      break;
    }
  }
  if (iter != address_list.end()) {
    if (msg.mode() == RTNLMessage::kModeDelete) {
      SLOG(this, 2) << "Delete address for interface " << interface_index;
      address_list.erase(iter);
    } else {
      iter->flags = status.flags;
      iter->scope = status.scope;
    }
  } else if (msg.mode() == RTNLMessage::kModeAdd) {
    address_list.push_back(AddressData(address, status.flags, status.scope));
    SLOG(this, 2) << "Add address " << address.ToString()
                  << " for interface " << interface_index;
  }

  DeviceRefPtr device = GetDevice(interface_index);
  if (device && address.family() == IPAddress::kFamilyIPv6 &&
      status.scope == RT_SCOPE_UNIVERSE) {
    device->OnIPv6AddressChanged();
  }
}

void DeviceInfo::RdnssMsgHandler(const RTNLMessage& msg) {
  SLOG(this, 2) << __func__;
  DCHECK(msg.type() == RTNLMessage::kTypeRdnss);
  int interface_index = msg.interface_index();
  if (!ContainsKey(infos_, interface_index)) {
    SLOG(this, 2) << "Got RDNSS option for unknown index "
                  << interface_index;
  }

  const RTNLMessage::RdnssOption& rdnss_option = msg.rdnss_option();
  infos_[interface_index].ipv6_dns_server_lifetime_seconds =
      rdnss_option.lifetime;
  infos_[interface_index].ipv6_dns_server_addresses = rdnss_option.addresses;
  if (!time_->GetSecondsBoottime(
          &infos_[interface_index].ipv6_dns_server_received_time_seconds)) {
    NOTREACHED();
  }

  // Notify device of the IPv6 DNS server addresses update.
  DeviceRefPtr device = GetDevice(interface_index);
  if (device) {
    device->OnIPv6DnsServerAddressesChanged();
  }
}

void DeviceInfo::DelayDeviceCreation(int interface_index) {
  delayed_devices_.insert(interface_index);
  delayed_devices_callback_.Reset(
      Bind(&DeviceInfo::DelayedDeviceCreationTask, AsWeakPtr()));
  dispatcher_->PostDelayedTask(delayed_devices_callback_.callback(),
                               kDelayedDeviceCreationSeconds * 1000);
}

// Re-evaluate the technology type for each delayed device.
void DeviceInfo::DelayedDeviceCreationTask() {
  while (!delayed_devices_.empty()) {
    set<int>::iterator it = delayed_devices_.begin();
    int dev_index = *it;
    delayed_devices_.erase(it);

    DCHECK(ContainsKey(infos_, dev_index));
    DCHECK(!GetDevice(dev_index));

    const string& link_name = infos_[dev_index].name;
    Technology::Identifier technology = GetDeviceTechnology(link_name);

    if (technology == Technology::kCDCEthernet) {
      LOG(INFO) << "In " << __func__ << ": device " << link_name
                << " is now assumed to be regular Ethernet.";
      technology = Technology::kEthernet;
    } else if (technology == Technology::kNoDeviceSymlink) {
      if (manager_->ignore_unknown_ethernet()) {
        SLOG(this, 2) << StringPrintf("%s: device %s, without driver name "
                                      "will be ignored",
                                      __func__, link_name.c_str());
        technology = Technology::kUnknown;
      } else {
        // Act the same as if there was a driver symlink, but we did not
        // recognize the driver name.
        SLOG(this, 2) << StringPrintf("%s: device %s, without driver name "
                                      "is defaulted to type ethernet",
                                      __func__, link_name.c_str());
        technology = Technology::kEthernet;
      }
    } else if (technology != Technology::kCellular &&
               technology != Technology::kTunnel) {
      LOG(WARNING) << "In " << __func__ << ": device " << link_name
                   << " is unexpected technology "
                   << Technology::NameFromIdentifier(technology);
    }
    string address =
        base::ToLowerASCII(infos_[dev_index].mac_address.HexEncode());

    if (technology != Technology::kTunnel &&
        technology != Technology::kUnknown) {
      DCHECK(!address.empty());
    }

    DeviceRefPtr device = CreateDevice(link_name, address, dev_index,
                                       technology);
    if (device) {
      RegisterDevice(device);
    }
  }
}

void DeviceInfo::RetrieveLinkStatistics(int interface_index,
                                        const RTNLMessage& msg) {
  if (!msg.HasAttribute(IFLA_STATS64)) {
    return;
  }
  ByteString stats_bytes(msg.GetAttribute(IFLA_STATS64));
  struct rtnl_link_stats64 stats;
  if (stats_bytes.GetLength() < sizeof(stats)) {
    LOG(WARNING) << "Link statistics size is too small: "
                 << stats_bytes.GetLength() << " < " << sizeof(stats);
    return;
  }

  memcpy(&stats, stats_bytes.GetConstData(), sizeof(stats));
  SLOG(this, 2) << "Link statistics for "
                << " interface index " << interface_index << ": "
                << "receive: " << stats.rx_bytes << "; "
                << "transmit: " << stats.tx_bytes << ".";
  infos_[interface_index].rx_bytes = stats.rx_bytes;
  infos_[interface_index].tx_bytes = stats.tx_bytes;
}

void DeviceInfo::RequestLinkStatistics() {
  rtnl_handler_->RequestDump(RTNLHandler::kRequestLink);
  dispatcher_->PostDelayedTask(request_link_statistics_callback_.callback(),
                               kRequestLinkStatisticsIntervalMilliseconds);
}

#if !defined(DISABLE_WIFI)
void DeviceInfo::GetWiFiInterfaceInfo(int interface_index) {
  GetInterfaceMessage msg;
  if (!msg.attributes()->SetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                              interface_index)) {
    LOG(ERROR) << "Unable to set interface index attribute for "
                  "GetInterface message.  Interface type cannot be "
                  "determined!";
    return;
  }
  netlink_manager_->SendNl80211Message(
      &msg,
      Bind(&DeviceInfo::OnWiFiInterfaceInfoReceived, AsWeakPtr()),
      Bind(&NetlinkManager::OnAckDoNothing),
      Bind(&NetlinkManager::OnNetlinkMessageError));
}

void DeviceInfo::OnWiFiInterfaceInfoReceived(const Nl80211Message& msg) {
  if (msg.command() != NL80211_CMD_NEW_INTERFACE) {
    LOG(ERROR) << "Message is not a new interface response";
    return;
  }

  uint32_t interface_index;
  if (!msg.const_attributes()->GetU32AttributeValue(NL80211_ATTR_IFINDEX,
                                                    &interface_index)) {
    LOG(ERROR) << "Message contains no interface index";
    return;
  }
  uint32_t interface_type;
  if (!msg.const_attributes()->GetU32AttributeValue(NL80211_ATTR_IFTYPE,
                                                    &interface_type)) {
    LOG(ERROR) << "Message contains no interface type";
    return;
  }
  const Info* info = GetInfo(interface_index);
  if (!info) {
    LOG(ERROR) << "Could not find device info for interface index "
               << interface_index;
    return;
  }
  if (info->device) {
    LOG(ERROR) << "Device already created for interface index "
               << interface_index;
    return;
  }
  if (interface_type != NL80211_IFTYPE_STATION) {
    LOG(INFO) << "Ignoring WiFi device "
              << info->name
              << " at interface index "
              << interface_index
              << " since it is not in station mode.";
    return;
  }
  LOG(INFO) << "Creating WiFi device for station mode interface "
              << info->name
              << " at interface index "
              << interface_index;
  string address = base::ToLowerASCII(info->mac_address.HexEncode());
  DeviceRefPtr device =
      new WiFi(control_interface_, dispatcher_, metrics_, manager_,
               info->name, address, interface_index);
  device->EnableIPv6Privacy();
  RegisterDevice(device);
}
#endif  // DISABLE_WIFI

bool DeviceInfo::SetHostname(const std::string& hostname) const {
  if (sethostname(hostname.c_str(), hostname.length())) {
    PLOG(ERROR) << "Failed to set hostname to: " << hostname;
    return false;
  }

  return true;
}

}  // namespace shill
