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

#include "shill/connection_diagnostics.h"

#include <base/bind.h>
#include <base/strings/stringprintf.h>

#include "shill/arp_client.h"
#include "shill/arp_packet.h"
#include "shill/connection.h"
#include "shill/connectivity_trial.h"
#include "shill/device_info.h"
#include "shill/dns_client.h"
#include "shill/dns_client_factory.h"
#include "shill/error.h"
#include "shill/event_dispatcher.h"
#include "shill/http_url.h"
#include "shill/icmp_session.h"
#include "shill/icmp_session_factory.h"
#include "shill/logging.h"
#include "shill/metrics.h"
#include "shill/net/byte_string.h"
#include "shill/net/rtnl_handler.h"
#include "shill/net/rtnl_listener.h"
#include "shill/net/rtnl_message.h"
#include "shill/routing_table.h"
#include "shill/routing_table_entry.h"

using base::Bind;
using base::StringPrintf;
using std::string;
using std::vector;

namespace {
// These strings are dependent on ConnectionDiagnostics::Type. Any changes to
// this array should be synced with ConnectionDiagnostics::Type.
const char* kEventNames[] = {
    "Portal detection",
    "Ping DNS servers",
    "DNS resolution",
    "Ping (target web server)",
    "Ping (gateway)",
    "Find route",
    "ARP table lookup",
    "Neighbor table lookup",
    "IP collision check"
};
// These strings are dependent on ConnectionDiagnostics::Phase. Any changes to
// this array should be synced with ConnectionDiagnostics::Phase.
const char* kPhaseNames[] = {
    "Start",
    "End",
    "End (Content)",
    "End (DNS)",
    "End (HTTP/CXN)"
};
// These strings are dependent on ConnectionDiagnostics::Result. Any changes to
// this array should be synced with ConnectionDiagnostics::Result.
const char* kResultNames[] = {
    "Success",
    "Failure",
    "Timeout"
};
// After we fail to ping the gateway, we 1) start ARP lookup, 2) fail ARP
// lookup, 3) start IP collision check, 4) end IP collision check.
const int kNumEventsFromPingGatewayEndToIpCollisionCheckEnd = 4;
}  // namespace

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kWiFi;
static string ObjectID(ConnectionDiagnostics* n) {
  return "(connection_diagnostics)";
}
}

const char ConnectionDiagnostics::kIssueIPCollision[] =
    "IP collision detected. Another host on the local network has been "
    "assigned the same IP address.";
const char ConnectionDiagnostics::kIssueRouting[] = "Routing problem detected.";
const char ConnectionDiagnostics::kIssueHTTPBrokenPortal[] =
    "Target URL is pingable. Connectivity problems might be caused by HTTP "
    "issues on the server or a broken portal.";
const char ConnectionDiagnostics::kIssueDNSServerMisconfig[] =
    "DNS servers responding to DNS queries, but sending invalid responses. "
    "DNS servers might be misconfigured.";
const char ConnectionDiagnostics::kIssueDNSServerNoResponse[] =
    "At least one DNS server is pingable, but is not responding to DNS "
    "requests. DNS server issue detected.";
const char ConnectionDiagnostics::kIssueNoDNSServersConfigured[] =
    "No DNS servers have been configured for this connection -- either the "
    "DHCP server or user configuration is invalid.";
const char ConnectionDiagnostics::kIssueDNSServersInvalid[] =
    "All configured DNS server addresses are invalid.";
const char ConnectionDiagnostics::kIssueNone[] =
    "No connection issue detected.";
const char ConnectionDiagnostics::kIssueCaptivePortal[] =
    "Trapped in captive portal.";
const char ConnectionDiagnostics::kIssueGatewayUpstream[] =
    "We can find a route to the target web server at a remote IP address, "
    "and the local gateway is pingable. Gatway issue or upstream "
    "connectivity problem detected.";
const char ConnectionDiagnostics::kIssueGatewayNotResponding[] =
    "This gateway appears to be on the local network, but is not responding to "
    "pings.";
const char ConnectionDiagnostics::kIssueServerNotResponding[] =
    "This web server appears to be on the local network, but is not responding "
    "to pings.";
const char ConnectionDiagnostics::kIssueGatewayArpFailed[] =
    "No ARP entry for the gateway. Either the gateway does not exist on the "
    "local network, or there are link layer issues.";
const char ConnectionDiagnostics::kIssueServerArpFailed[] =
    "No ARP entry for the web server. Either the web server does not exist on "
    "the local network, or there are link layer issues.";
const char ConnectionDiagnostics::kIssueInternalError[] =
    "The connection diagnostics encountered an internal failure.";
