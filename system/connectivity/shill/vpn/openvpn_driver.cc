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

#include "shill/vpn/openvpn_driver.h"

#include <arpa/inet.h>

#include <base/files/file_util.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_split.h>
#include <base/strings/string_util.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__

#include "shill/certificate_file.h"
#include "shill/connection.h"
#include "shill/device_info.h"
#include "shill/error.h"
#include "shill/ipconfig.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/net/sockets.h"
#include "shill/process_manager.h"
#include "shill/rpc_task.h"
#include "shill/virtual_device.h"
#include "shill/vpn/openvpn_management_server.h"
#include "shill/vpn/vpn_service.h"

using base::Closure;
using base::FilePath;
using base::SplitString;
using base::Unretained;
using base::WeakPtr;
using std::map;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(const OpenVPNDriver* o) {
  return o->GetServiceRpcIdentifier();
}
}

namespace {

const char kChromeOSReleaseName[] = "CHROMEOS_RELEASE_NAME";
const char kChromeOSReleaseVersion[] = "CHROMEOS_RELEASE_VERSION";
const char kOpenVPNEnvVarPlatformName[] = "IV_PLAT";
const char kOpenVPNEnvVarPlatformVersion[] = "IV_PLAT_REL";
const char kOpenVPNForeignOptionPrefix[] = "foreign_option_";
const char kOpenVPNIfconfigBroadcast[] = "ifconfig_broadcast";
const char kOpenVPNIfconfigLocal[] = "ifconfig_local";
const char kOpenVPNIfconfigNetmask[] = "ifconfig_netmask";
const char kOpenVPNIfconfigRemote[] = "ifconfig_remote";
const char kOpenVPNRedirectGateway[] = "redirect_gateway";
const char kOpenVPNRedirectPrivate[] = "redirect_private";
const char kOpenVPNRouteOptionPrefix[] = "route_";
const char kOpenVPNRouteVPNGateway[] = "route_vpn_gateway";
const char kOpenVPNTrustedIP[] = "trusted_ip";
const char kOpenVPNTunMTU[] = "tun_mtu";

const char kDefaultPKCS11Provider[] = "libchaps.so";

// Some configurations pass the netmask in the ifconfig_remote property.
// This is due to some servers not explicitly indicating that they are using
// a "broadcast mode" network instead of peer-to-peer.  See
// http://crbug.com/241264 for an example of this issue.
const char kSuspectedNetmaskPrefix[] = "255.";

void DoNothingWithExitStatus(int exit_status) {
}

}  // namespace

// static
const char OpenVPNDriver::kDefaultCACertificates[] =
    "/etc/ssl/certs/ca-certificates.crt";
// static
const char OpenVPNDriver::kOpenVPNPath[] = "/usr/sbin/openvpn";
// static
const char OpenVPNDriver::kOpenVPNScript[] = SHIMDIR "/openvpn-script";
// static
const VPNDriver::Property OpenVPNDriver::kProperties[] = {
  { kOpenVPNAuthNoCacheProperty, 0 },
  { kOpenVPNAuthProperty, 0 },
  { kOpenVPNAuthRetryProperty, 0 },
  { kOpenVPNAuthUserPassProperty, 0 },
  { kOpenVPNCaCertNSSProperty, 0 },
  { kOpenVPNCaCertProperty, 0 },
  { kOpenVPNCipherProperty, 0 },
  { kOpenVPNClientCertIdProperty, Property::kCredential },
  { kOpenVPNCompLZOProperty, 0 },
  { kOpenVPNCompNoAdaptProperty, 0 },
  { kOpenVPNIgnoreDefaultRouteProperty, 0 },
  { kOpenVPNKeyDirectionProperty, 0 },
  { kOpenVPNNsCertTypeProperty, 0 },
  { kOpenVPNOTPProperty,
    Property::kEphemeral | Property::kCredential | Property::kWriteOnly },
  { kOpenVPNPasswordProperty, Property::kCredential | Property::kWriteOnly },
  { kOpenVPNPinProperty, Property::kCredential },
  { kOpenVPNPortProperty, 0 },
  { kOpenVPNProtoProperty, 0 },
  { kOpenVPNProviderProperty, 0 },
  { kOpenVPNPushPeerInfoProperty, 0 },
  { kOpenVPNRemoteCertEKUProperty, 0 },
  { kOpenVPNRemoteCertKUProperty, 0 },
  { kOpenVPNRemoteCertTLSProperty, 0 },
  { kOpenVPNRenegSecProperty, 0 },
  { kOpenVPNServerPollTimeoutProperty, 0 },
  { kOpenVPNShaperProperty, 0 },
  { kOpenVPNStaticChallengeProperty, 0 },
  { kOpenVPNTLSAuthContentsProperty, 0 },
  { kOpenVPNTLSRemoteProperty, 0 },
  { kOpenVPNTokenProperty,
    Property::kEphemeral | Property::kCredential | Property::kWriteOnly },
  { kOpenVPNUserProperty, 0 },
  { kProviderHostProperty, 0 },
  { kProviderTypeProperty, 0 },
  { kOpenVPNCaCertPemProperty, Property::kArray },
  { kOpenVPNCertProperty, 0 },
  { kOpenVPNExtraCertPemProperty, Property::kArray },
  { kOpenVPNKeyProperty, 0 },
  { kOpenVPNPingExitProperty, 0 },
  { kOpenVPNPingProperty, 0 },
  { kOpenVPNPingRestartProperty, 0 },
  { kOpenVPNTLSAuthProperty, 0 },
  { kOpenVPNVerbProperty, 0 },
  { kOpenVPNVerifyHashProperty, 0 },
  { kOpenVPNVerifyX509NameProperty, 0 },
  { kOpenVPNVerifyX509TypeProperty, 0 },
  { kVPNMTUProperty, 0 },
};

