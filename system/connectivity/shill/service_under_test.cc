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

#include "shill/service_under_test.h"

#include <string>

#include "shill/mock_adaptors.h"
#include "shill/property_accessor.h"

using std::string;

namespace shill {

// static
const char ServiceUnderTest::kKeyValueStoreProperty[] = "key_value_store";
const char ServiceUnderTest::kRpcId[] = "/mock_device_rpc";
const char ServiceUnderTest::kStringsProperty[] = "strings";
const char ServiceUnderTest::kStorageId[] = "service";

ServiceUnderTest::ServiceUnderTest(ControlInterface* control_interface,
                                   EventDispatcher* dispatcher,
                                   Metrics* metrics,
                                   Manager* manager)
    : Service(control_interface, dispatcher, metrics, manager,
              Technology::kUnknown) {
  mutable_store()->RegisterStrings(kStringsProperty, &strings_);
  mutable_store()->RegisterDerivedKeyValueStore(
      kKeyValueStoreProperty,
      KeyValueStoreAccessor(
          new CustomAccessor<ServiceUnderTest, KeyValueStore>(
              this, &ServiceUnderTest::GetKeyValueStore,
              &ServiceUnderTest::SetKeyValueStore)));
}

ServiceUnderTest::~ServiceUnderTest() {}

string ServiceUnderTest::GetRpcIdentifier() const {
  return ServiceMockAdaptor::kRpcId;
}

string ServiceUnderTest::GetDeviceRpcId(Error* /*error*/) const {
  return kRpcId;
}

string ServiceUnderTest::GetStorageIdentifier() const { return kStorageId; }

bool ServiceUnderTest::SetKeyValueStore(
    const KeyValueStore& value, Error* error) {
  key_value_store_.Clear();
  key_value_store_.CopyFrom(value);
  return true;
}

KeyValueStore ServiceUnderTest::GetKeyValueStore(Error* error) {
  return key_value_store_;
}

}  // namespace shill
