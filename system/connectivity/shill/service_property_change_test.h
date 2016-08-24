//
// Copyright (C) 2013 The Android Open Source Project
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

#ifndef SHILL_SERVICE_PROPERTY_CHANGE_TEST_H_
#define SHILL_SERVICE_PROPERTY_CHANGE_TEST_H_

#include "shill/refptr_types.h"

namespace shill {

class MockManager;
class ServiceMockAdaptor;

// Test property change notifications that are implemented by all
// Services.
void TestCommonPropertyChanges(ServiceRefPtr service,
                               ServiceMockAdaptor* adaptor);
// Test AutoConnect property change notification. Implemented by
// all Services except EthernetService.
void TestAutoConnectPropertyChange(ServiceRefPtr service,
                                   ServiceMockAdaptor* adaptor);
// Test Name property change notification. Only VPNService allows
// changing the name property.
void TestNamePropertyChange(ServiceRefPtr service,
                           ServiceMockAdaptor* adaptor);
// Test that the common customer setters (for all Services) return
// false if setting to the same as the current value.
void TestCommonCustomSetterNoopChange(ServiceRefPtr service,
                                      MockManager* mock_manager);
}  // namespace shill

#endif  // SHILL_SERVICE_PROPERTY_CHANGE_TEST_H_