const char OpenVPNDriver::kLSBReleaseFile[] = "/etc/lsb-release";

// Directory where OpenVPN configuration files are exported while the
// process is running.
const char OpenVPNDriver::kDefaultOpenVPNConfigurationDirectory[] =
    RUNDIR "/openvpn_config";

const int OpenVPNDriver::kReconnectOfflineTimeoutSeconds = 2 * 60;
const int OpenVPNDriver::kReconnectTLSErrorTimeoutSeconds = 20;

OpenVPNDriver::OpenVPNDriver(ControlInterface* control,
                             EventDispatcher* dispatcher,
                             Metrics* metrics,
                             Manager* manager,
                             DeviceInfo* device_info,
                             ProcessManager* process_manager)
    : VPNDriver(dispatcher, manager, kProperties, arraysize(kProperties)),
      control_(control),
      metrics_(metrics),
      device_info_(device_info),
      process_manager_(process_manager),
      management_server_(new OpenVPNManagementServer(this)),
      certificate_file_(new CertificateFile()),
      extra_certificates_file_(new CertificateFile()),
      lsb_release_file_(kLSBReleaseFile),
      openvpn_config_directory_(kDefaultOpenVPNConfigurationDirectory),
      pid_(0),
      default_service_callback_tag_(0) {}

OpenVPNDriver::~OpenVPNDriver() {
  IdleService();
}

void OpenVPNDriver::IdleService() {
  Cleanup(Service::kStateIdle,
          Service::kFailureUnknown,
          Service::kErrorDetailsNone);
}

void OpenVPNDriver::FailService(Service::ConnectFailure failure,
                                const string& error_details) {
  Cleanup(Service::kStateFailure, failure, error_details);
}

void OpenVPNDriver::Cleanup(Service::ConnectState state,
                            Service::ConnectFailure failure,
                            const string& error_details) {
  SLOG(this, 2) << __func__ << "(" << Service::ConnectStateToString(state)
                << ", " << error_details << ")";
  StopConnectTimeout();
  // Disconnecting the management interface will terminate the openvpn
  // process. Ensure this is handled robustly by first unregistering
  // the callback for OnOpenVPNDied, and then terminating and reaping
  // the process with StopProcess().
  if (pid_) {
    process_manager_->UpdateExitCallback(
        pid_, base::Bind(DoNothingWithExitStatus));
  }
  management_server_->Stop();
  if (!tls_auth_file_.empty()) {
    base::DeleteFile(tls_auth_file_, false);
    tls_auth_file_.clear();
  }
  if (!openvpn_config_file_.empty()) {
    base::DeleteFile(openvpn_config_file_, false);
    openvpn_config_file_.clear();
  }
  if (default_service_callback_tag_) {
    manager()->DeregisterDefaultServiceCallback(default_service_callback_tag_);
    default_service_callback_tag_ = 0;
  }
  rpc_task_.reset();
  int interface_index = -1;
  if (device_) {
    interface_index = device_->interface_index();
    device_->DropConnection();
    device_->SetEnabled(false);
    device_ = nullptr;
  }
  if (pid_) {
    if (interface_index >= 0) {
      // NB: |callback| must be bound to a static method, as
      // |callback| may be called after our dtor completes.
      const auto callback(
          Bind(OnOpenVPNExited, device_info_->AsWeakPtr(), interface_index));
      interface_index = -1;
      process_manager_->UpdateExitCallback(pid_, callback);
    }
    process_manager_->StopProcess(pid_);
    pid_ = 0;
  }
  if (interface_index >= 0) {
    device_info_->DeleteInterface(interface_index);
  }
  tunnel_interface_.clear();
  if (service_) {
    if (state == Service::kStateFailure) {
      service_->SetErrorDetails(error_details);
      service_->SetFailure(failure);
    } else {
      service_->SetState(state);
    }
    service_ = nullptr;
  }
  ip_properties_ = IPConfig::Properties();
}

// static
string OpenVPNDriver::JoinOptions(const vector<vector<string>>& options,
                                  char separator) {
  vector<string> option_strings;
  for (const auto& option : options) {
    vector<string> quoted_option;
    for (const auto& argument : option) {
      if (argument.find(' ') != string::npos ||
          argument.find('\t') != string::npos ||
          argument.find('"') != string::npos ||
          argument.find(separator) != string::npos) {
        string quoted_argument(argument);
        const char separator_chars[] = { separator, '\0' };
        base::ReplaceChars(argument, separator_chars, " ", &quoted_argument);
        base::ReplaceChars(quoted_argument, "\\", "\\\\", &quoted_argument);
        base::ReplaceChars(quoted_argument, "\"", "\\\"", &quoted_argument);
        quoted_option.push_back("\"" + quoted_argument + "\"");
      } else {
        quoted_option.push_back(argument);
      }
    }
    option_strings.push_back(base::JoinString(quoted_option, " "));
  }
  return base::JoinString(option_strings, string{separator});
}

