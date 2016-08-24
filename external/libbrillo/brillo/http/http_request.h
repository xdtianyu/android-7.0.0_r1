// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_HTTP_REQUEST_H_
#define LIBBRILLO_BRILLO_HTTP_HTTP_REQUEST_H_

#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/macros.h>
#include <brillo/brillo_export.h>
#include <brillo/errors/error.h>
#include <brillo/http/http_connection.h>
#include <brillo/http/http_transport.h>

namespace brillo {
namespace http {

// HTTP request verbs
namespace request_type {
BRILLO_EXPORT extern const char kOptions[];
BRILLO_EXPORT extern const char kGet[];
BRILLO_EXPORT extern const char kHead[];
BRILLO_EXPORT extern const char kPost[];
BRILLO_EXPORT extern const char kPut[];
BRILLO_EXPORT extern const char kPatch[];  // Non-standard HTTP/1.1 verb
BRILLO_EXPORT extern const char kDelete[];
BRILLO_EXPORT extern const char kTrace[];
BRILLO_EXPORT extern const char kConnect[];
BRILLO_EXPORT extern const char kCopy[];   // Non-standard HTTP/1.1 verb
BRILLO_EXPORT extern const char kMove[];   // Non-standard HTTP/1.1 verb
}  // namespace request_type

// HTTP request header names
namespace request_header {
BRILLO_EXPORT extern const char kAccept[];
BRILLO_EXPORT extern const char kAcceptCharset[];
BRILLO_EXPORT extern const char kAcceptEncoding[];
BRILLO_EXPORT extern const char kAcceptLanguage[];
BRILLO_EXPORT extern const char kAllow[];
BRILLO_EXPORT extern const char kAuthorization[];
BRILLO_EXPORT extern const char kCacheControl[];
BRILLO_EXPORT extern const char kConnection[];
BRILLO_EXPORT extern const char kContentEncoding[];
BRILLO_EXPORT extern const char kContentLanguage[];
BRILLO_EXPORT extern const char kContentLength[];
BRILLO_EXPORT extern const char kContentLocation[];
BRILLO_EXPORT extern const char kContentMd5[];
BRILLO_EXPORT extern const char kContentRange[];
BRILLO_EXPORT extern const char kContentType[];
BRILLO_EXPORT extern const char kCookie[];
BRILLO_EXPORT extern const char kDate[];
BRILLO_EXPORT extern const char kExpect[];
BRILLO_EXPORT extern const char kExpires[];
BRILLO_EXPORT extern const char kFrom[];
BRILLO_EXPORT extern const char kHost[];
BRILLO_EXPORT extern const char kIfMatch[];
BRILLO_EXPORT extern const char kIfModifiedSince[];
BRILLO_EXPORT extern const char kIfNoneMatch[];
BRILLO_EXPORT extern const char kIfRange[];
BRILLO_EXPORT extern const char kIfUnmodifiedSince[];
BRILLO_EXPORT extern const char kLastModified[];
BRILLO_EXPORT extern const char kMaxForwards[];
BRILLO_EXPORT extern const char kPragma[];
BRILLO_EXPORT extern const char kProxyAuthorization[];
BRILLO_EXPORT extern const char kRange[];
BRILLO_EXPORT extern const char kReferer[];
BRILLO_EXPORT extern const char kTE[];
BRILLO_EXPORT extern const char kTrailer[];
BRILLO_EXPORT extern const char kTransferEncoding[];
BRILLO_EXPORT extern const char kUpgrade[];
BRILLO_EXPORT extern const char kUserAgent[];
BRILLO_EXPORT extern const char kVia[];
BRILLO_EXPORT extern const char kWarning[];
}  // namespace request_header

// HTTP response header names
namespace response_header {
BRILLO_EXPORT extern const char kAcceptRanges[];
BRILLO_EXPORT extern const char kAge[];
BRILLO_EXPORT extern const char kAllow[];
BRILLO_EXPORT extern const char kCacheControl[];
BRILLO_EXPORT extern const char kConnection[];
BRILLO_EXPORT extern const char kContentEncoding[];
BRILLO_EXPORT extern const char kContentLanguage[];
BRILLO_EXPORT extern const char kContentLength[];
BRILLO_EXPORT extern const char kContentLocation[];
BRILLO_EXPORT extern const char kContentMd5[];
BRILLO_EXPORT extern const char kContentRange[];
BRILLO_EXPORT extern const char kContentType[];
BRILLO_EXPORT extern const char kDate[];
BRILLO_EXPORT extern const char kETag[];
BRILLO_EXPORT extern const char kExpires[];
BRILLO_EXPORT extern const char kLastModified[];
BRILLO_EXPORT extern const char kLocation[];
BRILLO_EXPORT extern const char kPragma[];
BRILLO_EXPORT extern const char kProxyAuthenticate[];
BRILLO_EXPORT extern const char kRetryAfter[];
BRILLO_EXPORT extern const char kServer[];
BRILLO_EXPORT extern const char kSetCookie[];
BRILLO_EXPORT extern const char kTrailer[];
BRILLO_EXPORT extern const char kTransferEncoding[];
BRILLO_EXPORT extern const char kUpgrade[];
BRILLO_EXPORT extern const char kVary[];
BRILLO_EXPORT extern const char kVia[];
BRILLO_EXPORT extern const char kWarning[];
BRILLO_EXPORT extern const char kWwwAuthenticate[];
}  // namespace response_header

// HTTP request status (error) codes
namespace status_code {
// OK to continue with request
static const int Continue = 100;
// Server has switched protocols in upgrade header
static const int SwitchProtocols = 101;

// Request completed
static const int Ok = 200;
// Object created, reason = new URI
static const int Created = 201;
// Async completion (TBS)
static const int Accepted = 202;
// Partial completion
static const int Partial = 203;
// No info to return
static const int NoContent = 204;
// Request completed, but clear form
static const int ResetContent = 205;
// Partial GET fulfilled
static const int PartialContent = 206;

// Server couldn't decide what to return
static const int Ambiguous = 300;
// Object permanently moved
static const int Moved = 301;
// Object temporarily moved
static const int Redirect = 302;
// Redirection w/ new access method
static const int RedirectMethod = 303;
// If-Modified-Since was not modified
static const int NotModified = 304;
// Redirection to proxy, location header specifies proxy to use
static const int UseProxy = 305;
// HTTP/1.1: keep same verb
static const int RedirectKeepVerb = 307;

// Invalid syntax
static const int BadRequest = 400;
// Access denied
static const int Denied = 401;
// Payment required
static const int PaymentRequired = 402;
// Request forbidden
static const int Forbidden = 403;
// Object not found
static const int NotFound = 404;
// Method is not allowed
static const int BadMethod = 405;
// No response acceptable to client found
static const int NoneAcceptable = 406;
// Proxy authentication required
static const int ProxyAuthRequired = 407;
// Server timed out waiting for request
static const int RequestTimeout = 408;
// User should resubmit with more info
static const int Conflict = 409;
// The resource is no longer available
static const int Gone = 410;
// The server refused to accept request w/o a length
static const int LengthRequired = 411;
// Precondition given in request failed
static const int PrecondionFailed = 412;
// Request entity was too large
static const int RequestTooLarge = 413;
// Request URI too long
static const int UriTooLong = 414;
// Unsupported media type
static const int UnsupportedMedia = 415;
// Retry after doing the appropriate action.
static const int RetryWith = 449;

// Internal server error
static const int InternalServerError = 500;
// Request not supported
static const int NotSupported = 501;
// Error response received from gateway
static const int BadGateway = 502;
// Temporarily overloaded
static const int ServiceUnavailable = 503;
// Timed out waiting for gateway
static const int GatewayTimeout = 504;
// HTTP version not supported
static const int VersionNotSupported = 505;
}  // namespace status_code

class Response;  // Just a forward declaration.
class FormData;

///////////////////////////////////////////////////////////////////////////////
// Request class is the main object used to set up and initiate an HTTP
// communication session. It is used to specify the HTTP request method,
// request URL and many optional parameters (such as HTTP headers, user agent,
// referer URL and so on.
//
// Once everything is setup, GetResponse() method is used to send the request
// and obtain the server response. The returned Response object can be
// used to inspect the response code, HTTP headers and/or response body.
///////////////////////////////////////////////////////////////////////////////
class BRILLO_EXPORT Request final {
 public:
  // The main constructor. |url| specifies the remote host address/path
  // to send the request to. |method| is the HTTP request verb and
  // |transport| is the HTTP transport implementation for server communications.
  Request(const std::string& url,
          const std::string& method,
          std::shared_ptr<Transport> transport);
  ~Request();

