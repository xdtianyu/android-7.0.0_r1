// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/http/curl_api.h>

#include <base/logging.h>

namespace brillo {
namespace http {

namespace {

static_assert(CURLOPTTYPE_LONG == 0 &&
              CURLOPTTYPE_OBJECTPOINT == 10000 &&
              CURLOPTTYPE_FUNCTIONPOINT == 20000 &&
              CURLOPTTYPE_OFF_T == 30000,
              "CURL option types are expected to be multiples of 10000");

inline bool VerifyOptionType(CURLoption option, int expected_type) {
  int option_type = (static_cast<int>(option) / 10000) * 10000;
  return (option_type == expected_type);
}

}  // anonymous namespace

CurlApi::CurlApi() {
  curl_global_init(CURL_GLOBAL_ALL);
}

CurlApi::~CurlApi() {
  curl_global_cleanup();
}

CURL* CurlApi::EasyInit() {
  return curl_easy_init();
}

void CurlApi::EasyCleanup(CURL* curl) {
  curl_easy_cleanup(curl);
}

CURLcode CurlApi::EasySetOptInt(CURL* curl, CURLoption option, int value) {
  CHECK(VerifyOptionType(option, CURLOPTTYPE_LONG))
      << "Only options that expect a LONG data type must be specified here";
  // CURL actually uses "long" type, so have to make sure we feed it what it
  // expects.
  // NOLINTNEXTLINE(runtime/int)
  return curl_easy_setopt(curl, option, static_cast<long>(value));
}

CURLcode CurlApi::EasySetOptStr(CURL* curl,
                                CURLoption option,
                                const std::string& value) {
  CHECK(VerifyOptionType(option, CURLOPTTYPE_OBJECTPOINT))
      << "Only options that expect a STRING data type must be specified here";
  return curl_easy_setopt(curl, option, value.c_str());
}

CURLcode CurlApi::EasySetOptPtr(CURL* curl, CURLoption option, void* value) {
  CHECK(VerifyOptionType(option, CURLOPTTYPE_OBJECTPOINT))
      << "Only options that expect a pointer data type must be specified here";
  return curl_easy_setopt(curl, option, value);
}

CURLcode CurlApi::EasySetOptCallback(CURL* curl,
                                     CURLoption option,
                                     intptr_t address) {
  CHECK(VerifyOptionType(option, CURLOPTTYPE_FUNCTIONPOINT))
      << "Only options that expect a function pointers must be specified here";
  return curl_easy_setopt(curl, option, address);
}

CURLcode CurlApi::EasySetOptOffT(CURL* curl,
                                 CURLoption option,
                                 curl_off_t value) {
  CHECK(VerifyOptionType(option, CURLOPTTYPE_OFF_T))
      << "Only options that expect a large data size must be specified here";
  return curl_easy_setopt(curl, option, value);
}

CURLcode CurlApi::EasyPerform(CURL* curl) {
  return curl_easy_perform(curl);
}

CURLcode CurlApi::EasyGetInfoInt(CURL* curl, CURLINFO info, int* value) const {
  CHECK_EQ(CURLINFO_LONG, info & CURLINFO_TYPEMASK) << "Wrong option type";
  long data = 0;  // NOLINT(runtime/int) - curl expects a long here.
  CURLcode code = curl_easy_getinfo(curl, info, &data);
  if (code == CURLE_OK)
    *value = static_cast<int>(data);
  return code;
}

CURLcode CurlApi::EasyGetInfoDbl(CURL* curl,
                                 CURLINFO info,
                                 double* value) const {
  CHECK_EQ(CURLINFO_DOUBLE, info & CURLINFO_TYPEMASK) << "Wrong option type";
  return curl_easy_getinfo(curl, info, value);
}

CURLcode CurlApi::EasyGetInfoStr(CURL* curl,
                                 CURLINFO info,
                                 std::string* value) const {
  CHECK_EQ(CURLINFO_STRING, info & CURLINFO_TYPEMASK) << "Wrong option type";
  char* data = nullptr;
  CURLcode code = curl_easy_getinfo(curl, info, &data);
  if (code == CURLE_OK)
    *value = data;
  return code;
}

CURLcode CurlApi::EasyGetInfoPtr(CURL* curl,
                                 CURLINFO info,
                                 void** value) const {
  // CURL uses "string" type for generic pointer info. Go figure.
  CHECK_EQ(CURLINFO_STRING, info & CURLINFO_TYPEMASK) << "Wrong option type";
  return curl_easy_getinfo(curl, info, value);
}

std::string CurlApi::EasyStrError(CURLcode code) const {
  return curl_easy_strerror(code);
}

CURLM* CurlApi::MultiInit() {
  return curl_multi_init();
}

CURLMcode CurlApi::MultiCleanup(CURLM* multi_handle) {
  return curl_multi_cleanup(multi_handle);
}

CURLMsg* CurlApi::MultiInfoRead(CURLM* multi_handle, int* msgs_in_queue) {
  return curl_multi_info_read(multi_handle, msgs_in_queue);
}

CURLMcode CurlApi::MultiAddHandle(CURLM* multi_handle, CURL* curl_handle) {
  return curl_multi_add_handle(multi_handle, curl_handle);
}

CURLMcode CurlApi::MultiRemoveHandle(CURLM* multi_handle, CURL* curl_handle) {
  return curl_multi_remove_handle(multi_handle, curl_handle);
}

CURLMcode CurlApi::MultiSetSocketCallback(CURLM* multi_handle,
                                          curl_socket_callback socket_callback,
                                          void* userp) {
  CURLMcode code =
      curl_multi_setopt(multi_handle, CURLMOPT_SOCKETFUNCTION, socket_callback);
  if (code != CURLM_OK)
    return code;
  return curl_multi_setopt(multi_handle, CURLMOPT_SOCKETDATA, userp);
}

CURLMcode CurlApi::MultiSetTimerCallback(
    CURLM* multi_handle,
    curl_multi_timer_callback timer_callback,
    void* userp) {
  CURLMcode code =
      curl_multi_setopt(multi_handle, CURLMOPT_TIMERFUNCTION, timer_callback);
  if (code != CURLM_OK)
    return code;
  return curl_multi_setopt(multi_handle, CURLMOPT_TIMERDATA, userp);
}

CURLMcode CurlApi::MultiAssign(CURLM* multi_handle,
                               curl_socket_t sockfd,
                               void* sockp) {
  return curl_multi_assign(multi_handle, sockfd, sockp);
}

CURLMcode CurlApi::MultiSocketAction(CURLM* multi_handle,
                                     curl_socket_t s,
                                     int ev_bitmask,
                                     int* running_handles) {
  return curl_multi_socket_action(multi_handle, s, ev_bitmask, running_handles);
}

std::string CurlApi::MultiStrError(CURLMcode code) const {
  return curl_multi_strerror(code);
}

}  // namespace http
}  // namespace brillo