bool OpenVPNDriver::WriteConfigFile(
    const vector<vector<string>>& options,
    FilePath* config_file) {
  if (!base::DirectoryExists(openvpn_config_directory_)) {
    if (!base::CreateDirectory(openvpn_config_directory_)) {
      LOG(ERROR) << "Unable to create configuration directory  "
                 << openvpn_config_directory_.value();
      return false;
    }
    if (chmod(openvpn_config_directory_.value().c_str(), S_IRWXU)) {
      LOG(ERROR) << "Failed to set permissions on "
                 << openvpn_config_directory_.value();
      base::DeleteFile(openvpn_config_directory_, true);
      return false;
    }
  }

  string contents = JoinOptions(options, '\n');
  contents.push_back('\n');
  if (!base::CreateTemporaryFileInDir(openvpn_config_directory_, config_file) ||
      base::WriteFile(*config_file, contents.data(), contents.size()) !=
          static_cast<int>(contents.size())) {
    LOG(ERROR) << "Unable to setup OpenVPN config file.";
    return false;
  }
  return true;
}

bool OpenVPNDriver::SpawnOpenVPN() {
  SLOG(this, 2) << __func__ << "(" << tunnel_interface_ << ")";

  vector<vector<string>> options;
  Error error;
  InitOptions(&options, &error);
  if (error.IsFailure()) {
    return false;
  }
  LOG(INFO) << "OpenVPN process options: " << JoinOptions(options, ',');
  if (!WriteConfigFile(options, &openvpn_config_file_)) {
    return false;
  }

  // TODO(quiche): This should be migrated to use ExternalTask.
  // (crbug.com/246263).
  CHECK(!pid_);
  pid_t pid = process_manager_->StartProcess(
      FROM_HERE, FilePath(kOpenVPNPath),
      vector<string>{"--config", openvpn_config_file_.value()},
      GetEnvironment(),
      false,  // Do not terminate with parent.
      base::Bind(&OpenVPNDriver::OnOpenVPNDied, base::Unretained(this)));
  if (pid < 0) {
    LOG(ERROR) << "Unable to spawn: " << kOpenVPNPath;
    return false;
  }

  pid_ = pid;
  return true;
}

void OpenVPNDriver::OnOpenVPNDied(int exit_status) {
  SLOG(nullptr, 2) << __func__ << "(" << pid_ << ", "  << exit_status << ")";
  pid_ = 0;
  FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
  // TODO(petkov): Figure if we need to restart the connection.
}

// static
void OpenVPNDriver::OnOpenVPNExited(const WeakPtr<DeviceInfo>& device_info,
                                    int interface_index,
                                    int /* exit_status */) {
  if (device_info) {
    LOG(INFO) << "Deleting interface " << interface_index;
    device_info->DeleteInterface(interface_index);
  }
}

bool OpenVPNDriver::ClaimInterface(const string& link_name,
                                   int interface_index) {
  if (link_name != tunnel_interface_) {
    return false;
  }

  SLOG(this, 2) << "Claiming " << link_name << " for OpenVPN tunnel";

  CHECK(!device_);
  device_ = new VirtualDevice(control_, dispatcher(), metrics_, manager(),
                              link_name, interface_index, Technology::kVPN);
  device_->SetEnabled(true);

  rpc_task_.reset(new RPCTask(control_, this));
  if (SpawnOpenVPN()) {
    default_service_callback_tag_ =
        manager()->RegisterDefaultServiceCallback(
            Bind(&OpenVPNDriver::OnDefaultServiceChanged, Unretained(this)));
  } else {
    FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
  }
  return true;
}

void OpenVPNDriver::GetLogin(string* /*user*/, string* /*password*/) {
  NOTREACHED();
}

void OpenVPNDriver::Notify(const string& reason,
                           const map<string, string>& dict) {
  LOG(INFO) << "IP configuration received: " << reason;
  if (reason != "up") {
    device_->DropConnection();
    return;
  }
  // On restart/reconnect, update the existing IP configuration.
  ParseIPConfiguration(dict, &ip_properties_);
  device_->SelectService(service_);
  device_->UpdateIPConfig(ip_properties_);
  ReportConnectionMetrics();
  StopConnectTimeout();
}

