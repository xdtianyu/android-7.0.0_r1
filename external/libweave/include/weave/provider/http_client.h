// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_

#include <string>
#include <utility>
#include <vector>

#include <base/callback.h>
#include <weave/error.h>

namespace weave {
namespace provider {

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to make HTTP/HTTPS calls to external
// services.
//
// HttpClient interface has only one method SendRequest(...) to implement.
// However, user code should also implement Response interface, that will be
// passed into callback.
//
// Implementation of the SendRequest(...) method should make a proper
// HTTP / HTTPS call according to the input parameters:
//   method - of the supported methods (kGet, kPatch, kPost, kPut) which
//     should map to the corresponding HTTP verb (GET, PATCH, POST, PUT) in
//     the request.
//   url - full URL including protocol, domain, path and parameters. Protocol
//     may be "http" or "https". In case of "https", it is implementer's
//     responsibility to establish a secure connection and verify endpoint
//     certificate chain. libweave will attempt connecting to Google Weave
//     servers. Proper root CA certificates should be available on the device.
//   headers - list of HTTP request headers that should be attached to the
//     request.
//   data - binary data that should be sent within HTTP request body. Empty
//     string means no data. Implementation needs to check for that. For
//     example, kGet method should never have data. It is also possible to have
//     no data for other methods as well.
//   callback - standard callback to notify libweave when request is complete
//     and provide results and response data.
//
// Implementation of the SendRequest(...) should be non-blocking, meaning it
// should schedule network request and return right away. Later (after the
// request is complete), callback should be invokes on the same thread.
// Callback should never be called before SendRequest(...) returns.
//
// When invoking callback function, user should privide implementation
// of the Response interface. For example, the following could be used as a
// simple implementation:
//   struct ResponseImpl : public provider::HttpClient::Response {
//     int GetStatusCode() const override { return status; }
//     std::string GetContentType() const override { return content_type; }
//     std::string GetData() const override { return data; }
//     int status{0};
//     std::string content_type;
//     std::string data;
//   };
//
// See libweave/examples/provider/curl_http_client.cc for complete example
// implementing HttpClient interface using curl.

class HttpClient {
 public:
  enum class Method {
    kGet,
    kPatch,
    kPost,
    kPut,
  };

  class Response {
   public:
    virtual int GetStatusCode() const = 0;
    virtual std::string GetContentType() const = 0;
    virtual std::string GetData() const = 0;

    virtual ~Response() {}
  };

  using Headers = std::vector<std::pair<std::string, std::string>>;
  using SendRequestCallback =
      base::Callback<void(std::unique_ptr<Response> response, ErrorPtr error)>;

  virtual void SendRequest(Method method,
                           const std::string& url,
                           const Headers& headers,
                           const std::string& data,
                           const SendRequestCallback& callback) = 0;

 protected:
  virtual ~HttpClient() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_CLIENT_H_
