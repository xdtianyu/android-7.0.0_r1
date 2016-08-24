// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_SERVER_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_SERVER_H_

#include <evhtp.h>
#include <openssl/ssl.h>

#include <map>
#include <string>
#include <vector>

#include <base/memory/weak_ptr.h>
#include <weave/provider/http_server.h>

#include "examples/provider/event_deleter.h"

namespace weave {
namespace examples {

class EventTaskRunner;

// HTTP/HTTPS server implemented with libevhtp.
class HttpServerImpl : public provider::HttpServer {
 public:
  class RequestImpl;

  explicit HttpServerImpl(EventTaskRunner* task_runner);

  void AddHttpRequestHandler(const std::string& path_prefix,
                             const RequestHandlerCallback& callback) override;
  void AddHttpsRequestHandler(const std::string& path_prefix,
                              const RequestHandlerCallback& callback) override;
  uint16_t GetHttpPort() const override;
  uint16_t GetHttpsPort() const override;
  base::TimeDelta GetRequestTimeout() const override;
  std::vector<uint8_t> GetHttpsCertificateFingerprint() const override;

 private:
  void GenerateX509(X509* x509, EVP_PKEY* pkey);
  static void ProcessRequestCallback(evhtp_request_t* req, void* arg);
  void ProcessRequest(evhtp_request_t* req);
  void ProcessReply(std::shared_ptr<RequestImpl> request,
                    int status_code,
                    const std::string& data,
                    const std::string& mime_type);
  void NotFound(evhtp_request_t* req);

  std::map<std::string, RequestHandlerCallback> handlers_;

  std::vector<uint8_t> cert_fingerprint_;
  EventTaskRunner* task_runner_{nullptr};
  EventPtr<evhtp_t> httpd_;
  EventPtr<evhtp_t> httpsd_;

  base::WeakPtrFactory<HttpServerImpl> weak_ptr_factory_{this};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_EVENT_HTTP_SERVER_H_
