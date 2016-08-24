// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/http/http_connection_fake.h>

#include <base/logging.h>
#include <brillo/bind_lambda.h>
#include <brillo/http/http_request.h>
#include <brillo/mime_utils.h>
#include <brillo/streams/memory_stream.h>
#include <brillo/strings/string_utils.h>

namespace brillo {
namespace http {
namespace fake {

Connection::Connection(const std::string& url,
                       const std::string& method,
                       const std::shared_ptr<http::Transport>& transport)
    : http::Connection(transport), request_(url, method) {
  VLOG(1) << "fake::Connection created: " << method;
}

Connection::~Connection() {
  VLOG(1) << "fake::Connection destroyed";
}

bool Connection::SendHeaders(const HeaderList& headers,
                             brillo::ErrorPtr* /* error */) {
  request_.AddHeaders(headers);
  return true;
}

bool Connection::SetRequestData(StreamPtr stream,
                                brillo::ErrorPtr* /* error */) {
  request_.SetData(std::move(stream));
  return true;
}

bool Connection::FinishRequest(brillo::ErrorPtr*  /* error */) {
  using brillo::string_utils::ToString;
  request_.AddHeaders(
      {{request_header::kContentLength, ToString(request_.GetData().size())}});
  fake::Transport* transport = static_cast<fake::Transport*>(transport_.get());
  CHECK(transport) << "Expecting a fake transport";
  auto handler = transport->GetHandler(request_.GetURL(), request_.GetMethod());
  if (handler.is_null()) {
    LOG(ERROR) << "Received unexpected " << request_.GetMethod()
               << " request at " << request_.GetURL();
    response_.ReplyText(status_code::NotFound,
                        "<html><body>Not found</body></html>",
                        brillo::mime::text::kHtml);
  } else {
    handler.Run(request_, &response_);
  }
  return true;
}

RequestID Connection::FinishRequestAsync(
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback) {
  // Make sure the produced Closure holds a reference to the instance of this
  // connection.
  auto connection = std::static_pointer_cast<Connection>(shared_from_this());
  auto callback = [connection, success_callback, error_callback] {
    connection->FinishRequestAsyncHelper(success_callback, error_callback);
  };
  transport_->RunCallbackAsync(FROM_HERE, base::Bind(callback));
  return 1;
}

void Connection::FinishRequestAsyncHelper(
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback) {
  brillo::ErrorPtr error;
  if (!FinishRequest(&error)) {
    error_callback.Run(1, error.get());
  } else {
    std::unique_ptr<Response> response{new Response{shared_from_this()}};
    success_callback.Run(1, std::move(response));
  }
}

int Connection::GetResponseStatusCode() const {
  return response_.GetStatusCode();
}

std::string Connection::GetResponseStatusText() const {
  return response_.GetStatusText();
}

std::string Connection::GetProtocolVersion() const {
  return response_.GetProtocolVersion();
}

std::string Connection::GetResponseHeader(
    const std::string& header_name) const {
  return response_.GetHeader(header_name);
}

StreamPtr Connection::ExtractDataStream(brillo::ErrorPtr* error) {
  // HEAD requests must not return body.
  if (request_.GetMethod() != request_type::kHead) {
    return MemoryStream::OpenRef(response_.GetData(), error);
  } else {
    // Return empty data stream for HEAD requests.
    return MemoryStream::OpenCopyOf(nullptr, 0, error);
  }
}

}  // namespace fake
}  // namespace http
}  // namespace brillo
