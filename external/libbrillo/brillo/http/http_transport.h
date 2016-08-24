// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_HTTP_TRANSPORT_H_
#define LIBBRILLO_BRILLO_HTTP_HTTP_TRANSPORT_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/callback_forward.h>
#include <base/location.h>
#include <base/macros.h>
#include <base/time/time.h>
#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>

namespace brillo {
namespace http {

BRILLO_EXPORT extern const char kErrorDomain[];

class Request;
class Response;
class Connection;

using RequestID = int;

using HeaderList = std::vector<std::pair<std::string, std::string>>;
using SuccessCallback =
    base::Callback<void(RequestID, std::unique_ptr<Response>)>;
using ErrorCallback = base::Callback<void(RequestID, const brillo::Error*)>;

///////////////////////////////////////////////////////////////////////////////
// Transport is a base class for specific implementation of HTTP communication.
// This class (and its underlying implementation) is used by http::Request and
// http::Response classes to provide HTTP functionality to the clients.
///////////////////////////////////////////////////////////////////////////////
class BRILLO_EXPORT Transport : public std::enable_shared_from_this<Transport> {
 public:
  Transport() = default;
  virtual ~Transport() = default;

  // Creates a connection object and initializes it with the specified data.
  // |transport| is a shared pointer to this transport object instance,
  // used to maintain the object alive as long as the connection exists.
  // The |url| here is the full URL specified in the request. It is passed
  // to the underlying transport (e.g. CURL) to establish the connection.
  virtual std::shared_ptr<Connection> CreateConnection(
      const std::string& url,
      const std::string& method,
      const HeaderList& headers,
      const std::string& user_agent,
      const std::string& referer,
      brillo::ErrorPtr* error) = 0;

  // Runs |callback| on the task runner (message loop) associated with the
  // transport. For transports that do not contain references to real message
  // loops (e.g. a fake transport), calls the callback immediately.
  virtual void RunCallbackAsync(const tracked_objects::Location& from_here,
                                const base::Closure& callback) = 0;

  // Initiates an asynchronous transfer on the given |connection|.
  // The actual implementation of an async I/O is transport-specific.
  // Returns a request ID which can be used to cancel the request.
  virtual RequestID StartAsyncTransfer(
      Connection* connection,
      const SuccessCallback& success_callback,
      const ErrorCallback& error_callback) = 0;

  // Cancels a pending asynchronous request. This will cancel a pending request
  // scheduled by the transport while the I/O operations are still in progress.
  // As soon as all I/O completes for the request/response, or when an error
  // occurs, the success/error callbacks are invoked and the request is
  // considered complete and can no longer be canceled.
  // Returns false if pending request with |request_id| is not found (e.g. it
  // has already completed/its callbacks are dispatched).
  virtual bool CancelRequest(RequestID request_id) = 0;

  // Set the default timeout of requests made.
  virtual void SetDefaultTimeout(base::TimeDelta timeout) = 0;

  // Creates a default http::Transport (currently, using http::curl::Transport).
  static std::shared_ptr<Transport> CreateDefault();

 private:
  DISALLOW_COPY_AND_ASSIGN(Transport);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_HTTP_TRANSPORT_H_
