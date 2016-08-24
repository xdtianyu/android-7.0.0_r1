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

#ifndef SHILL_CONNECTION_DIAGNOSTICS_H_
#define SHILL_CONNECTION_DIAGNOSTICS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/cancelable_callback.h>
#include <base/memory/weak_ptr.h>

#include "shill/portal_detector.h"
#include "shill/refptr_types.h"

namespace shill {

class ArpClient;
class ByteString;
class DeviceInfo;
class DNSClient;
class DNSClientFactory;
class Error;
class EventDispatcher;
class HTTPURL;
class IcmpSession;
class IcmpSessionFactory;
class Metrics;
class RoutingTable;
struct RoutingTableEntry;
class RTNLHandler;
class RTNLListener;
class RTNLMessage;

// The ConnectionDiagnostics class implements facilities to diagnose problems
// that a connection encounters reaching a specific URL.
//
// Given a connection and a URL, ConnectionDiagnostics performs the following
// actions:
// (A) Start portal detection on the connection using the given URL.
//     (B) If portal detection ends in the content phase, the connection is
//         either functioning, or we are trapped in a captive portal. END.
//     (C) If the portal detection ends in the DNS phase and failed for any
//         reason other than a timeout, we have found a DNS server issue. END.
//     (D) If the portal detection ends in the DNS phase and failed because of a
//         timeout, ping all DNS servers.
//         (E) If none of the DNS servers reply to pings, then we might have a
//             problem issue reaching DNS servers. Send a request to the kernel
//             for a route the first DNS server on our list (step M).
//         (F) If at least one DNS server replies to pings, and we have DNS
//             retries left, attempt DNS resolution again using the pingable DNS
//             servers.
//         (G) If at least one DNS server replies to pings but we are out of DNS
//             retries, the DNS servers are at fault. END.
//     (H) If portal detection ends in any other phase (i.e. HTTP or Connection)
//         resolve the IP of the target web server via DNS.
//         (I) If DNS resolution fails because of a timeout, ping all DNS
//             servers (step D).
//         (J) If DNS resolution fails for any other reason, we have found a
//             DNS server issue. END.
//         (K) Otherwise, ping the IP address of the target web server.
//             (L) If ping is successful, we can reach the target web server. We
//                 might have a HTTP issue or a broken portal. END.
//             (M) If ping is unsuccessful, we send a request to the kernel for
//                 a route to the IP address of the target web server.
//                 (N) If no route is found, a routing issue has been found.
//                     END.
//                 (O) If a route is found, and the destination is a local IPv6
//                     address, look for a neighbor table entry.
//                     (P) If a neighbor table entry is found, then this
//                         gateway/web server appears to be on the local
//                         network, but is not responding to pings. END.
//                     (Q) If a neighbor table entry is not found, then either
//                         this gateway/web server does not exist on the local
//                         network, or there are link layer issues.
//                 (R) If a route is found and the destination is a remote
//                     address, ping the local gateway.
//                     (S) If the local gateway respond to pings, then we have
//                         found an upstream connectivity problem or gateway
//                         problem. END.
//                     (T) If the local gateway is at an IPv6 address and does
//                         not respond to pings, look for a neighbor table
//                         entry (step O).
//                     (U) If the local gateway is at an IPv4 address and does
//                         not respond to pings, check for an ARP table entry
//                         for its address (step V).
//                 (V) Otherwise, if a route is found and the destination is a
//                     local IPv4 address, look for an ARP table entry for it.
//                     (W) If an ARP table entry is found, then this gateway/
//                         web server appears to be on the local network, but is
//                         not responding to pings. END.
//                     (X) If an ARP table entry is not found, check for IP
//                         address collision in the local network by sending out
//                         an ARP request for the local IP address of this
//                         connection.
//                         (Y) If a reply is received, an IP collision has been
//                             detected. END.
//                         (Z) If no reply was received, no IP address collision
//                             was detected. Since we are here because ARP and
//                             ping failed, either the web server or gateway
//                             does not actually exist on the local network, or
//                             there is a link layer issue. END.
//
// TODO(samueltan): Step F: if retry succeeds, remove the unresponsive DNS
// servers so Chrome does not try to use them.
// TODO(samueltan): Step X: find ways to disambiguate the cause (e.g. can we see
// packets from other hosts?).
class ConnectionDiagnostics {
 public:
  // The ConnectionDiagnostics::kEventNames string array depends on this enum.
  // Any changes to this enum should be synced with that array.
  enum Type {
    kTypePortalDetection = 0,
    kTypePingDNSServers = 1,
    kTypeResolveTargetServerIP = 2,
    kTypePingTargetServer = 3,
    kTypePingGateway = 4,
    kTypeFindRoute = 5,
    kTypeArpTableLookup = 6,
    kTypeNeighborTableLookup = 7,
    kTypeIPCollisionCheck = 8
  };

