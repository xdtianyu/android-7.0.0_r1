// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_HTTP_UTILS_H_
#define LIBBRILLO_BRILLO_HTTP_HTTP_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <brillo/http/http_form_data.h>
#include <brillo/http/http_request.h>

namespace base {
class Value;
class DictionaryValue;
}  // namespace base

namespace brillo {
namespace http {

using FormFieldList = std::vector<std::pair<std::string, std::string>>;

////////////////////////////////////////////////////////////////////////////////
// The following are simple utility helper functions for common HTTP operations
// that use http::Request object behind the scenes and set it up accordingly.
// The values for request method, data MIME type, request header names should
// not be directly encoded in most cases, but use predefined constants from
// http_request.h.
// So, instead of calling:
//    SendRequestAndBlock("POST",
//                        "http://url",
//                        "data", 4,
//                        "text/plain",
//                        {{"Authorization", "Bearer TOKEN"}},
//                        transport, error);
// You should do use this instead:
//    SendRequestAndBlock(brillo::http::request_type::kPost,
//                        "http://url",
//                        "data", 4,
//                        brillo::mime::text::kPlain,
//                        {{brillo::http::request_header::kAuthorization,
//                          "Bearer TOKEN"}},
//                        transport, error);
//
// For more advanced functionality you need to use Request/Response objects
// directly.
////////////////////////////////////////////////////////////////////////////////

// Performs a generic HTTP request with binary data. Success status,
// returned data and additional information (such as returned HTTP headers)
// can be obtained from the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> SendRequestAndBlock(
    const std::string& method,
    const std::string& url,
    const void* data,
    size_t data_size,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Same as above, but without sending the request body.
// This is especially useful for requests like "GET" and "HEAD".
BRILLO_EXPORT std::unique_ptr<Response> SendRequestWithNoDataAndBlock(
    const std::string& method,
    const std::string& url,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Same as above but asynchronous. On success, |success_callback| is called
// with the response object. On failure, |error_callback| is called with the
// error details.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID SendRequest(
    const std::string& method,
    const std::string& url,
    StreamPtr stream,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Same as above, but takes a memory buffer. The pointer should be valid only
// until the function returns. The data is copied into an internal buffer to be
// available for the duration of the asynchronous operation.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID SendRequest(
    const std::string& method,
    const std::string& url,
    const void* data,
    size_t data_size,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Asynchronous version of SendRequestNoData().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID SendRequestWithNoData(
    const std::string& method,
    const std::string& url,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a GET request. Success status, returned data and additional
// information (such as returned HTTP headers) can be obtained from
// the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> GetAndBlock(
    const std::string& url,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Asynchronous version of http::Get().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID Get(
    const std::string& url,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a HEAD request. Success status and additional
// information (such as returned HTTP headers) can be obtained from
// the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> HeadAndBlock(
    const std::string& url,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Performs an asynchronous HEAD request.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID Head(
    const std::string& url,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a POST request with binary data. Success status, returned data
// and additional information (such as returned HTTP headers) can be obtained
// from the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> PostBinaryAndBlock(
    const std::string& url,
    const void* data,
    size_t data_size,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PostBinary().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostBinary(
    const std::string& url,
    StreamPtr stream,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Same as above, but takes a memory buffer. The pointer should be valid only
// until the function returns. The data is copied into an internal buffer
// to be available for the duration of the asynchronous operation.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostBinary(
    const std::string& url,
    const void* data,
    size_t data_size,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a POST request with text data. Success status, returned data
// and additional information (such as returned HTTP headers) can be obtained
// from the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> PostTextAndBlock(
    const std::string& url,
    const std::string& data,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PostText().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostText(
    const std::string& url,
    const std::string& data,
    const std::string& mime_type,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a POST request with form data. Success status, returned data
// and additional information (such as returned HTTP headers) can be obtained
// from the returned Response object. The form data is a list of key/value
// pairs. The data is posed as "application/x-www-form-urlencoded".
BRILLO_EXPORT std::unique_ptr<Response> PostFormDataAndBlock(
    const std::string& url,
    const FormFieldList& data,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PostFormData() above.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostFormData(
    const std::string& url,
    const FormFieldList& data,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a POST request with form data, including binary file uploads.
// Success status, returned data and additional information (such as returned
// HTTP headers) can be obtained from the returned Response object.
// The data is posed as "multipart/form-data".
BRILLO_EXPORT std::unique_ptr<Response> PostFormDataAndBlock(
    const std::string& url,
    std::unique_ptr<FormData> form_data,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PostFormData() above.
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostFormData(
    const std::string& url,
    std::unique_ptr<FormData> form_data,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a POST request with JSON data. Success status, returned data
// and additional information (such as returned HTTP headers) can be obtained
// from the returned Response object. If a JSON response is expected,
// use ParseJsonResponse() method on the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> PostJsonAndBlock(
    const std::string& url,
    const base::Value* json,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PostJson().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PostJson(
    const std::string& url,
    std::unique_ptr<base::Value> json,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Performs a PATCH request with JSON data. Success status, returned data
// and additional information (such as returned HTTP headers) can be obtained
// from the returned Response object. If a JSON response is expected,
// use ParseJsonResponse() method on the returned Response object.
BRILLO_EXPORT std::unique_ptr<Response> PatchJsonAndBlock(
    const std::string& url,
    const base::Value* json,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    brillo::ErrorPtr* error);

// Async version of PatchJson().
// Returns the ID of the request which can be used to cancel the pending
// request using Transport::CancelRequest().
BRILLO_EXPORT RequestID PatchJson(
    const std::string& url,
    std::unique_ptr<base::Value> json,
    const HeaderList& headers,
    std::shared_ptr<Transport> transport,
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback);

// Given an http::Response object, parse the body data into Json object.
// Returns null if failed. Optional |error| can be passed in to
// get the extended error information as to why the parse failed.
BRILLO_EXPORT std::unique_ptr<base::DictionaryValue> ParseJsonResponse(
    Response* response,
    int* status_code,
    brillo::ErrorPtr* error);

// Converts a request header name to canonical form (lowercase with uppercase
// first letter and each letter after a hyphen ('-')).
// "content-TYPE" will be converted to "Content-Type".
BRILLO_EXPORT std::string GetCanonicalHeaderName(const std::string& name);

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_HTTP_UTILS_H_
