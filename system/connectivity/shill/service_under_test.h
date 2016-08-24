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

#ifndef SHILL_SERVICE_UNDER_TEST_H_
#define SHILL_SERVICE_UNDER_TEST_H_

#include <string>
#include <vector>

#include "shill/key_value_store.h"
#include "shill/service.h"

namespace shill {

class ControlInterface;
class Error;
class EventDispatcher;
class Manager;
class Metrics;

// This is a simple Service subclass with all the pure-virtual methods stubbed.
class ServiceUnderTest : public Service {
 public:
  static const char kKeyValueStoreProperty[];
  static const char kRpcId[];
  static const char kStringsProperty[];
  static const char kStorageId[];

  ServiceUnderTest(ControlInterface* control_interface,
                   EventDispatcher* dispatcher,
                   Metrics* metrics,
                   Manager* manager);
  ~ServiceUnderTest() override;

  std::string GetRpcIdentifier() const override;
  std::string GetDeviceRpcId(Error* error) const override;
  std::string GetStorageIdentifier() const override;

  // Getter and setter for a string array property for use in testing.
  void set_strings(const std::vector<std::string>& strings) {
    strings_ = strings;
  }
  const std::vector<std::string>& strings() const { return strings_; }

  // Getter and setter for a KeyValueStore property for use in testing.
  bool SetKeyValueStore(const KeyValueStore& value, Error* error);
  KeyValueStore GetKeyValueStore(Error* error);

 private:
  // The Service superclass has no string array or KeyValueStore properties
  // but we need them in order to test Service::Configure.
  std::vector<std::string> strings_;
  KeyValueStore key_value_store_;

  DISALLOW_COPY_AND_ASSIGN(ServiceUnderTest);
};

}  // namespace shill

#endif  // SHILL_SERVICE_UNDER_TEST_H_
