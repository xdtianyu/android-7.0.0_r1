// Copyright 2015 The Android Open Source Project
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

#include "webservd/config.h"

#include <base/files/file_util.h>
#include <base/files/scoped_temp_dir.h>
#include <base/strings/string_util.h>
#include <brillo/errors/error_codes.h>
#include <gtest/gtest.h>

#include "webservd/error_codes.h"
#include "webservd/protocol_handler.h"

namespace {

const char kTestConfig[] = R"({
  "protocol_handlers": [
    {
      "name": "ue_p2p",
      "port": 16725,
      "dummy_data_to_ignore": 123,
    },
  ],
  "dummy_data_to_ignore2": "ignore me",
  "log_directory": "/var/log/mylogs",
})";

const char kMultipleHandlers[] = R"({
  "protocol_handlers": [
    {
      "name": "http",
      "port": 80
    },
    {
      "name": "http",
      "port": 8080
    }
  ]
})";

const char kInvalidConfig_NotDict[] = R"({
  "protocol_handlers": [
    "not_a_dict"
  ]
})";

const char kInvalidConfig_NoName[] = R"({
  "protocol_handlers": [
    {
      "port": 80,
      "use_tls": true
    }
  ]
})";

const char kInvalidConfig_NoPort[] = R"({
  "protocol_handlers": [
    {
      "name": "http",
      "use_tls": true
    }
  ]
})";

const char kInvalidConfig_InvalidPort[] = R"({
  "protocol_handlers": [
    {
      "name": "https",
      "port": 65536
    }
  ]
})";

void ValidateConfig(const webservd::Config& config) {
  EXPECT_FALSE(config.use_debug);
  EXPECT_EQ("/var/log/mylogs", config.log_directory);

  ASSERT_EQ(1u, config.protocol_handlers.size());

  auto it = config.protocol_handlers.begin();
  EXPECT_EQ("ue_p2p", it->name);
  EXPECT_EQ(16725u, it->port);
  EXPECT_FALSE(it->use_tls);
  EXPECT_TRUE(it->certificate.empty());
  EXPECT_TRUE(it->certificate_fingerprint.empty());
  EXPECT_TRUE(it->private_key.empty());
}

}  // anonymous namespace

namespace webservd {

TEST(Config, LoadDefault) {
  Config config;
  LoadDefaultConfig(&config);
  EXPECT_FALSE(config.use_debug);
  EXPECT_EQ("/var/log/webservd", config.log_directory);

  ASSERT_EQ(2u, config.protocol_handlers.size());

  for (const auto& handler_config : config.protocol_handlers) {
    if (handler_config.name == "http") {
      EXPECT_EQ(80u, handler_config.port);
      EXPECT_FALSE(handler_config.use_tls);
      EXPECT_TRUE(handler_config.certificate.empty());
      EXPECT_TRUE(handler_config.certificate_fingerprint.empty());
      EXPECT_TRUE(handler_config.private_key.empty());
    } else if (handler_config.name == "https") {
      EXPECT_EQ(443u, handler_config.port);
      EXPECT_TRUE(handler_config.use_tls);

      // TLS keys/certificates are set later in webservd::Server, not on load.
      EXPECT_TRUE(handler_config.certificate.empty());
      EXPECT_TRUE(handler_config.certificate_fingerprint.empty());
      EXPECT_TRUE(handler_config.private_key.empty());
    } else {
      FAIL() << "Unexpected handler: " << handler_config.name;
    }
  }
}

TEST(Config, LoadConfigFromString) {
  Config config;
  ASSERT_TRUE(LoadConfigFromString(kTestConfig, &config, nullptr));
  ValidateConfig(config);
}

TEST(Config, LoadConfigFromFile) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());
  base::FilePath config_path{temp.path().Append("test.config")};
  // For the record: I hate base::WriteFile() and its usage of ints.
  int data_len = sizeof(kTestConfig) - 1;
  ASSERT_EQ(data_len, base::WriteFile(config_path, kTestConfig, data_len));

  Config config;
  LoadConfigFromFile(config_path, &config);
  ValidateConfig(config);
}

TEST(Config, MultipleHandlers) {
  Config config;
  ASSERT_TRUE(LoadConfigFromString(kMultipleHandlers, &config, nullptr));
  ASSERT_EQ(2u, config.protocol_handlers.size());

  auto it = config.protocol_handlers.begin();
  EXPECT_EQ("http", it->name);
  EXPECT_EQ(80, it->port);
  ++it;
  EXPECT_EQ("http", it->name);
  EXPECT_EQ(8080, it->port);
}

TEST(Config, ParseError_ProtocolHandlersNotDict) {
  brillo::ErrorPtr error;
  Config config;
  ASSERT_FALSE(LoadConfigFromString(kInvalidConfig_NotDict, &config, &error));
  EXPECT_EQ(brillo::errors::json::kDomain, error->GetDomain());
  EXPECT_EQ(brillo::errors::json::kObjectExpected, error->GetCode());
  EXPECT_EQ("Protocol handler definition must be a JSON object",
            error->GetMessage());
}

TEST(Config, ParseError_NoName) {
  brillo::ErrorPtr error;
  Config config;
  ASSERT_FALSE(LoadConfigFromString(kInvalidConfig_NoName, &config, &error));
  EXPECT_EQ(webservd::errors::kDomain, error->GetDomain());
  EXPECT_EQ(webservd::errors::kInvalidConfig, error->GetCode());
  EXPECT_EQ("Protocol handler definition must include its name",
            error->GetMessage());
}

TEST(Config, ParseError_NoPort) {
  brillo::ErrorPtr error;
  Config config;
  ASSERT_FALSE(LoadConfigFromString(kInvalidConfig_NoPort, &config, &error));
  EXPECT_EQ(webservd::errors::kDomain, error->GetDomain());
  EXPECT_EQ(webservd::errors::kInvalidConfig, error->GetCode());
  EXPECT_EQ("Unable to parse config for protocol handler 'http'",
            error->GetMessage());
  EXPECT_EQ(webservd::errors::kDomain, error->GetInnerError()->GetDomain());
  EXPECT_EQ(webservd::errors::kInvalidConfig,
            error->GetInnerError()->GetCode());
  EXPECT_EQ("Port is missing", error->GetInnerError()->GetMessage());
}

TEST(Config, ParseError_InvalidPort) {
  brillo::ErrorPtr error;
  Config config;
  ASSERT_FALSE(LoadConfigFromString(kInvalidConfig_InvalidPort, &config,
               &error));
  EXPECT_EQ(webservd::errors::kDomain, error->GetDomain());
  EXPECT_EQ(webservd::errors::kInvalidConfig, error->GetCode());
  EXPECT_EQ("Unable to parse config for protocol handler 'https'",
            error->GetMessage());
  EXPECT_EQ(webservd::errors::kDomain, error->GetInnerError()->GetDomain());
  EXPECT_EQ(webservd::errors::kInvalidConfig,
            error->GetInnerError()->GetCode());
  EXPECT_EQ("Invalid port value: 65536", error->GetInnerError()->GetMessage());
}

}  // namespace webservd
