// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/provider/event_http_server.h"

#include <vector>

#include <base/bind.h>
#include <base/time/time.h>
#include <event2/bufferevent_ssl.h>
#include <evhtp.h>
#include <openssl/err.h>

#include "examples/provider/event_task_runner.h"

namespace weave {
namespace examples {

namespace {

std::string GetSslError() {
  char error[1000] = {};
  ERR_error_string_n(ERR_get_error(), error, sizeof(error));
  return error;
}

}  // namespace

class HttpServerImpl::RequestImpl : public Request {
 public:
  RequestImpl(EventPtr<evhtp_request_t> req) : req_(std::move(req)) {
    evbuf_t* input_buffer =
        bufferevent_get_input(evhtp_request_get_bev(req_.get()));
    data_.resize(evbuffer_get_length(input_buffer));
    evbuffer_remove(input_buffer, &data_[0], data_.size());
  }

  ~RequestImpl() {}

  std::string GetPath() const override { return req_->uri->path->full; }

  std::string GetFirstHeader(const std::string& name) const override {
    const char* header = evhtp_header_find(req_->headers_in, name.c_str());
    if (!header)
      return {};
    return header;
  }

  std::string GetData() { return data_; }

  void SendReply(int status_code,
                 const std::string& data,
                 const std::string& mime_type) override {
    EventPtr<evbuffer> buf{evbuffer_new()};
    evbuffer_add(buf.get(), data.data(), data.size());
    evhtp_header_key_add(req_->headers_out, "Content-Type", 0);
    evhtp_header_val_add(req_->headers_out, mime_type.c_str(), 1);
    evhtp_send_reply_start(req_.get(), status_code);
    evhtp_send_reply_body(req_.get(), buf.get());
    evhtp_send_reply_end(req_.get());
  }

 private:
  EventPtr<evhtp_request_t> req_;
  std::string data_;
};

HttpServerImpl::HttpServerImpl(EventTaskRunner* task_runner)
    : task_runner_{task_runner} {
  SSL_load_error_strings();
  SSL_library_init();

  std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)> ctx{
      SSL_CTX_new(TLSv1_2_server_method()), &SSL_CTX_free};
  CHECK(ctx);
  SSL_CTX_set_options(ctx.get(), SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE |
                                     SSL_OP_NO_SSLv2);

  std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> ec_key{
      EC_KEY_new_by_curve_name(NID_X9_62_prime256v1), &EC_KEY_free};
  CHECK(ec_key) << GetSslError();
  CHECK_EQ(1, SSL_CTX_set_tmp_ecdh(ctx.get(), ec_key.get())) << GetSslError();

  std::unique_ptr<X509, decltype(&X509_free)> x509{X509_new(), &X509_free};
  CHECK(x509);
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey{EVP_PKEY_new(),
                                                           &EVP_PKEY_free};
  CHECK(pkey);
  GenerateX509(x509.get(), pkey.get());
  CHECK_EQ(1, SSL_CTX_use_PrivateKey(ctx.get(), pkey.get())) << GetSslError();
  CHECK_EQ(1, SSL_CTX_use_certificate(ctx.get(), x509.get())) << GetSslError();

  CHECK_EQ(1, SSL_CTX_check_private_key(ctx.get())) << GetSslError();

  httpd_.reset(evhtp_new(task_runner_->GetEventBase(), nullptr));
  CHECK(httpd_);
  httpsd_.reset(evhtp_new(task_runner_->GetEventBase(), nullptr));
  CHECK(httpsd_);

  httpsd_.get()->ssl_ctx = ctx.release();

  CHECK_EQ(0, evhtp_bind_socket(httpd_.get(), "0.0.0.0", GetHttpPort(), -1));
  CHECK_EQ(0, evhtp_bind_socket(httpsd_.get(), "0.0.0.0", GetHttpsPort(), -1));
}

void HttpServerImpl::GenerateX509(X509* x509, EVP_PKEY* pkey) {
  CHECK(x509) << GetSslError();

  X509_set_version(x509, 2);

  X509_gmtime_adj(X509_get_notBefore(x509), 0);
  X509_gmtime_adj(X509_get_notAfter(x509),
                  base::TimeDelta::FromDays(365).InSeconds());

  CHECK(pkey) << GetSslError();
  std::unique_ptr<BIGNUM, decltype(&BN_free)> big_num(BN_new(), &BN_free);
  CHECK(BN_set_word(big_num.get(), 65537)) << GetSslError();
  auto rsa = RSA_new();
  RSA_generate_key_ex(rsa, 2048, big_num.get(), nullptr);
  CHECK(EVP_PKEY_assign_RSA(pkey, rsa)) << GetSslError();

  X509_set_pubkey(x509, pkey);

  CHECK(X509_sign(x509, pkey, EVP_sha256())) << GetSslError();

  cert_fingerprint_.resize(EVP_MD_size(EVP_sha256()));
  uint32_t len = 0;
  CHECK(X509_digest(x509, EVP_sha256(), cert_fingerprint_.data(), &len));
  CHECK_EQ(len, cert_fingerprint_.size());
}

void HttpServerImpl::NotFound(evhtp_request_t* req) {
  EventPtr<evbuffer> buf{evbuffer_new()};
  evbuffer_add_printf(buf.get(), "404 Not Found: %s\n", req->uri->path->full);
  evhtp_send_reply_start(req, 404);
  evhtp_send_reply_body(req, buf.get());
  evhtp_send_reply_end(req);
}

void HttpServerImpl::ProcessRequest(evhtp_request_t* req) {
  std::unique_ptr<RequestImpl> request{new RequestImpl{EventPtr<evhtp_request_t>{req}}};
  std::string path = request->GetPath();
  auto it = handlers_.find(path);
  if (it != handlers_.end()) {
    return it->second.Run(std::move(request));
  }
  NotFound(req);
}

void HttpServerImpl::ProcessRequestCallback(evhtp_request_t* req, void* arg) {
  static_cast<HttpServerImpl*>(arg)->ProcessRequest(req);
}

void HttpServerImpl::AddHttpRequestHandler(
    const std::string& path,
    const RequestHandlerCallback& callback) {
  handlers_.insert(std::make_pair(path, callback));
  evhtp_set_cb(httpd_.get(), path.c_str(), &ProcessRequestCallback, this);
}

void HttpServerImpl::AddHttpsRequestHandler(
    const std::string& path,
    const RequestHandlerCallback& callback) {
  handlers_.insert(std::make_pair(path, callback));
  evhtp_set_cb(httpsd_.get(), path.c_str(), &ProcessRequestCallback, this);
}

void HttpServerImpl::ProcessReply(std::shared_ptr<RequestImpl> request,
                                  int status_code,
                                  const std::string& data,
                                  const std::string& mime_type) {}

uint16_t HttpServerImpl::GetHttpPort() const {
  return 7780;
}

uint16_t HttpServerImpl::GetHttpsPort() const {
  return 7781;
}

base::TimeDelta HttpServerImpl::GetRequestTimeout() const {
  return base::TimeDelta::Max();
}

std::vector<uint8_t> HttpServerImpl::GetHttpsCertificateFingerprint() const {
  return cert_fingerprint_;
}

}  // namespace examples
}  // namespace weave
