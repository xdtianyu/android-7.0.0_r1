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

#ifndef SHILL_DEVICE_INFO_H_
#define SHILL_DEVICE_INFO_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/memory/ref_counted.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/device.h"
#include "shill/net/byte_string.h"
#include "shill/net/ip_address.h"
#include "shill/net/rtnl_listener.h"
#include "shill/net/shill_time.h"
#include "shill/technology.h"

namespace shill {

class Manager;
class Metrics;
class RoutingTable;
class RTNLHandler;
class RTNLMessage;
class Sockets;

#if !defined(DISABLE_WIFI)
class NetlinkManager;
class Nl80211Message;
#endif  // DISABLE_WIFI

class DeviceInfo : public base::SupportsWeakPtr<DeviceInfo> {
 public:
  struct AddressData {
    AddressData()
        : address(IPAddress::kFamilyUnknown), flags(0), scope(0) {}
    AddressData(const IPAddress& address_in,
                unsigned char flags_in,
                unsigned char scope_in)
        : address(address_in), flags(flags_in), scope(scope_in) {}
    IPAddress address;
    unsigned char flags;
    unsigned char scope;
  };

  // Device name prefix for modem pseudo devices used in testing.
  static const char kModemPseudoDeviceNamePrefix[];
  // Device name prefix for virtual ethernet devices used in testing.
  static const char kEthernetPseudoDeviceNamePrefix[];
  // Device name prefix for virtual ethernet devices that should be ignored.
  static const char kIgnoredDeviceNamePrefix[];
  // Time interval for polling for link statistics.
  static const int kRequestLinkStatisticsIntervalMilliseconds;

  DeviceInfo(ControlInterface* control_interface,
             EventDispatcher* dispatcher,
             Metrics* metrics,
             Manager* manager);
  virtual ~DeviceInfo();

  virtual void AddDeviceToBlackList(const std::string& device_name);
  virtual void RemoveDeviceFromBlackList(const std::string& device_name);
  virtual bool IsDeviceBlackListed(const std::string& device_name);
  void Start();
  void Stop();

  std::vector<std::string> GetUninitializedTechnologies() const;

  // Adds |device| to this DeviceInfo instance so that we can handle its link
  // messages, and registers it with the manager.
  virtual void RegisterDevice(const DeviceRefPtr& device);

  // Remove |device| from this DeviceInfo.  This function should only
  // be called for cellular devices because the lifetime of the
  // cellular devices is controlled by the Modem object and its
  // communication to modem manager, rather than by RTNL messages.
  virtual void DeregisterDevice(const DeviceRefPtr& device);

  virtual DeviceRefPtr GetDevice(int interface_index) const;
  virtual bool GetMACAddress(int interface_index, ByteString* address) const;

  // Queries the kernel for a MAC address for |interface_index|.  Returns an
  // empty ByteString on failure.
  virtual ByteString GetMACAddressFromKernel(int interface_index) const;

  // Queries the kernel for the MAC address of |peer| on |interface_index|.
  // Returns true and populates |mac_address| on success, otherwise returns
  // false.
  virtual bool GetMACAddressOfPeer(int interface_index,
                                   const IPAddress& peer,
                                   ByteString* mac_address) const;

  virtual bool GetFlags(int interface_index, unsigned int* flags) const;
  virtual bool GetByteCounts(int interface_index,
                             uint64_t* rx_bytes, uint64_t* tx_bytes) const;
  virtual bool GetAddresses(int interface_index,
                            std::vector<AddressData>* addresses) const;

  // Flush all addresses associated with |interface_index|.
  virtual void FlushAddresses(int interface_index) const;
  // Returns whether this interface does not have |this_address|
  // but has another non-temporary address of the same family.
  virtual bool HasOtherAddress(
      int interface_index, const IPAddress& this_address) const;

  // Get the preferred globally scoped IPv6 address for |interface_index|.
  // This method returns true and sets |address| if a primary IPv6 address
  // exists.  Otherwise it returns false and leaves |address| unmodified.
  virtual bool GetPrimaryIPv6Address(int interface_index, IPAddress* address);

  // Get the IPv6 DNS server addresses for |interface_index|. This method
  // returns true and sets |address_list| and |life_time_seconds| if the IPv6
  // DNS server addresses exists. Otherwise, it returns false and leave
  // |address_list| and |life_time_seconds| unmodified. |life_time_seconds|
  // indicates the number of the seconds the DNS server is still valid for at
  // the time of this function call. Value of 0 means the DNS server is not
  // valid anymore, and value of 0xFFFFFFFF means the DNS server is valid
  // forever.
  virtual bool GetIPv6DnsServerAddresses(int interface_index,
                                         std::vector<IPAddress>* address_list,
                                         uint32_t* life_time_seconds);