void OpenVPNDriver::ParseIPConfiguration(
    const map<string, string>& configuration,
    IPConfig::Properties* properties) const {
  ForeignOptions foreign_options;
  RouteOptions routes;
  bool is_gateway_route_required = false;

  properties->address_family = IPAddress::kFamilyIPv4;
  if (!properties->subnet_prefix) {
    properties->subnet_prefix =
        IPAddress::GetMaxPrefixLength(properties->address_family);
  }
  for (const auto& configuration_map : configuration) {
    const string& key = configuration_map.first;
    const string& value = configuration_map.second;
    SLOG(this, 2) << "Processing: " << key << " -> " << value;
    if (base::LowerCaseEqualsASCII(key, kOpenVPNIfconfigLocal)) {
      properties->address = value;
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNIfconfigBroadcast)) {
      properties->broadcast_address = value;
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNIfconfigNetmask)) {
      properties->subnet_prefix =
          IPAddress::GetPrefixLengthFromMask(properties->address_family, value);
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNIfconfigRemote)) {
      if (base::StartsWith(value, kSuspectedNetmaskPrefix,
                           base::CompareCase::INSENSITIVE_ASCII)) {
        LOG(WARNING) << "Option " << key << " value " << value
                     << " looks more like a netmask than a peer address; "
                     << "assuming it is the former.";
        // In this situation, the "peer_address" value will be left
        // unset and Connection::UpdateFromIPConfig() will treat the
        // interface as if it were a broadcast-style network.  The
        // kernel will, automatically set the peer address equal to
        // the local address.
        properties->subnet_prefix =
            IPAddress::GetPrefixLengthFromMask(properties->address_family,
                                               value);
      } else {
        properties->peer_address = value;
      }
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNRedirectGateway) ||
               base::LowerCaseEqualsASCII(key, kOpenVPNRedirectPrivate)) {
      is_gateway_route_required = true;
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNRouteVPNGateway)) {
      properties->gateway = value;
    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNTrustedIP)) {
      size_t prefix = IPAddress::GetMaxPrefixLength(properties->address_family);
      properties->exclusion_list.push_back(value + "/" +
                                           base::SizeTToString(prefix));

    } else if (base::LowerCaseEqualsASCII(key, kOpenVPNTunMTU)) {
      int mtu = 0;
      if (base::StringToInt(value, &mtu) && mtu >= IPConfig::kMinIPv4MTU) {
        properties->mtu = mtu;
      } else {
        LOG(ERROR) << "MTU " << value << " ignored.";
      }
    } else if (base::StartsWith(key, kOpenVPNForeignOptionPrefix,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      const string suffix = key.substr(strlen(kOpenVPNForeignOptionPrefix));
      int order = 0;
      if (base::StringToInt(suffix, &order)) {
        foreign_options[order] = value;
      } else {
        LOG(ERROR) << "Ignored unexpected foreign option suffix: " << suffix;
      }
    } else if (base::StartsWith(key, kOpenVPNRouteOptionPrefix,
                                base::CompareCase::INSENSITIVE_ASCII)) {
      ParseRouteOption(key.substr(strlen(kOpenVPNRouteOptionPrefix)),
                       value, &routes);
    } else {
      SLOG(this, 2) << "Key ignored.";
    }
  }
  ParseForeignOptions(foreign_options, properties);
  SetRoutes(routes, properties);

  if (const_args()->ContainsString(kOpenVPNIgnoreDefaultRouteProperty)) {
    if (is_gateway_route_required) {
      LOG(INFO) << "Configuration request to ignore default route is "
                << "overridden by the remote server.";
    } else {
      SLOG(this, 2) << "Ignoring default route parameter as requested by "
                    << "configuration.";
      properties->gateway.clear();
    }
  }
}

// static
void OpenVPNDriver::ParseForeignOptions(const ForeignOptions& options,
                                        IPConfig::Properties* properties) {
  vector<string> domain_search;
  vector<string> dns_servers;
  for (const auto& option_map : options) {
    ParseForeignOption(option_map.second, &domain_search, &dns_servers);
  }
  if (!domain_search.empty()) {
    properties->domain_search.swap(domain_search);
  }
  LOG_IF(WARNING, properties->domain_search.empty())
      << "No search domains provided.";
  if (!dns_servers.empty()) {
    properties->dns_servers.swap(dns_servers);
  }
  LOG_IF(WARNING, properties->dns_servers.empty())
      << "No DNS servers provided.";
}

// static
void OpenVPNDriver::ParseForeignOption(const string& option,
                                       vector<string>* domain_search,
                                       vector<string>* dns_servers) {
  SLOG(nullptr, 2) << __func__ << "(" << option << ")";
  vector<string> tokens = SplitString(option, " ", base::TRIM_WHITESPACE,
                                      base::SPLIT_WANT_ALL);
  if (tokens.size() != 3 ||
      !base::LowerCaseEqualsASCII(tokens[0], "dhcp-option")) {
    return;
  }
  if (base::LowerCaseEqualsASCII(tokens[1], "domain")) {
    domain_search->push_back(tokens[2]);
  } else if (base::LowerCaseEqualsASCII(tokens[1], "dns")) {
    dns_servers->push_back(tokens[2]);
  }
}

// static
IPConfig::Route* OpenVPNDriver::GetRouteOptionEntry(
    const string& prefix, const string& key, RouteOptions* routes) {
  int order = 0;
  if (!base::StartsWith(key, prefix, base::CompareCase::INSENSITIVE_ASCII) ||
      !base::StringToInt(key.substr(prefix.size()), &order)) {
    return nullptr;
  }
  return&(*routes)[order];
}

