// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBBRILLO_BRILLO_HTTP_CURL_API_H_
#define LIBBRILLO_BRILLO_HTTP_CURL_API_H_

#include <curl/curl.h>

#include <string>

#include <base/macros.h>
#include <brillo/brillo_export.h>

namespace brillo {
namespace http {

// Abstract wrapper around libcurl C API that allows us to mock it out in tests.
class CurlInterface {
 public:
  CurlInterface() = default;
  virtual ~CurlInterface() = default;

  // Wrapper around curl_easy_init().
  virtual CURL* EasyInit() = 0;

  // Wrapper around curl_easy_cleanup().
  virtual void EasyCleanup(CURL* curl) = 0;

  // Wrappers around curl_easy_setopt().
  virtual CURLcode EasySetOptInt(CURL* curl, CURLoption option, int value) = 0;
  virtual CURLcode EasySetOptStr(CURL* curl,
                                 CURLoption option,
                                 const std::string& value) = 0;
  virtual CURLcode EasySetOptPtr(CURL* curl,
                                 CURLoption option,
                                 void* value) = 0;
  virtual CURLcode EasySetOptCallback(CURL* curl,
                                      CURLoption option,
                                      intptr_t address) = 0;
  virtual CURLcode EasySetOptOffT(CURL* curl,
                                  CURLoption option,
                                  curl_off_t value) = 0;

  // A type-safe wrapper around function callback options.
  template<typename R, typename... Args>
  inline CURLcode EasySetOptCallback(CURL* curl,
                                     CURLoption option,
                                     R(*callback)(Args...)) {
    return EasySetOptCallback(
        curl, option, reinterpret_cast<intptr_t>(callback));
  }

  // Wrapper around curl_easy_perform().
  virtual CURLcode EasyPerform(CURL* curl) = 0;

  // Wrappers around curl_easy_getinfo().
  virtual CURLcode EasyGetInfoInt(CURL* curl,
                                  CURLINFO info,
                                  int* value) const = 0;
  virtual CURLcode EasyGetInfoDbl(CURL* curl,
                                  CURLINFO info,
                                  double* value) const = 0;
  virtual CURLcode EasyGetInfoStr(CURL* curl,
                                  CURLINFO info,
                                  std::string* value) const = 0;
  virtual CURLcode EasyGetInfoPtr(CURL* curl,
                                  CURLINFO info,
                                  void** value) const = 0;

  // Wrapper around curl_easy_strerror().
  virtual std::string EasyStrError(CURLcode code) const = 0;

  // Wrapper around curl_multi_init().
  virtual CURLM* MultiInit() = 0;

  // Wrapper around curl_multi_cleanup().
  virtual CURLMcode MultiCleanup(CURLM* multi_handle) = 0;

  // Wrapper around curl_multi_info_read().
  virtual CURLMsg* MultiInfoRead(CURLM* multi_handle, int* msgs_in_queue) = 0;

  // Wrapper around curl_multi_add_handle().
  virtual CURLMcode MultiAddHandle(CURLM* multi_handle, CURL* curl_handle) = 0;

  // Wrapper around curl_multi_remove_handle().
  virtual CURLMcode MultiRemoveHandle(CURLM* multi_handle,
                                      CURL* curl_handle) = 0;

  // Wrapper around curl_multi_setopt(CURLMOPT_SOCKETFUNCTION/SOCKETDATA).
  virtual CURLMcode MultiSetSocketCallback(
      CURLM* multi_handle,
      curl_socket_callback socket_callback,
      void* userp) = 0;

  // Wrapper around curl_multi_setopt(CURLMOPT_TIMERFUNCTION/TIMERDATA).
  virtual CURLMcode MultiSetTimerCallback(
      CURLM* multi_handle,
      curl_multi_timer_callback timer_callback,
      void* userp) = 0;

