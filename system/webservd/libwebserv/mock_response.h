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

#ifndef WEBSERVER_LIBWEBSERV_MOCK_RESPONSE_H_
#define WEBSERVER_LIBWEBSERV_MOCK_RESPONSE_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include <libwebserv/response.h>

namespace libwebserv {

// Mock Response implementation for testing.
class MockResponse : public Response {
 public:
  MockResponse() = default;

  MOCK_METHOD2(AddHeader, void(const std::string&, const std::string&));
  MOCK_METHOD1(AddHeaders,
               void(const std::vector<std::pair<std::string, std::string>>&));

  // Workaround for mocking with move-only StreamPtr.
  MOCK_METHOD3(MockReply, void(int, brillo::Stream*, const std::string&));

  MOCK_METHOD3(ReplyWithText,
               void(int, const std::string&, const std::string&));
  MOCK_METHOD2(ReplyWithJson, void(int, const base::Value*));
  MOCK_METHOD2(ReplyWithJson,
               void(int, const std::map<std::string, std::string>&));
  MOCK_METHOD2(Redirect, void(int, const std::string&));
  MOCK_METHOD2(ReplyWithError, void(int, const std::string&));
  MOCK_METHOD0(ReplyWithErrorNotFound, void());

 private:
  void Reply(int status_code,
             brillo::StreamPtr data_stream,
             const std::string &mime_type) override {
    return MockReply(status_code, data_stream.get(), mime_type);
  }

  DISALLOW_COPY_AND_ASSIGN(MockResponse);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_MOCK_RESPONSE_H_