const char ConnectionDiagnostics::kIssueGatewayNoNeighborEntry[] =
    "No neighbor table entry for the gateway. Either the gateway does not "
    "exist on the local network, or there are link layer issues.";
const char ConnectionDiagnostics::kIssueServerNoNeighborEntry[] =
    "No neighbor table entry for the web server. Either the web server does "
    "not exist on the local network, or there are link layer issues.";
const char ConnectionDiagnostics::kIssueGatewayNeighborEntryNotConnected[] =
    "Neighbor table entry for the gateway is not in a connected state. Either "
    "the web server does not exist on the local network, or there are link "
    "layer issues.";
const char ConnectionDiagnostics::kIssueServerNeighborEntryNotConnected[] =
    "Neighbor table entry for the web server is not in a connected state. "
    "Either the web server does not exist on the local network, or there are "
    "link layer issues.";
const int ConnectionDiagnostics::kMaxDNSRetries = 2;
const int ConnectionDiagnostics::kRouteQueryTimeoutSeconds = 1;
const int ConnectionDiagnostics::kArpReplyTimeoutSeconds = 1;
const int ConnectionDiagnostics::kNeighborTableRequestTimeoutSeconds = 1;
const int ConnectionDiagnostics::kDNSTimeoutSeconds = 3;

ConnectionDiagnostics::ConnectionDiagnostics(
    ConnectionRefPtr connection, EventDispatcher* dispatcher, Metrics* metrics,
    const DeviceInfo* device_info, const ResultCallback& result_callback)
    : weak_ptr_factory_(this),
      dispatcher_(dispatcher),
      metrics_(metrics),
      routing_table_(RoutingTable::GetInstance()),
      rtnl_handler_(RTNLHandler::GetInstance()),
      connection_(connection),
      device_info_(device_info),
      dns_client_factory_(DNSClientFactory::GetInstance()),
      portal_detector_(new PortalDetector(
          connection_, dispatcher_,
          Bind(&ConnectionDiagnostics::StartAfterPortalDetectionInternal,
               weak_ptr_factory_.GetWeakPtr()))),
      arp_client_(new ArpClient(connection_->interface_index())),
      icmp_session_(new IcmpSession(dispatcher_)),
      icmp_session_factory_(IcmpSessionFactory::GetInstance()),
      num_dns_attempts_(0),
      running_(false),
      result_callback_(result_callback) {}

ConnectionDiagnostics::~ConnectionDiagnostics() {
  Stop();
}

bool ConnectionDiagnostics::Start(const string& url_string) {
  SLOG(this, 3) << __func__ << "(" << url_string << ")";

  if (running()) {
    LOG(ERROR) << "Connection diagnostics already started";
    return false;
  }

  target_url_.reset(new HTTPURL());
  if (!target_url_->ParseFromString(url_string)) {
    LOG(ERROR) << "Failed to parse URL string: " << url_string;
    Stop();
    return false;
  }

  if (!portal_detector_->Start(url_string)) {
    Stop();
    return false;
  }

  running_ = true;
  AddEvent(kTypePortalDetection, kPhaseStart, kResultSuccess);
  return true;
}

bool ConnectionDiagnostics::StartAfterPortalDetection(
    const string& url_string, const PortalDetector::Result& result) {
  SLOG(this, 3) << __func__ << "(" << url_string << ")";

  if (running()) {
    LOG(ERROR) << "Connection diagnostics already started";
    return false;
  }

  target_url_.reset(new HTTPURL());
  if (!target_url_->ParseFromString(url_string)) {
    LOG(ERROR) << "Failed to parse URL string: " << url_string;
    Stop();
    return false;
  }

  running_ = true;
  dispatcher_->PostTask(
      Bind(&ConnectionDiagnostics::StartAfterPortalDetectionInternal,
           weak_ptr_factory_.GetWeakPtr(), result));
  return true;
}

void ConnectionDiagnostics::Stop() {
  SLOG(this, 3) << __func__;

  running_ = false;
  num_dns_attempts_ = 0;
  diagnostic_events_.clear();
  dns_client_.reset();
  arp_client_->Stop();
  icmp_session_->Stop();
  portal_detector_.reset();
  receive_response_handler_.reset();
  neighbor_msg_listener_.reset();
  id_to_pending_dns_server_icmp_session_.clear();
  target_url_.reset();
  route_query_callback_.Cancel();
  route_query_timeout_callback_.Cancel();
  arp_reply_timeout_callback_.Cancel();
  neighbor_request_timeout_callback_.Cancel();
}

