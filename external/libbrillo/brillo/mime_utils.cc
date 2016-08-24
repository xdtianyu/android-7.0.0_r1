// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/mime_utils.h>

#include <algorithm>
#include <base/strings/string_util.h>
#include <brillo/strings/string_utils.h>

namespace brillo {

// ***************************************************************************
// ******************************* MIME types ********************************
// ***************************************************************************
const char mime::types::kApplication[]             = "application";
const char mime::types::kAudio[]                   = "audio";
const char mime::types::kImage[]                   = "image";
const char mime::types::kMessage[]                 = "message";
const char mime::types::kMultipart[]               = "multipart";
const char mime::types::kText[]                    = "text";
const char mime::types::kVideo[]                   = "video";

const char mime::parameters::kCharset[]            = "charset";

const char mime::image::kJpeg[]                    = "image/jpeg";
const char mime::image::kPng[]                     = "image/png";
const char mime::image::kBmp[]                     = "image/bmp";
const char mime::image::kTiff[]                    = "image/tiff";
const char mime::image::kGif[]                     = "image/gif";

const char mime::text::kPlain[]                    = "text/plain";
const char mime::text::kHtml[]                     = "text/html";
const char mime::text::kXml[]                      = "text/xml";

const char mime::application::kOctet_stream[]      = "application/octet-stream";
const char mime::application::kJson[]              = "application/json";
const char mime::application::kWwwFormUrlEncoded[] =
    "application/x-www-form-urlencoded";
const char mime::application::kProtobuf[]          = "application/x-protobuf";

const char mime::multipart::kFormData[]            = "multipart/form-data";
const char mime::multipart::kMixed[]               = "multipart/mixed";

// ***************************************************************************
// **************************** Utility Functions ****************************
// ***************************************************************************
static std::string EncodeParam(const std::string& param) {
  // If the string contains one of "tspecials" characters as
  // specified in RFC 1521, enclose it in quotes.
  if (param.find_first_of("()<>@,;:\\\"/[]?=") != std::string::npos) {
    return '"' + param + '"';
  }
  return param;
}

static std::string DecodeParam(const std::string& param) {
  if (param.size() > 1 && param.front() == '"' && param.back() == '"') {
    return param.substr(1, param.size() - 2);
  }
  return param;
}

// ***************************************************************************
// ******************** Main MIME manipulation functions *********************
// ***************************************************************************

bool mime::Split(const std::string& mime_string,
                 std::string* type,
                 std::string* subtype,
                 mime::Parameters* parameters) {
  std::vector<std::string> parts =
      brillo::string_utils::Split(mime_string, ";");
  if (parts.empty())
    return false;

  if (!mime::Split(parts.front(), type, subtype))
    return false;

  if (parameters) {
    parameters->clear();
    parameters->reserve(parts.size() - 1);
    for (size_t i = 1; i < parts.size(); i++) {
      auto pair = brillo::string_utils::SplitAtFirst(parts[i], "=");
      pair.second = DecodeParam(pair.second);
      parameters->push_back(pair);
    }
  }
  return true;
}

bool mime::Split(const std::string& mime_string,
                 std::string* type,
                 std::string* subtype) {
  std::string mime = mime::RemoveParameters(mime_string);
  auto types = brillo::string_utils::SplitAtFirst(mime, "/");

  if (type)
    *type = types.first;

  if (subtype)
    *subtype = types.second;

  return !types.first.empty() && !types.second.empty();
}

std::string mime::Combine(const std::string& type,
                          const std::string& subtype,
                          const mime::Parameters& parameters) {
  std::vector<std::string> parts;
  parts.push_back(brillo::string_utils::Join("/", type, subtype));
  for (const auto& pair : parameters) {
    parts.push_back(
        brillo::string_utils::Join("=", pair.first, EncodeParam(pair.second)));
  }
  return brillo::string_utils::Join("; ", parts);
}

std::string mime::GetType(const std::string& mime_string) {
  std::string mime = mime::RemoveParameters(mime_string);
  return brillo::string_utils::SplitAtFirst(mime, "/").first;
}

std::string mime::GetSubtype(const std::string& mime_string) {
  std::string mime = mime::RemoveParameters(mime_string);
  return brillo::string_utils::SplitAtFirst(mime, "/").second;
}

mime::Parameters mime::GetParameters(const std::string& mime_string) {
  std::string type;
  std::string subtype;
  mime::Parameters parameters;

  if (mime::Split(mime_string, &type, &subtype, &parameters))
    return parameters;

  return mime::Parameters();
}

std::string mime::RemoveParameters(const std::string& mime_string) {
  return brillo::string_utils::SplitAtFirst(mime_string, ";").first;
}

std::string mime::AppendParameter(const std::string& mime_string,
                                  const std::string& paramName,
                                  const std::string& paramValue) {
  std::string mime(mime_string);
  mime += "; ";
  mime += brillo::string_utils::Join("=", paramName, EncodeParam(paramValue));
  return mime;
}

std::string mime::GetParameterValue(const std::string& mime_string,
                                    const std::string& paramName) {
  mime::Parameters params = mime::GetParameters(mime_string);
  for (const auto& pair : params) {
    if (base::EqualsCaseInsensitiveASCII(pair.first.c_str(), paramName.c_str()))
      return pair.second;
  }
  return std::string();
}

}  // namespace brillo
