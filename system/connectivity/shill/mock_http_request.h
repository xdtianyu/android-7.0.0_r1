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

#ifndef SHILL_MOCK_HTTP_REQUEST_H_
#define SHILL_MOCK_HTTP_REQUEST_H_

#include <base/macros.h>
#include <gmock/gmock.h>

#include "shill/http_request.h"
#include "shill/http_url.h"  // MOCK_METHOD3() call below needs sizeof(HTTPURL).

namespace shill {

class MockHTTPRequest : public HTTPRequest {
 public:
  explicit MockHTTPRequest(ConnectionRefPtr connection);
  ~MockHTTPRequest() override;

  MOCK_METHOD3(Start, HTTPRequest::Result(
      const HTTPURL& url,
      const base::Callback<void(const ByteString&)>& read_event_callback,
      const base::Callback<void(Result, const ByteString&)>& result_callback));
  MOCK_METHOD0(Stop, void());
  MOCK_CONST_METHOD0(response_data, const ByteString& ());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockHTTPRequest);
};

}  // namespace shill

#endif  // SHILL_MOCK_HTTP_REQUEST_H_
