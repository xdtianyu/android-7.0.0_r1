// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_HTTP_CONNECTION_H_
#define LIBBRILLO_BRILLO_HTTP_HTTP_CONNECTION_H_

#include <memory>
#include <string>
#include <vector>

#include <base/callback_forward.h>
#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <brillo/http/http_transport.h>
#include <brillo/streams/stream.h>

namespace brillo {
namespace http {

class Response;

///////////////////////////////////////////////////////////////////////////////
// Connection class is the base class for HTTP communication session.
// It abstracts the implementation of underlying transport library (ex libcurl).
// When the Connection-derived class is constructed, it is pre-set up with
// basic initialization information necessary to initiate the server request
// connection (such as the URL, request method, etc - see
// Transport::CreateConnection() for more details). But most implementations
// would not probably initiate the physical connection until SendHeaders
// is called.
// You normally shouldn't worry about using this class directly.
// http::Request and http::Response classes use it for communication.
// Effectively this class is the interface for the request/response objects to
// the transport-specific instance of the communication channel with the
// destination server. It is created by Transport as part of initiating
// the connection to the destination URI and is shared between the request and
// response object until all the data is sent to the server and the response
// is received. The class does NOT represent a persistent TCP connection though
// (e.g. in keep-alive scenarios).
///////////////////////////////////////////////////////////////////////////////
class BRILLO_EXPORT Connection
    : public std::enable_shared_from_this<Connection> {
 public:
  explicit Connection(const std::shared_ptr<Transport>& transport)
      : transport_(transport) {}
  virtual ~Connection() = default;

  // The following methods are used by http::Request object to initiate the
  // communication with the server, send the request data and receive the
  // response.

  // Called by http::Request to initiate the connection with the server.
  // This normally opens the socket and sends the request headers.
  virtual bool SendHeaders(const HeaderList& headers,
                           brillo::ErrorPtr* error) = 0;
  // If needed, this function can be called to send the request body data.
  virtual bool SetRequestData(StreamPtr stream, brillo::ErrorPtr* error) = 0;
  // If needed, this function can be called to customize where the response
  // data is streamed to.
  virtual void SetResponseData(StreamPtr stream) = 0;
  // This function is called when all the data is sent off and it's time
  // to receive the response data. The method will block until the whole
  // response message is received, or if an error occur in which case the
  // function will return false and fill the error details in |error| object.
  virtual bool FinishRequest(brillo::ErrorPtr* error) = 0;
  // Send the request asynchronously and invoke the callback with the response
  // received. Returns the ID of the pending async request.
  virtual RequestID FinishRequestAsync(const SuccessCallback& success_callback,
                                       const ErrorCallback& error_callback) = 0;

  // The following methods are used by http::Response object to obtain the
  // response data. They are used only after the response data has been received
  // since the http::Response object is not constructed until
  // Request::GetResponse()/Request::GetResponseAndBlock() methods are called.

  // Returns the HTTP status code (e.g. 200 for success).
  virtual int GetResponseStatusCode() const = 0;
  // Returns the status text (e.g. for error 403 it could be "NOT AUTHORIZED").
  virtual std::string GetResponseStatusText() const = 0;
  // Returns the HTTP protocol version (e.g. "HTTP/1.1").
  virtual std::string GetProtocolVersion() const = 0;
  // Returns the value of particular response header, or empty string if the
  // headers wasn't received.
  virtual std::string GetResponseHeader(
      const std::string& header_name) const = 0;
  // Returns the response data stream. This function can be called only once
  // as it transfers ownership of the data stream to the caller. Subsequent
  // calls will fail with "Stream closed" error.
  // Returns empty stream on failure and fills in the error information in
  // |error| object.
  virtual StreamPtr ExtractDataStream(brillo::ErrorPtr* error) = 0;

 protected:
  // |transport_| is mainly used to keep the object alive as long as the
  // connection exists. But some implementations of Connection could use
  // the Transport-derived class for their own needs as well.
  std::shared_ptr<Transport> transport_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Connection);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_HTTP_CONNECTION_H_