// static
void OpenVPNDriver::ParseRouteOption(
    const string& key, const string& value, RouteOptions* routes) {
  IPConfig::Route* route = GetRouteOptionEntry("network_", key, routes);
  if (route) {
    route->host = value;
    return;
  }
  route = GetRouteOptionEntry("netmask_", key, routes);
  if (route) {
    route->netmask = value;
    return;
  }
  route = GetRouteOptionEntry("gateway_", key, routes);
  if (route) {
    route->gateway = value;
    return;
  }
  LOG(WARNING) << "Unknown route option ignored: " << key;
}

// static
void OpenVPNDriver::SetRoutes(const RouteOptions& routes,
                              IPConfig::Properties* properties) {
  vector<IPConfig::Route> new_routes;
  for (const auto& route_map : routes) {
    const IPConfig::Route& route = route_map.second;
    if (route.host.empty() || route.netmask.empty() || route.gateway.empty()) {
      LOG(WARNING) << "Ignoring incomplete route: " << route_map.first;
      continue;
    }
    new_routes.push_back(route);
  }
  if (!new_routes.empty()) {
    properties->routes.swap(new_routes);
  }
  LOG_IF(WARNING, properties->routes.empty()) << "No routes provided.";
}

// static
bool OpenVPNDriver::SplitPortFromHost(
    const string& host, string* name, string* port) {
  vector<string> tokens = SplitString(host, ":", base::TRIM_WHITESPACE,
                                      base::SPLIT_WANT_ALL);
  int port_number = 0;
  if (tokens.size() != 2 || tokens[0].empty() || tokens[1].empty() ||
      !base::IsAsciiDigit(tokens[1][0]) ||
      !base::StringToInt(tokens[1], &port_number) ||
      port_number > std::numeric_limits<uint16_t>::max()) {
    return false;
  }
  *name = tokens[0];
  *port = tokens[1];
  return true;
}

void OpenVPNDriver::Connect(const VPNServiceRefPtr& service, Error* error) {
  StartConnectTimeout(kDefaultConnectTimeoutSeconds);
  service_ = service;
  service_->SetState(Service::kStateConfiguring);
  if (!device_info_->CreateTunnelInterface(&tunnel_interface_)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInternalError,
        "Could not create tunnel interface.");
    FailService(Service::kFailureInternal, Service::kErrorDetailsNone);
  }
  // Wait for the ClaimInterface callback to continue the connection process.
}

void OpenVPNDriver::InitOptions(vector<vector<string>>* options, Error* error) {
  string vpnhost = args()->LookupString(kProviderHostProperty, "");
  if (vpnhost.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments, "VPN host not specified.");
    return;
  }
  AppendOption("client", options);
  AppendOption("tls-client", options);

  string host_name, host_port;
  if (SplitPortFromHost(vpnhost, &host_name, &host_port)) {
    DCHECK(!host_name.empty());
    DCHECK(!host_port.empty());
    AppendOption("remote", host_name, host_port, options);
  } else {
    AppendOption("remote", vpnhost, options);
  }

  AppendOption("nobind", options);
  AppendOption("persist-key", options);
  AppendOption("persist-tun", options);

  CHECK(!tunnel_interface_.empty());
  AppendOption("dev", tunnel_interface_, options);
  AppendOption("dev-type", "tun", options);

  InitLoggingOptions(options);

  AppendValueOption(kVPNMTUProperty, "mtu", options);
  AppendValueOption(kOpenVPNProtoProperty, "proto", options);
  AppendValueOption(kOpenVPNPortProperty, "port", options);
  AppendValueOption(kOpenVPNTLSAuthProperty, "tls-auth", options);
  {
    string contents =
        args()->LookupString(kOpenVPNTLSAuthContentsProperty, "");
    if (!contents.empty()) {
      if (!base::CreateTemporaryFile(&tls_auth_file_) ||
          base::WriteFile(tls_auth_file_, contents.data(), contents.size()) !=
              static_cast<int>(contents.size())) {
        Error::PopulateAndLog(
            FROM_HERE, error, Error::kInternalError,
            "Unable to setup tls-auth file.");
        return;
      }
      AppendOption("tls-auth", tls_auth_file_.value(), options);
    }
  }
  AppendValueOption(kOpenVPNTLSRemoteProperty, "tls-remote", options);
  AppendValueOption(kOpenVPNCipherProperty, "cipher", options);
  AppendValueOption(kOpenVPNAuthProperty, "auth", options);
  AppendFlag(kOpenVPNAuthNoCacheProperty, "auth-nocache", options);
  AppendValueOption(kOpenVPNAuthRetryProperty, "auth-retry", options);
  AppendFlag(kOpenVPNCompLZOProperty, "comp-lzo", options);
  AppendFlag(kOpenVPNCompNoAdaptProperty, "comp-noadapt", options);
  AppendFlag(kOpenVPNPushPeerInfoProperty, "push-peer-info", options);
  AppendValueOption(kOpenVPNRenegSecProperty, "reneg-sec", options);
  AppendValueOption(kOpenVPNShaperProperty, "shaper", options);
  AppendValueOption(kOpenVPNServerPollTimeoutProperty,
                    "server-poll-timeout", options);

  if (!InitCAOptions(options, error)) {
    return;
  }

  // Additional remote certificate verification options.
  InitCertificateVerifyOptions(options);
  if (!InitExtraCertOptions(options, error)) {
    return;
  }

  // Client-side ping support.
  AppendValueOption(kOpenVPNPingProperty, "ping", options);
  AppendValueOption(kOpenVPNPingExitProperty, "ping-exit", options);
  AppendValueOption(kOpenVPNPingRestartProperty, "ping-restart", options);

  AppendValueOption(kOpenVPNNsCertTypeProperty, "ns-cert-type", options);

  InitClientAuthOptions(options);
  InitPKCS11Options(options);

  // TLS suport.
  string remote_cert_tls =
      args()->LookupString(kOpenVPNRemoteCertTLSProperty, "");
  if (remote_cert_tls.empty()) {
    remote_cert_tls = "server";
  }
  if (remote_cert_tls != "none") {
    AppendOption("remote-cert-tls", remote_cert_tls, options);
  }

  // This is an undocumented command line argument that works like a .cfg file
  // entry. TODO(sleffler): Maybe roll this into the "tls-auth" option?
  AppendValueOption(kOpenVPNKeyDirectionProperty, "key-direction", options);
  AppendValueOption(kOpenVPNRemoteCertEKUProperty, "remote-cert-eku", options);
  AppendDelimitedValueOption(kOpenVPNRemoteCertKUProperty,
                             "remote-cert-ku", ' ', options);

  if (!InitManagementChannelOptions(options, error)) {
    return;
  }

  // Setup openvpn-script options and RPC information required to send back
  // Layer 3 configuration.
  AppendOption("setenv", kRPCTaskServiceVariable,
               rpc_task_->GetRpcConnectionIdentifier(), options);
  AppendOption("setenv", kRPCTaskServiceVariable,
               rpc_task_->GetRpcConnectionIdentifier(), options);
  AppendOption("setenv", kRPCTaskPathVariable, rpc_task_->GetRpcIdentifier(),
               options);
  AppendOption("script-security", "2", options);
  AppendOption("up", kOpenVPNScript, options);
  AppendOption("up-restart", options);

  // Disable openvpn handling since we do route+ifconfig work.
  AppendOption("route-noexec", options);
  AppendOption("ifconfig-noexec", options);

  // Drop root privileges on connection and enable callback scripts to send
  // notify messages.
  AppendOption("user", "openvpn", options);
  AppendOption("group", "openvpn", options);
}

