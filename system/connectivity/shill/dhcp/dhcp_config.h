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

#ifndef SHILL_DHCP_DHCP_CONFIG_H_
#define SHILL_DHCP_DHCP_CONFIG_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/cancelable_callback.h>
#include <base/files/file_path.h>
#include <base/memory/weak_ptr.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/ipconfig.h"
#include "shill/key_value_store.h"

namespace shill {

class ControlInterface;
class DHCPProvider;
class DHCPProxyInterface;
class EventDispatcher;
class Metrics;
class ProcessManager;

// This class provides a DHCP client instance for the device |device_name|.
//
// The DHPCConfig instance asks the DHCP client to create a lease file
// containing the name |lease_file_suffix|.  If this suffix is the same as
// |device_name|, the lease is considered to be ephemeral, and the lease
// file is removed whenever this DHCPConfig instance is no longer needed.
// Otherwise, the lease file persists and will be re-used in future attempts.
class DHCPConfig : public IPConfig {
 public:
  DHCPConfig(ControlInterface* control_interface,
             EventDispatcher* dispatcher,
             DHCPProvider* provider,
             const std::string& device_name,
             const std::string& type,
             const std::string& lease_file_suffix);
  ~DHCPConfig() override;

  // Inherited from IPConfig.
  bool RequestIP() override;
  bool RenewIP() override;
  bool ReleaseIP(ReleaseReason reason) override;

  // If |proxy_| is not initialized already, sets it to a new D-Bus proxy to
  // |service|.
  void InitProxy(const std::string& service);

  // Processes an Event signal from dhcpcd.
  virtual void ProcessEventSignal(const std::string& reason,
                                  const KeyValueStore& configuration) = 0;

  // Processes an Status Change signal from dhcpcd.
  virtual void ProcessStatusChangeSignal(const std::string& status) = 0;

  // Set the minimum MTU that this configuration will respect.
  virtual void set_minimum_mtu(const int minimum_mtu) {
    minimum_mtu_ = minimum_mtu;
  }

 protected:
  // Overrides base clase implementation.
  void UpdateProperties(const Properties& properties,
                        bool new_lease_acquired) override;
  void NotifyFailure() override;

  int minimum_mtu() const { return minimum_mtu_; }

  void set_is_lease_active(bool active) { is_lease_active_ = active; }

  // Return true if the lease file is ephermeral, which means the lease file
  // should be deleted during cleanup.
  bool IsEphemeralLease() const;

  // Cleans up remaining state from a running client, if any, including freeing
  // its GPid, exit watch callback, and state files.
  // The file path for the lease file and pid file is different for IPv4
  // and IPv6. So make this function virtual to have the derived class delete
  // the files accordingly.
  virtual void CleanupClientState();

  // Return true if we should treat acquisition timeout as failure.
  virtual bool ShouldFailOnAcquisitionTimeout() { return true; }

  // Return true if we should keep the lease on disconnect.
  virtual bool ShouldKeepLeaseOnDisconnect() { return false; }

  // Return the list of flags used to start dhcpcd.
  virtual std::vector<std::string> GetFlags();

  base::FilePath root() const { return root_; }

