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

#ifndef SHILL_VPN_OPENVPN_DRIVER_H_
#define SHILL_VPN_OPENVPN_DRIVER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/files/file_path.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/ipconfig.h"
#include "shill/net/sockets.h"
#include "shill/refptr_types.h"
#include "shill/rpc_task.h"
#include "shill/service.h"
#include "shill/vpn/vpn_driver.h"

namespace base {

template<typename T>
class WeakPtr;

}  // namespace base

namespace shill {

class CertificateFile;
class ControlInterface;
class DeviceInfo;
class Error;
class Metrics;
class OpenVPNManagementServer;
class ProcessManager;

class OpenVPNDriver : public VPNDriver,
                      public RPCTaskDelegate {
 public:
  enum ReconnectReason {
    kReconnectReasonUnknown,
    kReconnectReasonOffline,
    kReconnectReasonTLSError,
  };

  OpenVPNDriver(ControlInterface* control,
                EventDispatcher* dispatcher,
                Metrics* metrics,
                Manager* manager,
                DeviceInfo* device_info,
                ProcessManager* process_manager);
  ~OpenVPNDriver() override;

  virtual void OnReconnecting(ReconnectReason reason);

  // Resets the VPN state and deallocates all resources. If there's a service
  // associated through Connect, sets its state to Service::kStateIdle and
  // disassociates from the service.
  virtual void IdleService();

  // Resets the VPN state and deallocates all resources. If there's a service
  // associated through Connect, sets its state to Service::kStateFailure, sets
  // the failure reason to |failure|, sets its ErrorDetails property to
  // |error_details|, and disassociates from the service.
  virtual void FailService(Service::ConnectFailure failure,
                           const std::string& error_details);

  // Append zero-valued, single-valued and double-valued options to the
  // |options| array.
  static void AppendOption(
      const std::string& option,
      std::vector<std::vector<std::string>>* options);
  static void AppendOption(
      const std::string& option,
      const std::string& value,
      std::vector<std::vector<std::string>>* options);
  static void AppendOption(
      const std::string& option,
      const std::string& value0,
      const std::string& value1,
      std::vector<std::vector<std::string>>* options);

  // Returns true if an option was appended.
  bool AppendValueOption(const std::string& property,
                         const std::string& option,
                         std::vector<std::vector<std::string>>* options);

  // If |property| exists, split its value up using |delimiter|.  Each element
  // will be a separate argument to |option|. Returns true if the option was
  // appended to |options|.
  bool AppendDelimitedValueOption(
      const std::string& property,
      const std::string& option,
      char delimiter,
      std::vector<std::vector<std::string>>* options);

  // Returns true if a flag was appended.
  bool AppendFlag(const std::string& property,
                  const std::string& option,
                  std::vector<std::vector<std::string>>* options);

  virtual std::string GetServiceRpcIdentifier() const;

 protected:
  // Inherited from VPNDriver. |Connect| initiates the VPN connection by
  // creating a tunnel device. When the device index becomes available, this
  // instance is notified through |ClaimInterface| and resumes the connection
  // process by setting up and spawning an external 'openvpn' process. IP
  // configuration settings are passed back from the external process through
  // the |Notify| RPC service method.
  void Connect(const VPNServiceRefPtr& service, Error* error) override;
  bool ClaimInterface(const std::string& link_name,
                      int interface_index) override;
  void Disconnect() override;
  std::string GetProviderType() const override;
  void OnConnectionDisconnected() override;
  void OnConnectTimeout() override;

 private:
  friend class OpenVPNDriverTest;
  FRIEND_TEST(OpenVPNDriverTest, ClaimInterface);
  FRIEND_TEST(OpenVPNDriverTest, Cleanup);
  FRIEND_TEST(OpenVPNDriverTest, Connect);
  FRIEND_TEST(OpenVPNDriverTest, ConnectTunnelFailure);
  FRIEND_TEST(OpenVPNDriverTest, Disconnect);
  FRIEND_TEST(OpenVPNDriverTest, GetEnvironment);
  FRIEND_TEST(OpenVPNDriverTest, GetRouteOptionEntry);
  FRIEND_TEST(OpenVPNDriverTest, InitCAOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitCertificateVerifyOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitClientAuthOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitExtraCertOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitLoggingOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitOptions);
  FRIEND_TEST(OpenVPNDriverTest, InitOptionsHostWithPort);
  FRIEND_TEST(OpenVPNDriverTest, InitOptionsNoHost);
  FRIEND_TEST(OpenVPNDriverTest, InitPKCS11Options);
  FRIEND_TEST(OpenVPNDriverTest, Notify);
  FRIEND_TEST(OpenVPNDriverTest, NotifyUMA);
  FRIEND_TEST(OpenVPNDriverTest, NotifyFail);
  FRIEND_TEST(OpenVPNDriverTest, OnDefaultServiceChanged);
  FRIEND_TEST(OpenVPNDriverTest, OnOpenVPNDied);
  FRIEND_TEST(OpenVPNDriverTest, OnOpenVPNExited);
  FRIEND_TEST(OpenVPNDriverTest, ParseForeignOption);
  FRIEND_TEST(OpenVPNDriverTest, ParseForeignOptions);
  FRIEND_TEST(OpenVPNDriverTest, ParseIPConfiguration);
  FRIEND_TEST(OpenVPNDriverTest, ParseRouteOption);
  FRIEND_TEST(OpenVPNDriverTest, SetRoutes);
  FRIEND_TEST(OpenVPNDriverTest, SpawnOpenVPN);
  FRIEND_TEST(OpenVPNDriverTest, SplitPortFromHost);
  FRIEND_TEST(OpenVPNDriverTest, WriteConfigFile);

  // The map is a sorted container that allows us to iterate through the options
  // in order.
  typedef std::map<int, std::string> ForeignOptions;
  typedef std::map<int, IPConfig::Route> RouteOptions;

  static const char kDefaultCACertificates[];

  static const char kOpenVPNPath[];
  static const char kOpenVPNScript[];
  static const Property kProperties[];

  static const char kLSBReleaseFile[];

  static const char kDefaultOpenVPNConfigurationDirectory[];

  static const int kReconnectOfflineTimeoutSeconds;
  static const int kReconnectTLSErrorTimeoutSeconds;

  static void ParseForeignOptions(const ForeignOptions& options,
                                  IPConfig::Properties* properties);
  static void ParseForeignOption(const std::string& option,
                                 std::vector<std::string>* domain_search,
                                 std::vector<std::string>* dns_servers);
  static IPConfig::Route* GetRouteOptionEntry(const std::string& prefix,
                                              const std::string& key,
                                              RouteOptions* routes);
  static void ParseRouteOption(const std::string& key,
                               const std::string& value,
                               RouteOptions* routes);
  static void SetRoutes(const RouteOptions& routes,
                        IPConfig::Properties* properties);

  // If |host| is in the "name:port" format, sets up |name| and |port|
  // appropriately and returns true. Otherwise, returns false.
  static bool SplitPortFromHost(const std::string& host,
                                std::string* name,
                                std::string* port);

  void InitOptions(
      std::vector<std::vector<std::string>>* options, Error* error);
  bool InitCAOptions(
      std::vector<std::vector<std::string>>* options, Error* error);
  void InitCertificateVerifyOptions(
      std::vector<std::vector<std::string>>* options);
  void InitClientAuthOptions(std::vector<std::vector<std::string>>* options);
  bool InitExtraCertOptions(
      std::vector<std::vector<std::string>>* options, Error* error);
  void InitPKCS11Options(std::vector<std::vector<std::string>>* options);
  bool InitManagementChannelOptions(
      std::vector<std::vector<std::string>>* options, Error* error);
  void InitLoggingOptions(std::vector<std::vector<std::string>>* options);

  std::map<std::string, std::string> GetEnvironment();
  void ParseIPConfiguration(
      const std::map<std::string, std::string>& configuration,
      IPConfig::Properties* properties) const;

  bool SpawnOpenVPN();

  // Implements the public IdleService and FailService methods. Resets the VPN
  // state and deallocates all resources. If there's a service associated
  // through Connect, sets its state |state|; if |state| is
  // Service::kStateFailure, sets the failure reason to |failure| and its
  // ErrorDetails property to |error_details|; disassociates from the service.
  void Cleanup(Service::ConnectState state,
               Service::ConnectFailure failure,
               const std::string& error_details);

  static int GetReconnectTimeoutSeconds(ReconnectReason reason);

  // Join a list of options into a single string.
  static std::string JoinOptions(
      const std::vector<std::vector<std::string>>& options, char separator);

  // Output an OpenVPN configuration.
  bool WriteConfigFile(const std::vector<std::vector<std::string>>& options,
                       base::FilePath* config_file);

  // Called when the openpvn process exits.
  void OnOpenVPNDied(int exit_status);

  // Standalone callback used to delete the tunnel interface when the
  // openvpn process exits as we clean up. ("Exiting" is expected
  // termination during cleanup, while "dying" is any unexpected
  // termination.)
  static void OnOpenVPNExited(const base::WeakPtr<DeviceInfo>& device_info,
                              int interface_index,
                              int exit_status);

  // Inherit from VPNDriver to add custom properties.
  KeyValueStore GetProvider(Error* error) override;

  // Implements RPCTaskDelegate.
  void GetLogin(std::string* user, std::string* password) override;
  void Notify(const std::string& reason,
              const std::map<std::string, std::string>& dict) override;

  void OnDefaultServiceChanged(const ServiceRefPtr& service);

  void ReportConnectionMetrics();

  ControlInterface* control_;
  Metrics* metrics_;
  DeviceInfo* device_info_;
  ProcessManager* process_manager_;
  Sockets sockets_;
  std::unique_ptr<OpenVPNManagementServer> management_server_;
  std::unique_ptr<CertificateFile> certificate_file_;
  std::unique_ptr<CertificateFile> extra_certificates_file_;
  base::FilePath lsb_release_file_;

  VPNServiceRefPtr service_;
  std::unique_ptr<RPCTask> rpc_task_;
  std::string tunnel_interface_;
  VirtualDeviceRefPtr device_;
  base::FilePath tls_auth_file_;
  base::FilePath openvpn_config_directory_;
  base::FilePath openvpn_config_file_;
  IPConfig::Properties ip_properties_;

  // The PID of the spawned openvpn process. May be 0 if no process has been
  // spawned yet or the process has died.
  int pid_;

  // Default service watch callback tag.
  int default_service_callback_tag_;

  DISALLOW_COPY_AND_ASSIGN(OpenVPNDriver);
};

}  // namespace shill

#endif  // SHILL_VPN_OPENVPN_DRIVER_H_