  // Gets/Sets "Accept:" header value. The default value is "*/*" if not set.
  void SetAccept(const std::string& accept_mime_types);
  const std::string& GetAccept() const;

  // Gets/Sets "Content-Type:" header value
  void SetContentType(const std::string& content_type);
  const std::string& GetContentType() const;

  // Adds additional HTTP request header
  void AddHeader(const std::string& header, const std::string& value);
  void AddHeaders(const HeaderList& headers);

  // Removes HTTP request header
  void RemoveHeader(const std::string& header);

  // Adds a request body. This is not to be used with GET method
  bool AddRequestBody(const void* data, size_t size, brillo::ErrorPtr* error);
  bool AddRequestBody(StreamPtr stream, brillo::ErrorPtr* error);

  // Adds a request body. This is not to be used with GET method.
  // This method also sets the correct content-type of the request, including
  // the multipart data boundary.
  bool AddRequestBodyAsFormData(std::unique_ptr<FormData> form_data,
                                brillo::ErrorPtr* error);

  // Adds a stream for the response. Otherwise a MemoryStream will be used.
  bool AddResponseStream(StreamPtr stream, brillo::ErrorPtr* error);

  // Makes a request for a subrange of data. Specifies a partial range with
  // either from beginning of the data to the specified offset (if |bytes| is
  // negative) or from the specified offset to the end of data (if |bytes| is
  // positive).
  // All individual ranges will be sent as part of "Range:" HTTP request header.
  void AddRange(int64_t bytes);