 private:
  friend class DHCPConfigTest;
  friend class DHCPv4ConfigTest;
  friend class DHCPv6ConfigTest;
  FRIEND_TEST(DHCPConfigCallbackTest, NotifyFailure);
  FRIEND_TEST(DHCPConfigCallbackTest, ProcessAcquisitionTimeout);
  FRIEND_TEST(DHCPConfigCallbackTest, RequestIPTimeout);
  FRIEND_TEST(DHCPConfigCallbackTest, StartTimeout);
  FRIEND_TEST(DHCPConfigCallbackTest, StoppedDuringFailureCallback);
  FRIEND_TEST(DHCPConfigCallbackTest, StoppedDuringSuccessCallback);
  FRIEND_TEST(DHCPConfigTest, InitProxy);
  FRIEND_TEST(DHCPConfigTest, KeepLeaseOnDisconnect);
  FRIEND_TEST(DHCPConfigTest, ReleaseIP);
  FRIEND_TEST(DHCPConfigTest, ReleaseIPStaticIPWithLease);
  FRIEND_TEST(DHCPConfigTest, ReleaseIPStaticIPWithoutLease);
  FRIEND_TEST(DHCPConfigTest, ReleaseLeaseOnDisconnect);
  FRIEND_TEST(DHCPConfigTest, RenewIP);
  FRIEND_TEST(DHCPConfigTest, RequestIP);
  FRIEND_TEST(DHCPConfigTest, Restart);
  FRIEND_TEST(DHCPConfigTest, RestartNoClient);
  FRIEND_TEST(DHCPConfigTest, StartFail);
  FRIEND_TEST(DHCPConfigTest, StartWithoutLeaseSuffix);
  FRIEND_TEST(DHCPConfigTest, Stop);
  FRIEND_TEST(DHCPConfigTest, StopDuringRequestIP);
  FRIEND_TEST(DHCPProviderTest, CreateIPv4Config);

  static const int kAcquisitionTimeoutSeconds;

  static const int kDHCPCDExitPollMilliseconds;
  static const int kDHCPCDExitWaitMilliseconds;
  static const char kDHCPCDPath[];
  static const char kDHCPCDUser[];
  static const char kDHCPCDGroup[];

  // Starts dhcpcd, returns true on success and false otherwise.
  bool Start();

  // Stops dhcpcd if running.
  void Stop(const char* reason);

  // Stops dhcpcd if already running and then starts it. Returns true on success
  // and false otherwise.
  bool Restart();

  // Called when the dhcpcd client process exits.
  void OnProcessExited(int exit_status);

  // Initialize a callback that will invoke ProcessAcquisitionTimeout if we
  // do not get a lease in a reasonable amount of time.
  void StartAcquisitionTimeout();
  // Cancel callback created by StartAcquisitionTimeout. One-liner included
  // for symmetry.
  void StopAcquisitionTimeout();
  // Called if we do not get a DHCP lease in a reasonable amount of time.
  // Informs upper layers of the failure.
  void ProcessAcquisitionTimeout();

  // Initialize a callback that will invoke ProcessExpirationTimeout if we
  // do not renew a lease in a |lease_duration_seconds|.
  void StartExpirationTimeout(uint32_t lease_duration_seconds);
  // Cancel callback created by StartExpirationTimeout. One-liner included
  // for symmetry.
  void StopExpirationTimeout();
  // Called if we do not renew a DHCP lease by the time the lease expires.
  // Informs upper layers of the expiration and restarts the DHCP client.
  void ProcessExpirationTimeout();

  // Kills DHCP client process.
  void KillClient();

  ControlInterface* control_interface_;

  DHCPProvider* provider_;

  // DHCP lease file suffix, used to differentiate the lease of one interface
  // or network from another.
  std::string lease_file_suffix_;

  // The PID of the spawned DHCP client. May be 0 if no client has been spawned
  // yet or the client has died.
  int pid_;

  // Whether a lease has been acquired from the DHCP server or gateway ARP.
  bool is_lease_active_;

  // The proxy for communicating with the DHCP client.
  std::unique_ptr<DHCPProxyInterface> proxy_;

  // Called if we fail to get a DHCP lease in a timely manner.
  base::CancelableClosure lease_acquisition_timeout_callback_;

  // Time to wait for a DHCP lease. Represented as field so that it
  // can be overriden in tests.
  unsigned int lease_acquisition_timeout_seconds_;

  // Called if a DHCP lease expires.
  base::CancelableClosure lease_expiration_callback_;

  // The minimum MTU value this configuration will respect.
  int minimum_mtu_;

  // Root file path, used for testing.
  base::FilePath root_;

  base::WeakPtrFactory<DHCPConfig> weak_ptr_factory_;
  EventDispatcher* dispatcher_;
  ProcessManager* process_manager_;
  Metrics* metrics_;

  DISALLOW_COPY_AND_ASSIGN(DHCPConfig);
};

}  // namespace shill

#endif  // SHILL_DHCP_DHCP_CONFIG_H_
