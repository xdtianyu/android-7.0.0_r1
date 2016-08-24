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

// The term "L2TP / IPSec" refers to a pair of layered protocols used
// together to establish a tunneled VPN connection.  First, an "IPSec"
// link is created, which secures a single IP traffic pair between the
// client and server.  For this link to complete, one or two levels of
// authentication are performed.  The first, inner mandatory authentication
// ensures the two parties establishing the IPSec link are correct.  This
// can use a certificate exchange or a less secure "shared group key"
// (PSK) authentication.  An optional outer IPSec authentication can also be
// performed, which is not fully supported by shill's implementation.
// In order to support "tunnel groups" from some vendor VPNs shill supports
// supplying the authentication realm portion during the outer authentication.
// Notably, XAUTH and other forms of user authentication on this outer link
// are not supported.
//
// When IPSec authentication completes, traffic is tunneled through a
// layer 2 tunnel, called "L2TP".  Using the secured link, we tunnel a
// PPP link, through which a second layer of authentication is performed,
// using the provided "user" and "password" properties.

#include "shill/vpn/l2tp_ipsec_driver.h"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>
#if defined(__ANDROID__)
#include <dbus/service_constants.h>
#else
#include <chromeos/dbus/service_constants.h>
#endif  // __ANDROID__
#include <vpn-manager/service_error.h>

#include "shill/certificate_file.h"
#include "shill/device_info.h"
#include "shill/error.h"
#include "shill/external_task.h"
#include "shill/ipconfig.h"
#include "shill/logging.h"
#include "shill/manager.h"
#include "shill/ppp_daemon.h"
#include "shill/ppp_device.h"
#include "shill/ppp_device_factory.h"
#include "shill/process_manager.h"
#include "shill/vpn/vpn_service.h"

using base::Bind;
using base::Closure;
using base::FilePath;
using std::map;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kVPN;
static string ObjectID(L2TPIPSecDriver* l) {
  return l->GetServiceRpcIdentifier();
}
}

namespace {
const char kL2TPIPSecIPSecTimeoutProperty[] = "L2TPIPsec.IPsecTimeout";
const char kL2TPIPSecLeftProtoPortProperty[] = "L2TPIPsec.LeftProtoPort";
const char kL2TPIPSecLengthBitProperty[] = "L2TPIPsec.LengthBit";
const char kL2TPIPSecPFSProperty[] = "L2TPIPsec.PFS";
const char kL2TPIPSecRefusePapProperty[] = "L2TPIPsec.RefusePap";
const char kL2TPIPSecRekeyProperty[] = "L2TPIPsec.Rekey";
const char kL2TPIPSecRequireAuthProperty[] = "L2TPIPsec.RequireAuth";
const char kL2TPIPSecRequireChapProperty[] = "L2TPIPsec.RequireChap";
const char kL2TPIPSecRightProtoPortProperty[] = "L2TPIPsec.RightProtoPort";
}  // namespace

// static
const char L2TPIPSecDriver::kL2TPIPSecVPNPath[] = "/usr/sbin/l2tpipsec_vpn";
// static
const VPNDriver::Property L2TPIPSecDriver::kProperties[] = {
  { kL2tpIpsecAuthenticationType, 0 },
  { kL2tpIpsecCaCertNssProperty, 0 },
  { kL2tpIpsecClientCertIdProperty, 0 },
  { kL2tpIpsecClientCertSlotProperty, 0 },
  { kL2tpIpsecIkeVersion, 0 },
  { kL2tpIpsecPasswordProperty, Property::kCredential | Property::kWriteOnly },
  { kL2tpIpsecPinProperty, Property::kCredential },
  { kL2tpIpsecPskProperty, Property::kCredential | Property::kWriteOnly },
  { kL2tpIpsecUserProperty, 0 },
  { kProviderHostProperty, 0 },
  { kProviderTypeProperty, 0 },
  { kL2tpIpsecCaCertPemProperty, Property::kArray },
  { kL2tpIpsecTunnelGroupProperty, 0 },
  { kL2TPIPSecIPSecTimeoutProperty, 0 },
  { kL2TPIPSecLeftProtoPortProperty, 0 },
  { kL2TPIPSecLengthBitProperty, 0 },
  { kL2TPIPSecPFSProperty, 0 },
  { kL2TPIPSecRefusePapProperty, 0 },
  { kL2TPIPSecRekeyProperty, 0 },
  { kL2TPIPSecRequireAuthProperty, 0 },
  { kL2TPIPSecRequireChapProperty, 0 },
  { kL2TPIPSecRightProtoPortProperty, 0 },
  { kL2tpIpsecXauthUserProperty, Property::kCredential | Property::kWriteOnly },
  { kL2tpIpsecXauthPasswordProperty,
    Property::kCredential | Property::kWriteOnly },
  { kL2tpIpsecLcpEchoDisabledProperty, 0 },
};

