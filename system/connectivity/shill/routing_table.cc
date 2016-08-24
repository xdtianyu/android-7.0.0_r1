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

#include "shill/routing_table.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/ether.h>
#include <net/if.h>  // NOLINT - must be included after netinet/ether.h
#include <net/if_arp.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <memory>
#include <string>

#include <base/bind.h>
#include <base/files/file_path.h>
#include <base/files/file_util.h>
#include <base/stl_util.h>
#include <base/strings/stringprintf.h>

#include "shill/ipconfig.h"
#include "shill/logging.h"
#include "shill/net/byte_string.h"
#include "shill/net/rtnl_handler.h"
#include "shill/net/rtnl_listener.h"
#include "shill/net/rtnl_message.h"
#include "shill/routing_table_entry.h"

using base::Bind;
using base::FilePath;
using base::Unretained;
using std::deque;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kRoute;
static string ObjectID(RoutingTable* r) { return "(routing_table)"; }
}

namespace {
base::LazyInstance<RoutingTable> g_routing_table = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
const char RoutingTable::kRouteFlushPath4[] = "/proc/sys/net/ipv4/route/flush";
// static
const char RoutingTable::kRouteFlushPath6[] = "/proc/sys/net/ipv6/route/flush";

RoutingTable::RoutingTable()
    : route_callback_(Bind(&RoutingTable::RouteMsgHandler, Unretained(this))),
      rtnl_handler_(RTNLHandler::GetInstance()) {
  SLOG(this, 2) << __func__;
}

RoutingTable::~RoutingTable() {}

RoutingTable* RoutingTable::GetInstance() {
  return g_routing_table.Pointer();
}

void RoutingTable::Start() {
  SLOG(this, 2) << __func__;

  route_listener_.reset(
      new RTNLListener(RTNLHandler::kRequestRoute, route_callback_));
  rtnl_handler_->RequestDump(RTNLHandler::kRequestRoute);
}

void RoutingTable::Stop() {
  SLOG(this, 2) << __func__;

  route_listener_.reset();
}

bool RoutingTable::AddRoute(int interface_index,
                            const RoutingTableEntry& entry) {
  SLOG(this, 2) << __func__ << ": "
                << "destination " << entry.dst.ToString()
                << " index " << interface_index
                << " gateway " << entry.gateway.ToString()
                << " metric " << entry.metric;

  CHECK(!entry.from_rtnl);
  if (!ApplyRoute(interface_index,
                  entry,
                  RTNLMessage::kModeAdd,
                  NLM_F_CREATE | NLM_F_EXCL)) {
    return false;
  }
  tables_[interface_index].push_back(entry);
  return true;
}

bool RoutingTable::GetDefaultRoute(int interface_index,
                                   IPAddress::Family family,
                                   RoutingTableEntry* entry) {
  RoutingTableEntry* found_entry;
  bool ret = GetDefaultRouteInternal(interface_index, family, &found_entry);
  if (ret) {
    *entry = *found_entry;
  }
  return ret;
}

bool RoutingTable::GetDefaultRouteInternal(int interface_index,
                                           IPAddress::Family family,
                                           RoutingTableEntry** entry) {
  SLOG(this, 2) << __func__ << " index " << interface_index
                << " family " << IPAddress::GetAddressFamilyName(family);

  Tables::iterator table = tables_.find(interface_index);
  if (table == tables_.end()) {
    SLOG(this, 2) << __func__ << " no table";
    return false;
  }

  for (auto& nent : table->second) {
    if (nent.dst.IsDefault() && nent.dst.family() == family) {
      *entry = &nent;
      SLOG(this, 2) << __func__ << ": found"
                    << " gateway " << nent.gateway.ToString()
                    << " metric " << nent.metric;
      return true;
    }
  }

  SLOG(this, 2) << __func__ << " no route";
  return false;
}

bool RoutingTable::SetDefaultRoute(int interface_index,
                                   const IPAddress& gateway_address,
                                   uint32_t metric,
                                   uint8_t table_id) {
  SLOG(this, 2) << __func__ << " index " << interface_index
                << " metric " << metric;

  RoutingTableEntry* old_entry;

  if (GetDefaultRouteInternal(interface_index,
                              gateway_address.family(),
                              &old_entry)) {
    if (old_entry->gateway.Equals(gateway_address)) {
      if (old_entry->metric != metric) {
        ReplaceMetric(interface_index, old_entry, metric);
      }
      return true;
    } else {
      // TODO(quiche): Update internal state as well?
      ApplyRoute(interface_index,
                 *old_entry,
                 RTNLMessage::kModeDelete,
                 0);
    }
  }

  IPAddress default_address(gateway_address.family());
  default_address.SetAddressToDefault();

  return AddRoute(interface_index,
                  RoutingTableEntry(default_address,
                                    default_address,
                                    gateway_address,
                                    metric,
                                    RT_SCOPE_UNIVERSE,
                                    false,
                                    table_id,
                                    RoutingTableEntry::kDefaultTag));
}

bool RoutingTable::ConfigureRoutes(int interface_index,
                                   const IPConfigRefPtr& ipconfig,
                                   uint32_t metric,
                                   uint8_t table_id) {
  bool ret = true;

  IPAddress::Family address_family = ipconfig->properties().address_family;
  const vector<IPConfig::Route>& routes = ipconfig->properties().routes;

  for (const auto& route : routes) {
    SLOG(this, 3) << "Installing route:"
                  << " Destination: " << route.host
                  << " Netmask: " << route.netmask
                  << " Gateway: " << route.gateway;
    IPAddress destination_address(address_family);
    IPAddress source_address(address_family);  // Left as default.
    IPAddress gateway_address(address_family);
    if (!destination_address.SetAddressFromString(route.host)) {
      LOG(ERROR) << "Failed to parse host "
                 << route.host;
      ret = false;
      continue;
    }
    if (!gateway_address.SetAddressFromString(route.gateway)) {
      LOG(ERROR) << "Failed to parse gateway "
                 << route.gateway;
      ret = false;
      continue;
    }
    destination_address.set_prefix(
        IPAddress::GetPrefixLengthFromMask(address_family, route.netmask));
    if (!AddRoute(interface_index,
                  RoutingTableEntry(destination_address,
                                    source_address,
                                    gateway_address,
                                    metric,
                                    RT_SCOPE_UNIVERSE,
                                    false,
                                    table_id,
                                    RoutingTableEntry::kDefaultTag))) {
      ret = false;
    }
  }
  return ret;
}

void RoutingTable::FlushRoutes(int interface_index) {
  SLOG(this, 2) << __func__;

  auto table = tables_.find(interface_index);
  if (table == tables_.end()) {
    return;
  }

  for (const auto& nent : table->second) {
    ApplyRoute(interface_index, nent, RTNLMessage::kModeDelete, 0);
  }
  table->second.clear();
}

void RoutingTable::FlushRoutesWithTag(int tag) {
  SLOG(this, 2) << __func__;

  for (auto& table : tables_) {
    for (auto nent = table.second.begin(); nent != table.second.end();) {
      if (nent->tag == tag) {
        ApplyRoute(table.first, *nent, RTNLMessage::kModeDelete, 0);
        nent = table.second.erase(nent);
      } else {
        ++nent;
      }
    }
  }
}

void RoutingTable::ResetTable(int interface_index) {
  tables_.erase(interface_index);
}

void RoutingTable::SetDefaultMetric(int interface_index, uint32_t metric) {
  SLOG(this, 2) << __func__ << " index " << interface_index
                << " metric " << metric;

  RoutingTableEntry* entry;
  if (GetDefaultRouteInternal(
          interface_index, IPAddress::kFamilyIPv4, &entry) &&
      entry->metric != metric) {
    ReplaceMetric(interface_index, entry, metric);
  }

  if (GetDefaultRouteInternal(
          interface_index, IPAddress::kFamilyIPv6, &entry) &&
      entry->metric != metric) {
    ReplaceMetric(interface_index, entry, metric);
  }
}

// static
bool RoutingTable::ParseRoutingTableMessage(const RTNLMessage& message,
                                            int* interface_index,
                                            RoutingTableEntry* entry) {
  if (message.type() != RTNLMessage::kTypeRoute ||
      message.family() == IPAddress::kFamilyUnknown ||
      !message.HasAttribute(RTA_OIF)) {
    return false;
  }

  const RTNLMessage::RouteStatus& route_status = message.route_status();

  if (route_status.type != RTN_UNICAST) {
    return false;
  }

  uint32_t interface_index_u32 = 0;
  if (!message.GetAttribute(RTA_OIF).ConvertToCPUUInt32(&interface_index_u32)) {
    return false;
  }
  *interface_index = interface_index_u32;

  uint32_t metric = 0;
  if (message.HasAttribute(RTA_PRIORITY)) {
    message.GetAttribute(RTA_PRIORITY).ConvertToCPUUInt32(&metric);
  }

  IPAddress default_addr(message.family());
  default_addr.SetAddressToDefault();

  ByteString dst_bytes(default_addr.address());
  if (message.HasAttribute(RTA_DST)) {
    dst_bytes = message.GetAttribute(RTA_DST);
  }
  ByteString src_bytes(default_addr.address());
  if (message.HasAttribute(RTA_SRC)) {
    src_bytes = message.GetAttribute(RTA_SRC);
  }
  ByteString gateway_bytes(default_addr.address());
  if (message.HasAttribute(RTA_GATEWAY)) {
    gateway_bytes = message.GetAttribute(RTA_GATEWAY);
  }

  entry->dst = IPAddress(message.family(), dst_bytes, route_status.dst_prefix);
  entry->src = IPAddress(message.family(), src_bytes, route_status.src_prefix);
  entry->gateway = IPAddress(message.family(), gateway_bytes);
  entry->metric = metric;
  entry->scope = route_status.scope;
  entry->from_rtnl = true;
  entry->table = route_status.table;

  return true;
}

void RoutingTable::RouteMsgHandler(const RTNLMessage& message) {
  int interface_index;
  RoutingTableEntry entry;

  if (!ParseRoutingTableMessage(message, &interface_index, &entry)) {
    return;
  }

  if (!route_queries_.empty() &&
      message.route_status().protocol == RTPROT_UNSPEC) {
    SLOG(this, 3) << __func__ << ": Message seq: " << message.seq()
                  << " mode " << message.mode()
                  << ", next query seq: " << route_queries_.front().sequence;

    // Purge queries that have expired (sequence number of this message is
    // greater than that of the head of the route query sequence).  Do the
    // math in a way that's roll-over independent.
    const auto kuint32max = std::numeric_limits<uint32_t>::max();
    while (route_queries_.front().sequence - message.seq() > kuint32max / 2) {
      LOG(ERROR) << __func__ << ": Purging un-replied route request sequence "
                 << route_queries_.front().sequence
                 << " (< " << message.seq() << ")";
      route_queries_.pop_front();
      if (route_queries_.empty())
        return;
    }

    const Query& query = route_queries_.front();
    if (query.sequence == message.seq()) {
      RoutingTableEntry add_entry(entry);
      add_entry.from_rtnl = false;
      add_entry.tag = query.tag;
      add_entry.table = query.table_id;
      bool added = true;
      if (add_entry.gateway.IsDefault()) {
        SLOG(this, 2) << __func__ << ": Ignoring route result with no gateway "
                      << "since we don't need to plumb these.";
      } else {
        SLOG(this, 2) << __func__ << ": Adding host route to "
                      << add_entry.dst.ToString();
        added = AddRoute(interface_index, add_entry);
      }
      if (added && !query.callback.is_null()) {
        SLOG(this, 2) << "Running query callback.";
        query.callback.Run(interface_index, add_entry);
      }
      route_queries_.pop_front();
    }
    return;
  } else if (message.route_status().protocol != RTPROT_BOOT) {
    // Responses to route queries come back with a protocol of
    // RTPROT_UNSPEC.  Otherwise, normal route updates that we are
    // interested in come with a protocol of RTPROT_BOOT.
    return;
  }

  TableEntryVector& table = tables_[interface_index];
  for (auto nent = table.begin(); nent != table.end(); ++nent)  {
    if (nent->dst.Equals(entry.dst) &&
        nent->src.Equals(entry.src) &&
        nent->gateway.Equals(entry.gateway) &&
        nent->scope == entry.scope) {
      if (message.mode() == RTNLMessage::kModeDelete &&
          nent->metric == entry.metric) {
        table.erase(nent);
      } else if (message.mode() == RTNLMessage::kModeAdd) {
        nent->from_rtnl = true;
        nent->metric = entry.metric;
      }
      return;
    }
  }

  if (message.mode() == RTNLMessage::kModeAdd) {
    SLOG(this, 2) << __func__ << " adding"
                  << " destination " << entry.dst.ToString()
                  << " index " << interface_index
                  << " gateway " << entry.gateway.ToString()
                  << " metric " << entry.metric;
    table.push_back(entry);
  }
}

bool RoutingTable::ApplyRoute(uint32_t interface_index,
                              const RoutingTableEntry& entry,
                              RTNLMessage::Mode mode,
                              unsigned int flags) {
  SLOG(this, 2) << base::StringPrintf(
      "%s: dst %s/%d src %s/%d index %d mode %d flags 0x%x",
      __func__, entry.dst.ToString().c_str(), entry.dst.prefix(),
      entry.src.ToString().c_str(), entry.src.prefix(),
      interface_index, mode, flags);

  RTNLMessage message(
      RTNLMessage::kTypeRoute,
      mode,
      NLM_F_REQUEST | flags,
      0,
      0,
      0,
      entry.dst.family());

  message.set_route_status(RTNLMessage::RouteStatus(
      entry.dst.prefix(),
      entry.src.prefix(),
      entry.table,
      RTPROT_BOOT,
      entry.scope,
      RTN_UNICAST,
      0));

  message.SetAttribute(RTA_DST, entry.dst.address());
  if (!entry.src.IsDefault()) {
    message.SetAttribute(RTA_SRC, entry.src.address());
  }
  if (!entry.gateway.IsDefault()) {
    message.SetAttribute(RTA_GATEWAY, entry.gateway.address());
  }
  message.SetAttribute(RTA_PRIORITY,
                       ByteString::CreateFromCPUUInt32(entry.metric));
  message.SetAttribute(RTA_OIF,
                       ByteString::CreateFromCPUUInt32(interface_index));

  return rtnl_handler_->SendMessage(&message);
}

// Somewhat surprisingly, the kernel allows you to create multiple routes
// to the same destination through the same interface with different metrics.
// Therefore, to change the metric on a route, we can't just use the
// NLM_F_REPLACE flag by itself.  We have to explicitly remove the old route.
// We do so after creating the route at a new metric so there is no traffic
// disruption to existing network streams.
void RoutingTable::ReplaceMetric(uint32_t interface_index,
                                 RoutingTableEntry* entry,
                                 uint32_t metric) {
  SLOG(this, 2) << __func__ << " index " << interface_index
                << " metric " << metric;
  RoutingTableEntry new_entry = *entry;
  new_entry.metric = metric;
  // First create the route at the new metric.
  ApplyRoute(interface_index, new_entry, RTNLMessage::kModeAdd,
             NLM_F_CREATE | NLM_F_REPLACE);
  // Then delete the route at the old metric.
  ApplyRoute(interface_index, *entry, RTNLMessage::kModeDelete, 0);
  // Now, update our routing table (via |*entry|) from |new_entry|.
  *entry = new_entry;
}

bool RoutingTable::FlushCache() {
  static const char* kPaths[2] = { kRouteFlushPath4, kRouteFlushPath6 };
  bool ret = true;

  SLOG(this, 2) << __func__;

  for (size_t i = 0; i < arraysize(kPaths); ++i) {
    if (base::WriteFile(FilePath(kPaths[i]), "-1", 2) != 2) {
      LOG(ERROR) << base::StringPrintf("Cannot write to route flush file %s",
                                       kPaths[i]);
      ret = false;
    }
  }

  return ret;
}

bool RoutingTable::RequestRouteToHost(const IPAddress& address,
                                      int interface_index,
                                      int tag,
                                      const Query::Callback& callback,
                                      uint8_t table_id) {
  // Make sure we don't get a cached response that is no longer valid.
  FlushCache();

  RTNLMessage message(
      RTNLMessage::kTypeRoute,
      RTNLMessage::kModeQuery,
      NLM_F_REQUEST,
      0,
      0,
      interface_index,
      address.family());

  RTNLMessage::RouteStatus status;
  status.dst_prefix = address.prefix();
  message.set_route_status(status);
  message.SetAttribute(RTA_DST, address.address());

  if (interface_index != -1) {
    message.SetAttribute(RTA_OIF,
                         ByteString::CreateFromCPUUInt32(interface_index));
  }

  if (!rtnl_handler_->SendMessage(&message)) {
    return false;
  }

  // Save the sequence number of the request so we can create a route for
  // this host when we get a reply.
  route_queries_.push_back(Query(message.seq(), tag, callback, table_id));

  return true;
}

bool RoutingTable::CreateBlackholeRoute(int interface_index,
                                        IPAddress::Family family,
                                        uint32_t metric,
                                        uint8_t table_id) {
  SLOG(this, 2) << base::StringPrintf(
      "%s: index %d family %s metric %d",
      __func__, interface_index,
      IPAddress::GetAddressFamilyName(family).c_str(), metric);

  RTNLMessage message(
      RTNLMessage::kTypeRoute,
      RTNLMessage::kModeAdd,
      NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL,
      0,
      0,
      0,
      family);

  message.set_route_status(RTNLMessage::RouteStatus(
      0,
      0,
      table_id,
      RTPROT_BOOT,
      RT_SCOPE_UNIVERSE,
      RTN_BLACKHOLE,
      0));

  message.SetAttribute(RTA_PRIORITY,
                       ByteString::CreateFromCPUUInt32(metric));
  message.SetAttribute(RTA_OIF,
                       ByteString::CreateFromCPUUInt32(interface_index));

  return rtnl_handler_->SendMessage(&message);
}

bool RoutingTable::CreateLinkRoute(int interface_index,
                                   const IPAddress& local_address,
                                   const IPAddress& remote_address,
                                   uint8_t table_id) {
  if (!local_address.CanReachAddress(remote_address)) {
    LOG(ERROR) << __func__ << " failed: "
               << remote_address.ToString() << " is not reachable from "
               << local_address.ToString();
    return false;
  }

  IPAddress default_address(local_address.family());
  default_address.SetAddressToDefault();
  IPAddress destination_address(remote_address);
  destination_address.set_prefix(
      IPAddress::GetMaxPrefixLength(remote_address.family()));
  SLOG(this, 2) << "Creating link route to " << destination_address.ToString()
                << " from " << local_address.ToString()
                << " on interface index " << interface_index;
  return AddRoute(interface_index,
                  RoutingTableEntry(destination_address,
                                    local_address,
                                    default_address,
                                    0,
                                    RT_SCOPE_LINK,
                                    false,
                                    table_id,
                                    RoutingTableEntry::kDefaultTag));
}

}  // namespace shill
