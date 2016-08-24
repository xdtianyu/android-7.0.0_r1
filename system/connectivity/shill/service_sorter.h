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

#ifndef SHILL_SERVICE_SORTER_H_
#define SHILL_SERVICE_SORTER_H_

#include <vector>

#include "shill/refptr_types.h"
#include "shill/service.h"

namespace shill {

class Manager;

// This is a closure used by the Manager for STL sorting of the
// Service array.  We pass instances of this object to STL sort(),
// which in turn will call the selected function in the Manager to
// compare two Service objects at a time.
class ServiceSorter {
 public:
  ServiceSorter(Manager* manager,
                bool compare_connectivity_state,
                const std::vector<Technology::Identifier>& tech_order)
      : manager_(manager),
        compare_connectivity_state_(compare_connectivity_state),
        technology_order_(tech_order) {}
  bool operator() (ServiceRefPtr a, ServiceRefPtr b) {
    const char* reason;
    return Service::Compare(manager_, a, b, compare_connectivity_state_,
                            technology_order_, &reason);
  }

 private:
  Manager* manager_;
  const bool compare_connectivity_state_;
  const std::vector<Technology::Identifier>& technology_order_;
  // We can't DISALLOW_COPY_AND_ASSIGN since this is passed by value to STL
  // sort.
};

}  // namespace shill

#endif  // SHILL_SERVICE_SORTER_H_
