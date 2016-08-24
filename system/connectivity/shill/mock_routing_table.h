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

#ifndef SHILL_MOCK_ROUTING_TABLE_H_
#define SHILL_MOCK_ROUTING_TABLE_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/routing_table.h"

namespace shill {

class MockRoutingTable : public RoutingTable {
 public:
  MockRoutingTable();
  ~MockRoutingTable() override;

  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD2(AddRoute, bool(int interface_index,
                              const RoutingTableEntry& entry));
  MOCK_METHOD3(GetDefaultRoute, bool(int interface_index,
                                     IPAddress::Family family,
                                     RoutingTableEntry* entry));
  MOCK_METHOD4(SetDefaultRoute, bool(int interface_index,
                                     const IPAddress& gateway_address,
                                     uint32_t metric,
                                     uint8_t table));
  MOCK_METHOD4(ConfigureRoutes, bool(int interface_index,
                                     const IPConfigRefPtr& ipconfig,
                                     uint32_t metric,
                                     uint8_t table));
  MOCK_METHOD4(CreateBlackholeRoute, bool(int interface_index,
                                          IPAddress::Family family,
                                          uint32_t metric,
                                          uint8_t table));
  MOCK_METHOD4(CreateLinkRoute, bool(int interface_index,
                                     const IPAddress& local_address,
                                     const IPAddress& remote_address,
                                     uint8_t table));
  MOCK_METHOD1(FlushRoutes, void(int interface_index));
  MOCK_METHOD1(FlushRoutesWithTag, void(int tag));
  MOCK_METHOD0(FlushCache, bool());
  MOCK_METHOD1(ResetTable, void(int interface_index));
  MOCK_METHOD2(SetDefaultMetric, void(int interface_index, uint32_t metric));
  MOCK_METHOD5(RequestRouteToHost, bool(const IPAddress& addresss,
                                        int interface_index,
                                        int tag,
                                        const Query::Callback& callback,
                                        uint8_t table));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockRoutingTable);
};

}  // namespace shill

#endif  // SHILL_MOCK_ROUTING_TABLE_H_