bool OpenVPNDriver::InitCAOptions(
    vector<vector<string>>* options, Error* error) {
  string ca_cert =
      args()->LookupString(kOpenVPNCaCertProperty, "");
  vector<string> ca_cert_pem;
  if (args()->ContainsStrings(kOpenVPNCaCertPemProperty)) {
    ca_cert_pem = args()->GetStrings(kOpenVPNCaCertPemProperty);
  }

  int num_ca_cert_types = 0;
  if (!ca_cert.empty())
      num_ca_cert_types++;
  if (!ca_cert_pem.empty())
      num_ca_cert_types++;
  if (num_ca_cert_types == 0) {
    // Use default CAs if no CA certificate is provided.
    AppendOption("ca", kDefaultCACertificates, options);
    return true;
  } else if (num_ca_cert_types > 1) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments,
        "Can't specify more than one of CACert and CACertPEM.");
    return false;
  }
  string cert_file;
  if (!ca_cert_pem.empty()) {
    DCHECK(ca_cert.empty());
    FilePath certfile = certificate_file_->CreatePEMFromStrings(ca_cert_pem);
    if (certfile.empty()) {
      Error::PopulateAndLog(
          FROM_HERE,
          error,
          Error::kInvalidArguments,
          "Unable to extract PEM CA certificates.");
      return false;
    }
    AppendOption("ca", certfile.value(), options);
    return true;
  }
  DCHECK(!ca_cert.empty() && ca_cert_pem.empty());
  AppendOption("ca", ca_cert, options);
  return true;
}

void OpenVPNDriver::InitCertificateVerifyOptions(
    std::vector<std::vector<std::string>>* options) {
  AppendValueOption(kOpenVPNVerifyHashProperty, "verify-hash", options);
  string x509_name = args()->LookupString(kOpenVPNVerifyX509NameProperty, "");
  if (!x509_name.empty()) {
    string x509_type = args()->LookupString(kOpenVPNVerifyX509TypeProperty, "");
    if (x509_type.empty()) {
      AppendOption("verify-x509-name", x509_name, options);
    } else {
      AppendOption("verify-x509-name", x509_name, x509_type, options);
    }
  }
}

bool OpenVPNDriver::InitExtraCertOptions(
    vector<vector<string>>* options, Error* error) {
  if (!args()->ContainsStrings(kOpenVPNExtraCertPemProperty)) {
    // It's okay for this parameter to be unspecified.
    return true;
  }

  vector<string> extra_certs = args()->GetStrings(kOpenVPNExtraCertPemProperty);
  if (extra_certs.empty()) {
    // It's okay for this parameter to be empty.
    return true;
  }

  FilePath certfile =
      extra_certificates_file_->CreatePEMFromStrings(extra_certs);
  if (certfile.empty()) {
    Error::PopulateAndLog(
        FROM_HERE,
        error,
        Error::kInvalidArguments,
        "Unable to extract extra PEM CA certificates.");
    return false;
  }

  AppendOption("extra-certs", certfile.value(), options);
  return true;
}

