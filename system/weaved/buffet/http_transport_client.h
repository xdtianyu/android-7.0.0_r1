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

#ifndef BUFFET_HTTP_TRANSPORT_CLIENT_H_
#define BUFFET_HTTP_TRANSPORT_CLIENT_H_

#include <memory>
#include <string>

#include <weave/provider/http_client.h>

namespace brillo {
namespace http {
class Transport;
}
}

namespace buffet {

class HttpTransportClient : public weave::provider::HttpClient {
 public:
  HttpTransportClient();

  ~HttpTransportClient() override;

  void SendRequest(Method method,
                   const std::string& url,
                   const Headers& headers,
                   const std::string& data,
                   const SendRequestCallback& callback) override;

 private:
  std::shared_ptr<brillo::http::Transport> transport_;
  DISALLOW_COPY_AND_ASSIGN(HttpTransportClient);
};

}  // namespace buffet

#endif  // BUFFET_HTTP_TRANSPORT_CLIENT_H_
