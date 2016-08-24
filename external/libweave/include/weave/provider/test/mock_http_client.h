// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_CLIENT_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_CLIENT_H_

#include <weave/provider/http_client.h>

#include <memory>
#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace provider {
namespace test {

class MockHttpClientResponse : public HttpClient::Response {
 public:
  MOCK_CONST_METHOD0(GetStatusCode, int());
  MOCK_CONST_METHOD0(GetContentType, std::string());
  MOCK_CONST_METHOD0(GetData, std::string());
};

class MockHttpClient : public HttpClient {
 public:
  ~MockHttpClient() override = default;

  MOCK_METHOD5(SendRequest,
               void(Method,
                    const std::string&,
                    const Headers&,
                    const std::string&,
                    const SendRequestCallback&));
};

}  // namespace test
}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_TEST_MOCK_HTTP_CLIENT_H_