// static
string ConnectionDiagnostics::EventToString(const Event& event) {
  string message("");
  message.append(StringPrintf("Event: %-26sPhase: %-17sResult: %-10s",
                              kEventNames[event.type], kPhaseNames[event.phase],
                              kResultNames[event.result]));
  if (!event.message.empty()) {
    message.append(StringPrintf("Msg: %s", event.message.c_str()));
  }
  return message;
}

void ConnectionDiagnostics::AddEvent(Type type, Phase phase, Result result) {
  AddEventWithMessage(type, phase, result, "");
}

void ConnectionDiagnostics::AddEventWithMessage(Type type, Phase phase,
                                                Result result,
                                                const string& message) {
  diagnostic_events_.push_back(Event(type, phase, result, message));
}

void ConnectionDiagnostics::ReportResultAndStop(const string& issue) {
  SLOG(this, 3) << __func__;

  metrics_->NotifyConnectionDiagnosticsIssue(issue);
  if (!result_callback_.is_null()) {
    LOG(INFO) << "Connection diagnostics events:";
    for (size_t i = 0; i < diagnostic_events_.size(); ++i) {
      LOG(INFO) << "  #" << i << ": "
                << EventToString(diagnostic_events_[i]);
    }
    LOG(INFO) << "Connection diagnostics completed. Connection issue: "
              << issue;
    result_callback_.Run(issue, diagnostic_events_);
  }
  Stop();
}

void ConnectionDiagnostics::StartAfterPortalDetectionInternal(
    const PortalDetector::Result& result) {
  SLOG(this, 3) << __func__;

  Result result_type;
  if (result.trial_result.status == ConnectivityTrial::kStatusSuccess) {
    result_type = kResultSuccess;
  } else if (result.trial_result.status == ConnectivityTrial::kStatusTimeout) {
    result_type = kResultTimeout;
  } else {
    result_type = kResultFailure;
  }

  switch (result.trial_result.phase) {
    case ConnectivityTrial::kPhaseContent: {
      AddEvent(kTypePortalDetection, kPhasePortalDetectionEndContent,
               result_type);
      // We have found the issue if we end in the content phase.
      ReportResultAndStop(result_type == kResultSuccess ? kIssueNone
                                                        : kIssueCaptivePortal);
      break;
    }
    case ConnectivityTrial::kPhaseDNS: {
      AddEvent(kTypePortalDetection, kPhasePortalDetectionEndDNS, result_type);
      if (result.trial_result.status == ConnectivityTrial::kStatusSuccess) {
        LOG(ERROR) << __func__ << ": portal detection should not end with "
                                  "success status in DNS phase";
        ReportResultAndStop(kIssueInternalError);
      } else if (result.trial_result.status ==
                 ConnectivityTrial::kStatusTimeout) {
        // DNS timeout occurred in portal detection. Ping DNS servers to make
        // sure they are reachable.
        dispatcher_->PostTask(Bind(&ConnectionDiagnostics::PingDNSServers,
                                   weak_ptr_factory_.GetWeakPtr()));
      } else {
        ReportResultAndStop(kIssueDNSServerMisconfig);
      }
      break;
    }
    case ConnectivityTrial::kPhaseConnection:
    case ConnectivityTrial::kPhaseHTTP:
    case ConnectivityTrial::kPhaseUnknown:
    default: {
      AddEvent(kTypePortalDetection, kPhasePortalDetectionEndOther,
               result_type);
      if (result.trial_result.status == ConnectivityTrial::kStatusSuccess) {
        LOG(ERROR) << __func__
                   << ": portal detection should not end with success status in"
                      " Connection/HTTP/Unknown phase";
        ReportResultAndStop(kIssueInternalError);
      } else {
        dispatcher_->PostTask(
            Bind(&ConnectionDiagnostics::ResolveTargetServerIPAddress,
                 weak_ptr_factory_.GetWeakPtr(), connection_->dns_servers()));
      }
      break;
    }
  }
}

