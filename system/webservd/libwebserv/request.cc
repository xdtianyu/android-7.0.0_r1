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

#include <libwebserv/request_impl.h>

#include <base/callback.h>
#include <brillo/http/http_utils.h>
#include <brillo/streams/file_stream.h>

#include <libwebserv/dbus_protocol_handler.h>

namespace libwebserv {

FileInfo::FileInfo(DBusProtocolHandler* handler,
                   int file_id,
                   const std::string& request_id,
                   const std::string& file_name,
                   const std::string& content_type,
                   const std::string& transfer_encoding)
    : handler_{handler},
      file_id_{file_id},
      request_id_{request_id},
      file_name_(file_name),
      content_type_(content_type),
      transfer_encoding_(transfer_encoding) {
}

void FileInfo::GetData(
    const base::Callback<void(brillo::StreamPtr)>& success_callback,
    const base::Callback<void(brillo::Error*)>& error_callback) const {
  handler_->GetFileData(request_id_,
                        file_id_,
                        success_callback,
                        error_callback);
}

RequestImpl::RequestImpl(DBusProtocolHandler* handler,
                         const std::string& url,
                         const std::string& method)
    : Request{url, method}, handler_{handler} {
}

brillo::StreamPtr RequestImpl::GetDataStream() {
  return brillo::FileStream::FromFileDescriptor(
      raw_data_fd_.GetPlatformFile(), false, nullptr);
}

std::vector<PairOfStrings> Request::GetFormData() const {
  auto data = GetFormDataGet();
  auto post_data = GetFormDataPost();
  data.insert(data.end(), post_data.begin(), post_data.end());
  return data;
}

std::vector<PairOfStrings> Request::GetFormDataGet() const {
  return std::vector<PairOfStrings>{get_data_.begin(), get_data_.end()};
}

std::vector<PairOfStrings> Request::GetFormDataPost() const {
  return std::vector<PairOfStrings>{post_data_.begin(), post_data_.end()};
}

std::vector<std::pair<std::string, const FileInfo*>> Request::GetFiles() const {
  std::vector<std::pair<std::string, const FileInfo*>> data;
  data.reserve(file_info_.size());
  for (const auto& pair : file_info_) {
    data.emplace_back(pair.first, pair.second.get());
  }
  return data;
}

std::vector<std::string> Request::GetFormField(const std::string& name) const {
  std::vector<std::string> data;
  auto pair = get_data_.equal_range(name);
  while (pair.first != pair.second) {
    data.push_back(pair.first->second);
    ++pair.first;
  }
  pair = post_data_.equal_range(name);
  while (pair.first != pair.second) {
    data.push_back(pair.first->second);
    ++pair.first;
  }
  return data;
}

std::vector<std::string> Request::GetFormFieldPost(
    const std::string& name) const {
  std::vector<std::string> data;
  auto pair = post_data_.equal_range(name);
  while (pair.first != pair.second) {
    data.push_back(pair.first->second);
    ++pair.first;
  }
  return data;
}

std::vector<std::string> Request::GetFormFieldGet(
    const std::string& name) const {
  std::vector<std::string> data;
  auto pair = get_data_.equal_range(name);
  while (pair.first != pair.second) {
    data.push_back(pair.first->second);
    ++pair.first;
  }
  return data;
}

std::vector<const FileInfo*> Request::GetFileInfo(
    const std::string& name) const {
  std::vector<const FileInfo*> data;
  auto pair = file_info_.equal_range(name);
  while (pair.first != pair.second) {
    data.push_back(pair.first->second.get());
    ++pair.first;
  }
  return data;
}

std::vector<PairOfStrings> Request::GetHeaders() const {
  return std::vector<PairOfStrings>{headers_.begin(), headers_.end()};
}

std::vector<std::string> Request::GetHeader(const std::string& name) const {
  std::vector<std::string> data;
  auto range =
      headers_.equal_range(brillo::http::GetCanonicalHeaderName(name));
  while (range.first != range.second) {
    data.push_back(range.first->second);
    ++range.first;
  }
  return data;
}

std::string Request::GetFirstHeader(const std::string& name) const {
  auto p = headers_.find(brillo::http::GetCanonicalHeaderName(name));
  return (p != headers_.end()) ? p->second : std::string{};
}


}  // namespace libwebserv