  // Makes a request for a subrange of data. Specifies a full range with
  // start and end bytes from the beginning of the requested data.
  // All individual ranges will be sent as part of "Range:" HTTP request header.
  void AddRange(uint64_t from_byte, uint64_t to_byte);

  // Returns the request URL
  const std::string& GetRequestURL() const;

  // Returns the request verb.
  const std::string& GetRequestMethod() const;

  // Gets/Sets a request referer URL (sent as "Referer:" request header).
  void SetReferer(const std::string& referer);
  const std::string& GetReferer() const;

  // Gets/Sets a user agent string (sent as "User-Agent:" request header).
  void SetUserAgent(const std::string& user_agent);
  const std::string& GetUserAgent() const;

  // Sends the request to the server and blocks until the response is received,
  // which is returned as the response object.
  // In case the server couldn't be reached for whatever reason, returns
  // empty unique_ptr (null). In such a case, the additional error information
  // can be returned through the optional supplied |error| parameter.
  std::unique_ptr<Response> GetResponseAndBlock(brillo::ErrorPtr* error);

  // Sends out the request and invokes the |success_callback| when the response
  // is received. In case of an error, the |error_callback| is invoked.
  // Returns the ID of the asynchronous request created.
  RequestID GetResponse(const SuccessCallback& success_callback,
                        const ErrorCallback& error_callback);

 private:
  friend class HttpRequestTest;

  // Helper function to create an http::Connection and send off request headers.
  BRILLO_PRIVATE bool SendRequestIfNeeded(brillo::ErrorPtr* error);

  // Implementation that provides particular HTTP transport.
  std::shared_ptr<Transport> transport_;

  // An established connection for adding request body. This connection
  // is maintained by the request object after the headers have been
  // sent and before the response is requested.
  std::shared_ptr<Connection> connection_;

  // Full request URL, such as "http://www.host.com/path/to/object"
  const std::string request_url_;
  // HTTP request verb, such as "GET", "POST", "PUT", ...
  const std::string method_;

  // Referrer URL, if any. Sent to the server via "Referer: " header.
  std::string referer_;
  // User agent string, if any. Sent to the server via "User-Agent: " header.
  std::string user_agent_;
  // Content type of the request body data.
  // Sent to the server via "Content-Type: " header.
  std::string content_type_;
  // List of acceptable response data types.
  // Sent to the server via "Accept: " header.
  std::string accept_ = "*/*";

  // List of optional request headers provided by the caller.
  std::multimap<std::string, std::string> headers_;
  // List of optional data ranges to request partial content from the server.
  // Sent to the server as "Range: " header.
  std::vector<std::pair<uint64_t, uint64_t>> ranges_;

  // range_value_omitted is used in |ranges_| list to indicate omitted value.
  // E.g. range (10,range_value_omitted) represents bytes from 10 to the end
  // of the data stream.
  const uint64_t range_value_omitted = std::numeric_limits<uint64_t>::max();

  DISALLOW_COPY_AND_ASSIGN(Request);
};

///////////////////////////////////////////////////////////////////////////////
// Response class is returned from Request::GetResponse() and is a way
// to get to response status, error codes, response HTTP headers and response
// data (body) if available.
///////////////////////////////////////////////////////////////////////////////
class BRILLO_EXPORT Response final {
 public:
  explicit Response(const std::shared_ptr<Connection>& connection);
  ~Response();

  // Returns true if server returned a success code (status code below 400).
  bool IsSuccessful() const;

  // Returns the HTTP status code (e.g. 200 for success)
  int GetStatusCode() const;

  // Returns the status text (e.g. for error 403 it could be "NOT AUTHORIZED").
  std::string GetStatusText() const;

  // Returns the content type of the response data.
  std::string GetContentType() const;

  // Returns response data stream by transferring ownership of the data stream
  // from Response class to the caller.
  StreamPtr ExtractDataStream(ErrorPtr* error);

  // Extracts the data from the underlying response data stream as a byte array.
  std::vector<uint8_t> ExtractData();

  // Extracts the data from the underlying response data stream as a string.
  std::string ExtractDataAsString();

  // Returns a value of a given response HTTP header.
  std::string GetHeader(const std::string& header_name) const;

 private:
  friend class HttpRequestTest;

  std::shared_ptr<Connection> connection_;

  DISALLOW_COPY_AND_ASSIGN(Response);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_HTTP_REQUEST_H_