void ConnectionDiagnostics::ResolveTargetServerIPAddress(
    const vector<string>& dns_servers) {
  SLOG(this, 3) << __func__;

  Error e;
  dns_client_.reset(dns_client_factory_->CreateDNSClient(
      connection_->IsIPv6() ? IPAddress::kFamilyIPv6 : IPAddress::kFamilyIPv4,
      connection_->interface_name(), dns_servers, kDNSTimeoutSeconds * 1000,
      dispatcher_, Bind(&ConnectionDiagnostics::OnDNSResolutionComplete,
                        weak_ptr_factory_.GetWeakPtr())));
  if (!dns_client_->Start(target_url_->host(), &e)) {
    LOG(ERROR) << __func__ << ": could not start DNS -- " << e.message();
    AddEventWithMessage(kTypeResolveTargetServerIP, kPhaseStart, kResultFailure,
                        e.message().c_str());
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  AddEventWithMessage(kTypeResolveTargetServerIP, kPhaseStart, kResultSuccess,
                      StringPrintf("Attempt #%d", num_dns_attempts_));
  SLOG(this, 3) << __func__ << ": looking up " << target_url_->host()
                << " (attempt " << num_dns_attempts_ << ")";
  ++num_dns_attempts_;
}

void ConnectionDiagnostics::PingDNSServers() {
  SLOG(this, 3) << __func__;

  if (connection_->dns_servers().empty()) {
    LOG(ERROR) << __func__ << ": no DNS servers for this connection";
    AddEventWithMessage(kTypePingDNSServers, kPhaseStart, kResultFailure,
                        "No DNS servers for this connection");
    ReportResultAndStop(kIssueNoDNSServersConfigured);
    return;
  }

  id_to_pending_dns_server_icmp_session_.clear();
  pingable_dns_servers_.clear();
  size_t num_invalid_dns_server_addr = 0;
  size_t num_failed_icmp_session_start = 0;
  for (size_t i = 0; i < connection_->dns_servers().size(); ++i) {
    // If we encounter any errors starting ping for any DNS server, carry on
    // attempting to ping the other DNS servers rather than failing. We only
    // need to successfully ping a single DNS server to decide whether or not
    // DNS servers can be reached.
    IPAddress dns_server_ip_addr(connection_->dns_servers()[i]);
    if (dns_server_ip_addr.family() == IPAddress::kFamilyUnknown) {
      LOG(ERROR) << __func__
                 << ": could not parse DNS server IP address from string";
      ++num_invalid_dns_server_addr;
      continue;
    }

    bool emplace_success =
        (id_to_pending_dns_server_icmp_session_.emplace(
             i, std::unique_ptr<IcmpSession>(
                    icmp_session_factory_->CreateIcmpSession(dispatcher_))))
            .second;
    if (emplace_success &&
        id_to_pending_dns_server_icmp_session_.at(i)
            ->Start(dns_server_ip_addr,
                    Bind(&ConnectionDiagnostics::OnPingDNSServerComplete,
                         weak_ptr_factory_.GetWeakPtr(), i))) {
      SLOG(this, 3) << __func__ << ": pinging DNS server at "
                    << dns_server_ip_addr.ToString();
    } else {
      LOG(ERROR) << "Failed to initiate ping for DNS server at "
                 << dns_server_ip_addr.ToString();
      ++num_failed_icmp_session_start;
      if (emplace_success) {
        id_to_pending_dns_server_icmp_session_.erase(i);
      }
    }
  }

  if (id_to_pending_dns_server_icmp_session_.empty()) {
    AddEventWithMessage(
        kTypePingDNSServers, kPhaseStart, kResultFailure,
        "Could not start ping for any of the given DNS servers");
    if (num_invalid_dns_server_addr == connection_->dns_servers().size()) {
      ReportResultAndStop(kIssueDNSServersInvalid);
    } else if (num_failed_icmp_session_start ==
               connection_->dns_servers().size()) {
      ReportResultAndStop(kIssueInternalError);
    }
  } else {
    AddEvent(kTypePingDNSServers, kPhaseStart, kResultSuccess);
  }
}

void ConnectionDiagnostics::FindRouteToHost(const IPAddress& address) {
  SLOG(this, 3) << __func__;

  RoutingTableEntry entry;
  route_query_callback_.Reset(Bind(&ConnectionDiagnostics::OnRouteQueryResponse,
                                   weak_ptr_factory_.GetWeakPtr()));
  if (!routing_table_->RequestRouteToHost(
          address, connection_->interface_index(), -1,
          route_query_callback_.callback(), connection_->table_id())) {
    route_query_callback_.Cancel();
    LOG(ERROR) << __func__ << ": could not request route to "
               << address.ToString();
    AddEventWithMessage(kTypeFindRoute, kPhaseStart, kResultFailure,
                        StringPrintf("Could not request route to %s",
                                     address.ToString().c_str()));
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  // RoutingTable implementation does not have a built-in timeout mechanism
  // for un-replied route requests, so use our own.
  route_query_timeout_callback_.Reset(
      Bind(&ConnectionDiagnostics::OnRouteQueryTimeout,
           weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(route_query_timeout_callback_.callback(),
                               kRouteQueryTimeoutSeconds * 1000);
  AddEventWithMessage(
      kTypeFindRoute, kPhaseStart, kResultSuccess,
      StringPrintf("Requesting route to %s", address.ToString().c_str()));
}

void ConnectionDiagnostics::FindArpTableEntry(const IPAddress& address) {
  SLOG(this, 3) << __func__;

  if (address.family() != IPAddress::kFamilyIPv4) {
    // We only perform ARP table lookups for IPv4 addresses.
    LOG(ERROR) << __func__ << ": " << address.ToString()
               << " is not an IPv4 address";
    AddEventWithMessage(
        kTypeArpTableLookup, kPhaseStart, kResultFailure,
        StringPrintf("%s is not an IPv4 address", address.ToString().c_str()));
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  AddEventWithMessage(kTypeArpTableLookup, kPhaseStart, kResultSuccess,
                      StringPrintf("Finding ARP table entry for %s",
                                   address.ToString().c_str()));
  ByteString target_mac_address;
  if (device_info_->GetMACAddressOfPeer(connection_->interface_index(), address,
                                        &target_mac_address)) {
    AddEventWithMessage(kTypeArpTableLookup, kPhaseEnd, kResultSuccess,
                        StringPrintf("Found ARP table entry for %s",
                                     address.ToString().c_str()));
    ReportResultAndStop(address.Equals(connection_->gateway())
                            ? kIssueGatewayNotResponding
                            : kIssueServerNotResponding);
    return;
  }

  AddEventWithMessage(kTypeArpTableLookup, kPhaseEnd, kResultFailure,
                      StringPrintf("Could not find ARP table entry for %s",
                                   address.ToString().c_str()));
  dispatcher_->PostTask(Bind(&ConnectionDiagnostics::CheckIpCollision,
                             weak_ptr_factory_.GetWeakPtr()));
}

void ConnectionDiagnostics::FindNeighborTableEntry(const IPAddress& address) {
  SLOG(this, 3) << __func__;

  if (address.family() != IPAddress::kFamilyIPv6) {
    // We only perform neighbor table lookups for IPv6 addresses.
    LOG(ERROR) << __func__ << ": " << address.ToString()
               << " is not an IPv6 address";
    AddEventWithMessage(
        kTypeNeighborTableLookup, kPhaseStart, kResultFailure,
        StringPrintf("%s is not an IPv6 address", address.ToString().c_str()));
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  neighbor_msg_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestNeighbor,
                       Bind(&ConnectionDiagnostics::OnNeighborMsgReceived,
                            weak_ptr_factory_.GetWeakPtr(), address)));
  rtnl_handler_->RequestDump(RTNLHandler::kRequestNeighbor);

  neighbor_request_timeout_callback_.Reset(
      Bind(&ConnectionDiagnostics::OnNeighborTableRequestTimeout,
           weak_ptr_factory_.GetWeakPtr(), address));
  dispatcher_->PostDelayedTask(route_query_timeout_callback_.callback(),
                               kNeighborTableRequestTimeoutSeconds * 1000);
  AddEventWithMessage(kTypeNeighborTableLookup, kPhaseStart, kResultSuccess,
                      StringPrintf("Finding neighbor table entry for %s",
                                   address.ToString().c_str()));
}

void ConnectionDiagnostics::CheckIpCollision() {
  SLOG(this, 3) << __func__;

  if (!device_info_->GetMACAddress(connection_->interface_index(),
                                   &local_mac_address_)) {
    LOG(ERROR) << __func__ << ": could not get local MAC address";
    AddEventWithMessage(kTypeIPCollisionCheck, kPhaseStart, kResultFailure,
                        "Could not get local MAC address");
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  if (!arp_client_->StartReplyListener()) {
    LOG(ERROR) << __func__ << ": failed to start ARP client";
    AddEventWithMessage(kTypeIPCollisionCheck, kPhaseStart, kResultFailure,
                        "Failed to start ARP client");
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  receive_response_handler_.reset(dispatcher_->CreateReadyHandler(
      arp_client_->socket(), IOHandler::kModeInput,
      Bind(&ConnectionDiagnostics::OnArpReplyReceived,
           weak_ptr_factory_.GetWeakPtr())));

  ArpPacket request(connection_->local(), connection_->local(),
                    local_mac_address_, ByteString());
  if (!arp_client_->TransmitRequest(request)) {
    LOG(ERROR) << __func__ << ": failed to send ARP request";
    AddEventWithMessage(kTypeIPCollisionCheck, kPhaseStart, kResultFailure,
                        "Failed to send ARP request");
    arp_client_->Stop();
    receive_response_handler_.reset();
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  arp_reply_timeout_callback_.Reset(
      Bind(&ConnectionDiagnostics::OnArpRequestTimeout,
           weak_ptr_factory_.GetWeakPtr()));
  dispatcher_->PostDelayedTask(arp_reply_timeout_callback_.callback(),
                               kArpReplyTimeoutSeconds * 1000);
  AddEvent(kTypeIPCollisionCheck, kPhaseStart, kResultSuccess);
}

void ConnectionDiagnostics::PingHost(const IPAddress& address) {
  SLOG(this, 3) << __func__;

  Type event_type = address.Equals(connection_->gateway())
                        ? kTypePingGateway
                        : kTypePingTargetServer;
  if (!icmp_session_->Start(
          address, Bind(&ConnectionDiagnostics::OnPingHostComplete,
                        weak_ptr_factory_.GetWeakPtr(), event_type, address))) {
    LOG(ERROR) << __func__ << ": failed to start ICMP session with "
               << address.ToString();
    AddEventWithMessage(event_type, kPhaseStart, kResultFailure,
                        StringPrintf("Failed to start ICMP session with %s",
                                     address.ToString().c_str()));
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  AddEventWithMessage(event_type, kPhaseStart, kResultSuccess,
                      StringPrintf("Pinging %s", address.ToString().c_str()));
}

void ConnectionDiagnostics::OnPingDNSServerComplete(
    int dns_server_index, const vector<base::TimeDelta>& result) {
  SLOG(this, 3) << __func__ << "(DNS server index " << dns_server_index << ")";

  if (!id_to_pending_dns_server_icmp_session_.erase(dns_server_index)) {
    // This should not happen, since we expect exactly one callback for each
    // IcmpSession started with a unique |dns_server_index| value in
    // ConnectionDiagnostics::PingDNSServers. However, if this does happen for
    // any reason, |id_to_pending_dns_server_icmp_session_| might never become
    // empty, and we might never move to the next step after pinging DNS
    // servers. Stop diagnostics immediately to prevent this from happening.
    LOG(ERROR) << __func__
               << ": no matching pending DNS server ICMP session found";
    ReportResultAndStop(kIssueInternalError);
    return;
  }

  if (IcmpSession::AnyRepliesReceived(result)) {
    pingable_dns_servers_.push_back(
        connection_->dns_servers()[dns_server_index]);
  }
  if (!id_to_pending_dns_server_icmp_session_.empty()) {
    SLOG(this, 3) << __func__ << ": not yet finished pinging all DNS servers";
    return;
  }

  if (pingable_dns_servers_.empty()) {
    // Use the first DNS server on the list and diagnose its connectivity.
    IPAddress first_dns_server_ip_addr(connection_->dns_servers()[0]);
    if (first_dns_server_ip_addr.family() == IPAddress::kFamilyUnknown) {
      LOG(ERROR) << __func__ << ": could not parse DNS server IP address "
                 << connection_->dns_servers()[0];
      AddEventWithMessage(kTypePingDNSServers, kPhaseEnd, kResultFailure,
                          StringPrintf("Could not parse DNS "
                                       "server IP address %s",
                                       connection_->dns_servers()[0].c_str()));
      ReportResultAndStop(kIssueInternalError);
      return;
    }
    AddEventWithMessage(
        kTypePingDNSServers, kPhaseEnd, kResultFailure,
        StringPrintf(
            "No DNS servers responded to pings. Pinging first DNS server at %s",
            first_dns_server_ip_addr.ToString().c_str()));
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindRouteToHost,
                               weak_ptr_factory_.GetWeakPtr(),
                               first_dns_server_ip_addr));
    return;
  }

  if (pingable_dns_servers_.size() != connection_->dns_servers().size()) {
    AddEventWithMessage(kTypePingDNSServers, kPhaseEnd, kResultSuccess,
                        "Pinged some, but not all, DNS servers successfully");
  } else {
    AddEventWithMessage(kTypePingDNSServers, kPhaseEnd, kResultSuccess,
                        "Pinged all DNS servers successfully");
  }

  if (num_dns_attempts_ < kMaxDNSRetries) {
    dispatcher_->PostTask(
        Bind(&ConnectionDiagnostics::ResolveTargetServerIPAddress,
             weak_ptr_factory_.GetWeakPtr(), pingable_dns_servers_));
  } else {
    SLOG(this, 3) << __func__ << ": max DNS resolution attempts reached";
    ReportResultAndStop(kIssueDNSServerNoResponse);
  }
}

void ConnectionDiagnostics::OnDNSResolutionComplete(const Error& error,
                                                    const IPAddress& address) {
  SLOG(this, 3) << __func__;

  if (error.IsSuccess()) {
    AddEventWithMessage(
        kTypeResolveTargetServerIP, kPhaseEnd, kResultSuccess,
        StringPrintf("Target address is %s", address.ToString().c_str()));
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::PingHost,
                               weak_ptr_factory_.GetWeakPtr(), address));
  } else if (error.type() == Error::kOperationTimeout) {
    AddEventWithMessage(
        kTypeResolveTargetServerIP, kPhaseEnd, kResultTimeout,
        StringPrintf("DNS resolution timed out: %s", error.message().c_str()));
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::PingDNSServers,
                               weak_ptr_factory_.GetWeakPtr()));
  } else {
    AddEventWithMessage(
        kTypeResolveTargetServerIP, kPhaseEnd, kResultFailure,
        StringPrintf("DNS resolution failed: %s", error.message().c_str()));
    ReportResultAndStop(kIssueDNSServerMisconfig);
  }
}

void ConnectionDiagnostics::OnPingHostComplete(
    Type ping_event_type, const IPAddress& address_pinged,
    const vector<base::TimeDelta>& result) {
  SLOG(this, 3) << __func__;

  string message(StringPrintf("Destination: %s,  Latencies: ",
                              address_pinged.ToString().c_str()));
  for (const auto& latency : result) {
    if (latency.is_zero()) {
      message.append("NA ");
    } else {
      message.append(StringPrintf("%4.2fms ", latency.InMillisecondsF()));
    }
  }

  Result result_type =
      IcmpSession::AnyRepliesReceived(result) ? kResultSuccess : kResultFailure;
  if (IcmpSession::IsPacketLossPercentageGreaterThan(result, 50)) {
    LOG(WARNING) << __func__ << ": high packet loss when pinging "
                 << address_pinged.ToString();
  }
  AddEventWithMessage(ping_event_type, kPhaseEnd, result_type, message);
  if (result_type == kResultSuccess) {
    // If pinging the target web server succeeded, we have found a HTTP issue or
    // broken portal. Otherwise, if pinging the gateway succeeded, we have found
    // an upstream connectivity problem or gateway issue.
    ReportResultAndStop(ping_event_type == kTypePingGateway
                            ? kIssueGatewayUpstream
                            : kIssueHTTPBrokenPortal);
  } else if (result_type == kResultFailure &&
             ping_event_type == kTypePingTargetServer) {
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindRouteToHost,
                               weak_ptr_factory_.GetWeakPtr(), address_pinged));
  } else if (result_type == kResultFailure &&
             ping_event_type == kTypePingGateway &&
             address_pinged.family() == IPAddress::kFamilyIPv4) {
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindArpTableEntry,
                               weak_ptr_factory_.GetWeakPtr(), address_pinged));
  } else {
    // We failed to ping an IPv6 gateway. Check for neighbor table entry for
    // this gateway.
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindNeighborTableEntry,
                               weak_ptr_factory_.GetWeakPtr(), address_pinged));
  }
}