  // The ConnectionDiagnostics::kPhaseNames string array depends on this enum.
  // Any changes to this enum should be synced with that array.
  enum Phase {
    kPhaseStart = 0,
    kPhaseEnd = 1,
    // End phases specific to kTypePortalDetection.
    kPhasePortalDetectionEndContent = 2,
    kPhasePortalDetectionEndDNS = 3,
    kPhasePortalDetectionEndOther = 4
  };

  // The ConnectionDiagnostics::kResultNames string array depends on this enum.
  // Any changes to this enum should be synced with that array.
  enum Result {
    kResultSuccess = 0,
    kResultFailure = 1,
    kResultTimeout = 2
  };

  struct Event {
    Event(Type type_in, Phase phase_in, Result result_in,
          const std::string& message_in)
        : type(type_in),
          phase(phase_in),
          result(result_in),
          message(message_in) {}
    Type type;
    Phase phase;
    Result result;
    std::string message;
  };

  // The result of the diagnostics is a string describing the connection issue
  // detected (if any), and list of events (e.g. routing table
  // lookup, DNS resolution) performed during the diagnostics.
  using ResultCallback =
      base::Callback<void(const std::string&, const std::vector<Event>&)>;

  // Metrics::NotifyConnectionDiagnosticsIssue depends on these kIssue strings.
  // Any changes to these strings should be synced with that Metrics function.
  static const char kIssueIPCollision[];
  static const char kIssueRouting[];
  static const char kIssueHTTPBrokenPortal[];
  static const char kIssueDNSServerMisconfig[];
  static const char kIssueDNSServerNoResponse[];
  static const char kIssueNoDNSServersConfigured[];
  static const char kIssueDNSServersInvalid[];
  static const char kIssueNone[];
  static const char kIssueCaptivePortal[];
  static const char kIssueGatewayUpstream[];
  static const char kIssueGatewayNotResponding[];
  static const char kIssueServerNotResponding[];
  static const char kIssueGatewayArpFailed[];
  static const char kIssueServerArpFailed[];
  static const char kIssueInternalError[];
  static const char kIssueGatewayNoNeighborEntry[];
  static const char kIssueServerNoNeighborEntry[];
  static const char kIssueGatewayNeighborEntryNotConnected[];
  static const char kIssueServerNeighborEntryNotConnected[];

  ConnectionDiagnostics(ConnectionRefPtr connection,
                        EventDispatcher* dispatcher,
                        Metrics* metrics,
                        const DeviceInfo* device_info,
                        const ResultCallback& result_callback);
  ~ConnectionDiagnostics();

  // Starts diagnosing problems that |connection_| encounters reaching
  // |url_string|.
  bool Start(const std::string& url_string);

  // Skips the portal detection initiated in ConnectionDiagnostics::Start and
  // performs further diagnostics based on the |result| from a completed portal
  // detection attempt.
  bool StartAfterPortalDetection(const std::string& url_string,
                                 const PortalDetector::Result& result);

  void Stop();

  // Returns a string representation of |event|.
  static std::string EventToString(const Event& event);

  bool running() { return running_; }

 private:
  friend class ConnectionDiagnosticsTest;

  static const int kMaxDNSRetries;
  static const int kRouteQueryTimeoutSeconds;
  static const int kArpReplyTimeoutSeconds;
  static const int kNeighborTableRequestTimeoutSeconds;
  static const int kDNSTimeoutSeconds;

  // Create a new Event with |type|, |phase|, |result|, and an empty message,
  // and add it to the end of |diagnostic_events_|.
  void AddEvent(Type type, Phase phase, Result result);

  // Same as ConnectionDiagnostics::AddEvent, except that the added event
  // contains the string |message|.
  void AddEventWithMessage(Type type, Phase phase, Result result,
                           const std::string& message);

  // Calls |result_callback_|, then stops connection diagnostics.
  // |diagnostic_events_| and |issue| are passed as arguments to
  // |result_callback_| to report the results of the diagnostics.
  void ReportResultAndStop(const std::string &issue);

  void StartAfterPortalDetectionInternal(const PortalDetector::Result& result);

  // Attempts to resolve the IP address of |target_url_| using |dns_servers|.
  void ResolveTargetServerIPAddress(
      const std::vector<std::string>& dns_servers);

  // Pings all the DNS servers of |connection_|.
  void PingDNSServers();

  // Finds a route to the host at |address| by querying the kernel's routing
  // table.
  void FindRouteToHost(const IPAddress& address);

  // Finds an ARP table entry for |address| by querying the kernel's ARP table.
  void FindArpTableEntry(const IPAddress& address);

  // Finds a neighbor table entry for |address| by requesting an RTNL neighbor
  // table dump, and looking for a matching neighbor table entry for |address|
  // in ConnectionDiagnostics::OnNeighborMsgReceived.
  void FindNeighborTableEntry(const IPAddress& address);

  // Checks for an IP collision by sending out an ARP request for the local IP
  // address assigned to |connection_|.
  void CheckIpCollision();

