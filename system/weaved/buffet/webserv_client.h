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

#ifndef BUFFET_WEBSERV_CLIENT_H_
#define BUFFET_WEBSERV_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <weave/provider/http_server.h>

namespace dbus {
class Bus;
}

namespace brillo {
namespace dbus_utils {
class AsyncEventSequencer;
}
}

namespace libwebserv {
class ProtocolHandler;
class Request;
class Response;
class Server;
}

namespace buffet {

// Wrapper around libwebserv that implements HttpServer interface.
class WebServClient : public weave::provider::HttpServer {
 public:
  WebServClient(const scoped_refptr<dbus::Bus>& bus,
                brillo::dbus_utils::AsyncEventSequencer* sequencer,
                const base::Closure& server_available_callback);
  ~WebServClient() override;

  // HttpServer implementation.
  void AddHttpRequestHandler(const std::string& path,
                             const RequestHandlerCallback& callback) override;
  void AddHttpsRequestHandler(const std::string& path,
                              const RequestHandlerCallback& callback) override;

  uint16_t GetHttpPort() const override;
  uint16_t GetHttpsPort() const override;
  base::TimeDelta GetRequestTimeout() const override;
  std::vector<uint8_t> GetHttpsCertificateFingerprint() const override;

 private:
  void OnRequest(const RequestHandlerCallback& callback,
                 std::unique_ptr<libwebserv::Request> request,
                 std::unique_ptr<libwebserv::Response> response);

  void OnProtocolHandlerConnected(
      libwebserv::ProtocolHandler* protocol_handler);

  void OnProtocolHandlerDisconnected(
      libwebserv::ProtocolHandler* protocol_handler);

  uint16_t http_port_{0};
  uint16_t https_port_{0};
  std::vector<uint8_t> certificate_;

  std::unique_ptr<libwebserv::Server> web_server_;
  base::Closure server_available_callback_;

  base::WeakPtrFactory<WebServClient> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(WebServClient);
};

}  // namespace buffet

#endif  // BUFFET_WEBSERV_CLIENT_H_