void ConnectionDiagnostics::OnArpReplyReceived(int fd) {
  SLOG(this, 3) << __func__ << "(fd " << fd << ")";

  ArpPacket packet;
  ByteString sender;
  if (!arp_client_->ReceivePacket(&packet, &sender)) {
    return;
  }

  if (!packet.IsReply()) {
    SLOG(this, 4) << __func__ << ": this is not a reply packet. Ignoring.";
    return;
  }

  if (!connection_->local().address().Equals(
          packet.remote_ip_address().address())) {
    SLOG(this, 4) << __func__ << ": response is not for our IP address.";
    return;
  }

  if (!local_mac_address_.Equals(packet.remote_mac_address())) {
    SLOG(this, 4) << __func__ << ": response is not for our MAC address.";
    return;
  }

  if (connection_->local().address().Equals(
          packet.local_ip_address().address())) {
    arp_reply_timeout_callback_.Cancel();
    AddEventWithMessage(kTypeIPCollisionCheck, kPhaseEnd, kResultSuccess,
                        "IP collision found");
    ReportResultAndStop(kIssueIPCollision);
  }
}

void ConnectionDiagnostics::OnArpRequestTimeout() {
  SLOG(this, 3) << __func__;

  AddEventWithMessage(kTypeIPCollisionCheck, kPhaseEnd, kResultFailure,
                      "No IP collision found");
  // TODO(samueltan): perform link-level diagnostics.
  if (DoesPreviousEventMatch(
          kTypePingGateway, kPhaseEnd, kResultFailure,
          kNumEventsFromPingGatewayEndToIpCollisionCheckEnd)) {
    // We came here from failing to ping the gateway.
    ReportResultAndStop(kIssueGatewayArpFailed);
  } else {
    // Otherwise, we must have come here from failing to ping the target web
    // server and successfully finding a route.
    ReportResultAndStop(kIssueServerArpFailed);
  }
}

