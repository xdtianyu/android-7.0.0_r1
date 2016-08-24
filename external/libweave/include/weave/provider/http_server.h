// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_SERVER_H_
#define LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_SERVER_H_

#include <string>
#include <vector>

#include <base/callback.h>
#include <base/time/time.h>
#include <weave/stream.h>

namespace weave {
namespace provider {

// This interface should be implemented by the user of libweave and
// provided during device creation in Device::Create(...)
// libweave will use this interface to handle HTTP / HTTPS requests for Privet
// APIs.
//
// This interface consist of 2 parts that need to be implemented by the
// libweave user: HttpServer and HttpServer::Request. HttpServer provides
// interface to control webserver, and is used to initialize Device object.
// Request provides an abstraction for a specific HTTP request and may be a
// short-lived object.
//
// Implementation of AddHttpsRequestHandler(...) method should follow the
// same guidelines as implementation of AddHttpRequestHandler(...) with the
// only difference, it is for HTTPS connection (not HTTP).
//
// Implementation of GetHttpPort() method should return port number on
// which HTTP server will be listening. Normally it is port 80, but this
// allows implementer to choose different port if necessary and tell it to
// libweave.
//
// Implementation of GetHttpsPort() should follow the same guidelines as
// GetHttpPort(). Default HTTPS port is 443, but could be changed and
// communicated to libweave using this method.
//
// Implementation of GetHttpsCertificateFingerprint() method should
// compute fingerprint of the certificate that HTTPS web server will be using.
// Method of computing fingerprint is the following:
//   fingerprint = SHA256 ( DER certificate )
// You can see example implementation in HttpServerImpl::GenerateX509()
// in libweave/examples/provider/event_http_server.cc
//
// Implementation of AddHttpRequestHandler(...) method should add path
// to the list of the exposed entry points for the webserver and store
// path and callback pair somewhere. Once webserver receives an HTTP request,
// it should check if there is a libweave-registered handler corresponding to
// the path in the request. If there is one, implementation should invoke
// the callback associated with this path. If there is no callback associated
// with request path, webserver should return HTTP status code 404.
//
// For example, let's say local IP is "192.168.0.1" and libweave called
//   AddHttpRequestHandler("/privet/info", InfoHandlerCallback);
// If webserver receives "http://192.168.0.1/privet/info" request, HttpServer
// implementation must invoke InfoHandlerCallback.
// If webserver receives "http://192.168.0.1/privet/auth" request, it must
// return HTTP status code 404 response.
//
// As everywhere else, invoking callbacks have some limitations:
//   - callback should not be called before AddHttpRequestHandler() returns
//   - callback should be called on the same thread
//
// Once HttpServer implementation invokes a registered callback, it should
// provide the Request interface implementation to access a request data.
//
// Implementation of GetPath() method should return path of the HTTP
// request. For example, "/privet/info".
//
// Implementation of the GetFirstHeader(...) method should return the first
// header in the request matching name parameter of this method.
// For example, GetFirstHeader("Content-Length") may return "3495".
//
// Implementation of GetData() method should return full request data
// in a binary format wrapped into std::string object.
//
// Implementation of the SendReply(...) method should send request response
// message with specified parameters:
//   status_code - standard HTTP status code, for example 200 to indicate
//     successful response.
//   data - binary data of the response body wrapped into std::string object.
//   mime_type - MIME type of the response, that should be transferred into
//     "Content-Type" HTTP header.
// Implementation of the SendReply(...) method may also add other standard
// HTTP headers, like "Content-Length" or "Transfer-Encoding" depending on
// capabilities of the server and client which made this request.
//
// In case a device has multiple networking interfaces, the device developer
// needs to make a decision where local APIs (Privet) are necessary and where
// they are not needed. For example, it may not make sense to expose local
// APIs on any external-facing network interface (cellular or WAN).
//
// In some cases, there might be more then one network interface where local
// APIs makes sense. For example, a device may have both WiFi and Ethernet
// connections. In such case, webserver should start on both interfaces
// simultaneously, and allow requests from both interfaces to be handled by
// libweave.
//
// From libweave perspective, it always looks like there is only one network
// interface. It is the job of HttpServer implementation to hide network
// complexity from libweave and to bring webserver up on the same port on all
// interfaces.

class HttpServer {
 public:
  class Request {
   public:
    virtual ~Request() {}

    virtual std::string GetPath() const = 0;
    virtual std::string GetFirstHeader(const std::string& name) const = 0;
    virtual std::string GetData() = 0;

    virtual void SendReply(int status_code,
                           const std::string& data,
                           const std::string& mime_type) = 0;
  };

  // Callback type for AddRequestHandler.
  using RequestHandlerCallback =
      base::Callback<void(std::unique_ptr<Request> request)>;

  // Adds callback called on new http/https requests with the given path.
  virtual void AddHttpRequestHandler(
      const std::string& path,
      const RequestHandlerCallback& callback) = 0;
  virtual void AddHttpsRequestHandler(
      const std::string& path,
      const RequestHandlerCallback& callback) = 0;

  virtual uint16_t GetHttpPort() const = 0;
  virtual uint16_t GetHttpsPort() const = 0;
  virtual std::vector<uint8_t> GetHttpsCertificateFingerprint() const = 0;

  // Specifies request timeout, after which the web server automatically aborts
  // requests. Should return base::TimeDelta::Max() if there is no timeout.
  virtual base::TimeDelta GetRequestTimeout() const = 0;

 protected:
  virtual ~HttpServer() {}
};

}  // namespace provider
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_PROVIDER_HTTP_SERVER_H_