L2TPIPSecDriver::L2TPIPSecDriver(ControlInterface* control,
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
      ppp_device_factory_(PPPDeviceFactory::GetInstance()),
      certificate_file_(new CertificateFile()),
      weak_ptr_factory_(this) {}

L2TPIPSecDriver::~L2TPIPSecDriver() {
  IdleService();
}

std::string L2TPIPSecDriver::GetServiceRpcIdentifier() {
  if (service_ == nullptr)
    return "(l2tp_ipsec_driver)";
  return service_->GetRpcIdentifier();
}

bool L2TPIPSecDriver::ClaimInterface(const string& link_name,
                                     int interface_index) {
  // TODO(petkov): crbug.com/212446.
  NOTIMPLEMENTED();
  return false;
}

void L2TPIPSecDriver::Connect(const VPNServiceRefPtr& service, Error* error) {
  StartConnectTimeout(kDefaultConnectTimeoutSeconds);
  service_ = service;
  service_->SetState(Service::kStateConfiguring);
  if (!SpawnL2TPIPSecVPN(error)) {
    FailService(Service::kFailureInternal);
  }
}

void L2TPIPSecDriver::Disconnect() {
  SLOG(this, 2) << __func__;
  IdleService();
}

void L2TPIPSecDriver::OnConnectionDisconnected() {
  LOG(INFO) << "Underlying connection disconnected.";
  IdleService();
}

void L2TPIPSecDriver::OnConnectTimeout() {
  VPNDriver::OnConnectTimeout();
  FailService(Service::kFailureConnect);
}

string L2TPIPSecDriver::GetProviderType() const {
  return kProviderL2tpIpsec;
}

void L2TPIPSecDriver::IdleService() {
  Cleanup(Service::kStateIdle, Service::kFailureUnknown);
}

void L2TPIPSecDriver::FailService(Service::ConnectFailure failure) {
  Cleanup(Service::kStateFailure, failure);
}

void L2TPIPSecDriver::Cleanup(Service::ConnectState state,
                              Service::ConnectFailure failure) {
  SLOG(this, 2) << __func__ << "("
                << Service::ConnectStateToString(state) << ", "
                << Service::ConnectFailureToString(failure) << ")";
  StopConnectTimeout();
  DeleteTemporaryFiles();
  external_task_.reset();
  if (device_) {
    device_->DropConnection();
    device_->SetEnabled(false);
    device_ = nullptr;
  }
  if (service_) {
    if (state == Service::kStateFailure) {
      service_->SetFailure(failure);
    } else {
      service_->SetState(state);
    }
    service_ = nullptr;
  }
}

void L2TPIPSecDriver::DeleteTemporaryFile(base::FilePath* temporary_file) {
  if (!temporary_file->empty()) {
    base::DeleteFile(*temporary_file, false);
    temporary_file->clear();
  }
}

void L2TPIPSecDriver::DeleteTemporaryFiles() {
  DeleteTemporaryFile(&psk_file_);
  DeleteTemporaryFile(&xauth_credentials_file_);
}

bool L2TPIPSecDriver::SpawnL2TPIPSecVPN(Error* error) {
  SLOG(this, 2) << __func__;
  std::unique_ptr<ExternalTask> external_task_local(
      new ExternalTask(control_,
                       process_manager_,
                       weak_ptr_factory_.GetWeakPtr(),
                       Bind(&L2TPIPSecDriver::OnL2TPIPSecVPNDied,
                            weak_ptr_factory_.GetWeakPtr())));

  vector<string> options;
  map<string, string> environment;  // No env vars passed.
  if (!InitOptions(&options, error)) {
    return false;
  }
  LOG(INFO) << "L2TP/IPSec VPN process options: "
            << base::JoinString(options, " ");

  if (external_task_local->Start(
          FilePath(kL2TPIPSecVPNPath), options, environment, true, error)) {
    external_task_ = std::move(external_task_local);
    return true;
  }
  return false;
}

bool L2TPIPSecDriver::InitOptions(vector<string>* options, Error* error) {
  string vpnhost = args()->LookupString(kProviderHostProperty, "");
  if (vpnhost.empty()) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInvalidArguments, "VPN host not specified.");
    return false;
  }

  if (!InitPSKOptions(options, error)) {
    return false;
  }

  if (!InitXauthOptions(options, error)) {
    return false;
  }

  options->push_back(base::StringPrintf("--remote_host=%s", vpnhost.c_str()));
  options->push_back(base::StringPrintf("--pppd_plugin=%s",
                                        PPPDaemon::kShimPluginPath));
  // Disable pppd from configuring IP addresses, routes, DNS.
  options->push_back("--nosystemconfig");

  // Accept a PEM CA certificate.
  InitPEMOptions(options);

  AppendValueOption(kL2tpIpsecClientCertIdProperty,
                    "--client_cert_id", options);
  AppendValueOption(kL2tpIpsecClientCertSlotProperty,
                    "--client_cert_slot", options);
  AppendValueOption(kL2tpIpsecPinProperty, "--user_pin", options);
  AppendValueOption(kL2tpIpsecUserProperty, "--user", options);
  AppendValueOption(kL2TPIPSecIPSecTimeoutProperty, "--ipsec_timeout", options);
  AppendValueOption(kL2TPIPSecLeftProtoPortProperty,
                    "--leftprotoport", options);
  AppendFlag(kL2TPIPSecPFSProperty, "--pfs", "--nopfs", options);
  AppendFlag(kL2TPIPSecRekeyProperty, "--rekey", "--norekey", options);
  AppendValueOption(kL2TPIPSecRightProtoPortProperty,
                    "--rightprotoport", options);
  AppendFlag(kL2TPIPSecRequireChapProperty,
             "--require_chap", "--norequire_chap", options);
  AppendFlag(kL2TPIPSecRefusePapProperty,
             "--refuse_pap", "--norefuse_pap", options);
  AppendFlag(kL2TPIPSecRequireAuthProperty,
             "--require_authentication", "--norequire_authentication", options);
  AppendFlag(kL2TPIPSecLengthBitProperty,
             "--length_bit", "--nolength_bit", options);
  AppendFlag(kL2tpIpsecLcpEchoDisabledProperty,
             "--noppp_lcp_echo", "--ppp_lcp_echo", options);
  AppendValueOption(kL2tpIpsecTunnelGroupProperty, "--tunnel_group", options);
  if (SLOG_IS_ON(VPN, 0)) {
    options->push_back("--debug");
  }
  return true;
}

