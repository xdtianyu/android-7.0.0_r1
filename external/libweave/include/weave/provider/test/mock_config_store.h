// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_CONFIG_STORE_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_CONFIG_STORE_H_

#include <map>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <weave/provider/config_store.h>

namespace weave {
namespace provider {
namespace test {

class MockConfigStore : public ConfigStore {
 public:
  explicit MockConfigStore(bool set_expectations = true) {
    using testing::_;
    using testing::Return;

    if (!set_expectations)
      return;

    EXPECT_CALL(*this, LoadDefaults(_))
        .WillRepeatedly(testing::Invoke([](Settings* settings) {
          settings->firmware_version = "TEST_FIRMWARE";
          settings->oem_name = "TEST_OEM";
          settings->model_name = "TEST_MODEL";
          settings->model_id = "ABCDE";
          settings->name = "TEST_NAME";
          settings->client_id = "TEST_CLIENT_ID";
          settings->client_secret = "TEST_CLIENT_SECRET";
          settings->api_key = "TEST_API_KEY";
          return true;
        }));
    EXPECT_CALL(*this, LoadSettings())
        .WillRepeatedly(Return(R"({
          "version": 1,
          "device_id": "TEST_DEVICE_ID"
        })"));
    EXPECT_CALL(*this, LoadSettings(_)).WillRepeatedly(Return(""));
    EXPECT_CALL(*this, SaveSettings(_, _, _))
        .WillRepeatedly(testing::WithArgs<1, 2>(testing::Invoke(
            [](const std::string& json, const DoneCallback& callback) {
              if (!callback.is_null())
                callback.Run(nullptr);
            })));
  }
  MOCK_METHOD1(LoadDefaults, bool(Settings*));
  MOCK_METHOD1(LoadSettings, std::string(const std::string&));
  MOCK_METHOD3(SaveSettings,
               void(const std::string&,
                    const std::string&,
                    const DoneCallback&));
  MOCK_METHOD0(LoadSettings, std::string());
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_CONFIG_STORE_H_
