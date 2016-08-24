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

#include "shill/dbus/chromeos_dbus_adaptor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "shill/property_store_unittest.h"

namespace shill {

class ChromeosDBusAdaptorTest : public PropertyStoreTest {
 public:
  ChromeosDBusAdaptorTest() {}

  virtual ~ChromeosDBusAdaptorTest() {}
};

TEST_F(ChromeosDBusAdaptorTest, SanitizePathElement) {
  EXPECT_EQ("0Ab_y_Z_9_",
            ChromeosDBusAdaptor::SanitizePathElement("0Ab/y:Z`9{"));
  EXPECT_EQ("aB_f_0_Y_z",
            ChromeosDBusAdaptor::SanitizePathElement("aB-f/0@Y[z"));
}

}  // namespace shill