void OpenVPNDriver::InitPKCS11Options(vector<vector<string>>* options) {
  string id = args()->LookupString(kOpenVPNClientCertIdProperty, "");
  if (!id.empty()) {
    string provider =
        args()->LookupString(kOpenVPNProviderProperty, "");
    if (provider.empty()) {
      provider = kDefaultPKCS11Provider;
    }
    AppendOption("pkcs11-providers", provider, options);
    AppendOption("pkcs11-id", id, options);
  }
}

void OpenVPNDriver::InitClientAuthOptions(vector<vector<string>>* options) {
  bool has_cert = AppendValueOption(kOpenVPNCertProperty, "cert", options) ||
      !args()->LookupString(kOpenVPNClientCertIdProperty, "").empty();
  bool has_key = AppendValueOption(kOpenVPNKeyProperty, "key", options);
  // If the AuthUserPass property is set, or the User property is non-empty, or
  // there's neither a key, nor a cert available, specify user-password client
  // authentication.
  if (args()->ContainsString(kOpenVPNAuthUserPassProperty) ||
      !args()->LookupString(kOpenVPNUserProperty, "").empty() ||
      (!has_cert && !has_key)) {
    AppendOption("auth-user-pass", options);
  }
}

bool OpenVPNDriver::InitManagementChannelOptions(
    vector<vector<string>>* options, Error* error) {
  if (!management_server_->Start(dispatcher(), &sockets_, options)) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInternalError,
        "Unable to setup management channel.");
    return false;
  }
  // If there's a connected default service already, allow the openvpn client to
  // establish connection as soon as it's started. Otherwise, hold the client
  // until an underlying service connects and OnDefaultServiceChanged is
  // invoked.
  if (manager()->IsConnected()) {
    management_server_->ReleaseHold();
  }
  return true;
}

void OpenVPNDriver::InitLoggingOptions(vector<vector<string>>* options) {
  AppendOption("syslog", options);

  string verb = args()->LookupString(kOpenVPNVerbProperty, "");
  if (verb.empty() && SLOG_IS_ON(VPN, 0)) {
    verb = "3";
  }
  if (!verb.empty()) {
    AppendOption("verb", verb, options);
  }
}

void OpenVPNDriver::AppendOption(
    const string& option, vector<vector<string>>* options) {
  options->push_back(vector<string>{ option });
}

void OpenVPNDriver::AppendOption(
    const string& option,
    const string& value,
    vector<vector<string>>* options) {
  options->push_back(vector<string>{ option, value });
}

void OpenVPNDriver::AppendOption(
    const string& option,
    const string& value0,
    const string& value1,
    vector<vector<string>>* options) {
  options->push_back(vector<string>{ option, value0, value1 });
}

bool OpenVPNDriver::AppendValueOption(
    const string& property,
    const string& option,
    vector<vector<string>>* options) {
  string value = args()->LookupString(property, "");
  if (!value.empty()) {
    AppendOption(option, value, options);
    return true;
  }
  return false;
}

bool OpenVPNDriver::AppendDelimitedValueOption(
    const string& property,
    const string& option,
    char delimiter,
    vector<vector<string>>* options) {
  string value = args()->LookupString(property, "");
  if (!value.empty()) {
    vector<string> parts = SplitString(
        value, std::string{delimiter}, base::TRIM_WHITESPACE,
        base::SPLIT_WANT_ALL);
    parts.insert(parts.begin(), option);
    options->push_back(parts);
    return true;
  }
  return false;
}

bool OpenVPNDriver::AppendFlag(
    const string& property,
    const string& option,
    vector<vector<string>>* options) {
  if (args()->ContainsString(property)) {
    AppendOption(option, options);
    return true;
  }
  return false;
}

string OpenVPNDriver::GetServiceRpcIdentifier() const {
  if (service_ == nullptr)
    return "(openvpn_driver)";
  return service_->GetRpcIdentifier();
}

void OpenVPNDriver::Disconnect() {
  SLOG(this, 2) << __func__;
  IdleService();
}

void OpenVPNDriver::OnConnectionDisconnected() {
  LOG(INFO) << "Underlying connection disconnected.";
  // Restart the OpenVPN client forcing a reconnect attempt.
  management_server_->Restart();
  // Indicate reconnect state right away to drop the VPN connection and start
  // the connect timeout. This ensures that any miscommunication between shill
  // and openvpn will not lead to a permanently stale connectivity state. Note
  // that a subsequent invocation of OnReconnecting due to a RECONNECTING
  // message will essentially be a no-op.
  OnReconnecting(kReconnectReasonOffline);
}

void OpenVPNDriver::OnConnectTimeout() {
  VPNDriver::OnConnectTimeout();
  Service::ConnectFailure failure =
      management_server_->state() == OpenVPNManagementServer::kStateResolve ?
      Service::kFailureDNSLookup : Service::kFailureConnect;
  FailService(failure, Service::kErrorDetailsNone);
}

