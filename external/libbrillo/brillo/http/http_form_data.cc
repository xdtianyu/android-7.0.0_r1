// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/http/http_form_data.h>

#include <limits>

#include <base/format_macros.h>
#include <base/rand_util.h>
#include <base/strings/stringprintf.h>

#include <brillo/errors/error_codes.h>
#include <brillo/http/http_transport.h>
#include <brillo/mime_utils.h>
#include <brillo/streams/file_stream.h>
#include <brillo/streams/input_stream_set.h>
#include <brillo/streams/memory_stream.h>

namespace brillo {
namespace http {

namespace form_header {
const char kContentDisposition[] = "Content-Disposition";
const char kContentTransferEncoding[] = "Content-Transfer-Encoding";
const char kContentType[] = "Content-Type";
}  // namespace form_header

const char content_disposition::kFile[] = "file";
const char content_disposition::kFormData[] = "form-data";

FormField::FormField(const std::string& name,
                     const std::string& content_disposition,
                     const std::string& content_type,
                     const std::string& transfer_encoding)
    : name_{name},
      content_disposition_{content_disposition},
      content_type_{content_type},
      transfer_encoding_{transfer_encoding} {
}

std::string FormField::GetContentDisposition() const {
  std::string disposition = content_disposition_;
  if (!name_.empty())
    base::StringAppendF(&disposition, "; name=\"%s\"", name_.c_str());
  return disposition;
}

std::string FormField::GetContentType() const {
  return content_type_;
}

std::string FormField::GetContentHeader() const {
  HeaderList headers{
      {form_header::kContentDisposition, GetContentDisposition()}
  };

  if (!content_type_.empty())
    headers.emplace_back(form_header::kContentType, GetContentType());

  if (!transfer_encoding_.empty()) {
    headers.emplace_back(form_header::kContentTransferEncoding,
                         transfer_encoding_);
  }

  std::string result;
  for (const auto& pair : headers) {
    base::StringAppendF(
        &result, "%s: %s\r\n", pair.first.c_str(), pair.second.c_str());
  }
  result += "\r\n";
  return result;
}

TextFormField::TextFormField(const std::string& name,
                             const std::string& data,
                             const std::string& content_type,
                             const std::string& transfer_encoding)
    : FormField{name,
                content_disposition::kFormData,
                content_type,
                transfer_encoding},
      data_{data} {
}

bool TextFormField::ExtractDataStreams(std::vector<StreamPtr>* streams) {
  streams->push_back(MemoryStream::OpenCopyOf(data_, nullptr));
  return true;
}

FileFormField::FileFormField(const std::string& name,
                             StreamPtr stream,
                             const std::string& file_name,
                             const std::string& content_disposition,
                             const std::string& content_type,
                             const std::string& transfer_encoding)
    : FormField{name, content_disposition, content_type, transfer_encoding},
      stream_{std::move(stream)},
      file_name_{file_name} {
}

std::string FileFormField::GetContentDisposition() const {
  std::string disposition = FormField::GetContentDisposition();
  base::StringAppendF(&disposition, "; filename=\"%s\"", file_name_.c_str());
  return disposition;
}

bool FileFormField::ExtractDataStreams(std::vector<StreamPtr>* streams) {
  if (!stream_)
    return false;
  streams->push_back(std::move(stream_));
  return true;
}

MultiPartFormField::MultiPartFormField(const std::string& name,
                                       const std::string& content_type,
                                       const std::string& boundary)
    : FormField{name,
                content_disposition::kFormData,
                content_type.empty() ? mime::multipart::kMixed : content_type,
                {}},
      boundary_{boundary} {
  if (boundary_.empty())
    boundary_ = base::StringPrintf("%016" PRIx64, base::RandUint64());
}

bool MultiPartFormField::ExtractDataStreams(std::vector<StreamPtr>* streams) {
  for (auto& part : parts_) {
    std::string data = GetBoundaryStart() + part->GetContentHeader();
    streams->push_back(MemoryStream::OpenCopyOf(data, nullptr));
    if (!part->ExtractDataStreams(streams))
      return false;

    streams->push_back(MemoryStream::OpenRef("\r\n", nullptr));
  }
  if (!parts_.empty()) {
    std::string data = GetBoundaryEnd();
    streams->push_back(MemoryStream::OpenCopyOf(data, nullptr));
  }
  return true;
}

std::string MultiPartFormField::GetContentType() const {
  return base::StringPrintf(
      "%s; boundary=\"%s\"", content_type_.c_str(), boundary_.c_str());
}

void MultiPartFormField::AddCustomField(std::unique_ptr<FormField> field) {
  parts_.push_back(std::move(field));
}

void MultiPartFormField::AddTextField(const std::string& name,
                                      const std::string& data) {
  AddCustomField(std::unique_ptr<FormField>{new TextFormField{name, data}});
}

bool MultiPartFormField::AddFileField(const std::string& name,
                                      const base::FilePath& file_path,
                                      const std::string& content_disposition,
                                      const std::string& content_type,
                                      brillo::ErrorPtr* error) {
  StreamPtr stream = FileStream::Open(file_path, Stream::AccessMode::READ,
                                      FileStream::Disposition::OPEN_EXISTING,
                                      error);
  if (!stream)
    return false;
  std::string file_name = file_path.BaseName().value();
  std::unique_ptr<FormField> file_field{new FileFormField{name,
                                                          std::move(stream),
                                                          file_name,
                                                          content_disposition,
                                                          content_type,
                                                          "binary"}};
  AddCustomField(std::move(file_field));
  return true;
}

std::string MultiPartFormField::GetBoundaryStart() const {
  return base::StringPrintf("--%s\r\n", boundary_.c_str());
}

std::string MultiPartFormField::GetBoundaryEnd() const {
  return base::StringPrintf("--%s--", boundary_.c_str());
}

FormData::FormData() : FormData{std::string{}} {
}

FormData::FormData(const std::string& boundary)
    : form_data_{"", mime::multipart::kFormData, boundary} {
}

void FormData::AddCustomField(std::unique_ptr<FormField> field) {
  form_data_.AddCustomField(std::move(field));
}

void FormData::AddTextField(const std::string& name, const std::string& data) {
  form_data_.AddTextField(name, data);
}

bool FormData::AddFileField(const std::string& name,
                            const base::FilePath& file_path,
                            const std::string& content_type,
                            brillo::ErrorPtr* error) {
  return form_data_.AddFileField(
      name, file_path, content_disposition::kFormData, content_type, error);
}

std::string FormData::GetContentType() const {
  return form_data_.GetContentType();
}

StreamPtr FormData::ExtractDataStream() {
  std::vector<StreamPtr> source_streams;
  if (form_data_.ExtractDataStreams(&source_streams))
    return InputStreamSet::Create(std::move(source_streams), nullptr);
  return {};
}

}  // namespace http
}  // namespace brillo
