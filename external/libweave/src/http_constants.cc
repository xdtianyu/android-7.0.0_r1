// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/http_constants.h"

#include <weave/enum_to_string.h>
#include <weave/provider/http_client.h>

namespace weave {
namespace http {

const char kAuthorization[] = "Authorization";
const char kContentType[] = "Content-Type";

const char kJson[] = "application/json";
const char kJsonUtf8[] = "application/json; charset=utf-8";
const char kPlain[] = "text/plain";
const char kWwwFormUrlEncoded[] = "application/x-www-form-urlencoded";

}  // namespace http

using provider::HttpClient;

namespace {

const weave::EnumToStringMap<HttpClient::Method>::Map kMapMethod[] = {
    {HttpClient::Method::kGet, "GET"},
    {HttpClient::Method::kPost, "POST"},
    {HttpClient::Method::kPut, "PUT"},
    {HttpClient::Method::kPatch, "PATCH"}};

}  // namespace

template <>
LIBWEAVE_EXPORT EnumToStringMap<HttpClient::Method>::EnumToStringMap()
    : EnumToStringMap(kMapMethod) {}

}  // namespace weave
