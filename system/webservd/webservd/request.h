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

#ifndef WEBSERVER_WEBSERVD_REQUEST_H_
#define WEBSERVER_WEBSERVD_REQUEST_H_

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <base/files/file.h>
#include <base/files/file_path.h>
#include <base/macros.h>
#include <base/memory/weak_ptr.h>
#include <brillo/streams/stream.h>

struct MHD_Connection;
struct MHD_PostProcessor;

namespace webservd {

class ProtocolHandler;
class TempFileManager;

using PairOfStrings = std::pair<std::string, std::string>;

// This class represents the file information about a file uploaded via
// POST request using multipart/form-data request.
class FileInfo final {
 public:
  FileInfo(const std::string& in_field_name,
           const std::string& in_file_name,
           const std::string& in_content_type,
           const std::string& in_transfer_encoding);

  // The name of the form field for the file upload.
  std::string field_name;
  // The name of the file name specified in the form field.
  std::string file_name;
  // The content type of the file data.
  std::string content_type;
  // Data transfer encoding specified. Could be empty if no transfer encoding
  // was specified.
  std::string transfer_encoding;
  // The file content data.
  brillo::StreamPtr data_stream;
  // The temporary file containing the file part data.
  base::FilePath temp_file_name;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileInfo);
};

// A class that represents the HTTP request data.
class Request final {
 public:
  Request(const std::string& request_handler_id,
          const std::string& url,
          const std::string& method,
          const std::string& version,
          MHD_Connection* connection,
          ProtocolHandler* protocol_handler);
  ~Request();

  // Obtains the file descriptor containing data of uploaded file identified
  // by |file_id|.
  base::File GetFileData(int file_id);

  // Finishes the request and provides the reply data.
  base::File Complete(
      int32_t status_code,
      const std::vector<std::tuple<std::string, std::string>>& headers,
      int64_t in_data_size);

  // Helper function to provide the string data and mime type.
  bool Complete(
      int32_t status_code,
      const std::vector<std::tuple<std::string, std::string>>& headers,
      const std::string& mime_type,
      const std::string& data);

  // Returns the unique ID of this request (GUID).
  const std::string& GetID() const { return id_; }

  // Returns the unique ID of the request handler this request is processed by
  // (GUID).
  const std::string& GetRequestHandlerID() const { return request_handler_id_; }

  // Returns the unique ID of the protocol handler this request is received
  // from (GUID or "http"/"https" for the two default handlers).
  const std::string& GetProtocolHandlerID() const;

  // Returns the object path of the HTTP request (e.g. "/privet/info").
  const std::string& GetURL() const { return url_; }

  // Returns the request method (e.g. "GET", "POST", ...).
  const std::string& GetMethod() const { return method_; }

  // Returns the output end of the body data stream pipe. The file descriptor
  // is owned by the caller and must be closed when no longer needed.
  // The pipe will contain the request body data, or no data if the request
  // had no body or a POST request has been parsed into form data.
  // In case there is no data, the file descriptor will represent a closed pipe,
  // so reading from it will just indicate end-of-file (no data to read).
  int GetBodyDataFileDescriptor() const;

  // Returns the POST form field data.
  const std::vector<PairOfStrings>& GetDataPost() const { return post_data_; }

  // Returns query parameters specified on the URL (as in "?param=value").
  const std::vector<PairOfStrings>& GetDataGet() const { return get_data_; }

  // Returns the information about any files uploaded as part of POST request.
  const std::vector<std::unique_ptr<FileInfo>>& GetFileInfo() const {
    return file_info_;
  }

  // Returns the HTTP request headers.
  const std::vector<PairOfStrings>& GetHeaders() const { return headers_; }

 private:
  friend class RequestHelper;
  friend class ServerHelper;

  // Helper methods for processing request data coming from the raw HTTP
  // connection.
  // Helper callback methods used by ProtocolHandler's ConnectionHandler to
  // transfer request headers and data to the Request object.
  bool BeginRequestData();
  bool AddRequestData(const void* data, size_t* size);
  void EndRequestData();

  // Callback for libmicrohttpd's PostProcessor.
  bool ProcessPostData(const char* key,
                       const char* filename,
                       const char* content_type,
                       const char* transfer_encoding,
                       const char* data,
                       uint64_t off,
                       size_t size);

  // These methods parse the request headers and data so they can be accessed
  // by request handlers later.
  // AddRawRequestData takes the amount of data to write in |*size|. On output,
  // |*size| contains the number of bytes REMAINING to be written, or 0 if all
  // data have been written successfully.
  bool AddRawRequestData(const void* data, size_t* size);
  bool AddPostFieldData(const char* key,
                        const char* filename,
                        const char* content_type,
                        const char* transfer_encoding,
                        const char* data,
                        size_t size);
  bool AppendPostFieldData(const char* key, const char* data, size_t size);

  // Callback to be called when data can be written to the output pipe again.
  void OnPipeAvailable(brillo::Stream::AccessMode mode);

  // Forwards the request to the request handler.
  void ForwardRequestToHandler();

  // Response data callback for MHD_create_response_from_callback().
  static ssize_t ResponseDataCallback(void* cls, uint64_t pos, char* buf,
                                      size_t max);

  TempFileManager* GetTempFileManager();

  std::string id_;
  std::string request_handler_id_;
  std::string url_;
  std::string method_;
  std::string version_;
  MHD_Connection* connection_{nullptr};
  MHD_PostProcessor* post_processor_{nullptr};
  // Data pipe for request body data (output/read end of the pipe).
  base::File request_data_pipe_out_;
  // Data stream for the input/write end of the request data pipe.
  brillo::StreamPtr request_data_stream_;

  bool last_posted_data_was_file_{false};
  bool request_forwarded_{false};
  bool request_data_finished_{false};
  bool response_data_started_{false};
  bool response_data_finished_{false};
  bool waiting_for_data_{false};

  std::vector<PairOfStrings> post_data_;
  std::vector<PairOfStrings> get_data_;
  std::vector<std::unique_ptr<FileInfo>> file_info_;
  std::vector<PairOfStrings> headers_;

  int response_status_code_{0};
  // Data size of response, -1 if unknown.
  int64_t response_data_size_{-1};
  // Data stream for the output/read end of the response data pipe.
  brillo::StreamPtr response_data_stream_;
  std::vector<PairOfStrings> response_headers_;
  ProtocolHandler* protocol_handler_;

  base::WeakPtrFactory<Request> weak_ptr_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(Request);
};

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_REQUEST_H_