void ConnectionDiagnostics::OnNeighborMsgReceived(
    const IPAddress& address_queried, const RTNLMessage& msg) {
  SLOG(this, 3) << __func__;

  DCHECK(msg.type() == RTNLMessage::kTypeNeighbor);
  const RTNLMessage::NeighborStatus& neighbor = msg.neighbor_status();

  if (neighbor.type != NDA_DST || !msg.HasAttribute(NDA_DST)) {
    SLOG(this, 4) << __func__ << ": neighbor message has no destination";
    return;
  }

  IPAddress address(msg.family(), msg.GetAttribute(NDA_DST));
  if (!address.Equals(address_queried)) {
    SLOG(this, 4) << __func__ << ": destination address (" << address.ToString()
                  << ") does not match address queried ("
                  << address_queried.ToString() << ")";
    return;
  }

  neighbor_request_timeout_callback_.Cancel();
  if (!(neighbor.state & (NUD_PERMANENT | NUD_NOARP | NUD_REACHABLE))) {
    AddEventWithMessage(
        kTypeNeighborTableLookup, kPhaseEnd, kResultFailure,
        StringPrintf("Neighbor table entry for %s is not in a connected state "
                     "(actual state = 0x%2x)",
                     address_queried.ToString().c_str(), neighbor.state));
    ReportResultAndStop(address_queried.Equals(connection_->gateway())
                            ? kIssueGatewayNeighborEntryNotConnected
                            : kIssueServerNeighborEntryNotConnected);
    return;
  }

  AddEventWithMessage(kTypeNeighborTableLookup, kPhaseEnd, kResultSuccess,
                      StringPrintf("Neighbor table entry found for %s",
                                   address_queried.ToString().c_str()));
  ReportResultAndStop(address_queried.Equals(connection_->gateway())
                          ? kIssueGatewayNotResponding
                          : kIssueServerNotResponding);
}

