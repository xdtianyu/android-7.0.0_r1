//
// Copyright (C) 2011 The Android Open Source Project
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

#ifndef SHILL_ROUTING_TABLE_ENTRY_H_
#define SHILL_ROUTING_TABLE_ENTRY_H_

#include <linux/rtnetlink.h>

#include "shill/net/ip_address.h"

namespace shill {

// Holds table entries for routing.  These are held in an STL vector
// in the RoutingTable object, hence the need for copy contructor and
// operator=.
struct RoutingTableEntry {
 public:
  static const int kDefaultTag = -1;

  RoutingTableEntry()
      : dst(IPAddress::kFamilyUnknown),
        src(IPAddress::kFamilyUnknown),
        gateway(IPAddress::kFamilyUnknown),
        metric(0),
        scope(0),
        from_rtnl(false),
        table(RT_TABLE_MAIN),
        tag(kDefaultTag) {}

  RoutingTableEntry(const IPAddress& dst_in,
                    const IPAddress& src_in,
                    const IPAddress& gateway_in,
                    uint32_t metric_in,
                    unsigned char scope_in,
                    bool from_rtnl_in)
      : dst(dst_in),
        src(src_in),
        gateway(gateway_in),
        metric(metric_in),
        scope(scope_in),
        from_rtnl(from_rtnl_in),
        table(RT_TABLE_MAIN),
        tag(kDefaultTag) {}

  RoutingTableEntry(const IPAddress& dst_in,
                    const IPAddress& src_in,
                    const IPAddress& gateway_in,
                    uint32_t metric_in,
                    unsigned char scope_in,
                    bool from_rtnl_in,
                    int tag_in)
      : dst(dst_in),
        src(src_in),
        gateway(gateway_in),
        metric(metric_in),
        scope(scope_in),
        from_rtnl(from_rtnl_in),
        table(RT_TABLE_MAIN),
        tag(tag_in) {}

  RoutingTableEntry(const IPAddress& dst_in,
                    const IPAddress& src_in,
                    const IPAddress& gateway_in,
                    uint32_t metric_in,
                    unsigned char scope_in,
                    bool from_rtnl_in,
                    unsigned char table_in,
                    int tag_in)
      : dst(dst_in),
        src(src_in),
        gateway(gateway_in),
        metric(metric_in),
        scope(scope_in),
        from_rtnl(from_rtnl_in),
        table(table_in),
        tag(tag_in) {}

  RoutingTableEntry(const RoutingTableEntry& b)
      : dst(b.dst),
        src(b.src),
        gateway(b.gateway),
        metric(b.metric),
        scope(b.scope),
        from_rtnl(b.from_rtnl),
        table(b.table),
        tag(b.tag) {}

  RoutingTableEntry& operator=(const RoutingTableEntry& b) {
    dst = b.dst;
    src = b.src;
    gateway = b.gateway;
    metric = b.metric;
    scope = b.scope;
    from_rtnl = b.from_rtnl;
    table = b.table;
    tag = b.tag;

    return *this;
  }

  ~RoutingTableEntry() {}

  bool Equals(const RoutingTableEntry& b) {
    return (dst.Equals(b.dst) &&
            src.Equals(b.src) &&
            gateway.Equals(b.gateway) &&
            metric == b.metric &&
            scope == b.scope &&
            from_rtnl == b.from_rtnl &&
            table == b.table &&
            tag == b.tag);
  }

  IPAddress dst;
  IPAddress src;
  IPAddress gateway;
  uint32_t metric;
  unsigned char scope;
  bool from_rtnl;
  unsigned char table;
  int tag;
};

}  // namespace shill


#endif  // SHILL_ROUTING_TABLE_ENTRY_H_
