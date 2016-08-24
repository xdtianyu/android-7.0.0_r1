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

#ifndef WEBSERVER_LIBWEBSERV_REQUEST_FAKE_H_
#define WEBSERVER_LIBWEBSERV_REQUEST_FAKE_H_

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
#include <libwebserv/request.h>

namespace libwebserv {

// Implementation of Request that allows custom data for testing.
class RequestFake : public Request {
 public:
  RequestFake(const std::string& url, const std::string& method)
    : Request(url, method) {}

  void SetDataStream(brillo::StreamPtr data_stream) {
    data_stream_ = std::move(data_stream);
  }

  void SetFormDataPost(std::multimap<std::string, std::string> post_data) {
    post_data_ = std::move(post_data);
  }

  void SetFormDataGet(std::multimap<std::string, std::string> get_data) {
    get_data_ = std::move(get_data);
  }

  void SetFileInfo(
      std::multimap<std::string, std::unique_ptr<FileInfo>> file_info) {
    file_info_ = std::move(file_info);
  }

  void SetHeaders(std::multimap<std::string, std::string> headers) {
    headers_ = std::move(headers);
  }

  // Overrides from Request.
  brillo::StreamPtr GetDataStream() override { return std::move(data_stream_); }

 private:
  brillo::StreamPtr data_stream_;

  DISALLOW_COPY_AND_ASSIGN(RequestFake);
};

}  // namespace libwebserv

#endif  // WEBSERVER_LIBWEBSERV_REQUEST_FAKE_H_