  // Starts an IcmpSession with |address|. Called when we want to ping the
  // target web server or local gateway.
  void PingHost(const IPAddress& address);

  // Called after each IcmpSession started in
  // ConnectionDiagnostics::PingDNSServers finishes or times out. The DNS server
  // that was pinged can be uniquely identified with |dns_server_index|.
  // Attempts to resolve the IP address of |target_url_| again if at least one
  // DNS server was pinged successfully, and if |num_dns_attempts_| has not yet
  // reached |kMaxDNSRetries|.
  void OnPingDNSServerComplete(int dns_server_index,
                               const std::vector<base::TimeDelta>& result);

  // Called after the DNS IP address resolution on started in
  // ConnectionDiagnostics::ResolveTargetServerIPAddress completes.
  void OnDNSResolutionComplete(const Error& error, const IPAddress& address);

  // Called after the IcmpSession started in ConnectionDiagnostics::PingHost on
  // |address_pinged| finishes or times out. |ping_event_type| indicates the
  // type of ping that was started (gateway or target web server), and |result|
  // is the result of the IcmpSession.
  void OnPingHostComplete(Type ping_event_type, const IPAddress& address_pinged,
                          const std::vector<base::TimeDelta>& result);

  // This I/O callback is triggered whenever the ARP reception socket has data
  // available to be received.
  void OnArpReplyReceived(int fd);

  // Called if no replies to the ARP request sent in
  // ConnectionDiagnostics::CheckIpCollision are received within
  // |kArpReplyTimeoutSeconds| seconds.
  void OnArpRequestTimeout();

  // Called when replies are received to the neighbor table dump request issued
  // in ConnectionDiagnostics::FindNeighborTableEntry.
  void OnNeighborMsgReceived(const IPAddress& address_queried,
                             const RTNLMessage& msg);

  // Called if no neighbor table entry for |address_queried| is received within
  // |kNeighborTableRequestTimeoutSeconds| of issuing a dump request in
  // ConnectionDiagnostics::FindNeighborTableEntry.
  void OnNeighborTableRequestTimeout(const IPAddress& address_queried);

  // Called upon receiving a reply to the routing table query issued in
  // ConnectionDiagnostics::FindRoute.
  void OnRouteQueryResponse(int interface_index,
                            const RoutingTableEntry& entry);

  // Called if no replies to the routing table query issued in
  // ConnectionDiagnostics::FindRoute are received within
  // |kRouteQueryTimeoutSeconds|.
  void OnRouteQueryTimeout();

  // Utility function that returns true iff the event in |diagnostic_events_|
  // that is |num_events_ago| before the last event has a matching |type|,
  // |phase|, and |result|.
  bool DoesPreviousEventMatch(Type type, Phase phase, Result result,
                              size_t num_events_ago);

  base::WeakPtrFactory<ConnectionDiagnostics> weak_ptr_factory_;
  EventDispatcher* dispatcher_;
  Metrics* metrics_;
  RoutingTable* routing_table_;
  RTNLHandler* rtnl_handler_;

  // The connection being diagnosed.
  ConnectionRefPtr connection_;

  // Used to get the MAC address of the device associated with |connection_|.
  const DeviceInfo* device_info_;

  // The MAC address of device associated with |connection_|.
  ByteString local_mac_address_;

  DNSClientFactory* dns_client_factory_;
  std::unique_ptr<DNSClient> dns_client_;
  std::unique_ptr<PortalDetector> portal_detector_;
  std::unique_ptr<ArpClient> arp_client_;
  std::unique_ptr<IcmpSession> icmp_session_;

  // The URL being diagnosed. Stored in unique_ptr so that it can be cleared
  // when we stop diagnostics.
  std::unique_ptr<HTTPURL> target_url_;

  // Used to ping multiple DNS servers in |connection_| in parallel.
  IcmpSessionFactory* icmp_session_factory_;
  std::map<int, std::unique_ptr<IcmpSession>>
      id_to_pending_dns_server_icmp_session_;
  std::vector<std::string> pingable_dns_servers_;

  int num_dns_attempts_;
  bool running_;

  ResultCallback result_callback_;
  base::CancelableCallback<void(int, const RoutingTableEntry&)>
      route_query_callback_;
  base::CancelableClosure route_query_timeout_callback_;
  base::CancelableClosure arp_reply_timeout_callback_;
  base::CancelableClosure neighbor_request_timeout_callback_;

  // IOCallback that fires when the socket associated with |arp_client_| has a
  // packet to be received.  Calls ConnectionDiagnostics::OnArpReplyReceived.
  std::unique_ptr<IOHandler> receive_response_handler_;

  std::unique_ptr<RTNLListener> neighbor_msg_listener_;

  // Record of all diagnostic events that occurred, sorted in order of
  // occurrence.
  std::vector<Event> diagnostic_events_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionDiagnostics);
};

}  // namespace shill

#endif  // SHILL_CONNECTION_DIAGNOSTICS_H_