  // Returns true if any of the addresses on |interface_index| are on the
  // same network prefix as |address|.
  virtual bool HasDirectConnectivityTo(
      int interface_index, const IPAddress& address) const;

  virtual bool CreateTunnelInterface(std::string* interface_name) const;
  virtual int OpenTunnelInterface(const std::string& interface_name) const;
  virtual bool DeleteInterface(int interface_index) const;

  // Returns the interface index for |interface_name| or -1 if unknown.
  virtual int GetIndex(const std::string& interface_name) const;

  // Sets the system hostname to |hostname|.
  virtual bool SetHostname(const std::string& hostname) const;

 private:
  friend class DeviceInfoDelayedCreationTest;
  friend class DeviceInfoTechnologyTest;
  friend class DeviceInfoTest;
  FRIEND_TEST(CellularTest, StartLinked);
  FRIEND_TEST(DeviceInfoTest, CreateDeviceWiMax);
  FRIEND_TEST(DeviceInfoTest, DeviceRemovedEvent);
  FRIEND_TEST(DeviceInfoTest, GetUninitializedTechnologies);
  FRIEND_TEST(DeviceInfoTest, HasSubdir);  // For HasSubdir.
  FRIEND_TEST(DeviceInfoTest, IPv6AddressChanged);  // For infos_.
  FRIEND_TEST(DeviceInfoTest, RequestLinkStatistics);
  FRIEND_TEST(DeviceInfoTest, StartStop);
  FRIEND_TEST(DeviceInfoTest, IPv6DnsServerAddressesChanged);  // For infos_.

  struct Info {
    Info()
        : flags(0),
          rx_bytes(0),
          tx_bytes(0),
          has_addresses_only(false),
          technology(Technology::kUnknown)
    {}

    DeviceRefPtr device;
    std::string name;
    ByteString mac_address;
    std::vector<AddressData> ip_addresses;
    std::vector<IPAddress> ipv6_dns_server_addresses;
    uint32_t ipv6_dns_server_lifetime_seconds;
    time_t ipv6_dns_server_received_time_seconds;
    unsigned int flags;
    uint64_t rx_bytes;
    uint64_t tx_bytes;

    // This flag indicates that link information has not been retrieved yet;
    // only the ip_addresses field is valid.
    bool has_addresses_only;

    Technology::Identifier technology;
  };

  // Root of the kernel sysfs directory holding network device info.
  static const char kDeviceInfoRoot[];
  // Name of the "cdc_ether" driver.  This driver is not included in the
  // kModemDrivers list because we need to do additional checking.
  static const char kDriverCdcEther[];
  // Name of the "cdc_ncm" driver.  This driver is not included in the
  // kModemDrivers list because we need to do additional checking.
  static const char kDriverCdcNcm[];
  // Name of the GDM WiMAX driver.
  static const char kDriverGdmWiMax[];
  // Name of the virtio network driver.
  static const char kDriverVirtioNet[];
  // Sysfs path to a device uevent file.
  static const char kInterfaceUevent[];
  // Content of a device uevent file that indicates it is a wifi device.
  static const char kInterfaceUeventWifiSignature[];
  // Sysfs path to a device via its interface name.
  static const char kInterfaceDevice[];
  // Sysfs path to the driver of a device via its interface name.
  static const char kInterfaceDriver[];
  // Sysfs path to the file that is used to determine if this is tun device.
  static const char kInterfaceTunFlags[];
  // Sysfs path to the file that is used to determine if a wifi device is
  // operating in monitor mode.
  static const char kInterfaceType[];
  // Modem drivers that we support.
  static const char* kModemDrivers[];
  // Path to the tun device.
  static const char kTunDeviceName[];
  // Time to wait before registering devices which need extra time to detect.
  static const int kDelayedDeviceCreationSeconds;

  // Create a Device object for the interface named |linkname|, with a
  // string-form MAC address |address|, whose kernel interface index
  // is |interface_index| and detected technology is |technology|.
  virtual DeviceRefPtr CreateDevice(const std::string& link_name,
                                    const std::string& address,
                                    int interface_index,
                                    Technology::Identifier technology);

