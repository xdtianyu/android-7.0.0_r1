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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/callback_forward.h>
#include <base/files/file.h>
#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include <brillo/errors/error.h>
#include <brillo/streams/stream.h>
#include <libwebserv/export.h>

struct MHD_Connection;

namespace libwebserv {

class DBusProtocolHandler;

using PairOfStrings = std::pair<std::string, std::string>;

// This class represents the file information about a file uploaded via
// POST request using multipart/form-data request.
class LIBWEBSERV_EXPORT FileInfo final {
 public:
  const std::string& GetFileName() const { return file_name_; }
  const std::string& GetContentType() const { return content_type_; }
  const std::string& GetTransferEncoding() const { return transfer_encoding_; }
  void GetData(
      const base::Callback<void(brillo::StreamPtr)>& success_callback,
      const base::Callback<void(brillo::Error*)>& error_callback) const;

 private:
  friend class DBusServer;

  LIBWEBSERV_PRIVATE FileInfo(DBusProtocolHandler* handler,
                              int file_id,
                              const std::string& request_id,
                              const std::string& file_name,
                              const std::string& content_type,
                              const std::string& transfer_encoding);

  DBusProtocolHandler* handler_{nullptr};
  int file_id_{0};
  std::string request_id_;
  std::string file_name_;
  std::string content_type_;
  std::string transfer_encoding_;

  DISALLOW_COPY_AND_ASSIGN(FileInfo);
};

// A class that represents the HTTP request data.
class LIBWEBSERV_EXPORT Request {
 public:
  Request(const std::string& url, const std::string& method)
    : url_{url}, method_{method} {}
  virtual ~Request() = default;

  // Gets the request body data stream. Note that the stream is available
  // only for requests that provided data and if this data is not already
  // pre-parsed by the server (e.g. "application/x-www-form-urlencoded" and
  // "multipart/form-data"). If there is no request body, or the data has been
  // pre-parsed by the server, the returned stream will be empty.
  // The stream returned is valid for as long as the Request object itself is
  // alive. Accessing the stream after the Request object is destroyed will lead
  // to an undefined behavior (will likely just crash).
  virtual brillo::StreamPtr GetDataStream() = 0;

  // Returns the request path (e.g. "/path/document").
  const std::string& GetPath() const { return url_; }

  // Returns the request method (e.g. "GET", "POST", etc).
  const std::string& GetMethod() const { return method_; }

  // Returns a list of key-value pairs that include values provided on the URL
  // (e.g. "http://server.com/?foo=bar") and the non-file form fields in the
  // POST data.
  std::vector<PairOfStrings> GetFormData() const;

  // Returns a list of key-value pairs for query parameters provided on the URL
  // (e.g. "http://server.com/?foo=bar").
  std::vector<PairOfStrings> GetFormDataGet() const;

  // Returns a list of key-value pairs for the non-file form fields in the
  // POST data.
  std::vector<PairOfStrings> GetFormDataPost() const;

  // Returns a list of file information records for all the file uploads in
  // the POST request.
  std::vector<std::pair<std::string, const FileInfo*>> GetFiles() const;

  // Gets the values of form field with given |name|. This includes both
  // values provided on the URL and as part of form data in POST request.
  std::vector<std::string> GetFormField(const std::string& name) const;

  // Gets the values of form field with given |name| for form data in POST
  // request.
  std::vector<std::string> GetFormFieldPost(const std::string& name) const;

  // Gets the values of URL query parameters with given |name|.
  std::vector<std::string> GetFormFieldGet(const std::string& name) const;

  // Gets the file upload parameters for a file form field of given |name|.
  std::vector<const FileInfo*> GetFileInfo(const std::string& name) const;

  // Returns a list of key-value pairs for all the request headers.
  std::vector<PairOfStrings> GetHeaders() const;

  // Returns the value(s) of a request header of given |name|.
  std::vector<std::string> GetHeader(const std::string& name) const;

  // Returns the value of a request header of given |name|. If there are more
  // than one header with this name, the value of the first header is returned.
  // An empty string is returned if the header does not exist in the request.
  std::string GetFirstHeader(const std::string& name) const;

 protected:
  std::string url_;
  std::string method_;
  std::multimap<std::string, std::string> post_data_;
  std::multimap<std::string, std::string> get_data_;
  std::multimap<std::string, std::unique_ptr<FileInfo>> file_info_;
  std::multimap<std::string, std::string> headers_;
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_H_
