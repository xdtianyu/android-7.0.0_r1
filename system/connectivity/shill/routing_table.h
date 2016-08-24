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

#ifndef SHILL_ROUTING_TABLE_H_
#define SHILL_ROUTING_TABLE_H_

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <base/callback.h>
#include <base/lazy_instance.h>
#include <base/memory/ref_counted.h>

#include "shill/net/ip_address.h"
#include "shill/net/rtnl_message.h"
#include "shill/refptr_types.h"

namespace shill {

class RTNLHandler;
class RTNLListener;
struct RoutingTableEntry;

// This singleton maintains an in-process copy of the routing table on
// a per-interface basis.  It offers the ability for other modules to
// make modifications to the routing table, centered around setting the
// default route for an interface or modifying its metric (priority).
class RoutingTable {
 public:
  typedef std::vector<RoutingTableEntry> TableEntryVector;
  typedef std::unordered_map<int, TableEntryVector> Tables;

  struct Query {
    // Callback::Run(interface_index, entry)
    typedef base::Callback<void(int, const RoutingTableEntry&)> Callback;

    Query() : sequence(0), tag(0), table_id(0) {}
    Query(uint32_t sequence_in,
          int tag_in,
          Callback callback_in,
          uint8_t table_id_in)
        : sequence(sequence_in),
          tag(tag_in),
          callback(callback_in),
          table_id(table_id_in) {}

    uint32_t sequence;
    int tag;
    Callback callback;
    uint8_t table_id;
  };

  virtual ~RoutingTable();

  static RoutingTable* GetInstance();

  virtual void Start();
  virtual void Stop();

  // Add an entry to the routing table.
  virtual bool AddRoute(int interface_index, const RoutingTableEntry& entry);

  // Get the default route associated with an interface of a given addr family.
  // The route is copied into |*entry|.
  virtual bool GetDefaultRoute(int interface_index,
                               IPAddress::Family family,
                               RoutingTableEntry* entry);

  // Set the default route for an interface with index |interface_index|,
  // given the IPAddress of the gateway |gateway_address| and priority
  // |metric|.
  virtual bool SetDefaultRoute(int interface_index,
                               const IPAddress& gateway_address,
                               uint32_t metric,
                               uint8_t table_id);

  // Configure routing table entries from the "routes" portion of |ipconfig|.
  // Returns true if all routes were installed successfully, false otherwise.
  virtual bool ConfigureRoutes(int interface_index,
                               const IPConfigRefPtr& ipconfig,
                               uint32_t metric,
                               uint8_t table_id);

  // Create a blackhole route for a given IP family.  Returns true
  // on successfully sending the route request, false otherwise.
  virtual bool CreateBlackholeRoute(int interface_index,
                                    IPAddress::Family family,
                                    uint32_t metric,
                                    uint8_t table_id);

  // Create a route to a link-attached remote host.  |remote_address|
  // must be directly reachable from |local_address|.  Returns true
  // on successfully sending the route request, false otherwise.
  virtual bool CreateLinkRoute(int interface_index,
                               const IPAddress& local_address,
                               const IPAddress& remote_address,
                               uint8_t table_id);

  // Remove routes associated with interface.
  // Route entries are immediately purged from our copy of the routing table.
  virtual void FlushRoutes(int interface_index);

  // Iterate over all routing tables removing routes tagged with |tag|.
  // Route entries are immediately purged from our copy of the routing table.
  virtual void FlushRoutesWithTag(int tag);

  // Flush the routing cache for all interfaces.
  virtual bool FlushCache();

  // Reset local state for this interface.
  virtual void ResetTable(int interface_index);

  // Set the metric (priority) on existing default routes for an interface.
  virtual void SetDefaultMetric(int interface_index, uint32_t metric);

  // Get the default route to |destination| through |interface_index| and create
  // a host route to that destination.  When creating the route, tag our local
  // entry with |tag|, so we can remove it later.  Connections use their
  // interface index as the tag, so that as they are destroyed, they can remove
  // all their dependent routes.  If |callback| is not null, it will be invoked
  // when the request-route response is received and the add-route request has
  // been sent successfully.
  virtual bool RequestRouteToHost(const IPAddress& destination,
                                  int interface_index,
                                  int tag,
                                  const Query::Callback& callback,
                                  uint8_t table_id);

 protected:
  RoutingTable();

 private:
  friend struct base::DefaultLazyInstanceTraits<RoutingTable>;
  friend class RoutingTableTest;

  static bool ParseRoutingTableMessage(const RTNLMessage& message,
                                       int* interface_index,
                                       RoutingTableEntry* entry);
  void RouteMsgHandler(const RTNLMessage& msg);
  bool ApplyRoute(uint32_t interface_index,
                  const RoutingTableEntry& entry,
                  RTNLMessage::Mode mode,
                  unsigned int flags);
  // Get the default route associated with an interface of a given addr family.
  // A pointer to the route is placed in |*entry|.
  virtual bool GetDefaultRouteInternal(int interface_index,
                               IPAddress::Family family,
                               RoutingTableEntry** entry);

  void ReplaceMetric(uint32_t interface_index,
                     RoutingTableEntry* entry,
                     uint32_t metric);

  static const char kRouteFlushPath4[];
  static const char kRouteFlushPath6[];

  Tables tables_;

  base::Callback<void(const RTNLMessage&)> route_callback_;
  std::unique_ptr<RTNLListener> route_listener_;
  std::deque<Query> route_queries_;

  // Cache singleton pointer for performance and test purposes.
  RTNLHandler* rtnl_handler_;

  DISALLOW_COPY_AND_ASSIGN(RoutingTable);
};

}  // namespace shill

#endif  // SHILL_ROUTING_TABLE_H_