bool L2TPIPSecDriver::InitPSKOptions(vector<string>* options, Error* error) {
  string psk = args()->LookupString(kL2tpIpsecPskProperty, "");
  if (!psk.empty()) {
    if (!base::CreateTemporaryFileInDir(manager()->run_path(), &psk_file_) ||
        chmod(psk_file_.value().c_str(), S_IRUSR | S_IWUSR) ||
        base::WriteFile(psk_file_, psk.data(), psk.size()) !=
            static_cast<int>(psk.size())) {
      Error::PopulateAndLog(
          FROM_HERE, error, Error::kInternalError, "Unable to setup psk file.");
      return false;
    }
    options->push_back(base::StringPrintf("--psk_file=%s",
                                          psk_file_.value().c_str()));
  }
  return true;
}

bool L2TPIPSecDriver::InitPEMOptions(vector<string>* options) {
  vector<string> ca_certs;
  if (args()->ContainsStrings(kL2tpIpsecCaCertPemProperty)) {
    ca_certs = args()->GetStrings(kL2tpIpsecCaCertPemProperty);
  }
  if (ca_certs.empty()) {
    return false;
  }
  FilePath certfile = certificate_file_->CreatePEMFromStrings(ca_certs);
  if (certfile.empty()) {
    LOG(ERROR) << "Unable to extract certificates from PEM string.";
    return false;
  }
  options->push_back(base::StringPrintf("--server_ca_file=%s",
                                        certfile.value().c_str()));
  return true;
}

