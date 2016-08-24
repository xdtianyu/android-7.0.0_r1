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

#include "buffet/http_transport_client.h"

#include <base/bind.h>
#include <brillo/errors/error.h>
#include <brillo/http/http_request.h>
#include <brillo/http/http_utils.h>
#include <brillo/streams/memory_stream.h>
#include <weave/enum_to_string.h>

#include "buffet/weave_error_conversion.h"

namespace buffet {

namespace {

using weave::provider::HttpClient;

// The number of seconds each HTTP request will be allowed before timing out.
const int kRequestTimeoutSeconds = 30;

class ResponseImpl : public HttpClient::Response {
 public:
  ~ResponseImpl() override = default;
  explicit ResponseImpl(std::unique_ptr<brillo::http::Response> response)
      : response_{std::move(response)},
        data_{response_->ExtractDataAsString()} {}

  // HttpClient::Response implementation
  int GetStatusCode() const override { return response_->GetStatusCode(); }

  std::string GetContentType() const override {
    return response_->GetContentType();
  }

  std::string GetData() const override { return data_; }

 private:
  std::unique_ptr<brillo::http::Response> response_;
  std::string data_;
  DISALLOW_COPY_AND_ASSIGN(ResponseImpl);
};

void OnSuccessCallback(const HttpClient::SendRequestCallback& callback,
                       int id,
                       std::unique_ptr<brillo::http::Response> response) {
  callback.Run(std::unique_ptr<HttpClient::Response>{new ResponseImpl{
                   std::move(response)}},
               nullptr);
}

void OnErrorCallback(const HttpClient::SendRequestCallback& callback,
                     int id,
                     const brillo::Error* brillo_error) {
  weave::ErrorPtr error;
  ConvertError(*brillo_error, &error);
  callback.Run(nullptr, std::move(error));
}

}  // anonymous namespace

HttpTransportClient::HttpTransportClient()
    : transport_{brillo::http::Transport::CreateDefault()} {
  transport_->SetDefaultTimeout(
      base::TimeDelta::FromSeconds(kRequestTimeoutSeconds));
}

HttpTransportClient::~HttpTransportClient() {}

void HttpTransportClient::SendRequest(Method method,
                                      const std::string& url,
                                      const Headers& headers,
                                      const std::string& data,
                                      const SendRequestCallback& callback) {
  brillo::http::Request request(url, weave::EnumToString(method), transport_);
  request.AddHeaders(headers);
  if (!data.empty()) {
    auto stream = brillo::MemoryStream::OpenCopyOf(data, nullptr);
    CHECK(stream->GetRemainingSize());
    brillo::ErrorPtr cromeos_error;
    if (!request.AddRequestBody(std::move(stream), &cromeos_error)) {
      weave::ErrorPtr error;
      ConvertError(*cromeos_error, &error);
      transport_->RunCallbackAsync(
          FROM_HERE, base::Bind(callback, nullptr, base::Passed(&error)));
      return;
    }
  }
  request.GetResponse(base::Bind(&OnSuccessCallback, callback),
                      base::Bind(&OnErrorCallback, callback));
}

}  // namespace buffet