  // Return the FilePath for a given |path_name| in the device sysinfo for
  // a specific interface |iface_name|.
  base::FilePath GetDeviceInfoPath(const std::string& iface_name,
                             const std::string& path_name);
  // Return the contents of the device info file |path_name| for interface
  // |iface_name| in output parameter |contents_out|.  Returns true if file
  // read succeeded, false otherwise.
  bool GetDeviceInfoContents(const std::string& iface_name,
                             const std::string& path_name,
                             std::string* contents_out);

  // Return the filepath for the target of the device info symbolic link
  // |path_name| for interface |iface_name| in output parameter |path_out|.
  // Returns true if symbolic link read succeeded, false otherwise.
  bool GetDeviceInfoSymbolicLink(const std::string& iface_name,
                                 const std::string& path_name,
                                 base::FilePath* path_out);
  // Classify the device named |iface_name|, and return an identifier
  // indicating its type.
  virtual Technology::Identifier GetDeviceTechnology(
      const std::string& iface_name);
  // Checks the device specified by |iface_name| to see if it's a modem device.
  // This method assumes that |iface_name| has already been determined to be
  // using the cdc_ether / cdc_ncm driver.
  bool IsCdcEthernetModemDevice(const std::string& iface_name);
  // Returns true if |base_dir| has a subdirectory named |subdir|.
  // |subdir| can be an immediate subdirectory of |base_dir| or can be
  // several levels deep.
  static bool HasSubdir(const base::FilePath& base_dir,
                        const base::FilePath& subdir);

  // Returns true and sets |link_name| to the interface name contained
  // in |msg| if one is provided.  Returns false otherwise.
  bool GetLinkNameFromMessage(const RTNLMessage& msg, std::string* link_name);

  // Returns true if |msg| pertains to a blacklisted device whose link name
  // is now different from the name it was assigned before.
  bool IsRenamedBlacklistedDevice(const RTNLMessage& msg);

  void AddLinkMsgHandler(const RTNLMessage& msg);
  void DelLinkMsgHandler(const RTNLMessage& msg);
  void LinkMsgHandler(const RTNLMessage& msg);
  void AddressMsgHandler(const RTNLMessage& msg);
  void RdnssMsgHandler(const RTNLMessage& msg);

  const Info* GetInfo(int interface_index) const;
  void RemoveInfo(int interface_index);
  void DelayDeviceCreation(int interface_index);
  void DelayedDeviceCreationTask();
  void RetrieveLinkStatistics(int interface_index, const RTNLMessage& msg);
  void RequestLinkStatistics();

#if !defined(DISABLE_WIFI)
  // Use nl80211 to get information on |interface_index|.
  void GetWiFiInterfaceInfo(int interface_index);
  void OnWiFiInterfaceInfoReceived(const Nl80211Message& message);
#endif  // DISABLE_WIFI

  void set_sockets(Sockets* sockets) { sockets_.reset(sockets); }

  ControlInterface* control_interface_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  Manager* manager_;

  std::map<int, Info> infos_;  // Maps interface index to Info.
  std::map<std::string, int> indices_;  // Maps interface name to index.

  base::Callback<void(const RTNLMessage&)> link_callback_;
  base::Callback<void(const RTNLMessage&)> address_callback_;
  base::Callback<void(const RTNLMessage&)> rdnss_callback_;
  std::unique_ptr<RTNLListener> link_listener_;
  std::unique_ptr<RTNLListener> address_listener_;
  std::unique_ptr<RTNLListener> rdnss_listener_;
  std::set<std::string> black_list_;
  base::FilePath device_info_root_;

  // Keep track of devices that require a delayed call to CreateDevice().
  base::CancelableClosure delayed_devices_callback_;
  std::set<int> delayed_devices_;

  // Maintain a callback for the periodic link statistics poll task.
  base::CancelableClosure request_link_statistics_callback_;

  // Cache copy of singleton pointers.
  RoutingTable* routing_table_;
  RTNLHandler* rtnl_handler_;
#if !defined(DISABLE_WIFI)
  NetlinkManager* netlink_manager_;
#endif  // DISABLE_WIFI

  // A member of the class so that a mock can be injected for testing.
  std::unique_ptr<Sockets> sockets_;

  Time* time_;

  DISALLOW_COPY_AND_ASSIGN(DeviceInfo);
};

}  // namespace shill

#endif  // SHILL_DEVICE_INFO_H_