bool L2TPIPSecDriver::InitXauthOptions(vector<string>* options, Error* error) {
  string user = args()->LookupString(kL2tpIpsecXauthUserProperty, "");
  string password = args()->LookupString(kL2tpIpsecXauthPasswordProperty, "");
  if (user.empty() && password.empty()) {
    // Xauth credentials not configured.
    return true;
  }
  if (user.empty() || password.empty()) {
      Error::PopulateAndLog(
          FROM_HERE, error, Error::kInvalidArguments,
          "XAUTH credentials are partially configured.");
    return false;
  }
  string xauth_credentials = user + "\n" + password + "\n";
  if (!base::CreateTemporaryFileInDir(manager()->run_path(),
                                      &xauth_credentials_file_) ||
      chmod(xauth_credentials_file_.value().c_str(), S_IRUSR | S_IWUSR) ||
      base::WriteFile(xauth_credentials_file_, xauth_credentials.data(),
                      xauth_credentials.size()) !=
          static_cast<int>(xauth_credentials.size())) {
    Error::PopulateAndLog(
        FROM_HERE, error, Error::kInternalError,
        "Unable to setup XAUTH credentials file.");
    return false;
  }
  options->push_back(base::StringPrintf("--xauth_credentials_file=%s",
                         xauth_credentials_file_.value().c_str()));
  return true;
}

bool L2TPIPSecDriver::AppendValueOption(
    const string& property, const string& option, vector<string>* options) {
  string value = args()->LookupString(property, "");
  if (!value.empty()) {
    options->push_back(base::StringPrintf("%s=%s", option.c_str(),
                                          value.c_str()));
    return true;
  }
  return false;
}

bool L2TPIPSecDriver::AppendFlag(const string& property,
                                 const string& true_option,
                                 const string& false_option,
                                 vector<string>* options) {
  string value = args()->LookupString(property, "");
  if (!value.empty()) {
    options->push_back(value == "true" ? true_option : false_option);
    return true;
  }
  return false;
}

void L2TPIPSecDriver::OnL2TPIPSecVPNDied(pid_t /*pid*/, int status) {
  FailService(TranslateExitStatusToFailure(status));
  // TODO(petkov): Figure if we need to restart the connection.
}

// static
Service::ConnectFailure L2TPIPSecDriver::TranslateExitStatusToFailure(
    int status) {
  if (!WIFEXITED(status)) {
    return Service::kFailureInternal;
  }
  switch (WEXITSTATUS(status)) {
    case vpn_manager::kServiceErrorResolveHostnameFailed:
      return Service::kFailureDNSLookup;
    case vpn_manager::kServiceErrorIpsecConnectionFailed:
    case vpn_manager::kServiceErrorL2tpConnectionFailed:
    case vpn_manager::kServiceErrorPppConnectionFailed:
      return Service::kFailureConnect;
    case vpn_manager::kServiceErrorIpsecPresharedKeyAuthenticationFailed:
      return Service::kFailureIPSecPSKAuth;
    case vpn_manager::kServiceErrorIpsecCertificateAuthenticationFailed:
      return Service::kFailureIPSecCertAuth;
    case vpn_manager::kServiceErrorPppAuthenticationFailed:
      return Service::kFailurePPPAuth;
    default:
      break;
  }
  return Service::kFailureUnknown;
}

void L2TPIPSecDriver::GetLogin(string* user, string* password) {
  LOG(INFO) << "Login requested.";
  string user_property =
      args()->LookupString(kL2tpIpsecUserProperty, "");
  if (user_property.empty()) {
    LOG(ERROR) << "User not set.";
    return;
  }
  string password_property =
      args()->LookupString(kL2tpIpsecPasswordProperty, "");
  if (password_property.empty()) {
    LOG(ERROR) << "Password not set.";
    return;
  }
  *user = user_property;
  *password = password_property;
}

