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

#include "buffet/webserv_client.h"

#include <memory>
#include <string>

#include <libwebserv/protocol_handler.h>
#include <libwebserv/request.h>
#include <libwebserv/response.h>
#include <libwebserv/server.h>

#include "buffet/dbus_constants.h"
#include "buffet/socket_stream.h"

namespace buffet {

namespace {

using weave::provider::HttpServer;

class RequestImpl : public HttpServer::Request {
 public:
  explicit RequestImpl(std::unique_ptr<libwebserv::Request> request,
                       std::unique_ptr<libwebserv::Response> response)
      : request_{std::move(request)}, response_{std::move(response)} {}
  ~RequestImpl() override {}

  // HttpServer::Request implementation.
  std::string GetPath() const override { return request_->GetPath(); }

  std::string GetFirstHeader(const std::string& name) const override {
    return request_->GetFirstHeader(name);
  }

  // TODO(avakulenko): Remove this method and rewrite all call sites in libweave
  // to use GetDataStream() instead.
  std::string GetData() override {
    if (request_data_)
      return *request_data_;

    request_data_.reset(new std::string);
    auto stream = request_->GetDataStream();
    if (stream) {
      if (stream->CanGetSize())
        request_data_->reserve(stream->GetRemainingSize());
      std::vector<char> buffer(16 * 1024);  // 16K seems to be good enough.
      size_t sz = 0;
      while (stream->ReadBlocking(buffer.data(), buffer.size(), &sz, nullptr) &&
             sz > 0) {
        request_data_->append(buffer.data(), buffer.data() + sz);
      }
    }
    return *request_data_;
  }

  void SendReply(int status_code,
                 const std::string& data,
                 const std::string& mime_type) override {
    response_->ReplyWithText(status_code, data, mime_type);
  }

  std::unique_ptr<weave::Stream> GetDataStream() const {
    auto stream = std::unique_ptr<weave::Stream>{
        new SocketStream{request_->GetDataStream()}};
    return stream;
  }

 private:
  std::unique_ptr<libwebserv::Request> request_;
  std::unique_ptr<libwebserv::Response> response_;
  mutable std::unique_ptr<std::string> request_data_;

  DISALLOW_COPY_AND_ASSIGN(RequestImpl);
};

}  // namespace

WebServClient::WebServClient(
    const scoped_refptr<dbus::Bus>& bus,
    brillo::dbus_utils::AsyncEventSequencer* sequencer,
    const base::Closure& server_available_callback)
    : server_available_callback_{server_available_callback} {
  web_server_ = libwebserv::Server::ConnectToServerViaDBus(
      bus, buffet::dbus_constants::kServiceName,
      sequencer->GetHandler("Server::Connect failed.", true),
      base::Bind(&base::DoNothing),
      base::Bind(&base::DoNothing));
  web_server_->OnProtocolHandlerConnected(
      base::Bind(&WebServClient::OnProtocolHandlerConnected,
                 weak_ptr_factory_.GetWeakPtr()));
  web_server_->OnProtocolHandlerDisconnected(
      base::Bind(&WebServClient::OnProtocolHandlerDisconnected,
                 weak_ptr_factory_.GetWeakPtr()));
}

WebServClient::~WebServClient() {}

void WebServClient::AddHttpRequestHandler(
    const std::string& path,
    const RequestHandlerCallback& callback) {
  web_server_->GetDefaultHttpHandler()->AddHandlerCallback(
      path, "", base::Bind(&WebServClient::OnRequest,
                           weak_ptr_factory_.GetWeakPtr(), callback));
}

void WebServClient::AddHttpsRequestHandler(
    const std::string& path,
    const RequestHandlerCallback& callback) {
  web_server_->GetDefaultHttpsHandler()->AddHandlerCallback(
      path, "", base::Bind(&WebServClient::OnRequest,
                           weak_ptr_factory_.GetWeakPtr(), callback));
}

uint16_t WebServClient::GetHttpPort() const {
  return http_port_;
}

uint16_t WebServClient::GetHttpsPort() const {
  return https_port_;
}

base::TimeDelta WebServClient::GetRequestTimeout() const {
  return web_server_->GetDefaultRequestTimeout();
}

brillo::Blob WebServClient::GetHttpsCertificateFingerprint() const {
  return certificate_;
}

void WebServClient::OnRequest(const RequestHandlerCallback& callback,
                              std::unique_ptr<libwebserv::Request> request,
                              std::unique_ptr<libwebserv::Response> response) {
  std::unique_ptr<Request> weave_request{
      new RequestImpl{std::move(request), std::move(response)}};
  callback.Run(std::move(weave_request));
}

void WebServClient::OnProtocolHandlerConnected(
    libwebserv::ProtocolHandler* protocol_handler) {
  if (protocol_handler->GetName() == libwebserv::ProtocolHandler::kHttp) {
    http_port_ = *protocol_handler->GetPorts().begin();
  } else if (protocol_handler->GetName() ==
             libwebserv::ProtocolHandler::kHttps) {
    https_port_ = *protocol_handler->GetPorts().begin();
    certificate_ = protocol_handler->GetCertificateFingerprint();
  }
  if (https_port_ && https_port_)
    server_available_callback_.Run();
}

void WebServClient::OnProtocolHandlerDisconnected(
    libwebserv::ProtocolHandler* protocol_handler) {
  if (protocol_handler->GetName() == libwebserv::ProtocolHandler::kHttp) {
    http_port_ = 0;
  } else if (protocol_handler->GetName() ==
             libwebserv::ProtocolHandler::kHttps) {
    https_port_ = 0;
    certificate_.clear();
  }
}

}  // namespace buffet