  // Wrapper around curl_multi_assign().
  virtual CURLMcode MultiAssign(CURLM* multi_handle,
                                curl_socket_t sockfd,
                                void* sockp) = 0;

  // Wrapper around curl_multi_socket_action().
  virtual CURLMcode MultiSocketAction(CURLM* multi_handle,
                                      curl_socket_t s,
                                      int ev_bitmask,
                                      int* running_handles) = 0;

  // Wrapper around curl_multi_strerror().
  virtual std::string MultiStrError(CURLMcode code) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CurlInterface);
};

class BRILLO_EXPORT CurlApi : public CurlInterface {
 public:
  CurlApi();
  ~CurlApi() override;

  // Wrapper around curl_easy_init().
  CURL* EasyInit() override;

  // Wrapper around curl_easy_cleanup().
  void EasyCleanup(CURL* curl) override;

  // Wrappers around curl_easy_setopt().
  CURLcode EasySetOptInt(CURL* curl, CURLoption option, int value) override;
  CURLcode EasySetOptStr(CURL* curl,
                         CURLoption option,
                         const std::string& value) override;
  CURLcode EasySetOptPtr(CURL* curl, CURLoption option, void* value) override;
  CURLcode EasySetOptCallback(CURL* curl,
                              CURLoption option,
                              intptr_t address) override;
  CURLcode EasySetOptOffT(CURL* curl,
                          CURLoption option,
                          curl_off_t value) override;

  // Wrapper around curl_easy_perform().
  CURLcode EasyPerform(CURL* curl) override;

  // Wrappers around curl_easy_getinfo().
  CURLcode EasyGetInfoInt(CURL* curl, CURLINFO info, int* value) const override;
  CURLcode EasyGetInfoDbl(CURL* curl,
                          CURLINFO info,
                          double* value) const override;
  CURLcode EasyGetInfoStr(CURL* curl,
                          CURLINFO info,
                          std::string* value) const override;
  CURLcode EasyGetInfoPtr(CURL* curl,
                          CURLINFO info,
                          void** value) const override;

  // Wrapper around curl_easy_strerror().
  std::string EasyStrError(CURLcode code) const override;

  // Wrapper around curl_multi_init().
  CURLM* MultiInit() override;

  // Wrapper around curl_multi_cleanup().
  CURLMcode MultiCleanup(CURLM* multi_handle) override;

  // Wrapper around curl_multi_info_read().
  CURLMsg* MultiInfoRead(CURLM* multi_handle, int* msgs_in_queue) override;

  // Wrapper around curl_multi_add_handle().
  CURLMcode MultiAddHandle(CURLM* multi_handle, CURL* curl_handle) override;

  // Wrapper around curl_multi_remove_handle().
  CURLMcode MultiRemoveHandle(CURLM* multi_handle, CURL* curl_handle) override;

  // Wrapper around curl_multi_setopt(CURLMOPT_SOCKETFUNCTION/SOCKETDATA).
  CURLMcode MultiSetSocketCallback(
      CURLM* multi_handle,
      curl_socket_callback socket_callback,
      void* userp) override;

  // Wrapper around curl_multi_setopt(CURLMOPT_TIMERFUNCTION/TIMERDATA).
  CURLMcode MultiSetTimerCallback(
      CURLM* multi_handle,
      curl_multi_timer_callback timer_callback,
      void* userp) override;

  // Wrapper around curl_multi_assign().
  CURLMcode MultiAssign(CURLM* multi_handle,
                        curl_socket_t sockfd,
                        void* sockp) override;

  // Wrapper around curl_multi_socket_action().
  CURLMcode MultiSocketAction(CURLM* multi_handle,
                              curl_socket_t s,
                              int ev_bitmask,
                              int* running_handles) override;

  // Wrapper around curl_multi_strerror().
  std::string MultiStrError(CURLMcode code) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CurlApi);
};

}  // namespace http
}  // namespace brillo

#endif  // LIBBRILLO_BRILLO_HTTP_CURL_API_H_