void OpenVPNDriver::OnReconnecting(ReconnectReason reason) {
  LOG(INFO) << __func__ << "(" << reason << ")";
  int timeout_seconds = GetReconnectTimeoutSeconds(reason);
  if (reason == kReconnectReasonTLSError &&
      timeout_seconds < connect_timeout_seconds()) {
    // Reconnect due to TLS error happens during connect so we need to cancel
    // the original connect timeout first and then reduce the time limit.
    StopConnectTimeout();
  }
  StartConnectTimeout(timeout_seconds);
  // On restart/reconnect, drop the VPN connection, if any. The openvpn client
  // might be in hold state if the VPN connection was previously established
  // successfully. The hold will be released by OnDefaultServiceChanged when a
  // new default service connects. This ensures that the client will use a fully
  // functional underlying connection to reconnect.
  if (device_) {
    device_->DropConnection();
  }
  if (service_) {
    service_->SetState(Service::kStateAssociating);
  }
}

// static
int OpenVPNDriver::GetReconnectTimeoutSeconds(ReconnectReason reason) {
  switch (reason) {
    case kReconnectReasonOffline:
      return kReconnectOfflineTimeoutSeconds;
    case kReconnectReasonTLSError:
      return kReconnectTLSErrorTimeoutSeconds;
    default:
      break;
  }
  return kDefaultConnectTimeoutSeconds;
}

string OpenVPNDriver::GetProviderType() const {
  return kProviderOpenVpn;
}

KeyValueStore OpenVPNDriver::GetProvider(Error* error) {
  SLOG(this, 2) << __func__;
  KeyValueStore props = VPNDriver::GetProvider(error);
  props.SetBool(kPassphraseRequiredProperty,
                args()->LookupString(kOpenVPNPasswordProperty, "").empty() &&
                args()->LookupString(kOpenVPNTokenProperty, "").empty());
  return props;
}

map<string, string> OpenVPNDriver::GetEnvironment() {
  SLOG(this, 2) << __func__ << "(" << lsb_release_file_.value() << ")";
  map<string, string> environment;
  string contents;
  if (!base::ReadFileToString(lsb_release_file_, &contents)) {
    LOG(ERROR) << "Unable to read the lsb-release file: "
               << lsb_release_file_.value();
    return environment;
  }
  vector<string> lines = SplitString(contents, "\n", base::TRIM_WHITESPACE,
                                     base::SPLIT_WANT_ALL);
  for (const auto& line : lines) {
    const size_t assign = line.find('=');
    if (assign == string::npos) {
      continue;
    }
    const string key = line.substr(0, assign);
    const string value = line.substr(assign + 1);
    if (key == kChromeOSReleaseName) {
      environment[kOpenVPNEnvVarPlatformName] = value;
    } else if (key == kChromeOSReleaseVersion) {
      environment[kOpenVPNEnvVarPlatformVersion] = value;
    }
    // Other LSB release values are irrelevant.
  }
  return environment;
}

void OpenVPNDriver::OnDefaultServiceChanged(const ServiceRefPtr& service) {
  SLOG(this, 2) << __func__
                << "(" << (service ? service->unique_name() : "-") << ")";
  // Allow the openvpn client to connect/reconnect only over a connected
  // underlying default service. If there's no default connected service, hold
  // the openvpn client until an underlying connection is established. If the
  // default service is our VPN service, hold the openvpn client on reconnect so
  // that the VPN connection can be torn down fully before a new connection
  // attempt is made over the underlying service.
  if (service && service != service_ && service->IsConnected()) {
    management_server_->ReleaseHold();
  } else {
    management_server_->Hold();
  }
}

void OpenVPNDriver::ReportConnectionMetrics() {
  metrics_->SendEnumToUMA(
      Metrics::kMetricVpnDriver,
      Metrics::kVpnDriverOpenVpn,
      Metrics::kMetricVpnDriverMax);

  if (args()->LookupString(kOpenVPNCaCertProperty, "") != "" ||
      (args()->ContainsStrings(kOpenVPNCaCertPemProperty) &&
       !args()->GetStrings(kOpenVPNCaCertPemProperty).empty())) {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnRemoteAuthenticationType,
        Metrics::kVpnRemoteAuthenticationTypeOpenVpnCertificate,
        Metrics::kMetricVpnRemoteAuthenticationTypeMax);
  } else {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnRemoteAuthenticationType,
        Metrics::kVpnRemoteAuthenticationTypeOpenVpnDefault,
        Metrics::kMetricVpnRemoteAuthenticationTypeMax);
  }

  bool has_user_authentication = false;
  if (args()->LookupString(kOpenVPNTokenProperty, "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeOpenVpnUsernameToken,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (args()->LookupString(kOpenVPNOTPProperty, "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePasswordOtp,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (args()->LookupString(kOpenVPNAuthUserPassProperty, "") != "" ||
      args()->LookupString(kOpenVPNUserProperty, "") != "")  {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeOpenVpnUsernamePassword,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (args()->LookupString(kOpenVPNClientCertIdProperty, "") != "" ||
      args()->LookupString(kOpenVPNCertProperty, "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeOpenVpnCertificate,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (!has_user_authentication) {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeOpenVpnNone,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
  }
}

}  // namespace shill