void L2TPIPSecDriver::Notify(
    const string& reason, const map<string, string>& dict) {
  LOG(INFO) << "IP configuration received: " << reason;

  if (reason == kPPPReasonAuthenticating ||
      reason == kPPPReasonAuthenticated) {
    // These are uninteresting intermediate states that do not indicate failure.
    return;
  }

  if (reason != kPPPReasonConnect) {
    DCHECK_EQ(kPPPReasonDisconnect, reason);
    // DestroyLater, rather than while on stack.
    external_task_.release()->DestroyLater(dispatcher());
    FailService(Service::kFailureUnknown);
    return;
  }

  DeleteTemporaryFiles();

  string interface_name = PPPDevice::GetInterfaceName(dict);
  int interface_index = device_info_->GetIndex(interface_name);
  if (interface_index < 0) {
    // TODO(petkov): Consider handling the race when the RTNL notification about
    // the new PPP device has not been received yet. We can keep the IP
    // configuration and apply it when ClaimInterface is
    // invoked. crbug.com/212446.
    NOTIMPLEMENTED() << ": No device info for " << interface_name << ".";
    return;
  }

  // There is no IPv6 support for L2TP/IPsec VPN at this moment, so create a
  // blackhole route for IPv6 traffic after establishing a IPv4 VPN.
  // TODO(benchan): Generalize this when IPv6 support is added.
  bool blackhole_ipv6 = true;

  if (!device_) {
    device_ = ppp_device_factory_->CreatePPPDevice(
        control_, dispatcher(), metrics_, manager(), interface_name,
        interface_index);
  }
  device_->SetEnabled(true);
  device_->SelectService(service_);

  // Reduce MTU to the minimum viable for IPv6, since the IPSec layer consumes
  // some variable portion of the payload.  Although this system does not yet
  // support IPv6, it is a reasonable value to start with, since the minimum
  // IPv6 packet size will plausibly be a size any gateway would support, and
  // is also larger than the IPv4 minimum size.
  device_->UpdateIPConfigFromPPPWithMTU(
      dict, blackhole_ipv6, IPConfig::kMinIPv6MTU);

  ReportConnectionMetrics();
  StopConnectTimeout();
}

bool L2TPIPSecDriver::IsPskRequired() const {
  return
    const_args()->LookupString(kL2tpIpsecPskProperty, "").empty() &&
    const_args()->LookupString(kL2tpIpsecClientCertIdProperty, "").empty();
}

KeyValueStore L2TPIPSecDriver::GetProvider(Error* error) {
  SLOG(this, 2) << __func__;
  KeyValueStore props = VPNDriver::GetProvider(error);
  props.SetBool(kPassphraseRequiredProperty,
                args()->LookupString(kL2tpIpsecPasswordProperty, "").empty());
  props.SetBool(kL2tpIpsecPskRequiredProperty, IsPskRequired());
  return props;
}

void L2TPIPSecDriver::ReportConnectionMetrics() {
  metrics_->SendEnumToUMA(
      Metrics::kMetricVpnDriver,
      Metrics::kVpnDriverL2tpIpsec,
      Metrics::kMetricVpnDriverMax);

  // We output an enum for each of the authentication types specified,
  // even if more than one is set at the same time.
  bool has_remote_authentication = false;
  if (args()->LookupString(kL2tpIpsecPskProperty, "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnRemoteAuthenticationType,
        Metrics::kVpnRemoteAuthenticationTypeL2tpIpsecPsk,
        Metrics::kMetricVpnRemoteAuthenticationTypeMax);
    has_remote_authentication = true;
  }
  if (!has_remote_authentication) {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnRemoteAuthenticationType,
        Metrics::kVpnRemoteAuthenticationTypeL2tpIpsecDefault,
        Metrics::kMetricVpnRemoteAuthenticationTypeMax);
  }

  bool has_user_authentication = false;
  if (args()->LookupString(kL2tpIpsecClientCertIdProperty,
                           "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeL2tpIpsecCertificate,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (args()->LookupString(kL2tpIpsecPasswordProperty, "") != "") {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeL2tpIpsecUsernamePassword,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
    has_user_authentication = true;
  }
  if (!has_user_authentication) {
    metrics_->SendEnumToUMA(
        Metrics::kMetricVpnUserAuthenticationType,
        Metrics::kVpnUserAuthenticationTypeL2tpIpsecNone,
        Metrics::kMetricVpnUserAuthenticationTypeMax);
  }
}

}  // namespace shill