void ConnectionDiagnostics::OnNeighborTableRequestTimeout(
    const IPAddress& address_queried) {
  SLOG(this, 3) << __func__;

  AddEventWithMessage(kTypeNeighborTableLookup, kPhaseEnd, kResultFailure,
                      StringPrintf("Failed to find neighbor table entry for %s",
                                   address_queried.ToString().c_str()));
  ReportResultAndStop(address_queried.Equals(connection_->gateway())
                          ? kIssueGatewayNoNeighborEntry
                          : kIssueServerNoNeighborEntry);
}

void ConnectionDiagnostics::OnRouteQueryResponse(
    int interface_index, const RoutingTableEntry& entry) {
  SLOG(this, 3) << __func__ << "(interface " << interface_index << ")";

  if (interface_index != connection_->interface_index()) {
    SLOG(this, 3) << __func__
                  << ": route query response not meant for this interface";
    return;
  }

  route_query_timeout_callback_.Cancel();
  AddEventWithMessage(
      kTypeFindRoute, kPhaseEnd, kResultSuccess,
      StringPrintf("Found route to %s (%s)", entry.dst.ToString().c_str(),
                   entry.gateway.IsDefault() ? "remote" : "local"));
  if (!entry.gateway.IsDefault()) {
    // We have a route to a remote destination, so ping the route gateway to
    // check if we have a means of reaching this host.
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::PingHost,
                               weak_ptr_factory_.GetWeakPtr(), entry.gateway));
  } else if (entry.dst.family() == IPAddress::kFamilyIPv4) {
    // We have a route to a local IPv4 destination, so check for an ARP table
    // entry.
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindArpTableEntry,
                               weak_ptr_factory_.GetWeakPtr(), entry.dst));
  } else {
    // We have a route to a local IPv6 destination, so check for a neighbor
    // table entry.
    dispatcher_->PostTask(Bind(&ConnectionDiagnostics::FindNeighborTableEntry,
                               weak_ptr_factory_.GetWeakPtr(), entry.dst));
  }
}

void ConnectionDiagnostics::OnRouteQueryTimeout() {
  SLOG(this, 3) << __func__;

  AddEvent(kTypeFindRoute, kPhaseEnd, kResultFailure);
  ReportResultAndStop(kIssueRouting);
}

bool ConnectionDiagnostics::DoesPreviousEventMatch(Type type, Phase phase,
                                                   Result result,
                                                   size_t num_events_ago) {
  int event_index = diagnostic_events_.size() - 1 - num_events_ago;
  if (event_index < 0) {
    LOG(ERROR) << __func__ << ": requested event " << num_events_ago
               << " before the last event, but we only have "
               << diagnostic_events_.size() << " logged";
    return false;
  }

  return (diagnostic_events_[event_index].type == type &&
          diagnostic_events_[event_index].phase == phase &&
          diagnostic_events_[event_index].result == result);
}

}  // namespace shill
